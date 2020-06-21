#ifndef png_h_INCLUDED
#define png_h_INCLUDED

#include <stdint.h>
#include <png.h>

#include "uri.h"

typedef struct png png_t;

struct png {
    png_struct *png;
    png_info *info;
};

int png_load(FILE *fp, png_t *png);
void png_close(png_t *png);

size_t png_width(png_t *png);
size_t png_height(png_t *png);
size_t png_depth(png_t *png);

uint8_t *png_raw(png_t *png);

#endif // png_h_INCLUDED
