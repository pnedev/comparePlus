/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
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
#include <utility>
#include "NppHelpers.h"
#include "diff.h"
#include "ProgressDlg.h"


enum class charType
{
	SPACECHAR,
	ALPHANUMCHAR,
	OTHERCHAR
};


struct Word
{
	charType type;

	int line;
	int pos;
	int length;

	unsigned int hash;

	inline bool operator==(const Word& rhs) const
	{
		return (hash == rhs.hash);
	}
};


// The returned bool is true if views are swapped and false otherwise
std::pair<std::vector<diff_edit>, bool>
		compareDocs(HWND& view1, HWND& view2, const UserSettings& settings, progress_ptr& progress);
bool compareBlocks(HWND view1, HWND view2, const UserSettings& settings, diff_edit& blockDiff1, diff_edit& blockDiff2);
bool showDiffs(HWND view1, HWND view2, const std::pair<std::vector<diff_edit>, bool>& cmpResults,
		progress_ptr& progress);
