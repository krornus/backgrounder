#ifndef uri_h_INCLUDED
#define uri_h_INCLUDED

#include <stddef.h>

typedef struct uri URI;

URI *uopen(const char *path, const char *op);
void uclose(URI *uri);

int uread(void *buf, size_t size, size_t nmemb, URI *uri);
int urewind(URI *uri, size_t len);

#endif // uri_h_INCLUDED
