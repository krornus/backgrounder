#include <assert.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "rpc.h"

#define arylen(x) (sizeof(x)/sizeof((x)[0]))

void rpc_close(rpc_t *rpc)
{
    for (unsigned i = 0; i < rpc->connidx; i++) {
        shutdown(rpc->connfd[i], SHUT_RDWR);
        close(rpc->connfd[i]);
    }

    close(rpc->sockfd);
    close(rpc->pollfd);

    memset(rpc, 0, sizeof(rpc_t));
}

int rpc_serve(const char *pathname, rpc_t *rpc)
{
    int rv, fd, poll;
    size_t pathlen;
    struct epoll_event ev;
    struct sockaddr_un saddr;

    memset(rpc, 0, sizeof(rpc_t));

    pathlen = strlen(pathname);
    if (pathlen >= sizeof(saddr.sun_path) - 1) {
        errno = ENOBUFS;
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));

    saddr.sun_family = AF_UNIX;
    /* we set sun_path[0] to be '\0' to make this be an
     * abstract pathname socket, i.e. it doesn't exist
     * on the filesystem. */
    memcpy(&saddr.sun_path[1], pathname, pathlen);

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0) {
        return -1;
    }

    rv = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (rv < 0) {
        close(fd);
        return -1;
    }

    rv = listen(fd, 64);
    if (rv < 0) {
        close(fd);
        return -1;
    }

    poll = epoll_create1(0);
    if (poll < 0) {
        close(fd);
        return -1;
    }

    rpc->sockfd = fd;
    rpc->pollfd = poll;

    ev.events = EPOLLIN;
    ev.data.fd = rpc->sockfd;

    /* poll the server after calling listen(), otherwise
     * HUP events will be generated. */
    rv = epoll_ctl(poll, EPOLL_CTL_ADD, rpc->sockfd, &ev);
    if (rv < 0) {
        rpc_close(rpc);
    }

    return 0;
}

static int rpc_accept(rpc_t *rpc)
{
    int rv, fd;
    struct epoll_event ev;
    struct sockaddr addr;
    socklen_t len;

    if (rpc->connidx >= arylen(rpc->connfd)) {
        /* not using ET mode for epoll, so we will just get
         * notified again later by epoll that there is a client
         * pending acceptance. */
        return 0;
    }

    len = sizeof(addr);
    fd = accept(rpc->sockfd, &addr, &len);
    if (fd < 0) {
        return -1;
    }

    rpc->connfd[rpc->connidx++] = fd;

    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = fd;

    rv = epoll_ctl(rpc->pollfd, EPOLL_CTL_ADD, fd, &ev);
    if (rv < 0) {
        return -1;
    }

    return 0;
}

static int rpc_recvmsg(int fd, char *buf, size_t *len)
{
    ssize_t rv;
    struct iovec io[1];
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char cbuf[sizeof(struct cmsghdr)];

    memset(&msg, 0, sizeof(msg));
    memset(io, 0, sizeof(io));
    memset(cbuf, 0, sizeof(cbuf));

    io[0].iov_base = buf;
    io[0].iov_len = *len;

    cmsg = (struct cmsghdr *)cbuf;

    msg.msg_iov = io;
    msg.msg_iovlen = arylen(io);
    msg.msg_control = cmsg;
    msg.msg_controllen = sizeof(cbuf);

    rv = recvmsg(fd, &msg, MSG_WAITALL);
    if (rv < 0) {
        return -1;
    } else if (cmsg->cmsg_len) {
        errno = ENOTSUP;
        err(errno, "ancillary data recieved from client");
    } else if (msg.msg_flags & MSG_TRUNC) {
        errno = ENOBUFS;
        *len = rv >= 0 ? (size_t)rv : 0;
        return -1;
    } else {
        *len = (size_t)rv;
        return 0;
    }
}

int rpc_send(const rpcmsg_t *msg)
{
    ssize_t rv;

    if (msg->len > SSIZE_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    rv = send(msg->fd, msg->buf, msg->len, 0);

    if (rv >= 0 && (size_t)rv != msg->len) {
        errno = EMSGSIZE;
        return -1;
    }

    return rv;
}

/*
 * handle a socket event, attempting to read a message
 * -1 on error,
 * 0 if no message was received,
 * 1 if a message was received
 */
int rpc_recv(rpc_t *rpc, int timeout, rpcmsg_t *msg)
{
    int rv;
    struct epoll_event ev[1];

    rv = epoll_wait(rpc->pollfd, ev, 1, timeout);
    if (rv <= 0) {
        return rv;
    }

    if (ev[0].data.fd == rpc->sockfd) {
        if (ev[0].events & EPOLLERR || ev[0].events & EPOLLHUP) {
            return -1;
        }

        if (ev[0].events & EPOLLIN) {
            return rpc_accept(rpc);
        } else {
            return 0;
        }
    } else {
        assert(rpc->connidx);

        rv = 0;
        msg->fd = ev[0].data.fd;

        if (ev[0].events & EPOLLERR) {
            return -1;
        } else if (ev[0].events & EPOLLIN) {
            /* we want to read from the fd before closing it,
             * so we must check for HUP after this. */
            if (rpc_recvmsg(ev[0].data.fd, msg->buf, &msg->len) < 0) {
                rv = -1;
            } else if (msg->len) {
                rv = 1;
            } else {
                rv = 0;
            }
        }

        if (ev[0].events & EPOLLHUP || ev[0].events & EPOLLRDHUP) {
            close(ev[0].data.fd);
            rpc->connidx--;
        }

        return rv;
    }
}

int rpc_client(const char *pathname)
{
    int rv, fd;
    size_t pathlen;
    struct sockaddr_un saddr;

    pathlen = strlen(pathname);
    if (pathlen >= sizeof(saddr.sun_path) - 1) {
        errno = ENOBUFS;
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));

    saddr.sun_family = AF_UNIX;
    memcpy(&saddr.sun_path[1], pathname, pathlen);

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0) {
        return -1;
    }

    rv = connect(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (rv < 0) {
        close(fd);
        return -1;
    } else {
        return fd;
    }
}
