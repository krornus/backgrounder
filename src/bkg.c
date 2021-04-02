#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include "img.h"
#include "rpc/rpc.h"
#include "rpc/cmd.h"

#define RPCSOCK "bkg:socket"

typedef struct args args_t;

struct args {
    int mode;
    const char *path;
    const char *output;
    char *color;
};

void __attribute__((noreturn)) exit_help(void)
{
    fprintf(stderr, "Usage: bkg [mode] <path>\n");
    fprintf(stderr, "Sets the desktop background\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Mandatory arguments:\n");
    fprintf(stderr, "  <path>       the path to the image, may be a url or file path\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Optional fill modes:\n");
    fprintf(stderr, "  -f, --fill   fill entire screen preserving aspect ratio\n");
    fprintf(stderr, "  -m, --max    fill screen without clipping the image and preserving\n");
    fprintf(stderr, "               aspect ratio\n");
    fprintf(stderr, "  -c, --center center the image on the screen, keeping original size\n");
    fprintf(stderr, "  -s, --scale  scale the image to fill the screen\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Optional arguments:\n");
    fprintf(stderr, "  -o, --output <path> the path to which the image should be written\n");
    fprintf(stderr, "  -C, --color <color> the background fill color, a hex string in the\n");
    fprintf(stderr, "                      form #rrggbb\n");

    exit(EXIT_SUCCESS);
}

void __attribute__((noreturn)) exit_usage(void)
{
    fprintf(stderr, "usage: %s [-h/--help] [options] <path>\n",
                    program_invocation_name);
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char *const argv[], args_t *args)
{
   int c;
   int longidx;

    static struct option long_options[] = {
        {"help", no_argument, 0, 0},
        {"fill", no_argument, 0, 0},
        {"max", no_argument, 0, 0},
        {"center", no_argument, 0, 0},
        {"scale", no_argument, 0, 0},
        {"output", required_argument, 0, 0},
        {"color", required_argument, 0, 0},
        {0, 0, 0, 0},
    };

    args->output = NULL;
    args->color = "black";
    args->mode = MODE_CENTER;

    for (;;) {
        c = getopt_long(argc, argv, "hfmcso:C:", long_options, &longidx);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            exit_help();
        case 'f':
            args->mode = MODE_FILL;
            break;
        case 'm':
            args->mode = MODE_MAX;
            break;
        case 'c':
            args->mode = MODE_CENTER;
            break;
        case 's':
            args->mode = MODE_SCALE;
            break;
        case 'o':
            args->output = optarg;
            break;
        case 'C':
            args->color = optarg;
            break;
        case '?':
            fprintf(stderr, "Try '%s --help' for more information.\n",
                    program_invocation_name);
            exit(EXIT_FAILURE);
        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    if (optind >= argc || optind + 1 != argc) {
        exit_usage();
    }

    args->path = argv[optind];
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
        errx(errno, "???");
    }

    return 0;
}

int main(int argc, char *const argv[])
{
    Display *disp;
    args_t args;
    int fd;
    rpc_t rpc;

    // parse_args(argc, argv, &args);

    if (argc == 2) {
        if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--client") == 0) {
            return do_client();
        }
    }

    disp = XOpenDisplay(NULL);
    if (!disp) {
        err(EXIT_FAILURE, "failed to open X display");
    }

    img_xinit(disp);

    fd = rpc_serve(RPCSOCK, &rpc);
    if (fd < 0) {
        err(errno, "failed to create server");
    }

    for (;;) {
        if (cmd_wait(&rpc, -1) < 0) {
            err(errno, "cmd_handle()");
        }
    }

    XCloseDisplay(disp);
}
