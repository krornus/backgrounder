#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <Imlib2.h>
#include <X11/Xatom.h>

#include "uri.h"
#include "img.h"

Display *disp;
Window root;
Visual *vis;
Screen *scr;

int img_copy(const char *path, const char *uri)
{
    FILE *src, *dest;
    long len;
    char buf[4096];

    src = uopen(uri);
    if (!src) {
        return -1;
    }

    dest = fopen(path, "w");
    if (!dest) {
        fclose(dest);
        return -1;
    }

    for (;;) {
        size_t idx;
        ssize_t nr, nw;

        nr = fread(buf, sizeof(buf[0]), sizeof(buf), src);
        if (nr < 0) {
            goto fail;
        } else if (nr == 0) {
            break;
        }

        idx = 0;
        while (nr) {
            nw = fwrite(&buf[idx], sizeof(buf[0]), nr, dest);
            if (nw < 0) {
                goto fail;
            }

            nr -= nw;
            idx += nw;
            len -= nw;
        }
    }

    if (ferror(src) || ferror(dest)) {
        goto fail;
    }

    fclose(src);
    fclose(dest);
    return 0;
fail:
    unlink(path);
    fclose(src);
    fclose(dest);
    return -1;
}

static unsigned char *xatom_data(Display *disp,
                                 Window root,
                                 const char *name,
                                 Atom *type)
{
    Atom prop;
    int format;
    unsigned long length, after;
    unsigned char *data;

    prop = XInternAtom(disp, name, True);
    if (prop != None) {
        XGetWindowProperty(disp, root, prop, 0L, 1L, False,
                           AnyPropertyType, type, &format, &length,
                           &after, &data);
        return data;
    } else {
        return NULL;
    }
}

static void xkill_bg_root(Display *disp, Window root)
{
    Atom type;
    unsigned char *data_root, *data_esetroot;

    data_root = xatom_data(disp, root, "_XROOTPMAP_ID", &type);
    data_esetroot = NULL;

    if (data_root && type == XA_PIXMAP) {
        data_esetroot = xatom_data(disp, root, "ESETROOT_PMAP_ID", &type);
        if (data_esetroot && type == XA_PIXMAP) {
            if (*((Pixmap *)data_root) == *((Pixmap *)data_esetroot)) {
                XKillClient(disp, *((Pixmap *)data_root));
            }
        }
    }

    if (data_root) {
        XFree(data_root);
    }

    if (data_esetroot) {
        XFree(data_esetroot);
    }
}

void img_xinit(Display *display)
{
    disp = display;
    root = RootWindow(disp, DefaultScreen(disp));
    vis = DefaultVisual(disp, DefaultScreen(disp));
    scr = ScreenOfDisplay(disp, DefaultScreen(disp));

    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
}

static void img_set_center(int x, int y)
{
    int offset_x, offset_y, w, h;

    w = scr->width;
    h = scr->height;

    offset_x = (w - imlib_image_get_width()) >> 1;
    offset_y = (h - imlib_image_get_height()) >> 1;

    imlib_render_image_part_on_drawable_at_size(
        ((offset_x < 0) ? -offset_x : 0),
        ((offset_y < 0) ? -offset_y : 0),
        w,
        h,
        x + ((offset_x > 0) ? offset_x : 0),
        y + ((offset_y > 0) ? offset_y : 0),
        w,
        h);
}

static void img_set_scale(int x, int y)
{
    int w, h;

    w = scr->width;
    h = scr->height;

    imlib_render_image_on_drawable_at_size(x, y, w, h);
}

static void img_set_fill(int x, int y)
{
    int img_w, img_h, cut_x;
    int render_w, render_h, render_x, render_y;
    int w, h;

    w = scr->width;
    h = scr->height;

    img_w = imlib_image_get_width();
    img_h = imlib_image_get_height();

    cut_x = (((img_w * h) > (img_h * w)) ? 1 : 0);

    render_w = (  cut_x ? ((img_h * w) / h) : img_w);
    render_h = ( !cut_x ? ((img_w * h) / w) : img_h);

    render_x = (  cut_x ? ((img_w - render_w) >> 1) : 0);
    render_y = ( !cut_x ? ((img_h - render_h) >> 1) : 0);

    imlib_render_image_part_on_drawable_at_size(
        render_x, render_y,
        render_w, render_h,
        x, y, w, h);
}

static void img_set_max(int x, int y)
{
    int img_w, img_h, border_x;
    int render_w, render_h, render_x, render_y;
    int margin_x, margin_y;
    int w, h;

    w = scr->width;
    h = scr->height;

    img_w = imlib_image_get_width();
    img_h = imlib_image_get_height();

    border_x = (((img_w * h) > (img_h * w)) ? 0 : 1);

    render_w = (border_x ? ((img_w * h) / img_h) : w);
    render_h = (!border_x ? ((img_h * w) / img_w) : h);

    margin_x = (w - render_w) >> 1;
    margin_y = (h - render_h) >> 1;

    render_x = x + (border_x ? margin_x : 0);
    render_y = y + (!border_x ? margin_y : 0);

    imlib_render_image_on_drawable_at_size(
        render_x, render_y,
        render_w, render_h);
}

int img_set(const char *path, int mode)
{
    int depth;
    Imlib_Image im;
    Pixmap drawable;
    Atom prop_root, prop_esetroot;

    im = imlib_load_image(path);
    if (!im) {
        return -1;
    }

    depth = DefaultDepth(disp, DefaultScreen(disp));
    drawable = XCreatePixmap(disp, root, scr->width, scr->height, depth);

    imlib_context_set_image(im);
    imlib_context_set_drawable(drawable);
    imlib_context_set_anti_alias(0);
    imlib_context_set_dither(1);
    imlib_context_set_blend(1);
    imlib_context_set_angle(0);

    switch (mode) {
    case MODE_CENTER:
        img_set_center(0, 0);
        break;
    case MODE_SCALE:
        img_set_scale(0, 0);
        break;
    case MODE_FILL:
        img_set_fill(0, 0);
        break;
    case MODE_MAX:
        img_set_max(0, 0);
        break;
    default:
        warnx("invalid image mode");
        return -1;
    }

    xkill_bg_root(disp, root);

    prop_root = XInternAtom(disp, "_XROOTPMAP_ID", False);
    prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", False);

    if (prop_root == None || prop_esetroot == None) {
        err(1, "root pixmap atom creation failed");
    }

    XChangeProperty(disp, root, prop_root, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&drawable, 1);
    XChangeProperty(disp, root, prop_esetroot, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&drawable, 1);

    XSetWindowBackgroundPixmap(disp, root, drawable);
    XClearWindow(disp, root);
    XFlush(disp);
    XSetCloseDownMode(disp, RetainPermanent);

    return 0;
}
