
#pragma once


/* varray - a variable size array implemented with std::vector */

#include <cstddef>
#include <vector>

template <typename Elem>
struct varray {
	varray() {}
	~varray() {}

	// Be very careful when using the returned Elem pointer! It may become invalid on consecutive calls to get()
	// because the vector memory might be reallocated!
	inline Elem* get(std::size_t i)
	{
		if (buf.size() <= i)
			buf.resize(i + 1, { 0 });

		return &buf[i];
	}

private:
	std::vector<Elem> buf;
};
