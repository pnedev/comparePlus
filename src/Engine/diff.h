/* Template class DiffCalc for diff generation, wrapping the actual diff algorithm and adding common post-processing
 * Copyright (C) 2025-2026  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include <utility>
#include <memory>
#include <vector>
#include <span>
#include <exception>

#include "diff_types.h"

#include "histogram_diff.h"
#include "myers_diff.h"


#ifdef MULTITHREAD

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "../mingw-std-threads/mingw.thread.h"
#else
#include <thread>
#endif // __MINGW32__ ...

#endif // MULTITHREAD


enum class DiffAlg {
	HISTOGRAM,
	MYERS,
	MIXED
};


/**
 *  \class  DiffCalc
 *  \brief  Compares and makes a differences list between two vectors (elements are template).
			cancelCheck() is a function that shall throw exception on cancel that shall be handled by upper layers
 */
template <typename Elem>
class DiffCalc
{
public:
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2, ThrowIfCancelledFn cancelCheck = nullptr);
	DiffCalc(const std::span<Elem>& v1, const std::span<Elem>& v2, ThrowIfCancelledFn cancelCheck = nullptr);
	DiffCalc(const Elem* v1, intptr_t v1_size, const Elem* v2, intptr_t v2_size,
			ThrowIfCancelledFn cancelCheck = nullptr);

	// Runs the actual compare and returns the differences
	diff_results operator()(DiffAlg alg = DiffAlg::MIXED,
			bool doDiffsCombine = false, bool doBoundaryShift = false,
			const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints = {});

	DiffCalc(const DiffCalc&) = delete;
	const DiffCalc& operator=(const DiffCalc&) = delete;

private:
	diff_results _run_algo(DiffAlg alg, const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize);

	void _combine_diffs(diff_results& diffs);
	void _shift_boundaries(diff_results& diffs);

	void ThrowIfCancelled() { if (_cancelCheck) _cancelCheck(); };

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	ThrowIfCancelledFn _cancelCheck;

	bool _diffsCombine;
	bool _boundaryShift;
};


template <typename Elem>
DiffCalc<Elem>::DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
		ThrowIfCancelledFn cancelCheck) :
	_a(v1.data()), _a_size(v1.size()), _b(v2.data()), _b_size(v2.size()), _cancelCheck(cancelCheck)
{
}


template <typename Elem>
DiffCalc<Elem>::DiffCalc(const std::span<Elem>& s1, const std::span<Elem>& s2,
		ThrowIfCancelledFn cancelCheck) :
	_a(s1.data()), _a_size(s1.size()), _b(s2.data()), _b_size(s2.size()), _cancelCheck(cancelCheck)
{
}


template <typename Elem>
DiffCalc<Elem>::DiffCalc(const Elem* v1, intptr_t v1_size, const Elem* v2, intptr_t v2_size,
		ThrowIfCancelledFn cancelCheck) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _cancelCheck(cancelCheck)
{
}


template <typename Elem>
diff_results DiffCalc<Elem>::_run_algo(DiffAlg alg, const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize)
{
	diff_results diffs;

	intptr_t off_s = 0;

	// The diff algorithm assumes we begin with a diff. The following ensures this is true by skipping any matches
	// in the beginning. This also helps to quickly process sequences that match entirely.
	while (off_s < asize && off_s < bsize && a[off_s] == b[off_s])
		++off_s;

	if (asize == bsize && off_s == asize)
		return diffs;

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

	const intptr_t histogram_lowcnt = (alg == DiffAlg::MIXED) ? 1 : 250;

	std::unique_ptr<diff_algorithm<Elem>> diff_alg;

	if (alg == DiffAlg::MYERS)
		diff_alg.reset(new MyersDiff<Elem>(_cancelCheck));
	else
		diff_alg.reset(new HistogramDiff<Elem>(_cancelCheck, histogram_lowcnt));

	// Compare with swapped sequences as well to see if result is more optimal
	if (diff_alg->needSwapCheck())
	{
		diff_results swapped_diffs;

#ifdef MULTITHREAD
		const bool parallel_run = (asize > 10000 && bsize > 10000 && std::thread::hardware_concurrency() > 1);

		if (parallel_run)
		{
			std::thread thr = std::thread([&]()
			{
				try
				{
					if (alg == DiffAlg::MYERS)
						MyersDiff<Elem>(_cancelCheck).run(b, bsize, a, asize, swapped_diffs, off_s);
					else
						HistogramDiff<Elem>(_cancelCheck, histogram_lowcnt).run(
								b, bsize, a, asize, swapped_diffs, off_s);
				}
				catch (const std::exception&)
				{
				}
			});

			try
			{
				diff_alg->run(a, asize, b, bsize, diffs, off_s);
			}
			catch (const std::exception&)
			{
				std::exception_ptr ep = std::current_exception();
				thr.join();
				std::rethrow_exception(ep);
			}

			thr.join();
		}
		else
#endif // MULTITHREAD
		{
			diff_alg->run(a, asize, b, bsize, diffs, off_s);
			diff_alg->run(b, bsize, a, asize, swapped_diffs, off_s);
		}

		const intptr_t swapped_replaces = swapped_diffs.count_replaces();

		// Check which result is more optimal
		if (swapped_replaces && swapped_replaces > diffs.count_replaces())
		{
			diffs = std::move(swapped_diffs);
			diffs.swap_ab();
		}
	}
	else
	{
		diff_alg->run(a, asize, b, bsize, diffs, off_s);
	}

	_diffsCombine = _diffsCombine && diff_alg->needDiffsCombine();
	_boundaryShift = _boundaryShift && diff_alg->needBoundaryShift();

	return diffs;
}


template <typename Elem>
diff_results DiffCalc<Elem>::operator()(DiffAlg alg, bool doDiffsCombine, bool doBoundaryShift,
	const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints)
{
	_diffsCombine = doDiffsCombine;
	_boundaryShift = doBoundaryShift;

	diff_results diffs;

	if (syncPoints.empty())
	{
		diffs = _run_algo(alg, _a, _a_size, _b, _b_size);
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

			diffs.append(_run_algo(alg, &_a[apos], syncP.first - apos, &_b[bpos], syncP.second - bpos), apos, bpos);

			apos = syncP.first;
			bpos = syncP.second;
		}

		diffs.append(_run_algo(alg, &_a[apos], _a_size - apos, &_b[bpos], _b_size - bpos), apos, bpos);
	}

	if (alg == DiffAlg::MIXED)
	{
		_diffsCombine = doDiffsCombine;
		_boundaryShift = doBoundaryShift;

		diff_results rough_diffs(std::move(diffs));

		for (const auto& d : rough_diffs)
		{
			if (d.a.len() > 1 && d.b.len() > 1)
				diffs.append(_run_algo(DiffAlg::MYERS, &_a[d.a.s], d.a.len(), &_b[d.b.s], d.b.len()), d.a.s, d.b.s);
			else
				diffs.emplace_back(d);
		}
	}

	if (_diffsCombine)
		_combine_diffs(diffs);

	if (_boundaryShift)
		_shift_boundaries(diffs);

	return diffs;
}


// If a whole matching block is contained at the end of the next diff block move match down:
// If [] surrounds the marked differences, basically [abc]d[efgd]hi is the same as [abcdefg]dhi
// If a whole diff block is contained at the end of the previous match block move diff up:
// If [] surrounds the marked differences, basically [abc]defg[fg]hi is the same as [abc]de[fg]fghi
// We combine diffs to make results more compact and clean
template <typename Elem>
void DiffCalc<Elem>::_combine_diffs(diff_results& diffs)
{
	intptr_t diffs_size = static_cast<intptr_t>(diffs.size());

	for (intptr_t i = 1; i < diffs_size; ++i)
	{
		// If both sequences are changed diff doesn't match previous match for sure - cannot combine anything
		if (diffs[i].is_replacement())
			continue;

		const Elem*		el;
		const range_t*	d;
		range_t*		prev_d;

		if (diffs[i].a.len())
		{
			el = _a;
			d = &diffs[i].a;
			prev_d = &diffs[i - 1].a;
		}
		else
		{
			el = _b;
			d = &diffs[i].b;
			prev_d = &diffs[i - 1].b;
		}

		intptr_t match_idx = d->s - 1;
		intptr_t diff_idx = d->e - 1;

		while (match_idx >= prev_d->e && diff_idx >= d->s && el[match_idx] == el[diff_idx])
		{
			--match_idx;
			--diff_idx;
		}

		// The whole match block between adjecent diff blocks match with the diff block's end ->
		// combine diffs shifting match down
		if (match_idx < prev_d->e)
		{
			prev_d->e = diff_idx + 1;

			// Diff merged into previous - erase it and recheck previous diff again
			diffs.erase(diffs.begin() + i);
			--diffs_size;
			i -= 2;
		}
		// The whole diff block is contained at the end of the previous match -> move diff up and recheck it
		else if (diff_idx < d->s)
		{
			diffs[i].shift(-d->len());
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
void DiffCalc<Elem>::_shift_boundaries(diff_results& diffs)
{
	intptr_t diffs_size = static_cast<intptr_t>(diffs.size());

	for (intptr_t i = 0; i < diffs_size; ++i)
	{
		// If both sequences are changed boundaries differ for sure
		if (diffs[i].is_replacement())
			continue;

		const Elem*		el;
		const range_t*	d;
		intptr_t		end_idx;

		// Changed range (a or b)
		if (diffs[i].a.len())
		{
			el = _a;
			d = &diffs[i].a;
			end_idx = (i + 1 < diffs_size) ? diffs[i + 1].a.s : _a_size;
		}
		else
		{
			el = _b;
			d = &diffs[i].b;
			end_idx = (i + 1 < diffs_size) ? diffs[i + 1].b.s : _b_size;
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
			diffs[i].shift(shift);

			// Check if next diff should be merged
			if (i + 1 < diffs_size && diffs[i].glue(diffs[i + 1]))
			{
				// Diff blocks merged - erase next and recheck same idx again
				diffs.erase(diffs.begin() + (i + 1));
				--diffs_size;
				--i;
			}
		}
	}
}
