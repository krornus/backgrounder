#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "uri.h"
#include "img/img.h"

int usage()
{
    fprintf(stderr, "usage: %s <IMAGE> [<IMAGE>, ...]\n", program_invocation_short_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *const argv[])
{
    URI *fp;
    img_t img;
    uint8_t *raw;

    for (int i = 1; i < argc; i++) {
        fp = uopen(argv[i], "r");

        if (fp) {
            if (img_load(fp, &img) == 0) {
                printf("%s: [%zdx%zd]\n", argv[i], img_width(&img), img_height(&img));
                raw = img_raw(&img);
                for (size_t w = 0; w < img_width(&img); w++) {
                    for (size_t h = 0; h < img_height(&img); h++) {
                    }
                }
                if (raw) {
                    free(raw);
                }
                img_close(&img);
            }

            uclose(fp);
        } else {
            warn("%s", argv[i]);
        }
    }
}
