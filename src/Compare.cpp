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

#include "Compare.h"
#include "NPPHelpers.h"
#include "ScmHelper.h"
#include "CProgress.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "NavDialog.h"
#include "diff.h"
#include "Engine.h"


NppData nppData;
UserSettings Settings;


const TCHAR UserSettings::mainSection[]				= TEXT("Main");
const TCHAR UserSettings::oldIsFirstSetting[]		= TEXT("Old is First");
const TCHAR UserSettings::oldFileOnLeftSetting[]	= TEXT("Old on Left");
const TCHAR UserSettings::gotoFirstDiffSetting[]	= TEXT("Go to First Diff");
const TCHAR UserSettings::alignMatchesSetting[]		= TEXT("Align Matches");
const TCHAR UserSettings::ignoreSpacesSetting[]		= TEXT("Include Spaces");
const TCHAR UserSettings::detectMovesSetting[]		= TEXT("Detect Moved Blocks");
const TCHAR UserSettings::navBarSetting[]			= TEXT("Navigation bar");
const TCHAR UserSettings::colorsSection[]			= TEXT("Colors");
const TCHAR UserSettings::addedColorSetting[]		= TEXT("Added");
const TCHAR UserSettings::removedColorSetting[]		= TEXT("Removed");
const TCHAR UserSettings::changedColorSetting[]		= TEXT("Changed");
const TCHAR UserSettings::movedColorSetting[]		= TEXT("Moved");
const TCHAR UserSettings::highlightColorSetting[]	= TEXT("Highlight");
const TCHAR UserSettings::highlightAlphaSetting[]	= TEXT("Alpha");


void UserSettings::load()
{
	TCHAR iniFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFile), (LPARAM)iniFile);

	::PathAppend(iniFile, TEXT("Compare.ini"));

	OldFileIsFirst	= ::GetPrivateProfileInt(mainSection, oldIsFirstSetting, 1, iniFile) == 1;
	OldFileViewId	=
			::GetPrivateProfileInt(mainSection, oldFileOnLeftSetting, 1, iniFile) == 1 ? MAIN_VIEW : SUB_VIEW;
	GotoFirstDiff	= ::GetPrivateProfileInt(mainSection, gotoFirstDiffSetting, 0, iniFile) == 1;

	AddLine      = ::GetPrivateProfileInt(mainSection, alignMatchesSetting,	1, iniFile) == 1;
	IncludeSpace = ::GetPrivateProfileInt(mainSection, ignoreSpacesSetting,	0, iniFile) == 1;
	DetectMove   = ::GetPrivateProfileInt(mainSection, detectMovesSetting,	1, iniFile) == 1;
	UseNavBar    = ::GetPrivateProfileInt(mainSection, navBarSetting,		0, iniFile) == 1;

	colors.added	 = ::GetPrivateProfileInt(colorsSection, addedColorSetting,		DEFAULT_ADDED_COLOR, iniFile);
	colors.deleted	 = ::GetPrivateProfileInt(colorsSection, removedColorSetting,	DEFAULT_DELETED_COLOR, iniFile);
	colors.changed	 = ::GetPrivateProfileInt(colorsSection, changedColorSetting,	DEFAULT_CHANGED_COLOR, iniFile);
	colors.moved	 = ::GetPrivateProfileInt(colorsSection, movedColorSetting,		DEFAULT_MOVED_COLOR, iniFile);
	colors.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorSetting,	DEFAULT_HIGHLIGHT_COLOR, iniFile);
	colors.alpha	 = ::GetPrivateProfileInt(colorsSection, highlightAlphaSetting,	DEFAULT_HIGHLIGHT_ALPHA, iniFile);
}


void UserSettings::save()
{
	TCHAR iniFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFile), (LPARAM)iniFile);
	::PathAppend(iniFile, TEXT("Compare.ini"));

	::WritePrivateProfileString(mainSection, oldIsFirstSetting,
			OldFileIsFirst ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, oldFileOnLeftSetting,
			OldFileViewId == MAIN_VIEW ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, gotoFirstDiffSetting,
			GotoFirstDiff ? TEXT("1") : TEXT("0"), iniFile);

	::WritePrivateProfileString(mainSection, alignMatchesSetting, AddLine		? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreSpacesSetting, IncludeSpace	? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, detectMovesSetting,  DetectMove	? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, navBarSetting,		  UseNavBar		? TEXT("1") : TEXT("0"), iniFile);

	TCHAR buffer[64];

	_itot_s(colors.added, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addedColorSetting, buffer, iniFile);

	_itot_s(colors.deleted, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, removedColorSetting, buffer, iniFile);

	_itot_s(colors.changed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedColorSetting, buffer, iniFile);

	_itot_s(colors.moved, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movedColorSetting, buffer, iniFile);

	_itot_s(colors.highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightColorSetting, buffer, iniFile);

	_itot_s(colors.alpha, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightAlphaSetting, buffer, iniFile);
}


namespace // anonymous namespace
{

const TCHAR PLUGIN_NAME[] = TEXT("Compare");


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

	static HWND	hNppToolbar;

	static BOOL CALLBACK enumWindowsCB(HWND hwnd, LPARAM lParam);

	bool	compareMode;
	bool	syncVScroll;
	bool	syncHScroll;

	void updatePluginMenu() const;
	void save();
	void setNormalMode();
	void setCompareMode();
};


HWND NppSettings::hNppToolbar = NULL;


BOOL CALLBACK NppSettings::enumWindowsCB(HWND hwnd, LPARAM lParam)
{
	TCHAR winClassName[64];

	::GetClassName(hwnd, winClassName, _countof(winClassName));

	if (!_tcscmp(winClassName, TOOLBARCLASSNAME))
	{
		hNppToolbar = hwnd;
		return FALSE;
	}

	return TRUE;
}


/**
 *  \struct
 *  \brief
 */
struct DeletedSection
{
	DeletedSection(int line, int len) : startLine(line)
	{
		markers.resize(len, 0);
	}

	int					startLine;
	std::vector<int>	markers;
};


using DeletedSections_t = std::vector<DeletedSection>;


/**
 *  \struct
 *  \brief
 */
struct ComparedFile
{
	void initFromCurrent(bool newFile);
	void updateFromCurrent();
	void updateView();
	void restore();

	bool	isNew;
	int		originalViewId;
	int		originalPos;
	int		compareViewId;
	int		buffId;
	int		sciDoc;
	TCHAR	name[MAX_PATH];

	// Members below are for user actions generated data
	DeletedSections_t	deletedSections;
};


/**
 *  \struct
 *  \brief
 */
struct ComparedPair
{
	ComparedFile& getFileByBuffId(int buffId);
	ComparedFile& getOtherFileByBuffId(int buffId);
	ComparedFile& getFileBySciDoc(int sciDoc);
	ComparedFile& getOldFile();
	ComparedFile& getNewFile();
	void positionFiles();

	ComparedFile file[2];
};


using CompareList_t = std::vector<ComparedPair>;


enum CompareResult_t
{
	COMPARE_ERROR = 0,
	COMPARE_CANCELLED,
	FILES_MATCH,
	FILES_DIFFER
};


/**
 *  \class
 *  \brief
 */
class DelayedWork
{
public:
	using workFunc_t = void(*)(int);

	static bool post(workFunc_t work, int buffId, UINT delay_ms);
	static void cancel();

private:
	static DelayedWork& instance()
	{
		static DelayedWork inst;
		return inst;
	}

	DelayedWork() : _timerId(0), _buffId(0) {}
	~DelayedWork()
	{
		if (_timerId)
			::KillTimer(NULL, _timerId);
	}

	static VOID CALLBACK timerCB(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	UINT_PTR	_timerId;
	workFunc_t	_work;
	int			_buffId;
};


bool DelayedWork::post(workFunc_t work, int buffId, UINT delay_ms)
{
	DelayedWork& inst = instance();

	inst._work = work;
	inst._buffId = buffId;
	inst._timerId = ::SetTimer(NULL, 0, delay_ms, timerCB);

	return (inst._timerId != 0);
}


void DelayedWork::cancel()
{
	DelayedWork& inst = instance();

	if (inst._timerId)
	{
		::KillTimer(NULL, inst._timerId);
		inst._timerId = 0;
	}
}


VOID CALLBACK DelayedWork::timerCB(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	cancel();

	DelayedWork& inst = instance();

	inst._work(inst._buffId);
}


/**
 *  \struct
 *  \brief
 */
struct SaveNotificationData
{
	SaveNotificationData(int buffId) : fileBuffId(buffId) {}

	int					fileBuffId;
	int					firstVisibleLine;
	int					position;
	BlankSections_t		blankSections;
};


CompareList_t compareList;
std::unique_ptr<ComparedPair> newPair;
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

AboutDialog   	AboutDlg;
SettingsDialog	SettingsDlg;
NavDialog     	NavDlg;

toolbarIcons  tbPrev;
toolbarIcons  tbNext;
toolbarIcons  tbFirst;
toolbarIcons  tbLast;

HINSTANCE hInstance;
FuncItem funcItem[NB_MENU_COMMANDS] = { 0 };


// Declare local functions that appear before they are defined
void Compare();
void ClearActiveCompare();
void First();
void openMemBlock(const char* memblock, long size);
void onBufferActivated(int buffId);
void onBufferActivatedDelayed(int buffId);


void NppSettings::updatePluginMenu() const
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
	int flag = MF_BYCOMMAND | (compareMode ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));

	::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ACTIVE]._cmdID, flag);

	if (hNppToolbar == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, 0);

	if (hNppToolbar)
	{
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_PREV]._cmdID, compareMode);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_NEXT]._cmdID, compareMode);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_FIRST]._cmdID, compareMode);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_LAST]._cmdID, compareMode);
	}
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


void ComparedFile::initFromCurrent(bool newFile)
{
	isNew = newFile;
	buffId = getCurrentBuffId();
	originalViewId = getCurrentViewId();
	originalPos = posFromBuffId(buffId);
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(name), (LPARAM)name);

	updateFromCurrent();
	updateView();
}


void ComparedFile::updateFromCurrent()
{
	sciDoc = getDocId(getCurrentView());
}


void ComparedFile::updateView()
{
	compareViewId = isNew ? ((Settings.OldFileViewId == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW) : Settings.OldFileViewId;
}


void ComparedFile::restore()
{
	if (buffId != getCurrentBuffId())
		activateBufferID(buffId);

	clearWindow(getCurrentView());

	if (compareViewId != originalViewId)
	{
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
	}
}


ComparedFile& ComparedPair::getFileByBuffId(int buffId)
{
	return (file[0].buffId == buffId) ? file[0] : file[1];
}


ComparedFile& ComparedPair::getOtherFileByBuffId(int buffId)
{
	return (file[0].buffId == buffId) ? file[1] : file[0];
}


ComparedFile& ComparedPair::getFileBySciDoc(int sciDoc)
{
	return (file[0].sciDoc == sciDoc) ? file[0] : file[1];
}


ComparedFile& ComparedPair::getOldFile()
{
	return file[0].isNew ? file[1] : file[0];
}


ComparedFile& ComparedPair::getNewFile()
{
	return file[0].isNew ? file[0] : file[1];
}


void ComparedPair::positionFiles()
{
	ComparedFile& oldFile = getOldFile();
	ComparedFile& newFile = getNewFile();

	if (viewIdFromBuffId(oldFile.buffId) != oldFile.compareViewId)
	{
		if (oldFile.buffId != getCurrentBuffId())
			activateBufferID(oldFile.buffId);
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		oldFile.updateFromCurrent();
	}

	if (viewIdFromBuffId(newFile.buffId) != newFile.compareViewId)
	{
		if (newFile.buffId != getCurrentBuffId())
			activateBufferID(newFile.buffId);
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		newFile.updateFromCurrent();
	}

	if (oldFile.sciDoc != getDocId(getView(oldFile.compareViewId)))
		activateBufferID(oldFile.buffId);

	if (newFile.sciDoc != getDocId(getView(newFile.compareViewId)))
		activateBufferID(newFile.buffId);
}


CompareList_t::iterator getCompare(int buffId)
{
	for (CompareList_t::iterator it = compareList.begin(); it < compareList.end(); ++it)
	{
		if (it->file[0].buffId == buffId || it->file[1].buffId == buffId)
			return it;
	}

	return compareList.end();
}


CompareList_t::iterator getCompareBySciDoc(int sciDoc)
{
	for (CompareList_t::iterator it = compareList.begin(); it < compareList.end(); ++it)
	{
		if (it->file[0].sciDoc == sciDoc || it->file[1].sciDoc == sciDoc)
			return it;
	}

	return compareList.end();
}


void resetCompareView(HWND view)
{
	if (!::IsWindowVisible(view))
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(view));
	if (cmpPair != compareList.end())
		setCompareView(view);
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


void progressFillCompareInfo(CompareList_t::iterator& cmpPair)
{
	const TCHAR* newName = ::PathFindFileName(cmpPair->getNewFile().name);
	const TCHAR* oldName = ::PathFindFileName(cmpPair->getOldFile().name);

	TCHAR msg[MAX_PATH];
	_sntprintf_s(msg, _countof(msg), _TRUNCATE, TEXT("Comparing \"%s\" vs. \"%s\"..."), newName, oldName);

	progDlg->SetInfo(msg);
}


void progressClose()
{
	if (progDlg)
	{
		::EnableWindow(nppData._nppHandle, TRUE);
		::SetForegroundWindow(nppData._nppHandle);

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
	NavDlg.SetColors(Settings.colors);
	NavDlg.doDialog(true);

	// Restore N++ focus
	::SetFocus(hwnd);
}


void jumpChangedLines(bool direction)
{
	HWND view = getCurrentView();

	const int sci_search_mask = (1 << MARKER_MOVED_LINE) |
								(1 << MARKER_CHANGED_LINE) |
								(1 << MARKER_ADDED_LINE) |
								(1 << MARKER_REMOVED_LINE) |
								(1 << MARKER_BLANK_LINE);

	const int lineMax = ::SendMessage(view, SCI_GETLINECOUNT, 0, 0);
	int lineStart = ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0);
	lineStart = ::SendMessage(view, SCI_LINEFROMPOSITION, lineStart, 0);

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

		nextLine = ::SendMessage(view, sci_marker_direction, currLine, sci_search_mask);

		if (nextLine < 0)
		{
			currLine = (direction) ? (0) : (lineMax);
			nextLine = ::SendMessage(view, sci_marker_direction, currLine, sci_search_mask);
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

	::SendMessage(view, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
	::SendMessage(view, SCI_GOTOLINE, nextLine, 0);
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

		// ClearActiveCompare();
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


void clearComparePair(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	nppSettings.setNormalMode();

	ScopedIncrementer incr(notificationsLock);

	cmpPair->getOtherFileByBuffId(buffId).restore();
	cmpPair->getFileByBuffId(buffId).restore();

	compareList.erase(cmpPair);

	resetCompareView(getOtherView());
}


void closeComparePair(CompareList_t::iterator& cmpPair)
{
	HWND currentView = getCurrentView();

	nppSettings.setNormalMode();
	setNormalView(nppData._scintillaMainHandle);
	setNormalView(nppData._scintillaSecondHandle);

	ScopedIncrementer incr(notificationsLock);

	activateBufferID(cmpPair->file[0].buffId);
	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
	activateBufferID(cmpPair->file[1].buffId);
	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);

	compareList.erase(cmpPair);

	if (::IsWindowVisible(currentView))
		::SetFocus(currentView);

	onBufferActivatedDelayed(getCurrentBuffId());
	resetCompareView(getOtherView());
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


bool isFileCompared(HWND view)
{
	const int sciDoc = getDocId(view);

	CompareList_t::iterator cmpPair = getCompareBySciDoc(sciDoc);
	if (cmpPair != compareList.end())
	{
		const TCHAR* fname = ::PathFindFileName(cmpPair->getFileBySciDoc(sciDoc).name);

		TCHAR msg[MAX_PATH];
		_sntprintf_s(msg, _countof(msg), _TRUNCATE,
				TEXT("File \"%s\" is already compared - operation ignored."), fname);
		::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin"), MB_OK);

		return true;
	}

	return false;
}


bool setFirst(bool newFile)
{
	HWND view = getCurrentView();

	if (isFileEmpty(view) || isFileCompared(view))
		return false;

	newPair.reset(new ComparedPair);

	newPair->file[0].initFromCurrent(newFile);

	firstFileCodepage = ::SendMessage(view, SCI_GETCODEPAGE, 0, 0);

	// Enable ClearActiveCompare command to be able to clear the first file that was just set
	::EnableMenuItem((HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0),
			funcItem[CMD_CLEAR_ACTIVE]._cmdID, MF_BYCOMMAND | MF_ENABLED);

	return true;
}


bool initNewCompare()
{
	bool firstIsSet = (bool)newPair;

	if (firstIsSet)
	{
		if (isFileEmpty(getView(newPair->file[0].originalViewId)))
			return false;

		// Compare to self?
		if (newPair->file[0].buffId == getCurrentBuffId())
			firstIsSet = false;
		else if (isFileEmpty(getCurrentView()))
			return false;
	}

	if (!firstIsSet)
	{
		const bool singleView = isSingleView();
		const bool isNew = singleView ? true : getCurrentViewId() != Settings.OldFileViewId;

		if (!setFirst(isNew))
			return false;

		if (singleView)
		{
			if (getNumberOfFiles(getCurrentViewId()) < 2)
			{
				::MessageBox(nppData._nppHandle, TEXT("Only one file opened - operation ignored."),
						TEXT("Compare Plugin"), MB_OK);
				return false;
			}

			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0,
					Settings.OldFileViewId == MAIN_VIEW ? IDM_VIEW_TAB_PREV : IDM_VIEW_TAB_NEXT);

			if (isFileEmpty(getCurrentView()))
			{
				activateBufferID(newPair->file[0].buffId);
				return false;
			}
		}
		else
		{
			HWND otherView = getOtherView();

			// Check if the file in the other view is compared already
			if (isFileEmpty(otherView) || isFileCompared(otherView))
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
			if (!firstIsSet)
				activateBufferID(newPair->file[0].buffId);

			return false;
		}
	}

	newPair->file[1].initFromCurrent(!newPair->file[0].isNew);

	return true;
}


CompareList_t::iterator addComparePair()
{
	if (compareList.empty())
		nppSettings.save();

	compareList.push_back(*newPair);
	newPair.reset();

	return compareList.end() - 1;
}


CompareResult_t doCompare(CompareList_t::iterator& cmpPair)
{
	HWND view1;
	HWND view2;

	if (Settings.OldFileViewId != MAIN_VIEW)
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
	const int doc1Length = doc1.size();

	if (doc1Length == 1 && doc1[0][0] == 0)
		return COMPARE_CANCELLED;

	std::vector<int> lineNum2;
	const DocLines_t doc2 = getAllLines(view2, lineNum2);
	const int doc2Length = doc2.size();

	if (doc2Length == 1 && doc2[0][0] == 0)
		return COMPARE_CANCELLED;

	progressOpen(TEXT("Computing hashes..."));

	std::vector<unsigned int> doc1Hashes = computeHashes(doc1, Settings.IncludeSpace);
	std::vector<unsigned int> doc2Hashes = computeHashes(doc2, Settings.IncludeSpace);

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

	progressFillCompareInfo(cmpPair);

	std::vector<diff_edit> diff = DiffCalc<unsigned int>(doc1Hashes, doc2Hashes)();
	if (diff.empty())
	{
		progressClose();
		return FILES_MATCH;
	}

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

	const std::size_t diffSize = diff.size();

	int	doc1Changed = 0;
	int	doc2Changed = 0;

	shiftBoundries(diff, doc1Hashes.data(), doc2Hashes.data(), doc1Length, doc2Length);

	if (Settings.DetectMove)
		findMoves(diff, doc1Hashes.data(), doc2Hashes.data());

	// Insert empty lines, count changed lines
	for (unsigned int i = 0; i < diffSize; ++i)
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

		for (unsigned int i = 0; i < diffSize; ++i)
		{
			diff_edit& e = diff[i];
			e.set = i;

			switch (e.op)
			{
				case DIFF_CHANGE1:
				case DIFF_DELETE:
					added = setDiffLines(e, doc1Changes, &doc1Idx, DIFF_DELETE, e.off + doc2Offset);
					doc1Offset += added;
					doc2Offset -= added;
				break;

				case DIFF_CHANGE2:
				case DIFF_INSERT:
					added = setDiffLines(e, doc2Changes, &doc2Idx, DIFF_INSERT, e.off + doc1Offset);
					doc1Offset -= added;
					doc2Offset += added;
				break;
			}
		}
	}

	if ((doc1Changed == 0) && (doc2Changed == 0))
	{
		progressClose();
		return FILES_MATCH;
	}

	if (isCompareCancelled())
		return COMPARE_CANCELLED;

	int textIndex;

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

	if (Settings.AddLine)
	{
		int length = 0;
		int off = 0;
		doc2Offset = 0;

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

	progressClose();

	return FILES_DIFFER;
}


CompareResult_t runCompare(CompareList_t::iterator& cmpPair)
{
	cmpPair->positionFiles();

	nppSettings.setCompareMode();

	ScopedViewUndoCollectionBlocker undoBlock1(nppData._scintillaMainHandle);
	ScopedViewUndoCollectionBlocker undoBlock2(nppData._scintillaSecondHandle);

	ScopedViewWriteEnabler writeEn1(nppData._scintillaMainHandle);
	ScopedViewWriteEnabler writeEn2(nppData._scintillaSecondHandle);

	setStyles(Settings);

	CompareResult_t result = COMPARE_ERROR;

	try
	{
		result = doCompare(cmpPair);
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


void SetAsFirst()
{
	if (!setFirst(!Settings.OldFileIsFirst))
	{
		if ((bool)newPair)
		{
			newPair.reset();

			if (!nppSettings.compareMode)
				nppSettings.updatePluginMenu();
		}
	}
}


void Compare()
{
	ScopedIncrementer incr(notificationsLock);

	const bool doubleView = !isSingleView();
	const int currentBuffId = getCurrentBuffId();

	ViewLocation location;
	bool recompare = false;

	CompareList_t::iterator cmpPair = getCompare(currentBuffId);

	// New compare
	if (cmpPair == compareList.end())
	{
		if (!initNewCompare())
		{
			newPair.reset();
			nppSettings.updatePluginMenu();
			return;
		}

		cmpPair = addComparePair();
	}
	// Re-Compare triggered - clear current results
	else
	{
		recompare = true;

		newPair.reset();

		if (isFileEmpty(nppData._scintillaMainHandle) || isFileEmpty(nppData._scintillaSecondHandle))
		{
			clearComparePair(currentBuffId);
			return;
		}

		location.saveCurrent();

		clearWindow(nppData._scintillaMainHandle);
		clearWindow(nppData._scintillaSecondHandle);

		ComparedFile& oldFile = cmpPair->getOldFile();
		ComparedFile& newFile = cmpPair->getNewFile();

		oldFile.deletedSections.clear();
		newFile.deletedSections.clear();

		oldFile.updateView();
		newFile.updateView();
	}

	switch (runCompare(cmpPair))
	{
		case FILES_DIFFER:
		{
			nppSettings.updatePluginMenu();

			if (Settings.UseNavBar)
				showNavBar();

			if (!Settings.GotoFirstDiff && recompare)
			{
				location.restore();
			}
			else
			{
				if (doubleView)
					activateBufferID(currentBuffId);
				else
					activateBufferID(cmpPair->getNewFile().buffId);

				First();
			}
		}
		break;

		case FILES_MATCH:
		{
			TCHAR msg[2 * MAX_PATH];

			const TCHAR* newName = ::PathFindFileName(cmpPair->getNewFile().name);
			const TCHAR* oldName = ::PathFindFileName(cmpPair->getOldFile().name);

			_sntprintf_s(msg, _countof(msg), _TRUNCATE,
					TEXT("Files \"%s\" and \"%s\" match.\n\nClose compared files?"), newName, oldName);
			if (::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin: Files Match"),
					MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
				closeComparePair(cmpPair);
			else
				ClearActiveCompare();
		}
		break;

		default:
			ClearActiveCompare();
	}
}


void ClearActiveCompare()
{
	newPair.reset();

	if (!nppSettings.compareMode)
		nppSettings.updatePluginMenu();
	else
		clearComparePair(getCurrentBuffId());
}


void ClearAllCompares()
{
	const int buffId = getCurrentBuffId();

	newPair.reset();

	if (!nppSettings.compareMode)
		nppSettings.updatePluginMenu();
	else
		nppSettings.setNormalMode();

	ScopedIncrementer incr(notificationsLock);

	::SetFocus(getOtherView());

	const int otherBuffId = getCurrentBuffId();

	for (int i = compareList.size() - 1; i >= 0; --i)
	{
		compareList[i].file[0].restore();
		compareList[i].file[1].restore();
	}

	compareList.clear();

	if (!isSingleView())
		activateBufferID(otherBuffId);

	activateBufferID(buffId);
}


void LastSaveDiff()
{
	TCHAR file[MAX_PATH];
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)file);

	if (file[0] != 0)
		::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	openFile(file);
}


void SvnDiff()
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

	::MessageBox(nppData._nppHandle, TEXT("Can not locate SVN information."), TEXT("Compare Plugin"), MB_OK);
}


void GitDiff()
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

	::MessageBox(nppData._nppHandle, TEXT("Can not locate GIT information."), TEXT("Compare Plugin"), MB_OK);
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


void OpenSettingsDlg(void)
{
	if (SettingsDlg.doDialog(&Settings) == IDOK)
	{
		Settings.save();

		newPair.reset();

		if (!nppSettings.compareMode)
			nppSettings.updatePluginMenu();

		if (!compareList.empty())
		{
			setStyles(Settings);

			if (NavDlg.isVisible())
			{
				NavDlg.SetColors(Settings.colors);
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
	_tcscpy_s(funcItem[CMD_SET_FIRST]._itemName, nbChar, TEXT("Set as First to Compare"));
	funcItem[CMD_SET_FIRST]._pFunc				= SetAsFirst;
	funcItem[CMD_SET_FIRST]._pShKey				= new ShortcutKey;
	funcItem[CMD_SET_FIRST]._pShKey->_isAlt		= true;
	funcItem[CMD_SET_FIRST]._pShKey->_isCtrl	= true;
	funcItem[CMD_SET_FIRST]._pShKey->_isShift	= false;
	funcItem[CMD_SET_FIRST]._pShKey->_key		= 'F';

	_tcscpy_s(funcItem[CMD_COMPARE]._itemName, nbChar, TEXT("Compare"));
	funcItem[CMD_COMPARE]._pFunc			= Compare;
	funcItem[CMD_COMPARE]._pShKey			= new ShortcutKey;
	funcItem[CMD_COMPARE]._pShKey->_isAlt	= true;
	funcItem[CMD_COMPARE]._pShKey->_isCtrl	= true;
	funcItem[CMD_COMPARE]._pShKey->_isShift	= false;
	funcItem[CMD_COMPARE]._pShKey->_key		= 'C';

	_tcscpy_s(funcItem[CMD_CLEAR_ACTIVE]._itemName, nbChar, TEXT("Clear Active Compare"));
	funcItem[CMD_CLEAR_ACTIVE]._pFunc				= ClearActiveCompare;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey 				= new ShortcutKey;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isAlt 		= true;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isCtrl		= true;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isShift	= false;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_key 		= 'X';

	_tcscpy_s(funcItem[CMD_CLEAR_ALL]._itemName, nbChar, TEXT("Clear All Compares"));
	funcItem[CMD_CLEAR_ALL]._pFunc 				= ClearAllCompares;
	funcItem[CMD_CLEAR_ALL]._pShKey 			= new ShortcutKey;
	funcItem[CMD_CLEAR_ALL]._pShKey->_isAlt 	= true;
	funcItem[CMD_CLEAR_ALL]._pShKey->_isCtrl	= true;
	funcItem[CMD_CLEAR_ALL]._pShKey->_isShift	= true;
	funcItem[CMD_CLEAR_ALL]._pShKey->_key 		= 'X';

	_tcscpy_s(funcItem[CMD_LAST_SAVE_DIFF]._itemName, nbChar, TEXT("Diff since last Save"));
	funcItem[CMD_LAST_SAVE_DIFF]._pFunc				= LastSaveDiff;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey 			= new ShortcutKey;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isAlt 	= true;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_key 		= 'D';

	_tcscpy_s(funcItem[CMD_SVN_DIFF]._itemName, nbChar, TEXT("SVN Diff"));
	funcItem[CMD_SVN_DIFF]._pFunc 				= SvnDiff;
	funcItem[CMD_SVN_DIFF]._pShKey 				= new ShortcutKey;
	funcItem[CMD_SVN_DIFF]._pShKey->_isAlt 		= true;
	funcItem[CMD_SVN_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_SVN_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_SVN_DIFF]._pShKey->_key 		= 'V';

	_tcscpy_s(funcItem[CMD_GIT_DIFF]._itemName, nbChar, TEXT("GIT Diff"));
	funcItem[CMD_GIT_DIFF]._pFunc 				= GitDiff;
	funcItem[CMD_GIT_DIFF]._pShKey 				= new ShortcutKey;
	funcItem[CMD_GIT_DIFF]._pShKey->_isAlt 		= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_GIT_DIFF]._pShKey->_key 		= 'G';

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

	_tcscpy_s(funcItem[CMD_SETTINGS]._itemName, nbChar, TEXT("Settings..."));
	funcItem[CMD_SETTINGS]._pFunc = OpenSettingsDlg;

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

	SettingsDlg.destroy();
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
	const int buffId = getCurrentBuffId();

	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	if ((notifyCode->modificationType & SC_MOD_BEFOREDELETE) &&
		(notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_REDO)))
	{
		ScopedIncrementer incr(notificationsLock);

		HWND currentView = getCurrentView();

		const int startLine = ::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position, 0);
		const int endLine =
			::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position + notifyCode->length, 0);

		DeletedSection delSection(startLine, endLine - startLine);
		bool markersDeleted = false;

		for (int line = startLine; line < endLine; ++line)
		{
			const int marker = ::SendMessage(currentView, SCI_MARKERGET, line, 0);
			if (marker)
			{
				delSection.markers[line - startLine] = marker;
				::SendMessage(currentView, SCI_MARKERDELETE, line, -1);
				markersDeleted = true;
			}
		}

		if (markersDeleted)
			cmpPair->getFileByBuffId(buffId).deletedSections.push_back(delSection);
	}
	else if ((notifyCode->modificationType & SC_MOD_INSERTTEXT) &&
			(notifyCode->modificationType & SC_PERFORMED_UNDO))
	{
		DeletedSections_t& deletedSections = cmpPair->getFileByBuffId(buffId).deletedSections;
		if (deletedSections.empty() || !notifyCode->linesAdded)
			return;

		ScopedIncrementer incr(notificationsLock);

		HWND currentView = getCurrentView();

		const int startLine = ::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position, 0);

		const DeletedSection& lastDeleted = deletedSections.back();
		if (lastDeleted.startLine == startLine)
		{
			const int linesCount = lastDeleted.markers.size();
			for (int i = 0; i < linesCount; ++i)
			{
				if (lastDeleted.markers[i])
				{
					int markerId = 0;
					for (int marker = lastDeleted.markers[i]; marker; marker >>= 1)
					{
						if (marker & 1)
							::SendMessage(currentView, SCI_MARKERADD, startLine + i, markerId);
						++markerId;
					}
				}
			}

			deletedSections.pop_back();
		}
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


void onBufferActivatedDelayed(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	bool switchedFromOtherPair = false;
	const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

	// When compared file is activated make sure its corresponding pair file is
	// also active in the other view
	if (getDocId(getOtherView()) != otherFile.sciDoc)
	{
		ScopedIncrementer incr(notificationsLock);

		activateBufferID(otherFile.buffId);
		activateBufferID(buffId);

		switchedFromOtherPair = true;
	}

	nppSettings.setCompareMode();
	nppSettings.updatePluginMenu();
	setCompareView(nppData._scintillaMainHandle);
	setCompareView(nppData._scintillaSecondHandle);

	if (Settings.UseNavBar && (switchedFromOtherPair || !NavDlg.isVisible()))
		NavDlg.doDialog(true);

	::SetFocus(getCurrentView());
}


void onBufferActivated(int buffId)
{
	DelayedWork::cancel();

	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
	{
		nppSettings.setNormalMode();
		setNormalView(getCurrentView());
		return;
	}

	DelayedWork::post(onBufferActivatedDelayed, buffId, 80);
}


void onFileBeforeClose(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	const int currentBuffId = getCurrentBuffId();

	nppSettings.setNormalMode();

	{
		ScopedIncrementer incr(notificationsLock);

		if (buffId != currentBuffId)
			activateBufferID(buffId);

		clearWindow(getCurrentView());

		cmpPair->getOtherFileByBuffId(buffId).restore();

		compareList.erase(cmpPair);
	}

	activateBufferID(currentBuffId);
}


void onFileClosed()
{
	resetCompareView(nppData._scintillaMainHandle);
	resetCompareView(nppData._scintillaSecondHandle);
}


void onFileBeforeSave(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	HWND view = getView(cmpPair->getFileByBuffId(buffId).compareViewId);

	saveNotifData.reset(new SaveNotificationData(buffId));

	saveNotifData->firstVisibleLine = ::SendMessage(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
	saveNotifData->position = ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0);
	saveNotifData->blankSections = removeBlankLines(view, true);
}


void onFileSaved(int buffId)
{
	if (saveNotifData->fileBuffId == buffId)
	{
		CompareList_t::iterator cmpPair = getCompare(buffId);
		if (cmpPair == compareList.end())
			return;

		HWND view = getView(cmpPair->getFileByBuffId(buffId).compareViewId);

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

	Settings.load();

	AboutDlg.init(hInstance, nppData);
	SettingsDlg.init(hInstance, nppData);
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

		case NPPN_FILECLOSED:
			if ((bool)newPair && (newPair->file[0].buffId == (int)notifyCode->nmhdr.idFrom))
			{
				newPair.reset();

				if (!nppSettings.compareMode)
					nppSettings.updatePluginMenu();
			}
			else if (!notificationsLock && !compareList.empty())
			{
				onFileClosed();
			}
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
				NavDlg.SetColors(Settings.colors);
				NavDlg.CreateBitmap();
			}
        }
        break;

		case NPPN_TBMODIFICATION:
			onToolBarReady();
		break;

		case NPPN_SHUTDOWN:
			Settings.save();
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

