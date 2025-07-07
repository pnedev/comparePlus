/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
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
#include <string>


// Those are interpreted as bool values
#define DEFAULT_FIRST_IS_NEW				1
#define DEFAULT_NEW_IN_SUB_VIEW				1
#define DEFAULT_COMPARE_TO_PREV				1

#define DEFAULT_ENCODINGS_CHECK				1
#define DEFAULT_SIZES_CHECK					1
#define DEFAULT_NEVER_MARK_IGNORED			0
#define DEFAULT_FOLLOWING_CARET				1
#define DEFAULT_WRAP_AROUND					0
#define DEFAULT_GOTO_FIRST_DIFF				1
#define DEFAULT_PROMPT_CLOSE_ON_MATCH		0

#define DEFAULT_STATUS_INFO					0

#define DEFAULT_ADDED_COLOR					0xC6FFC6
#define DEFAULT_REMOVED_COLOR				0xC6C6FF
#define DEFAULT_MOVED_COLOR					0xFFE6CC
#define DEFAULT_CHANGED_COLOR				0x98E7E7
#define DEFAULT_HIGHLIGHT_COLOR				0x0683FF
#define DEFAULT_HIGHLIGHT_MOVED_COLOR		0xF58742
#define DEFAULT_HIGHLIGHT_TRANSP			0
#define DEFAULT_CARET_LINE_TRANSP			60

#define DEFAULT_ADDED_COLOR_DARK			0x055A05
#define DEFAULT_REMOVED_COLOR_DARK			0x16164F
#define DEFAULT_MOVED_COLOR_DARK			0x4F361C
#define DEFAULT_CHANGED_COLOR_DARK			0x145050
#define DEFAULT_HIGHLIGHT_COLOR_DARK		0x0683FF
#define DEFAULT_HIGHLIGHT_MOVED_COLOR_DARK	0xF58742
#define DEFAULT_HIGHLIGHT_TRANSP_DARK		0
#define DEFAULT_CARET_LINE_TRANSP_DARK		80

#define DEFAULT_CHANGED_THRESHOLD			30

#define DEFAULT_ENABLE_TOOLBAR_TB			1
#define DEFAULT_SET_AS_FIRST_TB				1
#define DEFAULT_COMPARE_TB					1
#define DEFAULT_COMPARE_SEL_TB				1
#define DEFAULT_CLEAR_COMPARE_TB			1
#define DEFAULT_NAVIGATION_TB				1
#define DEFAULT_DIFFS_FILTER_TB				1
#define DEFAULT_NAV_BAR_TB					1


enum StatusType
{
	DIFFS_SUMMARY = 0,
	COMPARE_OPTIONS,
	STATUS_DISABLED,
	STATUS_TYPE_END
};


struct ColorSettings
{
	int added;
	int removed;
	int changed;
	int moved;
	int blank;
    int _default;
	int add_highlight;
	int rem_highlight;
	int mov_highlight;
	int highlight_transparency;
	int caret_line_transparency;
};


struct UserSettings
{
public:
	UserSettings() : _colors(&colorsLight) {}

	void load();
	void save();

	inline void markAsDirty()
	{
		dirty = true;
	}

	inline void useLightColors()
	{
		_colors = &colorsLight;
	}

	inline void useDarkColors()
	{
		_colors = &colorsDark;
	}

	inline ColorSettings& colors()
	{
		return *_colors;
	}

	static const TCHAR mainSection[];

	static const TCHAR newFileViewSetting[];
	static const TCHAR firstIsNewSetting[];
	static const TCHAR compareToPrevSetting[];

	static const TCHAR encodingsCheckSetting[];
	static const TCHAR sizesCheckSetting[];
	static const TCHAR markIgnoredLinesSetting[];
	static const TCHAR followingCaretSetting[];
	static const TCHAR wrapAroundSetting[];
	static const TCHAR gotoFirstDiffSetting[];
	static const TCHAR promptCloseOnMatchSetting[];

	static const TCHAR detectMovesSetting[];
	static const TCHAR detectSubBlockDiffsSetting[];
	static const TCHAR detectSubLineMovesSetting[];
	static const TCHAR detectCharDiffsSetting[];
	static const TCHAR ignoreEmptyLinesSetting[];
	static const TCHAR ignoreFoldedLinesSetting[];
	static const TCHAR ignoreHiddenLinesSetting[];
	static const TCHAR ignoreChangedSpacesSetting[];
	static const TCHAR ignoreAllSpacesSetting[];
	static const TCHAR ignoreEOLSetting[];
	static const TCHAR ignoreCaseSetting[];
	static const TCHAR ignoreRegexSetting[];
	static const TCHAR invertRegexSetting[];
	static const TCHAR inclRegexNomatchLinesSetting[];
	static const TCHAR ignoreRegexStrSetting[];

	static const TCHAR hideMatchesSetting[];
	static const TCHAR hideNewLinesSetting[];
	static const TCHAR hideChangedLinesSetting[];
	static const TCHAR hideMovedLinesSetting[];
	static const TCHAR showOnlySelSetting[];
	static const TCHAR navBarSetting[];

	static const TCHAR reCompareOnChangeSetting[];

	static const TCHAR statusInfoSetting[];

	static const TCHAR colorsSection[];

	static const TCHAR addedColorSetting[];
	static const TCHAR removedColorSetting[];
	static const TCHAR movedColorSetting[];
	static const TCHAR changedColorSetting[];
	static const TCHAR addHighlightColorSetting[];
	static const TCHAR remHighlightColorSetting[];
	static const TCHAR movHighlightColorSetting[];
	static const TCHAR highlightTranspSetting[];
	static const TCHAR caretLineTranspSetting[];

	static const TCHAR addedColorDarkSetting[];
	static const TCHAR removedColorDarkSetting[];
	static const TCHAR movedColorDarkSetting[];
	static const TCHAR changedColorDarkSetting[];
	static const TCHAR addHighlightColorDarkSetting[];
	static const TCHAR remHighlightColorDarkSetting[];
	static const TCHAR movHighlightColorDarkSetting[];
	static const TCHAR highlightTranspDarkSetting[];
	static const TCHAR caretLineTranspDarkSetting[];

	static const TCHAR changedThresholdSetting[];

	static const TCHAR toolbarSection[];

	static const TCHAR enableToolbarSetting[];
	static const TCHAR setAsFirstTBSetting[];
	static const TCHAR compareTBSetting[];
	static const TCHAR compareSelTBSetting[];
	static const TCHAR clearCompareTBSetting[];
	static const TCHAR navigationTBSetting[];
	static const TCHAR diffsFilterTBSetting[];
	static const TCHAR navBarTBSetting[];

	bool			FirstFileIsNew;
	int				NewFileViewId;
	bool			CompareToPrev;

	bool			EncodingsCheck;
	bool			SizesCheck;
	bool			NeverMarkIgnored;
	bool			FollowingCaret;
	bool			WrapAround;
	bool			GotoFirstDiff;
	bool			PromptToCloseOnMatch;

	bool			DetectMoves;
	bool			DetectSubBlockDiffs;
	bool			DetectSubLineMoves;
	bool			DetectCharDiffs;
	bool			IgnoreEmptyLines;
	bool			IgnoreFoldedLines;
	bool			IgnoreHiddenLines;
	bool			IgnoreChangedSpaces;
	bool			IgnoreAllSpaces;
	bool			IgnoreEOL;
	bool			IgnoreCase;
	bool			IgnoreRegex;
	bool			InvertRegex;
	bool			InclRegexNomatchLines;
	std::wstring	IgnoreRegexStr;

	bool			HideMatches;
	bool			HideNewLines;
	bool			HideChangedLines;
	bool			HideMovedLines;
	bool			ShowOnlySelections;
	bool			UseNavBar;

	bool			RecompareOnChange;
	StatusType		StatusInfo;

	int				ChangedThresholdPercent;

	bool			EnableToolbar;
	bool			SetAsFirstTB;
	bool			CompareTB;
	bool			CompareSelTB;
	bool			ClearCompareTB;
	bool			NavigationTB;
	bool			DiffsFilterTB;
	bool			NavBarTB;

private:
	bool dirty {false};

	ColorSettings	colorsLight;
	ColorSettings	colorsDark;

	ColorSettings*	_colors;
};
