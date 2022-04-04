/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2022 Pavel Nedev (pg.nedev@gmail.com)
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

#include <windows.h>
#include <cstdint>
#include <vector>
#include <utility>

#include "Compare.h"
#include "NppHelpers.h"


enum class CompareResult
{
	COMPARE_ERROR,
	COMPARE_CANCELLED,
	COMPARE_MATCH,
	COMPARE_MISMATCH
};


struct section_t
{
	section_t() : off(0), len(0) {}
	section_t(intptr_t o, intptr_t l) : off(o), len(l) {}

	intptr_t off;
	intptr_t len;
};


struct CompareOptions
{
	CompareOptions()
	{
		selections[0] = std::make_pair(-1, -1);
		selections[1] = std::make_pair(-1, -1);
	}

	int		newFileViewId;

	bool	findUniqueMode;

	bool	alignAllMatches;
	bool	neverMarkIgnored;
	bool	detectMoves;
	bool	detectCharDiffs;
	bool	bestSeqChangedLines;
	bool	ignoreSpaces;
	bool	ignoreEmptyLines;
	bool	ignoreCase;

	int		changedThresholdPercent;

	bool	selectionCompare;

	std::pair<intptr_t, intptr_t>	selections[2];
};


struct AlignmentViewData
{
	intptr_t	line {0};
	int			diffMask {0};
};


struct AlignmentPair
{
	AlignmentViewData main;
	AlignmentViewData sub;
};


using AlignmentInfo_t = std::vector<AlignmentPair>;


struct CompareSummary
{
	inline void clear()
	{
		diffLines	= 0;
		added		= 0;
		removed		= 0;
		moved		= 0;
		changed		= 0;
		match		= 0;

		alignmentInfo.clear();
	}

	intptr_t	diffLines;
	intptr_t	added;
	intptr_t	removed;
	intptr_t	moved;
	intptr_t	changed;
	intptr_t	match;

	AlignmentInfo_t	alignmentInfo;
};


CompareResult compareViews(const CompareOptions& options, const TCHAR* progressInfo, CompareSummary& summary);
