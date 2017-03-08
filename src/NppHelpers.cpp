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

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include <stdlib.h>
#include <vector>
#include <algorithm>

#include "NppHelpers.h"
#include "NppInternalDefines.h"

#include "icon_add_16.h"
#include "icon_sub_16.h"
#include "icon_diff_16.h"
#include "icon_moved_16.h"
#include "icon_moved_multiple_16.h"


// Don't use "INDIC_CONTAINER + 1" since it conflicts with DSpellCheck plugin
#define INDIC_HIGHLIGHT		INDIC_CONTAINER + 7


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

	_pos		= ::SendMessage(view, SCI_GETCURRENTPOS, 0, 0);
	_selStart	= ::SendMessage(view, SCI_GETSELECTIONSTART, 0, 0);
	_selEnd		= ::SendMessage(view, SCI_GETSELECTIONEND, 0, 0);

	const int line = ::SendMessage(view, SCI_LINEFROMPOSITION, _pos, 0);

	_visibleLineOffset = ::SendMessage(view, SCI_VISIBLEFROMDOCLINE, line, 0) -
			::SendMessage(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
}


void ViewLocation::restore()
{
	activateBufferID(_buffId);

	HWND view = getView(viewIdFromBuffId(_buffId));

	const int line = ::SendMessage(view, SCI_LINEFROMPOSITION, _pos, 0);

	::SendMessage(view, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);
	::SendMessage(view, SCI_SETSEL, _selStart, _selEnd);
	::SendMessage(view, SCI_SETFIRSTVISIBLELINE,
			::SendMessage(view, SCI_VISIBLEFROMDOCLINE, line, 0) - _visibleLineOffset, 0);
}


namespace // anonymous namespace
{

int blankStyle[2] = { 0, 0 };


void defineColor(int type, int color)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINE,	type, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETBACK,	type, (LPARAM)color);
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETFORE,	type, 0);

	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINE,		type, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETBACK,	type, (LPARAM)color);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETFORE,	type, 0);
}


void defineXpmSymbol(int type, const char **xpm)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);

	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
}


void setTextStyle(const ColorSettings& settings)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	::SendMessage(nppData._scintillaMainHandle, SCI_INDICSETFORE,	INDIC_HIGHLIGHT, (LPARAM)settings.highlight);
	::SendMessage(nppData._scintillaMainHandle, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT, (LPARAM)settings.alpha);

	::SendMessage(nppData._scintillaSecondHandle, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	::SendMessage(nppData._scintillaSecondHandle, SCI_INDICSETFORE,		INDIC_HIGHLIGHT, (LPARAM)settings.highlight);
	::SendMessage(nppData._scintillaSecondHandle, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT, (LPARAM)settings.alpha);
}


void setBlanksStyle(HWND view, int blankColor)
{
	const int blankIdx = (view == nppData._scintillaMainHandle) ? 0 : 1;

	if (blankStyle[blankIdx] == 0)
		blankStyle[blankIdx] = ::SendMessage(view, SCI_ALLOCATEEXTENDEDSTYLES, 1, 0);

	::SendMessage(view, SCI_ANNOTATIONSETSTYLEOFFSET,	blankStyle[blankIdx], 0);
	::SendMessage(view, SCI_STYLESETEOLFILLED,			blankStyle[blankIdx], 1);
	::SendMessage(view, SCI_STYLESETBACK,				blankStyle[blankIdx], blankColor);
	::SendMessage(view, SCI_ANNOTATIONSETVISIBLE, ANNOTATION_STANDARD, 0);
}

} // anonymous namespace


void activateBufferID(LRESULT buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	::SendMessage(nppData._nppHandle, NPPM_ACTIVATEDOC, index >> 30, index & 0x3FFFFFFF);
}


std::pair<int, int> getSelectionLines(HWND view)
{
	if (isSelectionVertical(view))
		return std::make_pair(-1, -1);

	const int selectionStart = ::SendMessage(view, SCI_GETSELECTIONSTART, 0, 0);
	const int selectionEnd = ::SendMessage(view, SCI_GETSELECTIONEND, 0, 0);

	if (selectionEnd - selectionStart == 0)
		return std::make_pair(-1, -1);

	int endLine = ::SendMessage(view, SCI_LINEFROMPOSITION, selectionEnd, 0);

	if (selectionEnd == ::SendMessage(view, SCI_POSITIONFROMLINE, endLine, 0))
		--endLine;

	return std::make_pair(::SendMessage(view, SCI_LINEFROMPOSITION, selectionStart, 0), endLine);
}


void centerAt(HWND view, int line)
{
	const int linesOnScreen = ::SendMessage(view, SCI_LINESONSCREEN, 0, 0);
	const int firstVisible = ::SendMessage(view, SCI_VISIBLEFROMDOCLINE, line, 0) - linesOnScreen / 2;

	::SendMessage(view, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);
	::SendMessage(view, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
	::SendMessage(view, SCI_GOTOLINE, line, 0);
}


void setNormalView(HWND view)
{
	::SendMessage(view, SCI_SETMARGINMASKN, 4, 0);
	::SendMessage(view, SCI_SETMARGINWIDTHN, 4, 0);

	::SendMessage(view, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);
}


void setCompareView(HWND view, int blankColor)
{
	::SendMessage(view, SCI_SETMARGINMASKN, 4, (LPARAM)MARKER_MASK_SYMBOL);
	::SendMessage(view, SCI_SETMARGINWIDTHN, 4, 16);

	::SendMessage(view, SCI_SETCARETLINEBACKALPHA, 96, 0);

	// For some reason the annotation blank styling is lost on Sci doc switch thus we need to reapply it
	setBlanksStyle(view, blankColor);
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

	defineColor(MARKER_CHANGED_LINE, settings.colors.changed);
	defineColor(MARKER_ADDED_LINE,   settings.colors.added);
	defineColor(MARKER_REMOVED_LINE, settings.colors.deleted);
	defineColor(MARKER_MOVED_LINE,   settings.colors.moved);

	defineXpmSymbol(MARKER_CHANGED_SYMBOL, 			icon_diff_16_xpm);
	defineXpmSymbol(MARKER_ADDED_SYMBOL,   			icon_add_16_xpm);
	defineXpmSymbol(MARKER_REMOVED_SYMBOL, 			icon_sub_16_xpm);
	defineXpmSymbol(MARKER_MOVED_SYMBOL,   			icon_moved_16_xpm);
	defineXpmSymbol(MARKER_MOVED_MULTIPLE_SYMBOL,	icon_moved_multiple_16_xpm);

	setTextStyle(settings.colors);
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
	HWND currentView = getCurrentView();
	HWND otherView = getOtherView(currentView);

	int nextLine = ::SendMessage(currentView, SCI_MARKERNEXT, 0, MARKER_MASK_LINE);
	const int otherLine = ::SendMessage(otherView, SCI_MARKERNEXT, 0, MARKER_MASK_LINE);

	if (nextLine < 0)
	{
		if (otherLine < 0)
			return;

		nextLine = otherViewMatchingLine(otherView, otherLine);
	}
	else if (otherLine >= 0)
	{
		int otherVisible = ::SendMessage(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		if (otherVisible < ::SendMessage(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
			nextLine = ::SendMessage(currentView, SCI_DOCLINEFROMVISIBLE, otherVisible, 0);
	}

	centerAt(currentView, nextLine);
}


void jumpToLastChange()
{
	HWND currentView = getCurrentView();
	HWND otherView = getOtherView(currentView);

	const int lineCount = ::SendMessage(currentView, SCI_GETLINECOUNT, 0, 0);
	int nextLine = ::SendMessage(currentView, SCI_MARKERPREVIOUS, lineCount, MARKER_MASK_LINE);

	const int otherLineCount = ::SendMessage(otherView, SCI_GETLINECOUNT, 0, 0);
	const int otherLine = ::SendMessage(otherView, SCI_MARKERPREVIOUS, otherLineCount, MARKER_MASK_LINE);

	if (nextLine < 0)
	{
		if (otherLine < 0)
			return;

		nextLine = otherViewMatchingLine(otherView, otherLine);
	}
	else if (otherLine >= 0)
	{
		int otherVisible = ::SendMessage(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		if (otherVisible > ::SendMessage(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
			nextLine = ::SendMessage(currentView, SCI_DOCLINEFROMVISIBLE, otherVisible, 0);
	}

	centerAt(currentView, nextLine);
}


void jumpToNextChange(bool down, bool wrapAround)
{
	HWND currentView = getCurrentView();
	HWND otherView = getOtherView(currentView);

	const int sci_next_marker = down ? SCI_MARKERNEXT : SCI_MARKERPREVIOUS;

	const int startingLine = getCurrentLine(currentView);

	int nextLine = startingLine;
	int otherLine;

	int lineCount = ::SendMessage(currentView, SCI_GETLINECOUNT, 0, 0);

	if (down)
		for (; (::SendMessage(currentView, SCI_MARKERGET, nextLine, 0) & MARKER_MASK_LINE) &&
				(nextLine < lineCount); ++nextLine);
	else
		for (; (::SendMessage(currentView, SCI_MARKERGET, nextLine, 0) & MARKER_MASK_LINE) &&
				(nextLine > -1); --nextLine);

	if (nextLine > -1 && nextLine < lineCount)
	{
		otherLine = ::SendMessage(otherView, sci_next_marker, otherViewMatchingLine(currentView, nextLine),
				MARKER_MASK_LINE);
		nextLine = ::SendMessage(currentView, sci_next_marker, nextLine, MARKER_MASK_LINE);
	}
	else
	{
		nextLine = -1;
		otherLine = -1;
	}

	int matchingLine = (otherLine >= 0) ? otherViewMatchingLine(otherView, otherLine) : -1;

	if (down && matchingLine == startingLine)
	{
		lineCount = ::SendMessage(otherView, SCI_GETLINECOUNT, 0, 0);

		for (; (::SendMessage(otherView, SCI_MARKERGET, otherLine, 0) & MARKER_MASK_LINE) &&
				(otherLine < lineCount); ++otherLine);

		if (otherLine < lineCount)
			otherLine = ::SendMessage(otherView, sci_next_marker, otherLine, MARKER_MASK_LINE);
		else
			otherLine = -1;

		if (otherLine >= 0)
			matchingLine = otherViewMatchingLine(otherView, otherLine);
		else
			matchingLine = -1;
	}

	if (nextLine < 0)
	{
		nextLine = matchingLine;

		if (nextLine < 0)
		{
			if (wrapAround)
			{
				if (down)
					jumpToFirstChange();
				else
					jumpToLastChange();

				FLASHWINFO flashInfo;
				flashInfo.cbSize = sizeof(flashInfo);
				flashInfo.hwnd = nppData._nppHandle;
				flashInfo.uCount = 2;
				flashInfo.dwTimeout = 100;
				flashInfo.dwFlags = FLASHW_ALL;
				::FlashWindowEx(&flashInfo);
			}

			return;
		}
	}
	else if (matchingLine >= 0)
	{
		const int otherVisible = ::SendMessage(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		if (down)
		{
			if (otherVisible < ::SendMessage(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
				nextLine = matchingLine;
		}
		else
		{
			if (otherVisible > ::SendMessage(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
				nextLine = matchingLine;
		}
	}

	centerAt(currentView, nextLine);
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


void toLowerCase(std::vector<char>& text)
{
	const int len = static_cast<int>(text.size());

	if (len == 0)
		return;

	std::vector<wchar_t> wText(len);

	::MultiByteToWideChar(CP_UTF8, 0, text.data(), -1, wText.data(), len * sizeof(wchar_t));

	wText.push_back(L'\0');
	::CharLowerW((LPWSTR)wText.data());
	wText.pop_back();

	::WideCharToMultiByte(CP_UTF8, 0, wText.data(), -1, text.data(), len * sizeof(char), NULL, NULL);
}


void clearWindow(HWND view)
{
	::SendMessage(view, SCI_ANNOTATIONCLEARALL, 0, 0);

	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_ADDED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_MOVED_SYMBOL, 0);
	::SendMessage(view, SCI_MARKERDELETEALL, MARKER_MOVED_MULTIPLE_SYMBOL, 0);

	clearChangedIndicator(view, 0, ::SendMessage(view, SCI_GETLENGTH, 0, 0));

	::SendMessage(view, SCI_COLOURISE, 0, -1);

	setNormalView(view);
}


void clearMarks(HWND view, int line)
{
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_CHANGED_LINE);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_ADDED_LINE);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_REMOVED_LINE);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_MOVED_LINE);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_CHANGED_SYMBOL);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_ADDED_SYMBOL);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_REMOVED_SYMBOL);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_MOVED_SYMBOL);
	::SendMessage(view, SCI_MARKERDELETE, line, MARKER_MOVED_MULTIPLE_SYMBOL);
}


void clearMarks(HWND view, int startLine, int linesCount)
{
	const int startPos = ::SendMessage(view, SCI_POSITIONFROMLINE, startLine, 0);
	const int len = ::SendMessage(view, SCI_GETLINEENDPOSITION, startLine + linesCount - 1, 0) - startPos;

	clearChangedIndicator(view, startPos, len);

	for (int line = ::SendMessage(view, SCI_MARKERPREVIOUS, startLine + linesCount - 1, MARKER_MASK_LINE);
			line >= startLine; line = ::SendMessage(view, SCI_MARKERPREVIOUS, line - 1, MARKER_MASK_LINE))
		clearMarks(view, line);
}


void clearMarksAndBlanks(HWND view, int startLine, int linesCount)
{
	clearMarks(view, startLine, linesCount);

	for (int line = startLine; line < linesCount; ++line)
	{
		if (::SendMessage(view, SCI_ANNOTATIONGETLINES, line, 0))
			::SendMessage(view, SCI_ANNOTATIONSETTEXT, line, (LPARAM)NULL);
	}
}


int getPrevUnmarkedLine(HWND view, int startLine, int markMask)
{
	int prevUnmarkedLine = startLine;

	for (; (prevUnmarkedLine > 0) &&
			(::SendMessage(view, SCI_MARKERGET, prevUnmarkedLine, 0) & markMask); --prevUnmarkedLine);

	return prevUnmarkedLine;
}


int getNextUnmarkedLine(HWND view, int startLine, int markMask)
{
	const int endLine = ::SendMessage(view, SCI_GETLINECOUNT, 0, 0) - 1;
	int nextUnmarkedLine = startLine;

	for (; (nextUnmarkedLine < endLine) &&
			(::SendMessage(view, SCI_MARKERGET, nextUnmarkedLine, 0) & markMask); ++nextUnmarkedLine);

	return nextUnmarkedLine;
}


void addBlankSection(HWND view, int line, int length)
{
	if (length <= 0)
		return;

	std::vector<char> blank(length - 1, '\n');
	blank.push_back('\0');

	::SendMessage(view, SCI_ANNOTATIONSETTEXT, line - 1, (LPARAM)blank.data());
}
