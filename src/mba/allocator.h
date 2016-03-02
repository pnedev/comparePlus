#ifndef MBA_ALLOCATOR_H
#define MBA_ALLOCATOR_H

/* allocator - allocate and free memory
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBMBA_API
#ifdef WIN32
# ifdef LIBMBA_EXPORTS
#  define LIBMBA_API  __declspec(dllexport)
# else /* LIBMBA_EXPORTS */
#  define LIBMBA_API  __declspec(dllimport)
# endif /* LIBMBA_EXPORTS */
#else /* WIN32 */
# define LIBMBA_API extern
#endif /* WIN32 */
#endif /* LIBMBA_API */

#include <stddef.h>

#define ALAL(a) ((a) && (a) != stdlib_allocator ? (a) : 0)
#define ALREF(a,p) ((size_t)((p) ? (char *)(p) - (char *)ALAL(a) : 0))
#define ALADR(a,r) ((void *)((r) ? (char *)ALAL(a) + (r) : NULL))

struct allocator;

typedef void *(*alloc_fn)(struct allocator *al, size_t size, int flags);
typedef void *(*realloc_fn)(struct allocator *al, void *obj, size_t size);
typedef int (*free_fn)(void *al, void *obj);

struct allocator {
	alloc_fn alloc;
	realloc_fn realloc;
	free_fn free;
};

extern struct allocator *stdlib_allocator;

extern void *allocator_alloc(struct allocator *al, size_t size, int flags);
extern void *allocator_realloc(struct allocator *al, void *obj, size_t size);
extern int allocator_free(void *al, void *obj);

#ifdef __cplusplus
}
#endif

#endif /* MBA_ALLOCATOR_H */

