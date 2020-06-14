#ifndef jpeg_h_INCLUDED
#define jpeg_h_INCLUDED

#include <stdio.h>
#include <stddef.h>
#include <jpeglib.h>

#include "uri.h"
#include "img/pixel.h"

typedef struct jpeg jpeg_t;

struct error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf env;
};

struct jpeg {
    struct jpeg_decompress_struct cinfo;
    struct error_mgr jerr;
    JSAMPARRAY row;
    size_t x;
    size_t y;
};

int jpeg_load(URI *fp, jpeg_t *jpeg);
void jpeg_close(jpeg_t *jpeg);

size_t jpeg_width(jpeg_t *jpeg);
size_t jpeg_height(jpeg_t *jpeg);

int jpeg_next_pixel(jpeg_t *jpeg, pixel_t *pix);

#endif // jpeg_h_INCLUDED
