/* suba - sub-allocate memory from larger chunk of memory
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
#include <stdio.h>
#include <errno.h>

#include "suba.h"
#include "dbug.h"
#include "msgno.h"

struct cell {
	size_t size;
/*	void *stk[4]; */
	int line;
	ref_t next; /* reference to next cell in free list */
};

#define ALIGNMASK 7UL
#define ALIGN(s) (((s) + ALIGNMASK) & ~ALIGNMASK)
#define POFF ALIGN(offsetof(struct cell, next))
#define C2P(c) ((char *)(c) + POFF)
#define P2C(p) ((struct cell *)((char *)(p) - POFF))
#define ISADJ(c1,c2) ((struct cell *)(C2P(c1) + (c1)->size) == (struct cell *)(c2))
#define SREF(s,p) (ref_t)((char *)(p) - (char *)(s))
#define SADR(s,r) (void *)((char *)(s) + (r))
#define RECLAIM_DEPTH_MAX 2
#define SUBA_MAGIC "\xFF\x15\x15\x15SUBA"

int suba_print_free_list(struct allocator *suba);

void *
suba_addr(const struct allocator *suba, const ref_t ref)
{
	if (suba && ref > 0 && ref <= suba->size) {
		return (char *)suba + ref;
	}
	return NULL;
}
ref_t
suba_ref(const struct allocator *suba, const void *ptr)
{
	if (suba && ptr) {
		ref_t ref = (char *)ptr - (char *)suba;
		if (ref > 0 && ref <= suba->size) {
			return ref;
		}
	}
	return 0;
}

struct allocator *
suba_init(void *mem, size_t size, int rst, size_t mincell)
{
	struct allocator *suba = mem;
	size_t hdrsiz;

	hdrsiz = ALIGN(sizeof *suba);

	if (mem == NULL || size <= (hdrsiz + POFF) ||
			(!rst && memcmp(SUBA_MAGIC, suba->magic, 8)) != 0) {
		PMNO(errno = EINVAL);
		return NULL;
	}

	if (rst) {
		struct cell *c;

		memset(suba, 0, hdrsiz);
		memcpy(suba->magic, SUBA_MAGIC, 8);
		suba->tail = hdrsiz;
        /* cell data must be large enough for next ref_t */
		suba->mincell = ALIGN(sizeof(size_t));
		if (mincell > suba->mincell) {
			suba->mincell = ALIGN(mincell);
		}
		suba->size = suba->max_free = size;

		c = suba_addr(suba, hdrsiz);
		c->size = size - (hdrsiz + POFF);
		c->next = suba->tail;
	}

	return suba;
}
void *
suba_alloc(struct allocator *suba, size_t size, int zero)
{
	struct cell *c1, *c2, *c3;
	size_t s = size;
	int reclaim = 0;

	size = size < suba->mincell ? suba->mincell : ALIGN(size);

again:
	if (reclaim) {
		int progress = 0;

		if (suba->reclaim && suba->reclaim_depth <= RECLAIM_DEPTH_MAX) {
			suba->reclaim_depth++;
			progress = suba->reclaim(suba, suba->reclaim_arg, reclaim);
			suba->reclaim_depth--;
		}

		if (!progress) {
			PMNO(errno = ENOMEM);
			return NULL;
		}
	}

	c2 = SADR(suba, suba->tail);
	for ( ;; ) {
		c1 = c2;
		if ((c2 = suba_addr(suba, c1->next)) == NULL) {
			PMNF(errno = EFAULT, ": 0x%08x", c1->next);
			return NULL;
		}
		if (c2->size >= size) {
			break;       /* found a cell large enough */
		}
		if (c1->next == suba->tail) {
			reclaim++;
			goto again;
		}
	}

	if (c2->size > (POFF + size + suba->mincell)) {
									/* split new cell */
		c3 = (struct cell *)(C2P(c2) + size);
		c3->size = c2->size - (size + POFF);
		if (c1 == c2) {
			c1 = c3;
		} else {
			c3->next = c2->next;
		}
		c1->next = SREF(suba, c3);
		c2->size = size;
		if (c2 == SADR(suba, suba->tail)) {
			suba->tail = SREF(suba, c3);
		}
	} else if (c1->next == suba->tail) {
                          /* never use the last cell! */
		reclaim++;
		goto again;
	} else {                   /* use the entire cell */
		c1->next = c2->next;
	}

	suba->alloc_total += POFF + c2->size;
	suba->size_total += s;

/*
	dbug_stacktrace(c2->stk, 1, 4);

suba_print_cell(suba, "ALLOC", c2);
 */

	return zero ? memset(C2P(c2), 0, size) : C2P(c2);
}
int
suba_free(void *suba0, void *ptr)
{
	struct allocator *suba = suba0;
	struct cell *c1, *c2, *c3;
	ref_t ref;
	int j1, j2;

	if (!ptr) return 0;

	if (!suba_ref(suba, ptr)) {
		PMNO(errno = EFAULT);
		return -1;
	}
                /* splice the cell back into the list */
	c1 = SADR(suba, suba->tail);
	c2 = P2C(ptr);
	if (c2->size > suba->max_free || (ref = suba_ref(suba, c2)) == 0) {
		PMNF(errno = EINVAL, ": %p: %d", ptr, c2->size);
		return -1;
	}

	suba->free_total += POFF + c2->size;
/*
	c2->stk[0] = NULL;

suba_print_cell(suba, " FREE", c2);
 */

	if (c2 > c1) {           /* append to end of list */
		if (ISADJ(c1,c2)) {    /* join with last cell */
			c1->size += POFF + c2->size;
			return 0;
		}
		c2->next = c1->next;
		suba->tail = c1->next = ref;

		return 0;
	}

	while (c1->next < ref) {   /* find insertion point */
		if (c1->next < POFF) {
			PMNF(errno = EINVAL, ": next ref corrupted: %d", c1->next);
			return -1;
		}
		c1 = SADR(suba, c1->next);
	}
	c3 = SADR(suba, c1->next);

	j1 = ISADJ(c1,c2); /* c1 and c2 need to be joined */
	j2 = ISADJ(c2,c3); /* c2 and c3 need to be joined */

	if (j1) {
		if (j2) {  /* splice all three cells together */
			if (SREF(suba, c3) == suba->tail) {
				suba->tail = SREF(suba, c1);
			}
			c1->next = c3->next;
			c1->size += POFF + c3->size;
		}
		c1->size += POFF + c2->size;
	} else {
		if (j2) {
			if (SREF(suba, c3) == suba->tail) {
				suba->tail = ref;
			}
			c2->next = c3->next == SREF(suba, c3) ? ref : c3->next;
			c2->size += POFF + c3->size;
		} else {
			c2->next = c1->next;
		}
		c1->next = ref;
	}

	return 0;
}
void *
suba_realloc(struct allocator *suba, void *ptr, size_t size)
{
	struct cell *c;
	void *p;

	if (ptr == NULL) {
		if ((p = suba_alloc(suba, size, 0)) == NULL) {
			AMSG("");
		}
		return p;
	}
	if (size == 0) {
		suba_free(suba, ptr);
		return NULL;
	}
	c = P2C(ptr);
	if (c->size < size || (c->size - ALIGN(size)) > suba->mincell) {
		p = suba_alloc(suba, size, 0);
	} else {
		return ptr;
	}
	if (p) {
		memcpy(p, ptr, size);
		suba_free(suba, ptr);
	}

	return p;
}

int
suba_print_cell(struct allocator *suba, const char *msg, struct cell *c)
{
	ref_t ref = suba_ref(suba, c);
	if (ref >= ALIGN(sizeof *suba) && (ref + POFF + c->size) <= 10000000) {
		fprintf(stderr, "%s: %8u-%-8lu %8u %-8u\n", msg, ref, ref + POFF + c->size, c->size, c->next);
	} else {
		fprintf(stderr, "%s: %8u-err %8u %-8u\n", msg, ref, c->size, c->next);
		return 0;
	}
	return 1;
}
/*
int
suba_print_alloc_list(struct allocator *suba, FILE *stream)
{
	struct cell *c, *tail = SADR(suba, suba->tail);

	c = (struct cell *)((char *)suba + ALIGN(sizeof *suba));
	while (c < tail) {
		if (c->stk[0]) {
			unsigned char buf[1024], *blim = buf + 1024;
			unsigned char msg[16];
			sprintf((char *)msg, "%d", c->size);
			dbug_sprint_stacktrace(buf, blim, c->stk, 4, msg);
			fputs((const char *)buf, stream);
			fflush(stream);
		}
		c = (struct cell *)((char *)c + POFF + c->size);
	}

	return 0;
}
*/
int
suba_print_free_list(struct allocator *suba)
{
	struct cell *c;
	char buf[10];
	int count = 0;
	int ret = 1;

	c = suba_addr(suba, suba->tail);
	while (c->next < suba->tail) {
		if (c->next < POFF) {
			PMNF(errno = EINVAL, ": next ref corrupted: %d", c->next);
			return -1;
		}
		c = suba_addr(suba, c->next);
		sprintf(buf, "%d", count++);
		if (!suba_print_cell(suba, buf, c)) {
			ret = 0;
		}
	}
	c = suba_addr(suba, c->next);
	sprintf(buf, "%d", count++);
	if (!suba_print_cell(suba, buf, c)) {
		ret = 0;
	}

	fprintf(stderr, "count: start-end         size next\n");

	return ret;
}

