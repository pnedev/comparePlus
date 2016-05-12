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


#define DEFAULT_ADDED_COLOR     	0xAAFFAA
#define DEFAULT_DELETED_COLOR   	0xC4C4FF
#define DEFAULT_CHANGED_COLOR   	0x82E1E1
#define DEFAULT_MOVED_COLOR     	0xCBBF89
#define DEFAULT_HIGHLIGHT_COLOR 	0x010101
#define DEFAULT_HIGHLIGHT_ALPHA		100

#define _MAX(a, b)	((a) > (b) ? (a) : (b))


enum MARKER_ID
{
	MARKER_BLANK_LINE = 0,
	MARKER_MOVED_LINE,
	MARKER_CHANGED_LINE,
	MARKER_ADDED_LINE,
	MARKER_REMOVED_LINE,
	MARKER_CHANGED_SYMBOL,
	MARKER_ADDED_SYMBOL,
	MARKER_REMOVED_SYMBOL,
	MARKER_MOVED_SYMBOL
};


enum MENU_COMMANDS
{
	CMD_SET_FIRST = 0,
	CMD_COMPARE,
	CMD_CLEAR_ACTIVE,
	CMD_CLEAR_ALL,
	CMD_SEPARATOR_1,
	CMD_LAST_SAVE_DIFF,
	CMD_SVN_DIFF,
	CMD_GIT_DIFF,
	CMD_SEPARATOR_2,
	CMD_ALIGN_MATCHES,
	CMD_IGNORE_SPACING,
	CMD_DETECT_MOVES,
	CMD_USE_NAV_BAR,
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
	void load();
	void save();

	static const TCHAR mainSection[];
	static const TCHAR oldIsFirstSetting[];
	static const TCHAR oldFileOnLeftSetting[];
	static const TCHAR gotoFirstDiffSetting[];
	static const TCHAR alignMatchesSetting[];
	static const TCHAR ignoreSpacesSetting[];
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
	bool           	GotoFirstDiff;

	bool           	AddLine;
	bool           	IncludeSpace;
	bool           	DetectMove;
	bool           	UseNavBar;

	ColorSettings	colors;
};


extern NppData nppData;


void ViewNavigationBar();
