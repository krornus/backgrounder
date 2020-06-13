#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "img.h"
#include "img/png.h"
#include "uri.h"

int usage()
{
    fprintf(stderr, "usage: %s <IMAGE> [<IMAGE>, ...]\n", program_invocation_short_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *const argv[])
{
    URI *fp;
    png_t img;
    pixel_t pix;

    for (int i = 1; i < argc; i++) {
        fp = uopen(argv[i], "r");

        if (!fp) {
            warn("%s", argv[i]);
        } else {
            if (png_load(fp, &img) == 0) {
                printf("%s: png: [%ux%u]\n", argv[i], img.width, img.height);
                while (png_next_pixel(&img, &pix) > 0) {
                    printf("  [%zdx%zd]: #%02x%02x%02x\n", pix.x, pix.y, pix.r, pix.g, pix.b);
                }
            }
            uclose(fp);
        }
    }
}
