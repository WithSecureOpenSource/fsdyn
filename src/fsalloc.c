#include <string.h>
#include "fsalloc.h"
#include "avltree_version.h"

static void *naive_realloc(void *ptr, size_t size)
{
#ifndef __linux__
    if (size == 0) {
        if (ptr != NULL)
            free(ptr);
        return NULL;
    }
#endif
    void *obj = realloc(ptr, size);
    if (obj == NULL && size != 0)
        abort();
    return obj;
}

static fs_realloc_t reallocator = naive_realloc;

void fs_set_reallocator(fs_realloc_t realloc)
{
    reallocator = realloc;
}

fs_realloc_t fs_get_reallocator(void)
{
    return reallocator;
}

void *fsalloc(size_t size)
{
    return reallocator(NULL, size);
}

void fsfree(void *ptr)
{
    if (ptr)
        reallocator(ptr, 0);
}

void *fsrealloc(void *ptr, size_t size)
{
    return reallocator(ptr, size);
}

void *fscalloc(size_t nmemb, size_t size)
{
    size *= nmemb;
    void *obj = fsalloc(size);
    if (obj != NULL)
        memset(obj, 0, size);
    return obj;
}
