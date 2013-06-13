/*
This file is part of Explorer Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef COMPARE_H
#define COMPARE_H

#define WIN32_LEAN_AND_MEAN

#include <math.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <stdio.h>
#include <assert.h>
#include "diff.h"
#include "Engine.h"
#include "PluginInterface.h"
#include "Scintilla.h"
#include "Notepad_plus_rc.h"
#include "Notepad_plus_msgs.h"
#include <iostream>
#include <fstream>
#include "Resource.h"
#include "CompareResource.h"
#include "AboutDialog.h"
#include "OptionDialog.h"
#include "NavDialog.h"
#include <window.h>
#include <Commdlg.h>

#define CLEANUP 1

using namespace std;
/* store name for ini file */

#define DEFAULT_ADDED_COLOR     0xE0FFE0
#define DEFAULT_DELETED_COLOR   0xE0E0FF
#define DEFAULT_CHANGED_COLOR   0x98E7E7
#define DEFAULT_MOVED_COLOR     0xB1A88D
#define DEFAULT_BLANK_COLOR     0xe4e4e4
#define DEFAULT_HIGHLIGHT_COLOR 0x010101
#define DEFAULT_HIGHLIGHT_ALPHA 100
// dont use "INDIC_CONTAINER + 1" since it conflicts with DSpellCheck plugin
#define INDIC_HIGHLIGHT         INDIC_CONTAINER + 7

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
	CMD_COMPARE = 0,
	CMD_CLEAR_RESULTS,
    CMD_SEPARATOR_1,
	CMD_COMPARE_LAST_SAVE,
	CMD_COMPARE_SVN_BASE,
	CMD_COMPARE_GIT_BASE,
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
	CMD_OPTION,
	CMD_ABOUT,
	NB_MENU_COMMANDS
};

struct sColorSettings
{
    int added;
    int deleted;
    int changed;
    int moved;
    int blank;
    int highlight;
    int alpha;
};

struct sUserSettings
{
    bool           UseNavBar;
    bool           AddLine;
    bool           IncludeSpace;
    bool           DetectMove;
    bool           OldSymbols;
    sColorSettings ColorSettings; 
};

enum eEOL 
{
	EOF_WIN,
	EOF_LINUX,
	EOF_MAC
};

const CHAR strEOL[3][3] = 
{
	"\r\n",
	"\r",
	"\n"
};

const UINT lenEOL[3] = {2,1,1};

void compare();
void compareLocal();
void compareSvnBase();
void compareGitBase();
int getCompare(int window);
void removeCompare(int window);
int setCompare(int window);
void alignMatches();
void includeSpacing();
void detectMoves();
void compareWithoutLines();
void reset();
void openAboutDlg(void);
void openOptionDlg(void);
void openFile(TCHAR *file);
void openMemBlock(void *memblock, long size);
HWND openTempFile();
void addEmptyLines(HWND hSci, int offset, int length);
bool startCompare();
void saveSettings(void);
void loadSettings(void);
void ViewNavigationBar(void);

void Prev(void);
void Next(void);
void First(void);
void Last(void);

//BOOL CALLBACK AboutDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#if 0
/* Extended Window Funcions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
#endif

#endif //COMPARE_H

