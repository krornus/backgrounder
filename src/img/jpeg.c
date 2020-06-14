#include <err.h>
#include <setjmp.h>
#include <stdlib.h>

#include "img/jpeg.h"

#define FREAD_SIZE 4096

/* SEE: libjpeg/jdatasrc.c */

typedef struct uri_source_mgr uri_source_mgr_t;

struct uri_source_mgr {
    struct jpeg_source_mgr pub;
    URI *uri;
    unsigned char *buf;
    int start;
};

static void init_source(j_decompress_ptr cinfo)
{
    uri_source_mgr_t *src;

    src = (uri_source_mgr_t *)cinfo->src;
    src->start = 1;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
    size_t len;
    uri_source_mgr_t *src;

    src = (uri_source_mgr_t *)cinfo->src;

    len = uread(src->buf, 1, FREAD_SIZE, src->uri);
    if (len == 0) {
        return FALSE;
    }

    src->pub.next_input_byte = src->buf;
    src->pub.bytes_in_buffer = len;

    src->start = 0;

    return TRUE;
}

static void skip_input_data(j_decompress_ptr cinfo, long len)
{
    struct jpeg_source_mgr *src;

    src = (struct jpeg_source_mgr *)cinfo->src;

    if (len > 0) {
        while (len > (long) src->bytes_in_buffer) {
            len -= (long) src->bytes_in_buffer;
            if ((*src->fill_input_buffer)(cinfo) == FALSE) {
                cinfo->err->error_exit((j_common_ptr)cinfo);
            }
        }
        src->next_input_byte += (size_t)len;
        src->bytes_in_buffer -= (size_t)len;
    }
}

static void term_source(j_decompress_ptr cinfo)
{
    (void)cinfo;
}

static void jpeg_error(j_common_ptr cinfo)
{
    struct error_mgr *err;
    err = (struct error_mgr *)cinfo->err;
    longjmp(err->env, 1);
}

static void jpeg_uri_src(j_decompress_ptr cinfo, URI *fp)
{
    uri_source_mgr_t *src;

    if (cinfo->src == NULL) {
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small)((j_common_ptr)cinfo,
                                       JPOOL_PERMANENT,
                                       sizeof(uri_source_mgr_t));
        src = (uri_source_mgr_t *)cinfo->src;
        src->buf = (unsigned char *)
            (*cinfo->mem->alloc_small)((j_common_ptr)cinfo,
                                       JPOOL_PERMANENT,
                                       FREAD_SIZE * sizeof(unsigned char));
    }

    src = (uri_source_mgr_t *)cinfo->src;
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    src->pub.term_source = term_source;
    src->pub.bytes_in_buffer = 0;
    src->pub.next_input_byte = NULL;
    src->uri = fp;
}

int jpeg_load(URI *fp, jpeg_t *jpeg)
{
    JDIMENSION stride;

    jpeg->cinfo.err = jpeg_std_error(&jpeg->jerr.pub);
    jpeg->jerr.pub.error_exit = jpeg_error;

    if (setjmp(jpeg->jerr.env)) {
        jpeg_destroy_decompress(&jpeg->cinfo);
        return -1;
    }

    jpeg_create_decompress(&jpeg->cinfo);
    jpeg_uri_src(&jpeg->cinfo, fp);

    jpeg_read_header(&jpeg->cinfo, TRUE);
    jpeg_start_decompress(&jpeg->cinfo);

    stride = jpeg->cinfo.output_width * jpeg->cinfo.output_components;
    /* allocate a sample array for the buffer (single row) */
    jpeg->row = (*jpeg->cinfo.mem->alloc_sarray)(
                                        (j_common_ptr)&jpeg->cinfo,
                                        JPOOL_IMAGE,
                                        stride,
                                        1);

    jpeg_read_scanlines(&jpeg->cinfo, jpeg->row, 1);

    if (setjmp(jpeg->jerr.env)) {
        /* abort if a jpeg error occurs after this */
        fprintf(stderr, "unreachable\n");
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

int jpeg_next_pixel(jpeg_t *jpeg, pixel_t *pix)
{
    unsigned char *pixel, *row;

    if (setjmp(jpeg->jerr.env)) {
        return -1;
    }

    if (jpeg->cinfo.output_scanline >= jpeg->cinfo.output_height) {
        return 0;
    }

    row = (unsigned char *)(jpeg->row[0]);
    pixel = &row[jpeg->x*jpeg->cinfo.output_components];
    pix->x = jpeg->x;
    pix->y = jpeg->cinfo.output_scanline;
    pix->r = pixel[0];
    pix->g = pixel[1];
    pix->b = pixel[2];

    if (jpeg->cinfo.output_components == 4) {
        pix->a = pixel[3];
    }

    jpeg->x++;

    if (jpeg->x == jpeg_width(jpeg)) {
        jpeg->x = 0;
        if (jpeg->cinfo.output_scanline < jpeg->cinfo.output_height) {
            jpeg_read_scanlines(&jpeg->cinfo, jpeg->row, 1);
        }
    }

    if (setjmp(jpeg->jerr.env)) {
        /* abort if a jpeg error occurs after this */
        fprintf(stderr, "unreachable\n");
        abort();
    }

    return 1;
}

void jpeg_close(jpeg_t *jpeg)
{
    jpeg_destroy_decompress(&jpeg->cinfo);
}
