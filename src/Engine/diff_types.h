/* Common diff types used with DiffCalc
 * Copyright (C) 2025-2026  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>


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


// Elements range [s, e) - s included, e excluded
// Very rudimentary structure used for ease, clarity and speed - not meant for generic usage as it relies on external
// precautions for data integrity
struct range_t
{
	range_t() noexcept : s(0), e(0) {};
	range_t(const range_t& rhs) noexcept : s(rhs.s), e(rhs.e) {};
	range_t(intptr_t start, intptr_t end) noexcept : s(start), e(end)
	{
		if (e < s)
			e = s;
	};

	intptr_t len() const noexcept { return e - s; };

	intptr_t distance_from(const range_t& rhs) const noexcept { return s - rhs.e; };
	intptr_t distance_from(intptr_t el) const noexcept { return s - el; };
	intptr_t distance_to(const range_t& rhs) const noexcept { return rhs.s - e; };
	intptr_t distance_to(intptr_t el) const noexcept { return el - e; };

	bool contains(intptr_t el) const noexcept { return (el >= s && el < e); };

	void shift(intptr_t off) noexcept
	{
		s += off;
		e += off;
	};

	bool glue(const range_t& rhs) noexcept
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


struct diff_info
{
	diff_info(intptr_t as, intptr_t ae, intptr_t bs, intptr_t be) noexcept : a(as, ae), b(bs, be) {};
	diff_info(const diff_info& rhs) noexcept : a(rhs.a), b(rhs.b) {};

	range_t	a;
	range_t	b;

	bool is_replacement() const noexcept { return (a.len() && b.len()); };

	void shift(intptr_t off) noexcept
	{
		a.shift(off);
		b.shift(off);
	};

	bool glue(const diff_info& rhs) noexcept
	{
		const bool united_a = a.glue(rhs.a);
		const bool united_b = b.glue(rhs.b);

		return united_a || united_b;
	};
};


// It is merely a std::vector with some helper functions
struct diff_results : public std::vector<diff_info>
{
	intptr_t count_replaces() const noexcept
	{
		intptr_t replaces = 0;

		for (const auto& d : *this)
			replaces += (d.a.len() < d.b.len() ? d.a.len() : d.b.len());

		return replaces;
	};

	void add(intptr_t as, intptr_t ae, intptr_t bs, intptr_t be)
	{
		this->emplace_back(as, ae, bs, be);
	};

	void swap_ab() noexcept
	{
		for (auto& d : *this)
			std::swap(d.a, d.b);
	};

	void append(diff_results&& diffs, intptr_t aoff, intptr_t boff)
	{
		if (diffs.empty())
			return;

		for (auto& d : diffs)
		{
			d.a.shift(aoff);
			d.b.shift(boff);
		}

		auto dItr = diffs.begin();

		// Unite border diffs
		if (this->size() && this->back().glue(*dItr))
			dItr++;

		this->insert(this->end(), dItr, diffs.end());
	};

	intptr_t get_diff_before_a(intptr_t aidx)
	{
		if (this->empty() || (*this)[0].a.e > aidx)
			return -1;

		intptr_t diff_idx = 0;

		for (intptr_t i = static_cast<intptr_t>(this->size()) / 2; i; i /= 2)
		{
			if ((*this)[diff_idx + i].a.e <= aidx)
				diff_idx += i;
		}

		while ((diff_idx < static_cast<intptr_t>(this->size())) && ((*this)[diff_idx].a.e <= aidx))
			++diff_idx;

		return --diff_idx;
	};

	intptr_t get_diff_before_b(intptr_t bidx)
	{
		if (this->empty() || (*this)[0].b.e > bidx)
			return -1;

		intptr_t diff_idx = 0;

		for (intptr_t i = static_cast<intptr_t>(this->size()) / 2; i; i /= 2)
		{
			if ((*this)[diff_idx + i].b.e <= bidx)
				diff_idx += i;
		}

		while ((diff_idx < static_cast<intptr_t>(this->size())) && ((*this)[diff_idx].b.e <= bidx))
			++diff_idx;

		return --diff_idx;
	};

	intptr_t get_diff_after_a(intptr_t aidx)
	{
		if (this->empty() || this->back().a.s < aidx)
			return -1;

		intptr_t diff_idx = 0;

		for (intptr_t i = static_cast<intptr_t>(this->size()) / 2; i; i /= 2)
		{
			if ((*this)[diff_idx + i].a.s < aidx)
				diff_idx += i;
		}

		while ((diff_idx < static_cast<intptr_t>(this->size())) && ((*this)[diff_idx].a.s < aidx))
			++diff_idx;

		return diff_idx;
	};

	intptr_t get_diff_after_b(intptr_t bidx)
	{
		if (this->empty() || this->back().b.s < bidx)
			return -1;

		intptr_t diff_idx = 0;

		for (intptr_t i = static_cast<intptr_t>(this->size()) / 2; i; i /= 2)
		{
			if ((*this)[diff_idx + i].b.s < bidx)
				diff_idx += i;
		}

		while ((diff_idx < static_cast<intptr_t>(this->size())) && ((*this)[diff_idx].b.s < bidx))
			++diff_idx;

		return diff_idx;
	};
};


using ThrowIfCancelledFn = std::function<void()>;


/**
 *  \class  diff_algorithm
 *  \brief  Base class that defines the common interface to any diff algorithm.
			Compares and makes a differences list between two sequences (elements are template,
			must have operator==, type HashType and get_hash() function that returns their unique HashType value.
			Best is for Elem to inherit from hash_type<T>)
 */
template <typename Elem>
class diff_algorithm
{
public:
	// cancelCheck() is function that should be periodically called (if provided) by the specific algorithm
	// at certain points to check if the user has cancelled the compare operation. It shall throw exception on cancel
	// that shall be handled in upper layers
	diff_algorithm(ThrowIfCancelledFn cancelCheck = nullptr) : _cancelCheck(cancelCheck) {};

	virtual ~diff_algorithm() {};

	// Runs the actual compare and fills the differences in diff param.
	// The diff algorithm assumes the sequences begin with a diff so provide here the offset to the first difference.
	virtual void run(const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize,
			diff_results& diffs, intptr_t off = 0) = 0;

	// Provides information if the specific algorithm's results can benefit from certain diffs post-processing
	virtual bool needSwapCheck() { return true; };
	virtual bool needDiffsCombine() { return true; };
	virtual bool needBoundaryShift() { return true; };

protected:
	void ThrowIfCancelled() { if (_cancelCheck) _cancelCheck(); };

private:
	ThrowIfCancelledFn _cancelCheck;
};
