/* Histogram diff algorithm implemented as a C++ template class HistogramDiff
 * Copyright (C) 2025  Pavel Nedev <pg.nedev@gmail.com>
 * Original algorithm is taken from Ray Gardner's C implementation in 2025
 * License: 0BSD
 */


#pragma once

#include "diff_types.h"

#include <vector>
#include <unordered_map>


template <typename Elem, typename UserDataT = void>
class HistogramDiff : public diff_algorithm<Elem, UserDataT>
{
public:
	HistogramDiff(IsCancelledFn isCancelled = nullptr) : diff_algorithm<Elem, UserDataT>(isCancelled) {};

	virtual void run(const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize,
			diff_results<UserDataT>& diff, intptr_t off);

	virtual bool needSwapCheck() { return true; };
	virtual bool needDiffsCombine() { return false; };
	virtual bool needBoundaryShift() { return false; };

private:
	using diff_algorithm<Elem, UserDataT>::_isCancelled;

	static constexpr int _cCancelCheckItrInterval {30000};

	void add_diff(diff_results<UserDataT>& diff, intptr_t al, intptr_t ah, intptr_t bl, intptr_t bh);

	void push_quad(std::vector<intptr_t>& stk, intptr_t al, intptr_t ah, intptr_t bl, intptr_t bh)
	{
		stk.emplace_back(al);
		stk.emplace_back(ah);
		stk.emplace_back(bl);
		stk.emplace_back(bh);
	};

	void pop_quad(std::vector<intptr_t>& stk, intptr_t* al, intptr_t* ah, intptr_t* bl, intptr_t* bh)
	{
		if (stk.size() < 4)
		{
			stk.clear();
			return;
		}

		*bh = stk.back();
		stk.pop_back();
		*bl = stk.back();
		stk.pop_back();
		*ah = stk.back();
		stk.pop_back();
		*al = stk.back();
		stk.pop_back();
	};

	intptr_t& getcnt_ref(intptr_t ap, std::vector<intptr_t>& acnt)
	{
		return acnt[ap] >= 0 ? acnt[ap] : acnt[-acnt[ap]];
	};


	intptr_t getcnt(intptr_t ap, const std::vector<intptr_t>& acnt)
	{
		return acnt[ap] >= 0 ? acnt[ap] : acnt[-acnt[ap]];
	};


	bool beqa(intptr_t ap, const std::vector<intptr_t>& acnt, intptr_t bp, const std::vector<intptr_t>& bref)
	{
		return bref[bp] == (acnt[ap] >= 0 ? ap : -acnt[ap]);
	};

	bool find_best_matching_region(
		const std::vector<intptr_t>& acnt, const std::vector<intptr_t>& anext, const std::vector<intptr_t>& bref,
		intptr_t alo, intptr_t ahi, intptr_t blo, intptr_t bhi,
		intptr_t* malo, intptr_t* mahi, intptr_t* mblo, intptr_t* mbhi);

	int _cancelCheckCount;

	intptr_t _last_a_pos;
};


template <typename Elem, typename UserDataT>
void HistogramDiff<Elem, UserDataT>::add_diff(diff_results<UserDataT>& diff,
	intptr_t al, intptr_t ah, intptr_t bl, intptr_t bh)
{
	if (al > _last_a_pos)
		diff._add(diff_type::DIFF_MATCH, _last_a_pos, al - _last_a_pos);

	if (ah > al)
		diff._add(diff_type::DIFF_IN_1, al, ah - al);

	if (bh > bl)
		diff._add(diff_type::DIFF_IN_2, bl, bh - bl);

	_last_a_pos = ah;
}


template <typename Elem, typename UserDataT>
void HistogramDiff<Elem, UserDataT>::run(const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize,
	diff_results<UserDataT>& diff, intptr_t off)
{
	_cancelCheckCount = _cCancelCheckItrInterval;

	_last_a_pos = off;

	for (auto it = diff.rbegin(); it != diff.rend(); ++it)
	{
		if (it->type != diff_type::DIFF_IN_2)
		{
			_last_a_pos = it->off + it->len;
			break;
		}
	}

	a += off;
	b += off;

	std::vector<intptr_t> anext(asize + 1, 0);
	std::vector<intptr_t> bref(bsize + 1, 0);
	{
		std::unordered_map<typename Elem::hash_type, intptr_t> amap;

		for (intptr_t i = asize; i; i--)
		{
			auto it = amap.find(a[i - 1].get_hash());

			if (it != amap.end())
			{
				anext[i] = it->second;
				it->second = i;
			}
			else
			{
				amap[a[i - 1].get_hash()] = i;
			}
		}

		for (intptr_t i = 1; i <= bsize; i++)
		{
			auto it = amap.find(b[i - 1].get_hash());

			if (it != amap.end())
				bref[i] = it->second;
		}
	}

	std::vector<intptr_t> acnt(asize + 1, 0);

	for (intptr_t i = 1; i <= asize; i++)
	{
		if (!acnt[i])
		{
			for (intptr_t j = i; (j = anext[j]);)
				acnt[j] = -i;
		}
	}

	intptr_t alo {0}, ahi {0}, blo {0}, bhi {0}; // bounds of current region
	intptr_t malo {0}, mahi {0}, mblo {0}, mbhi {0}; // bounds of best matching region

	std::vector<intptr_t> rstack;

	push_quad(rstack, 1, asize + 1, 1, bsize + 1);

	while (!rstack.empty())
	{
		pop_quad(rstack, &alo, &ahi, &blo, &bhi);

		// adjust acnt[x] for x in A range (alo <= x < ahi)
		for (intptr_t i = blo; i < bhi; i++)
		{
			if (bref[i])
				acnt[bref[i]] = 0;
		}

		for (intptr_t i = alo; i < ahi; i++)
			getcnt_ref(i, acnt) = 0;

		for (intptr_t i = alo; i < ahi; i++)
			getcnt_ref(i, acnt)++;

		if (find_best_matching_region(acnt, anext, bref, alo, ahi, blo, bhi, &malo, &mahi, &mblo, &mbhi))
		{
			// m-region is a match (malo upto mahi matches mblo upto mbhi)
			// if region after includes at least 1 line, push
			if (mahi < ahi || mbhi < bhi)
				push_quad(rstack, mahi, ahi, mbhi, bhi);

			// if region before includes at least 1 line, push
			if (alo < malo || blo < mblo)
				push_quad(rstack, alo, malo, blo, mblo);
		}
		else
		{
			add_diff(diff, alo + off - 1, ahi + off - 1, blo + off - 1, bhi + off - 1);
		}
	}

	add_diff(diff, asize + off, asize + off, 0, 0);
}


template <typename Elem, typename UserDataT>
bool HistogramDiff<Elem, UserDataT>::find_best_matching_region(
	const std::vector<intptr_t>& acnt, const std::vector<intptr_t>& anext, const std::vector<intptr_t>& bref,
	intptr_t alo, intptr_t ahi, intptr_t blo, intptr_t bhi,
	intptr_t* malo, intptr_t* mahi, intptr_t* mblo, intptr_t* mbhi)
{
	static constexpr intptr_t cLowcnt = 512; // jgit uses 65

	intptr_t j, lowcnt, rgnlowcnt, cnt, nextj, nexti, nalo, nahi, nblo, nbhi;
	*malo = 0;

	if (alo == ahi || blo == bhi)
		return false;

	lowcnt = cLowcnt;

	for (intptr_t i = blo; i < bhi; i = nexti)
	{
		nexti = i + 1;

		if (!(j = bref[i]))
			continue;

		if (j >= ahi || !acnt[j] || acnt[j] > lowcnt)
			continue;

		// try to find j inside A range
		while (j < alo && anext[j])
			j = anext[j];

		if (j < alo || j >= ahi)
			continue;

		for (;;)
		{
			nextj = anext[j];

			// alo <= j < ahi and a[j] matches b[i]
			// set current match, then expand it
			nalo = j;
			nahi = j + 1;
			nblo = i;
			nbhi = i + 1;

			rgnlowcnt = getcnt(nalo, acnt);

			while (alo < nalo && blo < nblo && beqa(nalo - 1, acnt, nblo - 1, bref))
			{
				nalo--;
				nblo--;
				cnt = getcnt(nalo, acnt);

				if (cnt < rgnlowcnt)
					rgnlowcnt = cnt;
			}

			while (nahi < ahi && nbhi < bhi && beqa(nahi, acnt, nbhi, bref))
			{
				cnt = getcnt(nahi, acnt);

				if (cnt < rgnlowcnt)
					rgnlowcnt = cnt;

				nahi++;
				nbhi++;
			}

			if (!*malo || nahi - nalo > *mahi - *malo || rgnlowcnt < lowcnt)
			{
				*malo = nalo;
				*mahi = nahi;
				*mblo = nblo;
				*mbhi = nbhi;
				lowcnt = rgnlowcnt;
			}

			if (nexti < nbhi)
				nexti = nbhi;

			while (nextj && nextj < nahi)
				nextj = anext[nextj];

			if (!nextj || nextj >= ahi)
				break;

			j = nextj;
		}
	}

	return bool(*malo);
}
