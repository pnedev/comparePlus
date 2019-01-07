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

#pragma once

#include <vector>
#include <utility>

#include "Compare.h"


enum Marker_t
{
	MARKER_CHANGED_LINE = 0,
	MARKER_ADDED_LINE,
	MARKER_REMOVED_LINE,
	MARKER_MOVED_LINE,
	MARKER_BLANK,
	MARKER_CHANGED_SYMBOL,
	MARKER_CHANGED_LOCAL_SYMBOL,
	MARKER_ADDED_SYMBOL,
	MARKER_ADDED_LOCAL_SYMBOL,
	MARKER_REMOVED_SYMBOL,
	MARKER_REMOVED_LOCAL_SYMBOL,
	MARKER_MOVED_LINE_SYMBOL,
	MARKER_MOVED_BLOCK_BEGIN_SYMBOL,
	MARKER_MOVED_BLOCK_MID_SYMBOL,
	MARKER_MOVED_BLOCK_END_SYMBOL,
	MARKER_ARROW_SYMBOL
};


const int MARKER_MASK_CHANGED		=	(1 << MARKER_CHANGED_LINE)	|	(1 << MARKER_CHANGED_SYMBOL);
const int MARKER_MASK_CHANGED_LOCAL	=	(1 << MARKER_CHANGED_LINE)	|	(1 << MARKER_CHANGED_LOCAL_SYMBOL);
const int MARKER_MASK_ADDED			=	(1 << MARKER_ADDED_LINE)	|	(1 << MARKER_ADDED_SYMBOL);
const int MARKER_MASK_ADDED_LOCAL	=	(1 << MARKER_ADDED_LINE)	|	(1 << MARKER_ADDED_LOCAL_SYMBOL);
const int MARKER_MASK_REMOVED		=	(1 << MARKER_REMOVED_LINE)	|	(1 << MARKER_REMOVED_SYMBOL);
const int MARKER_MASK_REMOVED_LOCAL	=	(1 << MARKER_REMOVED_LINE)	|	(1 << MARKER_REMOVED_LOCAL_SYMBOL);
const int MARKER_MASK_MOVED_LINE	=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_LINE_SYMBOL);
const int MARKER_MASK_MOVED_BEGIN	=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_BLOCK_BEGIN_SYMBOL);
const int MARKER_MASK_MOVED_MID		=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_BLOCK_MID_SYMBOL);
const int MARKER_MASK_MOVED_END		=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_BLOCK_END_SYMBOL);
const int MARKER_MASK_MOVED			=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_LINE_SYMBOL) |
																		(1 << MARKER_MOVED_BLOCK_BEGIN_SYMBOL) |
																		(1 << MARKER_MOVED_BLOCK_MID_SYMBOL) |
																		(1 << MARKER_MOVED_BLOCK_END_SYMBOL);

const int MARKER_MASK_BLANK			=	(1 << MARKER_BLANK);
const int MARKER_MASK_ARROW			=	(1 << MARKER_ARROW_SYMBOL);

const int MARKER_MASK_LINE			=	(1 << MARKER_CHANGED_LINE) |
										(1 << MARKER_ADDED_LINE) |
										(1 << MARKER_REMOVED_LINE) |
										(1 << MARKER_MOVED_LINE);

const int MARKER_MASK_SYMBOL		=	(1 << MARKER_CHANGED_SYMBOL) |
										(1 << MARKER_CHANGED_LOCAL_SYMBOL) |
										(1 << MARKER_ADDED_SYMBOL) |
										(1 << MARKER_ADDED_LOCAL_SYMBOL) |
										(1 << MARKER_REMOVED_SYMBOL) |
										(1 << MARKER_REMOVED_LOCAL_SYMBOL) |
										(1 << MARKER_MOVED_LINE_SYMBOL) |
										(1 << MARKER_MOVED_BLOCK_BEGIN_SYMBOL) |
										(1 << MARKER_MOVED_BLOCK_MID_SYMBOL) |
										(1 << MARKER_MOVED_BLOCK_END_SYMBOL);

const int MARKER_MASK_ALL				=	MARKER_MASK_LINE | MARKER_MASK_SYMBOL;
const int MARKER_MASK_ALL_PLUS_BLANK	=	MARKER_MASK_ALL | MARKER_MASK_BLANK;


/**
 *  \class
 *  \brief
 */
class NppToolbarHandleGetter
{
public:
	static HWND get();

private:
	static HWND	hNppToolbar;

	static BOOL CALLBACK enumWindowsCB(HWND hwnd, LPARAM lParam);
};


/**
 *  \class
 *  \brief
 */
class NppTabHandleGetter
{
public:
	static HWND get(int viewId);

private:
	static HWND	hNppTab[2];

	static BOOL CALLBACK enumWindowsCB(HWND hwnd, LPARAM lParam);
};


/**
 *  \struct
 *  \brief
 *  \warning  Don't use that helper struct if somewhere in its scope the view document is changed!!!
 */
struct ScopedViewWriteEnabler
{
	ScopedViewWriteEnabler(int view) : _view(view)
	{
		_isRO = CallScintilla(_view, SCI_GETREADONLY, 0, 0);
		if (_isRO)
			CallScintilla(_view, SCI_SETREADONLY, false, 0);
	}

	~ScopedViewWriteEnabler()
	{
		if (_isRO)
			CallScintilla(_view, SCI_SETREADONLY, true, 0);
	}

private:
	int	_view;
	int	_isRO;
};


/**
 *  \struct
 *  \brief
 *  \warning  Don't use that helper struct if somewhere in its scope the view document is changed!!!
 */
struct ScopedViewUndoCollectionBlocker
{
	ScopedViewUndoCollectionBlocker(int view) : _view(view)
	{
		_isUndoOn = CallScintilla(_view, SCI_GETUNDOCOLLECTION, 0, 0);
		if (_isUndoOn)
		{
			CallScintilla(_view, SCI_SETUNDOCOLLECTION, false, 0);
			CallScintilla(_view, SCI_EMPTYUNDOBUFFER, 0, 0);
		}
	}

	~ScopedViewUndoCollectionBlocker()
	{
		if (_isUndoOn)
			CallScintilla(_view, SCI_SETUNDOCOLLECTION, true, 0);
	}

private:
	int	_view;
	int	_isUndoOn;
};


/**
 *  \struct
 *  \brief
 */
struct ViewLocation
{
	ViewLocation() : _view(-1) {}

	ViewLocation(int view, int centerLine)
	{
		save(view, centerLine);
	}

	ViewLocation(int view)
	{
		save(view);
	}

#ifdef DLOG

	~ViewLocation()
	{
		LOGD("Clear view location\n");
	}

#endif

	void save(int view, int centerLine = -1);
	bool restore() const;

	inline int getView() const
	{
		return _view;
	}

private:
	int		_view;
	int		_centerLine;
	int		_visibleLineOffset;
	int		_pos;
	int		_selStart;
	int		_selEnd;
};


inline bool isRTLwindow(HWND hWin)
{
	return ((::GetWindowLongPtr(hWin, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0);
}


inline bool isSingleView()
{
	return (!::IsWindowVisible(nppData._scintillaSecondHandle) || !::IsWindowVisible(nppData._scintillaMainHandle));
}


inline bool isFileEmpty(int view)
{
	return (CallScintilla(view, SCI_GETLENGTH, 0, 0) == 0);
}


inline bool getWrapMode()
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	return (::GetMenuState(hMenu, IDM_VIEW_WRAP, MF_BYCOMMAND) & MF_CHECKED) != 0;
}


inline int getNumberOfFiles()
{
	return ((::IsWindowVisible(nppData._scintillaMainHandle) ?
				::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, PRIMARY_VIEW) : 0) +
			(::IsWindowVisible(nppData._scintillaSecondHandle) ?
				::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, SECOND_VIEW) : 0));
}


inline int getNumberOfFiles(int viewId)
{
	return ::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, viewId == MAIN_VIEW ? PRIMARY_VIEW : SECOND_VIEW);
}


inline HWND getView(int viewId)
{
	return (viewId == MAIN_VIEW) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}


inline int getViewId(HWND view)
{
	return (view == nppData._scintillaMainHandle) ? MAIN_VIEW : SUB_VIEW;
}


inline int getCurrentViewId()
{
	return ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0);
}


inline HWND getCurrentView()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ?
			nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}


inline int getOtherViewId()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW;
}


inline HWND getOtherView()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ?
			nppData._scintillaSecondHandle : nppData._scintillaMainHandle;
}


inline int getOtherViewId(int view)
{
	return (view == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW;
}


inline HWND getOtherView(int view)
{
	return (view == MAIN_VIEW) ? nppData._scintillaSecondHandle : nppData._scintillaMainHandle;
}


inline HWND getOtherView(HWND view)
{
	return (view == nppData._scintillaMainHandle) ? nppData._scintillaSecondHandle : nppData._scintillaMainHandle;
}


inline int viewIdFromBuffId(LRESULT buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	return (index >> 30);
}


inline int posFromBuffId(LRESULT buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	return (index & 0x3FFFFFFF);
}


inline LRESULT getCurrentBuffId()
{
	return ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
}


inline int getEncoding(LRESULT buffId)
{
	return ::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, buffId, 0);
}


inline int getDocId(int view)
{
	return CallScintilla(view, SCI_GETDOCPOINTER, 0, 0);
}


inline int getCurrentLine(int view)
{
	return CallScintilla(view, SCI_LINEFROMPOSITION, CallScintilla(view, SCI_GETCURRENTPOS, 0, 0), 0);
}


inline int getCurrentVisibleLine(int view)
{
	return CallScintilla(view, SCI_VISIBLEFROMDOCLINE,
			CallScintilla(view, SCI_LINEFROMPOSITION, CallScintilla(view, SCI_GETCURRENTPOS, 0, 0), 0), 0);
}


inline int getFirstVisibleLine(int view)
{
	return CallScintilla(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
}


inline int getFirstLine(int view)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, getFirstVisibleLine(view), 0);
}


inline int getLastVisibleLine(int view)
{
	return (getFirstVisibleLine(view) + CallScintilla(view, SCI_LINESONSCREEN, 0, 0) - 1);
}


inline int getLastLine(int view)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, getLastVisibleLine(view), 0);
}


inline int getUnhiddenLine(int view, int line)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0), 0);
}


inline int getPreviousUnhiddenLine(int view, int line)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - 1, 0);
}


inline int otherViewMatchingLine(int view, int line)
{
	const int otherView = getOtherViewId(view);

	line = CallScintilla(otherView, SCI_DOCLINEFROMVISIBLE,
			CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0), 0);

	return (line == CallScintilla(otherView, SCI_GETLINECOUNT, 0, 0)) ? line - 1 : line;
}


inline void gotoClosestUnhiddenLine(int view)
{
	CallScintilla(view, SCI_GOTOLINE, getUnhiddenLine(view, getCurrentLine(view)), 0);
}


inline int getWrapCount(int view, int line)
{
	return CallScintilla(view, SCI_WRAPCOUNT, line, 0);
}


inline int getLineAnnotation(int view, int line)
{
	return CallScintilla(view, SCI_ANNOTATIONGETLINES, line, 0);
}


inline bool isLineVisible(int view, int line)
{
	line = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0);

	return ((line >= getFirstVisibleLine(view)) && (line <= getLastVisibleLine(view)));
}


inline bool isLineWrapped(int view, int line)
{
	return (CallScintilla(view, SCI_WRAPCOUNT, line, 0) > 1);
}


inline bool isLineAnnotated(int view, int line)
{
	return (getLineAnnotation(view, line) > 0);
}


inline bool isLineMarked(int view, int line, int markMask)
{
	return ((CallScintilla(view, SCI_MARKERGET, line, 0) & markMask) != 0);
}


inline bool isLineEmpty(int view, int line)
{
	return ((CallScintilla(view, SCI_GETLINEENDPOSITION, line, 0) -
			CallScintilla(view, SCI_POSITIONFROMLINE, line, 0)) == 0);
}


inline bool isSelection(int view)
{
	return (CallScintilla(view, SCI_GETSELECTIONEND, 0, 0) - CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0) != 0);
}


inline int isSelectionVertical(int view)
{
	return CallScintilla(view, SCI_SELECTIONISRECTANGLE, 0, 0);
}


inline std::pair<int, int> getSelection(int view)
{
	return std::make_pair(CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0),
			CallScintilla(view, SCI_GETSELECTIONEND, 0, 0));
}


inline void clearSelection(int view)
{
	const int currentPos = CallScintilla(view, SCI_GETCURRENTPOS, 0, 0);
	CallScintilla(view, SCI_SETSEL, currentPos, currentPos);
}


void activateBufferID(LRESULT buffId);
std::pair<int, int> getSelectionLines(int view);

int showArrowSymbol(int view, int line, bool down);

void blinkLine(int view, int line);
void blinkRange(int view, int startPos, int endPos);

void centerAt(int view, int line);

void markTextAsChanged(int view, int start, int length);
void clearChangedIndicator(int view, int start, int length);

void setNormalView(int view);
void setCompareView(int view, int blankColor);

void setStyles(UserSettings& settings);


inline bool isAlignmentFirstLineInserted(int view)
{
	return isLineMarked(view, 0, MARKER_MASK_BLANK);
}


void insertAlignmentFirstLine(int view);
void removeAlignmentFirstLine(int view);

void clearWindow(int view);
void clearMarks(int view, int line);
int getPrevUnmarkedLine(int view, int startLine, int markMask);
int getNextUnmarkedLine(int view, int startLine, int markMask);

void hideUnmarked(int view, int markMask);

bool isAdjacentAnnotation(int view, int line, bool down);
bool isVisibleAdjacentAnnotation(int view, int line, bool down);

std::vector<char> getText(int view, int startPos, int endPos);
void toLowerCase(std::vector<char>& text);

void addBlankSection(int view, int line, int length, int selectionMarkPosition = 0);
