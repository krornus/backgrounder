#include <err.h>
#include <errno.h>
#include <stdlib.h>

#include "img/png.h"

#define PNG_SIG_LEN 8

int png_load(FILE *fp, png_t *png)
{
    char sig[PNG_SIG_LEN];

    if (fread(sig, sizeof(char), sizeof(sig), fp) != sizeof(sig)) {
        return -1;
    }

    if (png_sig_cmp((png_const_bytep)sig, 0, sizeof(sig)) != 0) {
        if (fseek(fp, SEEK_SET, 0) != 0 || ftell(fp) != 0) {
            errx(1, "png: rewind failed");
        }
        errno = EINVAL;
        return -1;
    }

    /* allocate the png struct */
    png->png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                 NULL, NULL, NULL);
    if (!png->png) {
        return -1;
    }

    /* allocate the info struct */
    png->info = png_create_info_struct(png->png);
    if (!png->info) {
        png_destroy_read_struct(&png->png, NULL, NULL);
        return -1;
    }

    /* set error handler */
    if (setjmp(png_jmpbuf(png->png)) != 0) {
        png_destroy_info_struct(png->png, &png->info);
        png_destroy_read_struct(&png->png, NULL, NULL);
        return -1;
    }

    png_init_io(png->png, fp);
    png_set_sig_bytes(png->png, PNG_SIG_LEN);
    png_read_info(png->png, png->info);

    if (setjmp(png_jmpbuf(png->png)) != 0) {
        fprintf(stderr, "png_load(): unreachable\n");
        abort();
    }

    return 0;
}

size_t png_width(png_t *png)
{
    return png_get_image_width(png->png, png->info);
}

size_t png_height(png_t *png)
{
    return png_get_image_height(png->png, png->info);
}

size_t png_depth(png_t *png)
{
    int type;

    type = png_get_color_type(png->png, png->info);
    if (type == PNG_COLOR_TYPE_RGB) {
        return 3;
    } else if (type == PNG_COLOR_TYPE_RGBA) {
        return 4;
    } else {
        errx(1, "unsupported png color type");
    }
}

uint8_t *png_raw(png_t *png)
{
    uint8_t *raw;
    size_t width, height, depth;
    png_bytepp image;
    png_uint_32 rowlen;

    if (setjmp(png_jmpbuf(png->png)) != 0) {
        free(raw);
        free(image);
        return NULL;
    }

    raw = NULL;
    image = NULL;

    width = png_width(png);
    height = png_height(png);
    depth = png_depth(png);

    raw = (uint8_t *)malloc(width * depth * height);
    if (!raw) {
        return NULL;
    }

    image = (png_bytepp)malloc(sizeof(png_bytep) * height);
    if (!image) {
        return NULL;
    }

    rowlen = png_get_rowbytes(png->png, png->info);
    for (size_t i = 0; i < height; i++) {
        image[i] = raw + (i * rowlen);
    }

    png_read_image(png->png, image);

    if (setjmp(png_jmpbuf(png->png)) != 0) {
        fprintf(stderr, "png_raw(): unreachable\n");
        abort();
    }

    return raw;
}

void png_close(png_t *png)
{
    png_destroy_info_struct(png->png, &png->info);
    png_destroy_read_struct(&png->png, NULL, NULL);
}
