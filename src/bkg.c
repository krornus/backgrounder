#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "img.h"
#include "uri.h"

int usage()
{
    fprintf(stderr, "usage: %s <IMAGE> [<IMAGE>, ...]\n", program_invocation_short_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *const argv[])
{
    URI *fp;
    img_t img;
    pixel_t pix;

    for (int i = 1; i < argc; i++) {
        fp = uopen(argv[i], "r");

        if (fp) {
            if (img_load(fp, &img) == 0) {
                printf("%s: [%zdx%zd]\n", argv[i], img_width(&img), img_height(&img));
                while (img_next_pixel(&img, &pix) > 0) {
                    printf("  [%zdx%zd]: #%02x%02x%02x\n",
                           pix.x, pix.y, pix.r, pix.g, pix.b);
                }
            }
        } else {
            warn("%s", argv[i]);
        }
    }
}
