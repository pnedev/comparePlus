/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2017-2019 Pavel Nedev (pg.nedev@gmail.com)
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
#define DEFAULT_FIRST_IS_NEW			1
#define DEFAULT_NEW_IN_SUB_VIEW			1
#define DEFAULT_COMPARE_TO_PREV			1

#define DEFAULT_ENCODINGS_CHECK			1
#define DEFAULT_ALIGN_ALL_MATCHES		0
#define DEFAULT_NEVER_MARK_IGNORED		0
#define DEFAULT_FOLLOWING_CARET			1
#define DEFAULT_WRAP_AROUND				0
#define DEFAULT_GOTO_FIRST_DIFF			1
#define DEFAULT_PROMPT_CLOSE_ON_MATCH	0

#define DEFAULT_STATUS_TYPE				0

#define DEFAULT_ADDED_COLOR				0xC6FFC6
#define DEFAULT_REMOVED_COLOR			0xC6C6FF
#define DEFAULT_CHANGED_COLOR			0x98E7E7
#define DEFAULT_MOVED_COLOR				0xFFE6CC
#define DEFAULT_HIGHLIGHT_COLOR			0x683FF
#define DEFAULT_HIGHLIGHT_TRANSP		0
#define DEFAULT_CHANGED_THRESHOLD		30


enum StatusType
{
	COMPARE_SUMMARY = 0,
	COMPARE_OPTIONS,
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
	int transparency;
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

	void toggleStatusType()
	{
		statusType = static_cast<StatusType>(static_cast<int>(statusType) + 1);

		if (statusType == StatusType::STATUS_TYPE_END)
			statusType = StatusType::COMPARE_SUMMARY;
	}

	static const TCHAR mainSection[];

	static const TCHAR newFileViewSetting[];
	static const TCHAR firstIsNewSetting[];
	static const TCHAR compareToPrevSetting[];

	static const TCHAR encodingsCheckSetting[];
	static const TCHAR alignAllMatchesSetting[];
	static const TCHAR markIgnoredLinesSetting[];
	static const TCHAR followingCaretSetting[];
	static const TCHAR wrapAroundSetting[];
	static const TCHAR gotoFirstDiffSetting[];
	static const TCHAR promptCloseOnMatchSetting[];

	static const TCHAR charPrecisionSetting[];
	static const TCHAR diffsBasedChangesSetting[];
	static const TCHAR ignoreSpacesSetting[];
	static const TCHAR ignoreEmptyLinesSetting[];
	static const TCHAR ignoreCaseSetting[];
	static const TCHAR detectMovesSetting[];
	static const TCHAR ignoreLineNumbers[];

	static const TCHAR showOnlySelSetting[];
	static const TCHAR showOnlyDiffSetting[];
	static const TCHAR navBarSetting[];

	static const TCHAR reCompareOnChangeSetting[];

	static const TCHAR statusTypeSetting[];

	static const TCHAR colorsSection[];

	static const TCHAR addedColorSetting[];
	static const TCHAR removedColorSetting[];
	static const TCHAR changedColorSetting[];
	static const TCHAR movedColorSetting[];
	static const TCHAR addHighlightColorSetting[];
	static const TCHAR remHighlightColorSetting[];
	static const TCHAR highlightTranspSetting[];
	static const TCHAR changedThresholdSetting[];

	bool           	FirstFileIsNew;
	int				NewFileViewId;
	bool           	CompareToPrev;

	bool           	EncodingsCheck;
	bool           	AlignAllMatches;
	bool           	NeverMarkIgnored;
	bool           	FollowingCaret;
	bool           	WrapAround;
	bool           	GotoFirstDiff;
	bool           	PromptToCloseOnMatch;

	bool           	CharPrecision;
	bool           	DiffsBasedLineChanges;
	bool           	IgnoreSpaces;
	bool           	IgnoreEmptyLines;
	bool           	IgnoreCase;
	bool           	DetectMoves;
	bool           	IgnoreLineNumbers;

	bool           	ShowOnlyDiffs;
	bool           	ShowOnlySelections;
	bool           	UseNavBar;

	bool           	RecompareOnChange;

	StatusType		SavedStatusType;
	StatusType		statusType;

	ColorSettings	colors;

	int				ChangedThresholdPercent;

private:
	bool dirty {false};
};
