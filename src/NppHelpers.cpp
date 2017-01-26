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

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include <stdlib.h>
#include <vector>

#include "Compare.h"
#include "NppHelpers.h"

#include "icon_add_16.h"
#include "icon_sub_16.h"
#include "icon_diff_16.h"
#include "icon_moved_16.h"


// don't use "INDIC_CONTAINER + 1" since it conflicts with DSpellCheck plugin
#define INDIC_HIGHLIGHT		INDIC_CONTAINER + 7


static const char EOLstr[3][3] =
{
	"\r\n",
	"\r",
	"\n"
};

static const short EOLlen[3] = { 2, 1, 1 };


HWND NppToolbarHandleGetter::hNppToolbar = NULL;


HWND NppToolbarHandleGetter::get()
{
	if (hNppToolbar == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, 0);

	return hNppToolbar;
}


BOOL CALLBACK NppToolbarHandleGetter::enumWindowsCB(HWND hwnd, LPARAM )
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


HWND NppTabHandleGetter::hNppTab[2] = { NULL, NULL };


HWND NppTabHandleGetter::get(int viewId)
{
	const int idx = (viewId == MAIN_VIEW) ? 0 : 1;

	if (hNppTab[idx] == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, idx);

	return hNppTab[idx];
}


BOOL CALLBACK NppTabHandleGetter::enumWindowsCB(HWND hwnd, LPARAM lParam)
{
	TCHAR winClassName[64];

	::GetClassName(hwnd, winClassName, _countof(winClassName));

	if (!_tcscmp(winClassName, WC_TABCONTROL))
	{
		RECT tabRect;
		RECT viewRect;

		::GetWindowRect(hwnd, &tabRect);
		::GetWindowRect(getView(lParam), &viewRect);

		if ((tabRect.left <= viewRect.left) && (tabRect.top <= viewRect.top) &&
			(tabRect.right >= viewRect.right) && (tabRect.bottom >= viewRect.bottom))
		{
			hNppTab[lParam] = hwnd;
			return FALSE;
		}
	}

	return TRUE;
}


void ViewLocation::save(LRESULT buffId)
{
	_buffId = buffId;

	HWND view = getView(viewIdFromBuffId(_buffId));

	_firstVisibleLine = ::SendMessage(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
	_pos = ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0);
}


void ViewLocation::restore()
{
	activateBufferID(_buffId);

	HWND view = getView(viewIdFromBuffId(_buffId));

	const int line = ::SendMessage(view, SCI_LINEFROMPOSITION, _pos, 0);

	::SendMessage(view, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);
	::SendMessage(view, SCI_SETSEL, _pos, _pos);
	::SendMessage(view, SCI_SETFIRSTVISIBLELINE, _firstVisibleLine, 0);
}


void activateBufferID(LRESULT buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	::SendMessage(nppData._nppHandle, NPPM_ACTIVATEDOC, index >> 30, index & 0x3FFFFFFF);
}


void defineColor(int type, int color)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETFORE,type, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETFORE,type, 0);
}


void defineSymbol(int type, int symbol)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINE, type, (LPARAM)symbol);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINE, type, (LPARAM)symbol);
}


static void setChangedStyle(HWND view, const ColorSettings& settings)
{
	::SendMessage(view, SCI_INDICSETSTYLE, INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	::SendMessage(view, SCI_INDICSETFORE, INDIC_HIGHLIGHT, (LPARAM)settings.highlight);
	::SendMessage(view, SCI_INDICSETALPHA, INDIC_HIGHLIGHT, (LPARAM)settings.alpha);
}


static void setTextStyle(HWND view, const ColorSettings& settings)
{
	setChangedStyle(view, settings);
}


static void setTextStyles(const ColorSettings& settings)
{
	setTextStyle(nppData._scintillaMainHandle, settings);
	setTextStyle(nppData._scintillaSecondHandle, settings);
}


void setBlank(HWND view, int color)
{
	::SendMessage(view, SCI_MARKERDEFINE, MARKER_BLANK_LINE, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(view, SCI_MARKERSETBACK, MARKER_BLANK_LINE, (LPARAM)color);
	::SendMessage(view, SCI_MARKERSETFORE, MARKER_BLANK_LINE, (LPARAM)color);
}


static void DefineXpmSymbol(int type, const char **xpm)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
}


void setNormalView(HWND view)
{
	::SendMessage(view, SCI_SETMARGINMASKN, 4, 0);
	::SendMessage(view, SCI_SETMARGINWIDTHN, 4, 0);

	::SendMessage(view, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);
}


void setCompareView(HWND view)
{
	const int marginMask =	(1 << MARKER_CHANGED_SYMBOL) |
							(1 << MARKER_ADDED_SYMBOL) |
							(1 << MARKER_REMOVED_SYMBOL) |
							(1 << MARKER_MOVED_SYMBOL);

	::SendMessage(view, SCI_SETMARGINMASKN, 4, (LPARAM)marginMask);
	::SendMessage(view, SCI_SETMARGINWIDTHN, 4, 16);

	::SendMessage(view, SCI_SETCARETLINEBACKALPHA, 96, 0);
}


void setStyles(UserSettings& settings)
{
	const int bg = ::SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0);

	settings.colors._default = bg;

	unsigned int r = bg & 0xFF;
	unsigned int g = bg >> 8 & 0xFF;
	unsigned int b = bg >> 16 & 0xFF;
	int colorShift = 0;

	if (((r + g + b) / 3) >= 128)
		colorShift = -30;
	else
		colorShift = 30;

	r = (r + colorShift) & 0xFF;
	g = (g + colorShift) & 0xFF;
	b = (b + colorShift) & 0xFF;

	settings.colors.blank = r | (g << 8) | (b << 16);

	setBlank(nppData._scintillaMainHandle,   settings.colors.blank);
	setBlank(nppData._scintillaSecondHandle, settings.colors.blank);

	defineColor(MARKER_ADDED_LINE,   settings.colors.added);
	defineColor(MARKER_CHANGED_LINE, settings.colors.changed);
	defineColor(MARKER_MOVED_LINE,   settings.colors.moved);
	defineColor(MARKER_REMOVED_LINE, settings.colors.deleted);

	DefineXpmSymbol(MARKER_ADDED_SYMBOL,   icon_add_16_xpm);
	DefineXpmSymbol(MARKER_REMOVED_SYMBOL, icon_sub_16_xpm);
	DefineXpmSymbol(MARKER_CHANGED_SYMBOL, icon_diff_16_xpm);
	DefineXpmSymbol(MARKER_MOVED_SYMBOL,   icon_moved_16_xpm);

	setTextStyles(settings.colors);
}


void markTextAsChanged(HWND view, int start, int length)
{
	if (length != 0)
	{
		int curIndic = ::SendMessage(view, SCI_GETINDICATORCURRENT, 0, 0);
		::SendMessage(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		::SendMessage(view, SCI_INDICATORFILLRANGE, start, length);
		::SendMessage(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void clearChangedIndicator(HWND view, int start, int length)
{
	if (length != 0)
	{
		int curIndic = ::SendMessage(view, SCI_GETINDICATORCURRENT, 0, 0);
		::SendMessage(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		::SendMessage(view, SCI_INDICATORCLEARRANGE, start, length);
		::SendMessage(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void jumpToFirstChange()
{
	HWND currView = getCurrentView();

	const int sci_search_mask = (1 << MARKER_MOVED_LINE)
							  | (1 << MARKER_CHANGED_LINE)
							  | (1 << MARKER_ADDED_LINE)
							  | (1 << MARKER_REMOVED_LINE)
							  | (1 << MARKER_BLANK_LINE);

	const int nextLine = ::SendMessage(currView, SCI_MARKERNEXT, 0, sci_search_mask);

	if (nextLine < 0)
		return;

	::SendMessage(currView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
	::SendMessage(currView, SCI_GOTOLINE, nextLine, 0);
}


void jumpToLastChange()
{
	HWND currView = getCurrentView();

	const int sci_search_mask = (1 << MARKER_MOVED_LINE)
							  | (1 << MARKER_CHANGED_LINE)
							  | (1 << MARKER_ADDED_LINE)
							  | (1 << MARKER_REMOVED_LINE)
							  | (1 << MARKER_BLANK_LINE);

	const int lineMax = ::SendMessage(currView, SCI_GETLINECOUNT, 0, 0);
	const int nextLine = ::SendMessage(currView, SCI_MARKERPREVIOUS, lineMax, sci_search_mask);

	if (nextLine < 0)
		return;

	::SendMessage(currView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
	::SendMessage(currView, SCI_GOTOLINE, nextLine, 0);
}


void jumpToNextChange(bool down, bool wrapAround)
{
	HWND view = getCurrentView();

	const int sci_search_mask = (1 << MARKER_MOVED_LINE) |
								(1 << MARKER_CHANGED_LINE) |
								(1 << MARKER_ADDED_LINE) |
								(1 << MARKER_REMOVED_LINE) |
								(1 << MARKER_BLANK_LINE);

	const int sci_marker_direction = down ? SCI_MARKERNEXT : SCI_MARKERPREVIOUS;

	int currentLine = getCurrentLine(view);

	const int lineMax = ::SendMessage(view, SCI_GETLINECOUNT, 0, 0);
	const int prevLine = currentLine;
	int nextLine = currentLine;

	while (nextLine == currentLine)
	{
		if (down)
		{
			while ((::SendMessage(view, SCI_MARKERGET, currentLine, 0) & sci_search_mask) && (currentLine < lineMax))
				++currentLine;
		}
		else
		{
			while ((::SendMessage(view, SCI_MARKERGET, currentLine, 0) & sci_search_mask) && (currentLine > -1))
				--currentLine;
		}

		nextLine = ::SendMessage(view, sci_marker_direction, currentLine, sci_search_mask);

		if (nextLine < 0)
		{
			if (!wrapAround)
				return;

			currentLine = down ? 0 : lineMax;
			nextLine = ::SendMessage(view, sci_marker_direction, currentLine, sci_search_mask);

			if (nextLine < 0)
				return;
			else
				break;
		}
	}

	if ((down && (nextLine < prevLine)) || (!down && (nextLine > prevLine)))
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


std::vector<char> getText(HWND view, int startPos, int endPos)
{
	const int lineLength = endPos - startPos;

	if (lineLength <= 0)
		return std::vector<char>(1, 0);

	std::vector<char> text(lineLength + 1, 0);

	Sci_TextRange tr;
	tr.chrg.cpMin = startPos;
	tr.chrg.cpMax = endPos;
	tr.lpstrText = text.data();

	::SendMessage(view, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	return text;
}


void clearWindow(HWND view)
{
	removeBlankLines(view);

	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_ADDED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_BLANK_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_MOVED_SYMBOL, 0);

	clearChangedIndicator(view, 0, ::SendMessage(view, SCI_GETLENGTH, 0, 0));

	// reset syntax highlighting:
	::SendMessage(view, SCI_COLOURISE, 0, -1);
	::SendMessage(view, SCN_UPDATEUI, 0, 0);

	setNormalView(view);
}


void clearMarks(HWND view, int startLine, int linesCount)
{
	const int startPos = ::SendMessage(view, SCI_POSITIONFROMLINE, startLine, 0);
	const int len = ::SendMessage(view, SCI_GETLINEENDPOSITION, startLine + linesCount - 1, 0) - startPos;

	clearChangedIndicator(view, startPos, len);

	for (int i = startLine + linesCount - 1; i >= startLine; --i)
		::SendMessage(view, SCI_MARKERDELETE, i, -1);
}


int clearMarksAndBlanks(HWND view, int startLine, int linesCount)
{
	int deletedLines = 0;

	const int eolLen = EOLlen[::SendMessage(view, SCI_GETEOLMODE, 0, 0)];
	const int blankMask = 1 << MARKER_BLANK_LINE;

	const int startPos = ::SendMessage(view, SCI_POSITIONFROMLINE, startLine, 0);
	const int len = ::SendMessage(view, SCI_GETLINEENDPOSITION, startLine + linesCount - 1, 0) - startPos;

	clearChangedIndicator(view, startPos, len);

	for (int i = startLine + linesCount - 1; i >= startLine; --i)
	{
		int deletePos = 0;
		int deleteLen = 0;

		if (::SendMessage(view, SCI_MARKERGET, i, 0) & blankMask)
		{
			deletePos = ::SendMessage(view, SCI_POSITIONFROMLINE, i, 0) - eolLen;
			deleteLen = ::SendMessage(view, SCI_GETLINEENDPOSITION, i, 0) - deletePos;

			const int lineIndent = ::SendMessage(view, SCI_GETLINEINDENTATION, i, 0);

			// Don't delete a line that is not blank
			if (deleteLen != lineIndent + eolLen)
				deleteLen = 0;
		}

		::SendMessage(view, SCI_MARKERDELETE, i, -1);

		if (deleteLen > 0)
		{
			ScopedViewUndoCollectionBlocker undoBlock(view);
			ScopedViewWriteEnabler writeEn(view);

			::SendMessage(view, SCI_DELETERANGE, deletePos, deleteLen);

			++deletedLines;
		}
	}

	return deletedLines;
}


int getPrevUnmarkedLine(HWND view, int startLine)
{
	const int searchMask = (1 << MARKER_MOVED_LINE) |
							(1 << MARKER_CHANGED_LINE) |
							(1 << MARKER_ADDED_LINE) |
							(1 << MARKER_REMOVED_LINE) |
							(1 << MARKER_BLANK_LINE);

	int prevUnmarkedLine = startLine;

	for (; (prevUnmarkedLine > 0) && (::SendMessage(view, SCI_MARKERGET, prevUnmarkedLine, 0) & searchMask);
			--prevUnmarkedLine);

	return prevUnmarkedLine;
}


int getNextUnmarkedLine(HWND view, int startLine)
{
	const int searchMask = (1 << MARKER_MOVED_LINE) |
							(1 << MARKER_CHANGED_LINE) |
							(1 << MARKER_ADDED_LINE) |
							(1 << MARKER_REMOVED_LINE) |
							(1 << MARKER_BLANK_LINE);

	const int lineMax = ::SendMessage(view, SCI_GETLINECOUNT, 0, 0) - 1;
	int nextUnmarkedLine = startLine;

	for (; (nextUnmarkedLine < lineMax) && (::SendMessage(view, SCI_MARKERGET, nextUnmarkedLine, 0) & searchMask);
			++nextUnmarkedLine);

	return nextUnmarkedLine;
}


static void adjustLineIndent(HWND view, int line)
{
	HWND otherView = getOtherView(view);

	const int otherWrap = ::SendMessage(otherView, SCI_WRAPCOUNT, line, 0);

	int indent = ::SendMessage(otherView, SCI_GETLINEENDPOSITION, line, 0) -
			::SendMessage(otherView, SCI_POSITIONFROMLINE, line, 0);

	::SendMessage(view, SCI_SETLINEINDENTATION, line, indent);

	int wrap = ::SendMessage(view, SCI_WRAPCOUNT, line, 0);

	while (otherWrap != wrap)
	{
		const int averageWrapLen = (otherWrap > wrap) ? (indent / otherWrap) : (indent / wrap);
		indent += averageWrapLen * (otherWrap - wrap);

		::SendMessage(view, SCI_SETLINEINDENTATION, line, indent);
		wrap = ::SendMessage(view, SCI_WRAPCOUNT, line, 0);
	}
}


void adjustBlanksWrap(HWND view)
{
	const int blankMarker = 1 << MARKER_BLANK_LINE;

	std::vector<HWND> views = (view != NULL) ? std::vector<HWND>{ view } :
			std::vector<HWND>{ nppData._scintillaMainHandle, nppData._scintillaSecondHandle };

	for (unsigned int i = 0; i < views.size() ; ++i)
	{
		int line = ::SendMessage(views[i], SCI_MARKERNEXT, 0, blankMarker);

		if (line < 0)
			continue;

		ScopedViewUndoCollectionBlocker undoBlock(views[i]);
		ScopedViewWriteEnabler writeEn(views[i]);

		for (; line >= 0; line = ::SendMessage(views[i], SCI_MARKERNEXT, line + 1, blankMarker))
		{
			const int lineIndent	= ::SendMessage(views[i], SCI_GETLINEINDENTATION, line, 0);
			const int lineLen		= ::SendMessage(views[i], SCI_GETLINEENDPOSITION, line, 0) -
					::SendMessage(views[i], SCI_POSITIONFROMLINE, line, 0);

			// Skip the line if it is not blank
			if (lineLen > lineIndent)
				continue;

			adjustLineIndent(views[i], line);
		}
	}
}


static int deleteBlankSection(HWND view, int line)
{
	const int eolLen = EOLlen[::SendMessage(view, SCI_GETEOLMODE, 0, 0)];
	const int lastLine = ::SendMessage(view, SCI_GETLINECOUNT, 0, 0) - 1;
	const int blankMask = 1 << MARKER_BLANK_LINE;

	int deleteStartPos = ::SendMessage(view, SCI_POSITIONFROMLINE, line, 0);
	int deleteLen = 0;
	int deletedLines = 0;

	while ((line <= lastLine) && (::SendMessage(view, SCI_MARKERGET, line, 0) & blankMask))
	{
		::SendMessage(view, SCI_MARKERDELETE, line, MARKER_BLANK_LINE);

		const int lineLen		= ::SendMessage(view, SCI_LINELENGTH, line, 0);
		const int lineIndent	= ::SendMessage(view, SCI_GETLINEINDENTATION, line, 0);

		// Don't delete a line that is not blank
		if ((line < lastLine && lineLen > lineIndent + eolLen) || (line == lastLine && lineLen > lineIndent))
			break;

		++deletedLines;
		deleteLen += lineLen;
		++line;
	}

	// Document's last line is blank but it doesn't have EOL to delete.
	// Thus, we delete the EOL from the previous line
	if (line > lastLine)
	{
		deleteStartPos -= eolLen;
		deleteLen += eolLen;
	}

	if (deleteLen > 0)
	{
		ScopedViewUndoCollectionBlocker undoBlock(view);
		ScopedViewWriteEnabler writeEn(view);

		::SendMessage(view, SCI_DELETERANGE, deleteStartPos, deleteLen);
	}

	return deletedLines;
}


void addBlankSection(HWND view, int line, int length)
{
	if (length <= 0)
		return;

	const UINT EOLtype = ::SendMessage(view, SCI_GETEOLMODE, 0, 0);
	const int lineCount = ::SendMessage(view, SCI_GETLINECOUNT, 0, 0);

	std::vector<char> buff(EOLlen[EOLtype] * length + 1);

	for (int j = 0; j < length; ++j)
	{
		int i = j * EOLlen[EOLtype];
		for (const char* eol = EOLstr[EOLtype]; *eol; ++eol)
			buff[i++] = *eol;
	}

	// SCI_INSERTTEXT needs \0 terminated string
	buff.back() = 0;

	ScopedViewUndoCollectionBlocker undoBlock(view);
	ScopedViewWriteEnabler writeEn(view);

	if (line < lineCount)
	{
		const int posAdd = ::SendMessage(view, SCI_POSITIONFROMLINE, line, 0);
		::SendMessage(view, SCI_INSERTTEXT, posAdd, (LPARAM)buff.data());
	}
	else
	{
		line = lineCount - 1;

		int marker = ::SendMessage(view, SCI_MARKERGET, line, 0);

		if (marker)
			::SendMessage(view, SCI_MARKERDELETE, line, -1);

		::SendMessage(view, SCI_APPENDTEXT, buff.size() - 1, (LPARAM)buff.data());

		for (int markerId = 0; marker; ++markerId)
		{
			if (marker & 1)
				::SendMessage(view, SCI_MARKERADD, line, markerId);
			marker >>= 1;
		}

		line = lineCount;
	}

	for (int i = 0; i < length; ++i)
	{
		adjustLineIndent(view, line + i);

		::SendMessage(view, SCI_MARKERADD, line + i, MARKER_BLANK_LINE);
	}
}


void addBlankLines(HWND view, const BlankSections_t& blanks)
{
	if (blanks.empty())
		return;

	const int size = blanks.size();
	for (int i = 0; i < size; ++i)
		addBlankSection(view, blanks[i].startLine, blanks[i].length);
}


BlankSections_t removeBlankLines(HWND view, bool saveBlanks)
{
	BlankSections_t blanks;
	const int marker = 1 << MARKER_BLANK_LINE;

	int deletedLines = 0;
	for (int line = ::SendMessage(view, SCI_MARKERNEXT, 0, marker); line >= 0;
			line = ::SendMessage(view, SCI_MARKERNEXT, line, marker))
	{
		const int len = deleteBlankSection(view, line);
		if (len > 0 && saveBlanks)
		{
			blanks.emplace_back(line + deletedLines, len);
			deletedLines += len;
		}
	}

	return blanks;
}
