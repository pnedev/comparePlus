/*
 * This file is part of Compare Plugin for Notepad++
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <windows.h>
#include <tchar.h>

#include "Compare.h"


// Those are interpreted as bool values
#define DEFAULT_OLD_IS_FIRST			1
#define DEFAULT_OLD_ON_LEFT				1
#define DEFAULT_COMPARE_TO_PREV			1

#define DEFAULT_DETECT_MOVE_LINE_MODE	0

#define DEFAULT_ENCODINGS_CHECK			1
#define DEFAULT_PROMPT_CLOSE_ON_MATCH	0
#define DEFAULT_ALIGN_REPLACEMENTS		1
#define DEFAULT_WRAP_AROUND				0
#define DEFAULT_RECOMPARE_ON_SAVE		1
#define DEFAULT_GOTO_FIRST_DIFF			0
#define DEFAULT_UPDATE_ON_CHANGE		0

#define DEFAULT_ADDED_COLOR				0xD9FFD9
#define DEFAULT_DELETED_COLOR			0xD7D7FF
#define DEFAULT_CHANGED_COLOR			0x98E7E7
#define DEFAULT_MOVED_COLOR				0xFFE6CC
#define DEFAULT_HIGHLIGHT_COLOR			0x683FF
#define DEFAULT_HIGHLIGHT_ALPHA			100


struct ColorSettings
{
	int added;
	int deleted;
	int changed;
	int moved;
	int blank;
    int _default;
	int highlight;
	int alpha;
};


struct UserSettings
{
public:
	void load();
	void save();

	inline void markAsDirty()
	{
		dirty = true;
	}

	static const TCHAR mainSection[];

	static const TCHAR oldIsFirstSetting[];
	static const TCHAR oldFileOnLeftSetting[];
	static const TCHAR compareToPrevSetting[];
	static const TCHAR detectMovesLineModeSetting[];
	static const TCHAR encodingsCheckSetting[];
	static const TCHAR promptCloseOnMatchSetting[];
	static const TCHAR alignReplacementsSetting[];
	static const TCHAR wrapAroundSetting[];
	static const TCHAR reCompareOnSaveSetting[];
	static const TCHAR gotoFirstDiffSetting[];
	static const TCHAR updateOnChangeSetting[];

	static const TCHAR ignoreSpacesSetting[];
	static const TCHAR ignoreCaseSetting[];
	static const TCHAR detectMovesSetting[];
	static const TCHAR navBarSetting[];

	static const TCHAR colorsSection[];

	static const TCHAR addedColorSetting[];
	static const TCHAR removedColorSetting[];
	static const TCHAR changedColorSetting[];
	static const TCHAR movedColorSetting[];
	static const TCHAR highlightColorSetting[];
	static const TCHAR highlightAlphaSetting[];

	bool           	OldFileIsFirst;
	int				OldFileViewId;
	bool           	CompareToPrev;
	bool           	DetectMovesLineMode;
	bool           	EncodingsCheck;
	bool           	PromptToCloseOnMatch;
	bool           	AlignReplacements;
	bool           	WrapAround;
	bool           	RecompareOnSave;
	bool           	GotoFirstDiff;
	bool           	UpdateOnChange;

	bool           	IgnoreSpaces;
	bool           	IgnoreCase;
	bool           	DetectMoves;
	bool           	UseNavBar;

	ColorSettings	colors;

private:
	bool dirty {false};
};
