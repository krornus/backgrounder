#ifndef wall_h_INCLUDED
#define wall_h_INCLUDED

enum {
    MODE_FILL,
    MODE_MAX,
    MODE_CENTER,
    MODE_SCALE,
};

void img_xinit(Display *disp);

int img_copy(const char *path, const char *uri);
int img_set(const char *path, int mode);

#endif // wall_h_INCLUDED
