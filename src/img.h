#ifndef wall_h_INCLUDED
#define wall_h_INCLUDED

#include <X11/Xlib.h>

enum {
    MODE_FILL=1,
    MODE_MAX,
    MODE_CENTER,
    MODE_SCALE,
};

void img_xinit(Display *disp);

int img_copy(const char *path, const char *uri);
int img_set(const char *path, int mode, const char *bgcolor);

int bkg_set(const char *uri, const char *out, int mode, const char *bgcolor);

#endif // wall_h_INCLUDED
