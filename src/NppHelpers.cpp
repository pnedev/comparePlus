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

	const int view = viewIdFromBuffId(_buffId);

	_pos		= CallScintilla(view, SCI_GETCURRENTPOS, 0, 0);
	_selStart	= CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0);
	_selEnd		= CallScintilla(view, SCI_GETSELECTIONEND, 0, 0);

	const int line = CallScintilla(view, SCI_LINEFROMPOSITION, _pos, 0);

	_visibleLineOffset = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) -
			CallScintilla(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
}


void ViewLocation::restore()
{
	activateBufferID(_buffId);

	const int view = viewIdFromBuffId(_buffId);

	const int caretLine = CallScintilla(view, SCI_LINEFROMPOSITION, _pos, 0);
	const int firstVisibleLine = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, caretLine, 0) - _visibleLineOffset;

	CallScintilla(view, SCI_ENSUREVISIBLEENFORCEPOLICY, caretLine, 0);
	CallScintilla(view, SCI_SETSEL, _selStart, _selEnd);
	CallScintilla(view, SCI_SETFIRSTVISIBLELINE, firstVisibleLine, 0);

	LOGDB(_buffId, "Restore view location, caret doc line: " + std::to_string(caretLine) + ", visible doc line: " +
			std::to_string(CallScintilla(view, SCI_DOCLINEFROMVISIBLE, firstVisibleLine, 0)) + "\n");
}


namespace // anonymous namespace
{

int blankStyle[2] = { 0, 0 };


void defineColor(int type, int color)
{
	CallScintilla(MAIN_VIEW, SCI_MARKERDEFINE,	type, (LPARAM)SC_MARK_BACKGROUND);
	CallScintilla(MAIN_VIEW, SCI_MARKERSETBACK,	type, (LPARAM)color);
	CallScintilla(MAIN_VIEW, SCI_MARKERSETFORE,	type, 0);

	CallScintilla(SUB_VIEW, SCI_MARKERDEFINE,	type, (LPARAM)SC_MARK_BACKGROUND);
	CallScintilla(SUB_VIEW, SCI_MARKERSETBACK,	type, (LPARAM)color);
	CallScintilla(SUB_VIEW, SCI_MARKERSETFORE,	type, 0);
}


void defineXpmSymbol(int type, const char **xpm)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
}


void setTextStyle(const ColorSettings& settings)
{
	CallScintilla(MAIN_VIEW, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	CallScintilla(MAIN_VIEW, SCI_INDICSETFORE,	INDIC_HIGHLIGHT, (LPARAM)settings.highlight);
	CallScintilla(MAIN_VIEW, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT, (LPARAM)settings.alpha);

	CallScintilla(SUB_VIEW, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	CallScintilla(SUB_VIEW, SCI_INDICSETFORE,	INDIC_HIGHLIGHT, (LPARAM)settings.highlight);
	CallScintilla(SUB_VIEW, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT, (LPARAM)settings.alpha);
}


void setBlanksStyle(int view, int blankColor)
{
	if (blankStyle[view] == 0)
		blankStyle[view] = CallScintilla(view, SCI_ALLOCATEEXTENDEDSTYLES, 1, 0);

	CallScintilla(view, SCI_ANNOTATIONSETSTYLEOFFSET,	blankStyle[view], 0);
	CallScintilla(view, SCI_STYLESETEOLFILLED,			blankStyle[view], 1);
	CallScintilla(view, SCI_STYLESETBACK,				blankStyle[view], blankColor);
	CallScintilla(view, SCI_ANNOTATIONSETVISIBLE, ANNOTATION_STANDARD, 0);
}

} // anonymous namespace


void activateBufferID(LRESULT buffId)
{
	if (buffId != getCurrentBuffId())
	{
		LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
		::SendMessage(nppData._nppHandle, NPPM_ACTIVATEDOC, index >> 30, index & 0x3FFFFFFF);
	}
}


std::pair<int, int> getSelectionLines(int view)
{
	if (isSelectionVertical(view))
		return std::make_pair(-1, -1);

	const int selectionStart = CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0);
	const int selectionEnd = CallScintilla(view, SCI_GETSELECTIONEND, 0, 0);

	if (selectionEnd - selectionStart == 0)
		return std::make_pair(-1, -1);

	int endLine = CallScintilla(view, SCI_LINEFROMPOSITION, selectionEnd, 0);

	if (selectionEnd == CallScintilla(view, SCI_POSITIONFROMLINE, endLine, 0))
		--endLine;

	return std::make_pair(CallScintilla(view, SCI_LINEFROMPOSITION, selectionStart, 0), endLine);
}


void centerAt(int view, int line)
{
	const int linesOnScreen = CallScintilla(view, SCI_LINESONSCREEN, 0, 0);
	const int firstVisible = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - linesOnScreen / 2;

	CallScintilla(view, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);
	CallScintilla(view, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
	CallScintilla(view, SCI_GOTOLINE, line, 0);
}


void setNormalView(int view)
{
	CallScintilla(view, SCI_SETMARGINMASKN, 4, 0);
	CallScintilla(view, SCI_SETMARGINWIDTHN, 4, 0);

	CallScintilla(view, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);
}


void setCompareView(int view, int blankColor)
{
	CallScintilla(view, SCI_SETMARGINMASKN, 4, (LPARAM)MARKER_MASK_SYMBOL);
	CallScintilla(view, SCI_SETMARGINWIDTHN, 4, 16);

	CallScintilla(view, SCI_SETCARETLINEBACKALPHA, 96, 0);

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


void markTextAsChanged(int view, int start, int length)
{
	if (length != 0)
	{
		int curIndic = CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0);
		CallScintilla(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		CallScintilla(view, SCI_INDICATORFILLRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void clearChangedIndicator(int view, int start, int length)
{
	if (length != 0)
	{
		int curIndic = CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0);
		CallScintilla(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		CallScintilla(view, SCI_INDICATORCLEARRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void jumpToFirstChange()
{
	const int currentView	= getCurrentViewId();
	const int otherView		= getOtherViewId(currentView);

	int nextLine = CallScintilla(currentView, SCI_MARKERNEXT, 0, MARKER_MASK_LINE);
	const int otherLine = CallScintilla(otherView, SCI_MARKERNEXT, 0, MARKER_MASK_LINE);

	if (nextLine < 0)
	{
		if (otherLine < 0)
			return;

		nextLine = otherViewMatchingLine(otherView, otherLine);
	}
	else if (otherLine >= 0)
	{
		int otherVisible = CallScintilla(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		if (otherVisible < CallScintilla(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
			nextLine = CallScintilla(currentView, SCI_DOCLINEFROMVISIBLE, otherVisible, 0);
	}

	centerAt(currentView, nextLine);
}


void jumpToLastChange()
{
	const int currentView	= getCurrentViewId();
	const int otherView		= getOtherViewId(currentView);

	const int lineCount = CallScintilla(currentView, SCI_GETLINECOUNT, 0, 0);
	int nextLine = CallScintilla(currentView, SCI_MARKERPREVIOUS, lineCount, MARKER_MASK_LINE);

	const int otherLineCount = CallScintilla(otherView, SCI_GETLINECOUNT, 0, 0);
	const int otherLine = CallScintilla(otherView, SCI_MARKERPREVIOUS, otherLineCount, MARKER_MASK_LINE);

	if (nextLine < 0)
	{
		if (otherLine < 0)
			return;

		nextLine = otherViewMatchingLine(otherView, otherLine);
	}
	else if (otherLine >= 0)
	{
		int otherVisible = CallScintilla(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		if (otherVisible > CallScintilla(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
			nextLine = CallScintilla(currentView, SCI_DOCLINEFROMVISIBLE, otherVisible, 0);
	}

	centerAt(currentView, nextLine);
}


void jumpToNextChange(bool down, bool wrapAround)
{
	const int currentView	= getCurrentViewId();
	const int otherView		= getOtherViewId(currentView);

	const int sci_next_marker = down ? SCI_MARKERNEXT : SCI_MARKERPREVIOUS;

	const int startingLine = getCurrentLine(currentView);

	int nextLine = startingLine;
	int otherLine;

	int lineCount = CallScintilla(currentView, SCI_GETLINECOUNT, 0, 0);

	if (down)
		for (; (CallScintilla(currentView, SCI_MARKERGET, nextLine, 0) & MARKER_MASK_LINE) &&
				(nextLine < lineCount); ++nextLine);
	else
		for (; (CallScintilla(currentView, SCI_MARKERGET, nextLine, 0) & MARKER_MASK_LINE) &&
				(nextLine > -1); --nextLine);

	if (nextLine > -1 && nextLine < lineCount)
	{
		otherLine = CallScintilla(otherView, sci_next_marker, otherViewMatchingLine(currentView, nextLine),
				MARKER_MASK_LINE);
		nextLine = CallScintilla(currentView, sci_next_marker, nextLine, MARKER_MASK_LINE);
	}
	else
	{
		nextLine = -1;
		otherLine = -1;
	}

	int matchingLine = (otherLine >= 0) ? otherViewMatchingLine(otherView, otherLine) : -1;

	if (down && matchingLine == startingLine)
	{
		lineCount = CallScintilla(otherView, SCI_GETLINECOUNT, 0, 0);

		for (; (CallScintilla(otherView, SCI_MARKERGET, otherLine, 0) & MARKER_MASK_LINE) &&
				(otherLine < lineCount); ++otherLine);

		if (otherLine < lineCount)
			otherLine = CallScintilla(otherView, sci_next_marker, otherLine, MARKER_MASK_LINE);
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
		const int otherVisible = CallScintilla(otherView, SCI_VISIBLEFROMDOCLINE, otherLine, 0);

		if (down)
		{
			if (otherVisible < CallScintilla(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
				nextLine = matchingLine;
		}
		else
		{
			if (otherVisible > CallScintilla(currentView, SCI_VISIBLEFROMDOCLINE, nextLine, 0))
				nextLine = matchingLine;
		}
	}

	centerAt(currentView, nextLine);
}


std::vector<char> getText(int view, int startPos, int endPos)
{
	const int lineLength = endPos - startPos;

	if (lineLength <= 0)
		return std::vector<char>(1, 0);

	std::vector<char> text(lineLength + 1, 0);

	Sci_TextRange tr;
	tr.chrg.cpMin = startPos;
	tr.chrg.cpMax = endPos;
	tr.lpstrText = text.data();

	CallScintilla(view, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

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


void clearWindow(int view)
{
	CallScintilla(view, SCI_ANNOTATIONCLEARALL, 0, 0);

	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_MULTIPLE_SYMBOL, 0);

	clearChangedIndicator(view, 0, CallScintilla(view, SCI_GETLENGTH, 0, 0));

	CallScintilla(view, SCI_COLOURISE, 0, -1);

	setNormalView(view);
}


void clearMarks(int view, int line)
{
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_CHANGED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_ADDED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_REMOVED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_CHANGED_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_ADDED_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_REMOVED_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_MULTIPLE_SYMBOL);
}


void clearMarks(int view, int startLine, int linesCount)
{
	const int startPos = CallScintilla(view, SCI_POSITIONFROMLINE, startLine, 0);
	const int len = CallScintilla(view, SCI_GETLINEENDPOSITION, startLine + linesCount - 1, 0) - startPos;

	clearChangedIndicator(view, startPos, len);

	for (int line = CallScintilla(view, SCI_MARKERPREVIOUS, startLine + linesCount - 1, MARKER_MASK_LINE);
			line >= startLine; line = CallScintilla(view, SCI_MARKERPREVIOUS, line - 1, MARKER_MASK_LINE))
		clearMarks(view, line);
}


void clearMarksAndBlanks(int view, int startLine, int linesCount)
{
	clearMarks(view, startLine, linesCount);

	for (int line = startLine; line < linesCount; ++line)
	{
		if (CallScintilla(view, SCI_ANNOTATIONGETLINES, line, 0))
			CallScintilla(view, SCI_ANNOTATIONSETTEXT, line, (LPARAM)NULL);
	}
}


int getPrevUnmarkedLine(int view, int startLine, int markMask)
{
	int prevUnmarkedLine = startLine;

	for (; (prevUnmarkedLine > 0) &&
			(CallScintilla(view, SCI_MARKERGET, prevUnmarkedLine, 0) & markMask); --prevUnmarkedLine);

	return prevUnmarkedLine;
}


int getNextUnmarkedLine(int view, int startLine, int markMask)
{
	const int endLine = CallScintilla(view, SCI_GETLINECOUNT, 0, 0) - 1;
	int nextUnmarkedLine = startLine;

	for (; (nextUnmarkedLine < endLine) &&
			(CallScintilla(view, SCI_MARKERGET, nextUnmarkedLine, 0) & markMask); ++nextUnmarkedLine);

	return nextUnmarkedLine;
}


void addBlankSection(int view, int line, int length)
{
	if (length <= 0)
		return;

	std::vector<char> blank(length - 1, '\n');
	blank.push_back('\0');

	CallScintilla(view, SCI_ANNOTATIONSETTEXT, line - 1, (LPARAM)blank.data());
}
