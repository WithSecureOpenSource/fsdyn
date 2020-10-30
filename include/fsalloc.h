#ifndef __FSALLOC__
#define __FSALLOC__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*fs_realloc_t)(void *, size_t);
typedef void (*fs_reallocator_counter_t)(ssize_t count);

/* Set a custom reallocator. Unlike POSIX realloc(), the argument
 * function must deallocate a memory block if size is zero. */
void fs_set_reallocator(fs_realloc_t realloc);
fs_realloc_t fs_get_reallocator(void);

void *fsalloc(size_t size)
  __attribute__((warn_unused_result));

/* NULL is an acceptable argument value. */
void fsfree(void *ptr);

/* If size is zero, the memory element is deallocated. */
void *fsrealloc(void *ptr, size_t size);
void *fscalloc(size_t nmemb, size_t size)
  __attribute__((warn_unused_result));

/* Set a reallocator counter. A reallocator counter is an optional
 * test facility offered to custom reallocators. The purpose of the
 * reallocator counter is to keep track of the number of currently
 * allocated memory blocks.
 *
 * It is up to the custom reallocator to keep track of its own
 * allocation statistics. Thus, the reallocator counter is not invoked
 * by fsalloc() et al. See fs_reallocator_skew() for how the
 * reallocator counter is intended to be used. */
void fs_set_reallocator_counter(fs_reallocator_counter_t counter);

/* Return the current reallocator counter. */
fs_reallocator_counter_t fs_get_reallocator_counter(void);

/* A customer reallocator may want to maintain statistics of
 * outstanding memory blocks. Typically, the application may want to
 * test that at the end of the execution, the number of allocated
 * blocks is zero. However, there are valid cases where libraries make
 * permanent memory allocations and spoil the statistic. The function
 * fs_reallocator_skew() is a mechanism for such a library function to
 * restore meaning to the count. When it makes a permanent memory
 * allocation of a single memory block, the library function should
 * call
 *
 *   fs_reallocator_skew(-1);
 *
 * which in turn causes the reallocator counter to be called with -1
 * as an argument. If the library function should free a "permanent"
 * block after all, it should call
 *
 *   fs_reallocator_skew(1);
 *
 * to balance the books. */
void fs_reallocator_skew(ssize_t count);

#ifdef __cplusplus
}

#include <functional>
#include <memory>

namespace fsecure {
namespace fsdyn {

template <typename T>
using AllocPtr = std::unique_ptr<T, std::function<void(void *)>>;

// Usage example:
// auto char_array = make_fsalloc_ptr<char>(fsalloc(size));
template <typename T>
inline AllocPtr<T> make_fsalloc_ptr(void *thing)
{
    return { static_cast<T *>(thing), fsfree };
}

} // namespace fsdyn
} // namespace fsecure

#endif

#endif
