#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <X11/Xlib.h>

#include "img.h"

#define DFLT_IM_PATH "bkg.img"
#define arysize(x) (sizeof(x)/(sizeof((x)[0])))

typedef struct args args_t;

struct args {
    int mode;
    const char *path;
    const char *output;
    char *color;
};

static char pathbuf[4096];

void __attribute__((noreturn)) help(void)
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

void __attribute__((noreturn)) usage(void)
{
    fprintf(stderr, "usage: %s [-h/--help] [options] <path>\n",
                    program_invocation_name);
    exit(EXIT_FAILURE);
}

int ishex(int c)
{
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

char *xcolorstr(const char *color)
{
    static char cstr[sizeof("rgb:00/00/00")] = "rgb:";
    char *ptr;

    if (*color++ != '#') {
        goto fail;
    }

    ptr = &cstr[strlen("rgb:")];

    for (int i = 0; i < 3; i++) {
        if (!ishex(color[0]) || !ishex(color[1])) {
            goto fail;
        }
        *ptr++ = *color++;
        *ptr++ = *color++;

        if (i < 2) {
            *ptr++ = '/';
        }
    }

    if (*color) {
        goto fail;
    }

    *ptr = '\0';

    return cstr;

fail:
    err(1, "invalid color string, must be hex string of the form: #rrggbb");
}

char *tmpdir(void)
{
    char *var;
    const char *env[] = {
        "TMPDIR", "TMP", "TEMP", "TEMPDIR"
    };

    for (size_t i = 0; i < arysize(env); i++) {
        var = getenv(env[i]);
        if (var) {
            return var;
        }
    }

    return "/tmp";
}

const char *outpath(const char *path)
{
    if (path) {
        return path;
    } else {
        int rv;
        char *tmp;

        tmp = tmpdir();
        if (!*tmp) {
            tmp = ".";
        }

        rv = snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
                      tmp, DFLT_IM_PATH);
        if (rv >= (int)sizeof(pathbuf)) {
            err(1, "path exceeds max length: %s/%s", tmp, DFLT_IM_PATH);
        }

        return (const char *)pathbuf;
    }
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
            help();
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
            args->color = xcolorstr(optarg);
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
        usage();
    }

    args->path = argv[optind];
    args->output = outpath(args->output);
}

int main(int argc, char *const argv[])
{
    Display *disp;
    args_t args;

    parse_args(argc, argv, &args);

    disp = XOpenDisplay(NULL);
    if (!disp) {
        err(EXIT_FAILURE, "failed to open X display");
    }

    img_xinit(disp);

    if (img_copy(args.output, args.path) < 0) {
        err(EXIT_FAILURE, "%s", argv[1]);
    }

    if (img_set(args.output, args.mode, args.color) < 0) {
        unlink(args.output);
        err(EXIT_FAILURE, "%s", argv[1]);
    }

    XCloseDisplay(disp);
}
