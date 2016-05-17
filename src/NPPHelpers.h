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

#pragma once

#include <vector>
#include "Compare.h"


enum MARKER_ID
{
	MARKER_BLANK_LINE = 0,
	MARKER_MOVED_LINE,
	MARKER_CHANGED_LINE,
	MARKER_ADDED_LINE,
	MARKER_REMOVED_LINE,
	MARKER_CHANGED_SYMBOL,
	MARKER_ADDED_SYMBOL,
	MARKER_REMOVED_SYMBOL,
	MARKER_MOVED_SYMBOL
};


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
	void saveCurrent();
	void restore();

private:
	HWND	_view;
	int		_firstVisibleLine;
	int		_line;
};


struct BlankSection
{
	BlankSection(unsigned int line, unsigned int len) : startLine(line), length(len) {}

	unsigned int startLine;
	unsigned int length;
};


using BlankSections_t = std::vector<BlankSection>;
using DocLines_t = std::vector<std::vector<char>>;


inline bool isSingleView()
{
	return (!::IsWindowVisible(nppData._scintillaSecondHandle) || !::IsWindowVisible(nppData._scintillaMainHandle));
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


inline int viewIdFromBuffId(int buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	return (index >> 30);
}


inline int posFromBuffId(int buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	return (index & 0x3FFFFFFF);
}


inline int getCurrentBuffId()
{
	return ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
}


inline int getDocId(HWND view)
{
	return ::SendMessage(view, SCI_GETDOCPOINTER, 0, 0);
}


void activateBufferID(int buffId);

void markAsBlank(HWND window, int line);
void markAsAdded(HWND window, int line);
void markAsChanged(HWND window, int line);
void markAsRemoved(HWND window, int line);
void markAsMoved(HWND window, int line);
void markTextAsChanged(HWND window, int start, int length);

void jumpToFirstChange();
void jumpToLastChange();
void jumpToNextChange(bool down);

void setNormalView(HWND window);
void setCompareView(HWND window);

void setStyles(UserSettings& settings);

void setBlank(HWND window, int color);

void defineSymbol(int type, int symbol);
void defineColor(int type, int color);
void clearWindow(HWND window);

DocLines_t getAllLines(HWND window, std::vector<int>& lineNum);

void addBlankSection(HWND window, int line, int length);

void addBlankLines(HWND window, const BlankSections_t& blanks);
BlankSections_t removeBlankLines(HWND window, bool saveBlanks = false);
