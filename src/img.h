#ifndef img_h_INCLUDED
#define img_h_INCLUDED

typedef struct pixel pixel_t;

struct pixel {
    size_t x;
    size_t y;
    unsigned r;
    unsigned g;
    unsigned b;
    unsigned a;
};

enum ImageType {
    IMG_TY_PNG,
    IMG_TY_UNKNOWN,
};

#endif // img_h_INCLUDED
