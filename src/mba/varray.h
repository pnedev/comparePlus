
#pragma once


/* varray - a variable size array implemented with std::vector */

#include <cstddef>
#include <vector>

template <typename Elem>
struct varray {
	varray() {}
	~varray() {}

	Elem* get(std::size_t i)
	{
		if (buf.size() <= i)
			buf.resize(i + 1, { 0 });

		return &buf[i];
	}

private:
	std::vector<Elem> buf;
};
