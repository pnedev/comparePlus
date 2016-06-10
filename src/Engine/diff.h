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

/* Modified into template class DiffCalc
 * Copyright (C) 2016  Pavel Nedev <pg.nedev@gmail.com>
 */

#pragma once

#include <stdlib.h>
#include <limits.h>
#include "varray.h"

#include "ProgressDlg.h"


enum diff_op
{
	DIFF_MATCH = 1,
	DIFF_DELETE,
	DIFF_INSERT,
	DIFF_CHANGE1,
	DIFF_CHANGE2,
	DIFF_MOVE
};


struct diff_change
{
	int off;
	unsigned int len;
	unsigned int line;
	unsigned int matchedLine;
};


struct diff_edit
{
	short op;
	int off; // off into s1 if MATCH or DELETE but into s2 if INSERT
	unsigned int len;

	// added lines for Notepad++ use
	int set;
	unsigned int matchedLine;
	int altLocation;

	varray_sh_ptr<diff_change> changes;
	unsigned int changeCount;

	std::vector<int> moves;
};


/**
 *  \class  DiffCalc
 *  \brief  Compares and makes a differences list between two vectors (elements are template, must have operator==)
 */
template <typename Elem>
class DiffCalc
{
public:
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2, int max = INT_MAX) :
		_a(v1), _b(v2), _dmax(max) {}

	std::vector<diff_edit> operator()();

private:
	struct middle_snake {
		int x, y, u, v;
	};

	void _setv(int k, int r, int val);
	int _v(int k, int r);
	void _edit(short op, int off, unsigned int len);
	int _find_middle_snake(unsigned int aoff, unsigned int aend, unsigned int boff, unsigned int bend,
			middle_snake *ms);
	int _ses(unsigned int aoff, unsigned int aend, unsigned int boff, unsigned int bend);

	const std::vector<Elem>& _a;
	const std::vector<Elem>& _b;

	std::vector<diff_edit> _diff;

	const int _dmax;
	varray<int> _buf;
};


template <typename Elem>
void DiffCalc<Elem>::_setv(int k, int r, int val)
{
	/* Pack -N to N into 0 to N * 2 */
	int j = (k <= 0) ? (-k * 4 + r) : (k * 4 + (r - 2));

	_buf.get(j) = val;
}


template <typename Elem>
int DiffCalc<Elem>::_v(int k, int r)
{
	int j = (k <= 0) ? (-k * 4 + r) : (k * 4 + (r - 2));

	return _buf.get(j);
}


template <typename Elem>
void DiffCalc<Elem>::_edit(short op, int off, unsigned int len)
{
	if (len == 0)
		return;

	bool add_elem = _diff.empty();

	if (!add_elem)
		add_elem = (_diff.rbegin()->op != op);

	if (add_elem)
	{
		diff_edit new_e;
		new_e.op = op;
		new_e.off = off;
		new_e.len = len;
		_diff.push_back(new_e);
	}
	else
	{
		_diff.rbegin()->len += len;
	}
}


template <typename Elem>
int DiffCalc<Elem>::_find_middle_snake(unsigned int aoff, unsigned int aend, unsigned int boff, unsigned int bend,
		middle_snake *ms)
{
	int delta, odd, mid, d;

	delta = aend - bend;
	odd = delta & 1;
	mid = (aend + bend) / 2;
	mid += odd;

	_setv(1, 0, 0);
	_setv(delta - 1, 1, aend);

	for (d = 0; d <= mid; d++)
	{
		int k, x, y;

		if (!ProgressDlg::Update(mid))
			return -1;

		if ((2 * d - 1) >= _dmax)
			return _dmax;

		for (k = d; k >= -d; k -= 2)
		{
			if (k == -d || (k != d && _v(k - 1, 0) < _v(k + 1, 0)))
				x = _v(k + 1, 0);
			else
				x = _v(k - 1, 0) + 1;

			y = x - k;

			ms->x = x;
			ms->y = y;

			while (x < (int)aend && y < (int)bend &&  _a[aoff + x] == _b[boff + y])
			{
				++x;
				++y;
			}

			_setv(k, 0, x);

			if (odd && k >= (delta - (d - 1)) && k <= (delta + (d - 1)))
			{
				if (x >= _v(k, 1))
				{
					ms->u = x;
					ms->v = y;
					return 2 * d - 1;
				}
			}
		}

		for (k = d; k >= -d; k -= 2)
		{
			int kr = (aend - bend) + k;

			if (k == d || (k != -d && _v(kr - 1, 1) < _v(kr + 1, 1)))
			{
				x = _v(kr - 1, 1);
			}
			else
			{
				x = _v(kr + 1, 1) - 1;
			}

			y = x - kr;

			ms->u = x;
			ms->v = y;

			while (x > 0 && y > 0 &&  _a[aoff + x - 1] == _b[boff + y - 1])
			{
				--x;
				--y;
			}

			_setv(kr, 1, x);

			if (!odd && kr >= -d && kr <= d)
			{
				if (x <= _v(kr, 0))
				{
					ms->x = x;
					ms->y = y;

					return 2 * d;
				}
			}
		}
	}

	return -1;
}


template <typename Elem>
int DiffCalc<Elem>::_ses(unsigned int aoff, unsigned int aend, unsigned int boff, unsigned int bend)
{
	middle_snake ms;
	int d;

	if (aend == 0)
	{
		_edit(DIFF_INSERT, boff, bend);
		d = bend;
	}
	else if (bend == 0)
	{
		_edit(DIFF_DELETE, aoff, aend);
		d = aend;
	}
	else
	{
		/* Find the middle "snake" around which we
		 * recursively solve the sub-problems.
		 */
		d = _find_middle_snake(aoff, aend, boff, bend, &ms);
		if (d == -1)
			return -1;

		if (d >= _dmax)
			return _dmax;

		if (d > 1)
		{
			if (_ses(aoff, ms.x, boff, ms.y) == -1)
				return -1;

			_edit(DIFF_MATCH, aoff + ms.x, ms.u - ms.x);

			aoff += ms.u;
			boff += ms.v;
			aend -= ms.u;
			bend -= ms.v;

			if (_ses(aoff, aend, boff, bend) == -1)
				return -1;
		}
		else
		{
			int x = ms.x;
			int u = ms.u;

			/* There are only 4 base cases when the
			 * edit distance is 1.
			 *
			 * aend > bend   bend > aend
			 *
			 *   -       |
			 *    \       \    x != u
			 *     \       \
			 *
			 *   \       \
			 *    \       \    x == u
			 *     -       |
			 */

			if (bend > aend)
			{
				if (x == u)
				{
					_edit(DIFF_MATCH, aoff, aend);
					_edit(DIFF_INSERT, boff + (bend - 1), 1);
				}
				else
				{
					_edit(DIFF_INSERT, boff, 1);
					_edit(DIFF_MATCH, aoff, aend);
				}
			}
			else
			{
				if (x == u)
				{
					_edit(DIFF_MATCH, aoff, bend);
					_edit(DIFF_DELETE, aoff + (aend - 1), 1);
				}
				else
				{
					_edit(DIFF_DELETE, aoff, 1);
					_edit(DIFF_MATCH, aoff + 1, bend);
				}
			}
		}
	}

	return d;
}


template <typename Elem>
std::vector<diff_edit> DiffCalc<Elem>::operator()()
{
	/* The _ses function assumes the SES will begin or end with a delete
	 * or insert. The following will ensure this is true by eating any
	 * beginning matches. This is also a quick to process sequences
	 * that match entirely.
	 */
	unsigned int x = 0, y = 0;

	const std::size_t a_size = _a.size();
	const std::size_t b_size = _b.size();

	while (x < a_size && y < b_size && _a[x] == _b[y])
	{
		++x;
		++y;
	}

	if (a_size == b_size && x == a_size)
		return _diff;

	_edit(DIFF_MATCH, 0, x);

	if (_ses(x, a_size - x, y, b_size - y) == -1)
		_diff.clear();

	// Wipe temporal buffer to free memory
	_buf.get().clear();

	return _diff;
}
