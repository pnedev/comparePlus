/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2022 Pavel Nedev (pg.nedev@gmail.com)
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
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cmath>
#include <cwchar>
#include <ctime>

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
#include "IgnoreRegexDialog.h"
#include "NavDialog.h"
#include "Engine.h"
#include "NppInternalDefines.h"
#include "resource.h"


const TCHAR PLUGIN_NAME[] = TEXT("ComparePlus");

NppData			nppData;
SciFnDirect		sciFunc;
sptr_t			sciPtr[2];

UserSettings	Settings;

int gMarginWidth = 0;

#ifdef DLOG

std::string		dLog("ComparePlus debug log\n\n");
DWORD			dLogTime_ms = 0;
static LRESULT	dLogBuf = -1;

#endif


namespace // anonymous namespace
{

constexpr int MIN_NOTEPADPP_VERSION_MAJOR = 8;
constexpr int MIN_NOTEPADPP_VERSION_MINOR = 420;

constexpr int MIN_NOTEPADPP_VERSION = ((MIN_NOTEPADPP_VERSION_MAJOR << 16) | MIN_NOTEPADPP_VERSION_MINOR);


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

	bool compareMode;

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

	int		_lineNumMode;

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
	/**
	 *  \struct
	 *  \brief
	 */
	struct UndoData
	{
		AlignmentInfo_t					alignment;
		std::pair<intptr_t, intptr_t>	selection {-1, -1};
		std::vector<int>				otherViewMarks;
	};

	DeletedSection(int action, intptr_t line, const std::shared_ptr<UndoData>& undo) :
			startLine(line), lineReplace(false), nextLineMarker(0), undoInfo(undo)
	{
		restoreAction = (action == SC_PERFORMED_UNDO) ? SC_PERFORMED_REDO : SC_PERFORMED_UNDO;
	}

	intptr_t			startLine;
	bool				lineReplace;
	int					restoreAction;
	std::vector<int>	markers;
	int					nextLineMarker;

	const std::shared_ptr<UndoData> undoInfo;
};


/**
 *  \struct
 *  \brief
 */
struct DeletedSectionsList
{
	DeletedSectionsList() : lastPushTimeMark(0) {}

	std::vector<DeletedSection>& get()
	{
		return sections;
	}

	bool push(int view, int currAction, intptr_t startLine, intptr_t len,
			const std::shared_ptr<DeletedSection::UndoData>& undo, bool recompareOnChange);
	std::shared_ptr<DeletedSection::UndoData> pop(int view, int currAction, intptr_t startLine);

	void clear()
	{
		sections.clear();
	}

private:
	DWORD						lastPushTimeMark;
	std::vector<DeletedSection>	sections;
};


bool DeletedSectionsList::push(int view, int currAction, intptr_t startLine, intptr_t len,
	const std::shared_ptr<DeletedSection::UndoData>& undo, bool recompareOnChange)
{
	if (len < 1)
		return false;

	// Is it line replacement revert operation?
	if (!sections.empty() && sections.back().restoreAction == currAction && sections.back().lineReplace)
		return false;

	DeletedSection delSection(currAction, startLine, undo);

	if (!recompareOnChange)
	{
		delSection.markers = getMarkers(view, startLine, len, MARKER_MASK_ALL);

		if (startLine + len < CallScintilla(view, SCI_GETLINECOUNT, 0, 0))
			delSection.nextLineMarker = CallScintilla(view, SCI_MARKERGET, startLine + len, 0) & MARKER_MASK_ALL;
	}
	else
	{
		clearMarks(view, startLine, len);
	}

	sections.push_back(delSection);

	lastPushTimeMark = ::GetTickCount();

	return true;
}


std::shared_ptr<DeletedSection::UndoData> DeletedSectionsList::pop(int view, int currAction, intptr_t startLine)
{
	if (sections.empty())
		return nullptr;

	DeletedSection& last = sections.back();

	if (last.startLine != startLine)
		return nullptr;

	if (last.restoreAction != currAction)
	{
		// Try to guess if this is the insert part of line replacement operation
		if (::GetTickCount() < lastPushTimeMark + 40)
			last.lineReplace = true;

		return nullptr;
	}

	if (!last.markers.empty())
	{
		setMarkers(view, last.startLine, last.markers);

		if (last.nextLineMarker)
		{
			clearMarks(view, startLine + last.markers.size());
			CallScintilla(view, SCI_MARKERADDSET, startLine + last.markers.size(), last.nextLineMarker);
		}
	}

	std::shared_ptr<DeletedSection::UndoData> undo = last.undoInfo;

	sections.pop_back();

	return undo;
}


enum Temp_t
{
	NO_TEMP = 0,
	LAST_SAVED_TEMP,
	CLIPBOARD_TEMP,
	SVN_TEMP,
	GIT_TEMP
};


enum FoldType_t
{
	NO_FOLD = 0,
	FOLD_MATCHES,
	FOLD_OUTSIDE_SELECTIONS
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
	void clear(bool keepDeleteHistory = false);
	void onBeforeClose() const;
	void close() const;
	void restore() const;
	bool isOpen() const;

	bool pushDeletedSection(int sciAction, intptr_t startLine, intptr_t len,
		const std::shared_ptr<DeletedSection::UndoData>& undo, bool recompareOnChange)
	{
		return deletedSections.push(compareViewId, sciAction, startLine, len, undo, recompareOnChange);
	}

	std::shared_ptr<DeletedSection::UndoData> popDeletedSection(int sciAction, intptr_t startLine)
	{
		return deletedSections.pop(compareViewId, sciAction, startLine);
	}

	Temp_t	isTemp;
	bool	isNew;

	int		originalViewId;
	int		originalPos;
	int		compareViewId;

	LRESULT		buffId;
	intptr_t	sciDoc;
	TCHAR		name[MAX_PATH];

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
	inline ComparedFile& getFileBySciDoc(intptr_t sciDoc);
	inline ComparedFile& getOldFile();
	inline ComparedFile& getNewFile();

	void positionFiles();
	void restoreFiles(LRESULT currentBuffId);

	void setStatus();

	void adjustAlignment(int view, intptr_t line, intptr_t offset);

	void setCompareDirty()
	{
		compareDirty = true;

		if (!inEqualizeMode)
			manuallyChanged = true;
	}

	ComparedFile	file[2];
	int				relativePos;

	CompareOptions	options;

	CompareSummary	summary;

	FoldType_t		foldType		= NO_FOLD;

	bool			compareDirty	= false;
	bool			manuallyChanged	= false;
	int				inEqualizeMode	= 0;

	int				autoUpdateDelay	= 0;
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
class SelectRangeTimeout : public DelayedWork
{
public:
	SelectRangeTimeout(int view, intptr_t startPos, intptr_t endPos) : DelayedWork(), _view(view)
	{
		_sel = getSelection(view);

		setSelection(view, startPos, endPos);
	}

	virtual ~SelectRangeTimeout();

	virtual void operator()();

private:
	int	_view;

	std::pair<intptr_t, intptr_t> _sel;
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
	{ TEXT("Clipboard_"),	TEXT(" ** Clipboard") },
	{ TEXT("_SVN"),			TEXT(" ** SVN") },
	{ TEXT("_Git"),			TEXT(" ** Git") }
};


CompareList_t compareList;
std::unique_ptr<NewCompare> newCompare = nullptr;

int notificationsLock = 0;

std::unique_ptr<ViewLocation> storedLocation = nullptr;
std::vector<int> copiedSectionMarks;

// Re-compare flags
bool goToFirst = false;
bool justCompared = false;
bool selectionAutoRecompare = false;

LRESULT currentlyActiveBuffID = 0;

DelayedAlign	delayedAlignment;
DelayedActivate	delayedActivation;
DelayedClose	delayedClosure;
DelayedUpdate	delayedUpdate;

NavDialog		NavDlg;

toolbarIconsWithDarkMode	tbSetFirst		{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbCompare		{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbCompareSel	{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbClearCompare	{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbFirst			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbPrev			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbNext			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbLast			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbDiffsOnly		{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbNavBar		{nullptr, nullptr, nullptr};

HINSTANCE hInstance;
FuncItem funcItem[NB_MENU_COMMANDS] = { 0 };

// Declare local functions that appear before they are defined
void onBufferActivated(LRESULT buffId);
void syncViews(int biasView);
void temporaryRangeSelect(int view, intptr_t startPos = -1, intptr_t endPos = -1);
void setArrowMark(int view, intptr_t line = -1, bool down = true);
intptr_t getAlignmentIdxAfter(const AlignmentViewData AlignmentPair::*pView, const AlignmentInfo_t &alignInfo,
		intptr_t line);
intptr_t getAlignmentLine(const AlignmentInfo_t &alignInfo, int view, intptr_t line);


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

	::EnableMenuItem(hMenu, funcItem[CMD_BOOKMARK_DIFFS]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_BOOKMARK_ADD_REM]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_BOOKMARK_CHANGED]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_COMPARE_SUMMARY]._cmdID, flag);
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

	_lineNumMode = (int)::SendMessage(nppData._nppHandle, NPPM_GETLINENUMBERWIDTHMODE, 0, 0);

	if (_mainZoom == 0)
		_mainZoom = static_cast<int>(CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0));

	if (_subZoom == 0)
		_subZoom = static_cast<int>(CallScintilla(SUB_VIEW, SCI_GETZOOM, 0, 0));
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

		if (_lineNumMode != LINENUMWIDTH_CONSTANT)
			::SendMessage(nppData._nppHandle, NPPM_SETLINENUMBERWIDTHMODE, 0, _lineNumMode);

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
		CallScintilla(MAIN_VIEW, SCI_GOTOLINE, getCurrentLine(MAIN_VIEW), 0);
		CallScintilla(SUB_VIEW, SCI_GOTOLINE, getCurrentLine(SUB_VIEW), 0);
	}

	// Disable N++ vertical scroll - we handle it manually because of the Word Wrap
	if (_syncVScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

	// Yaron - Enable N++ horizontal scroll sync
	if (!_syncHScroll)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLH);

	if (_lineNumMode != LINENUMWIDTH_CONSTANT)
		::SendMessage(nppData._nppHandle, NPPM_SETLINENUMBERWIDTHMODE, 0, LINENUMWIDTH_CONSTANT);

	// synchronize zoom levels
	if (_compareZoom == 0)
	{
		_compareZoom = static_cast<int>(CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0));
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _compareZoom, 0);
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
	HWND hNppMainTabBar	= NppTabHandleGetter::get(MAIN_VIEW);
	HWND hNppSubTabBar	= NppTabHandleGetter::get(SUB_VIEW);

	if (hNppMainTabBar && hNppSubTabBar)
	{
		HWND currentView = getCurrentView();

		refreshTabBar(hNppSubTabBar);
		refreshTabBar(hNppMainTabBar);

		::SetFocus(currentView);
	}
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

			size_t i = _tcslen(tabName) - 1 - _tcslen(tempMark[isTemp].fileMark);
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
	compareViewId = isNew ? Settings.NewFileViewId : ((Settings.NewFileViewId == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW);
}


void ComparedFile::clear(bool keepDeleteHistory)
{
	temporaryRangeSelect(-1);
	setArrowMark(-1);

	if (!keepDeleteHistory)
		deletedSections.clear();
}


void ComparedFile::onBeforeClose() const
{
	activateBufferID(buffId);

	const int view = getCurrentViewId();

	clearWindow(view);
	setNormalView(view);
	temporaryRangeSelect(-1);
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

	const int view = getCurrentViewId();

	intptr_t biasLine = getFirstLine(view);
	biasLine = CallScintilla(view, SCI_DOCLINEFROMVISIBLE,
			getFirstVisibleLine(view) + getLineAnnotation(view, biasLine) +
			getWrapCount(view, biasLine), 0);

	ViewLocation loc(view, biasLine);

	clearWindow(view);
	setNormalView(view);
	temporaryRangeSelect(-1);
	setArrowMark(-1);

	loc.restore(Settings.FollowingCaret);

	if (viewIdFromBuffId(buffId) != originalViewId)
	{
		moveFileToOtherView();

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


ComparedFile& ComparedPair::getFileBySciDoc(intptr_t sciDoc)
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
		moveFileToOtherView();
		oldFile.updateFromCurrent();
	}

	// If compare type is LastSaved or Git or SVN diff and folds are to be ignored then expand all folds in the
	// new (updated) file as its old version is restored unfolded and we shouldn't ignore folds
	const bool expandNewFileFolds = (options.ignoreFoldedLines && oldFile.isTemp && oldFile.isTemp != CLIPBOARD_TEMP);

	if (viewIdFromBuffId(newFile.buffId) != newFile.compareViewId)
	{
		activateBufferID(newFile.buffId);

		if (expandNewFileFolds)
			CallScintilla(newFile.originalViewId, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);

		moveFileToOtherView();
		newFile.updateFromCurrent();
	}
	else if (expandNewFileFolds)
	{
		CallScintilla(newFile.originalViewId, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
	}

	if (oldFile.sciDoc != getDocId(oldFile.compareViewId))
		activateBufferID(oldFile.buffId);

	if (newFile.sciDoc != getDocId(newFile.compareViewId))
		activateBufferID(newFile.buffId);

	activateBufferID(currentBuffId);
}


void ComparedPair::restoreFiles(LRESULT currentBuffId = -1)
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

	if (file[0].originalViewId == file[0].compareViewId)
	{
		file[0].restore();
		file[1].restore();
	}
	else
	{
		file[1].restore();
		file[0].restore();
	}

	if (currentBuffId != -1)
		activateBufferID(currentBuffId);
}


void ComparedPair::setStatus()
{
	if (Settings.StatusInfo == StatusType::STATUS_DISABLED)
		return;

	TCHAR info[512];

	if (compareDirty)
	{
		if (manuallyChanged)
			_tcscpy_s(info, _countof(info), TEXT("FILE MANUALLY CHANGED, PLEASE RE-COMPARE!"));
		else
			_tcscpy_s(info, _countof(info), TEXT("FILE CHANGED, COMPARE RESULTS MIGHT BE INACCURATE!"));
	}
	else
	{
		TCHAR buf[256] = TEXT(" ***");

		int infoCurrentPos = 0;

		if (options.selectionCompare)
		{
			_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" Selections - %Id-%Id vs. %Id-%Id ***"),
					options.selections[MAIN_VIEW].first + 1, options.selections[MAIN_VIEW].second + 1,
					options.selections[SUB_VIEW].first + 1, options.selections[SUB_VIEW].second + 1);
		}

		infoCurrentPos = _sntprintf_s(info, _countof(info), _TRUNCATE, TEXT("%s%s"),
				options.findUniqueMode ? TEXT("Find Unique") : TEXT("Compare"), buf);

		if (Settings.StatusInfo == StatusType::COMPARE_OPTIONS)
		{
			if (!options.findUniqueMode && options.detectMoves)
			{
				static constexpr TCHAR detectMovesStr[] = TEXT(" Detect Moves |");

				_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, detectMovesStr);
				infoCurrentPos += _countof(detectMovesStr) - 1;
			}

			if (options.ignoreEmptyLines || options.ignoreFoldedLines || options.ignoreAllSpaces ||
				options.ignoreChangedSpaces || options.ignoreCase || options.ignoreRegex)
			{
				const int len = _sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" Ignore :%s%s%s%s%s"),
						options.ignoreEmptyLines	? TEXT(" Empty Lines ,")	: TEXT(""),
						options.ignoreFoldedLines	? TEXT(" Folded Lines ,")	: TEXT(""),
						options.ignoreAllSpaces		? TEXT(" All Spaces ,")	: options.ignoreChangedSpaces
													? TEXT(" Changed Spaces ,") : TEXT(""),
						options.ignoreCase			? TEXT(" Case ,")			: TEXT(""),
						options.ignoreRegex			? TEXT(" Regex ,")			: TEXT(""));

				_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
				infoCurrentPos += len;
			}
		}
		else if (Settings.StatusInfo == StatusType::DIFFS_SUMMARY)
		{
			if (summary.diffLines)
			{
				const int len =
						_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Diff Lines: "), summary.diffLines);
				_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
				infoCurrentPos += len;
			}
			if (summary.added)
			{
				const int len =
						_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Added ,"), summary.added);
				_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
				infoCurrentPos += len;
			}
			if (summary.removed)
			{
				const int len =
						_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Removed ,"), summary.removed);
				_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
				infoCurrentPos += len;
			}
			if (summary.moved)
			{
				const int len =
						_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Moved ,"), summary.moved);
				_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
				infoCurrentPos += len;
			}
			if (summary.changed)
			{
				const int len =
						_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Changed ,"), summary.changed);
				_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
				infoCurrentPos += len;
			}
			if (summary.match)
			{
				const int len =
						_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(".  %Id Match ,"), summary.match) - 2;
				_tcscpy_s(info + infoCurrentPos - 2, _countof(info) - infoCurrentPos + 2, buf);
				infoCurrentPos += len;
			}
		}

		if (info[infoCurrentPos - 2] == TEXT(' '))
			info[infoCurrentPos - 2] = TEXT('\0');
	}

	::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, static_cast<LPARAM>((LONG_PTR)info));
}


void ComparedPair::adjustAlignment(int view, intptr_t line, intptr_t offset)
{
	AlignmentViewData AlignmentPair::*alignView = (view == MAIN_VIEW) ? &AlignmentPair::main : &AlignmentPair::sub;
	AlignmentInfo_t& alignInfo = summary.alignmentInfo;

	const intptr_t startIdx = getAlignmentIdxAfter(alignView, alignInfo, line);

	if ((startIdx < static_cast<intptr_t>(alignInfo.size())) && ((alignInfo[startIdx].*alignView).line >= line))
	{
		if (offset < 0)
		{
			intptr_t endIdx = startIdx;

			while ((endIdx < static_cast<intptr_t>(alignInfo.size())) &&
					(line > (alignInfo[endIdx].*alignView).line + offset))
				++endIdx;

			if (endIdx > startIdx)
				alignInfo.erase(alignInfo.begin() + startIdx, alignInfo.begin() + endIdx);
		}

		for (intptr_t i = startIdx; i < static_cast<intptr_t>(alignInfo.size()); ++i)
			(alignInfo[i].*alignView).line += offset;
	}
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
					_firstTabText, currFileIsNew ? TEXT("New") : TEXT("Old"));

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


SelectRangeTimeout::~SelectRangeTimeout()
{
	(*this)();
}


void SelectRangeTimeout::operator()()
{
	if ((_sel.first >= 0) && (_sel.second > _sel.first))
		setSelection(_view, _sel.first, _sel.second);
	else
		clearSelection(_view);
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


CompareList_t::iterator getCompareBySciDoc(intptr_t sciDoc)
{
	for (CompareList_t::iterator it = compareList.begin(); it < compareList.end(); ++it)
	{
		if (it->file[0].sciDoc == sciDoc || it->file[1].sciDoc == sciDoc)
			return it;
	}

	return compareList.end();
}


void temporaryRangeSelect(int view, intptr_t startPos, intptr_t endPos)
{
	static std::unique_ptr<SelectRangeTimeout> range;

	if (view < 0 || startPos < 0 || endPos < startPos)
	{
		range = nullptr;
		return;
	}

	range = nullptr;
	range = std::make_unique<SelectRangeTimeout>(view, startPos, endPos);

	if (range)
		range->post(2000);
}


void setArrowMark(int view, intptr_t line, bool down)
{
	static std::unique_ptr<LineArrowMarkTimeout> lineArrowMark;

	lineArrowMark = nullptr;

	if (view < 0 || line < 0)
		return;

	const int markerHandle = showArrowSymbol(view, line, down);

	lineArrowMark = std::make_unique<LineArrowMarkTimeout>(view, markerHandle);

	if (lineArrowMark)
		lineArrowMark->post(2000);
}


void showBlankAdjacentArrowMark(int view, intptr_t line, bool down)
{
	if (view < 0)
	{
		setArrowMark(-1);
		return;
	}

	if (line < 0 && Settings.FollowingCaret)
		line = getCurrentLine(view);

	if (line >= 0 && !isLineMarked(view, line, MARKER_MASK_LINE))
	{
		if (isAdjacentAnnotationVisible(view, line, down))
			setArrowMark(view, line, down);
		else if ((line == CallScintilla(view, SCI_GETLINECOUNT, 0, 0) - 1) &&
				isAdjacentAnnotationVisible(view, line, true))
			setArrowMark(view, line, true);
		else
			setArrowMark(-1);
	}
	else
	{
		setArrowMark(-1);
	}
}


intptr_t getCornerLine(int view, bool down, const CompareList_t::iterator& cmpPair)
{
	intptr_t cornerLine;

	if (cmpPair->options.selectionCompare)
	{
		if (down)
			cornerLine = cmpPair->options.selections[view].first;
		else
			cornerLine = cmpPair->options.selections[view].second;
	}
	else
	{
		if (down)
			cornerLine = 0;
		else
			cornerLine = CallScintilla(view, SCI_GETLINECOUNT, 0, 0) - 1;
	}

	return cornerLine;
}


std::pair<int, intptr_t> findNextChange(intptr_t mainStartLine, intptr_t subStartLine, bool down,
		bool goToCornerDiff = false)
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return std::make_pair(-1, -1);

	int view			= getCurrentViewId();
	const int otherView	= getOtherViewId(view);

	const int nextMarker = down ? SCI_MARKERNEXT : SCI_MARKERPREVIOUS;

	intptr_t mainNextLine = -1;
	intptr_t subNextLine = -1;

	if (mainStartLine >= 0)
	{
		mainNextLine = CallScintilla(MAIN_VIEW, nextMarker, mainStartLine, MARKER_MASK_LINE);

		if ((mainNextLine == mainStartLine) && !goToCornerDiff)
			mainNextLine = -1;
	}

	if (subStartLine >= 0)
	{
		subNextLine = CallScintilla(SUB_VIEW, nextMarker, subStartLine, MARKER_MASK_LINE);

		if ((subNextLine == subStartLine) && !goToCornerDiff)
			subNextLine = -1;
	}

	intptr_t line				= (view == MAIN_VIEW)		? mainNextLine : subNextLine;
	const intptr_t otherLine	= (otherView == MAIN_VIEW)	? mainNextLine : subNextLine;

	bool isChangedDiff = false;

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
		const intptr_t visibleLine		= CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0);
		const intptr_t otherVisibleLine	= CallScintilla(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		isChangedDiff = (CallScintilla(view, SCI_MARKERGET, line, 0) & MARKER_MASK_CHANGED) &&
				(CallScintilla(otherView, SCI_MARKERGET, otherLine, 0) & MARKER_MASK_CHANGED);

		if (!isChangedDiff)
		{
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
	}

	if (!isChangedDiff && !down && !Settings.ShowOnlyDiffs && isLineAnnotated(view, line) &&
			(line < CallScintilla(view, SCI_GETLINECOUNT, 0, 0) - 1))
		++line;

	if (cmpPair->options.selectionCompare && !isLineMarked(view, line, MARKER_MASK_LINE))
	{
		if (goToCornerDiff)
		{
			if (down && isLineHidden(view, line))
			{
				line = getPreviousUnhiddenLine(view, line);
			}
			else if (!down && (line > cmpPair->options.selections[view].second) &&
				(isLineMarked(view, cmpPair->options.selections[view].second, MARKER_MASK_LINE) ||
				Settings.ShowOnlySelections))
			{
				line = cmpPair->options.selections[view].second;
			}
		}
		else
		{
			if (!down)
			{
				if (line > cmpPair->options.selections[view].second &&
					(isLineMarked(view, cmpPair->options.selections[view].second, MARKER_MASK_LINE) ||
					Settings.ShowOnlySelections))
				{
					line = cmpPair->options.selections[view].second;
				}
				else if (line < cmpPair->options.selections[view].first)
				{
					line = cmpPair->options.selections[view].first;
				}
			}
		}
	}

	return std::make_pair(view, line);
}


std::pair<int, intptr_t> jumpToNextChange(intptr_t mainStartLine, intptr_t subStartLine, bool down,
		bool goToCornerDiff = false, bool doNotBlink = false)
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return std::make_pair(-1, -1);

	if (!cmpPair->options.findUniqueMode && !goToCornerDiff)
	{
		const int view		= getCurrentViewId();
		const int otherView	= getOtherViewId(view);

		const intptr_t edgeLine		= (down ? getLastLine(view) : getFirstLine(view));
		const intptr_t currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : edgeLine);

		// Is the caret line manually positioned on a screen edge and adjacent to invisible blank diff?
		// Make sure we don't miss it
		if (!isLineMarked(view, currentLine, MARKER_MASK_LINE) &&
			isAdjacentAnnotation(view, currentLine, down) &&
			!isAdjacentAnnotationVisible(view, currentLine, down) &&
			isLineMarked(otherView, otherViewMatchingLine(view, currentLine) + 1, MARKER_MASK_LINE))
		{
			centerAt(view, currentLine);

			showBlankAdjacentArrowMark(view, currentLine, down);

			return std::make_pair(view, currentLine);
		}
	}

	if (!goToCornerDiff && cmpPair->options.selectionCompare && Settings.ShowOnlySelections && !down)
	{
		const int view				= getCurrentViewId();
		const intptr_t startLine	= (view == MAIN_VIEW) ? mainStartLine : subStartLine;

		// Special selections compare corner case
		if (!isLineMarked(view, startLine, MARKER_MASK_LINE) && (startLine == cmpPair->options.selections[view].first))
		{
			down = !down;
			goToCornerDiff = true;
		}
	}

	if (goToCornerDiff)
	{
		mainStartLine	= getCornerLine(MAIN_VIEW, down, cmpPair);
		subStartLine	= getCornerLine(SUB_VIEW, down, cmpPair);
	}

	std::pair<int, intptr_t> nextDiff = findNextChange(mainStartLine, subStartLine, down, goToCornerDiff);

	int view		= nextDiff.first;
	intptr_t line	= nextDiff.second;

	if (view < 0)
	{
		if (goToCornerDiff || Settings.WrapAround)
			return nextDiff;

		// Last diff reached and direction was inverted to find the corner diff
		down = !down;
		goToCornerDiff = true;

		mainStartLine	= getCornerLine(MAIN_VIEW, down, cmpPair);
		subStartLine	= getCornerLine(SUB_VIEW, down, cmpPair);

		nextDiff = findNextChange(mainStartLine, subStartLine, down, goToCornerDiff);

		view = nextDiff.first;
		line = nextDiff.second;

		if (view < 0)
			return nextDiff;

		const intptr_t edgeLine		= (down ? getFirstLine(view) : getLastLine(view));
		const intptr_t currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : edgeLine);

		if ((down && (line >= currentLine) && (line > edgeLine)) ||
			(!down && (line <= currentLine) && (line < edgeLine)))
		{
			if (isLineVisible(view, line))
				blinkLine(view, line);
			else
				blinkLine(view, down ? getLastLine(view) : getFirstLine(view));

			// Adjust the direction of the blank annotation mark in case of selections compare corners
			if (cmpPair->options.selectionCompare && Settings.ShowOnlySelections && !down &&
					!isLineMarked(view, line, MARKER_MASK_LINE))
				down = !down;

			showBlankAdjacentArrowMark(view, line, down);

			return nextDiff;
		}
	}

	LOGD(LOG_VISIT, "Jump to " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
			" view, center doc line: " + std::to_string(line + 1) + "\n");

	if (cmpPair->options.findUniqueMode && Settings.FollowingCaret)
		::SetFocus(getView(view));

	// Adjust the direction of the blank annotation mark in case of selections compare corners
	if (goToCornerDiff && cmpPair->options.selectionCompare && Settings.ShowOnlySelections && !down &&
			!isLineMarked(view, line, MARKER_MASK_LINE))
		down = !down;

	if (Settings.ShowOnlyDiffs && isLineHidden(view, line))
		line = down ? getUnhiddenLine(view, line) : getPreviousUnhiddenLine(view, line);

	// Line is not visible - scroll into view
	if (!isLineVisible(view, line) || (!isLineMarked(view, line, MARKER_MASK_LINE) &&
		!isAdjacentAnnotationVisible(view, line, down) && (down || !goToCornerDiff || !isLineAnnotated(view, line))))
	{
		centerAt(view, line);
		doNotBlink = true;
	}

	CallScintilla(view, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);

	if (Settings.FollowingCaret && line != getCurrentLine(view))
	{
		intptr_t pos;

		if (down && (isLineAnnotated(view, line) && isLineWrapped(view, line) &&
				!isLineMarked(view, line, MARKER_MASK_LINE)))
			pos = getLineEnd(view, line);
		else
			pos = getLineStart(view, line);

		CallScintilla(view, SCI_SETEMPTYSELECTION, pos, 0);

		doNotBlink = true;
	}

	if (!doNotBlink)
		blinkLine(view, line);

	showBlankAdjacentArrowMark(view, line, down);

	return std::make_pair(view, line);
}


inline std::pair<int, intptr_t> jumpToFirstChange(bool doNotBlink = false)
{
	std::pair<int, intptr_t> viewLoc = jumpToNextChange(0, 0, true, true, doNotBlink);

	return viewLoc;
}


inline void jumpToLastChange(bool doNotBlink = false)
{
	jumpToNextChange(0, 0, false, true, doNotBlink);
}


void jumpToChange(bool down)
{
	std::pair<int, intptr_t> viewLoc;

	intptr_t mainStartLine	= 0;
	intptr_t subStartLine	= 0;

	const int currentView	= getCurrentViewId();
	const int otherView		= getOtherViewId(currentView);

	intptr_t& currentLine	= (currentView == MAIN_VIEW) ? mainStartLine : subStartLine;
	intptr_t& otherLine		= (currentView != MAIN_VIEW) ? mainStartLine : subStartLine;

	if (down)
	{
		currentLine = (Settings.FollowingCaret ? getCurrentLine(currentView) : getLastLine(currentView));

		if (Settings.FollowingCaret && isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
			(currentLine > getLastLine(currentView)))
		{
			// Current line is marked but invisible - get into view
			centerAt(currentView, currentLine);
			return;
		}

		const bool currentLineAnnotated = isLineAnnotated(currentView, currentLine);

		if (currentLineAnnotated && isAdjacentAnnotationVisible(currentView, currentLine, down))
			++currentLine;

		otherLine = (Settings.FollowingCaret ?
				otherViewMatchingLine(currentView, currentLine) : getLastLine(otherView));

		if (!currentLineAnnotated && isLineAnnotated(otherView, otherLine))
			++otherLine;

		viewLoc = jumpToNextChange(getNextUnmarkedLine(MAIN_VIEW, mainStartLine, MARKER_MASK_LINE),
				getNextUnmarkedLine(SUB_VIEW, subStartLine, MARKER_MASK_LINE), down);
	}
	else
	{
		currentLine = (Settings.FollowingCaret ? getCurrentLine(currentView) : getFirstLine(currentView));

		if (Settings.FollowingCaret && isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
			(currentLine < getFirstLine(currentView)))
		{
			// Current line is marked but invisible - get into view
			centerAt(currentView, currentLine);
			return;
		}

		if (isAdjacentAnnotationVisible(currentView, currentLine, down))
		{
			CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
			if (cmpPair == compareList.end())
				return;

			if (!(cmpPair->options.selectionCompare && Settings.ShowOnlySelections &&
				!isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
				(currentLine == cmpPair->options.selections[currentView].first)))
			{
				if (!(CallScintilla(currentView, SCI_MARKERGET, currentLine - 1, 0) & MARKER_MASK_CHANGED) ||
						!(CallScintilla(otherView, SCI_MARKERGET,
						otherViewMatchingLine(currentView, currentLine) - 1, 0) & MARKER_MASK_CHANGED))
					--currentLine;

				// Special selections compare corner case
				if (cmpPair->options.selectionCompare && Settings.ShowOnlySelections &&
						!isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
						(currentLine == cmpPair->options.selections[currentView].first))
					--currentLine;
			}
		}

		otherLine = (Settings.FollowingCaret ?
				otherViewMatchingLine(currentView, currentLine) : getFirstLine(otherView));

		viewLoc = jumpToNextChange(getPrevUnmarkedLine(MAIN_VIEW, mainStartLine, MARKER_MASK_LINE),
				getPrevUnmarkedLine(SUB_VIEW, subStartLine, MARKER_MASK_LINE), down);
	}

	if (viewLoc.first < 0)
	{
		if (Settings.WrapAround)
		{
			if (down)
				jumpToFirstChange(true);
			else
				jumpToLastChange(true);

			FLASHWINFO flashInfo;
			flashInfo.cbSize	= sizeof(flashInfo);
			flashInfo.hwnd		= nppData._nppHandle;
			flashInfo.uCount	= 3;
			flashInfo.dwTimeout	= 100;
			flashInfo.dwFlags	= FLASHW_ALL;
			::FlashWindowEx(&flashInfo);
		}
	}
}


void resetCompareView(int view)
{
	if (!::IsWindowVisible(getView(view)))
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(view));
	if (cmpPair != compareList.end())
		setCompareView(view, Settings.colors().blank, Settings.colors().caret_line_transparency);
}


intptr_t getAlignmentIdxAfter(const AlignmentViewData AlignmentPair::*pView, const AlignmentInfo_t &alignInfo,
	intptr_t line)
{
	intptr_t idx = 0;

	for (intptr_t i = static_cast<intptr_t>(alignInfo.size()) / 2; i; i /= 2)
	{
		if ((alignInfo[idx + i].*pView).line < line)
			idx += i;
	}

	while ((idx < static_cast<intptr_t>(alignInfo.size())) && ((alignInfo[idx].*pView).line < line))
		++idx;

	return idx;
}


intptr_t getAlignmentLine(const AlignmentInfo_t &alignInfo, int view, intptr_t line)
{
	if (line < 0)
		return -1;

	AlignmentViewData AlignmentPair::*alignView = (view == MAIN_VIEW) ? &AlignmentPair::main : &AlignmentPair::sub;

	const intptr_t alignIdx = getAlignmentIdxAfter(alignView, alignInfo, line);

	if ((alignIdx >= static_cast<intptr_t>(alignInfo.size())) || ((alignInfo[alignIdx].*alignView).line != line))
		return -1;

	alignView = (view == MAIN_VIEW) ? &AlignmentPair::sub : &AlignmentPair::main;

	return (alignInfo[alignIdx].*alignView).line;
}


bool isAlignmentNeeded(int view, const CompareList_t::iterator& cmpPair)
{
	const AlignmentInfo_t& alignmentInfo = cmpPair->summary.alignmentInfo;

	const AlignmentViewData AlignmentPair::*pView = (view == MAIN_VIEW) ? &AlignmentPair::main : &AlignmentPair::sub;

	const intptr_t firstLine	= getFirstLine(view);
	const intptr_t lastLine		= getLastLine(view);

	const intptr_t maxSize = static_cast<intptr_t>(alignmentInfo.size());

	intptr_t i = getAlignmentIdxAfter(pView, alignmentInfo, firstLine);

	if (i >= maxSize)
		return false;

	if (i)
		--i;

	// Ignore alignment on line 0 as it is currently not supported by Scintilla
	while ((i < maxSize) && ((alignmentInfo[i].main.line == 0) || (alignmentInfo[i].sub.line == 0)))
		++i;

	if (Settings.ShowOnlyDiffs)
	{
		for (; i < maxSize; ++i)
		{
			if (isLineFolded(MAIN_VIEW, alignmentInfo[i].main.line) ||
				isLineFolded(SUB_VIEW, alignmentInfo[i].sub.line))
				continue;

			if ((alignmentInfo[i].main.diffMask != 0) && (alignmentInfo[i].sub.diffMask != 0) &&
					(CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].main.line, 0) !=
					CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].sub.line, 0)))
				return true;

			if ((alignmentInfo[i].*pView).line > lastLine)
				return false;
		}
	}
	else
	{
		for (; i < maxSize; ++i)
		{
			if (isLineFolded(MAIN_VIEW, alignmentInfo[i].main.line) ||
				isLineFolded(SUB_VIEW, alignmentInfo[i].sub.line))
				continue;

			if ((alignmentInfo[i].main.diffMask == alignmentInfo[i].sub.diffMask) &&
					(CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].main.line, 0) !=
					CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].sub.line, 0)))
				return true;

			if ((alignmentInfo[i].*pView).line > lastLine)
				return false;
		}
	}

	intptr_t mainEndLine;
	intptr_t subEndLine;

	if (!cmpPair->options.selectionCompare)
	{
		mainEndLine	= CallScintilla(MAIN_VIEW, SCI_GETLINECOUNT, 0, 0) - 1;
		subEndLine	= CallScintilla(SUB_VIEW, SCI_GETLINECOUNT, 0, 0) - 1;
	}
	else
	{
		mainEndLine	= cmpPair->options.selections[MAIN_VIEW].second;
		subEndLine	= cmpPair->options.selections[SUB_VIEW].second;
	}

	if (Settings.ShowOnlyDiffs)
	{
		mainEndLine	= CallScintilla(MAIN_VIEW, SCI_MARKERPREVIOUS, mainEndLine, MARKER_MASK_LINE);
		subEndLine	= CallScintilla(SUB_VIEW, SCI_MARKERPREVIOUS, subEndLine, MARKER_MASK_LINE);

		if (mainEndLine < 0)
			mainEndLine = 0;
		if (subEndLine < 0)
			subEndLine = 0;
	}

	const intptr_t mainEndVisible = CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, mainEndLine, 0) +
			getWrapCount(MAIN_VIEW, mainEndLine) - 1;
	const intptr_t subEndVisible = CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, subEndLine, 0) +
			getWrapCount(SUB_VIEW, subEndLine) - 1;

	const intptr_t mismatchLen = std::abs(mainEndVisible - subEndVisible);
	const intptr_t linesOnScreen = CallScintilla(MAIN_VIEW, SCI_LINESONSCREEN, 0, 0);
	const intptr_t endMisalignment = (mismatchLen < linesOnScreen) ? mismatchLen : linesOnScreen;

	if (std::abs(getLineAnnotation(MAIN_VIEW, mainEndLine) - getLineAnnotation(SUB_VIEW, subEndLine)) !=
			endMisalignment)
		return true;

	return false;
}


void updateViewsFoldState(CompareList_t::iterator& cmpPair)
{
	if (Settings.ShowOnlyDiffs)
	{
		cmpPair->foldType = FOLD_MATCHES;

		hideUnmarked(MAIN_VIEW, MARKER_MASK_LINE);
		hideUnmarked(SUB_VIEW, MARKER_MASK_LINE);
	}
	else if (cmpPair->options.selectionCompare && Settings.ShowOnlySelections)
	{
		cmpPair->foldType = FOLD_OUTSIDE_SELECTIONS;

		hideOutsideRange(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first,
				cmpPair->options.selections[MAIN_VIEW].second);
		hideOutsideRange(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first,
				cmpPair->options.selections[SUB_VIEW].second);
	}
	else if (cmpPair->foldType != NO_FOLD)
	{
		cmpPair->foldType = NO_FOLD;

		auto foldedLines = getFoldedLines(MAIN_VIEW);
		CallScintilla(MAIN_VIEW, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
		setFoldedLines(MAIN_VIEW, foldedLines);

		foldedLines = getFoldedLines(SUB_VIEW);
		CallScintilla(SUB_VIEW, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
		setFoldedLines(SUB_VIEW, foldedLines);
	}
}


void alignDiffs(CompareList_t::iterator& cmpPair)
{
	updateViewsFoldState(cmpPair);

	const AlignmentInfo_t& alignmentInfo = cmpPair->summary.alignmentInfo;

	const intptr_t maxSize = static_cast<intptr_t>(alignmentInfo.size());

	intptr_t mainEndLine;
	intptr_t subEndLine;

	if (!cmpPair->options.selectionCompare)
	{
		mainEndLine	= CallScintilla(MAIN_VIEW, SCI_GETLINECOUNT, 0, 0) - 1;
		subEndLine	= CallScintilla(SUB_VIEW, SCI_GETLINECOUNT, 0, 0) - 1;
	}
	else
	{
		mainEndLine	= cmpPair->options.selections[MAIN_VIEW].second;
		subEndLine	= cmpPair->options.selections[SUB_VIEW].second;
	}

	bool skipFirst = false;

	intptr_t i = 0;

	// Handle zero line diffs that cannot be aligned because annotation on line 0 is not supported by Scintilla
	for (; i < maxSize && alignmentInfo[i].main.line <= mainEndLine && alignmentInfo[i].sub.line <= subEndLine; ++i)
	{
		intptr_t previousUnhiddenLine = getPreviousUnhiddenLine(MAIN_VIEW, alignmentInfo[i].main.line);

		if (isLineAnnotated(MAIN_VIEW, previousUnhiddenLine))
			clearAnnotation(MAIN_VIEW, previousUnhiddenLine);

		previousUnhiddenLine = getPreviousUnhiddenLine(SUB_VIEW, alignmentInfo[i].sub.line);

		if (isLineAnnotated(SUB_VIEW, previousUnhiddenLine))
			clearAnnotation(SUB_VIEW, previousUnhiddenLine);

		if (alignmentInfo[i].main.line == 0 || alignmentInfo[i].sub.line == 0)
		{
			skipFirst = (alignmentInfo[i].main.line == alignmentInfo[i].sub.line);
			continue;
		}

		if (i == 0 || skipFirst)
			break;

		const intptr_t mismatchLen =
				CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].main.line, 0) -
				CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].sub.line, 0);

		if (cmpPair->options.selectionCompare)
		{
			const intptr_t mainOffset	= (mismatchLen < 0) ? -mismatchLen : 0;
			const intptr_t subOffset	= (mismatchLen > 0) ? mismatchLen : 0;

			if (alignmentInfo[i - 1].main.line != 0)
			{
				addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first, mainOffset + 1, mainOffset + 1,
						"--- Selection Compare Block Start ---");
				addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, subOffset + 1, subOffset + 1,
						"Lines above cannot be properly aligned.");
			}
			else if (alignmentInfo[i - 1].sub.line != 0)
			{
				addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, mainOffset + 1, mainOffset + 1,
						"Lines above cannot be properly aligned.");
				addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first, subOffset + 1, subOffset + 1,
						"--- Selection Compare Block Start ---");
			}
			else
			{
				break;
			}
		}
		else
		{
			static const char *lineZeroAlignInfo =
						"Lines above cannot be properly aligned.\n"
						"To see them aligned, please manually insert one empty line\n"
						"in the beginning of each file and then re-compare.";

			if (mismatchLen > 0)
			{
				addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, 1, 1, lineZeroAlignInfo);
				addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, mismatchLen + 1, mismatchLen + 1,
						lineZeroAlignInfo);
			}
			else if (mismatchLen < 0)
			{
				addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, -mismatchLen + 1, -mismatchLen + 1,
						lineZeroAlignInfo);
				addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, 1, 1, lineZeroAlignInfo);
			}
		}

		++i;
		break;
	}

	// Align all other diffs
	for (; i < maxSize && alignmentInfo[i].main.line <= mainEndLine && alignmentInfo[i].sub.line <= subEndLine; ++i)
	{
		intptr_t previousUnhiddenLine = getPreviousUnhiddenLine(MAIN_VIEW, alignmentInfo[i].main.line);

		if (isLineAnnotated(MAIN_VIEW, previousUnhiddenLine))
			clearAnnotation(MAIN_VIEW, previousUnhiddenLine);

		previousUnhiddenLine = getPreviousUnhiddenLine(SUB_VIEW, alignmentInfo[i].sub.line);

		if (isLineAnnotated(SUB_VIEW, previousUnhiddenLine))
			clearAnnotation(SUB_VIEW, previousUnhiddenLine);

		if (isLineFolded(MAIN_VIEW, alignmentInfo[i].main.line) || isLineFolded(SUB_VIEW, alignmentInfo[i].sub.line))
			continue;

		const intptr_t mismatchLen =
				CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].main.line, 0) -
				CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, alignmentInfo[i].sub.line, 0);

		if (mismatchLen > 0)
		{
			if ((i + 1 < maxSize) && (alignmentInfo[i].sub.line == alignmentInfo[i + 1].sub.line))
				continue;

			addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, mismatchLen);
		}
		else if (mismatchLen < 0)
		{
			if ((i + 1 < maxSize) && (alignmentInfo[i].main.line == alignmentInfo[i + 1].main.line))
				continue;

			addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, -mismatchLen);
		}
	}

	if (Settings.ShowOnlyDiffs)
	{
		mainEndLine	= CallScintilla(MAIN_VIEW, SCI_MARKERPREVIOUS, mainEndLine, MARKER_MASK_LINE);
		subEndLine	= CallScintilla(SUB_VIEW, SCI_MARKERPREVIOUS, subEndLine, MARKER_MASK_LINE);

		if (mainEndLine < 0)
			mainEndLine = 0;
		if (subEndLine < 0)
			subEndLine = 0;
	}

	const intptr_t mainEndVisible = CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, mainEndLine, 0) +
			getWrapCount(MAIN_VIEW, mainEndLine) - 1;
	const intptr_t subEndVisible = CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, subEndLine, 0) +
			getWrapCount(SUB_VIEW, subEndLine) - 1;

	const intptr_t mismatchLen = mainEndVisible - subEndVisible;
	const intptr_t absMismatchLen = std::abs(mismatchLen);
	const intptr_t linesOnScreen = CallScintilla(MAIN_VIEW, SCI_LINESONSCREEN, 0, 0);
	const intptr_t endMisalignment = (absMismatchLen < linesOnScreen) ? absMismatchLen : linesOnScreen;

	if (std::abs(getLineAnnotation(MAIN_VIEW, mainEndLine) - getLineAnnotation(SUB_VIEW, subEndLine)) !=
		endMisalignment)
	{
		if (mismatchLen == 0)
		{
			clearAnnotation(MAIN_VIEW, mainEndLine);
			clearAnnotation(SUB_VIEW, subEndLine);
		}
		else if (mismatchLen > 0)
		{
			clearAnnotation(MAIN_VIEW, mainEndLine);
			addBlankSectionAfter(SUB_VIEW, subEndLine, endMisalignment);
		}
		else
		{
			addBlankSectionAfter(MAIN_VIEW, mainEndLine, endMisalignment);
			clearAnnotation(SUB_VIEW, subEndLine);
		}
	}

	// Mark selections for clarity
	if (cmpPair->options.selectionCompare)
	{
		// Line zero selections are already covered
		if (cmpPair->options.selections[MAIN_VIEW].first > 0 && cmpPair->options.selections[SUB_VIEW].first > 0)
		{
			intptr_t mainAnnotation = getLineAnnotation(MAIN_VIEW,
					getPreviousUnhiddenLine(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first));

			intptr_t subAnnotation = getLineAnnotation(SUB_VIEW,
					getPreviousUnhiddenLine(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first));

			const intptr_t visibleBlockStartMismatch =
				CallScintilla(MAIN_VIEW, SCI_VISIBLEFROMDOCLINE, cmpPair->options.selections[MAIN_VIEW].first, 0) -
				CallScintilla(SUB_VIEW, SCI_VISIBLEFROMDOCLINE, cmpPair->options.selections[SUB_VIEW].first, 0);

			++mainAnnotation;
			++subAnnotation;

			intptr_t mainAnnotPos	= mainAnnotation;
			intptr_t subAnnotPos	= subAnnotation;

			if (visibleBlockStartMismatch > 0)
				mainAnnotPos -= visibleBlockStartMismatch;
			else if (visibleBlockStartMismatch < 0)
				subAnnotPos += visibleBlockStartMismatch;

			addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first, mainAnnotation, mainAnnotPos,
					"--- Selection Compare Block Start ---");
			addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first, subAnnotation, subAnnotPos,
					"--- Selection Compare Block Start ---");
		}

		{
			intptr_t mainAnnotation = getLineAnnotation(MAIN_VIEW,
					getPreviousUnhiddenLine(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].second + 1));

			intptr_t subAnnotation = getLineAnnotation(SUB_VIEW,
					getPreviousUnhiddenLine(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].second + 1));

			if (mainAnnotation == 0 || subAnnotation == 0)
			{
				++mainAnnotation;
				++subAnnotation;
			}

			addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].second + 1,
					mainAnnotation, mainAnnotation, "--- Selection Compare Block End ---");
			addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].second + 1,
					subAnnotation, subAnnotation, "--- Selection Compare Block End ---");
		}
	}
}


void showNavBar()
{
	if (!NavDlg.SetColors(Settings.colors()))
		NavDlg.Show();
}


bool isFileCompared(int view)
{
	const intptr_t sciDoc = getDocId(view);

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

	std::pair<intptr_t, intptr_t> viewSel = getSelectionLines(view2);
	bool valid = !(viewSel.first < 0);

	if (view1 == view2)
		activateBufferID(currentBuffId);

	if (valid)
	{
		viewSel = getSelectionLines(view1);
		valid = !(viewSel.first < 0);
	}

	if (!valid)
		::MessageBox(nppData._nppHandle, TEXT("No valid selections to compare - operation ignored."),
				PLUGIN_NAME, MB_OK);

	return valid;
}


bool setFirst(bool currFileIsNew, bool markName = false)
{
	if (isFileCompared(getCurrentViewId()))
		return false;

	// Done on purpose: First wipe the std::unique_ptr so ~NewCompare is called before the new object constructor.
	// This is important because the N++ plugin menu is updated on NewCompare construct/destruct.
	newCompare = nullptr;
	newCompare = std::make_unique<NewCompare>(currFileIsNew, markName);

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
	if (!setFirst(true))
		return false;

	TCHAR tempFile[MAX_PATH];

	if (::GetTempPath(_countof(tempFile), tempFile))
	{
		const TCHAR* fileExt = ::PathFindExtension(newCompare->pair.file[0].name);

		BOOL success = (tempType == CLIPBOARD_TEMP);

		if (tempType != CLIPBOARD_TEMP)
		{
			const TCHAR* fileName = ::PathFindFileName(newCompare->pair.file[0].name);

			success = ::PathAppend(tempFile, fileName);

			if (success)
				::PathRemoveExtension(tempFile);
		}

		if (success)
		{
			_tcscat_s(tempFile, _countof(tempFile), tempMark[tempType].fileMark);

			size_t idxPos = _tcslen(tempFile);

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

			if (idxPos + 1 <= _countof(tempFile))
			{
				if (file)
				{
					success = ::CopyFile(file, tempFile, TRUE);

					if (success)
						::SetFileAttributes(tempFile, FILE_ATTRIBUTE_TEMPORARY);
				}
				else
				{
					HANDLE hFile = ::CreateFile(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
							FILE_ATTRIBUTE_TEMPORARY, NULL);

					success = (hFile != INVALID_HANDLE_VALUE);

					if (success)
						::CloseHandle(hFile);
				}

				if (success)
				{
					const int langType = static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE,
							newCompare->pair.file[0].buffId, 0));

					const int view = getCurrentViewId();
					const int encoding = static_cast<int>(CallScintilla(view, SCI_GETCODEPAGE, 0, 0));

					ScopedIncrementerInt incr(notificationsLock);

					if (::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)tempFile))
					{
						const LRESULT buffId = getCurrentBuffId();

						::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, buffId, langType);
						::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_SETREADONLY);

						CallScintilla(view, SCI_SETCODEPAGE, encoding, 0);

						newCompare->pair.file[1].isTemp = tempType;

						return true;
					}
				}
			}
		}
	}

	::MessageBox(nppData._nppHandle, TEXT("Creating temp file failed - operation aborted."), PLUGIN_NAME, MB_OK);

	newCompare = nullptr;

	return false;
}


void clearComparePair(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	ScopedIncrementerInt incr(notificationsLock);

	cmpPair->restoreFiles(buffId);

	compareList.erase(cmpPair);

	onBufferActivated(getCurrentBuffId());
}


void closeComparePair(CompareList_t::iterator cmpPair)
{
	HWND currentView = getCurrentView();

	ScopedIncrementerInt incr(notificationsLock);

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
		const bool isNew = singleView ? true : getCurrentViewId() == Settings.NewFileViewId;

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
	compareList.push_back(std::move(newCompare->pair));
	newCompare = nullptr;

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

	return compareViews(cmpPair->options, progressInfo, cmpPair->summary);
}


void compare(bool selectionCompare = false, bool findUniqueMode = false, bool autoUpdating = false)
{
	delayedUpdate.cancel();

	ScopedIncrementerInt incr(notificationsLock);

	// Just to be sure any old state is cleared
	storedLocation = nullptr;
	goToFirst = false;
	justCompared = false;
	copiedSectionMarks.clear();

	temporaryRangeSelect(-1);
	setArrowMark(-1);

	const bool				doubleView		= !isSingleView();
	const LRESULT			currentBuffId	= getCurrentBuffId();
	CompareList_t::iterator	cmpPair			= getCompare(currentBuffId);
	const bool				recompare		= (cmpPair != compareList.end());

	bool recompareSameSelections = false;

	if (recompare)
	{
		newCompare = nullptr;

		cmpPair->autoUpdateDelay = 0;

		if (!autoUpdating && selectionCompare)
		{
			bool checkSelections = false;

			// New selections to compare - validate them
			if (isSelection(MAIN_VIEW) && isSelection(SUB_VIEW))
			{
				checkSelections = true;
			}
			else if (isSelection(MAIN_VIEW) && (cmpPair->options.selections[SUB_VIEW].first != -1))
			{
				std::pair<intptr_t, intptr_t> newSelection = getSelectionLines(MAIN_VIEW);
				checkSelections = (newSelection.first == -1);

				if (!checkSelections)
					cmpPair->options.selections[MAIN_VIEW] = newSelection;

				recompareSameSelections = true;
			}
			else if (isSelection(SUB_VIEW) && (cmpPair->options.selections[MAIN_VIEW].first != -1))
			{
				std::pair<intptr_t, intptr_t> newSelection = getSelectionLines(SUB_VIEW);
				checkSelections = (newSelection.first == -1);

				if (!checkSelections)
					cmpPair->options.selections[SUB_VIEW] = newSelection;

				recompareSameSelections = true;
			}
			else
			{
				if ((cmpPair->options.selections[MAIN_VIEW].first == -1) ||
						(cmpPair->options.selections[SUB_VIEW].first == -1))
					checkSelections = true;

				recompareSameSelections = true;
			}

			if (checkSelections && !areSelectionsValid())
				return;
		}

		if ((!Settings.GotoFirstDiff && !selectionCompare) || autoUpdating)
			storedLocation = std::make_unique<ViewLocation>(getCurrentViewId());

		cmpPair->getOldFile().clear(autoUpdating);
		cmpPair->getNewFile().clear(autoUpdating);
	}
	// New compare
	else
	{
		if (!initNewCompare())
		{
			newCompare = nullptr;
			return;
		}

		cmpPair = addComparePair();

		if (cmpPair->getOldFile().isTemp)
		{
			activateBufferID(cmpPair->getNewFile().buffId);

			if (cmpPair->getOldFile().isTemp == CLIPBOARD_TEMP)
			{
				const int currentView = getCurrentViewId();

				if (selectionCompare && (isSelectionVertical(currentView) || isMultiSelection(currentView)))
					selectionCompare = false;
			}
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
		cmpPair->options.newFileViewId				= Settings.NewFileViewId;

		cmpPair->options.findUniqueMode				= findUniqueMode;
		cmpPair->options.alignAllMatches			= Settings.AlignAllMatches;
		cmpPair->options.neverMarkIgnored			= Settings.NeverMarkIgnored;
		cmpPair->options.detectMoves				= Settings.DetectMoves;
		cmpPair->options.detectCharDiffs			= Settings.DetectCharDiffs;
		cmpPair->options.ignoreEmptyLines			= Settings.IgnoreEmptyLines;
		cmpPair->options.ignoreFoldedLines			= Settings.IgnoreFoldedLines;
		cmpPair->options.ignoreChangedSpaces		= Settings.IgnoreChangedSpaces;
		cmpPair->options.ignoreAllSpaces			= Settings.IgnoreAllSpaces;
		cmpPair->options.ignoreCase					= Settings.IgnoreCase;

		if (Settings.IgnoreRegex)
			cmpPair->options.setIgnoreRegex(Settings.IgnoreRegexStr);
		else
			cmpPair->options.clearIgnoreRegex();

		cmpPair->options.changedThresholdPercent	= Settings.ChangedThresholdPercent;
		cmpPair->options.selectionCompare			= selectionCompare;

		cmpPair->positionFiles();

		if (selectionCompare && !recompareSameSelections)
		{
			if (cmpPair->getOldFile().isTemp != CLIPBOARD_TEMP)
			{
				cmpPair->options.selections[MAIN_VIEW]	= getSelectionLines(MAIN_VIEW);
				cmpPair->options.selections[SUB_VIEW]	= getSelectionLines(SUB_VIEW);
			}
			else
			{
				const int newView = cmpPair->getNewFile().compareViewId;
				const int tmpView = cmpPair->getOldFile().compareViewId;

				cmpPair->options.selections[newView] = getSelectionLines(newView);
				cmpPair->options.selections[tmpView] =
						std::make_pair(1, CallScintilla(tmpView, SCI_GETLINECOUNT, 0, 0) - 1);
			}
		}

		cmpPair->foldType = NO_FOLD;

		// New compare?
		if (!recompare)
		{
			constexpr int cLinesCountWarningLimit = 50000;

			bool largeFilesWarning = false;

			if (selectionCompare)
				largeFilesWarning =
					(cmpPair->options.selections[MAIN_VIEW].second -
					cmpPair->options.selections[MAIN_VIEW].first + 1 > cLinesCountWarningLimit) &&
					(cmpPair->options.selections[SUB_VIEW].second -
					cmpPair->options.selections[SUB_VIEW].first + 1 > cLinesCountWarningLimit);
			else
				largeFilesWarning =
					(CallScintilla(MAIN_VIEW, SCI_GETLINECOUNT, 0, 0) > cLinesCountWarningLimit) &&
					(CallScintilla(SUB_VIEW, SCI_GETLINECOUNT, 0, 0) > cLinesCountWarningLimit);

			if (largeFilesWarning)
			{
				if (::MessageBox(nppData._nppHandle,
					TEXT("Comparing large files such as these might take significant time ")
					TEXT("especially if they differ a lot.\n\n")
					TEXT("Compare anyway?"), PLUGIN_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
				{
					clearComparePair(getCurrentBuffId());
					return;
				}
			}
		}
	}

	selectionAutoRecompare = autoUpdating && cmpPair->options.selectionCompare;

	time_t startTime = time(0);

	const CompareResult cmpResult = runCompare(cmpPair);

	cmpPair->compareDirty		= false;
	cmpPair->manuallyChanged	= false;

	switch (cmpResult)
	{
		case CompareResult::COMPARE_MISMATCH:
		{
			// Honour 'Auto Re-compare On Change' user setting only if compare time is less than 5 sec.
			cmpPair->options.recompareOnChange = Settings.RecompareOnChange && (difftime(time(0), startTime) < 5.0);

			justCompared = true;

			if (Settings.UseNavBar)
				showNavBar();

			NppSettings::get().setCompareMode(true);

			setCompareView(MAIN_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);
			setCompareView(SUB_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);

			if (recompare)
			{
				updateViewsFoldState(cmpPair);
				cmpPair->setStatus();
			}

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
			}

			currentlyActiveBuffID = getCurrentBuffId();

			LOGD(LOG_ALL, "COMPARE READY\n");
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
					{
						_sntprintf_s(msg, _countof(msg), _TRUNCATE,
								TEXT("File \"%s\" has not been modified since last Save."), newName);
					}
					else if (oldFile.isTemp == CLIPBOARD_TEMP)
					{
						_sntprintf_s(msg, _countof(msg), _TRUNCATE,
								TEXT("%s \"%s\" has no changes against clipboard."),
								selectionCompare ? TEXT("Selection in file") : TEXT("File"), newName);
					}
					else
					{
						_sntprintf_s(msg, _countof(msg), _TRUNCATE,
								TEXT("File \"%s\" has no changes against %s."), newName,
								oldFile.isTemp == GIT_TEMP ? TEXT("Git") : TEXT("SVN"));
					}
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

		case CompareResult::COMPARE_ERROR:
			::MessageBox(nppData._nppHandle, TEXT("Failure allocating resources, compare aborted"),
					PLUGIN_NAME, MB_OK | MB_ICONERROR);

		default:
			clearComparePair(getCurrentBuffId());
	}

	storedLocation = nullptr;
}


std::vector<char> getClipboard(bool addLeadingNewLine = false)
{
	std::vector<char> content;

	if (!::OpenClipboard(NULL))
		return content;

	HANDLE hData = ::GetClipboardData(CF_UNICODETEXT);

	if (hData != NULL)
	{
		wchar_t* pText = static_cast<wchar_t*>(::GlobalLock(hData));

		if (pText != NULL)
		{
			const size_t wLen	= wcslen(pText) + 1;
			const size_t len	=
					::WideCharToMultiByte(CP_UTF8, 0, pText, static_cast<int>(wLen), NULL, 0, NULL, NULL);

			if (addLeadingNewLine)
			{
				content.resize(len + 1);
				content[0] = '\n'; // Needed for selections alignment after comparing

				::WideCharToMultiByte(CP_UTF8, 0, pText, static_cast<int>(wLen), content.data() + 1,
						static_cast<int>(len), NULL, NULL);
			}
			else
			{
				content.resize(len);

				::WideCharToMultiByte(CP_UTF8, 0, pText, static_cast<int>(wLen), content.data(),
						static_cast<int>(len), NULL, NULL);
			}
		}

		::GlobalUnlock(hData);
	}

	::CloseClipboard();

	return content;
}


void SetAsFirst()
{
	if (!setFirst(Settings.FirstFileIsNew, true))
		newCompare = nullptr;
}


void CompareWhole()
{
	compare();
}


void CompareSelections()
{
	compare(true);
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
	newCompare = nullptr;

	if (NppSettings::get().compareMode)
		clearComparePair(getCurrentBuffId());
}


void ClearAllCompares()
{
	newCompare = nullptr;

	if (!compareList.size())
		return;

	const LRESULT buffId = getCurrentBuffId();

	ScopedIncrementerInt incr(notificationsLock);

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
		compare();
}


void ClipboardDiff()
{
	const int view = getCurrentViewId();

	if (CallScintilla(view, SCI_GETLENGTH, 0, 0) == 0)
	{
		TCHAR file[MAX_PATH];

		::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

		if (::PathFileExists(file) == FALSE)
		{
			::MessageBox(nppData._nppHandle,
					TEXT("File is empty - operation ignored."), PLUGIN_NAME, MB_OK);

			return;
		}
	}

	const bool isSel = isSelection(view);

	std::vector<char> content = getClipboard(isSel);

	if (content.empty())
	{
		::MessageBox(nppData._nppHandle, TEXT("Clipboard does not contain any text to compare."), PLUGIN_NAME, MB_OK);
		return;
	}

	if (!createTempFile(nullptr, CLIPBOARD_TEMP))
		return;

	setContent(content.data());
	content.clear();

	compare(isSel);
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
		compare();
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

	if (!createTempFile(nullptr, GIT_TEMP))
		return;

	setContent(content.data());
	content.clear();

	compare();
}


void BookmarkDiffs()
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	bookmarkMarkedLines(getCurrentViewId(), MARKER_MASK_DIFF_LINE);
}


void BookmarkAddedRemoved()
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	bookmarkMarkedLines(getCurrentViewId(), MARKER_MASK_NEW_LINE);
}


void BookmarkChanged()
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	bookmarkMarkedLines(getCurrentViewId(), MARKER_MASK_CHANGED_LINE);
}


void ActiveCompareSummary()
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	TCHAR info[1024];

	int infoCurrentPos = _sntprintf_s(info, _countof(info), _TRUNCATE, TEXT("%s Summary:\n\n"),
			cmpPair->options.findUniqueMode ? TEXT("Find Unique") : TEXT("Compare"));

	TCHAR buf[256];

	if (cmpPair->summary.diffLines)
	{
		const int len =
				_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT("%Id Diff Lines:\n"), cmpPair->summary.diffLines);
		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
		infoCurrentPos += len;
	}
	if (cmpPair->summary.added)
	{
		const int len =
				_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Added ,"), cmpPair->summary.added);
		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
		infoCurrentPos += len;
	}
	if (cmpPair->summary.removed)
	{
		const int len =
				_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Removed ,"), cmpPair->summary.removed);
		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
		infoCurrentPos += len;
	}
	if (cmpPair->summary.moved)
	{
		const int len =
				_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Moved ,"), cmpPair->summary.moved);
		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
		infoCurrentPos += len;
	}
	if (cmpPair->summary.changed)
	{
		const int len =
				_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(" %Id Changed ,"), cmpPair->summary.changed);
		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
		infoCurrentPos += len;
	}
	if (cmpPair->summary.match)
	{
		const int len =
				_sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT(".\n%Id Match ,"), cmpPair->summary.match) - 2;
		_tcscpy_s(info + infoCurrentPos - 2, _countof(info) - infoCurrentPos + 2, buf);
		infoCurrentPos += len;
	}

	if (info[infoCurrentPos - 2] == TEXT(' '))
		infoCurrentPos -= 2;

	{
		static constexpr TCHAR comparisonOptStr[] = TEXT("\n\nComparison options:\n\n");

		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, comparisonOptStr);
		infoCurrentPos += _countof(comparisonOptStr) - 1;
	}

	if (!cmpPair->options.findUniqueMode && cmpPair->options.detectMoves)
	{
		static constexpr TCHAR detectMovesStr[] = TEXT("Detect Moves\n");

		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, detectMovesStr);
		infoCurrentPos += _countof(detectMovesStr) - 1;
	}

	if (cmpPair->options.ignoreEmptyLines || cmpPair->options.ignoreFoldedLines || cmpPair->options.ignoreAllSpaces ||
		cmpPair->options.ignoreChangedSpaces || cmpPair->options.ignoreCase || cmpPair->options.ignoreRegex)
	{
		const int len = _sntprintf_s(buf, _countof(buf), _TRUNCATE, TEXT("Ignore :%s%s%s%s%s"),
				cmpPair->options.ignoreEmptyLines	? TEXT(" Empty Lines ,")	: TEXT(""),
				cmpPair->options.ignoreFoldedLines	? TEXT(" Folded Lines ,")	: TEXT(""),
				cmpPair->options.ignoreAllSpaces	? TEXT(" All Spaces ,")	: cmpPair->options.ignoreChangedSpaces
													? TEXT(" Changed Spaces ,") : TEXT(""),
				cmpPair->options.ignoreCase			? TEXT(" Case ,")			: TEXT(""),
				cmpPair->options.ignoreRegex		? TEXT(" Regex ,")			: TEXT(""));

		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, buf);
		infoCurrentPos += len;
	}

	if (info[infoCurrentPos - 2] == TEXT(' '))
		info[infoCurrentPos - 2] = TEXT('\0');
	else
		_tcscpy_s(info + infoCurrentPos, _countof(info) - infoCurrentPos, TEXT("No specific options used."));

	::MessageBox(nppData._nppHandle, info, PLUGIN_NAME, MB_OK);
}


void DetectMoves()
{
	Settings.DetectMoves = !Settings.DetectMoves;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMoves);
	Settings.markAsDirty();
}


void DetectCharDiffs()
{
	Settings.DetectCharDiffs = !Settings.DetectCharDiffs;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_CHAR_DIFFS]._cmdID,
			(LPARAM)Settings.DetectCharDiffs);
	Settings.markAsDirty();
}


void IgnoreEmptyLines()
{
	Settings.IgnoreEmptyLines = !Settings.IgnoreEmptyLines;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_EMPTY_LINES]._cmdID,
			(LPARAM)Settings.IgnoreEmptyLines);
	Settings.markAsDirty();
}


void IgnoreFoldedLines()
{
	Settings.IgnoreFoldedLines = !Settings.IgnoreFoldedLines;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_FOLDED_LINES]._cmdID,
			(LPARAM)Settings.IgnoreFoldedLines);
	Settings.markAsDirty();
}


void IgnoreChangedSpaces()
{
	Settings.IgnoreChangedSpaces = !Settings.IgnoreChangedSpaces;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_CHANGED_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreChangedSpaces);

	if (Settings.IgnoreChangedSpaces)
	{
		Settings.IgnoreAllSpaces = false;
		::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_ALL_SPACES]._cmdID,
				(LPARAM)false);
	}

	Settings.markAsDirty();
}


void IgnoreAllSpaces()
{
	Settings.IgnoreAllSpaces = !Settings.IgnoreAllSpaces;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_ALL_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreAllSpaces);

	if (Settings.IgnoreAllSpaces)
	{
		Settings.IgnoreChangedSpaces = false;
		::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_CHANGED_SPACES]._cmdID,
				(LPARAM)false);
	}

	Settings.markAsDirty();
}


void IgnoreCase()
{
	Settings.IgnoreCase = !Settings.IgnoreCase;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_CASE]._cmdID,
			(LPARAM)Settings.IgnoreCase);
	Settings.markAsDirty();
}


void IgnoreRegex()
{
	const bool currentIgnoreRegexSwitch = Settings.IgnoreRegex;

	IgnoreRegexDialog IgnoreRegexDlg(hInstance, nppData);

	if (IgnoreRegexDlg.doDialog(&Settings) == IDOK)
		Settings.IgnoreRegex = !Settings.IgnoreRegexStr.empty();
	else
		Settings.IgnoreRegex = false;

	if (currentIgnoreRegexSwitch != Settings.IgnoreRegex)
	{
		::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_REGEX]._cmdID,
				(LPARAM)Settings.IgnoreRegex);
		Settings.markAsDirty();
	}
}


void ShowOnlyDiffs()
{
	Settings.ShowOnlyDiffs = !Settings.ShowOnlyDiffs;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_SHOW_ONLY_DIFF]._cmdID,
			(LPARAM)Settings.ShowOnlyDiffs);
	Settings.markAsDirty();

	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

	if (cmpPair != compareList.end())
	{
		ScopedIncrementerInt incr(notificationsLock);

		const int view			= getCurrentViewId();
		intptr_t currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : getFirstLine(view));

		if (!isLineMarked(view, currentLine, MARKER_MASK_LINE))
		{
			const intptr_t  nextMarkedLine = CallScintilla(view, SCI_MARKERNEXT, currentLine, MARKER_MASK_LINE);

			if (nextMarkedLine >= 0)
				currentLine = nextMarkedLine;
			else
				currentLine = CallScintilla(view, SCI_MARKERPREVIOUS, currentLine, MARKER_MASK_LINE);

			if (Settings.FollowingCaret)
				CallScintilla(view, SCI_GOTOLINE, currentLine, 0);
		}

		ViewLocation loc(view, currentLine);

		CallScintilla(MAIN_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
		CallScintilla(SUB_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);

		alignDiffs(cmpPair);

		loc.restore(Settings.FollowingCaret);

		NavDlg.Update();
	}
}


void ShowOnlySelections()
{
	Settings.ShowOnlySelections = !Settings.ShowOnlySelections;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_SHOW_ONLY_SEL]._cmdID,
			(LPARAM)Settings.ShowOnlySelections);
	Settings.markAsDirty();

	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

	if (cmpPair != compareList.end())
	{
		ScopedIncrementerInt incr(notificationsLock);

		const int view			= getCurrentViewId();
		intptr_t currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : getFirstLine(view));

		if (currentLine < cmpPair->options.selections[view].first)
			currentLine = cmpPair->options.selections[view].first;
		else if (currentLine > cmpPair->options.selections[view].second)
			currentLine = cmpPair->options.selections[view].second;

		if (Settings.FollowingCaret)
			CallScintilla(view, SCI_GOTOLINE, currentLine, 0);

		ViewLocation loc(view, currentLine);

		CallScintilla(MAIN_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
		CallScintilla(SUB_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);

		alignDiffs(cmpPair);

		loc.restore(Settings.FollowingCaret);

		NavDlg.Update();
	}
}


void AutoRecompare()
{
	Settings.RecompareOnChange = !Settings.RecompareOnChange;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_AUTO_RECOMPARE]._cmdID,
			(LPARAM)Settings.RecompareOnChange);
	Settings.markAsDirty();

	if (Settings.RecompareOnChange)
	{
		CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());

		if ((cmpPair != compareList.end()) && cmpPair->compareDirty && cmpPair->options.recompareOnChange)
			delayedUpdate.post(30);
	}
}


void Prev()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToChange(false);
	}
}


void Next()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToChange(true);
	}
}


void First()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToFirstChange();
	}
}


void Last()
{
	if (NppSettings::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToLastChange();
	}
}


void OpenSettingsDlg(void)
{
	SettingsDialog SettingsDlg(hInstance, nppData);

	if (SettingsDlg.doDialog(&Settings) == IDOK)
	{
		Settings.save();

		newCompare = nullptr;

		if (!compareList.empty())
		{
			setStyles(Settings);
			NavDlg.SetColors(Settings.colors());

			if (NppSettings::get().compareMode)
			{
				setCompareView(MAIN_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);
				setCompareView(SUB_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);
			}
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

	_tcscpy_s(funcItem[CMD_CLIPBOARD_DIFF]._itemName, nbChar, TEXT("Compare file/selection to Clipboard"));
	funcItem[CMD_CLIPBOARD_DIFF]._pFunc 			= ClipboardDiff;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey 			= new ShortcutKey;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_isAlt 	= true;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_key 		= 'M';

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

	_tcscpy_s(funcItem[CMD_BOOKMARK_DIFFS]._itemName, nbChar, TEXT("Bookmark All Diffs in Current View"));
	funcItem[CMD_BOOKMARK_DIFFS]._pFunc = BookmarkDiffs;

	_tcscpy_s(funcItem[CMD_BOOKMARK_ADD_REM]._itemName, nbChar, TEXT("Bookmark Added/Removed Lines in Current View"));
	funcItem[CMD_BOOKMARK_ADD_REM]._pFunc = BookmarkAddedRemoved;

	_tcscpy_s(funcItem[CMD_BOOKMARK_CHANGED]._itemName, nbChar, TEXT("Bookmark Changed Lines in Current View"));
	funcItem[CMD_BOOKMARK_CHANGED]._pFunc = BookmarkChanged;

	_tcscpy_s(funcItem[CMD_COMPARE_SUMMARY]._itemName, nbChar, TEXT("Active Compare Summary"));
	funcItem[CMD_COMPARE_SUMMARY]._pFunc = ActiveCompareSummary;

	_tcscpy_s(funcItem[CMD_DETECT_MOVES]._itemName, nbChar, TEXT("Detect Moves"));
	funcItem[CMD_DETECT_MOVES]._pFunc = DetectMoves;

	_tcscpy_s(funcItem[CMD_DETECT_CHAR_DIFFS]._itemName, nbChar, TEXT("Detect Character Diffs"));
	funcItem[CMD_DETECT_CHAR_DIFFS]._pFunc = DetectCharDiffs;

	_tcscpy_s(funcItem[CMD_IGNORE_EMPTY_LINES]._itemName, nbChar, TEXT("Ignore Empty Lines"));
	funcItem[CMD_IGNORE_EMPTY_LINES]._pFunc = IgnoreEmptyLines;

	_tcscpy_s(funcItem[CMD_IGNORE_FOLDED_LINES]._itemName, nbChar, TEXT("Ignore Folded Lines"));
	funcItem[CMD_IGNORE_FOLDED_LINES]._pFunc = IgnoreFoldedLines;

	_tcscpy_s(funcItem[CMD_IGNORE_CHANGED_SPACES]._itemName, nbChar, TEXT("Ignore Changed Spaces"));
	funcItem[CMD_IGNORE_CHANGED_SPACES]._pFunc = IgnoreChangedSpaces;

	_tcscpy_s(funcItem[CMD_IGNORE_ALL_SPACES]._itemName, nbChar, TEXT("Ignore All Spaces"));
	funcItem[CMD_IGNORE_ALL_SPACES]._pFunc = IgnoreAllSpaces;

	_tcscpy_s(funcItem[CMD_IGNORE_CASE]._itemName, nbChar, TEXT("Ignore Case"));
	funcItem[CMD_IGNORE_CASE]._pFunc = IgnoreCase;

	_tcscpy_s(funcItem[CMD_IGNORE_REGEX]._itemName, nbChar, TEXT("Ignore Regex..."));
	funcItem[CMD_IGNORE_REGEX]._pFunc = IgnoreRegex;

	_tcscpy_s(funcItem[CMD_SHOW_ONLY_DIFF]._itemName, nbChar, TEXT("Show Only Diffs (Hide Matches)"));
	funcItem[CMD_SHOW_ONLY_DIFF]._pFunc = ShowOnlyDiffs;

	_tcscpy_s(funcItem[CMD_SHOW_ONLY_SEL]._itemName, nbChar, TEXT("Show Only Compared Selections"));
	funcItem[CMD_SHOW_ONLY_SEL]._pFunc = ShowOnlySelections;

	_tcscpy_s(funcItem[CMD_NAV_BAR]._itemName, nbChar, TEXT("Navigation Bar"));
	funcItem[CMD_NAV_BAR]._pFunc = ToggleNavigationBar;

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


void freeToolbarObjects(toolbarIconsWithDarkMode& tb)
{
	if (tb.hToolbarBmp)
		::DeleteObject(tb.hToolbarBmp);
	if (tb.hToolbarIcon)
		::DestroyIcon(tb.hToolbarIcon);
	if (tb.hToolbarIconDarkMode)
		::DestroyIcon(tb.hToolbarIconDarkMode);
}


void deinitPlugin()
{
	// Always close it, else N++'s plugin manager would call 'ToggleNavigationBar'
	// on startup, when N++ has been shut down before with opened navigation bar
	if (NavDlg.isVisible())
		NavDlg.Hide();

	freeToolbarObjects(tbSetFirst);
	freeToolbarObjects(tbCompare);
	freeToolbarObjects(tbCompareSel);
	freeToolbarObjects(tbClearCompare);
	freeToolbarObjects(tbFirst);
	freeToolbarObjects(tbPrev);
	freeToolbarObjects(tbNext);
	freeToolbarObjects(tbLast);
	freeToolbarObjects(tbDiffsOnly);
	freeToolbarObjects(tbNavBar);

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

	intptr_t firstVisible				= getFirstVisibleLine(biasView);
	const intptr_t otherFirstVisible	= getFirstVisibleLine(otherView);

	const intptr_t endLine = getPreviousUnhiddenLine(biasView, CallScintilla(biasView, SCI_GETLINECOUNT, 0, 0) - 1);
	const intptr_t endVisible =
			CallScintilla(biasView, SCI_VISIBLEFROMDOCLINE, endLine, 0) + getWrapCount(biasView, endLine);

	intptr_t otherNewFirstVisible = otherFirstVisible;

	if (firstVisible > endVisible)
	{
		firstVisible = endVisible;

		ScopedIncrementerInt incr(notificationsLock);

		CallScintilla(biasView, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
	}

	if (firstVisible != otherFirstVisible)
	{
		const intptr_t otherEndLine =
				getPreviousUnhiddenLine(otherView, CallScintilla(otherView, SCI_GETLINECOUNT, 0, 0) - 1);
		const intptr_t otherEndVisible = CallScintilla(otherView, SCI_VISIBLEFROMDOCLINE,
				otherEndLine, 0) + getWrapCount(otherView, otherEndLine);

		if (firstVisible > otherEndVisible)
		{
			if (endVisible - firstVisible < CallScintilla(biasView, SCI_LINESONSCREEN, 0, 0))
				otherNewFirstVisible = firstVisible;
			else
				otherNewFirstVisible = otherEndVisible;
		}
		else
		{
			otherNewFirstVisible = firstVisible;
		}
	}

	if (otherNewFirstVisible != otherFirstVisible)
	{
		ScopedIncrementerInt incr(notificationsLock);

		CallScintilla(otherView, SCI_SETFIRSTVISIBLELINE, otherNewFirstVisible, 0);

		::UpdateWindow(getView(otherView));

		LOGD(LOG_SYNC, "Syncing to " + std::string(biasView == MAIN_VIEW ? "MAIN" : "SUB") +
				" view, visible doc line: " + std::to_string(
				CallScintilla(biasView, SCI_DOCLINEFROMVISIBLE, firstVisible, 0) + 1) + "\n");
	}

	if (Settings.FollowingCaret && biasView == getCurrentViewId())
	{
		const intptr_t line = getCurrentLine(biasView);

		otherNewFirstVisible = otherViewMatchingLine(biasView, line);

		if ((otherNewFirstVisible != getCurrentLine(otherView)) && !isSelection(otherView))
		{
			intptr_t pos;

			if (!isLineMarked(otherView, otherNewFirstVisible, MARKER_MASK_LINE) &&
					isLineAnnotated(otherView, otherNewFirstVisible) && isLineWrapped(otherView, otherNewFirstVisible))
				pos = getLineEnd(otherView, otherNewFirstVisible);
			else
				pos = getLineStart(otherView, otherNewFirstVisible);

			ScopedIncrementerInt incr(notificationsLock);

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

		ScopedIncrementerInt incr(notificationsLock);

		NppSettings::get().setCompareMode();
	}

	CallScintilla(MAIN_VIEW,	SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);
	CallScintilla(SUB_VIEW,		SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);

	temporaryRangeSelect(-1);
	setArrowMark(-1);

	setCompareView(MAIN_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);
	setCompareView(SUB_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);

	if (Settings.ShowOnlyDiffs || Settings.ShowOnlySelections)
	{
		CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

		if ((cmpPair != compareList.end()) && (Settings.ShowOnlyDiffs ||
			(cmpPair->options.selectionCompare && Settings.ShowOnlySelections)))
		{
			ScopedIncrementerInt incr(notificationsLock);

			alignDiffs(cmpPair);
		}
	}
}


std::pair<std::wstring, std::wstring> getTwoFilenamesFromCmdLine(const TCHAR* cmdLine)
{
	if (cmdLine == nullptr)
		return std::make_pair(std::wstring{}, std::wstring{});

	std::wstring firstFile;
	std::wstring secondFile;

	for (; *cmdLine != _T('\0'); ++cmdLine)
	{
		if (*cmdLine == _T(' '))
			continue;

		TCHAR sectionEnd = _T(' ');

		if (*cmdLine == _T('-'))
		{
			for (++cmdLine; *cmdLine != _T('\0'); ++cmdLine)
			{
				if (*cmdLine == sectionEnd)
					break;

				if (*cmdLine == _T('"') && sectionEnd == _T(' '))
					sectionEnd = _T('"');
				else if (*cmdLine == _T('\'') && sectionEnd == _T(' '))
					sectionEnd = _T('\'');
			}
		}
		else
		{
			if (*cmdLine == _T('"'))
			{
				sectionEnd = _T('"');
				++cmdLine;
			}
			else if (*cmdLine == _T('\''))
			{
				sectionEnd = _T('\'');
				++cmdLine;
			}

			if (*cmdLine == _T('\0'))
				break;

			const TCHAR* startPos = cmdLine;

			for (++cmdLine; *cmdLine != sectionEnd && *cmdLine != _T('\0'); ++cmdLine);

			if (firstFile.empty())
			{
				firstFile.assign(startPos, cmdLine);

				for (size_t i = 0; i < firstFile.size(); ++i)
					if (firstFile.at(i) == L'/')
						firstFile.at(i) = L'\\';
			}
			else
			{
				secondFile.assign(startPos, cmdLine);

				for (size_t i = 0; i < secondFile.size(); ++i)
					if (secondFile.at(i) == L'/')
						secondFile.at(i) = L'\\';

				break;
			}

			if (*cmdLine == _T('\0'))
				break;
		}
	}

	return std::make_pair(firstFile, secondFile);
}


// Find command line files full paths
// Because Notepad++ uses ::SetCurrentDirectory() the folder from which it was started is lost so no way to
// retrieve easily the command line files relative paths. That's why we try to parse all opened files to
// try to construct the command line files full paths
bool constructFullFilePaths(std::pair<std::wstring, std::wstring>& files)
{
	const int openedFilesCount =
			static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, ALL_OPEN_FILES));

	TCHAR** openedFiles = new TCHAR*[openedFilesCount];

	for (int i = 0; i < openedFilesCount; ++i)
		openedFiles[i] = new TCHAR[1024];

	::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMES, (WPARAM)openedFiles, (LPARAM)openedFilesCount);

	std::wstring* longerFileName  = files.first.size() >= files.second.size() ? &(files.first) : &(files.second);
	std::wstring* shorterFileName = &(files.first) == longerFileName ? &(files.second) : &(files.first);

	int longerFound = 0;
	int shorterFound = 0;

	std::wstring longer;
	std::wstring shorter;

	for (int i = 0; i < openedFilesCount; ++i)
	{
		size_t pathLen = wcslen(openedFiles[i]);

		if (pathLen >= longerFileName->size() &&
			wcsstr(openedFiles[i] + pathLen - longerFileName->size(), longerFileName->c_str()))
		{
			if (++longerFound == 1)
				longer = openedFiles[i];
		}
		else if (pathLen >= shorterFileName->size() &&
			wcsstr(openedFiles[i] + pathLen - shorterFileName->size(), shorterFileName->c_str()))
		{
			if (++shorterFound == 1)
				shorter = openedFiles[i];
		}

		delete [] openedFiles[i];
	}

	delete [] openedFiles;

	if (longerFound > 1 || shorterFound > 1)
		return false;

	*longerFileName = std::move(longer);
	*shorterFileName = std::move(shorter);

	return true;
}


void checkCmdLine()
{
	constexpr wchar_t compareRunCmd[]	= L"-pluginMessage=compare";
	constexpr int minCmdLineLen			= sizeof(compareRunCmd) / sizeof(wchar_t);

	TCHAR cmdLine[2048];

	if (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTCMDLINE, 2048, (LPARAM)cmdLine) <= minCmdLineLen)
		return;

	wchar_t* pos = wcsstr(cmdLine, compareRunCmd);

	if (pos == nullptr)
		return;

	if (pos == cmdLine)
		pos += minCmdLineLen;
	else
		pos = cmdLine;

	auto files = getTwoFilenamesFromCmdLine(pos);

	if (files.first.empty() || files.second.empty())
		return;

	{
		TCHAR tmp[MAX_PATH];

		::PathCanonicalize(tmp, files.first.c_str());

		files.first = tmp;

		::PathCanonicalize(tmp, files.second.c_str());

		files.second = tmp;
	}

	if (!constructFullFilePaths(files))
	{
		::MessageBox(nppData._nppHandle,
				TEXT("Command line file name ambiguous (several openned files with that name). Compare aborted.") \
				TEXT("\nEither use full file paths or add '-nosession' option to command line."),
				PLUGIN_NAME, MB_OK);

		return;
	}

	{
		ScopedIncrementerInt incr(notificationsLock);

		::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)files.first.c_str());

		// First file on the command line is the new one
		setFirst(true);

		::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)files.second.c_str());
	}

	compare();
}


void onToolBarReady()
{
	int bmpX = 0;
	int bmpY = 0;

	int icoX = 0;
	int icoY = 0;

	HDC hdc = ::GetDC(NULL);
	if (hdc)
	{
		// Bitmaps are 16x16
		bmpX = ::MulDiv(16, GetDeviceCaps(hdc, LOGPIXELSX), 96);
		bmpY = ::MulDiv(16, GetDeviceCaps(hdc, LOGPIXELSY), 96);

		gMarginWidth = bmpX;

		// Icons are 32x32
		icoX = ::MulDiv(32, GetDeviceCaps(hdc, LOGPIXELSX), 96);
		icoY = ::MulDiv(32, GetDeviceCaps(hdc, LOGPIXELSY), 96);

		ReleaseDC(NULL, hdc);
	}

	if (!Settings.EnableToolbar)
		return;

	UINT style = (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);

	if (Settings.SetAsFirstTB)
	{
		if (isRTLwindow(nppData._nppHandle))
		{
			tbSetFirst.hToolbarBmp			= (HBITMAP)
				::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST_RTL), IMAGE_BITMAP, bmpX, bmpY, style);
			tbSetFirst.hToolbarIcon			= (HICON)
				::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST_RTL_FL), IMAGE_ICON, icoX, icoY, style);
			tbSetFirst.hToolbarIconDarkMode	= (HICON)
				::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST_RTL_FL_DM), IMAGE_ICON, icoX, icoY, style);
		}
		else
		{
			tbSetFirst.hToolbarBmp			= (HBITMAP)
				::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST), IMAGE_BITMAP, bmpX, bmpY, style);
			tbSetFirst.hToolbarIcon			= (HICON)
				::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST_FL), IMAGE_ICON, icoX, icoY, style);
			tbSetFirst.hToolbarIconDarkMode	= (HICON)
				::LoadImage(hInstance, MAKEINTRESOURCE(IDB_SETFIRST_FL_DM), IMAGE_ICON, icoX, icoY, style);
		}

		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_SET_FIRST]._cmdID, (LPARAM)&tbSetFirst);
	}

	if (Settings.CompareTB)
	{
		tbCompare.hToolbarBmp				= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE), IMAGE_BITMAP, bmpX, bmpY, style);
		tbCompare.hToolbarIcon				= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE_FL), IMAGE_ICON, icoX, icoY, style);
		tbCompare.hToolbarIconDarkMode		= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE_FL_DM), IMAGE_ICON, icoX, icoY, style);

		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_COMPARE]._cmdID, (LPARAM)&tbCompare);
	}

	if (Settings.CompareSelTB)
	{
		tbCompareSel.hToolbarBmp			= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE_LINES), IMAGE_BITMAP, bmpX, bmpY, style);
		tbCompareSel.hToolbarIcon			= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE_LINES_FL), IMAGE_ICON, icoX, icoY, style);
		tbCompareSel.hToolbarIconDarkMode	= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_COMPARE_LINES_FL_DM), IMAGE_ICON, icoX, icoY, style);

		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_COMPARE_SEL]._cmdID, (LPARAM)&tbCompareSel);
	}

	if (Settings.ClearCompareTB)
	{
		tbClearCompare.hToolbarBmp			= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_CLEARCOMPARE), IMAGE_BITMAP, bmpX, bmpY, style);
		tbClearCompare.hToolbarIcon			= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_CLEARCOMPARE_FL), IMAGE_ICON, icoX, icoY, style);
		tbClearCompare.hToolbarIconDarkMode	= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_CLEARCOMPARE_FL_DM), IMAGE_ICON, icoX, icoY, style);

		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_CLEAR_ACTIVE]._cmdID, (LPARAM)&tbClearCompare);
	}

	if (Settings.NavigationTB)
	{
		tbFirst.hToolbarBmp					= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_FIRST), IMAGE_BITMAP, bmpX, bmpY, style);
		tbFirst.hToolbarIcon				= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_FIRST_FL), IMAGE_ICON, icoX, icoY, style);
		tbFirst.hToolbarIconDarkMode		= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_FIRST_FL_DM), IMAGE_ICON, icoX, icoY, style);

		tbPrev.hToolbarBmp					= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREV), IMAGE_BITMAP, bmpX, bmpY, style);
		tbPrev.hToolbarIcon					= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREV_FL), IMAGE_ICON, icoX, icoY, style);
		tbPrev.hToolbarIconDarkMode			= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREV_FL_DM), IMAGE_ICON, icoX, icoY, style);

		tbNext.hToolbarBmp					= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXT), IMAGE_BITMAP, bmpX, bmpY, style);
		tbNext.hToolbarIcon					= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXT_FL), IMAGE_ICON, icoX, icoY, style);
		tbNext.hToolbarIconDarkMode			= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXT_FL_DM), IMAGE_ICON, icoX, icoY, style);

		tbLast.hToolbarBmp					= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_LAST), IMAGE_BITMAP, bmpX, bmpY, style);
		tbLast.hToolbarIcon					= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_LAST_FL), IMAGE_ICON, icoX, icoY, style);
		tbLast.hToolbarIconDarkMode			= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_LAST_FL_DM), IMAGE_ICON, icoX, icoY, style);

		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_FIRST]._cmdID, (LPARAM)&tbFirst);
		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_PREV]._cmdID, (LPARAM)&tbPrev);
		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_NEXT]._cmdID, (LPARAM)&tbNext);
		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_LAST]._cmdID, (LPARAM)&tbLast);
	}

	if (Settings.ShowOnlyDiffsTB)
	{
		tbDiffsOnly.hToolbarBmp				= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_DIFFS_ONLY), IMAGE_BITMAP, bmpX, bmpY, style);
		tbDiffsOnly.hToolbarIcon			= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_DIFFS_ONLY_FL), IMAGE_ICON, icoX, icoY, style);
		tbDiffsOnly.hToolbarIconDarkMode	= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_DIFFS_ONLY_FL_DM), IMAGE_ICON, icoX, icoY, style);

		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_SHOW_ONLY_DIFF]._cmdID, (LPARAM)&tbDiffsOnly);
	}

	if (Settings.NavBarTB)
	{
		tbNavBar.hToolbarBmp				= (HBITMAP)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NAVBAR), IMAGE_BITMAP, bmpX, bmpY, style);
		tbNavBar.hToolbarIcon				= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NAVBAR_FL), IMAGE_ICON, icoX, icoY, style);
		tbNavBar.hToolbarIconDarkMode		= (HICON)
			::LoadImage(hInstance, MAKEINTRESOURCE(IDB_NAVBAR_FL_DM), IMAGE_ICON, icoX, icoY, style);

		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_NAV_BAR]._cmdID, (LPARAM)&tbNavBar);
	}
}


void onNppReady()
{
	// It's N++'s job actually to disable its scroll menu commands but since it's not the case provide this as a patch
	if (isSingleView())
		NppSettings::get().enableNppScrollCommands(false);

	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID,
			(LPARAM)Settings.DetectMoves);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_CHAR_DIFFS]._cmdID,
			(LPARAM)Settings.DetectCharDiffs);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_EMPTY_LINES]._cmdID,
			(LPARAM)Settings.IgnoreEmptyLines);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_FOLDED_LINES]._cmdID,
			(LPARAM)Settings.IgnoreFoldedLines);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_CHANGED_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreChangedSpaces);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_ALL_SPACES]._cmdID,
			(LPARAM)Settings.IgnoreAllSpaces);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_CASE]._cmdID,
			(LPARAM)Settings.IgnoreCase);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_REGEX]._cmdID,
			(LPARAM)Settings.IgnoreRegex);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_SHOW_ONLY_DIFF]._cmdID,
			(LPARAM)Settings.ShowOnlyDiffs);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_SHOW_ONLY_SEL]._cmdID,
			(LPARAM)Settings.ShowOnlySelections);
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID,
			(LPARAM)Settings.UseNavBar);

	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_AUTO_RECOMPARE]._cmdID,
			(LPARAM)Settings.RecompareOnChange);

	if (getNotepadVersion() < MIN_NOTEPADPP_VERSION)
	{
		TCHAR msg[256];

		_sntprintf_s(msg, _countof(msg), _TRUNCATE,
				_T("%s v%s is not compatible with current Notepad++ version.\nPlugin commands will be disabled."),
				PLUGIN_NAME, _T(TO_STR(PLUGIN_VERSION)));

		MessageBox(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

		notificationsLock = 1;

		HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

		for (size_t i = 0; i < NB_MENU_COMMANDS; ++i)
		{
			if (funcItem[i]._pFunc != nullptr)
				::EnableMenuItem(hMenu, funcItem[i]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}

		::DrawMenuBar(nppData._nppHandle);

		HWND hNppToolbar = NppToolbarHandleGetter::get();
		if (hNppToolbar)
		{
			::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, false);
			::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_FIRST]._cmdID, false);
			::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_PREV]._cmdID, false);
			::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_NEXT]._cmdID, false);
			::SendMessage(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_LAST]._cmdID, false);
		}
	}
	else
	{
		if (!allocateIndicator())
			::MessageBox(nppData._nppHandle,
				TEXT("Notepad++ marker allocation for visualizing diff changes failed - ")
				TEXT("\nusing default one but conflicts with other plugins might appear.")
				TEXT("\nPlease switch to Notepad++ version 8.5.6 or newer."),
				PLUGIN_NAME, MB_OK);

		if (isDarkMode())
			Settings.useDarkColors();
		else
			Settings.useLightColors();

		if (!isSQLlibFound())
		{
			HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

			::EnableMenuItem(hMenu, funcItem[CMD_SVN_DIFF]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}

		if (!isGITlibFound())
		{
			HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

			::EnableMenuItem(hMenu, funcItem[CMD_GIT_DIFF]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}

		readNppBookmarkID();

		NppSettings::get().updatePluginMenu();

		checkCmdLine();
	}
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

	bool realign = goToFirst || selectionAutoRecompare || justCompared;

	justCompared = false;

	ScopedIncrementerInt incr(notificationsLock);

	if (!realign)
	{
		const int view = storedLocation ? storedLocation->getView() : getCurrentViewId();

		realign = isAlignmentNeeded(view, cmpPair);
	}

	if (realign)
	{
		LOGD(LOG_NOTIF, "Aligning diffs\n");

		if (!storedLocation && !goToFirst)
			storedLocation = std::make_unique<ViewLocation>(getCurrentViewId());

		selectionAutoRecompare = false;

		alignDiffs(cmpPair);
	}

	if (goToFirst)
	{
		LOGD(LOG_NOTIF, "Go to first diff\n");

		goToFirst = false;

		std::pair<int, intptr_t> viewLoc = jumpToFirstChange(true);

		if (viewLoc.first >= 0)
			syncViews(viewLoc.first);

		cmpPair->setStatus();
	}
	else if (storedLocation)
	{
		if (!realign || (++_consecutiveAligns > 1))
		{
			_consecutiveAligns = 0;
		}
		else if (storedLocation->restore())
		{
			syncViews(storedLocation->getView());
			storedLocation = nullptr;
		}

		// Retry re-alignment one more time - might be needed in case line number margin width has changed
		if (_consecutiveAligns)
		{
			post(30);
		}
		else
		{
			if (realign)
				storedLocation->restore();

			syncViews(storedLocation->getView());

			storedLocation = nullptr;
			cmpPair->setStatus();
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
	ScopedIncrementerInt incr(notificationsLock);

	LOGD(LOG_NOTIF, "onSciUpdateUI()\n");

	storedLocation = std::make_unique<ViewLocation>(getViewId(view));
}


void DelayedUpdate::operator()()
{
	compare(false, false, true);
}


void onMarginClick(HWND view, intptr_t pos, int keyMods)
{
	if (keyMods & SCMOD_ALT)
		return;

	const int viewId = getViewId(view);

	if ((keyMods & SCMOD_CTRL) && CallScintilla(viewId, SCI_GETREADONLY, 0, 0))
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(viewId));
	if (cmpPair == compareList.end())
		return;

	const intptr_t line = CallScintilla(viewId, SCI_LINEFROMPOSITION, pos, 0);

	if (!isLineMarked(viewId, line, MARKER_MASK_LINE) && !isLineAnnotated(viewId, line))
		return;

	int mark = MARKER_MASK_LINE;

	if (keyMods & SCMOD_SHIFT)
	{
		const int markerMask = static_cast<int>(CallScintilla(viewId, SCI_MARKERGET, line, 0));

		if (markerMask & (1 << MARKER_CHANGED_LINE))
			mark = (1 << MARKER_CHANGED_LINE);
		else if (markerMask & MARKER_MASK_LINE)
			mark = (1 << MARKER_ADDED_LINE) | (1 << MARKER_REMOVED_LINE) | (1 << MARKER_MOVED_LINE);
	}

	const int otherViewId = getOtherViewId(viewId);

	if ((keyMods & SCMOD_SHIFT) && (mark == (1 << MARKER_CHANGED_LINE)))
	{
		const intptr_t startPos		= getLineStart(viewId, line);
		const intptr_t endPos		= getLineStart(viewId, line + 1);
		const intptr_t otherLine	= otherViewMatchingLine(viewId, line, 0, true);

		if ((otherLine < 0) ||
			!(CallScintilla(otherViewId, SCI_MARKERGET, otherLine, 0) & (1 << MARKER_CHANGED_LINE)))
		{
			if (!(keyMods & SCMOD_CTRL))
				setSelection(viewId, startPos, endPos);

			temporaryRangeSelect(-1);

			return;
		}

		if (!(keyMods & SCMOD_CTRL))
		{
			setSelection(viewId, startPos, endPos);

			temporaryRangeSelect(otherViewId,
					getLineStart(otherViewId, otherLine), getLineStart(otherViewId, otherLine + 1));

			return;
		}

		const auto text =
				getText(otherViewId, getLineStart(otherViewId, otherLine), getLineStart(otherViewId, otherLine + 1));

		const bool lineFolded = isLineFoldedFoldPoint(otherViewId, otherLine);
		const bool lastMarked = (endPos == CallScintilla(viewId, SCI_GETLENGTH, 0, 0));

		ScopedIncrementerInt		inEqualize(cmpPair->inEqualizeMode);
		ScopedViewUndoAction		scopedUndo(viewId);
		ScopedFirstVisibleLineStore	firstVisLine(viewId);

		clearSelection(viewId);
		temporaryRangeSelect(-1);

		if (!cmpPair->options.recompareOnChange)
		{
			copiedSectionMarks = getMarkers(otherViewId, otherLine, 1, MARKER_MASK_ALL);
			clearAnnotation(otherViewId, otherLine);
		}
		else
		{
			clearMarks(otherViewId, otherLine, 1);
		}

		CallScintilla(viewId, SCI_DELETERANGE, startPos, endPos - startPos);

		if (lastMarked)
			clearMarks(viewId, line);

		CallScintilla(viewId, SCI_INSERTTEXT, startPos, (LPARAM)text.data());

		if (lineFolded)
			CallScintilla(viewId, SCI_FOLDLINE, line, SC_FOLDACTION_CONTRACT);

		if (Settings.FollowingCaret)
			CallScintilla(viewId, SCI_SETEMPTYSELECTION, startPos, 0);

		if (!isLineVisible(viewId, line))
			firstVisLine.set(CallScintilla(viewId, SCI_VISIBLEFROMDOCLINE, line, 0));

		if (!cmpPair->options.recompareOnChange)
		{
			if (Settings.ShowOnlyDiffs)
				alignDiffs(cmpPair);

			if (Settings.UseNavBar)
				NavDlg.Show();
		}

		return;
	}

	std::pair<intptr_t, intptr_t> markedRange = getMarkedSection(viewId, line, line, mark);

	if ((markedRange.first < 0) && !(keyMods & SCMOD_SHIFT))
		markedRange = getMarkedSection(viewId, line + 1, line + 1, mark);

	if (cmpPair->options.findUniqueMode)
	{
		if (!(keyMods & SCMOD_CTRL))
		{
			if (markedRange.first >= 0)
				setSelection(viewId, markedRange.first, markedRange.second);
			else
				clearSelection(viewId);

			temporaryRangeSelect(-1);
		}

		return;
	}

	std::pair<intptr_t, intptr_t> otherMarkedRange;

	if (markedRange.first < 0)
	{
		const intptr_t lastLine = CallScintilla(viewId, SCI_GETLINECOUNT, 0, 0) - 1;

		otherMarkedRange.first	= otherViewMatchingLine(viewId, line, getWrapCount(viewId, line));

		if (line < lastLine)
			otherMarkedRange.second	= otherViewMatchingLine(viewId, line + 1, -1);
		else
			otherMarkedRange.second = CallScintilla(otherViewId, SCI_GETLINECOUNT, 0, 0) - 1;
	}
	else
	{
		intptr_t startLine		= CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0);
		intptr_t startOffset	= 0;

		if (!Settings.ShowOnlyDiffs && (startLine > 1) && isLineAnnotated(viewId, startLine - 1))
		{
			--startLine;
			startOffset = getWrapCount(viewId, startLine);
		}

		intptr_t endLine = CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.second, 0);

		if (getLineStart(viewId, endLine) == markedRange.second)
			--endLine;

		intptr_t endOffset = getWrapCount(viewId, endLine) - 1;

		if (!Settings.ShowOnlyDiffs)
			endOffset += getLineAnnotation(viewId, endLine);

		otherMarkedRange.first = otherViewMatchingLine(viewId, startLine, startOffset, true);

		if (otherMarkedRange.first < 0)
			otherMarkedRange.first = otherViewMatchingLine(viewId, startLine, startOffset) + 1;

		otherMarkedRange.second	= otherViewMatchingLine(viewId, endLine, endOffset);
	}

	if (!Settings.ShowOnlyDiffs)
	{
		for (; (otherMarkedRange.first <= otherMarkedRange.second) &&
				!isLineMarked(otherViewId, otherMarkedRange.first, mark); ++otherMarkedRange.first);
	}
	else
	{
		intptr_t otherLine = otherMarkedRange.second;

		for (; (otherLine > otherMarkedRange.first) && isLineMarked(otherViewId, otherLine, mark); --otherLine);

		if (otherLine > otherMarkedRange.first)
			otherMarkedRange.first = otherLine + 1;
	}

	if (otherMarkedRange.first > otherMarkedRange.second)
	{
		otherMarkedRange.first = -1;
	}
	else
	{
		for (; (otherMarkedRange.second >= otherMarkedRange.first) &&
				!isLineMarked(otherViewId, otherMarkedRange.second, mark); --otherMarkedRange.second);

		if (otherMarkedRange.second < otherMarkedRange.first)
		{
			otherMarkedRange.first = -1;
		}
		else
		{
			otherMarkedRange.first	= getLineStart(otherViewId, otherMarkedRange.first);
			otherMarkedRange.second	= getLineStart(otherViewId, otherMarkedRange.second + 1);
		}
	}

	if (!(keyMods & SCMOD_CTRL))
	{
		if (markedRange.first >= 0)
			setSelection(viewId, markedRange.first, markedRange.second);
		else
			clearSelection(viewId);

		if (otherMarkedRange.first >= 0)
			temporaryRangeSelect(otherViewId, otherMarkedRange.first, otherMarkedRange.second);
		else
			temporaryRangeSelect(-1);

		return;
	}

	ScopedIncrementerInt		inEqualize(cmpPair->inEqualizeMode);
	ScopedViewUndoAction		scopedUndo(viewId);
	ScopedFirstVisibleLineStore	firstVisLine(viewId);

	clearSelection(viewId);
	temporaryRangeSelect(-1);

	if (otherMarkedRange.first >= 0)
	{
		const intptr_t otherStartLine	= CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.first, 0);
		intptr_t otherEndLine			= CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.second, 0);

		if (otherMarkedRange.second == CallScintilla(otherViewId, SCI_GETLENGTH, 0, 0))
			++otherEndLine;

		if (!cmpPair->options.recompareOnChange)
		{
			copiedSectionMarks =
					getMarkers(otherViewId, otherStartLine, otherEndLine - otherStartLine, MARKER_MASK_ALL);
			clearAnnotations(otherViewId, otherStartLine, otherEndLine - otherStartLine);
		}
		else
		{
			clearMarks(otherViewId, otherStartLine, otherEndLine - otherStartLine);
		}
	}

	const intptr_t lastLine = CallScintilla(viewId, SCI_GETLINECOUNT, 0, 0) - 1;

	if (markedRange.first >= 0)
	{
		const intptr_t startLine = CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0);

		if ((otherMarkedRange.first >= 0) && (startLine > 0))
			clearAnnotation(viewId, startLine - 1);

		const bool lastMarked = (markedRange.second == CallScintilla(viewId, SCI_GETLENGTH, 0, 0));

		if (Settings.FollowingCaret)
			CallScintilla(viewId, SCI_SETEMPTYSELECTION, markedRange.first, 0);

		if (!isLineVisible(viewId, startLine))
			firstVisLine.set(CallScintilla(viewId, SCI_VISIBLEFROMDOCLINE, startLine, 0));

		CallScintilla(viewId, SCI_DELETERANGE, markedRange.first, markedRange.second - markedRange.first);

		if (lastMarked)
			clearMarks(viewId, CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0));
	}

	if (otherMarkedRange.first >= 0)
	{
		const intptr_t otherStartLine = CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.first, 0);

		bool copyOtherTillEnd = false;

		intptr_t startPos = markedRange.first;

		if (startPos < 0)
		{
			if (line < lastLine)
			{
				if (!Settings.ShowOnlyDiffs)
				{
					startPos = line + 1;
				}
				else
				{
					if (otherStartLine < 0)
						return;

					startPos = getAlignmentLine(cmpPair->summary.alignmentInfo, otherViewId, otherStartLine);

					if (startPos < 0)
						return;
				}

				startPos = getLineStart(viewId, startPos);
			}
			else
			{
				startPos = getLineEnd(viewId, line);

				if (otherStartLine > 0)
					otherMarkedRange.first = getLineEnd(otherViewId, otherStartLine - 1);

				copyOtherTillEnd = true;
			}

			clearAnnotation(viewId, line);
		}
		else if (CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0) == lastLine)
		{
			copyOtherTillEnd = true;
		}

		const bool endLineFolded = (!copyOtherTillEnd && isLineFoldedFoldPoint(otherViewId,
				CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.second, 0) - 1));

		if (copyOtherTillEnd)
			otherMarkedRange.second = getLineEnd(otherViewId, CallScintilla(otherViewId, SCI_GETLINECOUNT, 0, 0) - 1);

		const auto text = getText(otherViewId, otherMarkedRange.first, otherMarkedRange.second);

		if (otherStartLine > 0)
			clearAnnotation(otherViewId, otherStartLine - 1);

		CallScintilla(viewId, SCI_INSERTTEXT, startPos, (LPARAM)text.data());

		if (endLineFolded)
			CallScintilla(viewId, SCI_FOLDLINE, CallScintilla(viewId, SCI_LINEFROMPOSITION,
					startPos + otherMarkedRange.second - otherMarkedRange.first, 0) - 1, SC_FOLDACTION_CONTRACT);

		if (Settings.FollowingCaret)
			CallScintilla(viewId, SCI_SETEMPTYSELECTION, startPos, 0);

		const intptr_t firstLine = CallScintilla(viewId, SCI_LINEFROMPOSITION, startPos, 0);

		if (!isLineVisible(viewId, firstLine))
			firstVisLine.set(CallScintilla(viewId, SCI_VISIBLEFROMDOCLINE, firstLine, 0));
	}

	if (!cmpPair->options.recompareOnChange)
	{
		if (Settings.ShowOnlyDiffs)
			alignDiffs(cmpPair);

		if (Settings.UseNavBar)
			NavDlg.Show();
	}
}


void onSciModified(SCNotification* notifyCode)
{
	static bool notReverting = true;

	const int view = getViewIdSafe((HWND)notifyCode->nmhdr.hwndFrom);
	if (view < 0)
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(view));
	if (cmpPair == compareList.end())
		return;

	// For some reason this notification is never sent by Notepad++ and Scintilla eventhough it is allowed
	// by SCI_SETMODEVENTMASK
	// if (notifyCode->modificationType & SC_MOD_CHANGEFOLD)
	// {
		// CallScintilla(MAIN_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
		// CallScintilla(SUB_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);

		// // Use that flag to trigger force re-alignment (nothing to do with selections actually, just reuse the flag)
		// selectionAutoRecompare = true;

		// delayedAlignment.post(30);

		// return;
	// }

	std::shared_ptr<DeletedSection::UndoData> undo = nullptr;

	if (notifyCode->modificationType & SC_MOD_BEFOREDELETE)
	{
		const intptr_t startLine	= CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);
		const intptr_t endLine		= CallScintilla(view, SCI_LINEFROMPOSITION,
				notifyCode->position + notifyCode->length, 0);

		// Change is on single line?
		if (endLine <= startLine)
			return;

		LOGD(LOG_NOTIF, "SC_MOD_BEFOREDELETE: " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
				" view, lines range: " + std::to_string(startLine + 1) + "-" + std::to_string(endLine) + "\n");

		const int action = notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

		ScopedIncrementerInt incr(notificationsLock);

		if (cmpPair->options.selectionCompare)
		{
			undo = std::make_shared<DeletedSection::UndoData>();

			undo->selection = cmpPair->options.selections[view];
		}

		if (!cmpPair->options.recompareOnChange)
		{
			if (!undo)
				undo = std::make_shared<DeletedSection::UndoData>();

			undo->alignment = cmpPair->summary.alignmentInfo;

			if (cmpPair->inEqualizeMode && !copiedSectionMarks.empty())
				undo->otherViewMarks = std::move(copiedSectionMarks);
		}

		notReverting = cmpPair->getFileByViewId(view).pushDeletedSection(action, startLine, endLine - startLine, undo,
				cmpPair->options.recompareOnChange);

#ifdef DLOG
		if (notReverting)
		{
			if (undo)
			{
				if (undo->selection.first >= 0)
				{
					LOGD(LOG_NOTIF, "Selection stored.\n");
				}

				if (!undo->alignment.empty())
				{
					LOGD(LOG_NOTIF, "Alignment stored.\n");
				}

				if (!undo->otherViewMarks.empty())
				{
					LOGD(LOG_NOTIF, "Other view markers stored.\n");
				}
			}
		}
#endif

		return;
	}

	bool selectionsAdjusted = false;

	if ((notifyCode->modificationType & SC_MOD_INSERTTEXT) && notifyCode->linesAdded)
	{
		const intptr_t startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);

		const int action = notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

		LOGD(LOG_NOTIF, "SC_MOD_INSERTTEXT: " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
				" view, lines range: " + std::to_string(startLine + 1) + "-" +
				std::to_string(startLine + notifyCode->linesAdded) + "\n");

		ScopedIncrementerInt incr(notificationsLock);

		notReverting = true;

		undo = cmpPair->getFileByViewId(view).popDeletedSection(action, startLine);

		if (undo)
		{
			if ((undo->selection.first < undo->selection.second) &&
				(cmpPair->options.selections[view] != undo->selection))
			{
				cmpPair->options.selections[view] = undo->selection;

				selectionsAdjusted = true;

				LOGD(LOG_NOTIF, "Selection restored.\n");
			}

			if (!cmpPair->options.recompareOnChange)
			{
				cmpPair->summary.alignmentInfo = std::move(undo->alignment);

				LOGD(LOG_NOTIF, "Alignment restored.\n");

				if (!undo->otherViewMarks.empty())
				{
					const intptr_t alignLine = getAlignmentLine(cmpPair->summary.alignmentInfo, view, startLine);

					if (alignLine >= 0)
					{
						setMarkers(getOtherViewId(view), alignLine, undo->otherViewMarks);

						if (Settings.ShowOnlyDiffs)
							showRange(getOtherViewId(view), alignLine, undo->otherViewMarks.size());

						LOGD(LOG_NOTIF, "Other view markers restored.\n");
					}
				}
			}
		}
	}

	if ((notifyCode->modificationType & SC_MOD_DELETETEXT) || (notifyCode->modificationType & SC_MOD_INSERTTEXT))
	{
		delayedAlignment.cancel();
		delayedUpdate.cancel();

		if (notifyCode->linesAdded == 0)
			notReverting = true;

		bool updateStatus = false;

		// Set compare dirty flag if needed
		if (!cmpPair->options.recompareOnChange && notReverting && !undo)
		{
			if (!cmpPair->compareDirty || (!cmpPair->inEqualizeMode && !cmpPair->manuallyChanged))
			{
				if (!cmpPair->options.selectionCompare)
				{
					cmpPair->setCompareDirty();
					updateStatus = true;
				}
				else
				{
					intptr_t startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);

					if ((startLine >= cmpPair->options.selections[view].first) &&
						(startLine <= cmpPair->options.selections[view].second))
					{
						cmpPair->setCompareDirty();
						updateStatus = true;
					}

					if (!updateStatus && notifyCode->linesAdded && (notifyCode->modificationType & SC_MOD_DELETETEXT))
					{
						startLine += notifyCode->linesAdded + 1;

						if ((startLine >= cmpPair->options.selections[view].first) &&
							(startLine <= cmpPair->options.selections[view].second))
						{
							cmpPair->setCompareDirty();
							updateStatus = true;
						}
					}
				}
			}
		}

		// Adjust selections if in selection compare
		if (cmpPair->options.selectionCompare && notifyCode->linesAdded && !undo && !selectionsAdjusted)
		{
			intptr_t startLine		= CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);
			const intptr_t endLine	= startLine + std::abs(notifyCode->linesAdded) - 1;

			if (cmpPair->options.selections[view].first > startLine)
			{
				if (notifyCode->linesAdded > 0)
				{
					cmpPair->options.selections[view].first += notifyCode->linesAdded;
				}
				else
				{
					if (cmpPair->options.selections[view].first > endLine)
						cmpPair->options.selections[view].first += notifyCode->linesAdded;
					else
						cmpPair->options.selections[view].first -=
								(cmpPair->options.selections[view].first - startLine);
				}

				selectionsAdjusted = true;
			}

			// Handle the special case when the end of the selection compare is diff-equalized -
			// the insert part of the replace notification then needs to increase the selection accordingly to
			// include the equalized diff
			if (cmpPair->inEqualizeMode && (cmpPair->options.selections[view].second == startLine - 1) &&
					(notifyCode->linesAdded > 0))
				--startLine;

			if (cmpPair->options.selections[view].second >= startLine)
			{
				if (notifyCode->linesAdded > 0)
				{
					cmpPair->options.selections[view].second += notifyCode->linesAdded;
				}
				else
				{
					if (cmpPair->options.selections[view].second >= endLine)
						cmpPair->options.selections[view].second += notifyCode->linesAdded;
					else
						cmpPair->options.selections[view].second -=
								(cmpPair->options.selections[view].second - startLine + 1);
				}

				selectionsAdjusted = true;
			}

			if (!cmpPair->inEqualizeMode &&
				cmpPair->options.selections[view].second < cmpPair->options.selections[view].first)
			{
				clearComparePair(getCurrentBuffId());
				return;
			}

			LOGDIF(LOG_NOTIF, selectionsAdjusted, "Selection adjusted.\n");
		}

		if (cmpPair->options.recompareOnChange)
		{
			if (notifyCode->linesAdded)
				cmpPair->autoUpdateDelay = 500;
			else
				// Leave bigger delay before re-compare if change is on single line because the user might be typing
				// and we shouldn't interrupt / interfere
				cmpPair->autoUpdateDelay = 1000;

			return;
		}

		if (notifyCode->linesAdded)
		{
			intptr_t startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);

			if (!undo)
			{
				if (!cmpPair->options.selectionCompare || selectionsAdjusted)
				{
					cmpPair->adjustAlignment(view, startLine, notifyCode->linesAdded);

					LOGD(LOG_NOTIF, "Alignment adjusted.\n");
				}
			}

			if (selectionsAdjusted)
			{
				CallScintilla(MAIN_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
				CallScintilla(SUB_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);

				selectionAutoRecompare = true; // Force re-alignment in onSciPaint()
			}

			if (Settings.UseNavBar && !cmpPair->inEqualizeMode)
				NavDlg.Show();
		}

		if (updateStatus)
			cmpPair->setStatus();
	}
}


void onSciZoom()
{
	CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	ScopedIncrementerInt incr(notificationsLock);

	// sync both views zoom
	const int zoom = static_cast<int>(CallScintilla(getCurrentViewId(), SCI_GETZOOM, 0, 0));
	CallScintilla(getOtherViewId(), SCI_SETZOOM, zoom, 0);

	NppSettings::get().setCompareZoom(zoom);
}


void DelayedActivate::operator()()
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	LOGDB(LOG_NOTIF, buffId, "Activate\n");

	if (buffId != currentlyActiveBuffID)
	{
		const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

		ScopedIncrementerInt incr(notificationsLock);

		// When compared file is activated make sure its corresponding pair file is also active in the other view
		if (getDocId(getOtherViewId()) != otherFile.sciDoc)
		{
			activateBufferID(otherFile.buffId);
			activateBufferID(buffId);
		}

		currentlyActiveBuffID = buffId;

		comparedFileActivated();

		onSciUpdateUI(getView(viewIdFromBuffId(buffId)));
	}
	else
	{
		// NPPN_BUFFERACTIVATED for the same buffer is received if file is reloaded or if we try to close an
		// unsaved file. We try to distinguish here the reason for the notification - if file is saved then it
		// seems reloaded and we want to update the compare.
		if (isCurrentFileSaved())
		{
			delayedAlignment.cancel();
			delayedUpdate.post(30);
		}
	}
}


void onBufferActivated(LRESULT buffId)
{
	delayedAlignment.cancel();
	delayedUpdate.cancel();
	delayedActivation.cancel();

	// If compared pair was not active explicitly release mouse key as it might have been pressed and make a
	// false selection when files are activated and compare mode is set
	if (!NppSettings::get().compareMode)
	{
		INPUT inputs[1] = {};
		::ZeroMemory(inputs, sizeof(inputs));

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = ::GetSystemMetrics(SM_SWAPBUTTON) ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;

		::SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
	}

	ScopedIncrementerInt incr(notificationsLock);

	LOGDB(LOG_NOTIF, buffId, "onBufferActivated()\n");

	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
	{
		NppSettings::get().setNormalMode();
		setNormalView(getCurrentViewId());
		resetCompareView(getOtherViewId());

		currentlyActiveBuffID = buffId;
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

	ScopedIncrementerInt incr(notificationsLock);

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
				LOGDB(LOG_NOTIF, otherFile.buffId, "Close\n");

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
		moveFileToOtherView();
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

	ScopedIncrementerInt incr(notificationsLock);

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


void onFileSaved(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

	const LRESULT currentBuffId = getCurrentBuffId();
	const bool pairIsActive = (currentBuffId == buffId || currentBuffId == otherFile.buffId);

	ScopedIncrementerInt incr(notificationsLock);

	if (!pairIsActive)
	{
		activateBufferID(buffId);
	}
	else if (cmpPair->options.recompareOnChange && cmpPair->autoUpdateDelay)
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

} // anonymous namespace


void ToggleNavigationBar()
{
	Settings.UseNavBar = !Settings.UseNavBar;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID, (LPARAM)Settings.UseNavBar);
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
	nppData = notpadPlusData;

	sciFunc		= (SciFnDirect)::SendMessage(notpadPlusData._scintillaMainHandle, SCI_GETDIRECTFUNCTION, 0, 0);
	sciPtr[0]	= (sptr_t)::SendMessage(notpadPlusData._scintillaMainHandle, SCI_GETDIRECTPOINTER, 0, 0);
	sciPtr[1]	= (sptr_t)::SendMessage(notpadPlusData._scintillaSecondHandle, SCI_GETDIRECTPOINTER, 0, 0);

	if (!sciFunc || !sciPtr[0] || !sciPtr[1])
	{
		exit(EXIT_FAILURE);
	}

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
			if (NppSettings::get().compareMode && !notificationsLock && !storedLocation && !goToFirst &&
				!delayedActivation && !delayedClosure && !delayedUpdate &&
				(notifyCode->updated & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL)))
			{
				onSciUpdateUI((HWND)notifyCode->nmhdr.hwndFrom);
			}
		break;

		case SCN_MARGINCLICK:
			if (NppSettings::get().compareMode && !notificationsLock &&
					!delayedActivation && !delayedClosure && !delayedUpdate && (notifyCode->margin == MARGIN_NUM))
				onMarginClick((HWND)notifyCode->nmhdr.hwndFrom, notifyCode->position, notifyCode->modifiers);
		break;

		case NPPN_BUFFERACTIVATED:
			if (!compareList.empty() && !notificationsLock && !delayedClosure)
				onBufferActivated(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILEBEFORECLOSE:
			if (newCompare && (newCompare->pair.file[0].buffId == static_cast<LRESULT>(notifyCode->nmhdr.idFrom)))
				newCompare = nullptr;
#ifdef DLOG
			else if (dLogBuf == static_cast<LRESULT>(notifyCode->nmhdr.idFrom))
				dLogBuf = -1;
#endif
			else if (!compareList.empty() && !notificationsLock)
				onFileBeforeClose(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILESAVED:
			if (!compareList.empty() && !notificationsLock)
				onFileSaved(notifyCode->nmhdr.idFrom);
		break;

		// This is used to monitor fold state and deletion of lines to properly clear their compare markings
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
					NppSettings::get().setMainZoom(static_cast<int>(CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0)));
					NppSettings::get().setSubZoom(static_cast<int>(CallScintilla(SUB_VIEW, SCI_GETZOOM, 0, 0)));
				}
			}
		break;

		case NPPN_LANGCHANGED:
			if (NppSettings::get().compareMode)
			{
				CompareList_t::iterator	cmpPair = getCompare(notifyCode->nmhdr.idFrom);
				if (cmpPair != compareList.end())
					cmpPair->setStatus();
			}
		break;

		case NPPN_WORDSTYLESUPDATED:
		case NPPN_DARKMODECHANGED:
			if (isDarkMode())
				Settings.useDarkColors();
			else
				Settings.useLightColors();

			if (!compareList.empty())
			{
				setStyles(Settings);
				NavDlg.SetColors(Settings.colors());

				if (NppSettings::get().compareMode)
				{
					setCompareView(MAIN_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);
					setCompareView(SUB_VIEW, Settings.colors().blank, Settings.colors().caret_line_transparency);
				}
			}
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


extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM)
{
	return TRUE;
}


extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
