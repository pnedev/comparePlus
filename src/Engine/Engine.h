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

#include <windows.h>
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


struct DocCmpInfo
{
	HWND		view;
	section_t	section;
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

	inline bool operator!=(const Word& rhs) const
	{
		return (hash != rhs.hash);
	}

	inline bool operator==(unsigned int rhs) const
	{
		return (hash == rhs);
	}

	inline bool operator!=(unsigned int rhs) const
	{
		return (hash != rhs);
	}
};


// The returned bool is true if views are swapped and false otherwise
std::pair<std::vector<diff_info>, bool>
		compareDocs(DocCmpInfo& doc1, DocCmpInfo& doc2, const UserSettings& settings, progress_ptr& progress);
bool compareBlocks(const DocCmpInfo& doc1, const DocCmpInfo& doc2, const UserSettings& settings,
		diff_info& blockDiff1, diff_info& blockDiff2);
bool showDiffs(const DocCmpInfo& doc1, const DocCmpInfo& doc2, const UserSettings& settings,
		const std::pair<std::vector<diff_info>, bool>& cmpResults, progress_ptr& progress);
