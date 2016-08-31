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

#include "Compare.h"
#include "Engine.h"
#include "NppHelpers.h"

#include "icon_add_16.h"
#include "icon_sub_16.h"
#include "icon_warning_16.h"
#include "icon_moved_16.h"


// don't use "INDIC_CONTAINER + 1" since it conflicts with DSpellCheck plugin
#define INDIC_HIGHLIGHT		INDIC_CONTAINER + 7


static const char strEOL[3][3] =
{
	"\r\n",
	"\r",
	"\n"
};

static const unsigned short lenEOL[3] = { 2, 1, 1 };


HWND NppToolbarHandleGetter::hNppToolbar = NULL;


HWND NppToolbarHandleGetter::get()
{
	if (hNppToolbar == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, 0);

	return hNppToolbar;
}


BOOL CALLBACK NppToolbarHandleGetter::enumWindowsCB(HWND hwnd, LPARAM lParam)
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


void ViewLocation::save(int buffId)
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


void activateBufferID(int buffId)
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


static void setChangedStyle(HWND window, const ColorSettings& settings)
{
	::SendMessage(window, SCI_INDICSETSTYLE, INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	::SendMessage(window, SCI_INDICSETFORE, INDIC_HIGHLIGHT, (LPARAM)settings.highlight);
	::SendMessage(window, SCI_INDICSETALPHA, INDIC_HIGHLIGHT, (LPARAM)settings.alpha);
}


static void setTextStyle(HWND window, const ColorSettings& settings)
{
	setChangedStyle(window, settings);
}


static void setTextStyles(const ColorSettings& settings)
{
	setTextStyle(nppData._scintillaMainHandle, settings);
	setTextStyle(nppData._scintillaSecondHandle, settings);
}


void setBlank(HWND window, int color)
{
	::SendMessage(window, SCI_MARKERDEFINE, MARKER_BLANK_LINE, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(window, SCI_MARKERSETBACK, MARKER_BLANK_LINE, (LPARAM)color);
	::SendMessage(window, SCI_MARKERSETFORE, MARKER_BLANK_LINE, (LPARAM)color);
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

	setCompareView(nppData._scintillaMainHandle);
	setCompareView(nppData._scintillaSecondHandle);

	setBlank(nppData._scintillaMainHandle,   settings.colors.blank);
	setBlank(nppData._scintillaSecondHandle, settings.colors.blank);

	defineColor(MARKER_ADDED_LINE,   settings.colors.added);
	defineColor(MARKER_CHANGED_LINE, settings.colors.changed);
	defineColor(MARKER_MOVED_LINE,   settings.colors.moved);
	defineColor(MARKER_REMOVED_LINE, settings.colors.deleted);

	DefineXpmSymbol(MARKER_ADDED_SYMBOL,   icon_add_16_xpm);
	DefineXpmSymbol(MARKER_REMOVED_SYMBOL, icon_sub_16_xpm);
	DefineXpmSymbol(MARKER_CHANGED_SYMBOL, icon_warning_16_xpm);
	DefineXpmSymbol(MARKER_MOVED_SYMBOL,   icon_moved_16_xpm);

	setTextStyles(settings.colors);
}


void markAsBlank(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, MARKER_BLANK_LINE);
}


void markAsAdded(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, MARKER_ADDED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, MARKER_ADDED_LINE);
}


void markAsChanged(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, MARKER_CHANGED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, MARKER_CHANGED_LINE);
}


void markAsRemoved(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, MARKER_REMOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, MARKER_REMOVED_LINE);
}


void markAsMoved(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, MARKER_MOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, MARKER_MOVED_LINE);
}


void markTextAsChanged(HWND window, int start, int length)
{
	if (length != 0)
	{
		int curIndic = ::SendMessage(window, SCI_GETINDICATORCURRENT, 0, 0);
		::SendMessage(window, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		::SendMessage(window, SCI_INDICATORFILLRANGE, start, length);
		::SendMessage(window, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void clearChangedIndicator(HWND window, int start, int length)
{
	if (length != 0)
	{
		int curIndic = ::SendMessage(window, SCI_GETINDICATORCURRENT, 0, 0);
		::SendMessage(window, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		::SendMessage(window, SCI_INDICATORCLEARRANGE, start, length);
		::SendMessage(window, SCI_SETINDICATORCURRENT, curIndic, 0);
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


DocLines_t getAllLines(HWND window, std::vector<int>& lineNum, bool ignoreEOLs)
{
	int docLines = ::SendMessage(window, SCI_GETLENGTH, 0, 0);

	if (docLines)
		docLines = ::SendMessage(window, SCI_GETLINECOUNT, 0, 0);

	DocLines_t lines(docLines);
	lineNum.resize(docLines);

	for (int line = 0; line < docLines; ++line)
	{
		Sci_TextRange tr;
		tr.chrg.cpMin = ::SendMessage(window, SCI_POSITIONFROMLINE, line, 0);
		if (ignoreEOLs)
			tr.chrg.cpMax = ::SendMessage(window, SCI_GETLINEENDPOSITION, line, 0);
		else
			tr.chrg.cpMax = tr.chrg.cpMin + ::SendMessage(window, SCI_LINELENGTH, line, 0);

		lineNum[line] = tr.chrg.cpMin;

		int lineLength = tr.chrg.cpMax - tr.chrg.cpMin;

		lines[line].resize(lineLength + 1, 0);
		tr.lpstrText = lines[line].data();

		if (lineLength > 0)
			::SendMessage(window, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
	}

	return lines;
}


void clearWindow(HWND window)
{
	removeBlankLines(window);

	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE,   MARKER_CHANGED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_ADDED_LINE,     MARKER_ADDED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE,   MARKER_REMOVED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_MOVED_LINE,     MARKER_MOVED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_BLANK_LINE,     MARKER_BLANK_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, MARKER_CHANGED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL,   MARKER_ADDED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, MARKER_REMOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_MOVED_SYMBOL,   MARKER_MOVED_SYMBOL);

	clearChangedIndicator(window, 0, ::SendMessage(window, SCI_GETLENGTH, 0, 0));

	// reset syntax highlighting:
	::SendMessage(window, SCI_COLOURISE, 0, -1);
	::SendMessage(window, SCN_UPDATEUI, 0, 0);

	setNormalView(window);
}


void addBlankSection(HWND window, int line, int length)
{
	if (length <= 0)
		return;

	const UINT EOLtype = ::SendMessage(window, SCI_GETEOLMODE, 0, 0);

	int posAdd = ::SendMessage(window, SCI_POSITIONFROMLINE, line, 0);

	if (line == ::SendMessage(window, SCI_GETLINECOUNT, 0, 0))
		posAdd = ::SendMessage(window, SCI_GETLENGTH, 0, 0);

	std::vector<char> buff(lenEOL[EOLtype] * length);

	for (int j = 0; j < length; ++j)
	{
		unsigned int i = j * lenEOL[EOLtype];
		for (const char* eol = strEOL[EOLtype]; *eol; ++eol)
			buff[i++] = *eol;
	}

	// SCI_INSERTTEXT needs \0 terminated string
	buff.push_back(0);

	ScopedViewUndoCollectionBlocker undoBlock(window);
	ScopedViewWriteEnabler writeEn(window);

	::SendMessage(window, SCI_INSERTTEXT, posAdd, (LPARAM)buff.data());

	for (int i = 0; i < length; ++i)
	{
		posAdd = ::SendMessage(getOtherView(window), SCI_LINELENGTH, line + i, 0) - lenEOL[EOLtype];
		::SendMessage(window, SCI_SETLINEINDENTATION, line + i, posAdd);
		markAsBlank(window, line + i);
	}
}


int deleteBlankSection(HWND window, int line)
{
	const int eolLen = lenEOL[::SendMessage(window, SCI_GETEOLMODE, 0, 0)];
	const int lastLine = ::SendMessage(window, SCI_GETLINECOUNT, 0, 0) - 1;
	const int blankMask = 1 << MARKER_BLANK_LINE;

	int deleteStartPos = ::SendMessage(window, SCI_POSITIONFROMLINE, line, 0);
	int deleteLen = 0;
	int deletedLines = 0;

	while ((line <= lastLine) && (::SendMessage(window, SCI_MARKERGET, line, 0) & blankMask))
	{
		::SendMessage(window, SCI_MARKERDELETE, line, MARKER_BLANK_LINE);

		const int lineLen		= ::SendMessage(window, SCI_LINELENGTH, line, 0);
		const int lineIndent	= ::SendMessage(window, SCI_GETLINEINDENTATION, line, 0);

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
		ScopedViewUndoCollectionBlocker undoBlock(window);
		ScopedViewWriteEnabler writeEn(window);

		::SendMessage(window, SCI_DELETERANGE, deleteStartPos, deleteLen);
	}

	return deletedLines;
}


void addBlankLines(HWND window, const BlankSections_t& blanks)
{
	if (blanks.empty())
		return;

	const int size = blanks.size();
	for (int i = 0; i < size; ++i)
		addBlankSection(window, blanks[i].startLine, blanks[i].length);
}


BlankSections_t removeBlankLines(HWND window, bool saveBlanks)
{
	BlankSections_t blanks;
	const int marker = 1 << MARKER_BLANK_LINE;

	int deletedLines = 0;
	for (int line = ::SendMessage(window, SCI_MARKERNEXT, 0, marker); line != -1;
			line = ::SendMessage(window, SCI_MARKERNEXT, line, marker))
	{
		const int len = deleteBlankSection(window, line);
		if (len > 0 && saveBlanks)
		{
			blanks.emplace_back(line + deletedLines, len);
			deletedLines += len;
		}
	}

	return blanks;
}
