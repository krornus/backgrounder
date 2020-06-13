#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include "uri.h"
#include "buf.h"

typedef struct remote remote_t;

static CURLM *g_multi;

struct remote {
    CURL *curl;
    buf_t *io;
    int alive;
};

struct uri {
    enum {
        URI_FILE,
        URI_CURL,
    } ty;
    union {
        FILE *fp;
        remote_t rem;
    } un;
};

static size_t remote_write(char *buf, size_t size, size_t nmemb, void *io)
{
    int rv;
    rv = buf_write(io, buf, size * nmemb);
    if (rv == 0) {
        return size * nmemb;
    } else {
        return 0;
    }
}

static int remote_read(URI *uri, size_t len)
{
    int maxfd;
    int rv;
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    struct timeval timeout;
    long curl_timeout;

    remote_t *rem;

    rem = &uri->un.rem;
    if (!rem->alive || buf_len(rem->io) >= len) {
        return 0;
    }

    /* read in a loop until select reports that
     * the read would block, or the length is
     * satisfied */
    do {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        /* set a suitable timeout to fail on */
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        /* get the timeout if set */
        curl_multi_timeout(g_multi, &curl_timeout);
        if (curl_timeout >= 0) {
            timeout.tv_sec = curl_timeout / 1000;
            if (timeout.tv_sec > 1) {
                timeout.tv_sec = 1;
            } else {
                timeout.tv_usec = (curl_timeout % 1000) * 1000;
            }
        }

        /* get file descriptors from the transfers */
        rv = curl_multi_fdset(g_multi,
                              &readfds,
                              &writefds,
                              &exceptfds,
                              &maxfd);

        if (rv != CURLM_OK) {
            return -1;
        }

        if (maxfd == -1) {
            /* sleep 100ms and loop */
            usleep(100000);
            rv = 0;
        } else {
            /* select on the fds */
            rv = select(maxfd + 1, &readfds, &writefds, &exceptfds, &timeout);
        }

        if (rv >= 0) {
            /* perform the read */
            curl_multi_perform(g_multi, &rem->alive);
        }
    } while(rv >= 0 && rem->alive && buf_len(rem->io) < len);

    return 0;
}

URI *uopen(const char *path, const char *op)
{
    URI *uri;

    if (strcmp(op, "r") != 0) {
        errno = EINVAL;
        return NULL;
    }

    uri = malloc(sizeof(URI));
    if (!uri) {
        return NULL;
    }

    uri->un.fp = fopen(path, op);
    if (uri->un.fp) {
        uri->ty = URI_FILE;
    } else {
        remote_t *rem;

        rem = &uri->un.rem;
        uri->ty = URI_CURL;
        uri->un.rem.curl = curl_easy_init();
        uri->un.rem.io = buf_new(12288);

        curl_easy_setopt(uri->un.rem.curl, CURLOPT_URL, path);
        curl_easy_setopt(uri->un.rem.curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(uri->un.rem.curl, CURLOPT_WRITEDATA, rem->io);
        curl_easy_setopt(uri->un.rem.curl, CURLOPT_WRITEFUNCTION, remote_write);

        if(!g_multi) {
            g_multi = curl_multi_init();
            if (!g_multi) {
                err(EXIT_FAILURE, "failed to initialize curl");
            }
        }

        if (curl_multi_add_handle(g_multi, rem->curl) != CURLM_OK) {
            buf_close(rem->io);
            free(uri);
            return NULL;
        }

        if (curl_multi_perform(g_multi, &rem->alive) != CURLM_OK) {
            buf_close(rem->io);
            curl_multi_remove_handle(g_multi, rem->curl);
            free(uri);
            return NULL;
        }

        if (!buf_len(rem->io) && !rem->alive) {
            errno = ENOENT;
            uclose(uri);
            return NULL;
        }
    }

    return uri;
}

void uclose(URI *uri)
{
    if (uri) {
        if (uri->ty == URI_CURL) {
            remote_t *rem;

            rem = &uri->un.rem;

            buf_close(rem->io);
            curl_multi_remove_handle(g_multi, rem->curl);
            curl_easy_cleanup(rem->curl);
        } else if (uri->ty == URI_FILE) {
            fclose(uri->un.fp);
        }
        free(uri);
    }
}

int uread(void *buf, size_t size, size_t nmemb, URI *uri)
{
    if (uri->ty == URI_CURL) {
        size_t len;
        remote_t *rem;

        rem = &uri->un.rem;
        len = size * nmemb;

        if (remote_read(uri, len) < 0) {
            err(EXIT_FAILURE, "remote_read()");
            return -1;
        }

        if (!buf_len(rem->io)) {
            return 0;
        }

        return buf_read(rem->io, buf, len) / size;
    } else if (uri->ty == URI_FILE) {
        return fread(buf, size, nmemb, uri->un.fp);
    } else {
        errno = EBADF;
        return -1;
    }
}
