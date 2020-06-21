#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include "uri.h"
#include "buf.h"

#ifndef OFF64_MAX
#define OFF64_MAX INT64_MAX
#endif

typedef struct uri uri_t;

static CURLM *g_multi;

struct uri {
    CURL *curl;
    buf_t *io;
    int alive;
    size_t len;
    int have_len;
};

static size_t uri_write(char *buf, size_t size, size_t nmemb, void *arg)
{
    int rv;
    uri_t *uri;

    uri = (uri_t *)arg;
    if (!uri->have_len) {
        rv = curl_easy_getinfo(uri->curl,
                               CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
                               (curl_off_t *)&uri->len);
        if (rv == CURLE_OK) {
            uri->have_len = 1;
        }
    }

    if (uri->have_len) {
        buf_expand_to(uri->io, uri->len);
    }

    rv = buf_write(uri->io, buf, size * nmemb);
    if (rv == 0) {
        return size * nmemb;
    } else {
        return 0;
    }
}

static int uri_read(uri_t *uri, size_t len)
{
    int maxfd;
    int rv;
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    struct timeval timeout;
    long curl_timeout;

    if (!uri->alive || buf_len(uri->io) >= len) {
        return 0;
    }

    /* read in a loop until select reports that
     * the read would block, or the length is
     * satisfied */
    do {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        /* set a suitable timeout to fail on */
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        /* get the timeout if set */
        curl_multi_timeout(g_multi, &curl_timeout);
        if (curl_timeout >= 0) {
            timeout.tv_sec = curl_timeout / 1000;
            if (timeout.tv_sec > 1) {
                timeout.tv_sec = 1;
            } else {
                timeout.tv_usec = (curl_timeout % 1000) * 1000;
            }
        }

        /* get file descriptors from the transfers */
        rv = curl_multi_fdset(g_multi,
                              &readfds,
                              &writefds,
                              &exceptfds,
                              &maxfd);

        if (rv != CURLM_OK) {
            return -1;
        }

        if (maxfd == -1) {
            /* sleep 100ms and loop */
            usleep(100000);
            rv = 0;
        } else {
            /* select on the fds */
            rv = select(maxfd + 1, &readfds, &writefds, &exceptfds, &timeout);
        }

        if (rv >= 0) {
            /* perform the read */
            curl_multi_perform(g_multi, &uri->alive);
        }
    } while(rv >= 0 && uri->alive && buf_len(uri->io) < len);

    return 0;
}

static int uclose(uri_t *uri)
{
    if (uri) {
        buf_close(uri->io);
        curl_multi_remove_handle(g_multi, uri->curl);
        curl_easy_cleanup(uri->curl);
        free(uri);
    }

    return 0;
}

static ssize_t uread(uri_t *uri, char *buf, size_t size)
{
    int rv;

    if (uri_read(uri, size) < 0) {
        return EOF;
    } else if (!buf_len(uri->io)) {
        return 0;
    }

    rv = buf_read(uri->io, buf, size);
    if (rv < 0) {
        return EOF;
    } else {
        return rv;
    }
}

static int useek(uri_t *uri, off64_t *offset, int whence)
{
    int rewind;
    size_t len, loc;

    if (whence == SEEK_SET) {
        if (*offset < 0) {
            errno = EINVAL;
            return -1;
        } else {
            if ((size_t)*offset < buf_tell(uri->io)) {
                rewind = 1;
                len = (size_t)(buf_tell(uri->io) - *offset);
            } else {
                rewind = 0;
                len = (size_t)(*offset - buf_tell(uri->io));
            }
        }
    } else if (whence == SEEK_CUR) {
        if (*offset > 0) {
            rewind = 0;
            len = (size_t)*offset;
        } else {
            rewind = 1;
            len = (size_t)(*offset * -1);
        }
    } else if (whence == SEEK_END) {
        if (*offset > 0) {
            errno = EINVAL;
            return -1;
        } else {
            /* read everything */
            if (uri_read(uri, SIZE_MAX) < 0) {
                return -1;
            }

            /* seek to end */
            buf_seek(uri->io, buf_len(uri->io));
            assert(!buf_len(uri->io));

            /* now just rewind */
            rewind = 1;
            len = (size_t)(*offset * -1);
        }
    }

    if (!rewind) {
        /* uri read will exit early if we have enough */
        if (uri_read(uri, len) < 0) {
            return -1;
        }
        /* now skip */
        buf_seek(uri->io, len);
    } else {
        /* go backwards by amount, this may
         * be less than requested */
        buf_rewind(uri->io, len);
    }

    /* update offset */
    loc = buf_tell(uri->io);
    if (loc > OFF64_MAX) {
        errno = EOVERFLOW;
        return -1;
    } else {
        *offset = (off64_t)loc;
    }

    return 0;
}

static cookie_io_functions_t uri_io_funcs = {
    .read = (cookie_read_function_t *)uread,
    .seek = (cookie_seek_function_t *)useek,
    .close = (cookie_close_function_t *)uclose,
};

FILE *uopen(const char *path)
{
    FILE *fp;
    uri_t *uri;

    /* short-circuit to file */
    fp = fopen(path, "r");
    if (fp) {
        return fp;
    }

    uri = malloc(sizeof(uri_t));
    if (!uri) {
        return NULL;
    }

    uri->curl = curl_easy_init();
    uri->io = buf_new(2048);
    uri->have_len = 0;

    curl_easy_setopt(uri->curl, CURLOPT_URL, path);
    curl_easy_setopt(uri->curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(uri->curl, CURLOPT_WRITEDATA, uri);
    curl_easy_setopt(uri->curl, CURLOPT_WRITEFUNCTION, uri_write);

    if(!g_multi) {
        g_multi = curl_multi_init();
        if (!g_multi) {
            err(EXIT_FAILURE, "failed to initialize curl");
        }
    }

    if (curl_multi_add_handle(g_multi, uri->curl) != CURLM_OK) {
        buf_close(uri->io);
        free(uri);
        return NULL;
    }

    if (curl_multi_perform(g_multi, &uri->alive) != CURLM_OK) {
        buf_close(uri->io);
        curl_multi_remove_handle(g_multi, uri->curl);
        free(uri);
        return NULL;
    }

    if (!buf_len(uri->io) && !uri->alive) {
        errno = ENOENT;
        uclose(uri);
        return NULL;
    }

    return fopencookie(uri, "r", uri_io_funcs);
}
