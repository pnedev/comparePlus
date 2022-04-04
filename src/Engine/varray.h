/* varray - a variable size array implemented with std::vector */

#pragma once

#include <vector>


template <typename Elem>
struct varray
{
public:
	varray() {}
	~varray() {}

	// Be very careful when using the returned Elem reference! It may become invalid on consecutive calls to get()
	// because the vector memory might be reallocated!
	inline Elem& get(size_t i)
	{
		if (_buf.size() <= i)
			_buf.resize(i + 1, { 0 });

		return _buf[i];
	}

	inline std::vector<Elem>& get()
	{
		return _buf;
	}

private:
	std::vector<Elem> _buf;
};
