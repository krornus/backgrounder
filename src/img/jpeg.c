#include <err.h>
#include <setjmp.h>
#include <stdlib.h>

#include "img/jpeg.h"

#define FREAD_SIZE 4096

/* SEE: libjpeg/jdatasrc.c */

static void jpeg_error(j_common_ptr cinfo)
{
    struct error_mgr *err;
    err = (struct error_mgr *)cinfo->err;
    longjmp(err->env, 1);
}

int jpeg_load(FILE *fp, jpeg_t *jpeg)
{
    jpeg->cinfo.err = jpeg_std_error(&jpeg->jerr.pub);
    jpeg->jerr.pub.error_exit = jpeg_error;

    if (setjmp(jpeg->jerr.env)) {
        if (fseek(fp, SEEK_SET, 0) != 0 || ftell(fp) != 0) {
            errx(1, "png: rewind failed");
        }
        jpeg_destroy_decompress(&jpeg->cinfo);
        return -1;
    }

    jpeg_create_decompress(&jpeg->cinfo);
    jpeg_stdio_src(&jpeg->cinfo, fp);

    jpeg_read_header(&jpeg->cinfo, TRUE);
    jpeg_start_decompress(&jpeg->cinfo);

    if (setjmp(jpeg->jerr.env)) {
        /* abort if a jpeg error occurs after this */
        fprintf(stderr, "jpeg_load(): unreachable\n");
        abort();
    }

    return 0;
}

size_t jpeg_width(jpeg_t *jpeg)
{
    return (size_t)jpeg->cinfo.image_width;
}

size_t jpeg_height(jpeg_t *jpeg)
{
    return (size_t)jpeg->cinfo.image_height;
}

size_t jpeg_depth(jpeg_t *jpeg)
{
    return (size_t)jpeg->cinfo.num_components;
}

uint8_t *jpeg_raw(jpeg_t *jpeg)
{
    uint8_t *raw;
    size_t width, height, depth;
    JDIMENSION rowlen;
    JSAMPARRAY image;

    raw = NULL;
    image = NULL;

    if (setjmp(jpeg->jerr.env)) {
        free(raw);
        free(image);
        return NULL;
    }

    width = jpeg_width(jpeg);
    height = jpeg_height(jpeg);
    depth = jpeg_depth(jpeg);

    raw = (uint8_t *)malloc(width * depth * height);
    if (!raw) {
        return NULL;
    }

    image = (JSAMPARRAY)malloc(sizeof(JSAMPROW) * height);
    if (!image) {
        return NULL;
    }

    rowlen = jpeg->cinfo.output_width * jpeg->cinfo.output_components;
    for (size_t i = 0; i < height; i++) {
        image[i] = raw + (i * rowlen);
    }

    jpeg_read_scanlines(&jpeg->cinfo, image, height);

    if (setjmp(jpeg->jerr.env)) {
        fprintf(stderr, "jpeg_raw(): unreachable\n");
        abort();
    }

    return raw;
}

void jpeg_close(jpeg_t *jpeg)
{
    jpeg_destroy_decompress(&jpeg->cinfo);
}
