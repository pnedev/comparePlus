#ifndef MBA_HASHMAP_H
#define MBA_HASHMAP_H

/* hashmap - a rehashing hash map
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

#include <mba/iterator.h>
#include <mba/allocator.h>

#if USE_WCHAR
#define hash_text hash_wcs
#define cmp_text cmp_wcs
#else
#define hash_text hash_str
#define cmp_text cmp_str
#endif

typedef unsigned long (*hash_fn)(const void *object, void *context);
typedef int (*cmp_fn)(const void *object1, const void *object2, void *context);

extern const int table_sizes[];

struct entry;

struct hashmap {
	int table_size_index;
	ref_t hash;
	ref_t cmp;
	ref_t context;
	unsigned int size;
	unsigned int load_factor_high;
	unsigned int load_factor_low;
	ptrdiff_t al;
	ref_t table;
};

LIBMBA_API unsigned long hash_str(const void *str, void *context);
LIBMBA_API unsigned long hash_wcs(const void *wcs, void *context);
LIBMBA_API int cmp_str(const void *object1, const void *object2, void *context);
LIBMBA_API int cmp_wcs(const void *object1, const void *object2, void *context);

LIBMBA_API int hashmap_init(struct hashmap *h,
		unsigned int load_factor,
		hash_fn hash,
		cmp_fn cmp,
		void *context,
		struct allocator *al);

LIBMBA_API int hashmap_deinit(struct hashmap *h, del_fn key_del, del_fn data_del, void *context);
LIBMBA_API struct hashmap *hashmap_new(hash_fn hash, cmp_fn cmp, void *context, struct allocator *al);
LIBMBA_API int hashmap_del(struct hashmap *h, del_fn key_del, del_fn data_del, void *context);
LIBMBA_API int hashmap_clear(struct hashmap *h, del_fn key_del, del_fn data_del, void *context);
LIBMBA_API int hashmap_clean(struct hashmap *h);

LIBMBA_API int hashmap_put(struct hashmap *h, void *key, void *data);
LIBMBA_API int hashmap_is_empty(struct hashmap *h);
LIBMBA_API unsigned int hashmap_size(struct hashmap *h);
LIBMBA_API void *hashmap_get(const struct hashmap *h, const void *key);
LIBMBA_API void hashmap_iterate(void *h, iter_t *iter);
LIBMBA_API void *hashmap_next(void *h, iter_t *iter);
LIBMBA_API int hashmap_remove(struct hashmap *h, void **key, void **data);

#ifdef __cplusplus
}
#endif

#endif /* MBA_HASHMAP_H */

