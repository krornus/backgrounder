#ifndef pixel_h_INCLUDED
#define pixel_h_INCLUDED

#include <stddef.h>

typedef struct pixel pixel_t;

struct pixel {
    size_t x;
    size_t y;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

#endif // pixel_h_INCLUDED
