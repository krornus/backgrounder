#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "img.h"

int img_load(FILE *fp, img_t *img)
{
    if (jpeg_load(fp, &img->un.jpeg) == 0) {
        img->ty = IMG_TY_JPEG;
        return 0;
    } else if (png_load(fp, &img->un.png) == 0) {
        img->ty = IMG_TY_PNG;
        return 0;
    } else {
        errno = EINVAL;
        return -1;
    }
}

size_t img_width(img_t *img)
{
    switch (img->ty) {
    case IMG_TY_PNG:
        return png_width(&img->un.png);
    case IMG_TY_JPEG:
        return jpeg_width(&img->un.jpeg);
    default:
        abort();
    }
}

size_t img_height(img_t *img)
{
    switch (img->ty) {
    case IMG_TY_PNG:
        return png_height(&img->un.png);
    case IMG_TY_JPEG:
        return jpeg_height(&img->un.jpeg);
    default:
        abort();
    }
}

size_t img_depth(img_t *img)
{
    switch (img->ty) {
    case IMG_TY_PNG:
        return png_depth(&img->un.png);
    case IMG_TY_JPEG:
        return jpeg_depth(&img->un.jpeg);
    default:
        abort();
    }
}

uint8_t *img_raw(img_t *img)
{
    switch (img->ty) {
    case IMG_TY_PNG:
        return png_raw(&img->un.png);
    case IMG_TY_JPEG:
        return jpeg_raw(&img->un.jpeg);
    default:
        errno = EINVAL;
        return NULL;
    }
}

void img_close(img_t *img)
{
    switch (img->ty) {
    case IMG_TY_PNG:
        png_close(&img->un.png);
        break;
    case IMG_TY_JPEG:
        jpeg_close(&img->un.jpeg);
        break;
    default:
        break;
    }
}
