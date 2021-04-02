#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <Imlib2.h>
#include <X11/Xatom.h>

#include "uri.h"
#include "img.h"

#define DFLTIMG "bkg.img"
#define arysize(x) (sizeof(x)/(sizeof((x)[0])))

static Display *disp;
static Window root;
static Visual *vis;
static Screen *scr;
static Colormap cm;
static int depth;
static Atom root, esetroot;

static Pixmap drawable;
static char pathbuf[4096];

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
    cm = DefaultColormap(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    drawable = XCreatePixmap(disp, root, scr->width, scr->height, depth);


    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_drawable(drawable);
    imlib_context_set_anti_alias(1);
    imlib_context_set_dither(1);
    imlib_context_set_blend(1);
    imlib_context_set_angle(0);

    xkill_bg_root(disp, root);

    XSetCloseDownMode(disp, RetainPermanent);

    root = XInternAtom(disp, "_XROOTPMAP_ID", False);
    esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", False);
    if (root == None || esetroot == None) {
        err(1, "root pixmap atom creation failed");
    }
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

int img_set(const char *path, int mode, const char *bgcolor)
{
    Imlib_Image im;
    XColor color;
    XGCValues gcvalue;
    GC gc;

    im = imlib_load_image(path);
    if (!im) {
        return -1;
    }

    /* set the background color of drawable */
    XAllocNamedColor(disp, cm, bgcolor, &color, &color);
    gcvalue.foreground = color.pixel;
    gc = XCreateGC(disp, drawable, GCForeground, &gcvalue);
    XFillRectangle(disp, drawable, gc, 0, 0, scr->width, scr->height);
    XFreeGC(disp, gc);
    XSync(disp, False);

    /* prepare to render the image onto the drawable */
    imlib_context_set_image(im);

    /* render the image */
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

    /* change the _XROOTPMAP_ID / ESETROOT_PMAP_ID properties */
    XChangeProperty(disp, root, root, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&drawable, 1);
    XChangeProperty(disp, root, esetroot, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&drawable, 1);

    XSetWindowBackgroundPixmap(disp, root, drawable);
    XClearWindow(disp, root);
    XFlush(disp);

    return 0;
}

static int ishex(int c)
{
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

static char *xcolorstr(const char *color)
{
    static char cstr[sizeof("rgb:00/00/00")] = "rgb:";
    char *ptr;

    if (*color == '#') {
        color++;
    }

    ptr = &cstr[strlen("rgb:")];

    for (int i = 0; i < 3; i++) {
        if (!ishex(color[0]) || !ishex(color[1])) {
            errno = EINVAL;
            return NULL;
        }
        *ptr++ = *color++;
        *ptr++ = *color++;

        if (i < 2) {
            *ptr++ = '/';
        }
    }

    if (*color) {
        errno = EINVAL;
        return NULL;
    }

    *ptr = '\0';

    return cstr;
}

static char *tmpdir(void)
{
    char *var;
    const char *env[] = {
        "TMPDIR", "TMP", "TEMP", "TEMPDIR"
    };

    for (size_t i = 0; i < arysize(env); i++) {
        var = getenv(env[i]);
        if (var) {
            return var;
        }
    }

    return "/tmp";
}

static const char *outpath(const char *path)
{
    if (path) {
        return path;
    } else {
        int rv;
        char *tmp;

        tmp = tmpdir();
        if (!*tmp) {
            tmp = ".";
        }

        rv = snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
                      tmp, DFLTIMG);
        if (rv >= (int)sizeof(pathbuf)) {
            err(1, "path exceeds max length: %s/%s", tmp, DFLTIMG);
        }

        return (const char *)pathbuf;
    }
}

int bkg_set(const char *uri, const char *out, int mode, const char *bgcolor)
{
    const char *xcolor;

    if (!out && access(uri, F_OK) == 0) {
        out = uri;
    } else {
        out = outpath(out);
    }

    /* try to convert hex color -> x color,
     * but if it doesnt work we assume its already x color,
     * like 'black' or something. */
    xcolor = xcolorstr(bgcolor);
    if (!xcolor) {
        xcolor = bgcolor;
    }

    if (strcmp(out, uri) != 0) {
        if (img_copy(out, uri) < 0) {
            return -1;
        }
    }

    return img_set(out, mode, bgcolor);
}
