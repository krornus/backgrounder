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
#define PROG    program_invocation_short_name

typedef struct args args_t;

struct args {
    int mode;
    const char *path;
    const char *output;
    char *color;
};

void __attribute__((noreturn)) exit_help(void)
{
    fprintf(stderr, "Usage: %s [mode] <path>\n", PROG);
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

int main(int argc, char *const argv[])
{
    Display *disp;
    rpc_t rpc;
    int fd;
    (void)argc;
    (void)argv;

    // parse_args(argc, argv, &args);

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
