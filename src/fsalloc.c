#include <string.h>
#include "fsalloc.h"
#include "fsdyn_version.h"

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

static void dummy_reallocator_counter(ssize_t count)
{
}

static fs_reallocator_counter_t reallocator_counter =
    dummy_reallocator_counter;

void fs_set_reallocator_counter(fs_reallocator_counter_t counter)
{
    reallocator_counter = counter;
}

fs_reallocator_counter_t fs_get_reallocator_counter()
{
    return reallocator_counter;
}

void fs_reallocator_skew(ssize_t count)
{
    reallocator_counter(count);
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
