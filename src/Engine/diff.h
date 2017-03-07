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

#include <cstdlib>
#include <climits>
#include <utility>
#include "varray.h"


enum class diff_type
{
	DIFF_MATCH,
	DIFF_DELETE,
	DIFF_INSERT
};


enum detect_moves_type
{
	DONT_DETECT = 0,
	ELEMENT_BASED,
	BLOCK_BASED
};


enum moved_type
{
	NOT_MOVED = 0,
	MOVED,
	MOVED_MULTIPLE
};


struct section_t
{
	int off;
	int len;
};


struct diff_line
{
	diff_line(int lineNum) : line(lineNum) {}

	int line;
	std::vector<section_t> changes;
};


struct diff_info
{
	diff_type	type;
	int			off; // off into _a if MATCH or DELETE but into _b if INSERT
	int			len;

	// added lines for Notepad++ use
	const diff_info*	matchedDiff {nullptr};

	std::vector<diff_line>	changedLines;
	std::vector<moved_type>	moved;

	inline moved_type isMoved(unsigned int i) const
	{
		return (moved.empty() ? NOT_MOVED : moved[i]);
	}
};


/**
 *  \class  DiffCalc
 *  \brief  Compares and makes a differences list between two vectors (elements are template, must have operator==)
 */
template <typename Elem>
class DiffCalc
{
public:
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
			detect_moves_type findMoves = DONT_DETECT, int max = INT_MAX) :
		_a(v1), _b(v2), _findMoves(findMoves), _dmax(max) {}

	std::vector<diff_info> operator()();

	DiffCalc& operator=(const DiffCalc&) = delete;

private:
	struct middle_snake {
		int x, y, u, v;
	};

	struct move_match_info
	{
		section_t								asec;
		std::vector<std::pair<diff_info*, int>>	matches;
	};

	void _setv(int k, int r, int val);
	int _v(int k, int r);
	void _edit(diff_type type, int off, int len);
	int _find_middle_snake(int aoff, int aend, int boff, int bend, middle_snake* ms);
	int _ses(int aoff, int aend, int boff, int bend);
	void _shift_boundries();
	void _find_b_matches(const diff_info& adiff, int aidx, move_match_info& matchInfo);
	void _find_moves();

	const std::vector<Elem>& _a;
	const std::vector<Elem>& _b;

	std::vector<diff_info> _diff;

	const detect_moves_type	_findMoves;
	const int				_dmax;
	varray<int>				_buf;
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
void DiffCalc<Elem>::_edit(diff_type type, int off, int len)
{
	if (len == 0)
		return;

	bool add_elem = _diff.empty();

	if (!add_elem)
		add_elem = (_diff.rbegin()->type != type);

	if (add_elem)
	{
		diff_info new_di;
		new_di.type = type;
		new_di.off = off;
		new_di.len = len;
		_diff.push_back(new_di);
	}
	else
	{
		_diff.rbegin()->len += len;
	}
}


template <typename Elem>
int DiffCalc<Elem>::_find_middle_snake(int aoff, int aend, int boff, int bend, middle_snake* ms)
{
	const int delta = aend - bend;
	const int odd = delta & 1;
	const int mid = (aend + bend) / 2 + odd;

	_setv(1, 0, 0);
	_setv(delta - 1, 1, aend);

	for (int d = 0; d <= mid; d++)
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

			ms->x = x;
			ms->y = y;

			while (x < aend && y < bend &&  _a[aoff + x] == _b[boff + y])
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
int DiffCalc<Elem>::_ses(int aoff, int aend, int boff, int bend)
{
	middle_snake ms;
	int d;

	if (aend == 0)
	{
		_edit(diff_type::DIFF_INSERT, boff, bend);
		d = bend;
	}
	else if (bend == 0)
	{
		_edit(diff_type::DIFF_DELETE, aoff, aend);
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
					_edit(diff_type::DIFF_INSERT, boff + (bend - 1), 1);
				}
				else
				{
					_edit(diff_type::DIFF_INSERT, boff, 1);
					_edit(diff_type::DIFF_MATCH, aoff, aend);
				}
			}
			else
			{
				if (x == u)
				{
					_edit(diff_type::DIFF_MATCH, aoff, bend);
					_edit(diff_type::DIFF_DELETE, aoff + (aend - 1), 1);
				}
				else
				{
					_edit(diff_type::DIFF_DELETE, aoff, 1);
					_edit(diff_type::DIFF_MATCH, aoff + 1, bend);
				}
			}
		}
	}

	return d;
}


// Algorithm borrowed from WinMerge
// If the Elem after the delete is the same as the first Elem of the delete, shift differences down:
// If [] surrounds the marked differences, basically [abb]a is the same as a[bba]
// Since most languages start with unique elem and end with repetitive elem (end, </node>, }, ], ), >, etc)
// we shift the differences down to make results look cleaner
template <typename Elem>
void DiffCalc<Elem>::_shift_boundries()
{
	const int diff_size = static_cast<int>(_diff.size());
	const int asize = static_cast<int>(_a.size());
	const int bsize = static_cast<int>(_b.size());

	for (int i = 0; i < diff_size; ++i)
	{
		int amax = asize;
		int bmax = bsize;

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

		if (_diff[i].type == diff_type::DIFF_DELETE)
		{
			if (i + 1 < diff_size)
			{
				// If there is an insert after a delete, there is a potential match, so both blocks
				// need to be moved at the same time
				if (_diff[i + 1].type == diff_type::DIFF_INSERT)
				{
					diff_info& adiff = _diff[i];
					diff_info& bdiff = _diff[i + 1];

					bmax = bsize;

					for (int j = i + 2; j < diff_size; ++j)
					{
						if (bdiff.type == _diff[j].type)
						{
							bmax = _diff[j].off;
							break;
						}
					}

					++i;

					int aend = adiff.off + adiff.len;
					int bend = bdiff.off + bdiff.len;

					while (aend < amax && bend < bmax &&
							_a[adiff.off] == _a[aend] && _b[bdiff.off] == _b[bend])
					{
						++aend;
						++bend;
						++(adiff.off);
						++(bdiff.off);
					}
				}
				else
				{
					diff_info& adiff = _diff[i];

					int aend = adiff.off + adiff.len;

					while (aend < amax && _a[adiff.off] == _a[aend])
					{
						++aend;
						++(adiff.off);
					}
				}
			}
		}
		else if (_diff[i].type == diff_type::DIFF_INSERT)
		{
			diff_info& adiff = _diff[i];

			int aend = adiff.off + adiff.len;

			while (aend < bmax && _b[adiff.off] == _b[aend])
			{
				++aend;
				++(adiff.off);
			}
		}
	}
}


// Scan for best matching b blocks (containing aidx element).
template <typename Elem>
void DiffCalc<Elem>::_find_b_matches(const diff_info& adiff, int aidx, move_match_info& matchInfo)
{
	matchInfo.asec.len = 0;
	matchInfo.matches.clear();

	for (diff_info& bdiff : _diff)
	{
		// Is it bdiff?
		if (bdiff.type != diff_type::DIFF_INSERT)
			continue;

		for (int i = 0; i < bdiff.len; ++i)
		{
			if (bdiff.isMoved(i) || _a[adiff.off + aidx] != _b[bdiff.off + i])
				continue;

			int astart	= aidx - 1;
			int aend	= aidx + 1;
			int bstart	= i - 1;
			int bend	= i + 1;

			if (_findMoves == BLOCK_BASED)
			{
				// Check for the beginning of a matched block (containing aidx element).
				for (; astart >= 0 && bstart >= 0 && !bdiff.isMoved(bstart) &&
						_a[adiff.off + astart] == _b[bdiff.off + bstart]; --astart, --bstart);

				// Check for the end of a matched block (containing aidx element).
				for (; aend < adiff.len && bend < bdiff.len && !bdiff.isMoved(bend) &&
						_a[adiff.off + aend] == _b[bdiff.off + bend]; ++aend, ++bend);
			}

			++astart;
			++bstart;
			--aend;

			const int matchLen = aend - astart + 1;

			if (matchLen < matchInfo.asec.len)
				continue;

			if (matchLen > matchInfo.asec.len)
			{
				matchInfo.asec.off = astart;
				matchInfo.asec.len = matchLen;
				matchInfo.matches.clear();
			}

			if (matchInfo.asec.off == astart)
			{
				i = bstart + matchLen - 1;

				matchInfo.matches.emplace_back(&bdiff, bstart);
			}
		}
	}
}


template <typename Elem>
void DiffCalc<Elem>::_find_moves()
{
	const int diff_size = static_cast<int>(_diff.size());

	for (int i = 0; i < diff_size; ++i)
	{
		diff_info& adiff = _diff[i];

		// Is it adiff?
		if (adiff.type != diff_type::DIFF_DELETE)
			continue;

		// Go through all a diff's elements and check if each is moved
		for (int aidx = 0; aidx < adiff.len; ++aidx)
		{
			// Skip already detected moves and blanks
			if (adiff.isMoved(aidx) || _a[adiff.off + aidx] == 0)
				continue;

			move_match_info matchInfo;

			_find_b_matches(adiff, aidx, matchInfo);

			if (matchInfo.matches.empty())
				continue;

			diff_info* best_match_adiff = &adiff;

			// Search for the same a element in different deleted block - potential better match or multiple move
			for (int j = i + 1; j < diff_size; ++j)
			{
				diff_info& alt_adiff = _diff[j];

				// Is it adiff?
				if (alt_adiff.type != diff_type::DIFF_DELETE)
					continue;

				for (int alt_aidx = 0; alt_aidx < alt_adiff.len; ++alt_aidx)
				{
					if (alt_adiff.isMoved(alt_aidx) || _a[alt_adiff.off + alt_aidx] != _a[adiff.off + aidx])
						continue;

					move_match_info alt_matchInfo;

					// Find alternative matches
					_find_b_matches(alt_adiff, alt_aidx, alt_matchInfo);

					if (alt_matchInfo.matches.empty())
						continue;

					// The alternative match is actually better - the length of the matching block is bigger
					if (matchInfo.asec.len < alt_matchInfo.asec.len)
					{
						matchInfo = alt_matchInfo;
						best_match_adiff = &alt_adiff;

						alt_aidx = alt_matchInfo.asec.off + alt_matchInfo.asec.len - 1;
					}
					// Both matching blocks are of equal size - check if those are actually one and the same block
					// (multi-moved block)
					else if (matchInfo.asec.len == alt_matchInfo.asec.len)
					{
						int k = 0;

						for (; k < alt_matchInfo.asec.len &&
								_a[best_match_adiff->off + matchInfo.asec.off + k] ==
								_a[alt_adiff.off + alt_matchInfo.asec.off + k]; ++k);

						if (k == alt_matchInfo.asec.len)
						{
							// Blocks are identical - a multi-move detected
							matchInfo.matches.emplace_back(&alt_adiff, alt_matchInfo.asec.off);

							alt_aidx = alt_matchInfo.asec.off + alt_matchInfo.asec.len - 1;
						}
					}
				}
			}

			const moved_type moveType = (matchInfo.matches.size() == 1) ? MOVED : MOVED_MULTIPLE;

			// Move found - initialize move vectors
			if (best_match_adiff->moved.empty())
				best_match_adiff->moved.resize(best_match_adiff->len, NOT_MOVED);

			int end = matchInfo.asec.off + matchInfo.asec.len - 1;

			for (int k = matchInfo.asec.off; k <= end; ++k)
				best_match_adiff->moved[k] = moveType;

			for (auto& match : matchInfo.matches)
			{
				diff_info*	match_di	= match.first;
				int 		off			= match.second;

				if (match_di->moved.empty())
					match_di->moved.resize(match_di->len, NOT_MOVED);

				end = off + matchInfo.asec.len - 1;

				for (int k = off; k <= end; ++k)
					match_di->moved[k] = moveType;
			}

			// If the best a element matching block is the current the skip checks to the end  of the match block
			if (best_match_adiff == &adiff)
				aidx = matchInfo.asec.off + matchInfo.asec.len - 1;
			// Otherwise the current a element is still not matched - recheck it
			else
				--aidx;
		}
	}
}


template <typename Elem>
std::vector<diff_info> DiffCalc<Elem>::operator()()
{
	/* The _ses function assumes the SES will begin or end with a delete
	 * or insert. The following will ensure this is true by eating any
	 * beginning matches. This is also a quick to process sequences
	 * that match entirely.
	 */
	int x = 0, y = 0;

	int asize = static_cast<int>(_a.size());
	int bsize = static_cast<int>(_b.size());

	while (x < asize && y < bsize && _a[x] == _b[y])
	{
		++x;
		++y;
	}

	if (asize == bsize && x == asize)
		return _diff;

	_edit(diff_type::DIFF_MATCH, 0, x);

	asize -= x;
	bsize -= y;

	if (_ses(x, asize, y, bsize) == -1)
	{
		_diff.clear();
	}
	else
	{
		_shift_boundries();

		if (_findMoves)
			_find_moves();
	}

	// Wipe temporal buffer to free memory
	_buf.get().clear();

	return _diff;
}
