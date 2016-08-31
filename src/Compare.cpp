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

#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>

#include "Compare.h"
#include "NppHelpers.h"
#include "LibHelpers.h"
#include "ProgressDlg.h"
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
const TCHAR UserSettings::compareToPrevSetting[]	= TEXT("Default Compare is to Prev");
const TCHAR UserSettings::gotoFirstDiffSetting[]	= TEXT("Go to First Diff");
const TCHAR UserSettings::encodingsCheckSetting[]	= TEXT("Check Encodings");
const TCHAR UserSettings::wrapAroundSetting[]		= TEXT("Wrap Around");
const TCHAR UserSettings::compactNavBarSetting[]	= TEXT("Compact NavBar");
const TCHAR UserSettings::ignoreSpacesSetting[]		= TEXT("Ignore Spaces");
const TCHAR UserSettings::ignoreEOLsSetting[]		= TEXT("Ignore End of Lines");
const TCHAR UserSettings::detectMovesSetting[]		= TEXT("Detect Moves");
const TCHAR UserSettings::navBarSetting[]			= TEXT("Navigation Bar");
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

	OldFileIsFirst = ::GetPrivateProfileInt(mainSection, oldIsFirstSetting,    DEFAULT_OLD_IS_FIRST, iniFile) == 1;
	OldFileViewId  = ::GetPrivateProfileInt(mainSection, oldFileOnLeftSetting, DEFAULT_OLD_ON_LEFT, iniFile) == 1 ?
			MAIN_VIEW : SUB_VIEW;
	CompareToPrev  = ::GetPrivateProfileInt(mainSection, compareToPrevSetting,  DEFAULT_COMPARE_TO_PREV, iniFile) == 1;
	GotoFirstDiff  = ::GetPrivateProfileInt(mainSection, gotoFirstDiffSetting,  DEFAULT_GOTO_FIRST_DIFF, iniFile) == 1;
	EncodingsCheck = ::GetPrivateProfileInt(mainSection, encodingsCheckSetting, DEFAULT_ENCODINGS_CHECK, iniFile) == 1;
	WrapAround     = ::GetPrivateProfileInt(mainSection, wrapAroundSetting,     DEFAULT_WRAP_AROUND, iniFile) == 1;
	CompactNavBar  = ::GetPrivateProfileInt(mainSection, compactNavBarSetting,  DEFAULT_COMPACT_NAVBAR, iniFile) == 1;

	IgnoreSpaces = ::GetPrivateProfileInt(mainSection, ignoreSpacesSetting,	0, iniFile) == 1;
	IgnoreEOLs   = ::GetPrivateProfileInt(mainSection, ignoreEOLsSetting,	1, iniFile) == 1;
	DetectMoves  = ::GetPrivateProfileInt(mainSection, detectMovesSetting,	1, iniFile) == 1;
	UseNavBar    = ::GetPrivateProfileInt(mainSection, navBarSetting,		0, iniFile) == 1;

	colors.added     = ::GetPrivateProfileInt(colorsSection, addedColorSetting,		DEFAULT_ADDED_COLOR, iniFile);
	colors.deleted   = ::GetPrivateProfileInt(colorsSection, removedColorSetting,	DEFAULT_DELETED_COLOR, iniFile);
	colors.changed   = ::GetPrivateProfileInt(colorsSection, changedColorSetting,	DEFAULT_CHANGED_COLOR, iniFile);
	colors.moved     = ::GetPrivateProfileInt(colorsSection, movedColorSetting,		DEFAULT_MOVED_COLOR, iniFile);
	colors.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorSetting,	DEFAULT_HIGHLIGHT_COLOR, iniFile);
	colors.alpha     = ::GetPrivateProfileInt(colorsSection, highlightAlphaSetting,	DEFAULT_HIGHLIGHT_ALPHA, iniFile);
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
	::WritePrivateProfileString(mainSection, compareToPrevSetting,
			CompareToPrev ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, gotoFirstDiffSetting,
			GotoFirstDiff ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, encodingsCheckSetting,
			EncodingsCheck ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, wrapAroundSetting,
			WrapAround ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, compactNavBarSetting,
			CompactNavBar ? TEXT("1") : TEXT("0"), iniFile);

	::WritePrivateProfileString(mainSection, ignoreSpacesSetting,	IgnoreSpaces	? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreEOLsSetting,		IgnoreEOLs		? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, detectMovesSetting,	DetectMoves		? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, navBarSetting,			UseNavBar		? TEXT("1") : TEXT("0"), iniFile);

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
 *  \class
 *  \brief
 */
class NppSettings
{
public:
	static NppSettings& get()
	{
		static NppSettings instance;
		return instance;
	}

	void enableClearCommands() const;
	void updatePluginMenu() const;
	void save();
	void setNormalMode();
	void setCompareMode();

	bool	compareMode;

private:
	NppSettings() : compareMode(false) {}

	bool	_syncVScroll;
	bool	_syncHScroll;
};


/**
 *  \struct
 *  \brief
 */
struct ScopedIncrementer
{
	ScopedIncrementer(volatile unsigned& useCount) : _useCount(useCount)
	{
		++_useCount;
	}

	~ScopedIncrementer()
	{
		--_useCount;
	}

private:
	volatile unsigned&	_useCount;
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
	static bool isPending();
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


bool DelayedWork::isPending()
{
	return (instance()._timerId != 0);
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
struct DeletedSection
{
	DeletedSection(int action, int line, int len) : startLine(line), lineReplace(false)
	{
		restoreAction = (action == SC_PERFORMED_UNDO) ? SC_PERFORMED_REDO : SC_PERFORMED_UNDO;

		markers.resize(len, 0);
	}

	int					startLine;
	bool				lineReplace;
	int					restoreAction;
	std::vector<int>	markers;
};


/**
 *  \struct
 *  \brief
 */
struct DeletedSectionsList
{
	DeletedSectionsList() : skipPush(0), lastPushTimeMark(0) {}

	void push(int currAction, int startLine, int endLine);
	void pop(int currAction, int startLine);

	void clear()
	{
		skipPush = 0;
		sections.clear();
	}

private:
	int							skipPush;
	DWORD						lastPushTimeMark;
	std::vector<DeletedSection>	sections;
};


void DeletedSectionsList::push(int currAction, int startLine, int endLine)
{
	if (endLine <= startLine)
		return;

	if (skipPush)
	{
		--skipPush;
		return;
	}

	// Is it line replacement revert operation?
	if (!sections.empty() && sections.back().restoreAction == currAction && sections.back().lineReplace)
		return;

	DeletedSection delSection(currAction, startLine, endLine - startLine + 1);

	HWND currentView = getCurrentView();

	const int startPos = ::SendMessage(currentView, SCI_POSITIONFROMLINE, startLine, 0);
	clearChangedIndicator(currentView,
			startPos, ::SendMessage(currentView, SCI_POSITIONFROMLINE, endLine, 0) - startPos);

	for (int line = startLine; line <= endLine; ++line)
	{
		const int marker = ::SendMessage(currentView, SCI_MARKERGET, line, 0);
		if (marker)
		{
			delSection.markers[line - startLine] = marker;
			if (line != endLine)
				::SendMessage(currentView, SCI_MARKERDELETE, line, -1);
		}
	}

	sections.push_back(delSection);

	lastPushTimeMark = ::GetTickCount();
}


void DeletedSectionsList::pop(int currAction, int startLine)
{
	if (sections.empty())
	{
		++skipPush;
		return;
	}

	DeletedSection& last = sections.back();

	if (last.restoreAction != currAction)
	{
		// Try to guess if this is the insert part of line replacement operation
		if (::GetTickCount() < lastPushTimeMark + 40)
			last.lineReplace = true;
		else
			++skipPush;

		return;
	}

	if (last.startLine != startLine)
		return;

	HWND currentView = getCurrentView();

	const int linesCount = last.markers.size();

	const int startPos = ::SendMessage(currentView, SCI_POSITIONFROMLINE, last.startLine, 0);
	clearChangedIndicator(currentView,
			startPos, ::SendMessage(currentView, SCI_POSITIONFROMLINE, last.startLine + linesCount, 0) - startPos);

	for (int i = 0; i < linesCount; ++i)
	{
		::SendMessage(currentView, SCI_MARKERDELETE, last.startLine + i, -1);

		if (last.markers[i])
		{
			int markerId = 0;
			for (int marker = last.markers[i]; marker; marker >>= 1)
			{
				if (marker & 1)
					::SendMessage(currentView, SCI_MARKERADD, last.startLine + i, markerId);
				++markerId;
			}
		}
	}

	sections.pop_back();
}


enum Temp_t
{
	NO_TEMP = 0,
	LAST_SAVED_TEMP,
	SVN_TEMP,
	GIT_TEMP
};


/**
 *  \struct
 *  \brief
 */
struct ComparedFile
{
	ComparedFile() : isTemp(NO_TEMP) {}

	void initFromCurrent(bool currFileIsNew);
	void updateFromCurrent();
	void updateView();
	void clear();
	void onBeforeClose();
	void onClose();
	void close();
	void restore();

	Temp_t	isTemp;
	bool	isNew;
	int		originalViewId;
	int		originalPos;
	int		compareViewId;
	int		buffId;
	int		sciDoc;
	TCHAR	name[MAX_PATH];

	DeletedSectionsList	deletedSections;
};


/**
 *  \struct
 *  \brief
 */
struct ComparedPair
{
	ComparedFile& getFileByViewId(int viewId);
	ComparedFile& getFileByBuffId(int buffId);
	ComparedFile& getOtherFileByBuffId(int buffId);
	ComparedFile& getFileBySciDoc(int sciDoc);
	ComparedFile& getOldFile();
	ComparedFile& getNewFile();
	void positionFiles();

	ComparedFile file[2];
};


/**
 *  \struct
 *  \brief
 */
struct NewCompare
{
public:
	NewCompare(bool currFileIsNew, bool markFirstName);
	~NewCompare();

	ComparedPair	pair;

private:
	TCHAR	_firstTabText[64];
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
 *  \struct
 *  \brief
 */
struct SaveNotificationData
{
	SaveNotificationData(int buffId) : _location(buffId)
	{
		_blankSections = removeBlankLines(getView(viewIdFromBuffId(buffId)), true);
	}

	void restore()
	{
		if (!_blankSections.empty())
			addBlankLines(getView(viewIdFromBuffId(_location.getBuffId())), _blankSections);

		_location.restore();
	}

	inline int getBuffId()
	{
		return _location.getBuffId();
	}

private:
	ViewLocation	_location;
	BlankSections_t	_blankSections;
};


/**
 *  \struct
 *  \brief
 */
struct TempMark_t
{
	const TCHAR*	fileMark;
	const TCHAR*	tabMark;
};


static const TempMark_t tempMark[] =
{
	{ TEXT(""),				TEXT("") },
	{ TEXT("_LastSave"),	TEXT(" ** Last Save") },
	{ TEXT("_SVN"),			TEXT(" ** SVN") },
	{ TEXT("_Git"),			TEXT(" ** Git") }
};


CompareList_t compareList;
std::unique_ptr<NewCompare> newCompare;

volatile unsigned notificationsLock = 0;

std::unique_ptr<SaveNotificationData> saveNotifData;

AboutDialog   	AboutDlg;
SettingsDialog	SettingsDlg;
NavDialog     	NavDlg;

toolbarIcons  tbSetFirst;
toolbarIcons  tbCompare;
toolbarIcons  tbClearCompare;
toolbarIcons  tbFirst;
toolbarIcons  tbPrev;
toolbarIcons  tbNext;
toolbarIcons  tbLast;
toolbarIcons  tbNavBar;

HINSTANCE hInstance;
FuncItem funcItem[NB_MENU_COMMANDS] = { 0 };


// Declare local functions that appear before they are defined
void First();
void onBufferActivatedDelayed(int buffId);
void forceViewsSync(HWND focalView);


void NppSettings::enableClearCommands() const
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
	const int flag = MF_BYCOMMAND | MF_ENABLED;

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ACTIVE]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ALL]._cmdID, flag);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, true);
}


void NppSettings::updatePluginMenu() const
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
	const int flag = MF_BYCOMMAND | (compareMode ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ALL]._cmdID,
			MF_BYCOMMAND | ((compareList.empty() && !newCompare) ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ACTIVE]._cmdID,
			MF_BYCOMMAND | ((!compareMode && !newCompare) ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, flag);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
	{
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, compareMode);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_FIRST]._cmdID, compareMode);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_PREV]._cmdID, compareMode);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_NEXT]._cmdID, compareMode);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_LAST]._cmdID, compareMode);
	}
}


void NppSettings::save()
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	_syncVScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
	_syncHScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
}


void NppSettings::setNormalMode()
{
	if (compareMode == false)
		return;

	compareMode = false;

	if (NavDlg.isVisible())
		NavDlg.doDialog(false);

	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	bool syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
	if (syncScroll != _syncVScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

	syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
	if (syncScroll != _syncHScroll)
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


void ComparedFile::initFromCurrent(bool currFileIsNew)
{
	isNew = currFileIsNew;
	buffId = getCurrentBuffId();
	originalViewId = getCurrentViewId();
	compareViewId = originalViewId;
	originalPos = posFromBuffId(buffId);
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(name), (LPARAM)name);

	updateFromCurrent();
}


void ComparedFile::updateFromCurrent()
{
	sciDoc = getDocId(getCurrentView());

	if (isTemp)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(getCurrentViewId());

		if (hNppTabBar)
		{
			TCHAR tabName[MAX_PATH];
			_tcscpy_s(tabName, _countof(tabName), ::PathFindFileName(name));

			int i = _tcslen(tabName) - 1 - _tcslen(tempMark[isTemp].fileMark);
			for (; i > 0 && tabName[i] != TEXT('_'); --i);

			if (i > 0)
			{
				tabName[i] = 0;
				_tcscat_s(tabName, _countof(tabName), tempMark[isTemp].tabMark);

				TCITEM tab;
				tab.mask = TCIF_TEXT;
				tab.pszText = tabName;

				TabCtrl_SetItem(hNppTabBar, posFromBuffId(buffId), &tab);
			}
		}
	}
}


void ComparedFile::updateView()
{
	compareViewId = isNew ? ((Settings.OldFileViewId == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW) : Settings.OldFileViewId;
}


void ComparedFile::clear()
{
	clearWindow(getView(viewIdFromBuffId(buffId)));

	deletedSections.clear();
}


void ComparedFile::onBeforeClose()
{
	if (buffId != getCurrentBuffId())
		activateBufferID(buffId);

	HWND view = getCurrentView();
	clearWindow(view);

	if (isTemp)
		::SendMessage(view, SCI_SETSAVEPOINT, true, 0);
}


void ComparedFile::onClose()
{
	if (isTemp)
	{
		::SetFileAttributes(name, FILE_ATTRIBUTE_NORMAL);
		::DeleteFile(name);
	}
}


void ComparedFile::close()
{
	onBeforeClose();

	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);

	onClose();
}


void ComparedFile::restore()
{
	if (isTemp)
	{
		close();
		return;
	}

	if (buffId != getCurrentBuffId())
		activateBufferID(buffId);

	clearWindow(getCurrentView());

	if (viewIdFromBuffId(buffId) != originalViewId)
	{
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		// TODO: Restore file original position here
	}
}


ComparedFile& ComparedPair::getFileByViewId(int viewId)
{
	return (viewIdFromBuffId(file[0].buffId) == viewId) ? file[0] : file[1];
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
	const int currentBuffId = getCurrentBuffId();

	ComparedFile& oldFile = getOldFile();
	ComparedFile& newFile = getNewFile();

	oldFile.updateView();
	newFile.updateView();

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

	activateBufferID(currentBuffId);
}


NewCompare::NewCompare(bool currFileIsNew, bool markFirstName)
{
	_firstTabText[0] = 0;

	pair.file[0].initFromCurrent(currFileIsNew);

	// Enable commands to be able to clear the first file that was just set
	NppSettings::get().enableClearCommands();

	if (markFirstName)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(pair.file[0].originalViewId);

		if (hNppTabBar)
		{
			TCITEM tab;
			tab.mask = TCIF_TEXT;
			tab.pszText = _firstTabText;
			tab.cchTextMax = _countof(_firstTabText);

			TabCtrl_GetItem(hNppTabBar, pair.file[0].originalPos, &tab);

			TCHAR tabText[MAX_PATH];
			tab.pszText = tabText;

			_sntprintf_s(tabText, _countof(tabText), _TRUNCATE, TEXT("%s ** %s to Compare"),
					_firstTabText, Settings.OldFileIsFirst ? TEXT("Old") : TEXT("New"));

			TabCtrl_SetItem(hNppTabBar, pair.file[0].originalPos, &tab);
		}
	}
}


NewCompare::~NewCompare()
{
	if (_firstTabText[0] != 0)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(pair.file[0].originalViewId);

		if (hNppTabBar)
		{
			// This is workaround for Wine issue with tab bar refresh
			::InvalidateRect(hNppTabBar, NULL, FALSE);

			TCITEM tab;
			tab.mask = TCIF_TEXT;
			tab.pszText = _firstTabText;

			TabCtrl_SetItem(hNppTabBar, posFromBuffId(pair.file[0].buffId), &tab);

			::UpdateWindow(hNppTabBar);
		}
	}

	if (!NppSettings::get().compareMode)
		NppSettings::get().updatePluginMenu();
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


void showNavBar()
{
	NavDlg.SetConfig(Settings);
	NavDlg.doDialog();
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


bool isEncodingOK(const ComparedPair& cmpPair)
{
	// Warn about encoding mismatches as that might compromise the compare
	if (::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, cmpPair.file[0].buffId, 0) !=
		::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, cmpPair.file[1].buffId, 0))
	{
		if (::MessageBox(nppData._nppHandle,
			TEXT("Trying to compare files with different encodings - \n")
			TEXT("the result might be inaccurate and misleading.\n\n")
			TEXT("Compare anyway?"), TEXT("Compare Plugin"), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
		{
			return false;
		}
	}

	return true;
}


bool setFirst(bool currFileIsNew, bool markName = false)
{
	HWND view = getCurrentView();

	if (isFileCompared(view))
		return false;

	// Done on purpose: First wipe the std::unique_ptr so ~NewCompare is called before the new object constructor.
	// This is important because the N++ plugin menu is updated on NewCompare construct/destruct.
	newCompare.reset();
	newCompare.reset(new NewCompare(currFileIsNew, markName));

	return true;
}


void setContent(const char* content)
{
	HWND view = getCurrentView();

	ScopedViewUndoCollectionBlocker undoBlock(view);
	ScopedViewWriteEnabler writeEn(view);

	::SendMessage(view, SCI_SETTEXT, 0, (LPARAM)content);
	::SendMessage(view, SCI_SETSAVEPOINT, true, 0);
}


bool createTempFile(const TCHAR *file, Temp_t tempType)
{
	if (::PathFileExists(file) == FALSE)
	{
		::MessageBox(nppData._nppHandle, TEXT("File is not written to disk - operation ignored."),
				TEXT("Compare Plugin"), MB_OK);
		return false;
	}

	if (!setFirst(true))
		return false;

	TCHAR tempFile[MAX_PATH];

	if (::GetTempPath(_countof(tempFile), tempFile))
	{
		const TCHAR* fileName = ::PathFindFileName(newCompare->pair.file[0].name);

		if (::PathAppend(tempFile, fileName))
		{
			_tcscat_s(tempFile, _countof(tempFile), tempMark[tempType].fileMark);

			unsigned idxPos = _tcslen(tempFile);

			// Make sure temp file is unique
			for (int i = 1; ; ++i)
			{
				TCHAR idx[32];

				_itot_s(i, idx, _countof(idx), 10);

				if (_tcslen(idx) + idxPos + 1 > _countof(tempFile))
				{
					idxPos = _countof(tempFile);
					break;
				}

				_tcscat_s(tempFile, _countof(tempFile), idx);

				if (!::PathFileExists(tempFile))
					break;

				tempFile[idxPos] = 0;
			}

			if ((idxPos + 1 <= _countof(tempFile)) && ::CopyFile(file, tempFile, TRUE))
			{
				::SetFileAttributes(tempFile, FILE_ATTRIBUTE_TEMPORARY);

				const int langType = ::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE,
						newCompare->pair.file[0].buffId, 0);

				ScopedIncrementer incr(notificationsLock);

				if (::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)tempFile))
				{
					const int buffId = getCurrentBuffId();

					::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, buffId, langType);
					::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_SETREADONLY);

					newCompare->pair.file[1].isTemp = tempType;

					return true;
				}
			}
		}
	}

	::MessageBox(nppData._nppHandle, TEXT("Creating temp file failed - operation aborted."),
			TEXT("Compare Plugin"), MB_OK);

	newCompare.reset();

	return false;
}


void clearComparePair(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	NppSettings& nppSettings = NppSettings::get();
	nppSettings.setNormalMode();

	ScopedIncrementer incr(notificationsLock);

	cmpPair->getOtherFileByBuffId(buffId).restore();
	cmpPair->getFileByBuffId(buffId).restore();

	compareList.erase(cmpPair);

	resetCompareView(getOtherView());
	onBufferActivatedDelayed(getCurrentBuffId());

	nppSettings.updatePluginMenu();
}


void closeComparePair(CompareList_t::iterator& cmpPair)
{
	HWND currentView = getCurrentView();

	NppSettings& nppSettings = NppSettings::get();
	nppSettings.setNormalMode();

	ScopedIncrementer incr(notificationsLock);

	// First close the file in the SUB_VIEW as closing a file may lead to a single view mode
	// and if that happens we want to be in single main view
	cmpPair->getFileByViewId(SUB_VIEW).close();
	cmpPair->getFileByViewId(MAIN_VIEW).close();

	compareList.erase(cmpPair);

	if (::IsWindowVisible(currentView))
		::SetFocus(currentView);

	onBufferActivatedDelayed(getCurrentBuffId());
	resetCompareView(getOtherView());

	nppSettings.updatePluginMenu();
}


bool initNewCompare()
{
	bool firstIsSet = (bool)newCompare;

	// Compare to self?
	if (firstIsSet && (newCompare->pair.file[0].buffId == getCurrentBuffId()))
		firstIsSet = false;

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
					Settings.CompareToPrev ? IDM_VIEW_TAB_PREV : IDM_VIEW_TAB_NEXT);
		}
		else
		{
			HWND otherView = getOtherView();

			// Check if the file in the other view is compared already
			if (isFileCompared(otherView))
				return false;

			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SWITCHTO_OTHER_VIEW);
		}
	}

	newCompare->pair.file[1].initFromCurrent(!newCompare->pair.file[0].isNew);

	return true;
}


CompareList_t::iterator addComparePair()
{
	if (compareList.empty())
		NppSettings::get().save();

	compareList.push_back(newCompare->pair);
	newCompare.reset();

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
	const DocLines_t doc1 = getAllLines(view1, lineNum1, Settings.IgnoreEOLs);
	const int doc1Length = doc1.size();

	std::vector<int> lineNum2;
	const DocLines_t doc2 = getAllLines(view2, lineNum2, Settings.IgnoreEOLs);
	const int doc2Length = doc2.size();

	{
		const TCHAR* newName = ::PathFindFileName(cmpPair->getNewFile().name);
		const TCHAR* oldName = ::PathFindFileName(cmpPair->getOldFile().name);

		TCHAR msg[MAX_PATH];
		_sntprintf_s(msg, _countof(msg), _TRUNCATE, TEXT("Comparing \"%s\" vs. \"%s\"..."), newName, oldName);

		ProgressDlg::Open(msg);
	}

	std::vector<unsigned int> doc1Hashes = computeHashes(doc1, Settings.IgnoreSpaces);
	std::vector<unsigned int> doc2Hashes = computeHashes(doc2, Settings.IgnoreSpaces);

	if (ProgressDlg::IsCancelled())
		return COMPARE_CANCELLED;

	std::vector<diff_edit> diff = DiffCalc<unsigned int>(doc1Hashes, doc2Hashes)();

	if (ProgressDlg::IsCancelled())
		return COMPARE_CANCELLED;

	if (diff.empty())
	{
		ProgressDlg::Close();
		return FILES_MATCH;
	}

	const std::size_t diffSize = diff.size();

	int	doc1ChangedLinesCount = 0;
	int	doc2ChangedLinesCount = 0;

	shiftBoundries(diff, doc1Hashes.data(), doc2Hashes.data(), doc1Length, doc2Length);

	if (Settings.DetectMoves)
		findMoves(diff, doc1Hashes.data(), doc2Hashes.data());

	// Insert empty lines, count changed lines
	for (unsigned int i = 0; i < diffSize; ++i)
	{
		diff_edit& e1 = diff[i];

		if (e1.op == DIFF_DELETE)
		{
			e1.changeCount = 0;
			doc1ChangedLinesCount += e1.len;

			diff_edit& e2 = diff[i + 1];

			e2.changeCount = 0;

			if (e2.op == DIFF_INSERT)
			{
				// check if the DELETE/INSERT pair includes changed lines or it's a completely new block
				if (compareWords(e1, e2, doc1, doc2, Settings.IgnoreSpaces))
				{
					e1.op = DIFF_CHANGE1;
					e2.op = DIFF_CHANGE2;
					doc2ChangedLinesCount += e2.len;
				}
			}
		}
		else if (e1.op == DIFF_INSERT)
		{
			e1.changeCount = 0;
			doc2ChangedLinesCount += e1.len;
		}
	}

	if (ProgressDlg::IsCancelled())
		return COMPARE_CANCELLED;

	std::vector<diff_edit> doc1Changes(doc1ChangedLinesCount);
	std::vector<diff_edit> doc2Changes(doc2ChangedLinesCount);

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

	if (ProgressDlg::IsCancelled())
		return COMPARE_CANCELLED;

	if ((doc1ChangedLinesCount == 0) && (doc2ChangedLinesCount == 0))
	{
		ProgressDlg::Close();
		return FILES_MATCH;
	}

	for (int i = 0; i < doc1ChangedLinesCount; ++i)
	{
		switch (doc1Changes[i].op)
		{
			case DIFF_DELETE:
				markAsRemoved(view1, doc1Changes[i].off);
			break;

			case DIFF_CHANGE1:
			{
				markAsChanged(view1, doc1Changes[i].off);
				const int textIndex = lineNum1[doc1Changes[i].off];

				for (unsigned int k = 0; k < doc1Changes[i].changeCount; ++k)
				{
					diff_change& change = doc1Changes[i].changes->get(k);
					markTextAsChanged(view1, textIndex + change.off, change.len);
				}
			}
			break;

			case DIFF_MOVE:
				markAsMoved(view1, doc1Changes[i].off);
			break;
		}
	}

	for (int i = 0; i < doc2ChangedLinesCount; ++i)
	{
		switch (doc2Changes[i].op)
		{
			case DIFF_INSERT:
				markAsAdded(view2, doc2Changes[i].off);
			break;

			case DIFF_CHANGE2:
			{
				markAsChanged(view2, doc2Changes[i].off);
				const int textIndex = lineNum2[doc2Changes[i].off];

				for (unsigned int k = 0; k < doc2Changes[i].changeCount; ++k)
				{
					diff_change& change = doc2Changes[i].changes->get(k);
					markTextAsChanged(view2, textIndex + change.off, change.len);
				}
			}
			break;

			case DIFF_MOVE:
				markAsMoved(view2, doc2Changes[i].off);
			break;
		}
	}

	{
		int length = 0;
		int off = 0;
		doc2Offset = 0;

		for (int i = 0; i < doc1ChangedLinesCount; ++i)
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

		for (int i = 0; i < doc2ChangedLinesCount; i++)
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

	if (ProgressDlg::IsCancelled())
		return COMPARE_CANCELLED;

	ProgressDlg::Close();

	return FILES_DIFFER;
}


CompareResult_t runCompare(CompareList_t::iterator& cmpPair)
{
	cmpPair->positionFiles();

	NppSettings::get().setCompareMode();

	setStyles(Settings);

	CompareResult_t result = COMPARE_ERROR;

	try
	{
		result = doCompare(cmpPair);
	}
	catch (std::exception& e)
	{
		ProgressDlg::Close();
		char msg[128];
		_snprintf_s(msg, _countof(msg), _TRUNCATE, "Exception occurred: %s", e.what());
		MessageBoxA(nppData._nppHandle, msg, "Compare Plugin", MB_OK | MB_ICONWARNING);
	}
	catch (...)
	{
		ProgressDlg::Close();
		MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "Compare Plugin", MB_OK | MB_ICONWARNING);
	}

	return result;
}


void SetAsFirst()
{
	if (!setFirst(!Settings.OldFileIsFirst, true))
		newCompare.reset();
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
			newCompare.reset();
			return;
		}

		cmpPair = addComparePair();

		if (cmpPair->getOldFile().isTemp)
			activateBufferID(cmpPair->getNewFile().buffId);
		else
			activateBufferID(currentBuffId);
	}
	// Re-Compare triggered - clear current results
	else
	{
		recompare = true;

		newCompare.reset();

		location.save(currentBuffId);

		cmpPair->getOldFile().clear();
		cmpPair->getNewFile().clear();
	}

	CompareResult_t cmpResult;

	if (Settings.EncodingsCheck && !isEncodingOK(*cmpPair))
		cmpResult = COMPARE_CANCELLED;
	else
		cmpResult = runCompare(cmpPair);

	switch (cmpResult)
	{
		case FILES_DIFFER:
		{
			NppSettings::get().updatePluginMenu();

			if (Settings.UseNavBar)
				showNavBar();

			if (!Settings.GotoFirstDiff && recompare)
			{
				location.restore();
			}
			else
			{
				if (!doubleView)
					activateBufferID(cmpPair->getNewFile().buffId);

				First();
			}

			// Synchronize views
			forceViewsSync(getCurrentView());
		}
		break;

		case FILES_MATCH:
		{
			TCHAR msg[2 * MAX_PATH];

			const ComparedFile& oldFile = cmpPair->getOldFile();

			const TCHAR* newName = ::PathFindFileName(cmpPair->getNewFile().name);

			int choice = IDNO;

			if (oldFile.isTemp)
			{
				if (recompare)
				{
					_sntprintf_s(msg, _countof(msg), _TRUNCATE,
							TEXT("Files \"%s\" and \"%s\" match.\n\nTemp file will be closed."),
							newName, ::PathFindFileName(oldFile.name));
				}
				else
				{
					if (oldFile.isTemp == LAST_SAVED_TEMP)
						_sntprintf_s(msg, _countof(msg), _TRUNCATE,
								TEXT("File \"%s\" has not been modified since last Save."), newName);
					else
						_sntprintf_s(msg, _countof(msg), _TRUNCATE,
								TEXT("File \"%s\" has no changes against %s."), newName,
								oldFile.isTemp == GIT_TEMP ? TEXT("Git") : TEXT("SVN"));
				}

				::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin"), MB_OK);
			}
			else
			{
				_sntprintf_s(msg, _countof(msg), _TRUNCATE,
						TEXT("Files \"%s\" and \"%s\" match.\n\nClose compared files?"),
						newName, ::PathFindFileName(oldFile.name));

				choice = ::MessageBox(nppData._nppHandle, msg, TEXT("Compare Plugin"),
						MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
			}

			if (choice == IDYES)
				closeComparePair(cmpPair);
			else
				clearComparePair(getCurrentBuffId());
		}
		break;

		default:
			clearComparePair(getCurrentBuffId());
	}
}


void ClearActiveCompare()
{
	newCompare.reset();

	if (NppSettings::get().compareMode)
		clearComparePair(getCurrentBuffId());
}


void ClearAllCompares()
{
	const int buffId = getCurrentBuffId();

	newCompare.reset();

	NppSettings& nppSettings = NppSettings::get();
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

	nppSettings.updatePluginMenu();
}


void LastSaveDiff()
{
	TCHAR file[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (createTempFile(file, LAST_SAVED_TEMP))
		Compare();
}


void SvnDiff()
{
	TCHAR file[MAX_PATH];
	TCHAR svnFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (!GetSvnFile(file, svnFile, _countof(svnFile)))
		return;

	if (createTempFile(svnFile, SVN_TEMP))
		Compare();
}


void GitDiff()
{
	TCHAR file[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	std::vector<char> content = GetGitFileContent(file);

	if (content.empty())
		return;

	if (!createTempFile(file, GIT_TEMP))
		return;

	setContent(content.data());
	content.clear();

	Compare();
}


void IgnoreSpaces()
{
	Settings.IgnoreSpaces = !Settings.IgnoreSpaces;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreSpaces);
}


void IgnoreEOLs()
{
	Settings.IgnoreEOLs = !Settings.IgnoreEOLs;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_EOLS]._cmdID,
			(LPARAM)Settings.IgnoreEOLs);
}


void DetectMoves()
{
	Settings.DetectMoves = !Settings.DetectMoves;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMoves);
}


void Prev()
{
	if (NppSettings::get().compareMode)
		jumpToNextChange(false, Settings.WrapAround);
}


void Next()
{
	if (NppSettings::get().compareMode)
		jumpToNextChange(true, Settings.WrapAround);
}


void First()
{
	if (NppSettings::get().compareMode)
		jumpToFirstChange();
}


void Last()
{
	if (NppSettings::get().compareMode)
		jumpToLastChange();
}


void OpenSettingsDlg(void)
{
	if (SettingsDlg.doDialog(&Settings) == IDOK)
	{
		Settings.save();

		newCompare.reset();

		if (!compareList.empty())
		{
			setStyles(Settings);

			NavDlg.SetConfig(Settings);
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
	funcItem[CMD_SET_FIRST]._pShKey->_key		= '1';

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

	_tcscpy_s(funcItem[CMD_GIT_DIFF]._itemName, nbChar, TEXT("Git Diff"));
	funcItem[CMD_GIT_DIFF]._pFunc 				= GitDiff;
	funcItem[CMD_GIT_DIFF]._pShKey 				= new ShortcutKey;
	funcItem[CMD_GIT_DIFF]._pShKey->_isAlt 		= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_GIT_DIFF]._pShKey->_key 		= 'G';

	_tcscpy_s(funcItem[CMD_IGNORE_SPACES]._itemName, nbChar, TEXT("Ignore Spaces"));
	funcItem[CMD_IGNORE_SPACES]._pFunc = IgnoreSpaces;

	_tcscpy_s(funcItem[CMD_IGNORE_EOLS]._itemName, nbChar, TEXT("Ignore End of Lines"));
	funcItem[CMD_IGNORE_EOLS]._pFunc = IgnoreEOLs;

	_tcscpy_s(funcItem[CMD_DETECT_MOVES]._itemName, nbChar, TEXT("Detect Moves"));
	funcItem[CMD_DETECT_MOVES]._pFunc = DetectMoves;

	_tcscpy_s(funcItem[CMD_NAV_BAR]._itemName, nbChar, TEXT("Navigation Bar"));
	funcItem[CMD_NAV_BAR]._pFunc = ViewNavigationBar;

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

	if (tbSetFirst.hToolbarBmp)
		::DeleteObject(tbSetFirst.hToolbarBmp);

	if (tbCompare.hToolbarBmp)
		::DeleteObject(tbCompare.hToolbarBmp);

	if (tbClearCompare.hToolbarBmp)
		::DeleteObject(tbClearCompare.hToolbarBmp);

	if (tbFirst.hToolbarBmp)
		::DeleteObject(tbFirst.hToolbarBmp);

	if (tbPrev.hToolbarBmp)
		::DeleteObject(tbPrev.hToolbarBmp);

	if (tbNext.hToolbarBmp)
		::DeleteObject(tbNext.hToolbarBmp);

	if (tbLast.hToolbarBmp)
		::DeleteObject(tbLast.hToolbarBmp);

	if (tbNavBar.hToolbarBmp)
		::DeleteObject(tbNavBar.hToolbarBmp);

	SettingsDlg.destroy();
	AboutDlg.destroy();
	NavDlg.destroy();

	// Deallocate shortcut
	for (int i = 0; i < NB_MENU_COMMANDS; i++)
		if (funcItem[i]._pShKey != NULL)
			delete funcItem[i]._pShKey;
}


void forceViewsSync(HWND focalView)
{
	HWND otherView = getOtherView(focalView);

	const int firstVisibleLine1 = ::SendMessage(focalView, SCI_GETFIRSTVISIBLELINE, 0, 0);
	const int line = ::SendMessage(focalView, SCI_DOCLINEFROMVISIBLE, firstVisibleLine1, 0);
	int offset = ::SendMessage(focalView, SCI_VISIBLEFROMDOCLINE, line + 1, 0) - firstVisibleLine1;

	ScopedIncrementer incr(notificationsLock);

	::SendMessage(otherView, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);
	int firstVisibleLine2 = ::SendMessage(otherView, SCI_VISIBLEFROMDOCLINE, line + 1, 0) - offset;

	if (line != ::SendMessage(otherView, SCI_DOCLINEFROMVISIBLE, firstVisibleLine2, 0) ||
			firstVisibleLine1 == ::SendMessage(focalView, SCI_VISIBLEFROMDOCLINE, line, 0))
		firstVisibleLine2 = ::SendMessage(otherView, SCI_VISIBLEFROMDOCLINE, line, 0);

	::SendMessage(otherView, SCI_SETFIRSTVISIBLELINE, firstVisibleLine2, 0);

	::UpdateWindow(otherView);
}


void onToolBarReady()
{
	UINT style = (LR_SHARED | LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);

	const bool isRTL = ((::GetWindowLong(nppData._nppHandle, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0);

	if (isRTL)
		tbSetFirst.hToolbarBmp =
				(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST_RTL), IMAGE_BITMAP, 0, 0, style);
	else
		tbSetFirst.hToolbarBmp =
				(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST), IMAGE_BITMAP, 0, 0, style);

	tbCompare.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE), IMAGE_BITMAP, 0, 0, style);
	tbClearCompare.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_CLEARCOMPARE), IMAGE_BITMAP, 0, 0, style);
	tbFirst.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_FIRST),	IMAGE_BITMAP, 0, 0, style);
	tbPrev.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREV),	IMAGE_BITMAP, 0, 0, style);
	tbNext.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXT),	IMAGE_BITMAP, 0, 0, style);
	tbLast.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_LAST),	IMAGE_BITMAP, 0, 0, style);
	tbNavBar.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NAVBAR), IMAGE_BITMAP, 0, 0, style);

	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_SET_FIRST]._cmdID,		(LPARAM)&tbSetFirst);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_COMPARE]._cmdID,		(LPARAM)&tbCompare);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_CLEAR_ACTIVE]._cmdID,	(LPARAM)&tbClearCompare);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_FIRST]._cmdID,			(LPARAM)&tbFirst);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_PREV]._cmdID,			(LPARAM)&tbPrev);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_NEXT]._cmdID,			(LPARAM)&tbNext);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_LAST]._cmdID,			(LPARAM)&tbLast);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_NAV_BAR]._cmdID,		(LPARAM)&tbNavBar);

	NppSettings::get().updatePluginMenu();

	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreSpaces);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_EOLS]._cmdID,
			(LPARAM)Settings.IgnoreEOLs);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMoves);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID,
			(LPARAM)Settings.UseNavBar);
}


void onSciUpdateUI(SCNotification *notifyCode)
{
	if (notifyCode->updated & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL))
		forceViewsSync((HWND)notifyCode->nmhdr.hwndFrom);
}


void onSciModified(SCNotification *notifyCode)
{
	const int buffId = getCurrentBuffId();

	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	if (notifyCode->modificationType & SC_MOD_BEFOREDELETE)
	{
		HWND currentView = getCurrentView();

		const int startLine = ::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position, 0);
		const int endLine =
			::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position + notifyCode->length, 0);

		// Change is on single line?
		if (endLine <= startLine)
			return;

		const int currAction =
			notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

		cmpPair->getFileByBuffId(buffId).deletedSections.push(currAction, startLine, endLine);
	}
	else if ((notifyCode->modificationType & SC_MOD_INSERTTEXT) && notifyCode->linesAdded)
	{
		HWND currentView = getCurrentView();

		const int startLine = ::SendMessage(currentView, SCI_LINEFROMPOSITION, notifyCode->position, 0);

		const int currAction =
			notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

		cmpPair->getFileByBuffId(buffId).deletedSections.pop(currAction, startLine);
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

	const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

	ScopedIncrementer incr(notificationsLock);

	// When compared file is activated make sure its corresponding pair file is also active in the other view
	if (getDocId(getOtherView()) != otherFile.sciDoc)
	{
		activateBufferID(otherFile.buffId);
		activateBufferID(buffId);
	}

	forceViewsSync(getCurrentView());

	NppSettings& nppSettings = NppSettings::get();
	nppSettings.setCompareMode();
	nppSettings.updatePluginMenu();
	setCompareView(nppData._scintillaMainHandle);
	setCompareView(nppData._scintillaSecondHandle);

	if (Settings.UseNavBar && !NavDlg.isVisible())
		showNavBar();

	::SetFocus(getCurrentView());
}


void onBufferActivated(int buffId)
{
	DelayedWork::cancel();

	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
	{
		NppSettings& nppSettings = NppSettings::get();
		nppSettings.setNormalMode();

		setNormalView(getCurrentView());

		nppSettings.updatePluginMenu();
	}
	else
	{
		DelayedWork::post(onBufferActivatedDelayed, buffId, 50);
	}
}


void onFileBeforeClose(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	const int currentBuffId = getCurrentBuffId();

	NppSettings& nppSettings = NppSettings::get();
	nppSettings.setNormalMode();

	ScopedIncrementer incr(notificationsLock);

	cmpPair->getFileByBuffId(buffId).onBeforeClose();
	cmpPair->getOtherFileByBuffId(buffId).restore();

	activateBufferID(currentBuffId);

	nppSettings.updatePluginMenu();
}


void onFileClosed(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	cmpPair->getFileByBuffId(buffId).onClose();

	compareList.erase(cmpPair);

	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair != compareList.end())
	{
		resetCompareView(nppData._scintillaMainHandle);
		resetCompareView(nppData._scintillaSecondHandle);
	}
}


void onFileBeforeSave(int buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	saveNotifData.reset(new SaveNotificationData(buffId));
}


void onFileSaved(int buffId)
{
	if (saveNotifData->getBuffId() == buffId)
	{
		CompareList_t::iterator cmpPair = getCompare(buffId);
		if (cmpPair == compareList.end())
			return;

		saveNotifData->restore();

		const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);
		if (otherFile.isTemp == LAST_SAVED_TEMP)
		{
			HWND hNppTabBar = NppTabHandleGetter::get(otherFile.compareViewId);

			if (hNppTabBar)
			{
				TCHAR tabText[MAX_PATH];

				TCITEM tab;
				tab.mask = TCIF_TEXT;
				tab.pszText = tabText;
				tab.cchTextMax = _countof(tabText);

				const int tabPos = posFromBuffId(otherFile.buffId);
				TabCtrl_GetItem(hNppTabBar, tabPos, &tab);

				_tcscat_s(tabText, _countof(tabText), TEXT(" - Outdated"));

				TabCtrl_SetItem(hNppTabBar, tabPos, &tab);
			}
		}
	}

	saveNotifData.reset();
}

} // anonymous namespace


void ViewNavigationBar()
{
	Settings.UseNavBar = !Settings.UseNavBar;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID,
			(LPARAM)Settings.UseNavBar);

	if (NppSettings::get().compareMode)
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
	NavDlg.init(hInstance);
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
			if (NppSettings::get().compareMode && !DelayedWork::isPending())
				NavDlg.Update();
		break;

		// Emulate word-wrap aware vertical scroll sync
		case SCN_UPDATEUI:
			if (NppSettings::get().compareMode && !notificationsLock && !DelayedWork::isPending())
				onSciUpdateUI(notifyCode);
		break;

		case NPPN_BUFFERACTIVATED:
			if (!notificationsLock && !compareList.empty())
				onBufferActivated(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILEBEFORECLOSE:
			if ((bool)newCompare && (newCompare->pair.file[0].buffId == (int)notifyCode->nmhdr.idFrom))
				newCompare.reset();
			else if (!notificationsLock && !compareList.empty())
				onFileBeforeClose(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILECLOSED:
			if (!notificationsLock && !compareList.empty())
				onFileClosed(notifyCode->nmhdr.idFrom);
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
			if (!notificationsLock && NppSettings::get().compareMode)
				onSciModified(notifyCode);
		break;

		case SCN_ZOOM:
			if (!notificationsLock && NppSettings::get().compareMode)
				onSciZoom();
		break;

		case NPPN_WORDSTYLESUPDATED:
			setStyles(Settings);
			NavDlg.SetConfig(Settings);
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
