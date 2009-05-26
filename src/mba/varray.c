/* varray - a variable sized array
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

#include "iterator.h"
#include "varray.h"
#include "msgno.h"

#define VAAL(va) ((struct allocator *)((va)->al ? (char *)(va) - (ptrdiff_t)(va)->al : NULL))
#define BINSIZ(i) ((i) ? 1 << ((i) + (VARRAY_INIT_SIZE - 1)) : (1 << VARRAY_INIT_SIZE))

int
varray_init(struct varray *va, size_t membsize, struct allocator *al)
{
	if (va == NULL || membsize == 0) {
		PMNO(errno = EINVAL);
		return -1;
	}

	memset(va, 0, sizeof *va);
	va->size = membsize;
	if (al && al->tail) { /* al is a suba allocator */
		va->al = (char *)va - (char *)al;
	}

	return 0;
}
int
varray_deinit(struct varray *va)
{
	if (varray_release(va, 0) == -1) {
		AMSG("");
		return -1;
	}
	return 0;
}
struct varray *
varray_new(size_t membsize, struct allocator *al)
{
	struct varray *va;

	if ((va = allocator_alloc(al, sizeof *va, 0)) == NULL) {
		AMSG("");
		return NULL;
	}
	if (varray_init(va, membsize, al) == -1) {
		AMSG("");
		allocator_free(al, va);
		return NULL;
	}

	return va;
}
int
varray_del(void *va)
{
	int ret = 0;

	if (va) {
		ret += varray_release(va, 0);
		ret += allocator_free(VAAL((struct varray *)va), va);
	}

	if (ret) {
		AMSG("");
		return -1;
	}

	return 0;
}
int
varray_release(struct varray *va, unsigned int from)
{
	unsigned int r, i;
	int ret = 0;

	if (va == NULL) {
		return 0;
	}

	r = (1 << VARRAY_INIT_SIZE);
	for (i = 0; i < 16; i++) {
		if (from <= r) {
			break;
		}
		r *= 2;
	}
	if (from != 0) i++;
	for ( ; i < 16; i++) {
		if (va->bins[i]) {
			struct allocator *al = VAAL(va);
			ret += allocator_free(al, ALADR(al, va->bins[i]));
			va->bins[i] = 0;
		}
	}

	if (ret) {
		AMSG("");
		return -1;
	}

	return 0;
}
void *
varray_get(struct varray *va, unsigned int idx)
{
	unsigned int r, i, n;
	struct allocator *al;

	if (va == NULL) {
		PMNO(errno = EINVAL);
		return NULL;
	}

	r = (1 << VARRAY_INIT_SIZE);          /* First and second bins hold 32 then 64,128,256,... */
	for (i = 0; i < 16; i++) {
		if (r > idx) {
			break;
		}
		r *= 2;
	}
	if (i == 16) {
		PMNO(errno = ERANGE);
		return NULL;
	}

	al = VAAL(va);
	n = BINSIZ(i);                                                      /* n is nmemb in bin i */

	if (va->bins[i] == 0) {
		char *mem = allocator_alloc(al, n * va->size, 1);
		if (mem == NULL) {
			AMSG("");
			return NULL;
		}
		va->bins[i] = ALREF(al, mem);
	}

	return (char *)ALADR(al, va->bins[i]) + (i ? idx - n : idx) * va->size;
}
int
varray_index(struct varray *va, void *elem)
{
	ref_t er = ALREF(VAAL(va), elem);
	int i;

	for (i = 0; i < 16; i++) {
		if (va->bins[i]) {
			size_t n = BINSIZ(i);
			ref_t start = va->bins[i];
			ref_t end = start + n * va->size;
			if (er >= start && er < end) {
				return (i ? n : 0) + ((er - start) / va->size);
			}
		}
	}

	PMNO(errno = EFAULT);
	return -1;
}
void
varray_iterate(void *va, iter_t *iter)
{
	if (va && iter) {
		iter->i1 = iter->i2 = 0;
	}
}
void *
varray_next(void *va0, iter_t *iter)
{
	struct varray *va = va0;
	unsigned int n;

	if (va == NULL || iter == NULL) {
		PMNO(errno = EINVAL);
		return NULL;
	}

	/* n is nmemb in iter->i1 */
	n = iter->i1 == 0 ? (1 << VARRAY_INIT_SIZE) :
			1 << (iter->i1 + (VARRAY_INIT_SIZE - 1));

	if (iter->i2 == n) {
		iter->i2 = 0;
		iter->i1++;
	}
	while (va->bins[iter->i1] == 0) {
		iter->i1++;
		if (iter->i1 == 16) {
			return NULL;
		}
	}

	return (char *)ALADR(VAAL(va), va->bins[iter->i1]) + iter->i2++ * va->size;
}

