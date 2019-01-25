/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2018 Pavel Nedev (pg.nedev@gmail.com)
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

#include <cstdlib>
#include <vector>
#include <memory>

#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>

#include "Tools.h"
#include "Compare.h"
#include "NppHelpers.h"
#include "LibHelpers.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "NavDialog.h"
#include "Engine.h"
#include "NppInternalDefines.h"
#include "resource.h"


const TCHAR PLUGIN_NAME[] = TEXT("Compare");

NppData			nppData;
SciFnDirect		sciFunc;
sptr_t			sciPtr[2];

UserSettings	Settings;

#ifdef DLOG

std::string		dLog("Compare Plugin debug log\n\n");
DWORD			dLogTime_ms = 0;
static LRESULT	dLogBuf = -1;

#endif


namespace // anonymous namespace
{

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

	void enableClearCommands(bool enable) const;
	void enableNppScrollCommands(bool enable) const;
	void updatePluginMenu();
	void setNormalMode(bool forceUpdate = false);
	void setCompareMode(bool clearHorizontalScroll = false);

	void setMainZoom(int zoom)
	{
		_mainZoom = zoom;
	}

	void setSubZoom(int zoom)
	{
		_subZoom = zoom;
	}

	void setCompareZoom(int zoom)
	{
		_compareZoom = zoom;
	}

	bool	compareMode;

private:
	NppSettings() : compareMode(false), _restoreMultilineTab(false), _mainZoom(0), _subZoom(0), _compareZoom(0) {}

	void save();
	void toSingleLineTab();
	void restoreMultilineTab();
	void refreshTabBar(HWND hTabBar);
	void refreshTabBars();

	bool	_restoreMultilineTab;

	bool	_syncVScroll;
	bool	_syncHScroll;

	int		_mainZoom;
	int		_subZoom;
	int		_compareZoom;
};


/**
 *  \struct
 *  \brief
 */
struct DeletedSection
{
	DeletedSection(int action, int line, int len) : startLine(line), lineReplace(false), onlyAlignmentBlankChange(false)
	{
		restoreAction = (action == SC_PERFORMED_UNDO) ? SC_PERFORMED_REDO : SC_PERFORMED_UNDO;

		markers.resize(len, 0);
	}

	int					startLine;
	bool				lineReplace;
	int					restoreAction;
	bool				onlyAlignmentBlankChange;
	std::vector<int>	markers;
};


/**
 *  \struct
 *  \brief
 */
struct DeletedSectionsList
{
	DeletedSectionsList() : skipPush(0), lastPushTimeMark(0) {}

	std::vector<DeletedSection>& get()
	{
		return sections;
	}

	bool push(int view, int currAction, int startLine, int len);
	void pop(int view, int currAction, int startLine);

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


bool DeletedSectionsList::push(int view, int currAction, int startLine, int len)
{
	if (len < 1)
		return false;

	if (skipPush)
	{
		--skipPush;
		return false;
	}

	// Is it line replacement revert operation?
	if (!sections.empty() && sections.back().restoreAction == currAction && sections.back().lineReplace)
		return false;

	DeletedSection delSection(currAction, startLine, len);

	const int startPos = CallScintilla(view, SCI_POSITIONFROMLINE, startLine, 0);
	clearChangedIndicator(view, startPos, CallScintilla(view, SCI_POSITIONFROMLINE, startLine + len, 0) - startPos);

	for (int line = CallScintilla(view, SCI_MARKERPREVIOUS, startLine + len - 1, MARKER_MASK_ALL_PLUS_BLANK);
			line >= startLine;
			line = CallScintilla(view, SCI_MARKERPREVIOUS, line - 1, MARKER_MASK_ALL_PLUS_BLANK))
	{
		delSection.markers[line - startLine] =
				CallScintilla(view, SCI_MARKERGET, line, 0) & MARKER_MASK_ALL_PLUS_BLANK;
		clearMarks(view, line);
	}

	if ((len == 1) && (delSection.markers[0] & MARKER_MASK_BLANK))
		delSection.onlyAlignmentBlankChange = true;

	sections.push_back(delSection);

	lastPushTimeMark = ::GetTickCount();

	return delSection.onlyAlignmentBlankChange;
}


void DeletedSectionsList::pop(int view, int currAction, int startLine)
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

	const int linesCount = static_cast<int>(last.markers.size());

	const int startPos = CallScintilla(view, SCI_POSITIONFROMLINE, last.startLine, 0);
	clearChangedIndicator(view,
			startPos, CallScintilla(view, SCI_POSITIONFROMLINE, last.startLine + linesCount, 0) - startPos);

	for (int i = 0; i < linesCount; ++i)
	{
		clearMarks(view, last.startLine + i);

		if (last.markers[i])
			CallScintilla(view, SCI_MARKERADDSET, last.startLine + i, last.markers[i]);
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
 *  \class
 *  \brief
 */
class ComparedFile
{
public:
	ComparedFile() : isTemp(NO_TEMP) {}

	void initFromCurrent(bool currFileIsNew);
	void updateFromCurrent();
	void updateView();
	void clear();
	void onBeforeClose() const;
	void close() const;
	void restore() const;
	bool isOpen() const;

	bool pushDeletedSection(int sciAction, int startLine, int len)
	{
		return deletedSections.push(compareViewId, sciAction, startLine, len);
	}

	void popDeletedSection(int sciAction, int startLine)
	{
		deletedSections.pop(compareViewId, sciAction, startLine);
	}

	bool redoAlignmentBlankDeletion()
	{
		if (!deletedSections.get().empty() && deletedSections.get().back().onlyAlignmentBlankChange)
		{
			::PostMessage(getView(compareViewId), SCI_UNDO, 0, 0);
			return true;
		}

		return false;
	}

	Temp_t	isTemp;
	bool	isNew;

	int		originalViewId;
	int		originalPos;
	int		compareViewId;

	LRESULT	buffId;
	int		sciDoc;
	TCHAR	name[MAX_PATH];

	bool	restoreAlignmentLine {false};

private:
	DeletedSectionsList deletedSections;
};


/**
 *  \class
 *  \brief
 */
class ComparedPair
{
public:
	inline ComparedFile& getFileByViewId(int viewId);
	inline ComparedFile& getFileByBuffId(LRESULT buffId);
	inline ComparedFile& getOtherFileByBuffId(LRESULT buffId);
	inline ComparedFile& getFileBySciDoc(int sciDoc);
	inline ComparedFile& getOldFile();
	inline ComparedFile& getNewFile();

	void positionFiles();
	void restoreFiles(int currentBuffId);

	void setStatus();

	ComparedFile	file[2];
	int				relativePos;

	CompareOptions	options;

	AlignmentInfo_t	alignmentInfo;

	int				autoUpdateDelay = 0;
};


/**
 *  \class
 *  \brief
 */
class NewCompare
{
public:
	NewCompare(bool currFileIsNew, bool markFirstName);
	~NewCompare();

	ComparedPair	pair;

private:
	TCHAR	_firstTabText[64];
};


using CompareList_t = std::vector<ComparedPair>;


/**
 *  \class
 *  \brief
 */
class DelayedAlign : public DelayedWork
{
public:
	DelayedAlign() : DelayedWork(), _consecutiveAligns(0) {}
	virtual ~DelayedAlign() = default;

	virtual void operator()();

private:
	unsigned _consecutiveAligns; // Used as alignment oscillation filter in some corner cases
};


/**
 *  \class
 *  \brief
 */
class DelayedActivate : public DelayedWork
{
public:
	DelayedActivate() : DelayedWork() {}
	virtual ~DelayedActivate() = default;

	virtual void operator()();

	inline void operator()(LRESULT buff)
	{
		buffId = buff;
		operator()();
	}

	LRESULT buffId;
};


/**
 *  \class
 *  \brief
 */
class DelayedClose : public DelayedWork
{
public:
	DelayedClose() : DelayedWork() {}
	virtual ~DelayedClose() = default;

	virtual void operator()();

	std::vector<LRESULT> closedBuffs;
};


/**
 *  \class
 *  \brief
 */
class DelayedMaximize : public DelayedWork
{
public:
	DelayedMaximize() : DelayedWork() {}
	virtual ~DelayedMaximize() = default;

	virtual void operator()();
};


/**
 *  \class
 *  \brief
 */
class DelayedUpdate : public DelayedWork
{
public:
	DelayedUpdate() : DelayedWork() {}
	virtual ~DelayedUpdate() = default;

	virtual void operator()();
};


/**
 *  \class
 *  \brief
 */
class LineArrowMarkTimeout : public DelayedWork
{
public:
	LineArrowMarkTimeout(int view, int markerHandle) : DelayedWork(), _view(view), _markerHandle(markerHandle) {}
	virtual ~LineArrowMarkTimeout();

	virtual void operator()();

private:
	int	_view;
	int	_markerHandle;
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

volatile unsigned	notificationsLock = 0;
bool				isNppMinimized = false;

std::unique_ptr<ViewLocation> storedLocation;
bool goToFirst = false;

DelayedAlign	delayedAlignment;
DelayedActivate	delayedActivation;
DelayedClose	delayedClosure;
DelayedUpdate	delayedUpdate;
DelayedMaximize	delayedMaximize;

NavDialog     	NavDlg;

toolbarIcons  tbSetFirst;
toolbarIcons  tbCompare;
toolbarIcons  tbCompareSel;
toolbarIcons  tbClearCompare;
toolbarIcons  tbFirst;
toolbarIcons  tbPrev;
toolbarIcons  tbNext;
toolbarIcons  tbLast;
toolbarIcons  tbDiffsOnly;
toolbarIcons  tbNavBar;

HINSTANCE hInstance;
FuncItem funcItem[NB_MENU_COMMANDS] = { 0 };

// Declare local functions that appear before they are defined
void onBufferActivated(LRESULT buffId);
void syncViews(int biasView);
void setArrowMark(int view, int line = -1, bool down = true);


void NppSettings::enableClearCommands(bool enable) const
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ACTIVE]._cmdID,
			MF_BYCOMMAND | ((!enable && !compareMode) ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ALL]._cmdID,
			MF_BYCOMMAND | ((!enable && compareList.empty()) ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED));

	::DrawMenuBar(nppData._nppHandle);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, enable || compareMode);
}


void NppSettings::enableNppScrollCommands(bool enable) const
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);
	const int flag = MF_BYCOMMAND | (enable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));

	::EnableMenuItem(hMenu, IDM_VIEW_SYNSCROLLH, flag);
	::EnableMenuItem(hMenu, IDM_VIEW_SYNSCROLLV, flag);

	::DrawMenuBar(nppData._nppHandle);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
	{
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, IDM_VIEW_SYNSCROLLH, enable);
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, IDM_VIEW_SYNSCROLLV, enable);
	}
}


void NppSettings::updatePluginMenu()
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
	const int flag = MF_BYCOMMAND | (compareMode ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ACTIVE]._cmdID,
			MF_BYCOMMAND | ((!compareMode && !newCompare) ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ALL]._cmdID,
			MF_BYCOMMAND | ((compareList.empty() && !newCompare) ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, flag);

	::DrawMenuBar(nppData._nppHandle);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
	{
		::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, compareMode || newCompare);
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

	if (_mainZoom == 0)
		_mainZoom	= CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0);

	if (_subZoom == 0)
		_subZoom	= CallScintilla(SUB_VIEW, SCI_GETZOOM, 0, 0);
}


void NppSettings::setNormalMode(bool forceUpdate)
{
	if (compareMode)
	{
		compareMode = false;

		restoreMultilineTab();

		if (NavDlg.isVisible())
			NavDlg.Hide();

		if (!isSingleView())
		{
			enableNppScrollCommands(true);

			HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

			bool syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
			if (syncScroll != _syncVScroll)
				::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

			syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
			if (syncScroll != _syncHScroll)
				::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLH);
		}

		CallScintilla(MAIN_VIEW, SCI_SETZOOM, _mainZoom, 0);
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _subZoom, 0);

		updatePluginMenu();
	}
	else if (forceUpdate)
	{
		restoreMultilineTab();

		CallScintilla(MAIN_VIEW, SCI_SETZOOM, _mainZoom, 0);
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _subZoom, 0);

		updatePluginMenu();
	}
}


void NppSettings::setCompareMode(bool clearHorizontalScroll)
{
	if (compareMode)
		return;

	compareMode = true;

	save();

	toSingleLineTab();

	if (clearHorizontalScroll)
	{
		int pos = CallScintilla(MAIN_VIEW, SCI_POSITIONFROMLINE, getCurrentLine(MAIN_VIEW), 0);
		CallScintilla(MAIN_VIEW, SCI_SETSEL, pos, pos);

		pos = CallScintilla(SUB_VIEW, SCI_POSITIONFROMLINE, getCurrentLine(SUB_VIEW), 0);
		CallScintilla(SUB_VIEW, SCI_SETSEL, pos, pos);
	}

	// Disable N++ vertical scroll - we handle it manually because of the Word Wrap
	if (_syncVScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

	// Yaron - Enable N++ horizontal scroll sync
	if (!_syncHScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLH);

	// synchronize zoom levels
	if (_compareZoom == 0)
	{
		_compareZoom = CallScintilla(getCurrentViewId(), SCI_GETZOOM, 0, 0);
		CallScintilla(getOtherViewId(), SCI_SETZOOM, _compareZoom, 0);
	}
	else
	{
		CallScintilla(MAIN_VIEW, SCI_SETZOOM, _compareZoom, 0);
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _compareZoom, 0);
	}

	enableNppScrollCommands(false);
	updatePluginMenu();
}


void NppSettings::refreshTabBar(HWND hTabBar)
{
	if (::IsWindowVisible(hTabBar) && (TabCtrl_GetItemCount(hTabBar) > 1))
		{
		const int currentTabIdx = TabCtrl_GetCurSel(hTabBar);

		TabCtrl_SetCurFocus(hTabBar, (currentTabIdx) ? 0 : 1);
		TabCtrl_SetCurFocus(hTabBar, currentTabIdx);
	}
}


void NppSettings::refreshTabBars()
{
	HWND currentView = getCurrentView();

	HWND hNppTabBar = NppTabHandleGetter::get(SUB_VIEW);

	if (hNppTabBar)
		refreshTabBar(hNppTabBar);

	hNppTabBar = NppTabHandleGetter::get(MAIN_VIEW);

	if (hNppTabBar)
		refreshTabBar(hNppTabBar);

	::SetFocus(currentView);
}


void NppSettings::toSingleLineTab()
{
	if (!_restoreMultilineTab)
	{
		HWND hNppMainTabBar = NppTabHandleGetter::get(MAIN_VIEW);
		HWND hNppSubTabBar = NppTabHandleGetter::get(SUB_VIEW);

		if (hNppMainTabBar && hNppSubTabBar)
		{
			RECT tabRec;
			::GetWindowRect(hNppMainTabBar, &tabRec);
			const int mainTabYPos = tabRec.top;

			::GetWindowRect(hNppSubTabBar, &tabRec);
			const int subTabYPos = tabRec.top;

			// Both views are side-by-side positioned
			if (mainTabYPos == subTabYPos)
			{
				LONG_PTR tabStyle = ::GetWindowLongPtr(hNppMainTabBar, GWL_STYLE);

				if ((tabStyle & TCS_MULTILINE) && !(tabStyle & TCS_VERTICAL))
				{
					::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

					::SetWindowLongPtr(hNppMainTabBar, GWL_STYLE, tabStyle & ~TCS_MULTILINE);
					::SendMessage(hNppMainTabBar, WM_TABSETSTYLE, 0, 0);

					tabStyle = ::GetWindowLongPtr(hNppSubTabBar, GWL_STYLE);
					::SetWindowLongPtr(hNppSubTabBar, GWL_STYLE, tabStyle & ~TCS_MULTILINE);
					::SendMessage(hNppSubTabBar, WM_TABSETSTYLE, 0, 0);

					::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);

					// Scroll current tab into view
					refreshTabBars();

					_restoreMultilineTab = true;
				}
			}
		}
	}
}


void NppSettings::restoreMultilineTab()
{
	if (_restoreMultilineTab)
	{
		_restoreMultilineTab = false;

		HWND hNppMainTabBar = NppTabHandleGetter::get(MAIN_VIEW);
		HWND hNppSubTabBar = NppTabHandleGetter::get(SUB_VIEW);

		if (hNppMainTabBar && hNppSubTabBar)
		{
			LONG_PTR tabStyle = ::GetWindowLongPtr(hNppMainTabBar, GWL_STYLE);

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

			::SetWindowLongPtr(hNppMainTabBar, GWL_STYLE, tabStyle | TCS_MULTILINE);
			::SendMessage(hNppMainTabBar, WM_TABSETSTYLE, 0, 0);

			tabStyle = ::GetWindowLongPtr(hNppSubTabBar, GWL_STYLE);
			::SetWindowLongPtr(hNppSubTabBar, GWL_STYLE, tabStyle | TCS_MULTILINE);
			::SendMessage(hNppSubTabBar, WM_TABSETSTYLE, 0, 0);

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
		}
	}
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
	sciDoc = getDocId(getCurrentViewId());

	if (isTemp)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(getCurrentViewId());

		if (hNppTabBar)
		{
			const TCHAR* fileExt = ::PathFindExtension(name);

			TCHAR tabName[MAX_PATH];

			_tcscpy_s(tabName, _countof(tabName), ::PathFindFileName(name));
			::PathRemoveExtension(tabName);

			int i = _tcslen(tabName) - 1 - _tcslen(tempMark[isTemp].fileMark);
			for (; i > 0 && tabName[i] != TEXT('_'); --i);

			if (i > 0)
			{
				tabName[i] = 0;
				_tcscat_s(tabName, _countof(tabName), fileExt);
				_tcscat_s(tabName, _countof(tabName), tempMark[isTemp].tabMark);

				TCITEM tab;
				tab.mask = TCIF_TEXT;
				tab.pszText = tabName;

				::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

				TabCtrl_SetItem(hNppTabBar, posFromBuffId(buffId), &tab);

				::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
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
	clearWindow(viewIdFromBuffId(buffId));
	setArrowMark(-1);

	deletedSections.clear();
}


void ComparedFile::onBeforeClose() const
{
	activateBufferID(buffId);

	const int view = getCurrentViewId();

	clearWindow(view);
	setArrowMark(-1);

	if (isTemp)
	{
		CallScintilla(view, SCI_SETSAVEPOINT, 0, 0);
		::SetFileAttributes(name, FILE_ATTRIBUTE_NORMAL);
		::DeleteFile(name);
	}
}


void ComparedFile::close() const
{
	onBeforeClose();

	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
}


void ComparedFile::restore() const
{
	if (isTemp)
	{
		close();
		return;
	}

	activateBufferID(buffId);

	clearWindow(getCurrentViewId());
	setArrowMark(-1);

	if (viewIdFromBuffId(buffId) != originalViewId)
	{
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);

		if (!isOpen())
			return;

		const int currentPos = posFromBuffId(buffId);

		if (originalPos >= currentPos)
			return;

		for (int i = currentPos - originalPos; i; --i)
			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_TAB_MOVEBACKWARD);
	}
}


bool ComparedFile::isOpen() const
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffId, (LPARAM)NULL) >= 0);
}


ComparedFile& ComparedPair::getFileByViewId(int viewId)
{
	return (viewIdFromBuffId(file[0].buffId) == viewId) ? file[0] : file[1];
}


ComparedFile& ComparedPair::getFileByBuffId(LRESULT buffId)
{
	return (file[0].buffId == buffId) ? file[0] : file[1];
}


ComparedFile& ComparedPair::getOtherFileByBuffId(LRESULT buffId)
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
	const LRESULT currentBuffId = getCurrentBuffId();

	ComparedFile& oldFile = getOldFile();
	ComparedFile& newFile = getNewFile();

	oldFile.updateView();
	newFile.updateView();

	relativePos = (oldFile.originalViewId != newFile.originalViewId) ? 0 :
			(oldFile.originalViewId == oldFile.compareViewId) ?
			newFile.originalPos - oldFile.originalPos : oldFile.originalPos - newFile.originalPos;

	if (viewIdFromBuffId(oldFile.buffId) != oldFile.compareViewId)
	{
		activateBufferID(oldFile.buffId);
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		oldFile.updateFromCurrent();
	}

	if (viewIdFromBuffId(newFile.buffId) != newFile.compareViewId)
	{
		activateBufferID(newFile.buffId);
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		newFile.updateFromCurrent();
	}

	if (oldFile.sciDoc != getDocId(oldFile.compareViewId))
		activateBufferID(oldFile.buffId);

	if (newFile.sciDoc != getDocId(newFile.compareViewId))
		activateBufferID(newFile.buffId);

	activateBufferID(currentBuffId);
}


void ComparedPair::restoreFiles(int currentBuffId = -1)
{
	// Check if position update is needed -
	// this is for relative re-positioning to keep files initial order consistent
	if (relativePos)
	{
		ComparedFile* biasFile;
		ComparedFile* movedFile;

		// One of the files is in its original view and won't be moved - this is the bias file
		if (viewIdFromBuffId(file[0].buffId) == file[0].originalViewId)
		{
			biasFile = &file[0];
			movedFile = &file[1];
		}
		else
		{
			biasFile = &file[1];
			movedFile = &file[0];
		}

		if (biasFile->originalPos > movedFile->originalPos)
		{
			const int newPos = posFromBuffId(biasFile->buffId);

			if (newPos != biasFile->originalPos && newPos < movedFile->originalPos)
				movedFile->originalPos = newPos;
		}
	}

	if (currentBuffId == -1)
	{
		file[0].restore();
		file[1].restore();
	}
	else
	{
		getOtherFileByBuffId(currentBuffId).restore();
		getFileByBuffId(currentBuffId).restore();
	}
}


void ComparedPair::setStatus()
{
	const int alignOff = (isAlignmentFirstLineInserted(MAIN_VIEW) || isAlignmentFirstLineInserted(SUB_VIEW)) ? 2 : 1;
	TCHAR cmpType[128];

	if (options.selectionCompare)
		_sntprintf_s(cmpType, _countof(cmpType), _TRUNCATE, TEXT("Sel: %d-%d vs. %d-%d"),
				options.selections[MAIN_VIEW].first + alignOff, options.selections[MAIN_VIEW].second + alignOff,
				options.selections[SUB_VIEW].first + alignOff, options.selections[SUB_VIEW].second + alignOff);
	else
		_tcscpy_s(cmpType, _countof(cmpType), TEXT("Full"));

	TCHAR msg[512];

	_sntprintf_s(msg, _countof(msg), _TRUNCATE,
			TEXT("%s (%s)    Ignore Spaces (%s)    Ignore Case (%s)    Detect Moves (%s)"),
			options.findUniqueMode	? TEXT("Find Unique") : TEXT("Compare"), cmpType,
			options.ignoreSpaces	? TEXT("Y")	: TEXT("N"),
			options.ignoreCase		? TEXT("Y")	: TEXT("N"),
			options.detectMoves		? TEXT("Y")	: TEXT("N"));

	::SendMessageW(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, static_cast<LPARAM>((LONG_PTR)msg));
}


NewCompare::NewCompare(bool currFileIsNew, bool markFirstName)
{
	_firstTabText[0] = 0;

	pair.file[0].initFromCurrent(currFileIsNew);

	// Enable commands to be able to clear the first file that was just set
	NppSettings::get().enableClearCommands(true);

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

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

			TabCtrl_SetItem(hNppTabBar, pair.file[0].originalPos, &tab);

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
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

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

			TabCtrl_SetItem(hNppTabBar, posFromBuffId(pair.file[0].buffId), &tab);

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
		}
	}

	if (!NppSettings::get().compareMode)
		NppSettings::get().enableClearCommands(false);
}


LineArrowMarkTimeout::~LineArrowMarkTimeout()
{
	(*this)();
}


void LineArrowMarkTimeout::operator()()
{
	CallScintilla(_view, SCI_MARKERDELETEHANDLE, _markerHandle, 0);
}


CompareList_t::iterator getCompare(LRESULT buffId)
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


void setArrowMark(int view, int line, bool down)
{
	static std::unique_ptr<LineArrowMarkTimeout> lineArrowMark;

	if (view < 0 || line < 0)
	{
		lineArrowMark.reset();
		return;
	}

	const int markerHandle = showArrowSymbol(view, line, down);

	lineArrowMark.reset(new LineArrowMarkTimeout(view, markerHandle));

	if (lineArrowMark)
		lineArrowMark->post(3000);
}


void showBlankAdjacentArrowMark(int view, int line, bool down)
{
	if (view < 0)
	{
		setArrowMark(-1);
		return;
	}

	if (line < 0 && Settings.FollowingCaret)
		line = getCurrentLine(view);

	if (line >= 0 && !isLineMarked(view, line, MARKER_MASK_LINE) && isVisibleAdjacentAnnotation(view, line, down))
		setArrowMark(view, line, down);
	else
		setArrowMark(-1);
}


std::pair<int, int> jumpToNextChange(int mainStartLine, int subStartLine, bool down,
		bool goToCornerDiff = false, bool doNotBlink = false)
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return std::make_pair(-1, -1);

	int view			= getCurrentViewId();
	const int otherView	= getOtherViewId(view);

	if (!cmpPair->options.findUniqueMode && !goToCornerDiff)
	{
		const int edgeLine		= (down ? getLastLine(view) : getFirstLine(view));
		const int currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : edgeLine);

		// Is the bias line manually positioned on a screen edge and adjacent to invisible blank diff?
		// Make sure we don't miss it
		if (!isLineMarked(view, currentLine, MARKER_MASK_LINE) &&
			isAdjacentAnnotation(view, currentLine, down) &&
			!isVisibleAdjacentAnnotation(view, currentLine, down) &&
			isLineMarked(otherView, otherViewMatchingLine(view, currentLine) + 1, MARKER_MASK_LINE))
		{
			centerAt(view, currentLine);
			return std::make_pair(view, currentLine);
		}
	}

	const bool isCornerDiff = (((mainStartLine == 0) && (subStartLine == 0)) ||
			((mainStartLine == CallScintilla(MAIN_VIEW, SCI_GETLINECOUNT, 0, 0)) &&
			(subStartLine == CallScintilla(SUB_VIEW, SCI_GETLINECOUNT, 0, 0))));

	const int nextMarker = down ? SCI_MARKERNEXT : SCI_MARKERPREVIOUS;

	int mainNextLine	= CallScintilla(MAIN_VIEW, nextMarker, mainStartLine, MARKER_MASK_LINE);
	int subNextLine		= CallScintilla(SUB_VIEW, nextMarker, subStartLine, MARKER_MASK_LINE);

	if (mainNextLine == mainStartLine && (!goToCornerDiff || !isCornerDiff))
		mainNextLine = -1;

	if (subNextLine == subStartLine && (!goToCornerDiff || !isCornerDiff))
		subNextLine = -1;

	int line			= (view == MAIN_VIEW) ? mainNextLine : subNextLine;
	const int otherLine	= (view == MAIN_VIEW) ? subNextLine : mainNextLine;

	if (line < 0)
	{
		// End of doc reached - no more diffs
		if (otherLine < 0)
			return std::make_pair(-1, -1);

		if (cmpPair->options.findUniqueMode)
		{
			view = otherView;
			line = otherLine;
		}
		else
		{
			line = otherViewMatchingLine(otherView, otherLine);
		}
	}
	else if (otherLine >= 0)
	{
		const int visibleLine		= CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0);
		const int otherVisibleLine	= CallScintilla(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		const bool switchViews = down ? (otherVisibleLine < visibleLine) : (otherVisibleLine > visibleLine);

		if (switchViews)
		{
			if (cmpPair->options.findUniqueMode)
			{
				view = otherView;
				line = otherLine;
			}
			else
			{
				line = otherViewMatchingLine(otherView, otherLine);
			}
		}
	}

	if (cmpPair->options.findUniqueMode && Settings.FollowingCaret)
		::SetFocus(getView(view));

	if (!down && !Settings.HideMatches && isLineAnnotated(view, line))
		++line;

	// No explicit go to corner diff but we are there - diffs wrap has occurred - 'up/down' notion is inverted
	if (!goToCornerDiff && isCornerDiff)
	{
		const int edgeLine		= (down ? getLastLine(view) : getFirstLine(view));
		const int currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : edgeLine);

		bool dontChangeLine = false;

		if ((down && (currentLine <= line)) || (!down && (currentLine >= line)))
			dontChangeLine = true;

		if (dontChangeLine)
		{
			int lineToBlink;

			if (isLineVisible(view, line))
			{
				lineToBlink = line;
			}
			else
			{
				lineToBlink = edgeLine;

				if ((down && (edgeLine > line)) || (!down && (edgeLine < line)))
					dontChangeLine = false;
			}

			if (dontChangeLine)
			{
				blinkLine(view, lineToBlink);

				return std::make_pair(view, -1);
			}
		}
	}

	LOGD("Jump to " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
			" view, center doc line: " + std::to_string(line + 1) + "\n");

	// Line is not visible - scroll into view
	if (!isLineVisible(view, line) ||
		(!isLineMarked(view, line, MARKER_MASK_LINE) &&
		isAdjacentAnnotation(view, line, down) && !isVisibleAdjacentAnnotation(view, line, down)))
	{
		centerAt(view, line);
		doNotBlink = true;
	}

	if (Settings.FollowingCaret && line != getCurrentLine(view))
	{
		int pos;

		if (down && (isLineAnnotated(view, line) && isLineWrapped(view, line) &&
				!isLineMarked(view, line, MARKER_MASK_LINE)))
			pos = CallScintilla(view, SCI_GETLINEENDPOSITION, line, 0);
		else
			pos = CallScintilla(view, SCI_POSITIONFROMLINE, line, 0);

		CallScintilla(view, SCI_SETEMPTYSELECTION, pos, 0);

		doNotBlink = true;
		line = -1;
	}

	if (!doNotBlink)
		blinkLine(view, line);

	return std::make_pair(view, line);
}


std::pair<int, int> jumpToFirstChange(bool goToCornerDiff = false, bool doNotBlink = false)
{
	std::pair<int, int> viewLoc = jumpToNextChange(0, 0, true, goToCornerDiff, doNotBlink);
	showBlankAdjacentArrowMark(viewLoc.first, viewLoc.second, true);

	return viewLoc;
}


std::pair<int, int> jumpToLastChange(bool goToCornerDiff = false, bool doNotBlink = false)
{
	std::pair<int, int> viewLoc = jumpToNextChange(CallScintilla(MAIN_VIEW, SCI_GETLINECOUNT, 0, 0),
			CallScintilla(SUB_VIEW, SCI_GETLINECOUNT, 0, 0), false, goToCornerDiff, doNotBlink);
	showBlankAdjacentArrowMark(viewLoc.first, viewLoc.second, false);

	return viewLoc;
}


std::pair<int, int> jumpToChange(bool down, bool wrapAround)
{
	std::pair<int, int> viewLoc;

	int mainStartLine	= 0;
	int subStartLine	= 0;

	const int currentView	= getCurrentViewId();
	const int otherView		= getOtherViewId(currentView);

	int& currentLine	= (currentView == MAIN_VIEW) ? mainStartLine : subStartLine;
	int& otherLine		= (currentView != MAIN_VIEW) ? mainStartLine : subStartLine;

	if (down)
	{
		currentLine = (Settings.FollowingCaret ? getCurrentLine(currentView) : getLastLine(currentView));

		const bool currentLineNotAnnotated = !isLineAnnotated(currentView, currentLine);

		if (!currentLineNotAnnotated && isVisibleAdjacentAnnotation(currentView, currentLine, down))
			++currentLine;

		otherLine = (Settings.FollowingCaret ?
				otherViewMatchingLine(currentView, currentLine) : getLastLine(otherView));

		if (currentLineNotAnnotated && isLineAnnotated(otherView, otherLine))
			++otherLine;

		viewLoc = jumpToNextChange(getNextUnmarkedLine(MAIN_VIEW, mainStartLine, MARKER_MASK_LINE),
				getNextUnmarkedLine(SUB_VIEW, subStartLine, MARKER_MASK_LINE), down);
	}
	else
	{
		currentLine = (Settings.FollowingCaret ? getCurrentLine(currentView) : getFirstLine(currentView));

		if (isVisibleAdjacentAnnotation(currentView, currentLine, down))
			--currentLine;

		otherLine = (Settings.FollowingCaret ?
				otherViewMatchingLine(currentView, currentLine) : getFirstLine(otherView));

		viewLoc = jumpToNextChange(getPrevUnmarkedLine(MAIN_VIEW, mainStartLine, MARKER_MASK_LINE),
				getPrevUnmarkedLine(SUB_VIEW, subStartLine, MARKER_MASK_LINE), down);
	}

	if (viewLoc.first < 0)
	{
		if (wrapAround)
		{
			if (down)
				viewLoc = jumpToFirstChange(true, true);
			else
				viewLoc = jumpToLastChange(true, true);

			FLASHWINFO flashInfo;
			flashInfo.cbSize	= sizeof(flashInfo);
			flashInfo.hwnd		= nppData._nppHandle;
			flashInfo.uCount	= 3;
			flashInfo.dwTimeout	= 100;
			flashInfo.dwFlags	= FLASHW_ALL;
			::FlashWindowEx(&flashInfo);
		}
		else
		{
			if (down)
				viewLoc = jumpToLastChange();
			else
				viewLoc = jumpToFirstChange();
		}
	}
	else
	{
		showBlankAdjacentArrowMark(viewLoc.first, viewLoc.second, down);
	}

	return viewLoc;
}


void resetCompareView(int view)
{
	if (!::IsWindowVisible(getView(view)))
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(view));
	if (cmpPair != compareList.end())
		setCompareView(view, Settings.colors.blank);
}


bool isAlignmentNeeded(int view, const AlignmentInfo_t& alignmentInfo)
{
	const AlignmentViewData AlignmentPair::*pView = (view == MAIN_VIEW) ? &AlignmentPair::main : &AlignmentPair::sub;

	const int off		= isAlignmentFirstLineInserted(MAIN_VIEW) ? 1 : 0;
	const int subOff	= isAlignmentFirstLineInserted(SUB_VIEW) ? 1 : 0;

	if (off ^ subOff)
		return false;

	const int firstLine = getFirstLine(view);
	const int lastLine = getLastLine(view);

	const int maxSize = static_cast<int>(alignmentInfo.size());
	int i;

	for (i = 0; i < maxSize; ++i)
	{
		if ((alignmentInfo[i].*pView).line + off >= firstLine)
			break;
	}

	if (i == maxSize)
		return false;

	if (i)
		--i;

	for (; i < maxSize; ++i)
	{
		if (Settings.HideMatches)
		{
			if ((alignmentInfo[i].main.diffMask != 0) && (alignmentInfo[i].sub.diffMask != 0) &&
					(CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].main.line + off, 0) !=
					CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].sub.line + off, 0)))
				return true;
		}
		else
		{
			if ((alignmentInfo[i].main.diffMask == alignmentInfo[i].sub.diffMask) &&
					(CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].main.line + off, 0) !=
					CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].sub.line + off, 0)))
				return true;
		}

		if ((alignmentInfo[i].*pView).line + off > lastLine)
			break;
	}

	return false;
}


void alignDiffs(const CompareList_t::iterator& cmpPair)
{
	const AlignmentInfo_t& alignmentInfo = cmpPair->alignmentInfo;

	if (Settings.HideMatches)
	{
		hideUnmarked(MAIN_VIEW, MARKER_MASK_LINE);
		hideUnmarked(SUB_VIEW, MARKER_MASK_LINE);
	}
	else
	{
		CallScintilla(MAIN_VIEW, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
		CallScintilla(SUB_VIEW, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
	}

	int off = 0;
	{
		const bool mainAlignBlank	= isAlignmentFirstLineInserted(MAIN_VIEW);
		const bool subAlignBlank	= isAlignmentFirstLineInserted(SUB_VIEW);

		if (mainAlignBlank)
		{
			if (!subAlignBlank)
				insertAlignmentFirstLine(SUB_VIEW);

			off = 1;
		}
		else
		{
			if (subAlignBlank)
			{
				insertAlignmentFirstLine(MAIN_VIEW);
				off = 1;
			}
		}
	}

	const int mainEndLine = CallScintilla(MAIN_VIEW, SCI_GETLINECOUNT, 0, 0) - 1;
	const int subEndLine = CallScintilla(SUB_VIEW, SCI_GETLINECOUNT, 0, 0) - 1;

	const int maxSize = static_cast<int>(alignmentInfo.size());

	// Align diffs
	for (int i = 0; i < maxSize &&
			alignmentInfo[i].main.line + off <= mainEndLine && alignmentInfo[i].sub.line + off <= subEndLine; ++i)
	{
		int previousUnhiddenLine = getPreviousUnhiddenLine(MAIN_VIEW, alignmentInfo[i].main.line + off);

		if (isLineAnnotated(MAIN_VIEW, previousUnhiddenLine))
			CallScintilla(MAIN_VIEW, SCI_ANNOTATIONSETTEXT, previousUnhiddenLine, (LPARAM)NULL);

		previousUnhiddenLine = getPreviousUnhiddenLine(SUB_VIEW, alignmentInfo[i].sub.line + off);

		if (isLineAnnotated(SUB_VIEW, previousUnhiddenLine))
			CallScintilla(SUB_VIEW, SCI_ANNOTATIONSETTEXT, previousUnhiddenLine, (LPARAM)NULL);

		const int mismatchLen =
				CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].main.line + off, 0) -
				CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].sub.line + off, 0);

		if (off == 0 && mismatchLen != 0 && (alignmentInfo[i].main.line == 0 || alignmentInfo[i].sub.line == 0))
		{
			insertAlignmentFirstLine(MAIN_VIEW);
			insertAlignmentFirstLine(SUB_VIEW);
			off = 1;
		}

		if (mismatchLen > 0)
		{
			if ((i + 1 < maxSize) && (alignmentInfo[i].sub.line == alignmentInfo[i + 1].sub.line))
				continue;

			addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line + off, mismatchLen);
		}
		else if (mismatchLen < 0)
		{
			if ((i + 1 < maxSize) && (alignmentInfo[i].main.line == alignmentInfo[i + 1].main.line))
				continue;

			addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line + off, -mismatchLen);
		}
	}

	// Mark selections for clarity
	if (cmpPair->options.selectionCompare)
	{
		if (cmpPair->options.selections[MAIN_VIEW].first + off > 0 &&
			cmpPair->options.selections[SUB_VIEW].first + off > 0)
		{
			int mainAnnotation = getLineAnnotation(MAIN_VIEW,
					getPreviousUnhiddenLine(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first + off));

			int subAnnotation = getLineAnnotation(SUB_VIEW,
					getPreviousUnhiddenLine(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first + off));

			if (mainAnnotation == 0 || subAnnotation == 0)
			{
				++mainAnnotation;
				++subAnnotation;
			}

			addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first + off,
					mainAnnotation, -mainAnnotation);
			addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first + off,
					subAnnotation, -subAnnotation);
		}

		{
			int mainAnnotation = getLineAnnotation(MAIN_VIEW,
					getPreviousUnhiddenLine(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].second + off + 1));

			int subAnnotation = getLineAnnotation(SUB_VIEW,
					getPreviousUnhiddenLine(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].second + off + 1));

			if (mainAnnotation == 0 || subAnnotation == 0)
			{
				++mainAnnotation;
				++subAnnotation;
			}

			addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].second + off + 1,
					mainAnnotation, mainAnnotation);
			addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].second + off + 1,
					subAnnotation, subAnnotation);
		}
	}
}


void showNavBar()
{
	NavDlg.SetColors(Settings.colors);
	NavDlg.Show();
}


bool isFileCompared(int view)
{
	const int sciDoc = getDocId(view);

	CompareList_t::iterator cmpPair = getCompareBySciDoc(sciDoc);
	if (cmpPair != compareList.end())
	{
		const TCHAR* fname = ::PathFindFileName(cmpPair->getFileBySciDoc(sciDoc).name);

		TCHAR msg[MAX_PATH];
		_sntprintf_s(msg, _countof(msg), _TRUNCATE,
				TEXT("File \"%s\" is already compared - operation ignored."), fname);
		::MessageBox(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK);

		return true;
	}

	return false;
}


bool isEncodingOK(const ComparedPair& cmpPair)
{
	// Warn about encoding mismatches as that might compromise the compare
	if (getEncoding(cmpPair.file[0].buffId) != getEncoding(cmpPair.file[1].buffId))
	{
		if (::MessageBox(nppData._nppHandle,
			TEXT("Trying to compare files with different encodings - \n")
			TEXT("the result might be inaccurate and misleading.\n\n")
			TEXT("Compare anyway?"), PLUGIN_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
		{
			return false;
		}
	}

	return true;
}


// Call it with no arguments when re-comparing (the files are active in both views)
bool areSelectionsValid(LRESULT currentBuffId = -1, LRESULT otherBuffId = -1)
{
	int view1 = (currentBuffId == otherBuffId) ? MAIN_VIEW : viewIdFromBuffId(currentBuffId);
	int view2 = (currentBuffId == otherBuffId) ? SUB_VIEW : viewIdFromBuffId(otherBuffId);

	if (view1 == view2)
		activateBufferID(otherBuffId);

	std::pair<int, int> viewSel = getSelectionLines(view2);
	bool valid = !(viewSel.first < 0);

	if (view1 == view2)
		activateBufferID(currentBuffId);

	if (valid)
	{
		viewSel = getSelectionLines(view1);
		valid = !(viewSel.first < 0);
	}

	if (!valid)
		::MessageBox(nppData._nppHandle, TEXT("No selected lines to compare - operation ignored."),
				PLUGIN_NAME, MB_OK);

	return valid;
}


bool setFirst(bool currFileIsNew, bool markName = false)
{
	if (isFileCompared(getCurrentViewId()))
		return false;

	// Done on purpose: First wipe the std::unique_ptr so ~NewCompare is called before the new object constructor.
	// This is important because the N++ plugin menu is updated on NewCompare construct/destruct.
	newCompare.reset();
	newCompare.reset(new NewCompare(currFileIsNew, markName));

	return true;
}


void setContent(const char* content)
{
	const int view = getCurrentViewId();

	ScopedViewUndoCollectionBlocker undoBlock(view);
	ScopedViewWriteEnabler writeEn(view);

	CallScintilla(view, SCI_SETTEXT, 0, (LPARAM)content);
	CallScintilla(view, SCI_SETSAVEPOINT, 0, 0);
}


bool checkFileExists(const TCHAR *file)
{
	if (::PathFileExists(file) == FALSE)
	{
		::MessageBox(nppData._nppHandle, TEXT("File is not written to disk - operation ignored."),
				PLUGIN_NAME, MB_OK);
		return false;
	}

	return true;
}


bool createTempFile(const TCHAR *file, Temp_t tempType)
{
	if (!setFirst(true, false))
		return false;

	TCHAR tempFile[MAX_PATH];

	if (::GetTempPath(_countof(tempFile), tempFile))
	{
		const TCHAR* fileName	= ::PathFindFileName(newCompare->pair.file[0].name);
		const TCHAR* fileExt	= ::PathFindExtension(newCompare->pair.file[0].name);

		if (::PathAppend(tempFile, fileName))
		{
			::PathRemoveExtension(tempFile);

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
				_tcscat_s(tempFile, _countof(tempFile), fileExt);

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
					const LRESULT buffId = getCurrentBuffId();

					::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, buffId, langType);
					::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_SETREADONLY);

					newCompare->pair.file[1].isTemp = tempType;

					return true;
				}
			}
		}
	}

	::MessageBox(nppData._nppHandle, TEXT("Creating temp file failed - operation aborted."), PLUGIN_NAME, MB_OK);

	newCompare.reset();

	return false;
}


void clearComparePair(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	ScopedIncrementer incr(notificationsLock);

	cmpPair->restoreFiles(buffId);

	compareList.erase(cmpPair);

	onBufferActivated(getCurrentBuffId());
}


void closeComparePair(CompareList_t::iterator cmpPair)
{
	HWND currentView = getCurrentView();

	ScopedIncrementer incr(notificationsLock);

	// First close the file in the SUB_VIEW as closing a file may lead to a single view mode
	// and if that happens we want to be in single main view
	cmpPair->getFileByViewId(SUB_VIEW).close();
	cmpPair->getFileByViewId(MAIN_VIEW).close();

	compareList.erase(cmpPair);

	if (::IsWindowVisible(currentView))
		::SetFocus(currentView);

	onBufferActivated(getCurrentBuffId());
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
						PLUGIN_NAME, MB_OK);
				return false;
			}

			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0,
					Settings.CompareToPrev ? IDM_VIEW_TAB_PREV : IDM_VIEW_TAB_NEXT);
		}
		else
		{
			// Check if the file in the other view is compared already
			if (isFileCompared(getOtherViewId()))
				return false;

			// Check if comparing to cloned self
			if (getDocId(MAIN_VIEW) == getDocId(SUB_VIEW))
			{
				::MessageBox(nppData._nppHandle, TEXT("Trying to compare file to its clone - operation ignored."),
						PLUGIN_NAME, MB_OK);
				return false;
			}

			::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SWITCHTO_OTHER_VIEW);
		}
	}

	newCompare->pair.file[1].initFromCurrent(!newCompare->pair.file[0].isNew);

	return true;
}


CompareList_t::iterator addComparePair()
{
	compareList.push_back(newCompare->pair);
	newCompare.reset();

	return compareList.end() - 1;
}


CompareResult runCompare(CompareList_t::iterator cmpPair)
{
	setStyles(Settings);

	const TCHAR* newName = ::PathFindFileName(cmpPair->getNewFile().name);
	const TCHAR* oldName = ::PathFindFileName(cmpPair->getOldFile().name);

	TCHAR progressInfo[MAX_PATH];
	_sntprintf_s(progressInfo, _countof(progressInfo), _TRUNCATE, cmpPair->options.selectionCompare ?
			TEXT("Comparing selected lines in \"%s\" vs. selected lines in \"%s\"...") :
			TEXT("Comparing \"%s\" vs. \"%s\"..."), newName, oldName);

	return compareViews(cmpPair->options, progressInfo, cmpPair->alignmentInfo);
}


void compare(bool selectionCompare = false, bool findUniqueMode = false, bool autoUpdating = false)
{
	delayedUpdate.cancel();

	ScopedIncrementer incr(notificationsLock);

	const bool				doubleView		= !isSingleView();
	const LRESULT			currentBuffId	= getCurrentBuffId();
	CompareList_t::iterator	cmpPair			= getCompare(currentBuffId);
	const bool				recompare		= (cmpPair != compareList.end());

	// Just to be sure any old state is cleared
	storedLocation.reset();
	goToFirst = false;

	if (recompare)
	{
		newCompare.reset();

		cmpPair->autoUpdateDelay = 0;

		if (!autoUpdating && selectionCompare && !areSelectionsValid())
			return;

		// if (!cmpPair->options.selectionCompare && (!Settings.GotoFirstDiff || autoUpdating) && !selectionCompare)
		if (!Settings.GotoFirstDiff || autoUpdating)
			storedLocation.reset(new ViewLocation(getCurrentViewId()));

		cmpPair->getOldFile().clear();
		cmpPair->getNewFile().clear();
	}
	// New compare
	else
	{
		if (!initNewCompare())
		{
			newCompare.reset();
			return;
		}

		cmpPair = addComparePair();

		if (cmpPair->getOldFile().isTemp)
		{
			activateBufferID(cmpPair->getNewFile().buffId);
		}
		else
		{
			activateBufferID(currentBuffId);

			if (selectionCompare &&
				!areSelectionsValid(currentBuffId, cmpPair->getOtherFileByBuffId(currentBuffId).buffId))
			{
				compareList.erase(cmpPair);
				return;
			}
		}

		if (Settings.EncodingsCheck && !isEncodingOK(*cmpPair))
		{
			clearComparePair(getCurrentBuffId());
			return;
		}
	}

	// Compare is triggered manually - get/re-get compare settings and position/reposition files
	if (!autoUpdating)
	{
		cmpPair->options.oldFileViewId			= Settings.OldFileViewId;

		cmpPair->options.findUniqueMode			= findUniqueMode;
		cmpPair->options.charPrecision			= Settings.CharPrecision;
		cmpPair->options.ignoreSpaces			= Settings.IgnoreSpaces;
		cmpPair->options.ignoreCase				= Settings.IgnoreCase;
		cmpPair->options.detectMoves			= Settings.DetectMoves;
		cmpPair->options.matchPercentThreshold	= Settings.MatchPercentThreshold;
		cmpPair->options.selectionCompare		= selectionCompare;

		cmpPair->positionFiles();

		if (selectionCompare)
		{
			cmpPair->options.selections[MAIN_VIEW]	= getSelectionLines(MAIN_VIEW);
			cmpPair->options.selections[SUB_VIEW]	= getSelectionLines(SUB_VIEW);
		}
	}

	const CompareResult cmpResult = runCompare(cmpPair);

	switch (cmpResult)
	{
		case CompareResult::COMPARE_MISMATCH:
		{
			if (Settings.UseNavBar)
				showNavBar();

			NppSettings::get().setCompareMode(true);

			setCompareView(MAIN_VIEW, Settings.colors.blank);
			setCompareView(SUB_VIEW, Settings.colors.blank);

			if (!storedLocation)
			{
				if (!doubleView)
					activateBufferID(cmpPair->getNewFile().buffId);

				if (selectionCompare)
				{
					clearSelection(getCurrentViewId());
					clearSelection(getOtherViewId());
				}

				goToFirst = true;

				// Move the view so the Notepad++ line number area width is updated now and we avoid getting
				// second alignment request on Scintilla paint notification
				for (const AlignmentPair& alignment : cmpPair->alignmentInfo)
				{
					if (alignment.main.diffMask)
					{
						if (!isLineVisible(MAIN_VIEW, alignment.main.line))
							centerAt(MAIN_VIEW, alignment.main.line);

						if (!isLineVisible(SUB_VIEW, alignment.sub.line))
							centerAt(SUB_VIEW, alignment.sub.line);

						break;
					}
				}

				const int wrap = CallScintilla(MAIN_VIEW, SCI_GETWRAPMODE, 0, 0);
				if (wrap != SC_WRAP_NONE)
				{
					CallScintilla(MAIN_VIEW, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
					CallScintilla(SUB_VIEW, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
					CallScintilla(MAIN_VIEW, SCI_SETWRAPMODE, wrap, 0);
					CallScintilla(SUB_VIEW, SCI_SETWRAPMODE, wrap, 0);
				}
			}

			LOGD("COMPARE READY\n");
		}
		return;

		case CompareResult::COMPARE_MATCH:
		{
			const ComparedFile& oldFile = cmpPair->getOldFile();

			const TCHAR* newName = ::PathFindFileName(cmpPair->getNewFile().name);

			TCHAR msg[2 * MAX_PATH];

			int choice = IDNO;

			if (oldFile.isTemp)
			{
				if (recompare)
				{
					_sntprintf_s(msg, _countof(msg), _TRUNCATE,
							TEXT("%s \"%s\" and \"%s\" %s.\n\nTemp file will be closed."),
							selectionCompare ? TEXT("Selections in files") : TEXT("Files"),
							newName, ::PathFindFileName(oldFile.name),
							cmpPair->options.findUniqueMode ? TEXT("do not contain unique lines") : TEXT("match"));
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

				::MessageBox(nppData._nppHandle, msg, cmpPair->options.findUniqueMode ?
						TEXT("Find Unique") : TEXT("Compare"), MB_OK);
			}
			else
			{
				_sntprintf_s(msg, _countof(msg), _TRUNCATE,
						TEXT("%s \"%s\" and \"%s\" %s.%s"),
						selectionCompare ? TEXT("Selections in files") : TEXT("Files"),
						newName, ::PathFindFileName(oldFile.name),
						cmpPair->options.findUniqueMode ? TEXT("do not contain unique lines") : TEXT("match"),
						Settings.PromptToCloseOnMatch ? TEXT("\n\nClose compared files?") : TEXT(""));

				if (Settings.PromptToCloseOnMatch)
					choice = ::MessageBox(nppData._nppHandle, msg,
							cmpPair->options.findUniqueMode ? TEXT("Find Unique") : TEXT("Compare"),
							MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
				else
					::MessageBox(nppData._nppHandle, msg,
							cmpPair->options.findUniqueMode ? TEXT("Find Unique") : TEXT("Compare"), MB_OK);
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

	storedLocation.reset();
}


void SetAsFirst()
{
	if (!setFirst(!Settings.OldFileIsFirst, true))
		newCompare.reset();
}


void CompareWhole()
{
	compare(false, false);
}


void CompareSelections()
{
	compare(true, false);
}


void FindUnique()
{
	compare(false, true);
}


void FindSelectionsUnique()
{
	compare(true, true);
}


void ClearActiveCompare()
{
	newCompare.reset();

	if (NppSettings::get().compareMode)
		clearComparePair(getCurrentBuffId());
}


void ClearAllCompares()
{
	newCompare.reset();

	if (!compareList.size())
		return;

	const LRESULT buffId = getCurrentBuffId();

	ScopedIncrementer incr(notificationsLock);

	::SetFocus(getOtherView());

	const LRESULT otherBuffId = getCurrentBuffId();

	for (int i = static_cast<int>(compareList.size()) - 1; i >= 0; --i)
		compareList[i].restoreFiles();

	compareList.clear();

	NppSettings::get().setNormalMode(true);

	if (!isSingleView())
		activateBufferID(otherBuffId);

	activateBufferID(buffId);
}


void LastSaveDiff()
{
	TCHAR file[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (!checkFileExists(file))
		return;

	if (createTempFile(file, LAST_SAVED_TEMP))
		compare(false, false);
}


void SvnDiff()
{
	TCHAR file[MAX_PATH];
	TCHAR svnFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (!checkFileExists(file))
		return;

	if (!GetSvnFile(file, svnFile, _countof(svnFile)))
		return;

	if (createTempFile(svnFile, SVN_TEMP))
		compare(false, false);
}


void GitDiff()
{
	TCHAR file[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (!checkFileExists(file))
		return;

	std::vector<char> content = GetGitFileContent(file);

	if (content.empty())
		return;

	if (!createTempFile(file, GIT_TEMP))
		return;

	setContent(content.data());
	content.clear();

	compare(false, false);
}


void IgnoreSpaces()
{
	Settings.IgnoreSpaces = !Settings.IgnoreSpaces;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreSpaces);
	Settings.markAsDirty();
}


void IgnoreCase()
{
	Settings.IgnoreCase = !Settings.IgnoreCase;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_CASE]._cmdID,
			(LPARAM)Settings.IgnoreCase);
	Settings.markAsDirty();
}


void DetectMoves()
{
	Settings.DetectMoves = !Settings.DetectMoves;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMoves);
	Settings.markAsDirty();
}


void HideMatches()
{
	Settings.HideMatches = !Settings.HideMatches;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_HIDE_MATCHES]._cmdID,
			(LPARAM)Settings.HideMatches);
	Settings.markAsDirty();

	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

	if (cmpPair != compareList.end())
	{
		ScopedIncrementer incr(notificationsLock);

		const int view = getCurrentViewId();
		int currentLine = getCurrentLine(view);

		if (Settings.HideMatches && !isLineMarked(view, currentLine, MARKER_MASK_LINE))
		{
			currentLine = CallScintilla(view, SCI_MARKERNEXT, currentLine, MARKER_MASK_LINE);

			if (!isLineVisible(view, currentLine))
				centerAt(view, currentLine);

			CallScintilla(view, SCI_GOTOLINE, currentLine, 0);
		}

		ViewLocation loc;

		const int firstLine = isLineVisible(view, currentLine) ? -1 : getFirstLine(view);

		if (firstLine == -1)
			loc.save(view);

		CallScintilla(MAIN_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
		CallScintilla(SUB_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);

		alignDiffs(cmpPair);

		if (firstLine == -1)
			loc.restore();
		else
			CallScintilla(view, SCI_SETFIRSTVISIBLELINE, CallScintilla(view, SCI_VISIBLEFROMDOCLINE, firstLine, 0), 0);

		NavDlg.Update();
	}
}


void AutoRecompare()
{
	Settings.RecompareOnChange = !Settings.RecompareOnChange;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_AUTO_RECOMPARE]._cmdID,
			(LPARAM)Settings.RecompareOnChange);
	Settings.markAsDirty();
}


void Prev()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementer incr(notificationsLock);

		std::pair<int, int> viewLoc = jumpToChange(false, Settings.WrapAround);
		storedLocation.reset(new ViewLocation(viewLoc.first, viewLoc.second));
	}
}


void Next()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementer incr(notificationsLock);

		std::pair<int, int> viewLoc = jumpToChange(true, Settings.WrapAround);
		storedLocation.reset(new ViewLocation(viewLoc.first, viewLoc.second));
	}
}


void First()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementer incr(notificationsLock);

		std::pair<int, int> viewLoc = jumpToFirstChange(true);
		storedLocation.reset(new ViewLocation(viewLoc.first, viewLoc.second));
	}
}


void Last()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementer incr(notificationsLock);

		std::pair<int, int> viewLoc = jumpToLastChange(true);
		storedLocation.reset(new ViewLocation(viewLoc.first, viewLoc.second));
	}
}


void OpenSettingsDlg(void)
{
	SettingsDialog SettingsDlg(hInstance, nppData);

	if (SettingsDlg.doDialog(&Settings) == IDOK)
	{
		Settings.save();

		newCompare.reset();

		if (!compareList.empty())
		{
			setStyles(Settings);
			NavDlg.SetColors(Settings.colors);
		}
	}
}


void OpenAboutDlg()
{
#ifdef DLOG

	if (dLogBuf == -1)
	{
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

		dLogBuf = getCurrentBuffId();

		HWND hNppTabBar = NppTabHandleGetter::get(getCurrentViewId());

		if (hNppTabBar)
		{
			TCHAR name[] = { TEXT("CP_debug_log") };

			TCITEM tab;
			tab.mask = TCIF_TEXT;
			tab.pszText = name;

			TabCtrl_SetItem(hNppTabBar, posFromBuffId(dLogBuf), &tab);
		}
	}
	else
	{
		activateBufferID(dLogBuf);
	}

	const int view = getCurrentViewId();

	CallScintilla(view, SCI_APPENDTEXT, dLog.size(), (LPARAM)dLog.c_str());
	CallScintilla(view, SCI_SETSAVEPOINT, 0, 0);

	dLog.clear();

#else

	AboutDialog AboutDlg(hInstance, nppData);
	AboutDlg.doDialog();

#endif
}


void createMenu()
{
	_tcscpy_s(funcItem[CMD_SET_FIRST]._itemName, nbChar, TEXT("Set as First to Compare"));
	funcItem[CMD_SET_FIRST]._pFunc					= SetAsFirst;
	funcItem[CMD_SET_FIRST]._pShKey					= new ShortcutKey;
	funcItem[CMD_SET_FIRST]._pShKey->_isAlt			= true;
	funcItem[CMD_SET_FIRST]._pShKey->_isCtrl		= true;
	funcItem[CMD_SET_FIRST]._pShKey->_isShift		= false;
	funcItem[CMD_SET_FIRST]._pShKey->_key			= '1';

	_tcscpy_s(funcItem[CMD_COMPARE]._itemName, nbChar, TEXT("Compare"));
	funcItem[CMD_COMPARE]._pFunc					= CompareWhole;
	funcItem[CMD_COMPARE]._pShKey					= new ShortcutKey;
	funcItem[CMD_COMPARE]._pShKey->_isAlt			= true;
	funcItem[CMD_COMPARE]._pShKey->_isCtrl			= true;
	funcItem[CMD_COMPARE]._pShKey->_isShift			= false;
	funcItem[CMD_COMPARE]._pShKey->_key				= 'C';

	_tcscpy_s(funcItem[CMD_COMPARE_SEL]._itemName, nbChar, TEXT("Compare Selections"));
	funcItem[CMD_COMPARE_SEL]._pFunc				= CompareSelections;
	funcItem[CMD_COMPARE_SEL]._pShKey				= new ShortcutKey;
	funcItem[CMD_COMPARE_SEL]._pShKey->_isAlt		= true;
	funcItem[CMD_COMPARE_SEL]._pShKey->_isCtrl		= true;
	funcItem[CMD_COMPARE_SEL]._pShKey->_isShift		= false;
	funcItem[CMD_COMPARE_SEL]._pShKey->_key			= 'N';

	_tcscpy_s(funcItem[CMD_FIND_UNIQUE]._itemName, nbChar, TEXT("Find Unique Lines"));
	funcItem[CMD_FIND_UNIQUE]._pFunc				= FindUnique;
	funcItem[CMD_FIND_UNIQUE]._pShKey				= new ShortcutKey;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_isAlt		= true;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_isCtrl		= true;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_isShift		= true;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_key			= 'C';

	_tcscpy_s(funcItem[CMD_FIND_UNIQUE_SEL]._itemName, nbChar, TEXT("Find Unique Lines in Selections"));
	funcItem[CMD_FIND_UNIQUE_SEL]._pFunc			= FindSelectionsUnique;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey			= new ShortcutKey;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_isAlt	= true;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_isCtrl	= true;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_isShift	= true;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_key		= 'N';

	_tcscpy_s(funcItem[CMD_CLEAR_ACTIVE]._itemName, nbChar, TEXT("Clear Active Compare"));
	funcItem[CMD_CLEAR_ACTIVE]._pFunc				= ClearActiveCompare;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey 				= new ShortcutKey;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isAlt 		= true;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isCtrl		= true;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isShift	= false;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_key 		= 'X';

	_tcscpy_s(funcItem[CMD_CLEAR_ALL]._itemName, nbChar, TEXT("Clear All Compares"));
	funcItem[CMD_CLEAR_ALL]._pFunc	= ClearAllCompares;

	_tcscpy_s(funcItem[CMD_LAST_SAVE_DIFF]._itemName, nbChar, TEXT("Diff since last Save"));
	funcItem[CMD_LAST_SAVE_DIFF]._pFunc				= LastSaveDiff;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey 			= new ShortcutKey;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isAlt 	= true;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_key 		= 'D';

	_tcscpy_s(funcItem[CMD_SVN_DIFF]._itemName, nbChar, TEXT("SVN Diff"));
	funcItem[CMD_SVN_DIFF]._pFunc 					= SvnDiff;
	funcItem[CMD_SVN_DIFF]._pShKey 					= new ShortcutKey;
	funcItem[CMD_SVN_DIFF]._pShKey->_isAlt 			= true;
	funcItem[CMD_SVN_DIFF]._pShKey->_isCtrl 		= true;
	funcItem[CMD_SVN_DIFF]._pShKey->_isShift		= false;
	funcItem[CMD_SVN_DIFF]._pShKey->_key 			= 'V';

	_tcscpy_s(funcItem[CMD_GIT_DIFF]._itemName, nbChar, TEXT("Git Diff"));
	funcItem[CMD_GIT_DIFF]._pFunc 					= GitDiff;
	funcItem[CMD_GIT_DIFF]._pShKey 					= new ShortcutKey;
	funcItem[CMD_GIT_DIFF]._pShKey->_isAlt 			= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isCtrl 		= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isShift		= false;
	funcItem[CMD_GIT_DIFF]._pShKey->_key 			= 'G';

	_tcscpy_s(funcItem[CMD_IGNORE_SPACES]._itemName, nbChar, TEXT("Ignore Spaces"));
	funcItem[CMD_IGNORE_SPACES]._pFunc = IgnoreSpaces;

	_tcscpy_s(funcItem[CMD_IGNORE_CASE]._itemName, nbChar, TEXT("Ignore Case"));
	funcItem[CMD_IGNORE_CASE]._pFunc = IgnoreCase;

	_tcscpy_s(funcItem[CMD_DETECT_MOVES]._itemName, nbChar, TEXT("Detect Moves"));
	funcItem[CMD_DETECT_MOVES]._pFunc = DetectMoves;

	_tcscpy_s(funcItem[CMD_HIDE_MATCHES]._itemName, nbChar, TEXT("Hide Matches (Show Only Diffs)"));
	funcItem[CMD_HIDE_MATCHES]._pFunc = HideMatches;

	_tcscpy_s(funcItem[CMD_NAV_BAR]._itemName, nbChar, TEXT("Navigation Bar"));
	funcItem[CMD_NAV_BAR]._pFunc = ViewNavigationBar;

	_tcscpy_s(funcItem[CMD_AUTO_RECOMPARE]._itemName, nbChar, TEXT("Auto Re-Compare on Change"));
	funcItem[CMD_AUTO_RECOMPARE]._pFunc = AutoRecompare;

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

#ifdef DLOG
	_tcscpy_s(funcItem[CMD_ABOUT]._itemName, nbChar, TEXT("Show debug log"));
#else
	_tcscpy_s(funcItem[CMD_ABOUT]._itemName, nbChar, TEXT("Help / About..."));
#endif
	funcItem[CMD_ABOUT]._pFunc = OpenAboutDlg;
}


void deinitPlugin()
{
	// Always close it, else N++'s plugin manager would call 'ViewNavigationBar'
	// on startup, when N++ has been shut down before with opened navigation bar
	if (NavDlg.isVisible())
		NavDlg.Hide();

	if (tbSetFirst.hToolbarBmp)
		::DeleteObject(tbSetFirst.hToolbarBmp);

	if (tbCompare.hToolbarBmp)
		::DeleteObject(tbCompare.hToolbarBmp);

	if (tbCompareSel.hToolbarBmp)
		::DeleteObject(tbCompareSel.hToolbarBmp);

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

	if (tbDiffsOnly.hToolbarBmp)
		::DeleteObject(tbDiffsOnly.hToolbarBmp);

	if (tbNavBar.hToolbarBmp)
		::DeleteObject(tbNavBar.hToolbarBmp);

	NavDlg.destroy();

	// Deallocate shortcut
	for (int i = 0; i < NB_MENU_COMMANDS; i++)
	{
		if (funcItem[i]._pShKey != NULL)
        {
			delete funcItem[i]._pShKey;
            funcItem[i]._pShKey = NULL;
        }
	}
}


void syncViews(int biasView)
{
	const int otherView = getOtherViewId(biasView);

	const int firstVisible = getFirstVisibleLine(biasView);
	const int otherFirstVisible = getFirstVisibleLine(otherView);

	const int firstLine = CallScintilla(biasView, SCI_DOCLINEFROMVISIBLE, firstVisible, 0);

	int otherLine = -1;

	if (firstLine < CallScintilla(biasView, SCI_GETLINECOUNT, 0, 0) - 1)
	{
		if (firstVisible != otherFirstVisible)
		{
			LOGD("Syncing to " + std::string(biasView == MAIN_VIEW ? "MAIN" : "SUB") + " view, visible doc line: " +
					std::to_string(firstLine + 1) + "\n");

			const int otherLastVisible = CallScintilla(otherView, SCI_VISIBLEFROMDOCLINE,
					CallScintilla(otherView, SCI_GETLINECOUNT, 0, 0) - 1, 0);

			otherLine = (firstVisible > otherLastVisible) ? otherLastVisible : firstVisible;
		}
	}
	else if (firstVisible > otherFirstVisible)
	{
		otherLine = firstVisible;
	}

	if (otherLine >= 0)
	{
		ScopedIncrementer incr(notificationsLock);

		CallScintilla(otherView, SCI_SETFIRSTVISIBLELINE, otherLine, 0);

		::UpdateWindow(getView(otherView));
	}

	if (Settings.FollowingCaret && biasView == getCurrentViewId())
	{
		const int line = getCurrentLine(biasView);

		otherLine = otherViewMatchingLine(biasView, line);

		if ((otherLine != getCurrentLine(otherView)) && !isSelection(otherView))
		{
			int pos;

			if (isLineAnnotated(otherView, otherLine) && isLineWrapped(otherView, otherLine))
				pos = CallScintilla(otherView, SCI_GETLINEENDPOSITION, otherLine, 0);
			else
				pos = CallScintilla(otherView, SCI_POSITIONFROMLINE, otherLine, 0);

			ScopedIncrementer incr(notificationsLock);

			CallScintilla(otherView, SCI_SETEMPTYSELECTION, pos, 0);

			::UpdateWindow(getView(otherView));
		}
	}

	NavDlg.Update();
}


void comparedFileActivated()
{
	if (!NppSettings::get().compareMode)
	{
		if (Settings.UseNavBar && !NavDlg.isVisible())
			showNavBar();

		ScopedIncrementer incr(notificationsLock);

		NppSettings::get().setCompareMode();
	}

	CallScintilla(MAIN_VIEW,	SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);
	CallScintilla(SUB_VIEW,		SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);

	setArrowMark(-1);

	setCompareView(MAIN_VIEW,	Settings.colors.blank);
	setCompareView(SUB_VIEW,	Settings.colors.blank);

	if (Settings.HideMatches)
	{
		CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

		if (cmpPair != compareList.end())
		{
			ScopedIncrementer incr(notificationsLock);

			alignDiffs(cmpPair);
		}
	}
}


void onToolBarReady()
{
	UINT style = (LR_SHARED | LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);

	if (isRTLwindow(nppData._nppHandle))
		tbSetFirst.hToolbarBmp =
				(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST_RTL), IMAGE_BITMAP, 0, 0, style);
	else
		tbSetFirst.hToolbarBmp =
				(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST), IMAGE_BITMAP, 0, 0, style);

	tbCompare.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE), IMAGE_BITMAP, 0, 0, style);
	tbCompareSel.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE_LINES), IMAGE_BITMAP, 0, 0, style);
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
	tbDiffsOnly.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_DIFFS_ONLY), IMAGE_BITMAP, 0, 0, style);
	tbNavBar.hToolbarBmp =
			(HBITMAP)::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NAVBAR), IMAGE_BITMAP, 0, 0, style);

	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_SET_FIRST]._cmdID,			(LPARAM)&tbSetFirst);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_COMPARE]._cmdID,			(LPARAM)&tbCompare);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_COMPARE_SEL]._cmdID,		(LPARAM)&tbCompareSel);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_CLEAR_ACTIVE]._cmdID,		(LPARAM)&tbClearCompare);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_FIRST]._cmdID,				(LPARAM)&tbFirst);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_PREV]._cmdID,				(LPARAM)&tbPrev);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_NEXT]._cmdID,				(LPARAM)&tbNext);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_LAST]._cmdID,				(LPARAM)&tbLast);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_HIDE_MATCHES]._cmdID,		(LPARAM)&tbDiffsOnly);
	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
			(WPARAM)funcItem[CMD_NAV_BAR]._cmdID,			(LPARAM)&tbNavBar);
}


void onNppReady()
{
	// It's N++'s job actually to disable its scroll menu commands but since it's not the case provide this as a patch
	if (isSingleView())
		NppSettings::get().enableNppScrollCommands(false);

	NppSettings::get().updatePluginMenu();

	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreSpaces);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_CASE]._cmdID,
			(LPARAM)Settings.IgnoreCase);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMoves);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_HIDE_MATCHES]._cmdID,
			(LPARAM)Settings.HideMatches);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID,
			(LPARAM)Settings.UseNavBar);

	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_AUTO_RECOMPARE]._cmdID,
			(LPARAM)Settings.RecompareOnChange);
}


void DelayedAlign::operator()()
{
	const LRESULT			currentBuffId	= getCurrentBuffId();
	CompareList_t::iterator	cmpPair			= getCompare(currentBuffId);

	if (cmpPair == compareList.end())
		return;

	if (cmpPair->autoUpdateDelay)
	{
        delayedUpdate.post(cmpPair->autoUpdateDelay);

		return;
	}

	const AlignmentInfo_t& alignmentInfo = cmpPair->alignmentInfo;
	if (alignmentInfo.empty())
		return;

	bool realign = goToFirst;

	ScopedIncrementer incr(notificationsLock);

	if (!realign)
	{
		const int view = storedLocation ? storedLocation->getView() : getCurrentViewId();

		realign = isAlignmentNeeded(view, alignmentInfo);
	}

	if (realign)
	{
		LOGD("Aligning diffs\n");

		if (!storedLocation && !goToFirst)
			storedLocation.reset(new ViewLocation(getCurrentViewId()));

		alignDiffs(cmpPair);
	}

	if (goToFirst)
	{
		LOGD("Go to first diff\n");

		goToFirst = false;

		std::pair<int, int> viewLoc = jumpToFirstChange(true, true);

		if (viewLoc.first >= 0)
			syncViews(viewLoc.first);

		cmpPair->setStatus();

		::SetFocus(getCurrentView());
	}
	else if (storedLocation)
	{
		if (realign)
			++_consecutiveAligns;
		else
			_consecutiveAligns = 0;

		if (_consecutiveAligns > 1)
			_consecutiveAligns = 0;

		if (storedLocation->restore())
			syncViews(storedLocation->getView());

		// Retry re-alignment one more time - might be needed in case line number margin width has changed
		if (_consecutiveAligns)
		{
			post(30);
		}
		else
		{
			storedLocation.reset();
			cmpPair->setStatus();

			::SetFocus(getCurrentView());
		}
	}
	else if (cmpPair->options.findUniqueMode)
	{
		syncViews(getCurrentViewId());
	}
}


inline void onSciPaint()
{
	delayedAlignment.post(30);
}


void onSciUpdateUI(HWND view)
{
	ScopedIncrementer incr(notificationsLock);

	syncViews(getViewId(view));
}


void DelayedUpdate::operator()()
{
	compare(false, false, true);
}


void onSciModified(SCNotification* notifyCode)
{
	static bool skipPushDeletedSection = false;

	const int view = getViewId((HWND)notifyCode->nmhdr.hwndFrom);

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(view));
	if (cmpPair == compareList.end())
		return;

	if (notifyCode->modificationType & SC_MOD_BEFOREDELETE)
	{
		const int startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);
		const int endLine =
			CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position + notifyCode->length, 0);

		// Change is on single line?
		if (endLine <= startLine)
			return;

		if (!skipPushDeletedSection)
		{
			LOGD("SC_MOD_BEFOREDELETE: " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
					" view, lines range: " + std::to_string(startLine + 1) + "-" + std::to_string(endLine) + "\n");

			const int action =
				notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

			ScopedIncrementer incr(notificationsLock);

			if (cmpPair->getFileByViewId(view).pushDeletedSection(action, startLine, endLine - startLine))
				return;
		}
	}
	else if ((notifyCode->modificationType & SC_MOD_DELETETEXT) && notifyCode->linesAdded)
	{
		if (!skipPushDeletedSection)
		{
			if (cmpPair->getFileByViewId(view).redoAlignmentBlankDeletion())
				return;
		}
		else
		{
			skipPushDeletedSection = false;
			return;
		}
	}
	else if (notifyCode->modificationType & SC_MOD_INSERTTEXT)
	{
		const int startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);

		if (startLine <= CallScintilla(view, SCI_MARKERNEXT, 0, MARKER_MASK_BLANK))
		{
			if (notifyCode->linesAdded)
				skipPushDeletedSection = true;

			::PostMessage(getView(view), SCI_UNDO, 0, 0);

			return;
		}
		else if (notifyCode->linesAdded)
		{
			const int action =
				notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

			LOGD("SC_MOD_INSERTTEXT: " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
					" view, lines range: " + std::to_string(startLine + 1) + "-" +
					std::to_string(startLine + notifyCode->linesAdded) + "\n");

			ScopedIncrementer incr(notificationsLock);

			cmpPair->getFileByViewId(view).popDeletedSection(action, startLine);
		}
	}

	if ((notifyCode->modificationType & SC_MOD_DELETETEXT) || (notifyCode->modificationType & SC_MOD_INSERTTEXT))
	{
		delayedAlignment.cancel();
		delayedUpdate.cancel();

		if (cmpPair->options.selectionCompare && notifyCode->linesAdded)
		{
			const int startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);
			const int endLine = startLine + notifyCode->linesAdded - 1;

			if (cmpPair->options.selections[view].first > startLine)
			{
				if (cmpPair->options.selections[view].first > endLine)
					cmpPair->options.selections[view].first += notifyCode->linesAdded;
				else
					cmpPair->options.selections[view].first += (cmpPair->options.selections[view].first - startLine);
			}

			if (cmpPair->options.selections[view].second > startLine)
			{
				if (cmpPair->options.selections[view].second > endLine)
					cmpPair->options.selections[view].second += notifyCode->linesAdded;
				else
					cmpPair->options.selections[view].second += (cmpPair->options.selections[view].second - startLine);
			}
		}

		if (Settings.RecompareOnChange)
		{
			if (notifyCode->linesAdded)
				cmpPair->autoUpdateDelay = 400;
			else
				// Leave bigger delay before re-compare if change is on single line because the user might be typing
				// and we shouldn't interrupt / interfere
				cmpPair->autoUpdateDelay = 1000;
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
	const int zoom = CallScintilla(getCurrentViewId(), SCI_GETZOOM, 0, 0);
	CallScintilla(getOtherViewId(), SCI_SETZOOM, zoom, 0);

	NppSettings::get().setCompareZoom(zoom);
}


void DelayedActivate::operator()()
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	LOGDB(buffId, "Activate\n");

	storedLocation.reset(new ViewLocation(viewIdFromBuffId(buffId)));

	const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

	// When compared file is activated make sure its corresponding pair file is also active in the other view
	if (getDocId(getOtherViewId()) != otherFile.sciDoc)
	{
		ScopedIncrementer incr(notificationsLock);

		activateBufferID(otherFile.buffId);
		activateBufferID(buffId);
	}

	comparedFileActivated();
}


void onBufferActivated(LRESULT buffId)
{
	delayedAlignment.cancel();
	delayedUpdate.cancel();
	delayedActivation.cancel();

	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
	{
		NppSettings::get().setNormalMode();
		setNormalView(getCurrentViewId());
		resetCompareView(getOtherViewId());
	}
	else
	{
		delayedActivation.buffId = buffId;
		delayedActivation.post(30);
	}
}


void DelayedClose::operator()()
{
	const LRESULT currentBuffId = getCurrentBuffId();

	ScopedIncrementer incr(notificationsLock);

	for (int i = static_cast<int>(closedBuffs.size()) - 1; i >= 0; --i)
	{
		CompareList_t::iterator cmpPair = getCompare(closedBuffs[i]);
		if (cmpPair == compareList.end())
			continue;

		ComparedFile& closedFile = cmpPair->getFileByBuffId(closedBuffs[i]);
		ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(closedBuffs[i]);

		if (closedFile.isTemp && closedFile.isOpen())
			closedFile.close();

		if (otherFile.isTemp)
		{
			if (otherFile.isOpen())
			{
				LOGDB(otherFile.buffId, "Close\n");

				otherFile.close();
			}
		}
		else
		{
			if (otherFile.isOpen())
				otherFile.restore();
		}

		compareList.erase(cmpPair);
	}

	closedBuffs.clear();

	activateBufferID(currentBuffId);
	onBufferActivated(currentBuffId);

	// If it is the last file and it is not in the main view - move it there
	if (getNumberOfFiles() == 1 && getCurrentViewId() == SUB_VIEW)
	{
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

		const LRESULT newBuffId = getCurrentBuffId();

		activateBufferID(currentBuffId);
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);
		activateBufferID(newBuffId);
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
	}
}


void onFileBeforeClose(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	delayedAlignment.cancel();
	delayedUpdate.cancel();
	delayedActivation.cancel();

	delayedClosure.cancel();
	delayedClosure.closedBuffs.push_back(buffId);

	const LRESULT currentBuffId = getCurrentBuffId();

	ScopedIncrementer incr(notificationsLock);

	ComparedFile& closedFile = cmpPair->getFileByBuffId(buffId);
	closedFile.onBeforeClose();

	if (cmpPair->relativePos && (closedFile.originalViewId == viewIdFromBuffId(buffId)))
	{
		ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

		otherFile.originalPos = posFromBuffId(buffId) + cmpPair->relativePos;

		if (cmpPair->relativePos > 0)
			--otherFile.originalPos;
		else
			++otherFile.originalPos;

		if (otherFile.originalPos < 0)
			otherFile.originalPos = 0;
	}

	if (currentBuffId != buffId)
		activateBufferID(currentBuffId);

	delayedClosure.post(30);
}


void onFileBeforeSave(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

    const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

    const LRESULT currentBuffId = getCurrentBuffId();
    const bool pairIsActive = (currentBuffId == buffId || currentBuffId == otherFile.buffId);

	ScopedIncrementer incr(notificationsLock);

    if (!pairIsActive)
        activateBufferID(buffId);

	const int currentView = viewIdFromBuffId(buffId);
    ComparedFile& currentFile = cmpPair->getFileByBuffId(buffId);
	currentFile.restoreAlignmentLine = isAlignmentFirstLineInserted(currentView);

	if (currentFile.restoreAlignmentLine)
		removeAlignmentFirstLine(currentView);

    if (!pairIsActive)
        activateBufferID(currentBuffId);
}


void onFileSaved(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

    const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

    const LRESULT currentBuffId = getCurrentBuffId();
    const bool pairIsActive = (currentBuffId == buffId || currentBuffId == otherFile.buffId);

	ScopedIncrementer incr(notificationsLock);

    if (!pairIsActive)
        activateBufferID(buffId);

    ComparedFile& currentFile = cmpPair->getFileByBuffId(buffId);

	if (currentFile.restoreAlignmentLine)
	{
		currentFile.restoreAlignmentLine = false;

		const int view = viewIdFromBuffId(buffId);

		CallScintilla(view, SCI_UNDO, 0, 0);
		CallScintilla(view, SCI_MARKERADDSET, 0, MARKER_MASK_BLANK);
		CallScintilla(view, SCI_SETSAVEPOINT, 0, 0);
	}

    if (pairIsActive && Settings.RecompareOnChange && cmpPair->autoUpdateDelay)
    {
        delayedAlignment.cancel();
        delayedUpdate.post(30);
    }

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

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

            TabCtrl_SetItem(hNppTabBar, tabPos, &tab);

			::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
        }
    }

    if (!pairIsActive)
    {
        activateBufferID(currentBuffId);
        onBufferActivated(currentBuffId);
    }
}


void DelayedMaximize::operator()()
{
	isNppMinimized = false;

	if (notificationsLock)
		--notificationsLock;

	::SetFocus(getCurrentView());

	NavDlg.Update();
}

} // anonymous namespace


void SetLocation(int view, int line)
{
	storedLocation.reset(new ViewLocation(view, line));
}


void ViewNavigationBar()
{
	Settings.UseNavBar = !Settings.UseNavBar;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID,
			(LPARAM)Settings.UseNavBar);
	Settings.markAsDirty();

	if (NppSettings::get().compareMode)
	{
		if (Settings.UseNavBar)
			showNavBar();
		else
			NavDlg.Hide();
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
	nppData		= notpadPlusData;
	sciFunc		= (SciFnDirect)::SendMessage(notpadPlusData._scintillaMainHandle, SCI_GETDIRECTFUNCTION, 0, 0);
	sciPtr[0]	= (sptr_t)::SendMessage(notpadPlusData._scintillaMainHandle, SCI_GETDIRECTPOINTER, 0, 0);
	sciPtr[1]	= (sptr_t)::SendMessage(notpadPlusData._scintillaSecondHandle, SCI_GETDIRECTPOINTER, 0, 0);

	if (!sciFunc || !sciPtr[0] || !sciPtr[1])
	{
		::MessageBox(notpadPlusData._nppHandle,
				TEXT("Error getting direct Scintilla call pointers, plugin init failed!"),
				PLUGIN_NAME, MB_OK | MB_ICONERROR);

		exit(EXIT_FAILURE);
	}

	// Check just in case
	assert(MAIN_VIEW == 0 && SUB_VIEW == 1);

	Settings.load();

	NavDlg.init(hInstance);
}


extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return PLUGIN_NAME;
}


extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int* nbF)
{
	*nbF = NB_MENU_COMMANDS;
	return funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode)
{
	switch (notifyCode->nmhdr.code)
	{
		// Handle wrap refresh
		case SCN_PAINTED:
			if (NppSettings::get().compareMode && !notificationsLock &&
					!delayedActivation && !delayedClosure && !delayedUpdate)
				onSciPaint();
		break;

		// Vertical scroll sync
		case SCN_UPDATEUI:
			if (isNppMinimized)
			{
				delayedMaximize.cancel();
				delayedMaximize.post(500);
			}
			else if (NppSettings::get().compareMode && !notificationsLock && !storedLocation && !goToFirst &&
				!delayedActivation && !delayedClosure && !delayedUpdate &&
				(notifyCode->updated & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL)))
			{
				onSciUpdateUI((HWND)notifyCode->nmhdr.hwndFrom);
			}
		break;

		case NPPN_BUFFERACTIVATED:
			if (!compareList.empty() && !notificationsLock && !delayedClosure)
				onBufferActivated(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILEBEFORECLOSE:
			if (newCompare && (newCompare->pair.file[0].buffId == static_cast<LRESULT>(notifyCode->nmhdr.idFrom)))
				newCompare.reset();
#ifdef DLOG
			else if (dLogBuf == static_cast<LRESULT>(notifyCode->nmhdr.idFrom))
				dLogBuf = -1;
#endif
			else if (!compareList.empty() && !notificationsLock)
				onFileBeforeClose(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILEBEFORESAVE:
			if (!compareList.empty() && !notificationsLock)
				onFileBeforeSave(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILESAVED:
			if (!compareList.empty() && !notificationsLock)
				onFileSaved(notifyCode->nmhdr.idFrom);
		break;

		// This is used to monitor deletion of lines to properly clear their compare markings
		case SCN_MODIFIED:
			if (NppSettings::get().compareMode && !notificationsLock)
				onSciModified(notifyCode);
		break;

		case SCN_ZOOM:
			if (!notificationsLock)
			{
				if (NppSettings::get().compareMode)
				{
					onSciZoom();
				}
				else
				{
					NppSettings::get().setMainZoom(CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0));
					NppSettings::get().setSubZoom(CallScintilla(SUB_VIEW, SCI_GETZOOM, 0, 0));
				}
			}
		break;

		case NPPN_WORDSTYLESUPDATED:
			setStyles(Settings);
			NavDlg.SetColors(Settings.colors);
		break;

		case NPPN_TBMODIFICATION:
			onToolBarReady();
		break;

		case NPPN_READY:
			onNppReady();
		break;

		case NPPN_BEFORESHUTDOWN:
			ClearAllCompares();
		break;

		case NPPN_SHUTDOWN:
			Settings.save();
			deinitPlugin();
		break;
	}
}


extern "C" __declspec(dllexport) LRESULT messageProc(UINT msg, WPARAM wParam, LPARAM)
{
	if (msg == WM_SIZE)
	{
		if (wParam == SIZE_MINIMIZED)
		{
			if (!isNppMinimized && NppSettings::get().compareMode)
			{
				// On rare occasions Alignment is posted (Sci paint event is received)
				// before minimize event is received
				delayedAlignment.cancel();

				isNppMinimized = true;
				++notificationsLock;
			}
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			if (isNppMinimized && !delayedMaximize)
				delayedMaximize.post(500);
		}
	}

	return TRUE;
}


extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
