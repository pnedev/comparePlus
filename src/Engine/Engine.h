/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sébastien Leroy (jean.sebastien.leroy@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>
#include <string>
#include <limits.h>
#include "NppHelpers.h"


// Rotate a value n bits to the left
#define UINT_BIT (sizeof (unsigned) * CHAR_BIT)
#define ROL(v, n) ((v) << (n) | (v) >> (UINT_BIT - (n)))

// Given a hash value and a new character, return a new hash value
#define HASH(h, c) ((c) + ROL(h, 7))


// Forward declarations
struct diff_edit;


enum wordType
{
	SPACECHAR,
	EOLCHAR,
	ALPHANUMCHAR,
	OTHERCHAR
};


struct Word
{
	unsigned int line;
	unsigned int pos;
	unsigned int length;
	wordType type;
	std::string text;
	unsigned int hash;

	inline bool operator==(const Word& rhs) const
	{
		return (hash == rhs.hash);
	}
};


bool compareWords(diff_edit&, diff_edit&, const DocLines_t& doc1, const DocLines_t& doc2, bool IgnoreSpaces);
int setDiffLines(const diff_edit&, std::vector<diff_edit>&, int* idx, short op, int altLocation);

void findMoves(std::vector<diff_edit>&, const unsigned int *hash1, const unsigned int *hash2);
void shiftBoundries(std::vector<diff_edit>&, const unsigned int *hash1, const unsigned int *hash2,
		int doc1Length, int doc2Length);

std::vector<unsigned int> computeHashes(const DocLines_t& doc, bool IgnoreSpaces);
