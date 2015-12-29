#ifndef WPCP_UTIL_H
#define WPCP_UTIL_H

#include "wpcp.h"

#ifndef WPCP_DISABLE_ASSERT
#include <assert.h>
#define WPCP_ASSERT(x) assert(x)
#else
#define WPCP_ASSERT(x)
#endif

#if defined(_MSC_VER)
#define WPCP_STATIC_INLINE static __inline
#else
#define WPCP_STATIC_INLINE static inline
#endif

#define WPCP_COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

WPCP_BEGIN_EXTERN_C

#ifndef WPCP_USE_CUSTOM_ALLOCATOR

#include <stdlib.h>
#define wpcp_malloc malloc
#define wpcp_realloc realloc
#define wpcp_calloc calloc
#define wpcp_free free

#else

void* wpcp_malloc(size_t size);
void* wpcp_calloc(size_t count, size_t size);
void* wpcp_realloc(void* data, size_t size);
void wpcp_free(void* data);

#endif

WPCP_END_EXTERN_C

#endif
