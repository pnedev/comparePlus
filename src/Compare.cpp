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

const TCHAR sectionName[]			= TEXT("Compare Settings");
const TCHAR addLinesOption[]		= TEXT("Align Matches");
const TCHAR ignoreSpacesOption[]	= TEXT("Include Spaces");
const TCHAR detectMovesOption[]		= TEXT("Detect Move Blocks");

const TCHAR colorsSection[]			= TEXT("Colors");
const TCHAR addedColorOption[]		= TEXT("Added");
const TCHAR removedColorOption[]	= TEXT("Removed");
const TCHAR changedColorOption[]	= TEXT("Changed");
const TCHAR movedColorOption[]		= TEXT("Moved");
const TCHAR highlightColorOption[]	= TEXT("Highlight");
const TCHAR highlightAlphaOption[]	= TEXT("Alpha");
const TCHAR NavBarOption[]			= TEXT("Navigation bar");


/**
 *  \struct
 *  \brief
 */
struct AutoIncrementer
{
	AutoIncrementer(unsigned& useCount) : _useCount(useCount)
	{
		++_useCount;
	}

	~AutoIncrementer()
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

	void store();
	void restore();
	void setCompareMode();
};


/**
 *  \struct
 *  \brief
 */
struct ComparedFile
{
	int		originalSciView;
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
	SaveNotificationData(ComparedFile& savedFile) : file(savedFile) {}

	ComparedFile		file;
	int					firstVisibleLine;
	int					position;
	BlankSections_t		blankSections;
};


using ComparePair_t = std::pair<ComparedFile, ComparedFile>;
using CompareList_t = std::vector<ComparePair_t>;


CompareList_t compareList;
std::unique_ptr<ComparedFile> firstFile;
int fileToCompareCodepage;
NppSettings nppSettings;

unsigned notificationsLock = 0;

long start_line_old;
long visible_line_count_old;

std::unique_ptr<SaveNotificationData> saveNotifData;

// int  tempWindow = -1;
// bool skipAutoReset = false;
// int closingWin = -1;
// HWND closingView = NULL;
// bool panelsOpened = false;

CProgress* progDlg = NULL;
int progMax = 0;
int progCounter = 0;

// TCHAR compareFilePath[MAX_PATH];

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
void Compare();
void ClearCurrentCompare();
void First();
void openMemBlock(const char* memblock, long size);


void loadSettings()
{
	TCHAR iniFilePath[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFilePath), (LPARAM)iniFilePath);

	PathAppend(iniFilePath, TEXT("Compare.ini"));

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

	// Try loading behavior settings, else load default value
	Settings.AddLine      = ::GetPrivateProfileInt(sectionName, addLinesOption, 1, iniFilePath) == 1;
	Settings.IncludeSpace = ::GetPrivateProfileInt(sectionName, ignoreSpacesOption, 0, iniFilePath) == 1;
	Settings.DetectMove   = ::GetPrivateProfileInt(sectionName, detectMovesOption, 1, iniFilePath) == 1;
	Settings.UseNavBar    = ::GetPrivateProfileInt(sectionName, NavBarOption, 0, iniFilePath) == 1;
}


void saveSettings()
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

	::WritePrivateProfileString(sectionName, addLinesOption,
			Settings.AddLine ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, ignoreSpacesOption,
			Settings.IncludeSpace ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, detectMovesOption,
			Settings.DetectMove ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, NavBarOption,
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


void NppSettings::store()
{
	HMENU hMenu = ::GetMenu(nppData._nppHandle);

	syncVScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
	syncHScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
}


void NppSettings::restore()
{
	if (compareMode == false)
		return;

	compareMode = false;

	::SendMessage(nppData._scintillaMainHandle, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);

	HMENU hMenu = ::GetMenu(nppData._nppHandle);

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

	HMENU hMenu = ::GetMenu(nppData._nppHandle);

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


void enableMenuCommands(bool enable)
{
	HMENU hMenu = ::GetMenu(nppData._nppHandle);
	int flag = MF_BYCOMMAND | (enable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));

	::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, flag);

	if (enable || (bool)firstFile)
		::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_CURRENT]._cmdID, MF_BYCOMMAND | MF_ENABLED);
	else
		::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_CURRENT]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
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


void progressFillCompareInfo()
{
	ComparePair_t& cmpPair = compareList[compareList.size() - 1];

	TCHAR* first = ::PathFindFileName(cmpPair.first.name);
	TCHAR* second = ::PathFindFileName(cmpPair.second.name);

	TCHAR msg[256];

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
	HWND hwnd = GetFocus();

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


// HWND openTempFile()
// {
	// char original[MAX_PATH];

	// ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(original), (LPARAM)original);
	// HWND originalwindow = getCurrentView();

	// LRESULT curBuffer = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	// LangType curLang = (LangType)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE, curBuffer, 0);

	// int result = ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
	// HWND window = getCurrentView();
	// int win = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);

	// if (result == 0 || win != tempWindow)
	// {
		// ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);
		// ::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)compareFilePath);
		// tempWindow = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);

		// curBuffer = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		// ::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, curBuffer, curLang);
	// }

	// if (originalwindow == window)
	// {
		// ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)original);
		// skipAutoReset = true;
		// ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		// skipAutoReset = false;
		// ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
		// panelsOpened = true;
	// }

	// result=::SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE, 0, (LPARAM)original);

	// window = getOtherView();

	// int pointer = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);
	// if (tempWindow != pointer)
	// {
		// window = getCurrentView();
		// pointer = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);
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


bool doCompare()
{
	std::vector<int> lineNum1;
	const DocLines_t doc1 = getAllLines(nppData._scintillaMainHandle, lineNum1);
	int doc1Length = doc1.size();

	if (doc1Length == 1 && doc1[0][0] == 0)
	{
		ComparePair_t& cmpPair = compareList[compareList.size() - 1];
		TCHAR msg[MAX_PATH + 64];

		_sntprintf_s(msg, _countof(msg), _TRUNCATE,
				TEXT("File\n\"%s\"\n is empty, nothing to compare."), cmpPair.first.name);
		::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin"), MB_OK);

		return false;
	}

	std::vector<int> lineNum2;
	const DocLines_t doc2 = getAllLines(nppData._scintillaSecondHandle, lineNum2);
	int doc2Length = doc2.size();

	if (doc2Length == 1 && doc2[0][0] == 0)
	{
		ComparePair_t& cmpPair = compareList[compareList.size() - 1];
		TCHAR msg[MAX_PATH + 64];

		_sntprintf_s(msg, _countof(msg), _TRUNCATE,
				TEXT("File\n\"%s\"\n is empty, nothing to compare."), cmpPair.second.name);
		::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin"), MB_OK);

		return false;
	}

	progressOpen(TEXT("Computing hashes..."));

	std::vector<unsigned int> doc1Hashes = computeHashes(doc1, Settings.IncludeSpace);
	std::vector<unsigned int> doc2Hashes = computeHashes(doc2, Settings.IncludeSpace);

	if (isCompareCancelled())
		return false;

	progressFillCompareInfo();

	std::vector<diff_edit> diff = DiffCalc<unsigned int>(doc1Hashes, doc2Hashes)();

	if (isCompareCancelled())
		return false;

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
		return false;

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
		return false;

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
							addBlankSection(nppData._scintillaSecondHandle, off + doc2Offset, length);
							doc2Offset += length;
							off = doc1Changes[i].altLocation;
							length = 1;
						}
					break;
				}
			}

			addBlankSection(nppData._scintillaSecondHandle, off + doc2Offset, length);

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
							addBlankSection(nppData._scintillaMainHandle, off + doc1Offset, length);
							doc1Offset += length;
							off = doc2Changes[i].altLocation;
							length = 1;
						}
					break;
				}
			}

			addBlankSection(nppData._scintillaMainHandle, off + doc1Offset, length);
		}
	}

	if (isCompareCancelled())
		return false;

	progressClose();

	if (!diff.empty())
	{
		if (!different)
		{
			ComparePair_t& cmpPair = compareList[compareList.size() - 1];
			TCHAR msg[2 * MAX_PATH + 64];

			_sntprintf_s(msg, _countof(msg), _TRUNCATE,
					TEXT("The files\n\"%s\"\nand\n\"%s\" match.\n\nClose compared files?\n"),
					cmpPair.first.name, cmpPair.second.name);
			if (::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin: Files Match"),
					MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			{
				AutoIncrementer autoIncr(notificationsLock);

				nppSettings.restore();

				::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
				::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SWITCHTO_OTHER_VIEW);
				::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
				::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SWITCHTO_OTHER_VIEW);

				compareList.pop_back();
			}

			return false;
		}

		::SendMessage(nppData._scintillaMainHandle, SCI_SHOWLINES, 0, 1);
		::SendMessage(nppData._scintillaSecondHandle, SCI_SHOWLINES, 0, 1);

		First();
	}

	return true;
}


bool prepareAndRunCompare()
{
	if (compareList.empty())
		nppSettings.store();

	nppSettings.setCompareMode();

	int RODoc1 = ::SendMessage(nppData._scintillaMainHandle, SCI_GETREADONLY, 0, 0);
	// Remove read-only attribute
	if (RODoc1)
		::SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, false, 0);

	int RODoc2 = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETREADONLY, 0, 0);
	// Remove read-only attribute
	if (RODoc2)
		::SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, false, 0);

	setStyles(Settings);

	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);

	// Compare files (return true if files differ)
	bool result = false;

	try
	{
		result = doCompare();
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

	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);

	// Restore previous read-only attribute
	if (RODoc1)
		::SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, true, 0);

	if (RODoc2)
		::SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, true, 0);

	return result;
}


bool isAlreadyCompared(int buffId)
{
	if (!compareList.empty())
	{
		CompareList_t::iterator cmpPair = getCompare(buffId);
		if (cmpPair != compareList.end())
		{
			TCHAR msg[2 * MAX_PATH + 128];

			_sntprintf_s(msg, _countof(msg), _TRUNCATE,
				TEXT("File\n\"%s\"\nis currently compared to\n\"%s\"\n\nPlease close the current compare first."),
				(buffId == cmpPair->first.buffId) ? cmpPair->first.name : cmpPair->second.name,
				(buffId == cmpPair->first.buffId) ? cmpPair->second.name : cmpPair->first.name);

			::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin"), MB_OK | MB_ICONINFORMATION);

			return true;
		}
	}

	return false;
}


bool createComparePair()
{
	ComparePair_t cmpPair;

	cmpPair.first = *firstFile;
	firstFile.reset();

	cmpPair.second.originalSciView = getCurrentViewId();
	cmpPair.second.buffId = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);

	if (isAlreadyCompared(cmpPair.second.buffId))
	{
		activateBufferID(cmpPair.first.buffId);
		return false;
	}

	const bool moveToOtherViewNeeded = (cmpPair.first.originalSciView == cmpPair.second.originalSciView);

	if (moveToOtherViewNeeded && cmpPair.second.originalSciView == MAIN_VIEW)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);

	cmpPair.second.sciDoc = ::SendMessage(getCurrentView(), SCI_GETDOCPOINTER, 0, 0);
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH,
			_countof(cmpPair.second.name), (LPARAM)cmpPair.second.name);

	activateBufferID(cmpPair.first.buffId);

	if (moveToOtherViewNeeded && cmpPair.first.originalSciView == SUB_VIEW)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);

	compareList.push_back(cmpPair);

	return true;
}


void restoreComparedFile(const ComparedFile& comparedFile)
{
	const int sciViewId = viewIdFromBuffId(comparedFile.buffId);
	HWND hSci = getView(sciViewId);

	activateBufferID(comparedFile.buffId);

	// Remove read-only attribute
	const int RODoc = ::SendMessage(hSci, SCI_GETREADONLY, 0, 0);
	if (RODoc)
		::SendMessage(hSci, SCI_SETREADONLY, false, 0);

	clearWindow(hSci);

	// Restore previous read-only attribute
	if (RODoc)
		::SendMessage(hSci, SCI_SETREADONLY, true, 0);

	if (sciViewId != comparedFile.originalSciView)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
}


void clearComparePair(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);

	if (cmpPair == compareList.end())
		return;

	AutoIncrementer autoIncr(notificationsLock);

	if (cmpPair->first.buffId == buffId)
	{
		restoreComparedFile(cmpPair->second);
		restoreComparedFile(cmpPair->first);
	}
	else
	{
		restoreComparedFile(cmpPair->first);
		restoreComparedFile(cmpPair->second);
	}

	compareList.erase(cmpPair);

	::SetFocus(getCurrentView());
}


bool SelectFirstFile()
{
	firstFile.reset(new ComparedFile);

	firstFile->buffId = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	if (isAlreadyCompared(firstFile->buffId))
	{
		firstFile.reset();
		return false;
	}

	HWND currView = getCurrentView();

	firstFile->originalSciView = getCurrentViewId();
	firstFile->sciDoc = ::SendMessage(currView, SCI_GETDOCPOINTER, 0, 0);
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(firstFile->name), (LPARAM)firstFile->name);

	fileToCompareCodepage = SendMessage(currView, SCI_GETCODEPAGE, 0, 0);

	return true;
}


void SelectFirstToCompare()
{
	SelectFirstFile();
	::EnableMenuItem(::GetMenu(nppData._nppHandle), funcItem[CMD_CLEAR_CURRENT]._cmdID, MF_BYCOMMAND | MF_ENABLED);
}


void Compare()
{
	AutoIncrementer autoIncr(notificationsLock);

	bool autoSelectSecond = true;

	if (!firstFile)
	{
		if (!SelectFirstFile())
			return;
	}
	else
	{
		bool firstValid = false;

		// Check if the first file is still open
		const int buffLen = ::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, firstFile->buffId, 0);
		if (buffLen > 0)
		{
			std::vector<TCHAR> file(buffLen + 1, 0);
			::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, firstFile->buffId, (LPARAM)file.data());
			if (!_tcscmp(file.data(), firstFile->name))
				firstValid = true;
		}

		// Detect if the first file is moved to the other view
		if (firstValid && firstFile->originalSciView != viewIdFromBuffId(firstFile->buffId))
			firstValid = false;

		if (firstValid)
			autoSelectSecond = false;
		else if (!SelectFirstFile()) // First file is invalid - reselect it
			return;
	}

	if (autoSelectSecond)
	{
		if (isSingleView())
		{
			int viewId = getCurrentViewId();
			viewId = (viewId == MAIN_VIEW) ? PRIMARY_VIEW : SECOND_VIEW;

			if (::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, viewId) < 2)
			{
				::MessageBox(nppData._nppHandle, TEXT("Not enough opened files to run compare."),
						TEXT("Compare Plugin"), MB_OK);
				firstFile.reset();
				return;
			}

			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_TAB_PREV);
		}
		else
		{
			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SWITCHTO_OTHER_VIEW);
		}
	}
	else
	{
		// Compare to self?
		if (firstFile->buffId == ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0))
		{
			firstFile.reset();
			::MessageBox(nppData._nppHandle, TEXT("Trying to compare file to itself - ignored."),
					TEXT("Compare Plugin"), MB_OK | MB_ICONINFORMATION);
			return;
		}
	}

	// Warn about encoding mismatches as that might compromise the compare
	if (fileToCompareCodepage != SendMessage(getCurrentView(), SCI_GETCODEPAGE, 0, 0))
	{
		if (::MessageBox(nppData._nppHandle,
			TEXT("Trying to compare files with different encodings - \n")
			TEXT("the result might be inaccurate and misleading.\n\n")
			TEXT("Compare anyway?"), TEXT("Compare Plugin"), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
		{
			activateBufferID(firstFile->buffId);
			firstFile.reset();
			return;
		}
	}

	if (!createComparePair())
		return;

	if (prepareAndRunCompare())
	{
		enableMenuCommands(true);

		if (Settings.UseNavBar)
			showNavBar();
	}
	else
	{
		ClearCurrentCompare();
	}
}


void ClearCurrentCompare()
{
	firstFile.reset();

	enableMenuCommands(false);

	if (!nppSettings.compareMode)
		return;

	if (NavDlg.isVisible())
		NavDlg.doDialog(false);

	nppSettings.restore();

	const int buffId = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	clearComparePair(buffId);


	// if (tempWindow != -1)
	// {
		// if (doc1 != closingWin)
		// {
			// ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
			// HWND window = getCurrentView();
			// int tempPointer = ::SendMessage(window, SCI_GETDOCPOINTER, 0, 0);
			// if (tempPointer == tempWindow)
			// {
				// ::SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, 0);
				// skipAutoReset = true;
				// ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
				// skipAutoReset = false;
			// }
		// }
		// tempWindow = -1;
		// LRESULT ROTemp = RODoc1;
		// RODoc1 = RODoc2;
		// RODoc2 = ROTemp;
	// }

	// closingWin = -1;
	// closingView = NULL;
}


void ClearAllCompares()
{
	firstFile.reset();

	const int buffId = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);

	enableMenuCommands(false);

	if (NavDlg.isVisible())
		NavDlg.doDialog(false);

	nppSettings.restore();

	AutoIncrementer autoIncr(notificationsLock);

	for (int i = compareList.size() - 1; i >= 0; --i)
	{
		restoreComparedFile(compareList[i].first);
		restoreComparedFile(compareList[i].second);
	}

	compareList.clear();

	activateBufferID(buffId);
}


void CompareLocal()
{
	TCHAR file[MAX_PATH];
	::SendMessage(nppData._nppHandle,NPPM_GETCURRENTDIRECTORY,0,(LPARAM)file);

	if (file[0] != 0)
		::SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	openFile(file);
}


void CompareSvnBase()
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


void CompareGitBase()
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
	HMENU hMenu = ::GetMenu(nppData._nppHandle);
	Settings.AddLine = !Settings.AddLine;

	if (hMenu)
		::CheckMenuItem(hMenu, funcItem[CMD_ALIGN_MATCHES]._cmdID,
				MF_BYCOMMAND | (Settings.AddLine ? MF_CHECKED : MF_UNCHECKED));
}


void IncludeSpacing()
{
	HMENU hMenu = ::GetMenu(nppData._nppHandle);
	Settings.IncludeSpace = !Settings.IncludeSpace;

	if (hMenu)
		::CheckMenuItem(hMenu, funcItem[CMD_IGNORE_SPACING]._cmdID,
				MF_BYCOMMAND | (Settings.IncludeSpace ? MF_CHECKED : MF_UNCHECKED));
}


void DetectMoves()
{
	HMENU hMenu = ::GetMenu(nppData._nppHandle);
	Settings.DetectMove = !Settings.DetectMove;

	if (hMenu)
		::CheckMenuItem(hMenu, funcItem[CMD_DETECT_MOVES]._cmdID,
				MF_BYCOMMAND | (Settings.DetectMove ? MF_CHECKED : MF_UNCHECKED));
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

		const int nextLine = ::SendMessage(CurView, SCI_MARKERNEXT, 0, sci_search_mask );
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
	funcItem[CMD_SELECT_FIRST]._pFunc				= SelectFirstToCompare;
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

	enableMenuCommands(false);

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
	CompareList_t::iterator cmpPair = getCompare(::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0));
	if (cmpPair == compareList.end())
		return;

	if (notifyCode->modificationType == (SC_MOD_BEFOREDELETE | SC_PERFORMED_USER))
	{
		AutoIncrementer autoIncr(notificationsLock);

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
	CompareList_t::iterator cmpPair = getCompare(::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0));
	if (cmpPair == compareList.end())
		return;

	AutoIncrementer autoIncr(notificationsLock);

	// sync both views zoom
	int zoom = ::SendMessage(getCurrentView(), SCI_GETZOOM, 0, 0);
	::SendMessage(getOtherView(), SCI_SETZOOM, zoom, 0);
}


void onBufferActivated(SCNotification *notifyCode)
{
	CompareList_t::iterator cmpPair = getCompare(notifyCode->nmhdr.idFrom);

	if (cmpPair == compareList.end())
	{
		enableMenuCommands(false);

		if (NavDlg.isVisible())
			NavDlg.doDialog(false);

		nppSettings.restore();
	}
	else
	{
		// Compared file moved to other view? -> clear compare
		if (viewIdFromBuffId(cmpPair->first.buffId) == viewIdFromBuffId(cmpPair->second.buffId))
		{
			enableMenuCommands(false);

			if (NavDlg.isVisible())
				NavDlg.doDialog(false);

			nppSettings.restore();

			clearComparePair(notifyCode->nmhdr.idFrom);
			return;
		}

		bool switchedFromOtherPair = false;
		const int otherViewId = getOtherViewId();
		ComparedFile& otherFile = getFileFromPair(*cmpPair, otherViewId);

		// When compared file is activated make sure its corresponding pair file is
		// also active in the other view
		if (::SendMessage(getView(otherViewId), SCI_GETDOCPOINTER, 0, 0) != otherFile.sciDoc)
		{
			AutoIncrementer autoIncr(notificationsLock);

			activateBufferID(otherFile.buffId);
			activateBufferID(notifyCode->nmhdr.idFrom);

			switchedFromOtherPair = true;
		}

		enableMenuCommands(true);

		if (Settings.UseNavBar && (switchedFromOtherPair || !NavDlg.isVisible()))
			NavDlg.doDialog(true);

		nppSettings.setCompareMode();
	}

	::SetFocus(getCurrentView());
}


void onFileBeforeClose(SCNotification *notifyCode)
{
	CompareList_t::iterator cmpPair = getCompare(notifyCode->nmhdr.idFrom);
	if (cmpPair == compareList.end())
		return;

	enableMenuCommands(false);

	if (NavDlg.isVisible())
		NavDlg.doDialog(false);

	nppSettings.restore();

	AutoIncrementer autoIncr(notificationsLock);

	if (cmpPair->first.buffId == (int)notifyCode->nmhdr.idFrom)
		restoreComparedFile(cmpPair->second);
	else
		restoreComparedFile(cmpPair->first);

	compareList.erase(cmpPair);

	::SetFocus(getCurrentView());
}


void onFileBeforeSave(SCNotification *notifyCode)
{
	CompareList_t::iterator cmpPair = getCompare(notifyCode->nmhdr.idFrom);
	if (cmpPair == compareList.end())
		return;

	const int viewId = viewIdFromBuffId(notifyCode->nmhdr.idFrom);
	HWND view = getView(viewId);
	ComparedFile& cmpFile = getFileFromPair(*cmpPair, viewId);

	saveNotifData.reset(new SaveNotificationData(cmpFile));

	saveNotifData->firstVisibleLine = ::SendMessage(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
	saveNotifData->position = ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0);
	saveNotifData->blankSections = removeBlankLines(view, true);
}


void onFileSaved(SCNotification *notifyCode)
{
	if (saveNotifData->file.buffId == (int)notifyCode->nmhdr.idFrom)
	{
		const int viewId = viewIdFromBuffId(notifyCode->nmhdr.idFrom);
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
	HMENU hMenu = ::GetMenu(nppData._nppHandle);
	Settings.UseNavBar = !Settings.UseNavBar;

	if (hMenu)
		::CheckMenuItem(hMenu, funcItem[CMD_USE_NAV_BAR]._cmdID,
				MF_BYCOMMAND | (Settings.UseNavBar ? MF_CHECKED : MF_UNCHECKED));

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
				onBufferActivated(notifyCode);
		break;

		case NPPN_FILEBEFORECLOSE:
			if (!notificationsLock && !compareList.empty())
				onFileBeforeClose(notifyCode);
		break;

		case NPPN_FILEBEFORESAVE:
			if (!compareList.empty())
				onFileBeforeSave(notifyCode);
		break;

		case NPPN_FILESAVED:
			if ((bool)saveNotifData)
				onFileSaved(notifyCode);
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
