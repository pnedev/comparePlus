/* Template class DiffCalc for diff generation, wrapping the actual diff algorithm and adding common post-processing
 * Copyright (C) 2025-2026  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include <utility>

#include "diff_types.h"

#include "histogram_diff.h"
// #include "myers_diff.h"
// #include "fast_myers_diff.h"


#ifdef MULTITHREAD

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "../mingw-std-threads/mingw.thread.h"
#else
#include <thread>
#endif // __MINGW32__ ...

#endif // MULTITHREAD


/**
 *  \class  DiffCalc
 *  \brief  Compares and makes a differences list between two vectors (elements are template).
 */
template <typename Elem>
class DiffCalc
{
public:
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2, IsCancelledFn cancelledFn = nullptr);
	DiffCalc(const Elem* v1, intptr_t v1_size, const Elem* v2, intptr_t v2_size, IsCancelledFn cancelledFn = nullptr);

	// Runs the actual compare and returns the differences
	diff_results operator()(bool doDiffsCombine = false, bool doBoundaryShift = false,
			const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints = {});

	DiffCalc(const DiffCalc&) = delete;
	const DiffCalc& operator=(const DiffCalc&) = delete;

private:
	diff_results _run_algo(const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize);

	void _combine_diffs(diff_results& diff);
	void _shift_boundaries(diff_results& diff);

	bool isCancelled() { return (_cancelledFn && _cancelledFn()); };

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	IsCancelledFn _cancelledFn;

	bool _diffsCombine;
	bool _boundaryShift;
};


template <typename Elem>
DiffCalc<Elem>::DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
		IsCancelledFn cancelledFn) :
	_a(v1.data()), _a_size(v1.size()), _b(v2.data()), _b_size(v2.size()), _cancelledFn(cancelledFn)
{
}


template <typename Elem>
DiffCalc<Elem>::DiffCalc(const Elem* v1, intptr_t v1_size, const Elem* v2, intptr_t v2_size,
		IsCancelledFn cancelledFn) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _cancelledFn(cancelledFn)
{
}


template <typename Elem>
diff_results DiffCalc<Elem>::_run_algo(const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize)
{
	diff_results diff;

	intptr_t off_s = 0;

	// The diff algorithm assumes we begin with a diff. The following ensures this is true by skipping any matches
	// in the beginning. This also helps to quickly process sequences that match entirely.
	while (off_s < asize && off_s < bsize && a[off_s] == b[off_s])
		++off_s;

	if (asize == bsize && off_s == asize)
		return diff;

	const intptr_t aend = asize - 1;
	const intptr_t bend = bsize - 1;

	asize -= off_s;
	bsize -= off_s;

	intptr_t off_e = 0;

	// Check also for matches at the end
	while (off_e < asize && off_e < bsize && a[aend - off_e] == b[bend - off_e])
		++off_e;

	if (off_e)
	{
		asize -= off_e;
		bsize -= off_e;
	}

	using DiffAlg = HistogramDiff<Elem>;

	DiffAlg diff_alg(_cancelledFn);

	// Compare with swapped sequences as well to see if result is more optimal
	if (diff_alg.needSwapCheck())
	{
		diff_results swapped_diff;

#ifdef MULTITHREAD
		const bool parallel_run = (asize > 3000 && bsize > 3000 && std::thread::hardware_concurrency() > 1);

		if (parallel_run)
		{
			std::thread thr = std::thread([&]()
			{
				DiffAlg(_cancelledFn).run(b, bsize, a, asize, swapped_diff, off_s);
			});

			diff_alg.run(a, asize, b, bsize, diff, off_s);

			thr.join();
		}
		else
#endif // MULTITHREAD
		{
			diff_alg.run(a, asize, b, bsize, diff, off_s);

			if (isCancelled())
				return {};

			diff_alg.run(b, bsize, a, asize, swapped_diff, off_s);
		}

		if (isCancelled())
			return {};

		const intptr_t swapped_replaces = swapped_diff.count_replaces();

		// Check which result is more optimal
		if (swapped_replaces && swapped_replaces > diff.count_replaces())
		{
			diff = std::move(swapped_diff);
			diff.swap_ab();
		}
	}
	else
	{
		diff_alg.run(a, asize, b, bsize, diff, off_s);
	}

	if (isCancelled())
		return {};

	_diffsCombine = _diffsCombine && diff_alg.needDiffsCombine();
	_boundaryShift = _boundaryShift && diff_alg.needBoundaryShift();

	return diff;
}


template <typename Elem>
diff_results DiffCalc<Elem>::operator()(bool doDiffsCombine, bool doBoundaryShift,
	const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints)
{
	diff_results diff;

	_diffsCombine = doDiffsCombine;
	_boundaryShift = doBoundaryShift;

	if (syncPoints.empty())
	{
		diff = _run_algo(_a, _a_size, _b, _b_size);
	}
	else
	{
		intptr_t apos = 0;
		intptr_t bpos = 0;

		for (const auto& syncP : syncPoints)
		{
			if (syncP.first  < apos || syncP.first  >= _a_size ||
				syncP.second < bpos || syncP.second >= _b_size)
				break;

			diff.append(_run_algo(&_a[apos], syncP.first - apos, &_b[bpos], syncP.second - bpos), apos, bpos);

			if (isCancelled())
				return {};

			apos = syncP.first;
			bpos = syncP.second;
		}

		diff.append(_run_algo(&_a[apos], _a_size - apos, &_b[bpos], _b_size - bpos), apos, bpos);
	}

	if (isCancelled())
		return {};

	if (_diffsCombine)
		_combine_diffs(diff);

	if (_boundaryShift)
		_shift_boundaries(diff);

	return diff;
}


// If a whole matching block is contained at the end of the next diff block shift match down:
// If [] surrounds the marked differences, basically [abc]d[efgd]hi is the same as [abcdefg]dhi
// We combine diffs to make results more compact and clean
template <typename Elem>
void DiffCalc<Elem>::_combine_diffs(diff_results& diff)
{
	const intptr_t diffs_size = static_cast<intptr_t>(diff.size());

	for (intptr_t i = 1; i < diffs_size; ++i)
	{
		// If both sequences are changed diff endings differ for sure -> cannot both match previous match block
		if (diff[i].is_replacement())
			continue;

		const Elem*		el;
		const range_t*	d;
		range_t*		prev_d;

		if (diff[i].a.len())
		{
			el = _a;
			d = &diff[i].a;
			prev_d = &diff[i - 1].a;
		}
		else
		{
			el = _b;
			d = &diff[i].b;
			prev_d = &diff[i - 1].b;
		}

		intptr_t match_idx = d->s - 1;
		intptr_t diff_idx = d->e - 1;

		while (match_idx >= prev_d->e && diff_idx > d->s && el[match_idx] == el[diff_idx])
		{
			--match_idx;
			--diff_idx;
		}

		// The whole match block between adjecent diff blocks match with the diff block's end ->
		// combine diffs shifting match down
		if (match_idx < prev_d->e)
		{
			const intptr_t shift = d->s - match_idx;

			prev_d->e = d->e - shift;

			if (i + 1 < diffs_size)
			{
				if (!diff[i + 1].a.len())
					diff[i + 1].a.shift(-shift);
				else if (!diff[i + 1].b.len())
					diff[i + 1].b.shift(-shift);
			}

			// Diff merged into previous - erase it and recheck same idx again
			diff.erase(diff.begin() + i);
			--i;
		}
	}
}


// Algorithm borrowed from WinMerge
// If the Elem after the diff is the same as the first Elem of the diff shift differences down:
// If [] surrounds the marked differences, basically [abb]a is the same as a[bba]
// Since the most coding languages start with unique elem and end with repetitive elem (end, </node>, }, ], ), >, etc)
// we shift the differences down to make results look cleaner
template <typename Elem>
void DiffCalc<Elem>::_shift_boundaries(diff_results& diff)
{
	const intptr_t diffs_size = static_cast<intptr_t>(diff.size());

	for (intptr_t i = 0; i < diffs_size; ++i)
	{
		// If both sequences are changed boundaries differ for sure
		if (diff[i].is_replacement())
			continue;

		const Elem*		el;
		const range_t*	d;
		intptr_t		end_idx;

		// Changed range (a or b)
		if (diff[i].a.len())
		{
			el = _a;
			d = &diff[i].a;
			end_idx = (i + 1 < diffs_size) ? diff[i + 1].a.s : _a_size;
		}
		else
		{
			el = _b;
			d = &diff[i].b;
			end_idx = (i + 1 < diffs_size) ? diff[i + 1].b.s : _b_size;
		}

		intptr_t diff_idx = d->s;
		intptr_t match_idx = d->e;

		while (diff_idx < d->e && match_idx < end_idx && el[diff_idx] == el[match_idx])
		{
			++diff_idx;
			++match_idx;
		}

		const intptr_t shift = diff_idx - d->s;

		if (shift > 0)
		{
			diff[i].shift(shift);

			// Check if next diff should be merged
			if (i + 1 < diffs_size && diff[i].glue(diff[i + 1]))
			{
				// Diff blocks merged - erase next and recheck same idx again
				diff.erase(diff.begin() + (i + 1));
				--i;
			}
		}
	}
}
