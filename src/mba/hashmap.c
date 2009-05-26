/* hashmap - a rehashing hash map
 * Copyright (c) 2003 Michael B. Allen <mba2000 ioplex.com>
 *
 * The MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <wchar.h>

#include "msgno.h"
#include "iterator.h"
#include "allocator.h"
#include "suba.h"
#include "hashmap.h"


#define HAL(h) ((struct allocator *)((h)->al ? (char *)(h) - (ptrdiff_t)(h)->al : NULL))

#define TABLE_SIZES_SIZE 20
#define DELETED 1

const int table_sizes[] = {
	0,
	17,      31,      61,      131,
	257,     509,     1021,    2053,
	4099,    8191,    16381,   32771,
	65537,   131071,  262147,  524287,
	1048573, 2097143, 4194301, 8388617
};

struct entry {
	unsigned long hash;
	ref_t key;
	ref_t data;
};

unsigned long
hash_str(const void *str, void *context)
{
	unsigned long h = 5381;
	const unsigned char *s = (const unsigned char *)str;
	int c;

	if (context) { /* then str is ref and context is suba */
		s = (const unsigned char *)context + (size_t)str;
	}

	while ((c = *s++)) {
		h = ((h << 5) + h) + c;
	}

	return h;
}
unsigned long
hash_wcs(const void *wcs, void *context)
{
	unsigned long h = 5381;
	const wchar_t *s = (const wchar_t *)wcs;
	wint_t c;

	if (context) { /* then wcs is ref and context is suba */
		s = (const wchar_t *)context + (size_t)wcs;
	}

	while ((c = *s++)) {
		h = ((h << 5) + h) + c;
	}

	return h;
}
static unsigned long
hash_ptr(const void *ptr, void *context)
{
	unsigned long h = (unsigned long)ptr;

	if (context) {
		h = (unsigned long)((char *)context + (size_t)ptr);
	}

	return h;
}
int
cmp_str(const void *object1, const void *object2, void *context)
{
	const unsigned char *s1 = object1;
	const unsigned char *s2 = object2;
	const unsigned char *slim;

	if (context) {
		s1 = (const unsigned char *)context + (size_t)object1;
		s2 = (const unsigned char *)context + (size_t)object2;
		slim = (const unsigned char *)context + ((struct allocator *)context)->size;
	} else {
		slim = (const unsigned char *)-1;
	}

	while (s1 < slim && s2 < slim) {
		if (*s1 != *s2) {
			return *s1 < *s2 ? -1 : 1;
		} else if (*s1 == '\0') {
			return 0;
		}
		s1++;
		s2++;
	}

	return s2 < slim ? -1 : 1;
}
int
cmp_wcs(const void *object1, const void *object2, void *context)
{
	const wchar_t *s1 = object1;
	const wchar_t *s2 = object2;
	const wchar_t *slim;

	if (context) {
		s1 = (const wchar_t *)((char *)context + (size_t)object1);
		s2 = (const wchar_t *)((char *)context + (size_t)object2);
		slim = (const wchar_t *)((char *)context + ((struct allocator *)context)->size);
	} else {
		slim = (const wchar_t *)-1;
	}

	while (s1 < slim && s2 < slim) {
		if (*s1 != *s2) {
			return *s1 < *s2 ? -1 : 1;
		} else if (*s1 == '\0') {
			return 0;
		}
		s1++;
		s2++;
	}

	return s2 < slim ? -1 : 1;
}

int
hashmap_init(struct hashmap *h,
		unsigned int load_factor,
		hash_fn hash,
		cmp_fn cmp,
		void *context,
		struct allocator *al)
{
	memset(h, 0, sizeof *h);
	h->table_size_index = 0;
	h->hash = ALREF(al, hash);
	h->cmp = ALREF(al, cmp);
	h->context = ALREF(al, context);
	h->size = 0;
	if (load_factor == 0 || load_factor > 100) {
		h->load_factor_high = 75;
		h->load_factor_low = 18;
	} else { 
		h->load_factor_high = load_factor;
		h->load_factor_low = load_factor / 4;
	}
	if (al && al->tail) { /* al is a suba allocator */
		h->al = (char *)h - (char *)al;
	}
	h->table = 0;

	return 0;
}
int
hashmap_deinit(struct hashmap *h, del_fn key_del, del_fn data_del, void *context)
{
	int ret = 0;

	if (h) {
		struct allocator *al = HAL(h);
		ret += hashmap_clear(h, key_del, data_del, context);
		ret += allocator_free(al, ALADR(al, h->table));
	}

	if (ret) {
		AMSG("");
		return -1;
	}

	return 0;
}
struct hashmap *
hashmap_new(hash_fn hash, cmp_fn cmp, void *context, struct allocator *al)
{
	struct hashmap *h;

	if ((h = allocator_alloc(al, sizeof *h, 0)) == NULL ||
				hashmap_init(h, 75, hash, cmp, context, al) == -1) {
		AMSG("");
		allocator_free(al, h);
		return NULL;
	}

	return h;
}
int
hashmap_del(struct hashmap *h, del_fn key_del, del_fn data_del, void *context)
{
	int ret = 0;

	if (h) {
		ret += hashmap_deinit(h, key_del, data_del, context);
		ret += allocator_free(HAL(h), h);
	}

	if (ret) {
		AMSG("");
		return -1;
	}

	return 0;
}
int
hashmap_clear(struct hashmap *h, del_fn key_del, del_fn data_del, void *context)
{
	int ret = 0;

	if (h->table) {
		int idx, table_size;
		struct allocator *al = HAL(h);
		struct entry *table = ALADR(al, h->table);

		table_size = table_sizes[h->table_size_index];

		for (idx = 0; idx < table_size; idx++) {
			struct entry *e = table + idx;

			if (e->key > DELETED) {
				void *k = ALADR(al, e->key);

				if (key_del)
					ret += key_del(context, k);
				if (data_del)
					ret += data_del(context, ALADR(al, e->data));
			}
		}

		ret += allocator_free(al, table);

		h->table_size_index = 0;
		h->size = 0;
		h->table = 0;
	}

	if (ret) {
		AMSG("");
		return -1;
	}

	return 0;
}
int
hashmap_clean(struct hashmap *h)
{
	(void)h; /* TODO */
	return 0;
}
static int
hashmap_resize(struct hashmap *h, int new_table_size_index)
{
	struct entry *old_table, *oe;
	int old_table_size, table_size, i;
	struct allocator *al = HAL(h);

	if ((oe = allocator_alloc(al, table_sizes[new_table_size_index] * sizeof *oe, 1)) == NULL) {
		AMSG("");
		return -1;
	}

	old_table_size = table_sizes[h->table_size_index];
	old_table = ALADR(al, h->table);
	h->table_size_index = new_table_size_index;
	h->table = ALREF(al, oe);

	if (old_table == NULL) {
		return 0;
	}

	table_size = table_sizes[h->table_size_index];
	for (i = 0; i < old_table_size; i++) {
		oe = old_table + i;

		if (oe->key > DELETED) {
			int idx, rehash_adv;
			struct entry *e;

			idx = oe->hash % table_size;
			rehash_adv = 1 + oe->hash % (table_size - 2);

			for ( ;; ) {
				e = (struct entry *)ALADR(al, h->table) + idx;

				if (e->key == 0) {
					*e = *oe;
					break;
				}

				idx = (idx + rehash_adv) % table_size;
			}
		}
	}

	if (allocator_free(al, old_table) == -1) {
		AMSG("");
		return -1;
	}

	return 0;
}
int
hashmap_put(struct hashmap *h, void *key, void *data)
{
	unsigned long hash;
	int idx, rehash_adv, count, table_size;
	struct entry *e;
	struct allocator *al = HAL(h);

	if (h->table_size_index == 0 ||
				((h->size * 100 / table_sizes[h->table_size_index]) >= h->load_factor_high &&
				h->table_size_index < TABLE_SIZES_SIZE)) {
		if (hashmap_resize(h, h->table_size_index + 1) == -1) {
			AMSG("");
			return -1;
		}
	}

	hash = h->hash ? ((hash_fn)ALADR(al, h->hash))(key, ALADR(al, h->context)) : hash_ptr(key, ALADR(al, h->context));
	table_size = table_sizes[h->table_size_index];
	idx = hash % table_size;
	rehash_adv = 1 + hash % (table_size - 2);

	count = table_size;
	do {
		e = (struct entry *)ALADR(al, h->table) + idx;
		if (e->key <= DELETED) {
			break;
		} else {
			void *k = ALADR(al, e->key);

			if (hash == e->hash && (h->cmp ? ((cmp_fn)ALADR(al, h->cmp))(key, k, ALADR(al, h->context)) == 0 : key == k)) {
				errno = EEXIST;
				PMNO(errno);
				return -1;
			}
		}
		idx = (idx + rehash_adv) % table_size;
	} while(--count);

	if (!count) {
		errno = ENOSPC;
		PMNO(errno);
		return -1;
	}

	e->hash = hash;
	e->key = ALREF(al, key);
	e->data = ALREF(al, data);
	h->size++;

	return 0;
}
void *
hashmap_get(const struct hashmap *h, const void *key)
{
	if (h->table) {
		unsigned long hash;
		int idx, rehash_adv, count, table_size;
		struct entry *e;
		struct allocator *al = HAL(h);

		hash = h->hash ? ((hash_fn)ALADR(al, h->hash))(key, ALADR(al, h->context)) : hash_ptr(key, ALADR(al, h->context));
		table_size = table_sizes[h->table_size_index];
		idx = hash % table_size;
		rehash_adv = 1 + hash % (table_size - 2);
	
		count = table_size;
		do {
			e = (struct entry *)ALADR(al, h->table) + idx;
			if (e->key == 0) {
				break;
			} else if (e->key != DELETED) {
				void *k = ALADR(al, e->key);

				if (hash == e->hash && (h->cmp ? ((cmp_fn)ALADR(al, h->cmp))(key, k, ALADR(al, h->context)) == 0 : key == k)) {
					return ALADR(al, e->data);
				}
			}
			idx = (idx + rehash_adv) % table_size;
		} while(count--);
	}

	return NULL;
}
int
hashmap_remove(struct hashmap *h, void **key, void **data)
{
	if (h->table) {
		unsigned long hash;
		int idx, rehash_adv, count, table_size;
		struct entry *e;
		struct allocator *al = HAL(h);

		if (h->table_size_index > 1 &&
					(h->size * 100 / table_sizes[h->table_size_index]) < h->load_factor_low) {
			if (hashmap_resize(h, h->table_size_index - 1) == -1) {
				AMSG("");
				return -1;
			}
		}

		hash = h->hash ? ((hash_fn)ALADR(al, h->hash))(*key, ALADR(al, h->context)) : hash_ptr(*key, ALADR(al, h->context));
		table_size = table_sizes[h->table_size_index];
		idx = hash % table_size;
		rehash_adv = 1 + hash % (table_size - 2);

		count = table_size;
		do {
			e = (struct entry *)ALADR(al, h->table) + idx;
			if (e->key == 0) {
				break;
			} else if (e->key != DELETED) {
				void *k = ALADR(al, e->key);

				if (hash == e->hash && (h->cmp ? ((cmp_fn)ALADR(al, h->cmp))(*key, k, ALADR(al, h->context)) == 0 : *key == k)) {
					*key = k;
					*data = ALADR(al, e->data);
					e->key = DELETED;
					h->size--;
					return 0;
				}
			}
			idx = (idx + rehash_adv) % table_size;
		} while(count--);
	}

	*data = NULL;

	errno = ENOENT;
	PMNO(errno);

	return -1;
}
int
hashmap_is_empty(struct hashmap *h)
{
	return h == NULL || h->size == 0;
}
unsigned int
hashmap_size(struct hashmap *h)
{
	return h ? h->size : 0;
}
void
hashmap_iterate(void *h, iter_t *iter)
{
	memset(iter, 0, sizeof *iter);
	(void)h;
}
void *
hashmap_next(void *h0, iter_t *iter)
{
	struct hashmap *h = h0;

	if (h->table) {
		int idx, table_size;
		struct entry *e;
		struct allocator *al = HAL(h);

		table_size = table_sizes[h->table_size_index];

		idx = iter->i2;
		while (idx < table_size) {
			e = (struct entry *)ALADR(al, h->table) + idx++;
			if (e->key > DELETED) {
				iter->i2 = idx;
				return ALADR(al, e->key);
			}
		}
	}

	return NULL;
}

