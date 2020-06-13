#ifndef png_h_INCLUDED
#define png_h_INCLUDED

#include <png.h>
#include "uri.h"
#include "img.h"

typedef struct png png_t;

struct png {
    png_struct *png;
    png_info *info;
    png_byte type;
    png_uint_32 width;
    png_uint_32 height;
    png_uint_32 rowlen;
    png_uint_32 x;
    png_uint_32 y;
    png_byte *row;
};

int png_load(URI *fp, png_t *png);
int png_next_pixel(png_t *png, pixel_t *pix);

#endif // png_h_INCLUDED