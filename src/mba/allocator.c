/* allocator - allocate and free memory
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
#include <errno.h>

#include "allocator.h"
#include "suba.h"
#include "msgno.h"

void *
allocator_alloc(struct allocator *al, size_t size, int zero)
{
	void *p;

	if (!al) {
		al = global_allocator ? global_allocator : stdlib_allocator;
	}

	if (al->tail) { /* fn ptr in shared mem may be invalid */
		p = suba_alloc(al, size, zero);
	} else {
		p = al->alloc(al, size, zero);
	}
	if (p == NULL) {
		AMSG("");
	}

	return p;
}
void *
allocator_realloc(struct allocator *al, void *obj, size_t size)
{
	void *p;

	if (!al) {
		al = global_allocator ? global_allocator : stdlib_allocator;
	}

	if (al->tail) { /* fn ptr in shared mem may be invalid */
		p = suba_realloc(al, obj, size);
	} else {
		p = al->realloc(al, obj, size);
	}
	if (p == NULL && size) {
		AMSG("");
	}

	return p;
}
int
allocator_free(void *al0, void *obj)
{
	struct allocator *al = al0;

	if (!al) {
		al = global_allocator ? global_allocator : stdlib_allocator;
	}

	if (al->tail) { /* fn ptr in shared mem may be invalid */
		if (suba_free(al, obj) == -1) {
			AMSG("");
			return -1;
		}
	} else if (al->free(al, obj) == -1) {
		AMSG("");
		return -1;
	}

	return 0;
}
void
allocator_set_reclaim(struct allocator *al, reclaim_fn recl, void *arg)
{
	if (!al) {
		if (global_allocator) {
			al = global_allocator;
		} else {
			return; /* stdlib_allocator does not support reclaim_fn */
		}
	}

	al->reclaim = recl;
	al->reclaim_arg = arg;
}

void *
stdlib_alloc(struct allocator *al, size_t size, int zero)
{
	void *p;

	if (zero) {
		p = calloc(1, size);
	} else {
		p = malloc(size);
	}
	if (p == NULL) {
		PMNO(errno);
		return NULL;
	}

	(void)al;
	return p;
}
void *
stdlib_realloc(struct allocator *al, void *obj, size_t size)
{
	void *p;

	if ((p = realloc(obj, size)) == NULL && size) {
		PMNO(errno);
	}

	(void)al;
	return p;
}
int
stdlib_free(void *al, void *obj)
{
	free(obj);
	(void)al;
	return 0;
}

struct allocator stdlib_allocator0 = {
	"", 0, 0, 0, 0, 0, 0, 0,
	&stdlib_alloc,
	&stdlib_realloc,
	&stdlib_free,
	NULL, NULL, 0, 0
};

struct allocator *stdlib_allocator = &stdlib_allocator0;
struct allocator *global_allocator = NULL;

