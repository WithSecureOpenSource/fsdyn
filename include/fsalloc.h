#ifndef __FSALLOC__
#define __FSALLOC__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*fs_realloc_t)(void *, size_t);

void fs_set_reallocator(fs_realloc_t realloc);
fs_realloc_t fs_get_reallocator(void);

void *fsalloc(size_t size)
  __attribute__((warn_unused_result));
void fsfree(void *ptr);
void *fsrealloc(void *ptr, size_t size)
  __attribute__((warn_unused_result));
void *fscalloc(size_t nmemb, size_t size)
  __attribute__((warn_unused_result));

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
