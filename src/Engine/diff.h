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
 * Copyright (C) 2017-2025  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include <cstdint>
#include <cstdlib>
#include <climits>
#include <utility>
#include <functional>

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
	intptr_t	off; // off into 1 if DIFF_MATCH and DIFF_IN_1 but into 2 if DIFF_IN_2
	intptr_t	len;

	UserDataT	info;
};


template <>
struct diff_info<void>
{
	diff_type	type;
	intptr_t	off; // off into 1 if DIFF_MATCH and DIFF_IN_1 but into 2 if DIFF_IN_2
	intptr_t	len;
};


typedef std::function<bool()> IsCancelledFn;


/**
 *  \class  DiffCalc
 *  \brief  Compares and makes a differences list between two vectors (elements are template, must have operator==)
 */
template <typename Elem, typename UserDataT = void>
class DiffCalc
{
public:
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
		IsCancelledFn isCancelled = nullptr);
	DiffCalc(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
		IsCancelledFn isCancelled = nullptr);

	// Runs the actual compare and returns the differences
	std::vector<diff_info<UserDataT>> operator()(
			bool doSwapCheck = true, bool doDiffsCombine = false, bool doBoundaryShift = false,
			const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints = {});

	DiffCalc(const DiffCalc&) = delete;
	const DiffCalc& operator=(const DiffCalc&) = delete;

private:
	class Engine
	{
	public:
		Engine(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
			IsCancelledFn isCancelled = nullptr);

		// Runs the actual compare and returns the differences
		std::vector<diff_info<UserDataT>> operator()(bool doSwapCheck = true);

		Engine(const Engine&) = delete;
		const Engine& operator=(const Engine&) = delete;

	private:
		static constexpr int		_cCancelCheckItrInterval {3000};
		static constexpr intptr_t	_cDmax {INTPTR_MAX};

		struct middle_snake {
			intptr_t x, y, u, v;
		};

		inline intptr_t& _v(intptr_t k, intptr_t r);
		void _add(diff_type type, intptr_t off, intptr_t len);
		intptr_t _find_middle_snake(intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend, middle_snake& ms);
		intptr_t _ses(intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend);

		inline intptr_t _count_replaces();
		inline void _swap_diff1_diff2();

		const Elem*	_a;
		intptr_t _a_size;
		const Elem*	_b;
		intptr_t _b_size;

		IsCancelledFn _isCancelled;
		int _cancelCheckCount;

		std::vector<diff_info<UserDataT>> _diff;

		varray<intptr_t> _buf;
	};

	void _diff_append(intptr_t aoff, intptr_t boff, std::vector<diff_info<UserDataT>>&& diff);
	void _combine_diffs();
	void _shift_boundaries();

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	std::vector<diff_info<UserDataT>> _diff;

	IsCancelledFn _isCancelled;
};


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::Engine::Engine(
		const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size, IsCancelledFn isCancelled) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size),
	_isCancelled(isCancelled), _cancelCheckCount(_cCancelCheckItrInterval)
{
}


template <typename Elem, typename UserDataT>
std::vector<diff_info<UserDataT>> DiffCalc<Elem, UserDataT>::Engine::operator()(bool doSwapCheck)
{
	intptr_t off_s = 0;

	intptr_t asize = _a_size;
	intptr_t bsize = _b_size;

	// The diff algorithm assumes we begin with a diff. The following ensures this is true by skipping any matches
	// in the beginning. This also helps to quickly process sequences that match entirely.
	while (off_s < asize && off_s < bsize && _a[off_s] == _b[off_s])
		++off_s;

	if (off_s)
		_add(diff_type::DIFF_MATCH, 0, off_s);

	if (asize == bsize && off_s == asize)
		return _diff;

	intptr_t aend = asize - 1;
	intptr_t bend = bsize - 1;

	asize -= off_s;
	bsize -= off_s;

	intptr_t off_e = 0;

	while (off_e < asize && off_e < bsize && _a[aend - off_e] == _b[bend - off_e])
		++off_e;

	if (off_e)
	{
		asize -= off_e;
		bsize -= off_e;
	}

	if (_ses(off_s, asize, off_s, bsize) == -1)
	{
		_diff.clear();
		return _diff;
	}

	// Wipe temporal buffer to free memory
	_buf.get().clear();

	// Swap compared sequences and re-compare to see if result is more optimal
	if (doSwapCheck)
	{
		const intptr_t replacesCount = _count_replaces();

		// Store current compare result
		std::vector<diff_info<UserDataT>> storedDiff = std::move(_diff);
		std::swap(_a, _b);
		std::swap(asize, bsize);

		// Restore first matching block before continuing
		if (storedDiff[0].type == diff_type::DIFF_MATCH)
			_diff.push_back(storedDiff[0]);

		intptr_t newReplacesCount = _ses(off_s, asize, off_s, bsize);

		// Wipe temporal buffer to free memory
		_buf.get().clear();

		if (newReplacesCount != -1)
			newReplacesCount = _count_replaces();

		std::swap(_a, _b);

		// If re-compare result is not more optimal - restore the previous state
		if (newReplacesCount < replacesCount)
			_diff = std::move(storedDiff);
		else
			_swap_diff1_diff2();
	}

	if (off_e)
		_add(diff_type::DIFF_MATCH, aend - off_e + 1, off_e);

	return _diff;
}


template <typename Elem, typename UserDataT>
inline intptr_t& DiffCalc<Elem, UserDataT>::Engine::_v(intptr_t k, intptr_t r)
{
	// Pack -N to N into 0 to 2 * N
	const intptr_t j = (k <= 0) ? (-k * 4 + r) : (k * 4 + (r - 2));

	return _buf.get(j);
}


template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::Engine::_add(diff_type type, intptr_t off, intptr_t len)
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
intptr_t DiffCalc<Elem, UserDataT>::Engine::_find_middle_snake(
	intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend, middle_snake& ms)
{
	const intptr_t delta = aend - bend;
	const intptr_t odd = delta & 1;
	const intptr_t mid = (aend + bend) / 2 + odd;

	_v(1, 0) = 0;
	_v(delta - 1, 1) = aend;

	for (intptr_t d = 0; d <= mid; ++d)
	{
		intptr_t k, x, y;

		if ((2 * d - 1) >= _cDmax)
			return _cDmax;

		if (!--_cancelCheckCount)
		{
			if (_isCancelled && _isCancelled())
				return -1;

			_cancelCheckCount = _cCancelCheckItrInterval;
		}

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
			intptr_t kr = (aend - bend) + k;

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
intptr_t DiffCalc<Elem, UserDataT>::Engine::_ses(
	intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend)
{
	middle_snake ms = { 0 };
	intptr_t d;

	if (aend == 0)
	{
		_add(diff_type::DIFF_IN_2, boff, bend);
		d = bend;
	}
	else if (bend == 0)
	{
		_add(diff_type::DIFF_IN_1, aoff, aend);
		d = aend;
	}
	else
	{
		// Find the middle "snake" around which we
		// recursively solve the sub-problems.
		d = _find_middle_snake(aoff, aend, boff, bend, ms);
		if (d == -1)
			return -1;

		if (d >= _cDmax)
			return _cDmax;

		if (d > 1)
		{
			if (_ses(aoff, ms.x, boff, ms.y) == -1)
				return -1;

			_add(diff_type::DIFF_MATCH, aoff + ms.x, ms.u - ms.x);

			aoff += ms.u;
			boff += ms.v;
			aend -= ms.u;
			bend -= ms.v;

			if (_ses(aoff, aend, boff, bend) == -1)
				return -1;
		}
		else
		{
			intptr_t x = ms.x;
			intptr_t u = ms.u;

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
					_add(diff_type::DIFF_MATCH, aoff, aend);
					_add(diff_type::DIFF_IN_2, boff + (bend - 1), 1);
				}
				else
				{
					_add(diff_type::DIFF_IN_2, boff, 1);
					_add(diff_type::DIFF_MATCH, aoff, aend);
				}
			}
			else
			{
				if (x == u)
				{
					_add(diff_type::DIFF_MATCH, aoff, bend);
					_add(diff_type::DIFF_IN_1, aoff + (aend - 1), 1);
				}
				else
				{
					_add(diff_type::DIFF_IN_1, aoff, 1);
					_add(diff_type::DIFF_MATCH, aoff + 1, bend);
				}
			}
		}
	}

	return d;
}


template <typename Elem, typename UserDataT>
inline intptr_t DiffCalc<Elem, UserDataT>::Engine::_count_replaces()
{
	const intptr_t diffSize = static_cast<intptr_t>(_diff.size()) - 1;
	intptr_t replaces = 0;

	for (intptr_t i = 0; i < diffSize; ++i)
	{
		if ((_diff[i].type == diff_type::DIFF_IN_1) && (_diff[i + 1].type == diff_type::DIFF_IN_2))
		{
			replaces += std::min(_diff[i].len, _diff[i + 1].len);
			++i;
		}
	}

	return replaces;
}


template <typename Elem, typename UserDataT>
inline void DiffCalc<Elem, UserDataT>::Engine::_swap_diff1_diff2()
{
	intptr_t off2 = 0;
	diff_info<UserDataT>* reorderDiff = nullptr;

	// Swap DIFF_IN_1 and DIFF_IN_2
	for (auto& d: _diff)
	{
		if (d.type == diff_type::DIFF_MATCH)
		{
			d.off = off2;
			off2 += d.len;

			reorderDiff = nullptr;
		}
		else if (d.type == diff_type::DIFF_IN_1)
		{
			d.type = diff_type::DIFF_IN_2;

			reorderDiff = &d;
		}
		else
		{
			d.type = diff_type::DIFF_IN_1;
			off2 += d.len;

			if (reorderDiff)
			{
				std::swap(reorderDiff->type, d.type);
				std::swap(reorderDiff->off,  d.off);
				std::swap(reorderDiff->len,  d.len);

				reorderDiff = nullptr;
			}
		}
	}
}


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
		IsCancelledFn isCancelled) :
	_a(v1.data()), _a_size(v1.size()), _b(v2.data()), _b_size(v2.size()), _isCancelled(isCancelled)
{
}


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
		IsCancelledFn isCancelled) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _isCancelled(isCancelled)
{
}


template <typename Elem, typename UserDataT>
std::vector<diff_info<UserDataT>> DiffCalc<Elem, UserDataT>::operator()(
	bool doSwapCheck, bool doDiffsCombine, bool doBoundaryShift,
	const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints)
{
	if (syncPoints.empty())
	{
		_diff = Engine(_a, _a_size, _b, _b_size)(doSwapCheck);
	}
	else
	{
		intptr_t apos = 0;
		intptr_t bpos = 0;

		for (const auto& syncP: syncPoints)
		{
			if (syncP.first  < apos || syncP.first  >= _a_size ||
				syncP.second < bpos || syncP.second >= _b_size)
				break;

			_diff_append(apos, bpos,
					Engine(&_a[apos], syncP.first - apos, &_b[bpos], syncP.second - bpos)(doSwapCheck));

			apos = syncP.first;
			bpos = syncP.second;
		}

		_diff_append(apos, bpos, Engine(&_a[apos], _a_size - apos, &_b[bpos], _b_size - bpos)(doSwapCheck));
	}

	if (doDiffsCombine)
		_combine_diffs();

	if (doBoundaryShift)
		_shift_boundaries();

	return _diff;
}


template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_diff_append(intptr_t aoff, intptr_t boff, std::vector<diff_info<UserDataT>>&& diff)
{
	if (diff.empty())
		return;

	for (auto& d: diff)
		d.off += (d.type == diff_type::DIFF_IN_2) ? boff : aoff;

	auto dItr = diff.begin();

	if (_diff.size() && _diff.back().type == dItr->type)
	{
		_diff.back().len += dItr->len;
		dItr++;
	}

	_diff.insert(_diff.end(), dItr, diff.end());
}


// If a whole matching block is contained at the end of the next diff block shift match down:
// If [] surrounds the marked differences, basically [abc]d[efgd]hi is the same as [abcdefg]dhi
// We combine diffs to make results more compact and clean
template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_combine_diffs()
{
	for (intptr_t i = 1; i < static_cast<intptr_t>(_diff.size()); ++i)
	{
		if (_diff[i].type != diff_type::DIFF_MATCH)
			continue;

		if (i + 1 < static_cast<intptr_t>(_diff.size()))
		{
			const Elem*	el	= _b;

			if (_diff[i + 1].type == diff_type::DIFF_IN_1)
			{
				// If there is DIFF_IN_2 after DIFF_IN_1 both sequences are changed - diff endings don't match for sure
				if ((i + 2 < static_cast<intptr_t>(_diff.size())) && (_diff[i + 2].type == diff_type::DIFF_IN_2))
				{
					i += 2;
					continue;
				}

				el	= _a;
			}

			diff_info<UserDataT>& match = _diff[i];
			diff_info<UserDataT>* next_diff = &_diff[i + 1];

			if (match.len > next_diff->len)
			{
				++i;
				continue;
			}

			intptr_t match_len = match.len;

			intptr_t match_off = next_diff->off - 1;
			intptr_t check_off = next_diff->off + next_diff->len - 1;

			while ((match_len > 0) && (el[match_off] == el[check_off]))
			{
				--match_off;
				--check_off;
				--match_len;
			}

			if (match_len > 0)
			{
				++i;
				continue;
			}

			// The whole match is contained at the end of the next diff -
			// move the match down linking the surrounding diffs and matches

			// Link match to the next matching block
			if (i + 2 < static_cast<intptr_t>(_diff.size()))
			{
				_diff[i + 2].off -= match.len;
				_diff[i + 2].len += match.len;
			}
			// Create new match block at the end
			else
			{
				diff_info<UserDataT> end_match;

				end_match.type = diff_type::DIFF_MATCH;
				end_match.off = match.off + next_diff->len;
				end_match.len = match.len;

				_diff.emplace_back(end_match);
			}

			next_diff->off -= match.len;

			_diff.erase(_diff.begin() + i);

			next_diff = &_diff[i];

			intptr_t k = i - 1;

			diff_info<UserDataT>* prev_diff = &_diff[k];

			if (next_diff->type != prev_diff->type)
			{
				if ((k > 0) && (_diff[k - 1].type == next_diff->type))
					prev_diff = &_diff[--k];
			}

			// Merge diffs
			if (next_diff->type == prev_diff->type)
			{
				prev_diff->len += next_diff->len;

				_diff.erase(_diff.begin() + i);
				--i;
			}
			// Swap diffs to represent block replacement (DIFF_IN_1 followed by DIFF_IN_2)
			else if (next_diff->type == diff_type::DIFF_IN_1)
			{
				std::swap(prev_diff->type, next_diff->type);
				std::swap(prev_diff->off,  next_diff->off);
				std::swap(prev_diff->len,  next_diff->len);
			}

			// Check if previous match is suitable for combining
			if (k > 1)
				i = k - 2;
		}
	}
}


// Algorithm borrowed from WinMerge
// If the Elem after the DIFF_IN_1 is the same as the first Elem of the DIFF_IN_1, shift differences down:
// If [] surrounds the marked differences, basically [abb]a is the same as a[bba]
// Since most languages start with unique elem and end with repetitive elem (end, </node>, }, ], ), >, etc)
// we shift the differences down to make results look cleaner
template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_shift_boundaries()
{
	for (intptr_t i = 0; i < static_cast<intptr_t>(_diff.size()); ++i)
	{
		if (_diff[i].type == diff_type::DIFF_MATCH)
			continue;

		if (i + 1 < static_cast<intptr_t>(_diff.size()))
		{
			const Elem*	el	= _b;

			if (_diff[i].type == diff_type::DIFF_IN_1)
			{
				// If there is DIFF_IN_2 after DIFF_IN_1 both sequences are changed - boundaries do not match for sure
				if (_diff[i + 1].type == diff_type::DIFF_IN_2)
				{
					++i;
					continue;
				}

				el	= _a;
			}

			diff_info<UserDataT>& diff = _diff[i];
			diff_info<UserDataT>* next_match_diff = &_diff[i + 1];

			const intptr_t max_len = (diff.len > next_match_diff->len) ? next_match_diff->len : diff.len;

			intptr_t check_off = diff.off + diff.len;
			intptr_t shift_len = 0;

			while (shift_len < max_len && el[diff.off] == el[check_off])
			{
				++diff.off;
				++check_off;
				++shift_len;
			}

			// Diff block shifted - we need to adjust the surrounding matching blocks accordingly
			if (shift_len)
			{
				if (i > 0)
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

				next_match_diff = &_diff[i + 1];

				next_match_diff->off += shift_len;
				next_match_diff->len -= shift_len;

				// The whole match diff shifted - erase it and merge surrounding diff blocks
				if (next_match_diff->len == 0)
				{
					intptr_t j = i + 1;

					_diff.erase(_diff.begin() + j);

					if (j < static_cast<intptr_t>(_diff.size()) && _diff[i].type == _diff[j].type)
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
