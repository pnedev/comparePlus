/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017 Pavel Nedev (pg.nedev@gmail.com)
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
#include "UserSettings.h"


enum Marker_t
{
	MARKER_CHANGED_LINE = 0,
	MARKER_ADDED_LINE,
	MARKER_REMOVED_LINE,
	MARKER_MOVED_LINE,
	MARKER_CHANGED_SYMBOL,
	MARKER_ADDED_SYMBOL,
	MARKER_REMOVED_SYMBOL,
	MARKER_MOVED_SYMBOL,
	MARKER_MOVED_MULTIPLE_SYMBOL
};


const int MARKER_MASK_CHANGED			= (1 << MARKER_CHANGED_LINE)	| (1 << MARKER_CHANGED_SYMBOL);
const int MARKER_MASK_ADDED				= (1 << MARKER_ADDED_LINE)		| (1 << MARKER_ADDED_SYMBOL);
const int MARKER_MASK_REMOVED			= (1 << MARKER_REMOVED_LINE)	| (1 << MARKER_REMOVED_SYMBOL);
const int MARKER_MASK_MOVED				= (1 << MARKER_MOVED_LINE)		| (1 << MARKER_MOVED_SYMBOL);
const int MARKER_MASK_MOVED_MULTIPLE	= (1 << MARKER_MOVED_LINE)		| (1 << MARKER_MOVED_MULTIPLE_SYMBOL);

const int MARKER_MASK_LINE =	(1 << MARKER_CHANGED_LINE) |
								(1 << MARKER_ADDED_LINE) |
								(1 << MARKER_REMOVED_LINE) |
								(1 << MARKER_MOVED_LINE);

const int MARKER_MASK_SYMBOL =	(1 << MARKER_CHANGED_SYMBOL) |
								(1 << MARKER_ADDED_SYMBOL) |
								(1 << MARKER_REMOVED_SYMBOL) |
								(1 << MARKER_MOVED_SYMBOL) |
								(1 << MARKER_MOVED_MULTIPLE_SYMBOL);

const int MARKER_MASK_ALL =	MARKER_MASK_LINE | MARKER_MASK_SYMBOL;


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
 *  \warning  Don't use that helper struct if somewhere in its scope the hView document is changed!!!
 */
struct ScopedViewWriteEnabler
{
	ScopedViewWriteEnabler(HWND hView) : _hView(hView)
	{
		_isRO = ::SendMessage(_hView, SCI_GETREADONLY, 0, 0);
		if (_isRO)
			::SendMessage(_hView, SCI_SETREADONLY, false, 0);
	}

	~ScopedViewWriteEnabler()
	{
		if (_isRO)
			::SendMessage(_hView, SCI_SETREADONLY, true, 0);
	}

private:
	HWND	_hView;
	int		_isRO;
};


/**
 *  \struct
 *  \brief
 *  \warning  Don't use that helper struct if somewhere in its scope the hView document is changed!!!
 */
struct ScopedViewUndoCollectionBlocker
{
	ScopedViewUndoCollectionBlocker(HWND hView) : _hView(hView)
	{
		_isUndoOn = ::SendMessage(_hView, SCI_GETUNDOCOLLECTION, 0, 0);
		if (_isUndoOn)
		{
			::SendMessage(_hView, SCI_SETUNDOCOLLECTION, false, 0);
			::SendMessage(_hView, SCI_EMPTYUNDOBUFFER, 0, 0);
		}
	}

	~ScopedViewUndoCollectionBlocker()
	{
		if (_isUndoOn)
			::SendMessage(_hView, SCI_SETUNDOCOLLECTION, true, 0);
	}

private:
	HWND	_hView;
	int		_isUndoOn;
};


/**
 *  \struct
 *  \brief
 */
struct ViewLocation
{
	ViewLocation() {}
	ViewLocation(LRESULT buffId)
	{
		save(buffId);
	}

	void save(LRESULT buffId);
	void restore();

	inline LRESULT getBuffId()
	{
		return _buffId;
	}

private:
	LRESULT	_buffId;
	int		_visibleLineOffset;
	int		_pos;
	int		_selStart;
	int		_selEnd;
};


inline bool isSingleView()
{
	return (!::IsWindowVisible(nppData._scintillaSecondHandle) || !::IsWindowVisible(nppData._scintillaMainHandle));
}


inline bool isFileEmpty(HWND view)
{
	return (::SendMessage(view, SCI_GETLENGTH, 0, 0) == 0);
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


inline int getDocId(HWND view)
{
	return ::SendMessage(view, SCI_GETDOCPOINTER, 0, 0);
}


inline int getCurrentLine(HWND view)
{
	return ::SendMessage(view, SCI_LINEFROMPOSITION, ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0), 0);
}


inline int otherViewMatchingLine(HWND view, int line)
{
	return ::SendMessage(getOtherView(view), SCI_DOCLINEFROMVISIBLE,
					::SendMessage(view, SCI_VISIBLEFROMDOCLINE, line, 0), 0);
}


inline bool isSelection(HWND view)
{
	return (::SendMessage(view, SCI_GETSELECTIONEND, 0, 0) - ::SendMessage(view, SCI_GETSELECTIONSTART, 0, 0) != 0);
}


inline int isSelectionVertical(HWND view)
{
	return SendMessage(view, SCI_SELECTIONISRECTANGLE, 0, 0);
}


inline std::pair<int, int> getSelection(HWND view)
{
	return std::make_pair(::SendMessage(view, SCI_GETSELECTIONSTART, 0, 0),
			::SendMessage(view, SCI_GETSELECTIONEND, 0, 0));
}


inline void clearSelection(HWND view)
{
	const int currentPos = ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0);
	::SendMessage(view, SCI_SETSEL, currentPos, currentPos);
}


void activateBufferID(LRESULT buffId);
std::pair<int, int> getSelectionLines(HWND view);

void centerAt(HWND view, int line);

void markTextAsChanged(HWND view, int start, int length);
void clearChangedIndicator(HWND view, int start, int length);

void jumpToFirstChange();
void jumpToLastChange();
void jumpToNextChange(bool down, bool wrapAround);

void setNormalView(HWND view);
void setCompareView(HWND view, int blankColor);

void setStyles(UserSettings& settings);

void clearWindow(HWND view);
void clearMarks(HWND view, int line);
void clearMarks(HWND view, int startLine, int linesCount);
void clearMarksAndBlanks(HWND view, int startLine, int linesCount);
int getPrevUnmarkedLine(HWND view, int startLine, int markMask);
int getNextUnmarkedLine(HWND view, int startLine, int markMask);

std::vector<char> getText(HWND view, int startPos, int endPos);
void toLowerCase(std::vector<char>& text);

void addBlankSection(HWND view, int line, int length);
