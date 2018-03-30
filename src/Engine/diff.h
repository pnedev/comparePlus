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
 *   E. Myers, "An O(ND) Difference Algorithm and Its Variations",
 *   Algorithmica 1, 2 (1986), 251-266.
 *   http://www.cs.arizona.edu/people/gene/PAPERS/diff.ps
 *
 * This is the same algorithm used by GNU diff(1).
 */

/* Modified into template class DiffCalc
 * Copyright (C) 2017-2018  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include <cstdlib>
#include <climits>
#include <utility>

#include "varray.h"


enum class diff_type
{
	DIFF_MATCH,
	DIFF_IN_1,
	DIFF_IN_2
};


template <typename UserDataT>
struct diff_info
{
	diff_type	type;
	int			off; // off into _a if DIFF_MATCH and DIFF_IN_1 but into _b if DIFF_IN_2
	int			len;

	UserDataT	info;
};


template <>
struct diff_info<void>
{
	diff_type	type;
	int			off; // off into _a if DIFF_MATCH and DIFF_IN_1 but into _b if DIFF_IN_2
	int			len;
};


/**
 *  \class  DiffCalc
 *  \brief  Compares and makes a differences list between two vectors (elements are template, must have operator==)
 */
template <typename Elem, typename UserDataT = void>
class DiffCalc
{
public:
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
			const Elem& blankVal = Elem(), int max = INT_MAX);

	DiffCalc(const Elem v1[], int v1_size, const Elem v2[], int v2_size,
			const Elem& blankVal = Elem(), int max = INT_MAX);

	std::vector<diff_info<UserDataT>> operator()();

	DiffCalc(const DiffCalc&) = delete;
	const DiffCalc& operator=(const DiffCalc&) = delete;

private:
	struct middle_snake {
		int x, y, u, v;
	};

	inline int& _v(int k, int r);
	void _edit(diff_type type, int off, int len);
	int _find_middle_snake(int aoff, int aend, int boff, int bend, middle_snake& ms);
	int _ses(int aoff, int aend, int boff, int bend);
	void _shift_boundries();
	bool _blocks_match(const diff_info<UserDataT>& di1, const diff_info<UserDataT>& di2);
	void _find_moves();

	const Elem*	_a;
	const int	_a_size;
	const Elem*	_b;
	const int	_b_size;

	std::vector<diff_info<UserDataT>>	_diff;

	const Elem	_blankVal;
	const int	_dmax;
	varray<int>	_buf;
};


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
	const Elem& blankVal, int max) :
	_a(v1.data()), _a_size(v1.size()), _b(v2.data()), _b_size(v2.size()), _blankVal(blankVal), _dmax(max)
{
}


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const Elem v1[], int v1_size, const Elem v2[], int v2_size,
	const Elem& blankVal, int max) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _blankVal(blankVal), _dmax(max)
{
}


template <typename Elem, typename UserDataT>
inline int& DiffCalc<Elem, UserDataT>::_v(int k, int r)
{
	/* Pack -N to N into 0 to N * 2 */
	const int j = (k <= 0) ? (-k * 4 + r) : (k * 4 + (r - 2));

	return _buf.get(j);
}


template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_edit(diff_type type, int off, int len)
{
	if (len == 0)
		return;

	bool add_elem = _diff.empty();

	if (!add_elem)
		add_elem = (_diff.back().type != type);

	if (add_elem)
	{
		diff_info<UserDataT> new_di;

		new_di.type = type;
		new_di.off = off;
		new_di.len = len;

		_diff.push_back(new_di);
	}
	else
	{
		_diff.back().len += len;
	}
}


template <typename Elem, typename UserDataT>
int DiffCalc<Elem, UserDataT>::_find_middle_snake(int aoff, int aend, int boff, int bend, middle_snake& ms)
{
	const int delta = aend - bend;
	const int odd = delta & 1;
	const int mid = (aend + bend) / 2 + odd;

	_v(1, 0) = 0;
	_v(delta - 1, 1) = aend;

	for (int d = 0; d <= mid; ++d)
	{
		int k, x, y;

		if ((2 * d - 1) >= _dmax)
			return _dmax;

		for (k = d; k >= -d; k -= 2)
		{
			if (k == -d || (k != d && _v(k - 1, 0) < _v(k + 1, 0)))
				x = _v(k + 1, 0);
			else
				x = _v(k - 1, 0) + 1;

			y = x - k;

			ms.x = x;
			ms.y = y;

			while (x < aend && y < bend &&  _a[aoff + x] == _b[boff + y])
			{
				++x;
				++y;
			}

			_v(k, 0) = x;

			if (odd && k >= (delta - (d - 1)) && k <= (delta + (d - 1)))
			{
				if (x >= _v(k, 1))
				{
					ms.u = x;
					ms.v = y;
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

			ms.u = x;
			ms.v = y;

			while (x > 0 && y > 0 &&  _a[aoff + x - 1] == _b[boff + y - 1])
			{
				--x;
				--y;
			}

			_v(kr, 1) = x;

			if (!odd && kr >= -d && kr <= d)
			{
				if (x <= _v(kr, 0))
				{
					ms.x = x;
					ms.y = y;

					return 2 * d;
				}
			}
		}
	}

	return -1;
}


template <typename Elem, typename UserDataT>
int DiffCalc<Elem, UserDataT>::_ses(int aoff, int aend, int boff, int bend)
{
	middle_snake ms = { 0 };
	int d;

	if (aend == 0)
	{
		_edit(diff_type::DIFF_IN_2, boff, bend);
		d = bend;
	}
	else if (bend == 0)
	{
		_edit(diff_type::DIFF_IN_1, aoff, aend);
		d = aend;
	}
	else
	{
		/* Find the middle "snake" around which we
		 * recursively solve the sub-problems.
		 */
		d = _find_middle_snake(aoff, aend, boff, bend, ms);
		if (d == -1)
			return -1;

		if (d >= _dmax)
			return _dmax;

		if (d > 1)
		{
			if (_ses(aoff, ms.x, boff, ms.y) == -1)
				return -1;

			_edit(diff_type::DIFF_MATCH, aoff + ms.x, ms.u - ms.x);

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
					_edit(diff_type::DIFF_MATCH, aoff, aend);
					_edit(diff_type::DIFF_IN_2, boff + (bend - 1), 1);
				}
				else
				{
					_edit(diff_type::DIFF_IN_2, boff, 1);
					_edit(diff_type::DIFF_MATCH, aoff, aend);
				}
			}
			else
			{
				if (x == u)
				{
					_edit(diff_type::DIFF_MATCH, aoff, bend);
					_edit(diff_type::DIFF_IN_1, aoff + (aend - 1), 1);
				}
				else
				{
					_edit(diff_type::DIFF_IN_1, aoff, 1);
					_edit(diff_type::DIFF_MATCH, aoff + 1, bend);
				}
			}
		}
	}

	return d;
}


// Algorithm borrowed from WinMerge
// If the Elem after the diff_in_1 is the same as the first Elem of the diff_in_1, shift differences down:
// If [] surrounds the marked differences, basically [abb]a is the same as a[bba]
// Since most languages start with unique elem and end with repetitive elem (end, </node>, }, ], ), >, etc)
// we shift the differences down to make results look cleaner
template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_shift_boundries()
{
	int diff_size = static_cast<int>(_diff.size());

	for (int i = 0; i < diff_size; ++i)
	{
		int amax = _a_size;
		int bmax = _b_size;
		int offset = 0;

		if (_diff[i].type != diff_type::DIFF_MATCH)
		{
			for (int j = i + 1; j < diff_size; ++j)
			{
				if (_diff[i].type == _diff[j].type)
				{
					amax = _diff[j].off;
					bmax = _diff[j].off;
					break;
				}
			}
		}

		if (_diff[i].type == diff_type::DIFF_IN_1)
		{
			if (i + 1 < diff_size)
			{
				// If there is a diff_in_2 after a diff_in_1, there is a potential match, so both blocks
				// need to be moved at the same time
				if (_diff[i + 1].type == diff_type::DIFF_IN_2)
				{
					diff_info<UserDataT>& adiff = _diff[i];
					diff_info<UserDataT>& bdiff = _diff[i + 1];

					bmax = _b_size;

					for (int j = i + 2; j < diff_size; ++j)
					{
						if (bdiff.type == _diff[j].type)
						{
							bmax = _diff[j].off;
							break;
						}
					}

					int aend = adiff.off + adiff.len;
					int bend = bdiff.off + bdiff.len;

					while (aend < amax && bend < bmax && _a[adiff.off] == _a[aend] && _b[bdiff.off] == _b[bend])
					{
						++aend;
						++bend;
						++(adiff.off);
						++(bdiff.off);
						++offset;
					}

					++i;
				}
				else
				{
					diff_info<UserDataT>& adiff = _diff[i];

					int aend = adiff.off + adiff.len;

					while (aend < amax && _a[adiff.off] == _a[aend])
					{
						++aend;
						++(adiff.off);
						++offset;
					}
				}
			}
		}
		else if (_diff[i].type == diff_type::DIFF_IN_2)
		{
			diff_info<UserDataT>& bdiff = _diff[i];

			int bend = bdiff.off + bdiff.len;

			while (bend < bmax && _b[bdiff.off] == _b[bend])
			{
				++bend;
				++(bdiff.off);
				++offset;
			}
		}

		// Diff block shifted - we need to adjust the surrounding match blocks accordingly
		if (offset)
		{
			if (i + 1 < diff_size)
			{
				diff_info<UserDataT>& post_diff = _diff[i + 1];

				if (post_diff.len > offset)
				{
					post_diff.off += offset;
					post_diff.len -= offset;
				}
				else
				{
					_diff.erase(_diff.begin() + (i + 1));
					--diff_size;
				}
			}

			if (i)
			{
				diff_info<UserDataT>& pre_diff = _diff[i - 1];

				pre_diff.len += offset;
			}
			// Create new match block in the beginning
			else
			{
				diff_info<UserDataT> new_di;

				new_di.type = diff_type::DIFF_MATCH;
				new_di.off = 0;
				new_di.len = offset;

				_diff.insert(_diff.begin(), new_di);
				++diff_size;
				++i;
			}
		}
	}
}


template <typename Elem, typename UserDataT>
std::vector<diff_info<UserDataT>> DiffCalc<Elem, UserDataT>::operator()()
{
	/* The _ses function assumes we begin with a diff. The following ensures this is true by skipping any matches
	 * in the beginning. This also helps to quickly process sequences that match entirely.
	 */
	int x = 0, y = 0;

	int asize = _a_size;
	int bsize = _b_size;

	while (x < asize && y < bsize && _a[x] == _b[y])
	{
		++x;
		++y;
	}

	_edit(diff_type::DIFF_MATCH, 0, x);

	if (asize == bsize && x == asize)
		return _diff;

	asize -= x;
	bsize -= y;

	if (_ses(x, asize, y, bsize) == -1)
		_diff.clear();
	else
		_shift_boundries();

	// Wipe temporal buffer to free memory
	_buf.get().clear();

	return _diff;
}
