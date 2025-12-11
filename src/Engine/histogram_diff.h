/* Histogram diff algorithm implemented as a C++ template class DiffAlgo
 * Copyright (C) 2025  Pavel Nedev <pg.nedev@gmail.com>
 * Original algorithm is taken from Ray Gardner's C implementation in 2025
 * License: 0BSD
 */


#pragma once

#include "diff_types.h"

#include <vector>
#include <unordered_map>


/**
 *  \class  DiffAlgo
 *  \brief  Compares and makes a differences list between two sequences (elements are template, must have operator==)
 */
template <typename Elem, typename UserDataT = void>
class DiffAlgo
{
public:
	DiffAlgo(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size, diff_results<UserDataT>& diff,
			IsCancelledFn isCancelled = nullptr);

	// Runs the actual compare and fills the differences in diff member.
	// The diff algorithm assumes the sequences begin with a diff so provide here the offset to the first difference.
	void operator()(intptr_t off);

	DiffAlgo(const DiffAlgo&) = delete;
	const DiffAlgo& operator=(const DiffAlgo&) = delete;

private:
	static constexpr int _cCancelCheckItrInterval {30000};

	void add_diff(intptr_t al, intptr_t ah, intptr_t bl, intptr_t bh);

	void push_quad(std::vector<intptr_t>& stk, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
	{
		stk.emplace_back(a);
		stk.emplace_back(b);
		stk.emplace_back(c);
		stk.emplace_back(d);
	};

	void pop_quad(std::vector<intptr_t>& stk, intptr_t* a, intptr_t* b, intptr_t* c, intptr_t* d)
	{
		if (stk.size() < 4)
		{
			stk.clear();
			return;
		}

		*d = stk.back();
		stk.pop_back();
		*c = stk.back();
		stk.pop_back();
		*b = stk.back();
		stk.pop_back();
		*a = stk.back();
		stk.pop_back();
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

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	diff_results<UserDataT>& _diff;

	intptr_t _last_a_pos;

	IsCancelledFn _isCancelled;
	intptr_t _cancelCheckCount;
};


template <typename Elem, typename UserDataT>
DiffAlgo<Elem, UserDataT>::DiffAlgo(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
		diff_results<UserDataT>& diff, IsCancelledFn isCancelled) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _diff(diff),
	_isCancelled(isCancelled), _cancelCheckCount(_cCancelCheckItrInterval)
{
}


template <typename Elem, typename UserDataT>
void DiffAlgo<Elem, UserDataT>::add_diff(intptr_t al, intptr_t ah, intptr_t bl, intptr_t bh)
{
	if (al > _last_a_pos)
		_diff._add(diff_type::DIFF_MATCH, _last_a_pos, al - _last_a_pos);

	if (ah > al)
		_diff._add(diff_type::DIFF_IN_1, al, ah - al);

	if (bh > bl)
		_diff._add(diff_type::DIFF_IN_2, bl, bh - bl);

	_last_a_pos = ah;
}


template <typename Elem, typename UserDataT>
void DiffAlgo<Elem, UserDataT>::operator()(intptr_t off)
{
	_last_a_pos = off;

	for (auto it = _diff.rbegin(); it != _diff.rend(); ++it)
	{
		if (it->type != diff_type::DIFF_IN_2)
		{
			_last_a_pos = it->off + it->len;
			break;
		}
	}

	const Elem* a = _a + off;
	const Elem* b = _b + off;

	std::vector<intptr_t> anext(_a_size + 1, 0);
	std::vector<intptr_t> bref(_b_size + 1, 0);
	std::vector<intptr_t> acnt(_a_size + 1, 0);
	{
		std::unordered_map<typename Elem::hash_type, intptr_t> amap;

		for (intptr_t i = _a_size; i; i--)
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

		for (intptr_t i = 1; i <= _b_size; i++)
		{
			auto it = amap.find(b[i - 1].get_hash());

			if (it != amap.end())
				bref[i] = it->second;
		}

		for (intptr_t i = 1; i <= _a_size; i++)
		{
			if (!acnt[i])
			{
				acnt[i] = 1;

				for (intptr_t j = i; (j = anext[j]); acnt[i]++)
					acnt[j] = -i;
			}
		}
	}

	intptr_t alo {0}, ahi {0}, blo {0}, bhi {0}; // bounds of current region
	intptr_t malo {0}, mahi {0}, mblo {0}, mbhi {0}; // bounds of best matching region

	std::vector<intptr_t> rstack;

	push_quad(rstack, 1, _a_size + 1, 1, _b_size + 1);

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
		{
			if (acnt[i] < 0)
				acnt[-acnt[i]] = 0;
			else
				acnt[i] = 0;
		}

		for (intptr_t i = alo; i < ahi; i++)
		{
			if (acnt[i] < 0)
				acnt[-acnt[i]]++;
			else if (!acnt[i])
				acnt[i]++;
			else
				return;
		}

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
			add_diff(alo + off - 1, ahi + off - 1, blo + off - 1, bhi + off - 1);
		}
	}
}


template <typename Elem, typename UserDataT>
bool DiffAlgo<Elem, UserDataT>::find_best_matching_region(
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
