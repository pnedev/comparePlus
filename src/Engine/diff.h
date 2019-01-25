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
	int			off; // off into 1 if DIFF_MATCH and DIFF_IN_1 but into 2 if DIFF_IN_2
	int			len;

	UserDataT	info;
};


template <>
struct diff_info<void>
{
	diff_type	type;
	int			off; // off into 1 if DIFF_MATCH and DIFF_IN_1 but into 2 if DIFF_IN_2
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
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2, int max = INT_MAX);
	DiffCalc(const Elem v1[], int v1_size, const Elem v2[], int v2_size, int max = INT_MAX);

	// Runs the actual compare and returns the differences + swap flag indicating if the
	// compared sequences have been swapped for better results (if true, _a and _b have been swapped,
	// meaning that DIFF_IN_1 in the differences is regarding _b instead _a)
	std::pair<std::vector<diff_info<UserDataT>>, bool> operator()(bool doBoundaryShift = true);

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
	void _shift_boundaries();
	inline int _count_replaces();

	const Elem*	_a;
	int _a_size;
	const Elem*	_b;
	int _b_size;

	std::vector<diff_info<UserDataT>>	_diff;

	const int	_dmax;
	varray<int>	_buf;
};


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2, int max) :
	_a(v1.data()), _a_size(v1.size()), _b(v2.data()), _b_size(v2.size()), _dmax(max)
{
}


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const Elem v1[], int v1_size, const Elem v2[], int v2_size, int max) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _dmax(max)
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
// If the Elem after the DIFF_IN_1 is the same as the first Elem of the DIFF_IN_1, shift differences down:
// If [] surrounds the marked differences, basically [abb]a is the same as a[bba]
// Since most languages start with unique elem and end with repetitive elem (end, </node>, }, ], ), >, etc)
// we shift the differences down to make results look cleaner
template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_shift_boundaries()
{
	for (int i = 0; i < static_cast<int>(_diff.size()); ++i)
	{
		if (_diff[i].type == diff_type::DIFF_MATCH)
			continue;

		const Elem*	el	= _b;

		if (_diff[i].type == diff_type::DIFF_IN_1)
		{
			// If there is DIFF_IN_2 after DIFF_IN_1 both sequences are changed - boundaries do not match for sure
			if (i + 1 < static_cast<int>(_diff.size()) && _diff[i + 1].type == diff_type::DIFF_IN_2)
			{
				++i;
				continue;
			}

			el	= _a;
		}

		if (i + 1 < static_cast<int>(_diff.size()))
		{
			diff_info<UserDataT>& diff = _diff[i];
			diff_info<UserDataT>& next_match_diff = _diff[i + 1];

			const int max_len = (diff.len > next_match_diff.len) ? next_match_diff.len : diff.len;

			int check_off = diff.off + diff.len;
			int shift_len = 0;

			while (shift_len < max_len && el[diff.off] == el[check_off])
			{
				++diff.off;
				++check_off;
				++shift_len;
			}

			// Diff block shifted - we need to adjust the surrounding matching blocks accordingly
			if (shift_len)
			{
				if (i >= 1)
				{
					_diff[i - 1].len += shift_len;
				}
				// Create new match block in the beginning
				else
				{
					diff_info<UserDataT> prev_match_diff;

					prev_match_diff.type = diff_type::DIFF_MATCH;
					prev_match_diff.off = 0;
					prev_match_diff.len = shift_len;

					_diff.insert(_diff.begin(), prev_match_diff);
					++i;
				}

				diff_info<UserDataT>& next_match_diff = _diff[i + 1];

				next_match_diff.off += shift_len;
				next_match_diff.len -= shift_len;

				// The whole match diff shifted - erase it and merge surrounding diff blocks
				if (next_match_diff.len == 0)
				{
					int j = i + 1;

					_diff.erase(_diff.begin() + j);

					while (j < static_cast<int>(_diff.size()) && _diff[i].type != _diff[j].type)
						++j;

					if (j < static_cast<int>(_diff.size()))
					{
						_diff[i].len += _diff[j].len;
						_diff.erase(_diff.begin() + j);

						// Diff blocks merged - recheck same diff
						--i;
					}
				}
			}
		}
	}
}


template <typename Elem, typename UserDataT>
inline int DiffCalc<Elem, UserDataT>::_count_replaces()
{
	const int diffSize = static_cast<int>(_diff.size()) - 1;
	int replaces = 0;

	for (int i = 0; i < diffSize; ++i)
	{
		if ((_diff[i].type == diff_type::DIFF_IN_1) && (_diff[i + 1].type == diff_type::DIFF_IN_2))
		{
			++replaces;
			++i;
		}
	}

	return replaces;
}


template <typename Elem, typename UserDataT>
std::pair<std::vector<diff_info<UserDataT>>, bool> DiffCalc<Elem, UserDataT>::operator()(bool doBoundaryShift)
{
	bool swapped = (_a_size < _b_size);

	if (swapped)
	{
		std::swap(_a, _b);
		std::swap(_a_size, _b_size);
	}

	/* The _ses function assumes we begin with a diff. The following ensures this is true by skipping any matches
	 * in the beginning. This also helps to quickly process sequences that match entirely.
	 */
	int off = 0;

	int asize = _a_size;
	int bsize = _b_size;

	while (off < asize && off < bsize && _a[off] == _b[off])
		++off;

	_edit(diff_type::DIFF_MATCH, 0, off);

	if (asize == bsize && off == asize)
		return std::make_pair(_diff, swapped);

	asize -= off;
	bsize -= off;

	if (_ses(off, asize, off, bsize) == -1)
	{
		_diff.clear();
		return std::make_pair(_diff, swapped);
	}

	// Wipe temporal buffer to free memory
	_buf.get().clear();

	// Swap compared sequences and re-compare to see if result is more optimal
	if (_a_size == _b_size)
	{
		const int replacesCount = _count_replaces();

		// Store current compare result
		std::vector<diff_info<UserDataT>> storedDiff = std::move(_diff);
		std::swap(_a, _b);
		swapped = !swapped;

		// Restore first matching block before continuing
		if (storedDiff[0].type == diff_type::DIFF_MATCH)
			_diff.push_back(storedDiff[0]);

		int newReplacesCount = _ses(off, asize, off, bsize);

		// Wipe temporal buffer to free memory
		_buf.get().clear();

		if (newReplacesCount != -1)
			newReplacesCount = _count_replaces();

		// If re-compare result is not more optimal - restore the previous state
		if (newReplacesCount < replacesCount)
		{
			_diff = std::move(storedDiff);
			std::swap(_a, _b);
			swapped = !swapped;
		}
	}

	if (doBoundaryShift)
		_shift_boundaries();

	return std::make_pair(_diff, swapped);
}
