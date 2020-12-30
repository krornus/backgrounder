#ifndef rpc_h_INCLUDED
#define rpc_h_INCLUDED

#define MAXCONN 128

typedef struct rpc rpc_t;
typedef struct rpcmsg rpcmsg_t;

struct rpc {
    int sockfd;
    int pollfd;
    int connfd[MAXCONN];
    unsigned connidx;
};

struct rpcmsg {
    int fd;
    char *buf;
    size_t len;
};

int rpc_serve(const char *pathname, rpc_t *rpc);
int rpc_client(const char *pathname);

int rpc_send(const rpcmsg_t *msg);
int rpc_recv(rpc_t *rpc, int timeout, rpcmsg_t *msg);

void rpc_close(rpc_t *rpc);

#endif // rpc_h_INCLUDED
