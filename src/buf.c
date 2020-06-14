#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <err.h>

#include "buf.h"

struct buf {
    char *buf;
    size_t wndx;
    size_t rndx;
    size_t pndx;
    size_t cap;
    size_t len;
    size_t rwlen;
    size_t total;
};

buf_t *buf_new(size_t cap)
{
    buf_t *io;

    io = (buf_t *)malloc(sizeof(buf_t));
    if (!io) {
        err(EXIT_FAILURE, "allocation failed");
    }

    memset(io, 0, sizeof(buf_t));
    io->buf = (char *)malloc(cap);
    if (!io->buf) {
        err(EXIT_FAILURE, "allocation failed");
    }

    io->cap = cap;

    return io;
}

int buf_close(buf_t *io)
{
    free(io->buf);
    io->buf = NULL;
    free(io);
    return 0;
}

size_t buf_len(buf_t *io)
{
    return io->len;
}

char *buf_ptr(buf_t *io, int close)
{
    char *ptr;

    ptr = io->buf;
    if (close) {
        io->buf = NULL;
        buf_close(io);
    }

    return ptr;
}

int buf_expand(buf_t *io, size_t len)
{
    return buf_expand_to(io, io->len + len);
}

int buf_expand_to(buf_t *io, size_t len)
{
    void *tmp;
    size_t max, dbl;

    if (len <= io->cap) {
        return 0;
    }

    dbl = io->cap * 2;
    /* either double current capacity,
     * or allocate enough for the data */
    max = dbl > len ? dbl : len;

    /* reallocate the buffer */
    tmp = realloc(io->buf, max);
    if (!tmp) {
        err(EXIT_FAILURE, "allocation failed");
    } else {
        io->buf = tmp;
    }

    /* now unwrap any circled data */
    if (io->wndx < io->rndx) {
        /* realloc: [aaaa--aa]
         *              ^  ^ ^
         *              w  r c
         * result:  [aaaa--aa--------]
         *              ^  ^ ^
         *              w  r c
         * unwrap:  [------aaaaaa----]
         *          ^   ^  ^ ^  ^
         *          b   p  r c  w
         * b:  io->buf
         * r:  io->rndx
         * w:  io->wndx
         * p:  io->pndx
         * c:  io->cap */
        memcpy(io->buf + io->cap, io->buf, io->wndx);
        io->wndx = io->wndx + io->cap;
        /* at minimum, the buffer size is doubled. this means that the
         * entirety of the current buffer can fit in the buffer, so we
         * dont need to worry about memmoving leftover dat backward */
    }

    /* now set the new capacity */
    io->cap = max;

    return 0;
}

int buf_write(buf_t *io, const char *buf, size_t len)
{
    size_t next;

    /* ensure we have enough space */
    buf_expand(io, len);

    /* calculate the forward index */
    next = (io->wndx + len) % io->cap;

    /* single copy, next == 0 implies we use the entire buffer */
    if (next > io->wndx || next == 0) {

        memcpy(io->buf + io->wndx, buf, len);

        /* if pndx should be moved */
        if ((io->pndx > io->wndx ||
            (io->pndx == io->wndx && io->rwlen)) &&
            io->wndx + len > io->pndx)
        {
            /* rewind index is being moved by this write */
            io->wndx = next;
            io->rwlen = io->rwlen - next - io->pndx;
            io->pndx = next;
        } else {
            io->wndx = io->wndx + len;
        }
    } else {
        size_t fwd, bkwd;
        /* front copy len */
        fwd = io->cap - io->wndx;
        /* back copy len */
        bkwd = len - fwd;

        /* the first part of the buffer */
        memcpy(io->buf + io->wndx, buf, fwd);
        /* the seond part of the buffer */
        memcpy(io->buf, buf + fwd, bkwd);

        if (io->pndx >= io->wndx) {
            io->rwlen -= len - (io->pndx - io->wndx);
            io->wndx = bkwd;
            io->pndx = io->wndx;
        } else if (bkwd > io->pndx) {
            io->wndx = bkwd;
            io->rwlen = io->rwlen - (bkwd - io->pndx);
            io->pndx = io->wndx;
        } else {
            io->wndx = bkwd;
        }
    }

    io->len += len;
    io->total += len;

    /* dont return bytes written to avoid confusion
     * (this is a pass/fail function) */
    return 0;
}

int buf_read(buf_t *io, char *buf, size_t len)
{
    size_t next;

    if (!io->len) {
        return 0;
    }

    /* clamp the read length to the available data */
    if (io->len < len) {
        len = io->len;
    }

    /* also clamp to INT_MAX for the return value */
    if (io->len > INT_MAX) {
        len = INT_MAX;
    }

    next = (io->rndx + len) % io->cap;

    /* check if the read is wrapped around */
    if (next > io->rndx) {
        /* single copy */
        memcpy(buf, &io->buf[io->rndx], len);
        io->rndx = next;
    } else {
        size_t consume;
        /* the part not wrapped */
        consume = len - next;
        /* copy in parts */
        memcpy(buf, &io->buf[io->rndx], consume);
        memcpy(buf + consume, io->buf, next);

        io->rndx = next;
    }

    io->len -= len;
    io->rwlen += len;

    return (int)len;
}

int buf_rewind(buf_t *io, size_t len)
{
    /* clamp length to max possible */
    len = len > INT_MAX ? INT_MAX : len;
    len = len > io->rwlen ? io->rwlen : len;

    io->rwlen -= len;
    if (len <= io->rndx) {
        io->rndx -= len;
    } else {
        io->rndx = io->cap - len;
    }

    io->total -= len;
    io->len += len;

    return (int)len;
}
