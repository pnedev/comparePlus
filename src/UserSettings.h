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


// Those are interpreted as bool values
#define DEFAULT_OLD_IS_FIRST			1
#define DEFAULT_OLD_IN_SUB_VIEW			0
#define DEFAULT_COMPARE_TO_PREV			1

#define DEFAULT_CHAR_PRECISION			1
#define DEFAULT_ENCODINGS_CHECK			1
#define DEFAULT_FOLLOWING_CARET			1
#define DEFAULT_WRAP_AROUND				0
#define DEFAULT_GOTO_FIRST_DIFF			1
#define DEFAULT_PROMPT_CLOSE_ON_MATCH	0

#define DEFAULT_ADDED_COLOR				0xC6FFC6
#define DEFAULT_DELETED_COLOR			0xC6C6FF
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
	static const TCHAR oldFileViewSetting[];
	static const TCHAR compareToPrevSetting[];

	static const TCHAR charPrecisionSetting[];
	static const TCHAR encodingsCheckSetting[];
	static const TCHAR followingCaretSetting[];
	static const TCHAR wrapAroundSetting[];
	static const TCHAR gotoFirstDiffSetting[];
	static const TCHAR promptCloseOnMatchSetting[];

	static const TCHAR ignoreSpacesSetting[];
	static const TCHAR ignoreCaseSetting[];
	static const TCHAR detectMovesSetting[];

	static const TCHAR hideMatchesSetting[];
	static const TCHAR navBarSetting[];

	static const TCHAR reCompareOnChangeSetting[];

	static const TCHAR colorsSection[];

	static const TCHAR addedColorSetting[];
	static const TCHAR removedColorSetting[];
	static const TCHAR changedColorSetting[];
	static const TCHAR movedColorSetting[];
	static const TCHAR highlightColorSetting[];
	static const TCHAR highlightAlphaSetting[];

	static const int MatchPercentThreshold = 35;

	bool           	OldFileIsFirst;
	int				OldFileViewId;
	bool           	CompareToPrev;

	bool           	CharPrecision;
	bool           	EncodingsCheck;
	bool           	FollowingCaret;
	bool           	WrapAround;
	bool           	GotoFirstDiff;
	bool           	PromptToCloseOnMatch;

	bool           	IgnoreSpaces;
	bool           	IgnoreCase;
	bool           	DetectMoves;

	bool           	HideMatches;
	bool           	UseNavBar;

	bool           	RecompareOnChange;

	ColorSettings	colors;

private:
	bool dirty {false};
};
