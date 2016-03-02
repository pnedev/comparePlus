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

#include "varray.h"
#include "msgno.h"

#define VAAL(va) ((struct allocator *)((va)->al ? (char *)(va) - (ptrdiff_t)(va)->al : NULL))
#define BINSIZ(i) ((i) ? 1 << ((i) + (VARRAY_INIT_SIZE - 1)) : (1 << VARRAY_INIT_SIZE))

int
varray_init(struct varray *va, size_t membsize, struct allocator *al)
{
	if (va == NULL || membsize == 0) {
		errno = EINVAL;
		MsgA("varray_init failed");
		return -1;
	}

	memset(va, 0, sizeof *va);
	va->size = membsize;

	return 0;
}

int
varray_deinit(struct varray *va)
{
	if (varray_release(va, 0) == -1) {
		MsgA("varray_deinit failed");
		return -1;
	}
	return 0;
}

struct varray *
varray_new(size_t membsize, struct allocator *al)
{
	struct varray *va;

	if ((va = allocator_alloc(al, sizeof *va, 0)) == NULL) {
		MsgA("varray_new, allocator_alloc failed");
		return NULL;
	}
	if (varray_init(va, membsize, al) == -1) {
		MsgA("varray_new, varray_init failed");
		allocator_free(al, va);
		return NULL;
	}

	return va;
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
		MsgA("varray_release failed");
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
		errno = EINVAL;
		MsgA("varray_get failed");
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
		errno = ERANGE;
		MsgA("varray_get failed");
		return NULL;
	}

	al = VAAL(va);
	n = BINSIZ(i);                                                      /* n is nmemb in bin i */

	if (va->bins[i] == 0) {
		char *mem = allocator_alloc(al, n * va->size, 1);
		if (mem == NULL) {
			MsgA("varray_get, allocator_alloc failed");
			return NULL;
		}
		va->bins[i] = ALREF(al, mem);
	}

	return (char *)ALADR(al, va->bins[i]) + (i ? idx - n : idx) * va->size;
}
