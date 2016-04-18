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

#include <stdlib.h>
#include <exception>
#include <vector>
#include <memory>
#include <utility>

#include "Compare.h"
#include "NPPHelpers.h"
#include "ScmHelper.h"
#include "CProgress.h"
#include "AboutDialog.h"
#include "OptionDialog.h"
#include "NavDialog.h"
#include "diff.h"
#include "Engine.h"


NppData nppData;
sUserSettings Settings;


namespace // anonymous namespace
{

const TCHAR PLUGIN_NAME[]			= TEXT("Compare");

const TCHAR mainSection[]			= TEXT("Compare Settings");
const TCHAR firstFileViewOption[]	= TEXT("Place First Compared File in the Right/Bottom View");
const TCHAR addLinesOption[]		= TEXT("Align Matches");
const TCHAR ignoreSpacesOption[]	= TEXT("Include Spaces");
const TCHAR detectMovesOption[]		= TEXT("Detect Move Blocks");
const TCHAR NavBarOption[]			= TEXT("Navigation bar");

const TCHAR colorsSection[]			= TEXT("Colors");
const TCHAR addedColorOption[]		= TEXT("Added");
const TCHAR removedColorOption[]	= TEXT("Removed");
const TCHAR changedColorOption[]	= TEXT("Changed");
const TCHAR movedColorOption[]		= TEXT("Moved");
const TCHAR highlightColorOption[]	= TEXT("Highlight");
const TCHAR highlightAlphaOption[]	= TEXT("Alpha");


/**
 *  \struct
 *  \brief
 */
struct ScopedIncrementer
{
	ScopedIncrementer(unsigned& useCount) : _useCount(useCount)
	{
		++_useCount;
	}

	~ScopedIncrementer()
	{
		--_useCount;
	}

private:
	unsigned&	_useCount;
};


/**
 *  \struct
 *  \brief
 */
struct NppSettings
{
	NppSettings() : compareMode(false) {}

	bool	compareMode;
	bool	syncVScroll;
	bool	syncHScroll;

	void updatePluginMenu() const;
	void save();
	void setNormalMode();
	void setCompareMode();
};


/**
 *  \struct
 *  \brief
 */
struct ComparedFile
{
	int		originalViewId;
	int		buffId;
	int		sciDoc;
	TCHAR	name[MAX_PATH];
};


/**
 *  \struct
 *  \brief
 */
struct SaveNotificationData
{
	SaveNotificationData(const ComparedFile& savedFile) : file(savedFile) {}

	ComparedFile		file;
	int					firstVisibleLine;
	int					position;
	BlankSections_t		blankSections;
};


enum CompareResult_t
{
	COMPARE_ERROR = 0,
	COMPARE_CANCELLED,
	FILES_MATCH,
	FILES_DIFFER
};


using ComparePair_t = std::pair<ComparedFile, ComparedFile>;
using CompareList_t = std::vector<ComparePair_t>;


CompareList_t compareList;
std::unique_ptr<ComparedFile> firstFile;
int firstFileCodepage;
NppSettings nppSettings;

unsigned notificationsLock = 0;

long start_line_old;
long visible_line_count_old;

std::unique_ptr<SaveNotificationData> saveNotifData;

// int  tempWindow = -1;
// bool skipAutoReset = false;
// TCHAR compareFilePath[MAX_PATH];

CProgress* progDlg = NULL;
int progMax = 0;
int progCounter = 0;

AboutDialog   AboutDlg;
OptionDialog  OptionDlg;
NavDialog     NavDlg;

toolbarIcons  tbPrev;
toolbarIcons  tbNext;
toolbarIcons  tbFirst;
toolbarIcons  tbLast;

HINSTANCE hInstance;
FuncItem funcItem[NB_MENU_COMMANDS] = { 0 };


// Declare local functions that appear before they are defined
void SetAsFirst();
void Compare();
void ClearCurrentCompare();
void First();
void openMemBlock(const char* memblock, long size);
void onBufferActivated(int buffId);


void loadSettings()
{
	TCHAR iniFilePath[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFilePath), (LPARAM)iniFilePath);

	::PathAppend(iniFilePath, TEXT("Compare.ini"));

	Settings.ColorSettings.added =
			::GetPrivateProfileInt(colorsSection, addedColorOption, DEFAULT_ADDED_COLOR, iniFilePath);
	Settings.ColorSettings.deleted =
			::GetPrivateProfileInt(colorsSection, removedColorOption, DEFAULT_DELETED_COLOR, iniFilePath);
	Settings.ColorSettings.changed =
			::GetPrivateProfileInt(colorsSection, changedColorOption, DEFAULT_CHANGED_COLOR, iniFilePath);
	Settings.ColorSettings.moved =
			::GetPrivateProfileInt(colorsSection, movedColorOption, DEFAULT_MOVED_COLOR, iniFilePath);
	Settings.ColorSettings.highlight =
			::GetPrivateProfileInt(colorsSection, highlightColorOption, DEFAULT_HIGHLIGHT_COLOR, iniFilePath);
	Settings.ColorSettings.alpha =
			::GetPrivateProfileInt(colorsSection, highlightAlphaOption, DEFAULT_HIGHLIGHT_ALPHA, iniFilePath);

	Settings.FirstFileCompareViewId = ::GetPrivateProfileInt(mainSection, firstFileViewOption, SUB_VIEW, iniFilePath);

	Settings.AddLine      = ::GetPrivateProfileInt(mainSection, addLinesOption,		1, iniFilePath) == 1;
	Settings.IncludeSpace = ::GetPrivateProfileInt(mainSection, ignoreSpacesOption,	0, iniFilePath) == 1;
	Settings.DetectMove   = ::GetPrivateProfileInt(mainSection, detectMovesOption,	1, iniFilePath) == 1;
	Settings.UseNavBar    = ::GetPrivateProfileInt(mainSection, NavBarOption,		0, iniFilePath) == 1;
}


void saveSettings()
{
	TCHAR iniFilePath[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFilePath), (LPARAM)iniFilePath);
	::PathAppend(iniFilePath, TEXT("Compare.ini"));

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

	_itot_s(Settings.FirstFileCompareViewId, buffer, 64, 10);
	::WritePrivateProfileString(mainSection, firstFileViewOption, buffer, iniFilePath);

	::WritePrivateProfileString(mainSection, addLinesOption,
			Settings.AddLine ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(mainSection, ignoreSpacesOption,
			Settings.IncludeSpace ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(mainSection, detectMovesOption,
			Settings.DetectMove ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(mainSection, NavBarOption,
			Settings.UseNavBar ? TEXT("1") : TEXT("0"), iniFilePath);
}


ComparedFile& getFileFromPair(ComparePair_t& pair, int viewId)
{
	return (viewIdFromBuffId(pair.first.buffId) == viewId) ? pair.first : pair.second;
}


CompareList_t::iterator getCompare(int buffId)
{
	for (CompareList_t::iterator it = compareList.begin(); it < compareList.end(); ++it)
	{
		if (it->first.buffId == buffId || it->second.buffId == buffId)
			return it;
	}

	return compareList.end();
}


CompareList_t::iterator getCompareBySciDoc(int sciDoc)
{
	for (CompareList_t::iterator it = compareList.begin(); it < compareList.end(); ++it)
	{
		if (it->first.sciDoc == sciDoc || it->second.sciDoc == sciDoc)
			return it;
	}

	return compareList.end();
}


void NppSettings::updatePluginMenu() const
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
	int flag = MF_BYCOMMAND | (compareMode ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));

	::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_CURRENT]._cmdID, flag);
}


void NppSettings::save()
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	syncVScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
	syncHScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
}


void NppSettings::setNormalMode()
{
	if (compareMode == false)
		return;

	compareMode = false;

	updatePluginMenu();

	if (NavDlg.isVisible())
		NavDlg.doDialog(false);

	::SendMessage(nppData._scintillaMainHandle, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);

	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	bool syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
	if (syncScroll != syncVScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

	syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
	if (syncScroll != syncHScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLH);
}


void NppSettings::setCompareMode()
{
	if (compareMode == true)
		return;

	compareMode = true;

	::SendMessage(nppData._scintillaMainHandle, SCI_SETCARETLINEBACKALPHA, 96, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETCARETLINEBACKALPHA, 96, 0);

	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	bool syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;

	// Disable N++ vertical scroll - we handle it manually because of the Word Wrap
	if (syncScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

	syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;

	// Yaron - Enable N++ horizontal scroll sync
	if (!syncScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLH);

	// synchronize zoom levels
	int zoom = ::SendMessage(getCurrentView(), SCI_GETZOOM, 0, 0);
	::SendMessage(getOtherView(), SCI_SETZOOM, zoom, 0);
}


void progressOpen(const TCHAR* msg)
{
	progMax = 0;
	progCounter = 0;
	progDlg = new CProgress();
	progDlg->Open(nppData._nppHandle, TEXT("Compare Plugin"));
	progDlg->SetInfo(msg);

	::EnableWindow(nppData._nppHandle, FALSE);
}


void progressFillCompareInfo(const TCHAR* first, const TCHAR* second)
{
	first = ::PathFindFileName(first);
	second = ::PathFindFileName(second);

	TCHAR msg[MAX_PATH];
	_sntprintf_s(msg, _countof(msg), _TRUNCATE, TEXT("Comparing \"%s\" vs. \"%s\"..."), first, second);

	progDlg->SetInfo(msg);
}


void progressClose()
{
	if (progDlg)
	{
		::EnableWindow(nppData._nppHandle, TRUE);

		progDlg->Close();
		delete progDlg;
		progDlg = NULL;
	}
}


bool isCompareCancelled()
{
	if (progDlg->IsCancelled())
	{
		progressClose();
		return true;
	}

	return false;
}


void showNavBar()
{
	start_line_old = -1;
	visible_line_count_old = -1;

	// Save current N++ focus
	HWND hwnd = ::GetFocus();

	// Display NavBar
	NavDlg.SetColor(Settings.ColorSettings);
	NavDlg.doDialog(true);

	// Restore N++ focus
	::SetFocus(hwnd);
}


void jumpChangedLines(bool direction)
{
	HWND CurView = getCurrentView();

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

	if ((direction && (nextLine < prevLine)) || (!direction && (nextLine > prevLine)))
	{
		FLASHWINFO flashInfo;
		flashInfo.cbSize = sizeof(flashInfo);
		flashInfo.hwnd = nppData._nppHandle;
		flashInfo.uCount = 2;
		flashInfo.dwTimeout = 100;
		flashInfo.dwFlags = FLASHW_ALL;
		::FlashWindowEx(&flashInfo);
	}

	::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
	::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
}


// HWND openTempFile()
// {
	// char original[MAX_PATH];

	// ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(original), (LPARAM)original);
	// HWND originalwindow = getCurrentView();

	// LRESULT curBuffer = getCurrentBuffId();
	// LangType curLang = (LangType)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE, curBuffer, 0);

	// int result = ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
	// HWND window = getCurrentView();
	// int win = getDocId(window);

	// if (result == 0 || win != tempWindow)
	// {
		// ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);
		// ::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)compareFilePath);
		// tempWindow = getDocId(window);

		// curBuffer = getCurrentBuffId();
		// ::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, curBuffer, curLang);
	// }

	// if (originalwindow == window)
	// {
		// ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)original);
		// skipAutoReset = true;
		// ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		// skipAutoReset = false;
		// ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
	// }

	// result=::SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE, 0, (LPARAM)original);

	// window = getOtherView();

	// int pointer = getDocId(window);
	// if (tempWindow != pointer)
	// {
		// window = getCurrentView();
		// pointer = getDocId(window);
	// }

	// move focus to new document, or the other document will be marked as dirty
	// ::SendMessage(window, SCI_GRABFOCUS, 0, 0);
	// ::SendMessage(window, SCI_SETREADONLY, 0, 0);
	// ::SendMessage(window, SCI_CLEARALL, 0, 0);

	// return window;
	// return NULL;
// }


void openFile(const TCHAR *file)
{
	if(file == NULL || ::PathFileExists(file) == FALSE)
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


void openMemBlock(const char* memblock, long size)
{
	// HWND window = openTempFile();

	// ::SendMessage(window, SCI_GRABFOCUS, 0, 0);
	// ::SendMessage(window, SCI_APPENDTEXT, size, (LPARAM)memblock);

	// Compare();
	// if (!startCompare())
	// {
		// ::SendMessage(window, SCI_GRABFOCUS, 0, 0);
		// ::SendMessage(window, SCI_SETSAVEPOINT, 1, 0);
		// ::SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, 0);
		// ::SendMessage(window, SCI_SETREADONLY, 1, 0);

		// ClearCurrentCompare();
	// }
	// else
	// {
		// ::SendMessage(window, SCI_GRABFOCUS, 0, 0);
		// ::SendMessage(window, SCI_SETSAVEPOINT, 1, 0);
		// ::SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, 0);
		// ::SendMessage(window, SCI_SETREADONLY, 1, 0);
		// ::SendMessage(nppData._scintillaSecondHandle, SCI_GRABFOCUS, 0, 1);
	// }
}


CompareResult_t doCompare(const TCHAR* first, const TCHAR* second)
{
	HWND view1;
	HWND view2;

	if (Settings.FirstFileCompareViewId == MAIN_VIEW)
	{
		view1 = nppData._scintillaSecondHandle;
		view2 = nppData._scintillaMainHandle;
	}
	else
	{
		view1 = nppData._scintillaMainHandle;
		view2 = nppData._scintillaSecondHandle;
	}

	std::vector<int> lineNum1;
	const DocLines_t doc1 = getAllLines(view1, lineNum1);
	int doc1Length = doc1.size();

	if (doc1Length == 1 && doc1[0][0] == 0)
		return COMPARE_CANCELLED;

	std::vector<int> lineNum2;
	const DocLines_t doc2 = getAllLines(view2, lineNum2);
	int doc2Length = doc2.size();

	if (doc2Length == 1 && doc2[0][0] == 0)
		return COMPARE_CANCELLED;

	progressOpen(TEXT("Computing hashes..."));

	std::vector<unsigned int> doc1Hashes = computeHashes(doc1, Settings.IncludeSpace);
	std::vector<unsigned int> doc2Hashes = computeHashes(doc2, Settings.IncludeSpace);

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

	progressFillCompareInfo(first, second);

	std::vector<diff_edit> diff = DiffCalc<unsigned int>(doc1Hashes, doc2Hashes)();

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

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

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

	std::vector<diff_edit> doc1Changes(doc1Changed);
	std::vector<diff_edit> doc2Changes(doc2Changed);

	int doc1Offset = 0;
	int doc2Offset = 0;

	// Switch from blocks of lines to one change per line.
	// Change CHANGE to DELETE or INSERT if there are no changes on that line
	{
		int added;
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

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

	bool different = true;

	if (!diff.empty())
	{
		int textIndex;
		different = (doc1Changed > 0) || (doc2Changed > 0);

		for (int i = 0; i < doc1Changed; ++i)
		{
			switch (doc1Changes[i].op)
			{
				case DIFF_DELETE:
					markAsRemoved(view1, doc1Changes[i].off);
				break;

				case DIFF_CHANGE1:
					markAsChanged(view1, doc1Changes[i].off);
					textIndex = lineNum1[doc1Changes[i].off];

					for (unsigned int k = 0; k < doc1Changes[i].changeCount; ++k)
					{
						diff_change& change = doc1Changes[i].changes->get(k);
						markTextAsChanged(view1, textIndex + change.off, change.len);
					}
				break;

				case DIFF_MOVE:
					markAsMoved(view1, doc1Changes[i].off);
				break;
			}
		}

		for (int i = 0; i < doc2Changed; ++i)
		{
			switch (doc2Changes[i].op)
			{
				case DIFF_INSERT:
					markAsAdded(view2, doc2Changes[i].off);
				break;

				case DIFF_CHANGE2:
					markAsChanged(view2, doc2Changes[i].off);
					textIndex = lineNum2[doc2Changes[i].off];

					for (unsigned int k = 0; k < doc2Changes[i].changeCount; ++k)
					{
						diff_change& change = doc2Changes[i].changes->get(k);
						markTextAsChanged(view2, textIndex + change.off, change.len);
					}
				break;

				case DIFF_MOVE:
					markAsMoved(view2, doc2Changes[i].off);
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
							addBlankSection(view2, off + doc2Offset, length);
							doc2Offset += length;
							off = doc1Changes[i].altLocation;
							length = 1;
						}
					break;
				}
			}

			addBlankSection(view2, off + doc2Offset, length);

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
							addBlankSection(view1, off + doc1Offset, length);
							doc1Offset += length;
							off = doc2Changes[i].altLocation;
							length = 1;
						}
					break;
				}
			}

			addBlankSection(view1, off + doc1Offset, length);
		}
	}

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

	progressClose();

	return (!diff.empty() && !different) ? FILES_MATCH : FILES_DIFFER;
}


CompareResult_t runCompare(CompareList_t::iterator& cmpPair)
{
	nppSettings.setCompareMode();

	ScopedViewWriteEnabler writeEn1(nppData._scintillaMainHandle);
	ScopedViewWriteEnabler writeEn2(nppData._scintillaSecondHandle);

	setStyles(Settings);

	ScopedViewUndoCollectionBlocker undoBlock1(nppData._scintillaMainHandle);
	ScopedViewUndoCollectionBlocker undoBlock2(nppData._scintillaSecondHandle);

	CompareResult_t result = COMPARE_ERROR;

	try
	{
		result = doCompare(cmpPair->first.name, cmpPair->second.name);
	}
	catch (std::exception& e)
	{
		progressClose();
		char msg[128];
		_snprintf_s(msg, _countof(msg), _TRUNCATE, "Exception occurred: %s", e.what());
		MessageBoxA(nppData._nppHandle, msg, "Compare Plugin", MB_OK | MB_ICONWARNING);
	}
	catch (...)
	{
		progressClose();
		MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "Compare Plugin", MB_OK | MB_ICONWARNING);
	}

	return result;
}


CompareList_t::iterator createComparePair()
{
	const int secondViewId = getCurrentViewId();
	const bool moveToOtherViewNeeded = (firstFile->originalViewId == secondViewId);

	// This is needed to help us put pair.first in main view and pair.second in sub view
	const bool swapPair = moveToOtherViewNeeded ?
		(Settings.FirstFileCompareViewId != MAIN_VIEW) : (firstFile->originalViewId != MAIN_VIEW);

	ComparePair_t cmpPair;
	ComparedFile* first;
	ComparedFile* second;

	if (swapPair)
	{
		first = &cmpPair.second;
		second = &cmpPair.first;
	}
	else
	{
		first = &cmpPair.first;
		second = &cmpPair.second;
	}

	*first = *firstFile;
	firstFile.reset();

	second->originalViewId = secondViewId;
	second->buffId = getCurrentBuffId();
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(second->name), (LPARAM)second->name);

	if (moveToOtherViewNeeded && second->originalViewId == Settings.FirstFileCompareViewId)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);

	second->sciDoc = getDocId(getCurrentView());

	activateBufferID(first->buffId);

	if (moveToOtherViewNeeded && first->originalViewId != Settings.FirstFileCompareViewId)
	{
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);

		// If we change the first file view we need to re-set it's doc Id.
		// The current view will open semi-random file when the first is moved -
		// we need to activate the second file there. Then return to the first file again
		first->sciDoc = getDocId(getCurrentView());
		activateBufferID(second->buffId);
		activateBufferID(first->buffId);
	}

	compareList.push_back(cmpPair);

	return compareList.end() - 1;
}


void restoreFile(const ComparedFile& comparedFile)
{
	const int viewId = viewIdFromBuffId(comparedFile.buffId);
	HWND hView = getView(viewId);

	activateBufferID(comparedFile.buffId);

	if (viewId != comparedFile.originalViewId)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);

	ScopedViewWriteEnabler writeEn(hView);
	clearWindow(hView);
}


void clearComparePair(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);

	if (cmpPair == compareList.end())
		return;

	nppSettings.setNormalMode();
	restoreMargin(nppData._scintillaMainHandle);
	restoreMargin(nppData._scintillaSecondHandle);

	ScopedIncrementer incr(notificationsLock);

	if (cmpPair->first.buffId == buffId)
	{
		restoreFile(cmpPair->second);
		restoreFile(cmpPair->first);
	}
	else
	{
		restoreFile(cmpPair->first);
		restoreFile(cmpPair->second);
	}

	compareList.erase(cmpPair);
}


void closeComparePair(CompareList_t::iterator& cmpPair)
{
	nppSettings.setNormalMode();
	restoreMargin(nppData._scintillaMainHandle);
	restoreMargin(nppData._scintillaSecondHandle);

	HWND currentView = getCurrentView();

	ScopedIncrementer incr(notificationsLock);

	activateBufferID(cmpPair->second.buffId);
	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
	activateBufferID(cmpPair->first.buffId);
	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);

	compareList.erase(cmpPair);

	onBufferActivated(getCurrentBuffId());

	if (::IsWindowVisible(currentView))
		::SetFocus(currentView);
}


bool isFileEmpty(HWND view)
{
	if (::SendMessage(view, SCI_GETLENGTH, 0, 0) == 0)
	{
		::MessageBox(nppData._nppHandle, TEXT("Trying to compare empty file - operation ignored."),
				TEXT("Compare Plugin"), MB_OK);
		return true;
	}

	return false;
}


bool prepareFiles()
{
	bool manualSelectionMode = false;

	if ((bool)firstFile)
	{
		// Check if the first file is still open
		const int buffLen = ::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, firstFile->buffId, 0);
		if (buffLen > 0)
		{
			std::vector<TCHAR> file(buffLen + 1, 0);
			::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, firstFile->buffId, (LPARAM)file.data());
			if (!_tcscmp(file.data(), firstFile->name))
				manualSelectionMode = true;
		}

		// Detect if the first file is moved to the other view
		if (manualSelectionMode && firstFile->originalViewId != viewIdFromBuffId(firstFile->buffId))
			manualSelectionMode = false;
	}

	if (manualSelectionMode)
	{
		// Compare to self?
		if (firstFile->buffId == getCurrentBuffId())
		{
			::MessageBox(nppData._nppHandle, TEXT("Trying to compare file to itself - operation ignored."),
					TEXT("Compare Plugin"), MB_OK);
			return false;
		}

		if (isFileEmpty(getCurrentView()))
			return false;
	}
	else
	{
		SetAsFirst();

		HWND currentView = getCurrentView();

		if (isFileEmpty(currentView))
			return false;

		if (isSingleView())
		{
			int viewId = getCurrentViewId();
			viewId = (viewId == MAIN_VIEW) ? PRIMARY_VIEW : SECOND_VIEW;

			if (::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, viewId) < 2)
			{
				::MessageBox(nppData._nppHandle, TEXT("Only one file opened - operation ignored."),
						TEXT("Compare Plugin"), MB_OK);
				return false;
			}

			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0,
					Settings.FirstFileCompareViewId == MAIN_VIEW ? IDM_VIEW_TAB_NEXT : IDM_VIEW_TAB_PREV);

			if (isFileEmpty(currentView))
			{
				activateBufferID(firstFile->buffId);
				return false;
			}
		}
		else
		{
			// Check if the file in the other view is compared already
			HWND otherView = getOtherView();
			const int sciDoc = getDocId(otherView);

			CompareList_t::iterator cmpPair = getCompareBySciDoc(sciDoc);
			if (cmpPair != compareList.end())
			{
				const TCHAR* fname;
				if (cmpPair->first.sciDoc == sciDoc)
					fname = ::PathFindFileName(cmpPair->first.name);
				else
					fname = ::PathFindFileName(cmpPair->second.name);

				TCHAR msg[MAX_PATH];
				_sntprintf_s(msg, _countof(msg), _TRUNCATE,
						TEXT("File \"%s\" is already compared - operation ignored."), fname);
				::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin"), MB_OK);

				return false;
			}

			if (isFileEmpty(otherView))
				return false;

			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SWITCHTO_OTHER_VIEW);
		}
	}

	// Warn about encoding mismatches as that might compromise the compare
	if (firstFileCodepage != SendMessage(getCurrentView(), SCI_GETCODEPAGE, 0, 0))
	{
		if (::MessageBox(nppData._nppHandle,
			TEXT("Trying to compare files with different encodings - \n")
			TEXT("the result might be inaccurate and misleading.\n\n")
			TEXT("Compare anyway?"), TEXT("Compare Plugin"), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
		{
			activateBufferID(firstFile->buffId);
			return false;
		}
	}

	if (compareList.empty())
		nppSettings.save();

	return true;
}


void SetAsFirst()
{
	firstFile.reset(new ComparedFile);

	firstFile->buffId = getCurrentBuffId();

	HWND view = getCurrentView();

	firstFile->originalViewId = getCurrentViewId();
	firstFile->sciDoc = getDocId(view);
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(firstFile->name), (LPARAM)firstFile->name);

	firstFileCodepage = SendMessage(view, SCI_GETCODEPAGE, 0, 0);

	// Enable ClearCurrentCompare command to be able to clear the first file that was just set
	::EnableMenuItem((HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0),
			funcItem[CMD_CLEAR_CURRENT]._cmdID, MF_BYCOMMAND | MF_ENABLED);
}


void Compare()
{
	ScopedIncrementer incr(notificationsLock);

	CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());

	// New compare
	if (cmpPair == compareList.end())
	{
		if (!prepareFiles())
		{
			firstFile.reset();
			nppSettings.updatePluginMenu();
			return;
		}

		cmpPair = createComparePair();
	}
	// Re-Compare triggered - clear current results
	else
	{
		firstFile.reset();
		clearWindow(nppData._scintillaMainHandle);
		clearWindow(nppData._scintillaSecondHandle);
	}

	switch (runCompare(cmpPair))
	{
		case FILES_DIFFER:
		{
			First();

			nppSettings.updatePluginMenu();

			if (Settings.UseNavBar)
				showNavBar();
		}
		break;

		case FILES_MATCH:
		{
			TCHAR msg[2 * MAX_PATH];

			const TCHAR* first = ::PathFindFileName(cmpPair->first.name);
			const TCHAR* second = ::PathFindFileName(cmpPair->second.name);

			_sntprintf_s(msg, _countof(msg), _TRUNCATE,
					TEXT("Files \"%s\" and \"%s\" match.\n\nClose compared files?"), first, second);
			if (::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin: Files Match"),
					MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
				closeComparePair(cmpPair);
			else
				ClearCurrentCompare();
		}
		break;

		default:
			ClearCurrentCompare();
	}
}


void ClearCurrentCompare()
{
	firstFile.reset();

	if (!nppSettings.compareMode)
		nppSettings.updatePluginMenu();
	else
		clearComparePair(getCurrentBuffId());
}


void ClearAllCompares()
{
	firstFile.reset();

	if (!nppSettings.compareMode)
		nppSettings.updatePluginMenu();
	else
		nppSettings.setNormalMode();

	restoreMargin(nppData._scintillaMainHandle);
	restoreMargin(nppData._scintillaSecondHandle);

	const int buffId = getCurrentBuffId();

	ScopedIncrementer incr(notificationsLock);

	for (int i = compareList.size() - 1; i >= 0; --i)
	{
		restoreFile(compareList[i].first);
		restoreFile(compareList[i].second);
	}

	compareList.clear();

	activateBufferID(buffId);
}


void CompareToSave()
{
	TCHAR file[MAX_PATH];
	::SendMessage(nppData._nppHandle,NPPM_GETCURRENTDIRECTORY,0,(LPARAM)file);

	if (file[0] != 0)
		::SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	openFile(file);
}


void CompareToSvn()
{
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


void CompareToGit()
{
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


void AlignMatches()
{
	Settings.AddLine = !Settings.AddLine;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_ALIGN_MATCHES]._cmdID,
			(LPARAM)Settings.AddLine);
}


void IncludeSpacing()
{
	Settings.IncludeSpace = !Settings.IncludeSpace;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACING]._cmdID,
			(LPARAM)Settings.IncludeSpace);
}


void DetectMoves()
{
	Settings.DetectMove = !Settings.DetectMove;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMove);
}


void Prev()
{
	if (nppSettings.compareMode)
		jumpChangedLines(false);
}


void Next()
{
	if (nppSettings.compareMode)
		jumpChangedLines(true);
}


void First()
{
	if (nppSettings.compareMode)
	{
		HWND CurView = getCurrentView();
		HWND OtherView = getOtherView();

		const int sci_search_mask = (1 << MARKER_MOVED_LINE)
								  | (1 << MARKER_CHANGED_LINE)
								  | (1 << MARKER_ADDED_LINE)
								  | (1 << MARKER_REMOVED_LINE)
								  | (1 << MARKER_BLANK_LINE);

		const int nextLine = ::SendMessage(CurView, SCI_MARKERNEXT, 0, sci_search_mask);
		::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(OtherView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
		::SendMessage(OtherView, SCI_GOTOLINE, nextLine, 0);
	}
}


void Last()
{
	if (nppSettings.compareMode)
	{
		HWND CurView = getCurrentView();

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


void OpenOptionDlg(void)
{
	if (OptionDlg.doDialog(&Settings) == IDOK)
	{
		saveSettings();

		if (!compareList.empty())
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


void OpenAboutDlg()
{
	AboutDlg.doDialog();
}


void createMenu()
{
	_tcscpy_s(funcItem[CMD_SELECT_FIRST]._itemName, nbChar, TEXT("Set as first to Compare"));
	funcItem[CMD_SELECT_FIRST]._pFunc				= SetAsFirst;
	funcItem[CMD_SELECT_FIRST]._pShKey				= new ShortcutKey;
	funcItem[CMD_SELECT_FIRST]._pShKey->_isAlt		= true;
	funcItem[CMD_SELECT_FIRST]._pShKey->_isCtrl		= false;
	funcItem[CMD_SELECT_FIRST]._pShKey->_isShift	= false;
	funcItem[CMD_SELECT_FIRST]._pShKey->_key		= 'F';

	_tcscpy_s(funcItem[CMD_COMPARE]._itemName, nbChar, TEXT("Compare"));
	funcItem[CMD_COMPARE]._pFunc			= Compare;
	funcItem[CMD_COMPARE]._pShKey			= new ShortcutKey;
	funcItem[CMD_COMPARE]._pShKey->_isAlt	= true;
	funcItem[CMD_COMPARE]._pShKey->_isCtrl	= false;
	funcItem[CMD_COMPARE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE]._pShKey->_key		= 'D';

	_tcscpy_s(funcItem[CMD_CLEAR_CURRENT]._itemName, nbChar, TEXT("Clear Current Compare"));
	funcItem[CMD_CLEAR_CURRENT]._pFunc 				= ClearCurrentCompare;
	funcItem[CMD_CLEAR_CURRENT]._pShKey 			= new ShortcutKey;
	funcItem[CMD_CLEAR_CURRENT]._pShKey->_isAlt 	= true;
	funcItem[CMD_CLEAR_CURRENT]._pShKey->_isCtrl	= false;
	funcItem[CMD_CLEAR_CURRENT]._pShKey->_isShift	= true;
	funcItem[CMD_CLEAR_CURRENT]._pShKey->_key 		= 'D';

	_tcscpy_s(funcItem[CMD_CLEAR_ALL]._itemName, nbChar, TEXT("Clear All Compares"));
	funcItem[CMD_CLEAR_ALL]._pFunc 				= ClearAllCompares;
	funcItem[CMD_CLEAR_ALL]._pShKey 			= new ShortcutKey;
	funcItem[CMD_CLEAR_ALL]._pShKey->_isAlt 	= true;
	funcItem[CMD_CLEAR_ALL]._pShKey->_isCtrl	= true;
	funcItem[CMD_CLEAR_ALL]._pShKey->_isShift	= true;
	funcItem[CMD_CLEAR_ALL]._pShKey->_key 		= 'D';

	_tcscpy_s(funcItem[CMD_COMPARE_LAST_SAVE]._itemName, nbChar, TEXT("Compare to last Save"));
	funcItem[CMD_COMPARE_LAST_SAVE]._pFunc 				= CompareToSave;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey 			= new ShortcutKey;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isAlt 	= true;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isCtrl 	= false;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_key 		= 'S';

	_tcscpy_s(funcItem[CMD_COMPARE_SVN_BASE]._itemName, nbChar, TEXT("Compare to SVN base"));
	funcItem[CMD_COMPARE_SVN_BASE]._pFunc 				= CompareToSvn;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey 				= new ShortcutKey;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isAlt 		= true;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isCtrl 	= false;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_key 		= 'B';

	_tcscpy_s(funcItem[CMD_COMPARE_GIT_BASE]._itemName, nbChar, TEXT("Compare to GIT base"));
	funcItem[CMD_COMPARE_GIT_BASE]._pFunc 				= CompareToGit;
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

	_tcscpy_s(funcItem[CMD_OPTION]._itemName, nbChar, TEXT("Options..."));
	funcItem[CMD_OPTION]._pFunc = OpenOptionDlg;

	_tcscpy_s(funcItem[CMD_ABOUT]._itemName, nbChar, TEXT("About..."));
	funcItem[CMD_ABOUT]._pFunc = OpenAboutDlg;
}


void deinitPlugin()
{
	// Always close it, else N++'s plugin manager would call 'ViewNavigationBar'
	// on startup, when N++ has been shut down before with opened navigation bar
	if (NavDlg.isVisible())
		NavDlg.doDialog(false);

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


void onToolBarReady()
{
	UINT style = (LR_SHARED | LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);

	tbPrev.hToolbarBmp	= (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREV),	IMAGE_BITMAP, 0, 0, style);
	tbNext.hToolbarBmp	= (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXT),	IMAGE_BITMAP, 0, 0, style);
	tbFirst.hToolbarBmp	= (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_FIRST),	IMAGE_BITMAP, 0, 0, style);
	tbLast.hToolbarBmp	= (HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_LAST),	IMAGE_BITMAP, 0, 0, style);

	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_FIRST]._cmdID,	(LPARAM)&tbFirst);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_PREV]._cmdID,	(LPARAM)&tbPrev);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_NEXT]._cmdID,	(LPARAM)&tbNext);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_LAST]._cmdID,	(LPARAM)&tbLast);

	nppSettings.updatePluginMenu();

	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_ALIGN_MATCHES]._cmdID,
			(LPARAM)Settings.AddLine);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACING]._cmdID,
			(LPARAM)Settings.IncludeSpace);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMove);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_USE_NAV_BAR]._cmdID,
			(LPARAM)Settings.UseNavBar);
}


void onSciPaint()
{
	// update NavBar if Npp views got scrolled, resized, etc..
	long start_line, visible_line_count;

	start_line = ::SendMessage(nppData._scintillaMainHandle, SCI_GETFIRSTVISIBLELINE, 0, 0);
	visible_line_count = _MAX(
			::SendMessage(nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0),
			::SendMessage(nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0));
	visible_line_count = ::SendMessage(nppData._scintillaMainHandle, SCI_VISIBLEFROMDOCLINE, visible_line_count, 0);

	if ((start_line != start_line_old) || (visible_line_count != visible_line_count_old))
	{
		NavDlg.DrawView();
		start_line_old = start_line;
		visible_line_count_old = visible_line_count;
	}
}


void onSciUpdateUI(SCNotification *notifyCode)
{
	HWND activeView = NULL;
	HWND otherView = NULL;

	if (notifyCode->updated & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL))
	{
		int currentView = -1;

		if (notifyCode->updated & SC_UPDATE_SELECTION)
		{
			currentView = getCurrentViewId();
		}
		else if (notifyCode->updated & SC_UPDATE_V_SCROLL)
		{
			if (notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle)
				currentView = MAIN_VIEW;
			else if (notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)
				currentView = SUB_VIEW;
		}
		if (currentView != -1)
		{
			activeView = getView(currentView);
			otherView = getView(!currentView);
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


void onSciModified(SCNotification *notifyCode)
{
	CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	if (notifyCode->modificationType == (SC_MOD_BEFOREDELETE | SC_PERFORMED_USER))
	{
		ScopedIncrementer incr(notificationsLock);

		HWND currentView = getCurrentView();

		int line = ::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position, 0);
		const int endLine =
			::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position + notifyCode->length, 0);

		while (line < endLine)
			::SendMessage(currentView, SCI_MARKERDELETE, line++, -1);
	}
}


void onSciZoom()
{
	CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	ScopedIncrementer incr(notificationsLock);

	// sync both views zoom
	int zoom = ::SendMessage(getCurrentView(), SCI_GETZOOM, 0, 0);
	::SendMessage(getOtherView(), SCI_SETZOOM, zoom, 0);
}


void onBufferActivated(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);

	if (cmpPair == compareList.end())
	{
		nppSettings.setNormalMode();
		restoreMargin(getCurrentView());
		return;
	}

	// Compared file moved to other view? -> clear compare
	if (viewIdFromBuffId(cmpPair->first.buffId) == viewIdFromBuffId(cmpPair->second.buffId))
	{
		clearComparePair(buffId);
		return;
	}

	bool switchedFromOtherPair = false;
	const int otherViewId = getOtherViewId();
	const ComparedFile& otherFile = getFileFromPair(*cmpPair, otherViewId);

	// When compared file is activated make sure its corresponding pair file is
	// also active in the other view
	if (getDocId(getView(otherViewId)) != otherFile.sciDoc)
	{
		ScopedIncrementer incr(notificationsLock);

		activateBufferID(otherFile.buffId);
		activateBufferID(buffId);

		switchedFromOtherPair = true;
	}

	nppSettings.setCompareMode();
	nppSettings.updatePluginMenu();
	setCompareMargin(nppData._scintillaMainHandle);
	setCompareMargin(nppData._scintillaSecondHandle);

	if (Settings.UseNavBar && (switchedFromOtherPair || !NavDlg.isVisible()))
		NavDlg.doDialog(true);

	::SetFocus(getCurrentView());
}


void onFileBeforeClose(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	nppSettings.setNormalMode();
	restoreMargin(getView(viewIdFromBuffId(buffId)));

	ScopedIncrementer incr(notificationsLock);

	if (cmpPair->first.buffId == buffId)
		restoreFile(cmpPair->second);
	else
		restoreFile(cmpPair->first);

	compareList.erase(cmpPair);
}


void onFileBeforeSave(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	const int viewId = viewIdFromBuffId(buffId);
	HWND view = getView(viewId);
	const ComparedFile& cmpFile = getFileFromPair(*cmpPair, viewId);

	saveNotifData.reset(new SaveNotificationData(cmpFile));

	saveNotifData->firstVisibleLine = ::SendMessage(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
	saveNotifData->position = ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0);
	saveNotifData->blankSections = removeBlankLines(view, true);
}


void onFileSaved(int buffId)
{
	if (saveNotifData->file.buffId == buffId)
	{
		const int viewId = viewIdFromBuffId(buffId);
		HWND view = getView(viewId);

		if (!saveNotifData->blankSections.empty())
			addBlankLines(view, saveNotifData->blankSections);

		::SendMessage(view, SCI_SETFIRSTVISIBLELINE, saveNotifData->firstVisibleLine, 0);
		::SendMessage(view, SCI_SETSEL, saveNotifData->position, saveNotifData->position);
	}

	saveNotifData.reset();
}

} // anonymous namespace


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


void ViewNavigationBar()
{
	Settings.UseNavBar = !Settings.UseNavBar;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_USE_NAV_BAR]._cmdID,
			(LPARAM)Settings.UseNavBar);

	if (nppSettings.compareMode)
	{
		if (Settings.UseNavBar)
			showNavBar();
		else
			NavDlg.doDialog(false);
	}
}


// Main plugin DLL function
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD  reasonForCall, LPVOID)
 {
	hInstance = hinstDLL;

	switch (reasonForCall)
	{
		case DLL_PROCESS_ATTACH:
			createMenu();
		break;

		case DLL_PROCESS_DETACH:
			deinitPlugin();
		break;

		case DLL_THREAD_ATTACH:
		break;

		case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}


//
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
		// This is used to update the NavBar
		case SCN_PAINTED:
			if (NavDlg.isVisible())
				onSciPaint();
		break;

		// Emulate word-wrap aware vertical scroll sync
		case SCN_UPDATEUI:
			if (nppSettings.compareMode)
				onSciUpdateUI(notifyCode);
		break;

		case NPPN_BUFFERACTIVATED:
			if (!notificationsLock && !compareList.empty())
				onBufferActivated(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILEBEFORECLOSE:
			if (!notificationsLock && !compareList.empty())
				onFileBeforeClose(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILEBEFORESAVE:
			if (!compareList.empty())
				onFileBeforeSave(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILESAVED:
			if ((bool)saveNotifData)
				onFileSaved(notifyCode->nmhdr.idFrom);
		break;

		// This is used to monitor deletion of lines to properly clear their compare markings
        case SCN_MODIFIED:
			if (!notificationsLock && nppSettings.compareMode)
				onSciModified(notifyCode);
        break;

        case SCN_ZOOM:
			if (!notificationsLock && nppSettings.compareMode)
				onSciZoom();
        break;

        case NPPN_WORDSTYLESUPDATED:
        {
			setStyles(Settings);

			if (NavDlg.isVisible())
			{
				NavDlg.SetColor(Settings.ColorSettings);
				NavDlg.CreateBitmap();
			}
        }
        break;

		case NPPN_TBMODIFICATION:
			onToolBarReady();
		break;

		case NPPN_SHUTDOWN:
			saveSettings();
			deinitPlugin();
		break;
	}
}


extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM, LPARAM)
{
	return TRUE;
}


extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
