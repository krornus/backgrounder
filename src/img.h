#ifndef img_h_INCLUDED
#define img_h_INCLUDED

#include "img/pixel.h"
#include "img/png.h"
#include "img/jpeg.h"
#include "uri.h"

typedef struct img img_t;

enum ImageType {
    IMG_TY_PNG,
    IMG_TY_JPEG,
    IMG_TY_UNKNOWN,
};

struct img {
    enum ImageType ty;
    union {
        png_t png;
        jpeg_t jpeg;
    } un;
};

int img_load(URI *fp, img_t *img);
void img_close(img_t *img);

size_t img_width(img_t *img);
size_t img_height(img_t *img);

int img_next_pixel(img_t *img, pixel_t *pix);

#endif // img_h_INCLUDED
