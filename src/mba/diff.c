/* diff - compute a shortest edit script (SES) given two sequences
 * Copyright (c) 2004 Michael B. Allen <mba2000 ioplex.com>
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

/* This algorithm is basically Myers' solution to SES/LCS with
 * the Hirschberg linear space refinement as described in the
 * following publication:
 *
 *   E. Myers, ``An O(ND) Difference Algorithm and Its Variations,''
 *   Algorithmica 1, 2 (1986), 251-266.
 *   http://www.cs.arizona.edu/people/gene/PAPERS/diff.ps
 *
 * This is the same algorithm used by GNU diff(1).
 */

#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "msgno.h"
#include "diff.h"

#define FV(k) _v(ctx, (k), 0)
#define RV(k) _v(ctx, (k), 1)

struct _ctx {
	idx_fn idx;
	cmp_fn cmp;
	void *context;
	struct varray *buf;
	struct varray *ses;
	int si;
	int dmax;
};

struct middle_snake {
	int x, y, u, v;
};

static void
_setv(struct _ctx *ctx, int k, int r, int val)
{
	int j;
	int *i;
                /* Pack -N to N into 0 to N * 2
                 */
	j = k <= 0 ? -k * 4 + r : k * 4 + (r - 2);

	i = (int *)varray_get(ctx->buf, j);
	*i = val;
}
static int
_v(struct _ctx *ctx, int k, int r)
{
	int j;

	j = k <= 0 ? -k * 4 + r : k * 4 + (r - 2);

	return *((int *)varray_get(ctx->buf, j));
}

static int
_find_middle_snake(const void *a, int aoff, int n,
		const void *b, int boff, int m,
		struct _ctx *ctx,
		struct middle_snake *ms)
{
	int delta, odd, mid, d;

	delta = n - m;
	odd = delta & 1;
	mid = (n + m) / 2;
	mid += odd;

	_setv(ctx, 1, 0, 0);
	_setv(ctx, delta - 1, 1, n);

	for (d = 0; d <= mid; d++) {
		int k, x, y;

		if ((2 * d - 1) >= ctx->dmax) {
			return ctx->dmax;
		}

		for (k = d; k >= -d; k -= 2) {
			if (k == -d || (k != d && FV(k - 1) < FV(k + 1))) {
				x = FV(k + 1);
			} else {
				x = FV(k - 1) + 1;
			}
			y = x - k;

			ms->x = x;
			ms->y = y;
			if (ctx->cmp) {
				while (x < n && y < m && ctx->cmp(ctx->idx(a, aoff + x, ctx->context),
							ctx->idx(b, boff + y, ctx->context), ctx->context) == 0) {
					x++; y++;
				}
			} else {
				const unsigned char *a0 = (const unsigned char *)a + aoff;
				const unsigned char *b0 = (const unsigned char *)b + boff;
				while (x < n && y < m && a0[x] == b0[y]) {
					x++; y++;
				}
			}
			_setv(ctx, k, 0, x);

			if (odd && k >= (delta - (d - 1)) && k <= (delta + (d - 1))) {
				if (x >= RV(k)) {
					ms->u = x;
					ms->v = y;
					return 2 * d - 1;
				}
			}
		}
		for (k = d; k >= -d; k -= 2) {
			int kr = (n - m) + k;

			if (k == d || (k != -d && RV(kr - 1) < RV(kr + 1))) {
				x = RV(kr - 1);
			} else {
				x = RV(kr + 1) - 1;
			}
			y = x - kr;

			ms->u = x;
			ms->v = y;
			if (ctx->cmp) {
				while (x > 0 && y > 0 && ctx->cmp(ctx->idx(a, aoff + (x - 1), ctx->context),
							ctx->idx(b, boff + (y - 1), ctx->context), ctx->context) == 0) {
					x--; y--;
				}
			} else {
				const unsigned char *a0 = (const unsigned char *)a + aoff;
				const unsigned char *b0 = (const unsigned char *)b + boff;
				while (x > 0 && y > 0 && a0[x - 1] == b0[y - 1]) {
					x--; y--;
				}
			}
			_setv(ctx, kr, 1, x);

			if (!odd && kr >= -d && kr <= d) {
				if (x <= FV(kr)) {
					ms->x = x;
					ms->y = y;
					return 2 * d;
				}
			}
		}
	}

	errno = EFAULT;

	return -1;
}

static void
_edit(struct _ctx *ctx, int op, int off, int len)
{
	struct diff_edit *e;

	if (len == 0 || ctx->ses == NULL) {
		return;
	}               /* Add an edit to the SES (or
                     * coalesce if the op is the same)
                     */
	e = varray_get(ctx->ses, ctx->si);
	if (e->op != op) {
		if (e->op) {
			ctx->si++;
			e = varray_get(ctx->ses, ctx->si);
		}
		e->op = op;
		e->off = off;
		e->len = len;
	} else {
		e->len += len;
	}
}

static int
_ses(const void *a, int aoff, int n,
		const void *b, int boff, int m,
		struct _ctx *ctx)
{
	struct middle_snake ms;
	int d;

	if (n == 0) {
		_edit(ctx, DIFF_INSERT, boff, m);
		d = m;
	} else if (m == 0) {
		_edit(ctx, DIFF_DELETE, aoff, n);
		d = n;
	} else {
                    /* Find the middle "snake" around which we
                     * recursively solve the sub-problems.
                     */
		d = _find_middle_snake(a, aoff, n, b, boff, m, ctx, &ms);
		if (d == -1) {
			return -1;
		} else if (d >= ctx->dmax) {
			return ctx->dmax;
		} else if (ctx->ses == NULL) {
			return d;
		} else if (d > 1) {
			if (_ses(a, aoff, ms.x, b, boff, ms.y, ctx) == -1) {
				return -1;
			}

			_edit(ctx, DIFF_MATCH, aoff + ms.x, ms.u - ms.x);

			aoff += ms.u;
			boff += ms.v;
			n -= ms.u;
			m -= ms.v;
			if (_ses(a, aoff, n, b, boff, m, ctx) == -1) {
				return -1;
			}
		} else {
			int x = ms.x;
			int u = ms.u;

                 /* There are only 4 base cases when the
                  * edit distance is 1.
                  *
                  * n > m   m > n
                  *
                  *   -       |
                  *    \       \    x != u
                  *     \       \
                  *
                  *   \       \
                  *    \       \    x == u
                  *     -       |
                  */

			if (m > n) {
				if (x == u) {
					_edit(ctx, DIFF_MATCH, aoff, n);
					_edit(ctx, DIFF_INSERT, boff + (m - 1), 1);
				} else {
					_edit(ctx, DIFF_INSERT, boff, 1);
					_edit(ctx, DIFF_MATCH, aoff, n);
				}
			} else {
				if (x == u) {
					_edit(ctx, DIFF_MATCH, aoff, m);
					_edit(ctx, DIFF_DELETE, aoff + (n - 1), 1);
				} else {
					_edit(ctx, DIFF_DELETE, aoff, 1);
					_edit(ctx, DIFF_MATCH, aoff + 1, m);
				}
			}
		}
	}

	return d;
}
int
diff(const void *a, int aoff, int n,
		const void *b, int boff, int m,
		idx_fn idx, cmp_fn cmp, void *context, int dmax,
		struct varray *ses, int *sn,
		struct varray *buf)
{
	struct _ctx ctx;
	int d, x, y;
	struct diff_edit *e = NULL;
	struct varray tmp;

	if (!idx != !cmp) { /* ensure both NULL or both non-NULL */
		errno = EINVAL;
		return -1;
	}

	ctx.idx = idx;
	ctx.cmp = cmp;
	ctx.context = context;
	if (buf) {
		ctx.buf = buf;
	} else {
		varray_init(&tmp, sizeof(int), NULL);
		ctx.buf = &tmp;
	}
	ctx.ses = ses;
	ctx.si = 0;
	ctx.dmax = dmax ? dmax : INT_MAX;

	if (ses && sn) {
		if ((e = varray_get(ses, 0)) == NULL) {
			AMSG("");
			if (buf == NULL) {
				varray_deinit(&tmp);
			}
			return -1;
		}
		e->op = 0;
	}

         /* The _ses function assumes the SES will begin or end with a delete
          * or insert. The following will insure this is true by eating any
          * beginning matches. This is also a quick to process sequences
          * that match entirely.
          */
	x = y = 0;
	if (cmp) {
		while (x < n && y < m && cmp(idx(a, aoff + x, context),
					idx(b, boff + y, context), context) == 0) {
			x++; y++;
		}
	} else {
		const unsigned char *a0 = (const unsigned char *)a + aoff;
		const unsigned char *b0 = (const unsigned char *)b + boff;
		while (x < n && y < m && a0[x] == b0[y]) {
			x++; y++;
		}
	}
	_edit(&ctx, DIFF_MATCH, aoff, x);

	if ((d = _ses(a, aoff + x, n - x, b, boff + y, m - y, &ctx)) == -1) {
		AMSG("");
		if (buf == NULL) {
			varray_deinit(&tmp);
		}
		return -1;
	}
	if (ses && sn) {
		*sn = e->op ? ctx.si + 1 : 0;
	}

	if (buf == NULL) {
		varray_deinit(&tmp);
	}

	return d;
}

