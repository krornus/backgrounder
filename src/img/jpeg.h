#ifndef jpeg_h_INCLUDED
#define jpeg_h_INCLUDED

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <jpeglib.h>

#include "uri.h"

typedef struct jpeg jpeg_t;

struct error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf env;
};

struct jpeg {
    struct jpeg_decompress_struct cinfo;
    struct error_mgr jerr;
};

int jpeg_load(FILE *fp, jpeg_t *jpeg);
void jpeg_close(jpeg_t *jpeg);

size_t jpeg_width(jpeg_t *jpeg);
size_t jpeg_height(jpeg_t *jpeg);
size_t jpeg_depth(jpeg_t *jpeg);

uint8_t *jpeg_raw(jpeg_t *jpeg);

#endif // jpeg_h_INCLUDED
