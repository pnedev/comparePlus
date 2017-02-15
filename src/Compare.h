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
#include <commdlg.h>

#include "Notepad_plus_msgs.h"
#include "Scintilla.h"
#include "menuCmdID.h"
#include "PluginInterface.h"

#include "resource.h"


// Those are interpreted as bool values
#define DEFAULT_OLD_IS_FIRST		1
#define DEFAULT_OLD_ON_LEFT			1
#define DEFAULT_COMPARE_TO_PREV		1

#define DEFAULT_ENCODINGS_CHECK		1
#define DEFAULT_WRAP_AROUND			0
#define DEFAULT_RECOMPARE_ON_SAVE	1
#define DEFAULT_GOTO_FIRST_DIFF		0
#define DEFAULT_UPDATE_ON_CHANGE	0
#define DEFAULT_COMPACT_NAVBAR		1

#define DEFAULT_ADDED_COLOR			0xD9FFD9
#define DEFAULT_DELETED_COLOR		0xD7D7FF
#define DEFAULT_CHANGED_COLOR		0x98E7E7
#define DEFAULT_MOVED_COLOR			0xFFE6CC
#define DEFAULT_HIGHLIGHT_COLOR		0x683FF
#define DEFAULT_HIGHLIGHT_ALPHA		100

#define _MIN(a, b)	((a) < (b) ? (a) : (b))
#define _MAX(a, b)	((a) > (b) ? (a) : (b))


enum MENU_COMMANDS
{
	CMD_SET_FIRST = 0,
	CMD_COMPARE,
	CMD_COMPARE_LINES,
	CMD_CLEAR_ACTIVE,
	CMD_CLEAR_ALL,
	CMD_SEPARATOR_1,
	CMD_LAST_SAVE_DIFF,
	CMD_SVN_DIFF,
	CMD_GIT_DIFF,
	CMD_SEPARATOR_2,
	CMD_IGNORE_SPACES,
	CMD_IGNORE_CASE,
	CMD_DETECT_MOVES,
	CMD_NAV_BAR,
	CMD_SEPARATOR_3,
	CMD_PREV,
	CMD_NEXT,
	CMD_FIRST,
	CMD_LAST,
	CMD_SEPARATOR_4,
	CMD_SETTINGS,
	CMD_ABOUT,
	NB_MENU_COMMANDS
};


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

	static const TCHAR encodingsCheckSetting[];
	static const TCHAR wrapAroundSetting[];
	static const TCHAR reCompareOnSaveSetting[];
	static const TCHAR gotoFirstDiffSetting[];
	static const TCHAR updateOnChangeSetting[];
	static const TCHAR compactNavBarSetting[];

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

	bool           	EncodingsCheck;
	bool           	WrapAround;
	bool           	RecompareOnSave;
	bool           	GotoFirstDiff;
	bool           	UpdateOnChange;
	bool           	CompactNavBar;

	bool           	IgnoreSpaces;
	bool           	IgnoreCase;
	bool           	DetectMoves;
	bool           	UseNavBar;

	ColorSettings	colors;

private:
	bool dirty {false};
};


extern NppData nppData;


void ViewNavigationBar();
