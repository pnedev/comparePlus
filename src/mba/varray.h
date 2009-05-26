#ifndef MBA_VARRAY_H
#define MBA_VARRAY_H

/* varray - a variable sized array
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
#include <mba/allocator.h>
#include <mba/iterator.h>

/*
0  1      32768
1  2      65536
2  4      131072
3  8      262144
4  16     524288
5  32     1048576  2^5 is default VARRAY_INIT_SIZE
6  64     2097152
7  128    4194304
8  256    8388608
9  512
10 1024
11 2048
12 4096
13 8192
14 16384
15 32768
16 65536
17 131072
18 262144
19 524288
20 1048576
21 2097152
22 4194304
23 8388608
*/

#ifndef VARRAY_INIT_SIZE
#define VARRAY_INIT_SIZE 5
#endif

struct varray {
	size_t size;                                          /* element size */
	ptrdiff_t al;  /* relative offset of this object to allocator of bins */
	ref_t bins[16];                                 /* 0 to 2^20 elements */
};

LIBMBA_API int varray_init(struct varray *va, size_t membsize, struct allocator *al);
LIBMBA_API int varray_reinit(struct varray *va, struct allocator *al);
LIBMBA_API int varray_deinit(struct varray *va);
LIBMBA_API struct varray *varray_new(size_t membsize, struct allocator *al);
LIBMBA_API int varray_del(void *va);
LIBMBA_API int varray_release(struct varray *va, unsigned int from);
LIBMBA_API void *varray_get(struct varray *va, unsigned int idx);
LIBMBA_API int varray_index(struct varray *va, void *elem);
LIBMBA_API void varray_iterate(void *va, iter_t *iter);
LIBMBA_API void *varray_next(void *va, iter_t *iter);

#ifdef __cplusplus
}
#endif

#endif /* MBA_VARRAY_H */
