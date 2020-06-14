#include <err.h>
#include <errno.h>
#include <stdlib.h>

#include "buf.h"
#include "img/png.h"

#define PNG_SIG_LEN 8

static void png_uread(png_struct *png, png_byte *buf, png_size_t len)
{
    URI *sp;
    size_t read;

    sp = (URI *)png_get_io_ptr(png);
    if (!sp) {
        errx(1, "png: read failed");
    }

    read = uread(buf, sizeof(char), len, sp);
    if ((png_size_t)read != len) {
        errx(1, "png: invalid png");
    }
}

int png_load(URI *fp, png_t *png)
{
    char sig[PNG_SIG_LEN];

    if (uread(sig, sizeof(char), sizeof(sig), fp) != sizeof(sig)) {
        return -1;
    }

    if (png_sig_cmp((png_const_bytep)sig, 0, sizeof(sig)) != 0) {
        if (urewind(fp, PNG_SIG_LEN) != PNG_SIG_LEN) {
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

    png_set_read_fn(png->png, fp, png_uread);
    png_set_sig_bytes(png->png, PNG_SIG_LEN);

    png_read_info(png->png, png->info);

    png->type = png_get_color_type(png->png, png->info);
    png->rowlen = png_get_rowbytes(png->png, png->info);

    png->x = 0;
    png->y = 0;

    png->row = (png_byte *)png_malloc(png->png, png->rowlen);
    png_read_row(png->png, png->row, NULL);

    if (setjmp(png_jmpbuf(png->png)) != 0) {
        fprintf(stderr, "unreachable\n");
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

int png_next_pixel(png_t *png, pixel_t *pix)
{
    png_byte *pixel;
    size_t stride, width, height;

    width = png_width(png);
    height = png_height(png);

    if (png->y >= height) {
        return 0;
    }

    switch (png->type) {
    case PNG_COLOR_TYPE_RGB:
        stride = 3;
        break;
    case PNG_COLOR_TYPE_RGBA:
        stride = 4;
        break;
    default:
        errno = EINVAL;
        return -1;
    }

    /* fix setjmp */
    if (setjmp(png_jmpbuf(png->png)) != 0) {
        return -1;
    }

    pixel = &png->row[png->x*stride];
    pix->x = png->x;
    pix->y = png->y;
    pix->r = pixel[0];
    pix->g = pixel[1];
    pix->b = pixel[2];

    if (png->type == PNG_COLOR_TYPE_RGBA) {
        pix->a = pixel[3];
    }

    png->x++;

    if (png->x == width) {
        png->x = 0;
        png->y++;
        if (png->y < height) {
            png_read_row(png->png, png->row, NULL);
        }
    }

    if (setjmp(png_jmpbuf(png->png)) != 0) {
        fprintf(stderr, "unreachable\n");
        abort();
    }

    return 1;
}

void png_close(png_t *png)
{
    png_destroy_info_struct(png->png, &png->info);
    png_destroy_read_struct(&png->png, NULL, NULL);
    free(png->row);
}
