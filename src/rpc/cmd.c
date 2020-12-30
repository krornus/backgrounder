#include <err.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "parson.h"
#include "rpc.h"
#include "img.h"

#define MAXMSG 1024*1024
#define SOCKPATH "bkg:socket"

#define CMD_SET "set"
#define CMD_SET_PATH "path"
#define CMD_SET_MODE "mode"
#define CMD_SET_MODE_DEFAULT "center"
#define CMD_SET_BGCOLOR "background-color"
#define CMD_SET_DEST "destination"

enum {
    SUCCESS,
    EMSGLEN,
    ESYNTAX,
    ESCHEMA,
    EINTERN,
    EUNKNOWN,
    EARG,
};

/* allow for a null terminator */
static char msgbuf[MAXMSG+1];

static int vcmd_response(int fd, int code, const char *fmt, va_list ap)
{
    JSON_Value *json;
    JSON_Object *root;
    rpcmsg_t rpcmsg;
    char *message;

    if (vasprintf(&message, fmt, ap) < 0) {
        err(errno, "command response message formatting failed");
    }

    json = json_value_init_object();
    if (!json) {
        err(errno, "failed to initialize json object");
    }

    root = json_value_get_object(json);

    if (json_object_set_number(root, "status", code) == JSONFailure) {
        err(errno, "failed to set json status code");
    }

    if (json_object_set_string(root, "message", message) == JSONFailure) {
        err(errno, "failed to set json message");
    }

    rpcmsg.buf = json_serialize_to_string(json);
    if (!rpcmsg.buf) {
        err(errno, "failed to serialize json");
    }

    rpcmsg.len = strlen(rpcmsg.buf);
    rpcmsg.fd = fd;

    if (rpc_send(&rpcmsg) < 0) {
        warn("failed to respond to client: %s", rpcmsg.buf);
    }

    json_free_serialized_string(rpcmsg.buf);
    json_value_free(json);
    json = NULL;

    return code;
}

static int cmd_response(int fd, int code, const char *fmt, ...)
{
    int rv;
    va_list ap;

    va_start(ap, fmt);
    rv = vcmd_response(fd, code, fmt, ap);
    va_end(ap);

    return rv;
}

#include <sys/socket.h>

static int do_client(void)
{
    rpcmsg_t msg;
    char buf[1024];

    msg.fd = rpc_client("bkg:socket");

    if (msg.fd < 0) {
        err(errno, "failed to connect to server");
    }

    msg.len = fread(buf, sizeof(char), sizeof(buf), stdin);
    if (ferror(stdin)) {
        err(errno, "failed to read from stdin");
    }

    msg.buf = buf;

    if (rpc_send(&msg) < 0) {
        err(errno, "client send");
    }

    ssize_t rv;
    rv = recv(msg.fd, buf, sizeof(buf), 0);
    if (rv > 0) {
        printf("%.*s\n", (int)rv, buf);
    } else {
        err(errno, "???");
    }

    return 0;
}

static int cmd_set(int fd, const JSON_Object *cmd)
{
    int rv;
    int emode;
    const char *uri, *dest, *mode, *bgcolor;

    uri = json_object_get_string(cmd, CMD_SET_PATH);
    if (!uri) {
        return cmd_response(fd, EARG,
            "set: missing argument: '%s'",
            CMD_SET_PATH);
    }

    mode = json_object_get_string(cmd, CMD_SET_MODE);
    if (!mode) {
        mode = CMD_SET_MODE_DEFAULT;
    }

    if (strcmp(mode, "center") == 0) {
        emode = MODE_CENTER;
    } else if (strcmp(mode, "fill") == 0) {
        emode = MODE_FILL;
    } else if (strcmp(mode, "max") == 0) {
        emode = MODE_MAX;
    } else if (strcmp(mode, "scale") == 0) {
        emode = MODE_SCALE;
    } else {
        return cmd_response(fd, EARG,
            "set: invalid image-scaling mode: '%s'. "
            "expected one of {center, fill, max, scale}",
            CMD_SET_PATH);
    }

    bgcolor = json_object_get_string(cmd, CMD_SET_BGCOLOR);
    if (!bgcolor) {
        bgcolor = "black";
    }

    dest = json_object_get_string(cmd, CMD_SET_DEST);
    rv = 0;

    if (rv == 0) {
        return cmd_response(fd, SUCCESS, "set: success");
    } else {
        return cmd_response(fd, EARG, "set: %s: %s", uri, strerror(errno));
    }
}

static int exec_command(int fd, const JSON_Object *cmd)
{
    const char *name;

    name = json_object_get_string(cmd, "command");
    if (!name) {
        return cmd_response(fd, ESCHEMA,
            "invalid command: missing 'command' field");
    }

    if (strcmp(name, CMD_SET) == 0) {
        return cmd_set(fd, cmd);
    } else {
        return cmd_response(fd, EUNKNOWN,
            "unknown command: '%s'", name);
    }
}

static int exec_commands(int fd, const JSON_Array *commands)
{
    int rv;
    JSON_Object *cmd;

    for (size_t i = 0; i < json_array_get_count(commands); i++) {
        cmd = json_array_get_object(commands, i);
        rv = exec_command(fd, cmd);
        if (rv != 0) {
            return rv;
        }
    }

    return 0;
}

static int exec_command_string(int fd, const char *buf)
{
    int rv;
    JSON_Value *json;
    JSON_Object *root;
    JSON_Array *cmds;

    json = json_parse_string(buf);
    if (!json) {
        json_value_free(json);
        return cmd_response(fd, ESYNTAX, "invalid syntax");
    }

    root = json_value_get_object(json);
    if (!root) {
        json_value_free(json);
        return cmd_response(fd, ESCHEMA,
            "invalid command: expected json object");
    }

    cmds = json_object_get_array(root, "commands");
    if (!cmds) {
        json_value_free(json);
        return cmd_response(fd, ESCHEMA,
            "invalid command: expected 'commands' array");
    }

    rv = exec_commands(fd, cmds);

    json_value_free(json);

    return rv;
}

static int cmd_wait(rpc_t *rpc, int timeout)
{
    int rv;
    rpcmsg_t msg;

    msg.len = sizeof(msgbuf);
    msg.buf = msgbuf;
    msg.fd = -1;

    errno = EINTR;
    rv = rpc_recv(rpc, timeout, &msg);
    if (rv < 0) {
        if (errno == EINTR) {
            return -1;
        } else if (errno == ENOBUFS) {
            if (msg.fd > -1) {
                cmd_response(msg.fd, EMSGLEN,
                             "command exceeds maximum length: %d",
                             MAXMSG);
                return -1;
            } else {
                err(errno, "cmd_handle: unreachable");
            }
        } else {
            err(errno, "rpc_recv()");
        }
    } else if (!rv) {
        return 0;
    }

    assert(msg.fd >= 0);

    /* we did receive something, so
     * now we can null terminate */
    msg.buf[msg.len] = '\0';

    return exec_command_string(msg.fd, msg.buf);
}

int main(int argc, char **argv)
{
    int fd;
    rpc_t rpc;

    if (argc == 2) {
        if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--client") == 0) {
            return do_client();
        }
    }

    fd = rpc_serve(SOCKPATH, &rpc);
    if (fd < 0) {
        err(errno, "failed to create server");
    }

    for (;;) {
        if (cmd_wait(&rpc, -1) < 0) {
            err(errno, "cmd_handle()");
        }
    }

    return 0;
}
