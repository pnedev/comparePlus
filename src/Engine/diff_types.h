/* Common diff types used with DiffCalc
 * Copyright (C) 2025-2026  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

// Should be defined before including windows.h so this header needs to be included above any windows.h inclusion.
// That cannot be guaranteed but add the define here anyway for completeness
#define NOMINMAX	1


#include <cstdint>
#include <vector>
#include <string>
#include <functional>


// Elements range [s, e) - s included, e excluded
// Very rudimentary structure used for ease, clarity and speed - not meant for generic usage as it relies on external
// precautions for data integrity
struct range_t
{
	range_t() : s(0), e(0) {};
	range_t(intptr_t start, intptr_t end) : s(start), e(end)
	{
		if (e < s)
			e = s;
	};

	intptr_t len() const { return e - s; };

	intptr_t distance_from(const range_t& rhs) const { return s - (rhs.len() ? rhs.e + 1 : rhs.e); };
	intptr_t distance_from(intptr_t el) const { return s - el; };
	intptr_t distance_to(const range_t& rhs) const { return rhs.s - (len() ? e + 1 : e); };
	intptr_t distance_to(intptr_t el) const { return el - (len() ? e + 1 : e); };

	bool contains(intptr_t el) const { return (el >= s && el < e); };

	void shift(intptr_t off)
	{
		s += off;
		e += off;
	};

	bool glue(const range_t& rhs)
	{
		if (e == rhs.s)
		{
			e = rhs.e;
			return true;
		}

		return false;
	};

	std::string to_string() const
	{
		std::string str {"["};
		str += std::to_string(s);
		str += ", ";
		str += std::to_string(e);
		str += ')';

		return str;
	};

	intptr_t s;
	intptr_t e;
};


template <typename T>
struct hash_type
{
	using HashType = T;

	hash_type(HashType h) : hash(h) {};

	HashType hash;

	HashType get_hash() const { return hash; };

	bool operator==(const hash_type& rhs) const { return (hash == rhs.hash); };
	bool operator!=(const hash_type& rhs) const { return (hash != rhs.hash); };
	bool operator==(HashType rhs) const { return (hash == rhs); };
	bool operator!=(HashType rhs) const { return (hash != rhs); };
};


// UserData is not used by the diff algorithm - it is provided as a data placeholder for further user processings
template <typename UserData>
struct diff_info : public UserData
{
	diff_info(intptr_t as, intptr_t ae, intptr_t bs, intptr_t be) : a(as, ae), b(bs, be) {};

	range_t	a;
	range_t	b;

	bool is_replacement() const { return (a.len() && b.len()); };

	void shift(intptr_t off)
	{
		a.shift(off);
		b.shift(off);
	};

	bool glue(const diff_info<UserData>& rhs)
	{
		const bool united_a = a.glue(rhs.a);
		const bool united_b = b.glue(rhs.b);

		return united_a || united_b;
	};
};


template <>
struct diff_info<void>
{
	diff_info(intptr_t as, intptr_t ae, intptr_t bs, intptr_t be) : a(as, ae), b(bs, be) {};

	range_t	a;
	range_t	b;

	bool is_replacement() const { return (a.len() && b.len()); };

	void shift(intptr_t off)
	{
		a.shift(off);
		b.shift(off);
	};

	bool glue(const diff_info<void>& rhs)
	{
		const bool united_a = a.glue(rhs.a);
		const bool united_b = b.glue(rhs.b);

		return united_a || united_b;
	};
};


// It is merely a std::vector with some helper functions
template <typename UserData = void>
struct diff_results : public std::vector<diff_info<UserData>>
{
	intptr_t count_replaces() const
	{
		intptr_t replaces = 0;

		for (const auto& d : *this)
			replaces += std::min(d.a.len(), d.b.len());

		return replaces;
	};

	void add(intptr_t as, intptr_t ae, intptr_t bs, intptr_t be)
	{
		this->emplace_back(as, ae, bs, be);
	};

	void swap_ab()
	{
		for (auto& d : *this)
			std::swap(d.a, d.b);
	};

	void append(diff_results<UserData>&& diff, intptr_t aoff, intptr_t boff)
	{
		if (diff.empty())
			return;

		for (auto& d : diff)
		{
			d.a.shift(aoff);
			d.b.shift(boff);
		}

		auto dItr = diff.begin();

		// Unite border diffs
		if (this->size() && this->back().glue(*dItr))
			dItr++;

		this->insert(this->end(), dItr, diff.end());
	};
};


using IsCancelledFn = std::function<bool()>;


/**
 *  \class  diff_algorithm
 *  \brief  Base class that defines the common interface to any diff algorithm.
			Compares and makes a differences list between two sequences (elements are template,
			must have operator==, type HashType and get_hash() function that returns unique HashType value.
			Best is for Elem to inherit from hash_type<T>)
 */
template <typename Elem, typename UserData = void>
class diff_algorithm
{
public:
	// cancelledFn() is function that should be periodically called (if provided) by the specific algorithm
	// at certain points to check if the user has cancelled the compare operation
	diff_algorithm(IsCancelledFn cancelledFn = nullptr) : _cancelledFn(cancelledFn) {};

	virtual ~diff_algorithm() {};

	// Runs the actual compare and fills the differences in diff member.
	// The diff algorithm assumes the sequences begin with a diff so provide here the offset to the first difference.
	virtual void run(const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize,
			diff_results<UserData>& diff, intptr_t off = 0) = 0;

	// Provides information if the specific algorithm's results can benefit from certain diffs post-processing
	virtual bool needSwapCheck() { return true; };
	virtual bool needDiffsCombine() { return true; };
	virtual bool needBoundaryShift() { return true; };

protected:
	bool isCancelled() { return (_cancelledFn && _cancelledFn()); };

private:
	IsCancelledFn _cancelledFn;
};
