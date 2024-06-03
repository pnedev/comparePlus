/* Fast Myers Diff algorithm implemented in C++
 */

/* Modified into template class DiffCalc
 * Copyright (C) 2024  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include "Compare.h"


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

	// Runs the actual compare and returns the differences + swap flag indicating if the
	// compared sequences have been swapped for better results (if true, _a and _b have been swapped,
	// meaning that DIFF_IN_1 in the differences is regarding _b instead of _a)
	std::pair<std::vector<diff_info<UserDataT>>, bool> operator()(bool doDiffsCombine = false,
			bool doBoundaryShift = false);

	DiffCalc(const DiffCalc&) = delete;
	const DiffCalc& operator=(const DiffCalc&) = delete;

private:
	static constexpr int _cCancelCheckItrInterval {300000};

	struct DiffState
	{
		intptr_t i;
		intptr_t N;
		intptr_t j;
		intptr_t M;

		intptr_t Z;

		intptr_t pxs;
		intptr_t pxe;
		intptr_t pys;
		intptr_t pye;
		intptr_t oxs;
		intptr_t oxe;
		intptr_t oys;
		intptr_t oye;

		std::vector<intptr_t>& stack;
		std::vector<intptr_t>& buf;
	};

	inline bool _cancel_check();

	void _add(diff_type type, intptr_t off, intptr_t len);
	inline void _to_diff_blocks(intptr_t& aoff, intptr_t& boff, intptr_t as, intptr_t ae, intptr_t bs, intptr_t be);

	intptr_t _diff_core(intptr_t aoff, intptr_t asize, intptr_t boff, intptr_t bsize);
	int _diff_internal(DiffState& state, int c);

	void _combine_diffs();
	void _shift_boundaries();
	inline intptr_t _count_replaces();

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	IsCancelledFn _isCancelled;
	int _cancelCheckCount;

	std::vector<diff_info<UserDataT>> _diff;
};


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
		IsCancelledFn isCancelled) :
	_a(v1.data()), _a_size(v1.size()), _b(v2.data()), _b_size(v2.size()),
	_isCancelled(isCancelled), _cancelCheckCount(_cCancelCheckItrInterval)
{
}


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
		IsCancelledFn isCancelled) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size),
	_isCancelled(isCancelled), _cancelCheckCount(_cCancelCheckItrInterval)
{
}


template <typename Elem, typename UserDataT>
std::pair<std::vector<diff_info<UserDataT>>, bool> DiffCalc<Elem, UserDataT>::operator()(bool doDiffsCombine,
		bool doBoundaryShift)
{
	bool swapped = false;

	/* The diff algorithm assumes we begin with a diff. The following ensures this is true by skipping any matches
	 * in the beginning. This also helps to quickly process sequences that match entirely.
	 */
	intptr_t off_s = 0;

	intptr_t asize = _a_size;
	intptr_t bsize = _b_size;

	while (off_s < asize && off_s < bsize && _a[off_s] == _b[off_s])
		++off_s;

	if (off_s)
		_add(diff_type::DIFF_MATCH, 0, off_s);

	if (asize == bsize && off_s == asize)
		return std::make_pair(_diff, swapped);

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

	if (_diff_core(off_s, asize, off_s, bsize) == -1)
	{
		_diff.clear();
		return std::make_pair(_diff, swapped);
	}

	// Swap compared sequences and re-compare to see if result is more optimal
	{
		const intptr_t replacesCount = _count_replaces();

		// Store current compare result
		std::vector<diff_info<UserDataT>> storedDiff = std::move(_diff);
		std::swap(_a, _b);
		std::swap(asize, bsize);
		swapped = !swapped;

		// Restore first matching block before continuing
		if (storedDiff[0].type == diff_type::DIFF_MATCH)
			_diff.push_back(storedDiff[0]);

		if (_diff_core(off_s, asize, off_s, bsize) == -1)
		{
			_diff.clear();
			return std::make_pair(_diff, swapped);
		}

		// If re-compare result is not more optimal - restore the previous state
		if (_count_replaces() < replacesCount)
		{
			_diff = std::move(storedDiff);
			std::swap(_a, _b);
			swapped = !swapped;
		}
		else
		{
			std::swap(aend, bend);
		}
	}

	if (off_e)
		_add(diff_type::DIFF_MATCH, aend - off_e + 1, off_e);

	if (doDiffsCombine)
		_combine_diffs();

	if (doBoundaryShift)
		_shift_boundaries();

	return std::make_pair(_diff, swapped);
}


template <typename Elem, typename UserDataT>
inline bool DiffCalc<Elem, UserDataT>::_cancel_check()
{
	if (!--_cancelCheckCount)
	{
		if (_isCancelled && _isCancelled())
			return true;

		_cancelCheckCount = _cCancelCheckItrInterval;
	}

	return false;
}


template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_add(diff_type type, intptr_t off, intptr_t len)
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
inline void DiffCalc<Elem, UserDataT>::_to_diff_blocks(intptr_t& aoff, intptr_t& boff,
	intptr_t as, intptr_t ae, intptr_t bs, intptr_t be)
{
	if (as - aoff > 0)
		_add(diff_type::DIFF_MATCH, aoff, as - aoff);

	if (ae - as > 0)
	{
		_add(diff_type::DIFF_IN_1, as, ae - as);
		aoff = ae;
	}

	if (be - bs > 0)
	{
		_add(diff_type::DIFF_IN_2, bs, be - bs);
		aoff = ae;
		boff = be;
	}
}


template <typename Elem, typename UserDataT>
intptr_t DiffCalc<Elem, UserDataT>::_diff_core(intptr_t aoff, intptr_t asize, intptr_t boff, intptr_t bsize)
{
	const intptr_t Z = 2 * (std::min(asize, bsize) + 1);

	std::vector<intptr_t> stack;
	std::vector<intptr_t> buf;

	DiffState state {
		aoff, asize, boff, bsize, Z,
		-1, -1, -1, -1,
		-1, -1, -1, -1,
		stack, buf
	};

	for (int c = 0; c < 2;)
	{
		c = _diff_internal(state, c);

		if (c < 0)
			return -1;

		if (c == 1)
		{
			// LOGD(LOG_ALGO, "O -> " + std::to_string(state.oxs) + ", " + std::to_string(state.oxe) + " / " +
					// std::to_string(state.oys) + ", " + std::to_string(state.oye) + "\n");

			_to_diff_blocks(aoff, boff, state.oxs, state.oxe, state.oys, state.oye);

			continue;
		}

		if (state.pxs >= 0)
		{
			// LOGD(LOG_ALGO, "P -> " + std::to_string(state.pxs) + ", " + std::to_string(state.pxe) + " / " +
					// std::to_string(state.pys) + ", " + std::to_string(state.pye) + "\n");

			_to_diff_blocks(aoff, boff, state.pxs, state.pxe, state.pys, state.pye);

			continue;
		}

		break;
	}

	return 0;
}


// Find the list of differences between 2 lists by
// recursive subdivision, requring O(min(N,M)) space
// and O(min(N,M)*D) worst-case execution time where
// D is the number of differences.
template <typename Elem, typename UserDataT>
int DiffCalc<Elem, UserDataT>::_diff_internal(DiffState& state, int c)
{
	intptr_t i = state.i;
	intptr_t N = state.N;
	intptr_t j = state.j;
	intptr_t M = state.M;
	intptr_t Z = state.Z;

	std::vector<intptr_t>& stack = state.stack;
	std::vector<intptr_t>& buf = state.buf;

	for (;;)
	{
		switch(c)
		{
			case 0:
			{
				Z_block: while (N > 0 && M > 0)
				{
					buf.assign(3 * Z, 0);
					intptr_t* b = buf.data() + Z;

					const intptr_t W = N - M;
					const intptr_t L = N + M;
					const intptr_t parity = L & 1;
					const intptr_t offsetx = i + N - 1;
					const intptr_t offsety = j + M - 1;
					const intptr_t hmax = (L + parity) / 2;

					intptr_t z;

					// h_loop
					for (intptr_t h = 0; h <= hmax; ++h)
					{
						const intptr_t kmin = 2 * std::max((intptr_t)0, h - M) - h;
						const intptr_t kmax = h - 2 * std::max((intptr_t)0, h - N);

						// Forward pass
						for (intptr_t k = kmin; k <= kmax; k += 2)
						{
							if (_cancel_check())
								return -1;

							const intptr_t gkm = b[k - 1 - Z * (intptr_t)std::floor((k - 1)/Z)];
							const intptr_t gkp = b[k + 1 - Z * (intptr_t)std::floor((k + 1)/Z)];
							const intptr_t u = (k == -h || (k != h && gkm < gkp)) ? gkp : gkm + 1;
							const intptr_t v = u - k;

							intptr_t x = u;
							intptr_t y = v;

							while (x < N && y < M && (_a[i + x] == _b[j + y]))
								++x, ++y;

							b[k - Z * (intptr_t)std::floor(k/Z)] = x;

							if (parity == 1 && (z = W - k) >= 1 - h && z < h &&
								x + b[Z + z - Z * (intptr_t)std::floor(z/Z)] >= N)
							{
								if (h > 1 || x != u)
								{
									stack.push_back(i + x);
									stack.push_back(N - x);
									stack.push_back(j + y);
									stack.push_back(M - y);

									N = u;
									M = v;
									Z = 2 * (std::min(N, M) + 1);

									goto Z_block;
								}
								else
								{
									goto h_loop_end;
								}
							}
						}

						// Reverse pass
						for (intptr_t k = kmin; k <= kmax; k += 2)
						{
							if (_cancel_check())
								return -1;

							const intptr_t pkm = b[Z + k - 1 - Z * (intptr_t)std::floor((k - 1)/Z)];
							const intptr_t pkp = b[Z + k + 1 - Z * (intptr_t)std::floor((k + 1)/Z)];
							const intptr_t u = (k == -h || (k != h && pkm < pkp)) ? pkp : pkm + 1;
							const intptr_t v = u - k;

							intptr_t x = u;
							intptr_t y = v;

							while (x < N && y < M && (_a[offsetx - x] == _b[offsety - y]))
								++x, ++y;

							b[Z + k - Z * (intptr_t)std::floor(k/Z)] = x;

							if (parity == 0 && (z = W - k) >= -h && z <= h &&
								x + b[z - Z * (intptr_t)std::floor(z/Z)] >= N)
							{
								if (h > 0 || x != u)
								{
									stack.push_back(i + N - u);
									stack.push_back(u);
									stack.push_back(j + M - v);
									stack.push_back(v);

									N = N - x;
									M = M - y;
									Z = 2 * (std::min(N, M) + 1);

									goto Z_block;
								}
								else
								{
									goto h_loop_end;
								}
							}
						}
					}

					h_loop_end:;

					if (N == M)
						continue;

					if (M > N)
					{
						i += N;
						j += N;
						M -= N;
						N = 0;
					}
					else
					{
						i += M;
						j += M;
						N -= M;
						M = 0;
					}

					// We already know either N or M is zero, so we can
					// skip the extra check at the top of the loop.
					break;
				}

				// yield delete_start, delete_end, insert_start, insert_end
				// At this point, at least one of N & M is zero, or we
				// wouldn't have gotten out of the preceding loop yet.
				if (N + M != 0)
				{
					if (state.pxe == i || state.pye == j)
					{
						// it is a contiguous difference extend the existing one
						state.pxe = i + N;
						state.pye = j + M;
					}
					else
					{
						const intptr_t sx = state.pxs;

						state.oxs = state.pxs;
						state.oxe = state.pxe;
						state.oys = state.pys;
						state.oye = state.pye;

						// Defer this one until we can check the next one
						state.pxs = i;
						state.pxe = i + N;
						state.pys = j;
						state.pye = j + M;

						if (sx >= 0)
						{
							state.i = i;
							state.N = N;
							state.j = j;
							state.M = M;
							state.Z = Z;

							return 1;
						}
					}
				}
			}
			// Intentional fall-through

			case 1:
			{
				if (stack.empty())
					return 2;

				M = stack.back();
				stack.pop_back();
				j = stack.back();
				stack.pop_back();
				N = stack.back();
				stack.pop_back();
				i = stack.back();
				stack.pop_back();

				Z = 2 * (std::min(N, M) + 1);
				c = 0;
			}
		}
	}

	return -1;
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
				std::swap(*prev_diff, *next_diff);
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

		const Elem*	el	= _b;

		if (_diff[i].type == diff_type::DIFF_IN_1)
		{
			// If there is DIFF_IN_2 after DIFF_IN_1 both sequences are changed - boundaries do not match for sure
			if ((i + 1 < static_cast<intptr_t>(_diff.size())) && (_diff[i + 1].type == diff_type::DIFF_IN_2))
			{
				++i;
				continue;
			}

			el	= _a;
		}

		if (i + 1 < static_cast<intptr_t>(_diff.size()))
		{
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


template <typename Elem, typename UserDataT>
inline intptr_t DiffCalc<Elem, UserDataT>::_count_replaces()
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
