#ifndef buf_h_INCLUDED
#define buf_h_INCLUDED

#include <stddef.h>

typedef struct buf buf_t;

buf_t *buf_new(size_t cap);
int buf_close(buf_t *io);
size_t buf_len(buf_t *io);
char *buf_ptr(buf_t *io, int close);
int buf_expand(buf_t *io, size_t len);
int buf_expand_to(buf_t *io, size_t len);
int buf_write(buf_t *io, const char *buf, size_t len);
int buf_read(buf_t *io, char *buf, size_t len);

size_t buf_rewind(buf_t *io, size_t len);
size_t buf_seek(buf_t *io, size_t len);
size_t buf_tell(buf_t *io);

int dbg_buf(buf_t *io);

#endif // buf_h_INCLUDED
