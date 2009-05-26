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
#include "msgno.h"
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
#include <window.h>
#include <Commdlg.h>

#define CLEANUP 1

using namespace std;
/* store name for ini file */

#define DEFAULT_ADDED_COLOR     0xe0e0ff
#define DEFAULT_DELETED_COLOR   0xfff0e0
#define DEFAULT_CHANGED_COLOR   0xe0ffe0
#define DEFAULT_MOVED_COLOR     0xc8c8c8
#define DEFAULT_BLANK_COLOR     0xe4e4e4

struct blankLineList
{
	int line;
	int length;
	struct blankLineList *next;
};

struct chunk_info
{
	int *linePos;
	int *lineEndPos;
	int lineCount;
	int lineStart;
	struct varray *changes;
	struct varray *words;
	int changeCount;
	char *text;
	int count;
	//int *mappings;
	int *lineMappings;
};

struct sColorSettings
{
    int added;
    int deleted;
    int changed;
    int moved;
    int blank;
};

struct sUserSettings
{
    bool           AddLine;
    bool           IncludeSpace;
    bool           DetectMove;
    sColorSettings ColorSettings; 
};

enum eEOL 
{
	EOF_WIN,
	EOF_LINUX,
	EOF_MAC
};
enum wordType 
{
	SPACECHAR,ALPHANUMCHAR,OTHERCHAR
};

struct Word
{
	int line;
	int pos;
	int length;
	wordType type;
	string text;
	unsigned int hash;
};

const CHAR strEOL[3][3] = 
{
	"\r\n",
	"\r",
	"\n"
};

const UINT lenEOL[3] = {2,1,1};

void compare();
void compareWithoutLines();
void reset();
void openAboutDlg(void);
void openOptionDlg(void);
void openFile(TCHAR *file);
HWND openTempFile();
void addEmptyLines(HWND hSci, int offset, int length);
bool startCompare();
void saveSettings(void);
void loadSettings(void);

//BOOL CALLBACK AboutDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#if 0
/* Extended Window Funcions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
#endif

#endif //COMPARE_H

