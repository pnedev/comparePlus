/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2018 Pavel Nedev (pg.nedev@gmail.com)
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
	section_t(int o, int l) : off(o), len(l) {}

	int off;
	int len;
};


struct CompareOptions
{
	int		oldFileViewId;

	bool	findUniqueMode;

	bool	charPrecision;
	bool	ignoreSpaces;
	bool	ignoreCase;
	bool	detectMoves;

	int		matchPercentThreshold;

	bool	selectionCompare;

	std::pair<int, int>	selections[2];
};


struct AlignmentViewData
{
	int	line {0};
	int	diffMask {0};
};


struct AlignmentPair
{
	AlignmentViewData main;
	AlignmentViewData sub;
};


using AlignmentInfo_t = std::vector<AlignmentPair>;


CompareResult compareViews(const CompareOptions& options, const TCHAR* progressInfo, AlignmentInfo_t& alignmentInfo);
