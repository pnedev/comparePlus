/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sébastien Leroy (jean.sebastien.leroy@gmail.com)
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

#include <stdlib.h>
#include <exception>
#include <vector>

#include "Compare.h"
#include "NPPHelpers.h"
#include "ScmHelper.h"
#include "CProgress.h"
#include "AboutDialog.h"
#include "OptionDialog.h"
#include "NavDialog.h"
#include "diff.h"
#include "Engine.h"


#define MAXCOMPARE 2


static const TCHAR PLUGIN_NAME[]			= TEXT("Compare");

static const TCHAR sectionName[]			= TEXT("Compare Settings");
static const TCHAR addLinesOption[]			= TEXT("Align Matches");
static const TCHAR ignoreSpacesOption[]		= TEXT("Include Spaces");
static const TCHAR detectMovesOption[]		= TEXT("Detect Move Blocks");

static const TCHAR colorsSection[]        	= TEXT("Colors");
static const TCHAR addedColorOption[]     	= TEXT("Added");
static const TCHAR removedColorOption[]   	= TEXT("Removed");
static const TCHAR changedColorOption[]   	= TEXT("Changed");
static const TCHAR movedColorOption[]     	= TEXT("Moved");
static const TCHAR highlightColorOption[]	= TEXT("Highlight");
static const TCHAR highlightAlphaOption[] 	= TEXT("Alpha");
static const TCHAR NavBarOption[]         	= TEXT("Navigation bar");


static int compareDocs[MAXCOMPARE];
static TCHAR emptyLinesDoc[MAX_PATH];

static int  tempWindow = -1;
static bool active = false;
static bool skipAutoReset = false;
static int closingWin = -1;
static HWND closingView = NULL;
static blankLineList* lastEmptyLines = NULL;
static int  topLine = 0;
static long start_line_old = -1;
static long visible_line_count_old = -1;
static bool panelsOpened = false;

static bool syncScrollVwasChecked = false;
static bool syncScrollHwasChecked = false;

static CProgress* progDlg = NULL;
static int progMax = 0;
static int progCounter = 0;

static TCHAR compareFilePath[MAX_PATH];

static AboutDialog   AboutDlg;
static OptionDialog  OptionDlg;
static NavDialog     NavDlg;

static toolbarIcons  tbPrev;
static toolbarIcons  tbNext;
static toolbarIcons  tbFirst;
static toolbarIcons  tbLast;

static FuncItem funcItem[NB_MENU_COMMANDS] = { 0 };


HINSTANCE hInstance;

NppData nppData;

sUserSettings Settings;


// Declare functions that appear before they are defined
static void ClearCompare();
static void First();
static void openMemBlock(const char* memblock, long size);
static bool startCompare();


bool progressUpdate(int mid)
{
	if (!progDlg)
		return false;

	if (progDlg->IsCancelled())
		return false;

	if (mid > progMax)
		progMax = mid;

	if (progMax)
	{
		int perc = (++progCounter * 100) / (progMax * 4);
		progDlg->SetPercent(perc);
	}

	return true;
}


static void progressClose()
{
	if (progDlg)
	{
		EnableWindow(nppData._nppHandle, TRUE);

		progDlg->Close();
		delete progDlg;
		progDlg = NULL;
	}
}


static void loadSettings(void)
{
	TCHAR iniFilePath[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFilePath), (LPARAM)iniFilePath);

	_tcscpy_s(compareFilePath, _countof(compareFilePath), iniFilePath);

	PathAppend(iniFilePath, TEXT("Compare.ini"));

	// Try loading previous color settings
	int colors = GetPrivateProfileInt(colorsSection, addedColorOption, -1, iniFilePath);

	// If there is no previous color settings, load default value
	if(colors == -1)
	{
		Settings.ColorSettings.added     = ::GetPrivateProfileInt(colorsSection, addedColorOption, DEFAULT_ADDED_COLOR, iniFilePath);
		Settings.ColorSettings.deleted   = ::GetPrivateProfileInt(colorsSection, removedColorOption, DEFAULT_DELETED_COLOR, iniFilePath);
		Settings.ColorSettings.changed   = ::GetPrivateProfileInt(colorsSection, changedColorOption, DEFAULT_CHANGED_COLOR, iniFilePath);
		Settings.ColorSettings.moved     = ::GetPrivateProfileInt(colorsSection, movedColorOption, DEFAULT_MOVED_COLOR, iniFilePath);
		Settings.ColorSettings.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorOption, DEFAULT_HIGHLIGHT_COLOR, iniFilePath);
		Settings.ColorSettings.alpha     = ::GetPrivateProfileInt(colorsSection, highlightAlphaOption, DEFAULT_HIGHLIGHT_ALPHA, iniFilePath);
	}
	// Else load stored color settings
	else
	{
		Settings.ColorSettings.added     = ::GetPrivateProfileInt(colorsSection, addedColorOption, DEFAULT_ADDED_COLOR, iniFilePath);
		Settings.ColorSettings.deleted   = ::GetPrivateProfileInt(colorsSection, removedColorOption, DEFAULT_DELETED_COLOR, iniFilePath);
		Settings.ColorSettings.changed   = ::GetPrivateProfileInt(colorsSection, changedColorOption, DEFAULT_CHANGED_COLOR, iniFilePath);
		Settings.ColorSettings.moved     = ::GetPrivateProfileInt(colorsSection, movedColorOption, DEFAULT_MOVED_COLOR, iniFilePath);
		Settings.ColorSettings.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorOption, DEFAULT_HIGHLIGHT_COLOR, iniFilePath);
		Settings.ColorSettings.alpha     = ::GetPrivateProfileInt(colorsSection, highlightAlphaOption, DEFAULT_HIGHLIGHT_ALPHA, iniFilePath);
	}

	// Try loading behavior settings, else load default value
	Settings.AddLine      = ::GetPrivateProfileInt(sectionName, addLinesOption, 1, iniFilePath) == 1;
	Settings.IncludeSpace = ::GetPrivateProfileInt(sectionName, ignoreSpacesOption, 0, iniFilePath) == 1;
	Settings.DetectMove   = ::GetPrivateProfileInt(sectionName, detectMovesOption, 1, iniFilePath) == 1;
	Settings.UseNavBar    = ::GetPrivateProfileInt(sectionName, NavBarOption, 1, iniFilePath) == 1;
}


static void saveSettings(void)
{
	TCHAR iniFilePath[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFilePath), (LPARAM)iniFilePath);
	PathAppend(iniFilePath, TEXT("Compare.ini"));

	TCHAR buffer[64];

	_itot_s(Settings.ColorSettings.added, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.deleted, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, removedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.changed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.moved, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.alpha, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightAlphaOption, buffer, iniFilePath);

	::WritePrivateProfileString(sectionName, addLinesOption, Settings.AddLine ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, ignoreSpacesOption, Settings.IncludeSpace ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, detectMovesOption, Settings.DetectMove ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, NavBarOption, Settings.UseNavBar ? TEXT("1") : TEXT("0"), iniFilePath);
}


static int getCompare(int window)
{
	for(int i = 0; i < MAXCOMPARE; i++)
		if(compareDocs[i] == window)
			return i;

	return -1;
}


static void removeCompare(int window)
{
	const int val = getCompare(window);

	if (val != -1)
		compareDocs[val] = -1;
}


static int setCompare(int window)
{
	const int val = getCompare(window);

	if(val != -1)
		return val;

	for(int i = 0; i < MAXCOMPARE; i++)
	{
		if(compareDocs[i] == -1)
		{
			compareDocs[i] = window;
			return i;
		}
	}

	return -1;
}


static void jumpChangedLines(bool direction)
{
	HWND CurView = getCurrentWindow();

	const int sci_search_mask = (1 << MARKER_MOVED_LINE) |
								(1 << MARKER_CHANGED_LINE) |
								(1 << MARKER_ADDED_LINE) |
								(1 << MARKER_REMOVED_LINE) |
								(1 << MARKER_BLANK_LINE);

	const int posStart = ::SendMessage(CurView, SCI_GETCURRENTPOS, 0, 0);
	const int lineMax = ::SendMessage(CurView, SCI_GETLINECOUNT, 0, 0);
	int lineStart = ::SendMessage(CurView, SCI_LINEFROMPOSITION, posStart, 0);
	int prevLine = lineStart;

	int currLine;
	int nextLine;
	int sci_marker_direction;

	while (true)
	{
		if (direction)
		{
			currLine = (lineStart < lineMax) ? (lineStart + 1) : (0);
			sci_marker_direction = SCI_MARKERNEXT;
		}
		else
		{
			currLine = (lineStart > 0) ? (lineStart - 1) : (lineMax);
			sci_marker_direction = SCI_MARKERPREVIOUS;
		}

		nextLine = ::SendMessage(CurView, sci_marker_direction, currLine, sci_search_mask);

		if (nextLine < 0)
		{
			currLine = (direction) ? (0) : (lineMax);
			nextLine = ::SendMessage(CurView, sci_marker_direction, currLine, sci_search_mask);
			break;
		}

		if (nextLine != currLine)
			break;
		else if (direction)
			lineStart++;
		else
			lineStart--;
	}

	if ((direction && (nextLine < prevLine)) ||
		(!direction && (nextLine > prevLine)))
	{
		FLASHWINFO flashInfo;
		flashInfo.cbSize = sizeof(flashInfo);
		flashInfo.hwnd = nppData._nppHandle;
		flashInfo.uCount = 2;
		flashInfo.dwTimeout = 100;
		flashInfo.dwFlags = FLASHW_ALL;
		FlashWindowEx(&flashInfo);
	}

	::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
	::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
}


static HWND openTempFile()
{
	char original[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)original);
	HWND originalwindow = getCurrentWindow();

	LRESULT curBuffer = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	LangType curLang = (LangType)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE, curBuffer, 0);

	int result = ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
	HWND window = getCurrentWindow();
	int win = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);

	if (result == 0 || win != tempWindow)
	{
		::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_NEW, 0);
		::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)compareFilePath);
		tempWindow = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);

		curBuffer = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, curBuffer, curLang);
	}

	if (originalwindow == window)
	{
		::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)original);
		skipAutoReset = true;
		::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
		skipAutoReset = false;
		::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
		panelsOpened = true;
	}

	result=::SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE, 0, (LPARAM)original);

	window = getOtherWindow();

	int pointer = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);
	if (tempWindow != pointer)
	{
		window = getCurrentWindow();
		pointer = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);
	}

	// move focus to new document, or the other document will be marked as dirty
	::SendMessage(window, SCI_GRABFOCUS, 0, 0);
	::SendMessage(window, SCI_SETREADONLY, 0, 0);
	::SendMessage(window, SCI_CLEARALL, 0, 0);

	return window;
}


static void openFile(const TCHAR *file)
{
	if(file == NULL || PathFileExists(file) == FALSE)
	{
		::MessageBox(nppData._nppHandle, TEXT("No file to open."), TEXT("Compare Plugin"), MB_OK);
		return;
	}

	FILE* myfile;
	_tfopen_s(&myfile, file, _T("rb"));

	if (myfile)
	{
		fseek(myfile, 0, SEEK_END);
		long size = ftell(myfile);
		std::vector<char> memblock(size + 1, 0);

		fseek(myfile, 0, SEEK_SET);
		fread(memblock.data(), 1, size, myfile);
		fclose(myfile);

		openMemBlock(memblock.data(), size);
	}
}


static void openMemBlock(const char* memblock, long size)
{
	HWND window = openTempFile();

	::SendMessage(window, SCI_GRABFOCUS, 0, 0);
	::SendMessage(window, SCI_APPENDTEXT, size, (LPARAM)memblock);

	if (startCompare())
	{
		::SendMessage(window, SCI_GRABFOCUS, 0, 0);
		::SendMessage(window, SCI_SETSAVEPOINT, 1, 0);
		::SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, 0);
		::SendMessage(window, SCI_SETREADONLY, 1, 0);

		ClearCompare();
	}
	else
	{
		::SendMessage(window, SCI_GRABFOCUS, 0, 0);
		::SendMessage(window, SCI_SETSAVEPOINT, 1, 0);
		::SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, 0);
		::SendMessage(window, SCI_SETREADONLY, 1, 0);
		::SendMessage(nppData._scintillaSecondHandle, SCI_GRABFOCUS, 0, 1);
	}
}


static bool newCompare()
{
	clearWindow(nppData._scintillaMainHandle);
	clearWindow(nppData._scintillaSecondHandle);

	active = true;

	std::vector<int> lineNum1;
	const DocLines_t doc1 = getAllLines(nppData._scintillaMainHandle, lineNum1);
	int doc1Length = doc1.size();

	if (doc1Length == 1 && doc1[0][0] == 0)
	{
		::MessageBox(nppData._nppHandle, TEXT("File empty, nothing to compare."), TEXT("Compare Plugin"), MB_OK);
		return true;
	}

	std::vector<int> lineNum2;
	const DocLines_t doc2 = getAllLines(nppData._scintillaSecondHandle, lineNum2);
	int doc2Length = doc2.size();

	if (doc2Length == 1 && doc2[0][0] == 0)
	{
		::MessageBox(nppData._nppHandle, TEXT("File empty, nothing to compare."), TEXT("Compare Plugin"), MB_OK);
		return true;
	}

	std::vector<unsigned int> doc1Hashes = computeHashes(doc1, Settings.IncludeSpace);
	std::vector<unsigned int> doc2Hashes = computeHashes(doc2, Settings.IncludeSpace);

	// Save current N++ focus
	HWND hwnd = GetFocus();

	TCHAR filenameMain[MAX_PATH];
	TCHAR filenameSecond[MAX_PATH];

	SetFocus(nppData._scintillaMainHandle);
	::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)filenameMain);
	SetFocus(nppData._scintillaSecondHandle);
	::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)filenameSecond);

	// Restore N++ focus
	SetFocus(hwnd);

	TCHAR buffer[1024];

	if (_tcslen(filenameMain) > 28)
		_tcscpy_s(filenameMain + 25, 4, TEXT("..."));

	if (_tcslen(filenameSecond) > 28)
		_tcscpy_s(filenameSecond + 25, 4, TEXT("..."));

	_sntprintf_s(buffer, _countof(buffer), _TRUNCATE, TEXT("Comparing \"%s\" vs. \"%s\"..."),
			filenameMain, filenameSecond);
	progMax = 0;
	progCounter = 0;
	progDlg = new CProgress();
	progDlg->Open(nppData._nppHandle, TEXT("Compare Plugin"));
	progDlg->SetInfo(buffer);

	EnableWindow(nppData._nppHandle, FALSE);

	std::vector<diff_edit> diff = DiffCalc<unsigned int>(doc1Hashes, doc2Hashes)();

	int	doc1Changed = 0;
	int	doc2Changed = 0;

	if (!diff.empty())
	{
		shiftBoundries(diff, doc1Hashes.data(), doc2Hashes.data(), doc1Length, doc2Length);

		if (Settings.DetectMove)
			findMoves(diff, doc1Hashes.data(), doc2Hashes.data());

		// Insert empty lines, count changed lines
		std::size_t diffSize = diff.size();
		for (unsigned int i = 0; i < diffSize; i++)
		{
			diff_edit& e1 = diff[i];

			if (e1.op == DIFF_DELETE)
			{
				e1.changeCount = 0;
				doc1Changed += e1.len;

				diff_edit& e2 = diff[i + 1];

				e2.changeCount = 0;

				if (e2.op == DIFF_INSERT)
				{
					// check if the DELETE/INSERT COMBO includes changed lines or it's a completely new block
					if (compareWords(e1, e2, doc1, doc2, Settings.IncludeSpace))
					{
						e1.op = DIFF_CHANGE1;
						e2.op = DIFF_CHANGE2;
						doc2Changed += e2.len;
					}
				}
			}
			else if (e1.op == DIFF_INSERT)
			{
				e1.changeCount = 0;
				doc2Changed += e1.len;
			}
		}
	}

	std::vector<diff_edit> doc1Changes(doc1Changed);
	std::vector<diff_edit> doc2Changes(doc2Changed);

	int doc1Offset = 0;
	int doc2Offset = 0;

	// Switch from blocks of lines to one change per line.
	// Change CHANGE to DELETE or INSERT if there are no changes on that line
	int added;

	if (!progDlg->IsCancelled())
	{
		int doc1Idx = 0;
		int doc2Idx = 0;

		std::size_t diffSize = diff.size();
		for (unsigned int i = 0; i < diffSize; i++)
		{
			diff_edit& e = diff[i];
			e.set = i;

			switch (e.op)
			{
				case DIFF_CHANGE1:
				case DIFF_DELETE:
					added = setDiffLines(e, doc1Changes, &doc1Idx, DIFF_DELETE, e.off + doc2Offset);
					doc2Offset -= added;
					doc1Offset += added;
				break;

				case DIFF_INSERT:
				case DIFF_CHANGE2:
					added = setDiffLines(e, doc2Changes, &doc2Idx, DIFF_INSERT, e.off + doc1Offset);
					doc1Offset -= added;
					doc2Offset += added;
				break;
			}
		}
	}

	bool different = true;

	if (!diff.empty() && !progDlg->IsCancelled())
	{
		int textIndex;
		different = (doc1Changed > 0) || (doc2Changed > 0);

		for (int i = 0; i < doc1Changed; ++i)
		{
			switch (doc1Changes[i].op)
			{
				case DIFF_DELETE:
					markAsRemoved(nppData._scintillaMainHandle, doc1Changes[i].off);
				break;

				case DIFF_CHANGE1:
					markAsChanged(nppData._scintillaMainHandle, doc1Changes[i].off);
					textIndex = lineNum1[doc1Changes[i].off];

					for (unsigned int k = 0; k < doc1Changes[i].changeCount; ++k)
					{
						diff_change& change = doc1Changes[i].changes->get(k);
						markTextAsChanged(nppData._scintillaMainHandle, textIndex + change.off, change.len);
					}
				break;

				case DIFF_MOVE:
					markAsMoved(nppData._scintillaMainHandle, doc1Changes[i].off);
				break;
			}
		}

		for (int i = 0; i < doc2Changed; ++i)
		{
			switch (doc2Changes[i].op)
			{
				case DIFF_INSERT:
					markAsAdded(nppData._scintillaSecondHandle, doc2Changes[i].off);
				break;

				case DIFF_CHANGE2:
					markAsChanged(nppData._scintillaSecondHandle, doc2Changes[i].off);
					textIndex = lineNum2[doc2Changes[i].off];

					for (unsigned int k = 0; k < doc2Changes[i].changeCount; ++k)
					{
						diff_change& change = doc2Changes[i].changes->get(k);
						markTextAsChanged(nppData._scintillaSecondHandle, textIndex + change.off, change.len);
					}
				break;

				case DIFF_MOVE:
					markAsMoved(nppData._scintillaSecondHandle, doc2Changes[i].off);
				break;
			}
		}

		doc1Offset = 0;
		doc2Offset = 0;

		if (Settings.AddLine)
		{
			int length = 0;
			int off = -1;
			for (int i = 0; i < doc1Changed; ++i)
			{
				switch (doc1Changes[i].op)
				{
					case DIFF_DELETE:
					case DIFF_MOVE:
						if (doc1Changes[i].altLocation == off)
						{
							length++;
						}
						else
						{
							addEmptyLines(nppData._scintillaSecondHandle, off + doc2Offset, length);
							doc2Offset += length;
							off = doc1Changes[i].altLocation;
							length = 1;
						}
					break;
				}
			}

			addEmptyLines(nppData._scintillaSecondHandle, off + doc2Offset, length);

			if (doc2Offset > 0)
			{
				clearUndoBuffer(nppData._scintillaSecondHandle);
			}

			length = 0;
			off = 0;
			doc1Offset = 0;

			for (int i = 0; i < doc2Changed; i++)
			{
				switch (doc2Changes[i].op)
				{
					case DIFF_INSERT:
					case DIFF_MOVE:
						if (doc2Changes[i].altLocation == off)
						{
							++length;
						}
						else
						{
							addEmptyLines(nppData._scintillaMainHandle, off + doc1Offset, length);
							doc1Offset += length;
							off = doc2Changes[i].altLocation;
							length = 1;
						}
					break;
				}
			}

			addEmptyLines(nppData._scintillaMainHandle, off + doc1Offset, length);

			if (doc1Offset > 0)
			{
				clearUndoBuffer(nppData._scintillaMainHandle);
			}
		}
	}

	bool compareCanceled = progDlg->IsCancelled();
	progressClose();

	if (compareCanceled)
	{
		return true;
	}

	if (!diff.empty())
	{
		if (!different)
		{
			_sntprintf_s(buffer, _countof(buffer), _TRUNCATE,
					TEXT("The files \"%s\" and \"%s\" match.\n\nClose compared files?\n"),
					filenameMain, filenameSecond);
			if (::MessageBox(nppData._nppHandle, buffer, TEXT("Compare Plugin: Files Match"),
					MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_CLOSE, 0);
				::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_SWITCHTO_OTHER_VIEW, 0);
				::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_CLOSE, 0);
				::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_SWITCHTO_OTHER_VIEW, 0);
			}
			return true;
		}

		::SendMessage(nppData._scintillaMainHandle, SCI_SHOWLINES, 0, 1);
		::SendMessage(nppData._scintillaSecondHandle, SCI_SHOWLINES, 0, 1);

		First();
	}

	return false;
}


static void activateBufferID(LRESULT bufferID, int view)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, bufferID, view);
	index = index & 0x3FFFFFFF;

	::SendMessage(nppData._nppHandle, NPPM_ACTIVATEDOC, view, index);
}


static bool startCompare()
{
	if (!IsWindowVisible(nppData._scintillaMainHandle) || !IsWindowVisible(nppData._scintillaSecondHandle))
	{
		skipAutoReset = true;

		// Yaron - In One-View mode, the current view can be 0 or 1.
		// In Two-Views mode, the top (or left) view is ALWAYS 0, and the bottom view is ALWAYS 1.
		int currentView = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0);
		LRESULT bufferID = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);

		// Yaron - If the current view is 0, we want the prev file to be moved to the bottom;
		// if the current view is 1, we still need to activate it and make sure it's compared to the current file.
		::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_TAB_PREV, 0);

		if (currentView == 1)
		{
			bufferID = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
			::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_TAB_NEXT, 0); // Yaron - Switch back to current file.
		}

		// Yaron - Current file is ALWAYS at top, and prev file is ALWAYS at bottom.
		::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);

		// Yaron - If the current view is 0, activate current file at top;
		// if the current view is 1, activate prev file at bottom (possibly multiple files there).
		activateBufferID(bufferID, currentView);

		// Yaron - Activate current file at top.
		if (currentView == 1)
			::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_SWITCHTO_OTHER_VIEW, 0);

		skipAutoReset = false;
		panelsOpened = true;
	}

	if (!IsWindowVisible(nppData._scintillaMainHandle) || !IsWindowVisible(nppData._scintillaSecondHandle))
	{
		panelsOpened = false;
		::MessageBox(nppData._nppHandle, TEXT("No files to compare."), TEXT("Compare Plugin"), MB_OK);
		return true;
	}

	if (SendMessage(nppData._scintillaMainHandle, SCI_GETCODEPAGE, 0, 0) !=
		SendMessage(nppData._scintillaSecondHandle, SCI_GETCODEPAGE, 0, 0))
	{
		if (::MessageBox(nppData._nppHandle,
			TEXT("Trying to compare files with different encoding - \nthe result might be inaccurate and misleading.")
			TEXT("\n\nCompare anyway?"), TEXT("Compare Plugin"), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
		{
			// TODO: Return back to single view
			panelsOpened = false;
			return true;
		}
	}

	LRESULT RODoc1 = ::SendMessage(nppData._scintillaMainHandle, SCI_GETREADONLY, 0, 0);
	// Remove read-only attribute
	if (RODoc1)
		::SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, false, 0);

	LRESULT RODoc2 = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETREADONLY, 0, 0);
	// Remove read-only attribute
	if (RODoc2)
		::SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, false, 0);

	setStyles(Settings);

	int doc1 = ::SendMessage(nppData._scintillaMainHandle, SCI_GETDOCPOINTER, 0, 0);
	int doc2 = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETDOCPOINTER, 0, 0);

	setCompare(doc1);
	setCompare(doc2);

	::SendMessage(nppData._scintillaMainHandle, SCI_GOTOPOS, 0, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_GOTOPOS, 0, 0);
	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);

	/* sync pannels */
	HMENU hMenu = ::GetMenu(nppData._nppHandle);

	syncScrollVwasChecked = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
	syncScrollHwasChecked = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;

	// Disable N++ vertical scroll - we handle it manually because of the Word Wrap
	if (syncScrollVwasChecked)
		::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);

	// Yaron - Enable N++ horizontal scroll sync
	if (!syncScrollHwasChecked)
		::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);

	// let the second view inherit the zoom level of the main view
	int mainZoomLevel = ::SendMessage(nppData._scintillaMainHandle, SCI_GETZOOM, 0, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETZOOM, mainZoomLevel, 0);

	// Compare files (return false if files differ)
	bool result = false;

	try
	{
		result = newCompare();
	}
	catch (std::exception& e)
	{
		progressClose();
		char msg[128];
		_snprintf_s(msg, _countof(msg), _TRUNCATE, "Exception occurred: %s", e.what());
		MessageBoxA(nppData._nppHandle, msg, "Compare Plugin", MB_OK | MB_ICONWARNING);
		ClearCompare();
	}
	catch (...)
	{
		progressClose();
		MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "Compare Plugin", MB_OK | MB_ICONWARNING);
		ClearCompare();
	}

	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);

	// Restore previous read-only attribute
	if (RODoc1)
		::SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, true, 0);

	if (RODoc2)
		::SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, true, 0);

	if (!result)
	{
		// Enable Prev/Next menu entry
		::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID,  MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, MF_BYCOMMAND | MF_ENABLED);

		if (Settings.UseNavBar)
		{
			start_line_old = -1;
			visible_line_count_old = -1;

			// Save current N++ focus
			HWND hwnd = GetFocus();

			// Display NavBar
			NavDlg.SetColor(Settings.ColorSettings);
			NavDlg.doDialog(true);

			// Restore N++ focus
			SetFocus(hwnd);
		}
	}

	return result;
}


static void Compare()
{
	if (active)
		return;

	bool filesMatch = startCompare();

	if (filesMatch)
		ClearCompare();
}


// Exit current compare
// - Clear results
// - Delete objects
// - Restore previous N++ appearance (markers, highlight, ...)
static void ClearCompare()
{
	if (active)
	{
		active = false;

		LRESULT RODoc1;
		LRESULT RODoc2;
		int doc1 = 0;
		int doc2 = 0;
		int doc1Index = -1;
		int doc2Index = -1;

		// Close NavBar
		NavDlg.doDialog(false);

		// Restore sync scroll buttons state
		if (syncScrollVwasChecked)
			::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
		if (!syncScrollHwasChecked)
			::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);

		// Disable Prev/Next menu entry
		HMENU hMenu = ::GetMenu(nppData._nppHandle);
		::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

		if (closingView != nppData._scintillaMainHandle)
		{
			// Remove read-only attribute
			if ((RODoc1 = ::SendMessage(nppData._scintillaMainHandle, SCI_GETREADONLY, 0, 0)) == 1)
				::SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, false, 0);
			doc1 = ::SendMessage(nppData._scintillaMainHandle, SCI_GETDOCPOINTER, 0, 0);
			doc1Index = getCompare(doc1);
			if (doc1Index != -1)
				clearWindow(nppData._scintillaMainHandle);
			// Remove margin mask
			::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINMASKN, (WPARAM)4, 0);
			// Remove margin
			::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, 0);
			// Restore previous read-only attribute
			if (RODoc1 == 1)
				::SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, true, 0);
		}
		removeCompare(doc1);

		if (closingView != nppData._scintillaSecondHandle)
		{
			if ((RODoc2 = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETREADONLY, 0, 0)) == 1)
				::SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, false, 0);
			doc2 = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETDOCPOINTER, 0, 0);
			doc2Index = getCompare(doc2);
			if (doc2Index != -1)
				clearWindow(nppData._scintillaSecondHandle);
			::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINMASKN, (WPARAM)4, 0);
			::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, 0);
			if (RODoc2 == 1)
				::SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, true, 0);
		}
		removeCompare(doc2);

		if (panelsOpened && (closingWin == -1))
		{
			::SendMessage(nppData._scintillaSecondHandle, SCI_GRABFOCUS, 0, 0);
			skipAutoReset = true;
			::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
			skipAutoReset = false;
		}
		panelsOpened = false;

		if (tempWindow != -1)
		{
			if (doc1 != closingWin)
			{
				::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
				HWND window = getCurrentWindow();
				int tempPointer = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);
				if (tempPointer == tempWindow)
				{
					::SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, 0);
					skipAutoReset = true;
					::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_CLOSE, 0);
					skipAutoReset = false;
				}
			}
			tempWindow = -1;
			LRESULT ROTemp = RODoc1;
			RODoc1 = RODoc2;
			RODoc2 = ROTemp;
		}

		::SendMessage(getCurrentWindow(), SCI_GRABFOCUS, 0, 0);

		closingWin = -1;
		closingView = NULL;
	}
}


static void CompareLocal()
{
	if (active)
		return;

	TCHAR file[MAX_PATH];
	::SendMessage(nppData._nppHandle,NPPM_GETCURRENTDIRECTORY,0,(LPARAM)file);

	if(file[0] != 0)
		::SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,0,(LPARAM)file);

	openFile(file);
}


static void CompareSvnBase()
{
	if (active)
		return;

	TCHAR curDir[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)curDir);

	if (curDir[0] != 0)
	{
		TCHAR curDirCanon[MAX_PATH];
		TCHAR svnDir[MAX_PATH];

		PathCanonicalize(curDirCanon, curDir);

		if (GetScmBaseFolder(TEXT(".svn"), curDirCanon, svnDir, _countof(svnDir)))
		{
			TCHAR curFile[MAX_PATH];
			TCHAR svnBaseFile[MAX_PATH];

			::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)curFile);

			if (curFile[0] != 0)
			{
				if (GetSvnBaseFile(curDirCanon, svnDir, curFile, svnBaseFile, _countof(svnBaseFile)))
				{
					openFile(svnBaseFile);
					return;
				}
			}
		}
	}
	MessageBox(nppData._nppHandle, TEXT("Can not locate SVN information."), TEXT("Compare Plugin"), MB_OK);
}


static void CompareGitBase()
{
	if (active)
		return;

	TCHAR curDir[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)curDir);

	if (curDir[0] != 0)
	{
		TCHAR curDirCanon[MAX_PATH];
		TCHAR gitDir[MAX_PATH];

		PathCanonicalize(curDirCanon, curDir);

		if (GetScmBaseFolder(TEXT(".git"), curDirCanon, gitDir, _countof(gitDir)))
		{
			TCHAR curFile[MAX_PATH];

			::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)curFile);

			if (curFile[0] != 0)
			{
				TCHAR gitBaseFile[MAX_PATH];

				GetLocalScmPath(curDir, gitDir, curFile, gitBaseFile);

				long size = 0;
				HGLOBAL hMem = GetContentFromGitRepo(gitDir, gitBaseFile, &size);

				if (size)
				{
					openMemBlock((const char*)hMem, size);
					GlobalFree(hMem);

					return;
				}
			}
		}
	}

	MessageBox(nppData._nppHandle, TEXT("Can not locate GIT information."), TEXT("Compare Plugin"), MB_OK);
}


static void AlignMatches()
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.AddLine = !Settings.AddLine;

	if (hMenu)
		CheckMenuItem(hMenu,
					funcItem[CMD_ALIGN_MATCHES]._cmdID,
					MF_BYCOMMAND | (Settings.AddLine ? MF_CHECKED : MF_UNCHECKED));
}


static void IncludeSpacing()
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.IncludeSpace = !Settings.IncludeSpace;

	if (hMenu)
		CheckMenuItem(hMenu,
					funcItem[CMD_IGNORE_SPACING]._cmdID,
					MF_BYCOMMAND | (Settings.IncludeSpace ? MF_CHECKED : MF_UNCHECKED));
}


static void DetectMoves()
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.DetectMove = !Settings.DetectMove;

	if (hMenu)
		CheckMenuItem(hMenu,
					funcItem[CMD_DETECT_MOVES]._cmdID,
					MF_BYCOMMAND | (Settings.DetectMove ? MF_CHECKED : MF_UNCHECKED));
}


static void ViewNavigationBar()
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.UseNavBar = !Settings.UseNavBar;

	if (hMenu)
		CheckMenuItem(hMenu,
					funcItem[CMD_USE_NAV_BAR]._cmdID,
					MF_BYCOMMAND | (Settings.UseNavBar ? MF_CHECKED : MF_UNCHECKED));

	if (active)
	{
		if (Settings.UseNavBar)
		{
			start_line_old = -1;
			visible_line_count_old = -1;

			// Save current N++ focus
			HWND hwnd = GetFocus();

			// Display NavBar
			NavDlg.SetColor(Settings.ColorSettings);
			NavDlg.doDialog(true);

			// Restore N++ focus
			SetFocus(hwnd);
		}
		else
		{
			NavDlg.doDialog(false);
		}
	}
}


static void Prev()
{
	if (active)
		jumpChangedLines(false);
}


static void Next()
{
	if (active)
		jumpChangedLines(true);
}


static void First()
{
	if (active)
	{
		HWND CurView = getCurrentWindow();
		HWND OtherView = getOtherWindow();

		const int sci_search_mask = (1 << MARKER_MOVED_LINE)
								  | (1 << MARKER_CHANGED_LINE)
								  | (1 << MARKER_ADDED_LINE)
								  | (1 << MARKER_REMOVED_LINE)
								  | (1 << MARKER_BLANK_LINE);

		const int nextLine = ::SendMessage(CurView, SCI_MARKERNEXT, 0, sci_search_mask );
		::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(OtherView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
		::SendMessage(OtherView, SCI_GOTOLINE, nextLine, 0);
	}
}


static void Last()
{
	if (active)
	{
		HWND CurView = getCurrentWindow();

		const int sci_search_mask = (1 << MARKER_MOVED_LINE)
								  | (1 << MARKER_CHANGED_LINE)
								  | (1 << MARKER_ADDED_LINE)
								  | (1 << MARKER_REMOVED_LINE)
								  | (1 << MARKER_BLANK_LINE);

		const int lineMax = ::SendMessage(CurView, SCI_GETLINECOUNT, 0, 0);
		const int nextLine = ::SendMessage(CurView, SCI_MARKERPREVIOUS, lineMax, sci_search_mask);
		::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
	}
}


static void OpenOptionDlg(void)
{
	if (OptionDlg.doDialog(&Settings) == IDOK)
	{
		saveSettings();

		if (active)
		{
			setStyles(Settings);

			if (NavDlg.isVisible())
			{
				NavDlg.SetColor(Settings.ColorSettings);
				NavDlg.CreateBitmap();
			}
		}
	}
}


static void OpenAboutDlg()
{
	AboutDlg.doDialog();
}


static void createMenu()
{
	_tcscpy_s(funcItem[CMD_COMPARE]._itemName, nbChar, TEXT("Compare"));
	funcItem[CMD_COMPARE]._pFunc			= Compare;
	funcItem[CMD_COMPARE]._pShKey			= new ShortcutKey;
	funcItem[CMD_COMPARE]._pShKey->_isAlt	= true;
	funcItem[CMD_COMPARE]._pShKey->_isCtrl	= false;
	funcItem[CMD_COMPARE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE]._pShKey->_key		= 'D';

	_tcscpy_s(funcItem[CMD_CLEAR_RESULTS]._itemName, nbChar, TEXT("Clear Results"));
	funcItem[CMD_CLEAR_RESULTS]._pFunc 				= ClearCompare;
	funcItem[CMD_CLEAR_RESULTS]._pShKey 			= new ShortcutKey;
	funcItem[CMD_CLEAR_RESULTS]._pShKey->_isAlt 	= true;
	funcItem[CMD_CLEAR_RESULTS]._pShKey->_isCtrl	= true;
	funcItem[CMD_CLEAR_RESULTS]._pShKey->_isShift	= false;
	funcItem[CMD_CLEAR_RESULTS]._pShKey->_key 		= 'D';

	_tcscpy_s(funcItem[CMD_COMPARE_LAST_SAVE]._itemName, nbChar, TEXT("Compare to last Save"));
	funcItem[CMD_COMPARE_LAST_SAVE]._pFunc 				= CompareLocal;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey 			= new ShortcutKey;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isAlt 	= true;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isCtrl 	= false;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_key 		= 'S';

	_tcscpy_s(funcItem[CMD_COMPARE_SVN_BASE]._itemName, nbChar, TEXT("Compare to SVN base"));
	funcItem[CMD_COMPARE_SVN_BASE]._pFunc 				= CompareSvnBase;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey 				= new ShortcutKey;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isAlt 		= true;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isCtrl 	= false;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_key 		= 'B';

	_tcscpy_s(funcItem[CMD_COMPARE_GIT_BASE]._itemName, nbChar, TEXT("Compare to GIT base"));
	funcItem[CMD_COMPARE_GIT_BASE]._pFunc 				= CompareGitBase;
	funcItem[CMD_COMPARE_GIT_BASE]._pShKey 				= new ShortcutKey;
	funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_isAlt 		= true;
	funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_isCtrl 	= true;
	funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_key 		= 'G';

	_tcscpy_s(funcItem[CMD_ALIGN_MATCHES]._itemName, nbChar, TEXT("Align Matches"));
	funcItem[CMD_ALIGN_MATCHES]._pFunc = AlignMatches;

	_tcscpy_s(funcItem[CMD_IGNORE_SPACING]._itemName, nbChar, TEXT("Ignore Spacing"));
	funcItem[CMD_IGNORE_SPACING]._pFunc = IncludeSpacing;

	_tcscpy_s(funcItem[CMD_DETECT_MOVES]._itemName, nbChar, TEXT("Detect Moves"));
	funcItem[CMD_DETECT_MOVES]._pFunc = DetectMoves;

	_tcscpy_s(funcItem[CMD_USE_NAV_BAR]._itemName, nbChar, TEXT("Navigation Bar"));
	funcItem[CMD_USE_NAV_BAR]._pFunc = ViewNavigationBar;

	_tcscpy_s(funcItem[CMD_PREV]._itemName, nbChar, TEXT("Previous"));
	funcItem[CMD_PREV]._pFunc 				= Prev;
	funcItem[CMD_PREV]._pShKey 				= new ShortcutKey;
	funcItem[CMD_PREV]._pShKey->_isAlt 		= true;
	funcItem[CMD_PREV]._pShKey->_isCtrl 	= false;
	funcItem[CMD_PREV]._pShKey->_isShift	= false;
	funcItem[CMD_PREV]._pShKey->_key 		= VK_PRIOR;

	_tcscpy_s(funcItem[CMD_NEXT]._itemName, nbChar, TEXT("Next"));
	funcItem[CMD_NEXT]._pFunc 				= Next;
	funcItem[CMD_NEXT]._pShKey 				= new ShortcutKey;
	funcItem[CMD_NEXT]._pShKey->_isAlt 		= true;
	funcItem[CMD_NEXT]._pShKey->_isCtrl 	= false;
	funcItem[CMD_NEXT]._pShKey->_isShift	= false;
	funcItem[CMD_NEXT]._pShKey->_key 		= VK_NEXT;

	_tcscpy_s(funcItem[CMD_FIRST]._itemName, nbChar, TEXT("First"));
	funcItem[CMD_FIRST]._pFunc 				= First;
	funcItem[CMD_FIRST]._pShKey 			= new ShortcutKey;
	funcItem[CMD_FIRST]._pShKey->_isAlt 	= true;
	funcItem[CMD_FIRST]._pShKey->_isCtrl 	= true;
	funcItem[CMD_FIRST]._pShKey->_isShift	= false;
	funcItem[CMD_FIRST]._pShKey->_key 		= VK_PRIOR;

	_tcscpy_s(funcItem[CMD_LAST]._itemName, nbChar, TEXT("Last"));
	funcItem[CMD_LAST]._pFunc 				= Last;
	funcItem[CMD_LAST]._pShKey 				= new ShortcutKey;
	funcItem[CMD_LAST]._pShKey->_isAlt 		= true;
	funcItem[CMD_LAST]._pShKey->_isCtrl 	= true;
	funcItem[CMD_LAST]._pShKey->_isShift	= false;
	funcItem[CMD_LAST]._pShKey->_key 		= VK_NEXT;

	_tcscpy_s(funcItem[CMD_OPTION]._itemName, nbChar, TEXT("Options"));
	funcItem[CMD_OPTION]._pFunc = OpenOptionDlg;

	_tcscpy_s(funcItem[CMD_ABOUT]._itemName, nbChar, TEXT("About"));
	funcItem[CMD_ABOUT]._pFunc = OpenAboutDlg;
}


// Main plugin DLL function
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD  reasonForCall, LPVOID)
 {
	hInstance = hinstDLL;

	switch (reasonForCall)
	{
		case DLL_PROCESS_ATTACH:
		{
			createMenu();

			for (int i = 0; i < MAXCOMPARE; ++i)
				compareDocs[i] = -1;
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			if (tbNext.hToolbarBmp)
				::DeleteObject(tbNext.hToolbarBmp);

			if (tbPrev.hToolbarBmp)
				::DeleteObject(tbPrev.hToolbarBmp);

			if (tbFirst.hToolbarBmp)
				::DeleteObject(tbFirst.hToolbarBmp);

			if (tbLast.hToolbarBmp)
				::DeleteObject(tbLast.hToolbarBmp);

			OptionDlg.destroy();
			AboutDlg.destroy();
			NavDlg.destroy();

			// Don't forget to deallocate your shortcut here
			for (int i = 0; i < NB_MENU_COMMANDS; i++)
				if (funcItem[i]._pShKey != NULL)
					delete funcItem[i]._pShKey;
		}
		break;

		case DLL_THREAD_ATTACH:
		break;

		case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}


// Notepad++ API functions below
//

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;

	loadSettings();

	AboutDlg.init(hInstance, nppData);
	OptionDlg.init(hInstance, nppData);
	NavDlg.init(hInstance, nppData);
}


extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return PLUGIN_NAME;
}


extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = NB_MENU_COMMANDS;
	return funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code)
	{
		case SCN_PAINTED:
		{
			if (active && NavDlg.isVisible())
			{
				// update NavBar if Npp views got scrolled, resized, etc..
				long start_line, visible_line_count;

				start_line = ::SendMessage(nppData._scintillaMainHandle, SCI_GETFIRSTVISIBLELINE, 0, 0);
				visible_line_count = _MAX(
						::SendMessage(nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0),
						::SendMessage(nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0));
				visible_line_count =
						::SendMessage(nppData._scintillaMainHandle, SCI_VISIBLEFROMDOCLINE, visible_line_count, 0);

				if ((start_line != start_line_old) || (visible_line_count != visible_line_count_old))
				{
					NavDlg.DrawView();
					start_line_old = start_line;
					visible_line_count_old = visible_line_count;
				}
			}
		}
		break;

		case NPPN_TBMODIFICATION:
		{
			UINT style = (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);

			tbPrev.hToolbarBmp = (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREV), IMAGE_BITMAP, 0, 0, style);
			tbNext.hToolbarBmp = (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXT), IMAGE_BITMAP, 0, 0, style);
			tbFirst.hToolbarBmp = (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_FIRST), IMAGE_BITMAP, 0, 0, style);
			tbLast.hToolbarBmp = (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_LAST), IMAGE_BITMAP, 0, 0, style);

			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_FIRST]._cmdID, (LPARAM)&tbFirst);
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_PREV]._cmdID, (LPARAM)&tbPrev);
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_NEXT]._cmdID, (LPARAM)&tbNext);
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_LAST]._cmdID, (LPARAM)&tbLast);

			HMENU hMenu = ::GetMenu(nppData._nppHandle);
			::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_ALIGN_MATCHES]._cmdID, (LPARAM)Settings.AddLine);
			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACING]._cmdID, (LPARAM)Settings.IncludeSpace);
			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID, (LPARAM)Settings.DetectMove);
			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_USE_NAV_BAR]._cmdID, (LPARAM)Settings.UseNavBar);
		}
		break;

		case SCN_UPDATEUI:
		{
			if (active)
			{
				HWND activeView = NULL;
				HWND otherView = NULL;
				if (notifyCode->updated & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL))
				{
					int currentEdit = -1;
					if (notifyCode->updated & SC_UPDATE_SELECTION)
					{
						::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
					}
					else if (notifyCode->updated & SC_UPDATE_V_SCROLL)
					{
						if (notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle)
						{
							currentEdit = 0;
						}
						else if (notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)
						{
							currentEdit = 1;
						}
					}
					if (currentEdit != -1)
					{
						activeView = (currentEdit == 0) ? (nppData._scintillaMainHandle) : (nppData._scintillaSecondHandle);
						otherView = (currentEdit == 0) ? (nppData._scintillaSecondHandle) : (nppData._scintillaMainHandle);
					}
				}
				if ((activeView != NULL) && (otherView != NULL))
				{
					int activeViewTopLine = ::SendMessage(activeView, SCI_GETFIRSTVISIBLELINE, 0, 0);
					activeViewTopLine = ::SendMessage(activeView, SCI_DOCLINEFROMVISIBLE, activeViewTopLine, 0);
					int otherViewTopLine = ::SendMessage(otherView, SCI_VISIBLEFROMDOCLINE, activeViewTopLine, 0);
					::SendMessage(otherView, SCI_SETFIRSTVISIBLELINE, otherViewTopLine, 0);
				}
			}
		}
		break;

		case NPPN_FILEBEFORECLOSE:
		{
			if (!skipAutoReset)
			{
				HWND window = getCurrentWindow();
				int win = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);
				if (getCompare(win) != -1)
				{
					closingWin = win;
					closingView = (HWND)notifyCode->nmhdr.hwndFrom;
					ClearCompare();
				}
			}
		}
		break;

		case NPPN_FILEBEFORESAVE:
		{
			::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)emptyLinesDoc);

			HWND window = getCurrentWindow();
			int win = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);

			if (getCompare(win) != -1)
			{
				topLine = ::SendMessage(window, SCI_GETFIRSTVISIBLELINE, 0, 0);
				lastEmptyLines = removeEmptyLines(window, true);
			}
			else
			{
				lastEmptyLines = NULL;
			}
		}
		break;

		case NPPN_FILESAVED:
		{
			TCHAR name[MAX_PATH];
			::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)name);

			if (lastEmptyLines != NULL && _tcscmp(name, emptyLinesDoc) == 0)
			{
				HWND window = getCurrentWindow();
				::addBlankLines(window,lastEmptyLines);
				int linesOnScreen = ::SendMessage(window, SCI_LINESONSCREEN, 0, 0);
				int curPosBeg = ::SendMessage(window, SCI_GETSELECTIONSTART, 0, 0);
				int curPosEnd = ::SendMessage(window, SCI_GETSELECTIONEND, 0, 0);
				::SendMessage(window, SCI_GOTOLINE, topLine, 0);
				::SendMessage(window, SCI_GOTOLINE, topLine + linesOnScreen - 1, 0);
				::SendMessage(window, SCI_SETSEL, curPosBeg, curPosEnd);
				cleanEmptyLines(lastEmptyLines);
				delete lastEmptyLines;
				lastEmptyLines = NULL;
				emptyLinesDoc[0] = 0;
			}
		}
		break;

		case NPPN_SHUTDOWN:
		{
			saveSettings();

			// Always close it, else N++'s plugin manager would call 'ViewNavigationBar'
			// on startup, when N++ has been shut down before with opened navigation bar
			if (NavDlg.isVisible())
				NavDlg.doDialog(false);
		}
		break;
	}
}


extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM, LPARAM)
{
	if (Message == WM_CREATE)
	{
		HMENU hMenu = ::GetMenu(nppData._nppHandle);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_1]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_2]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_3]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_4]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	}

	return TRUE;
}


extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
