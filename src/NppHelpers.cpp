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

#include "icon_add.h"
#include "icon_sub.h"
#include "icon_diff.h"
#include "icon_moved.h"
#include "icon_moved_multiple.h"
#include "icon_arrows.h"


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


void ViewLocation::save(int view, int centerLine)
{
	_view		= view;
	_centerLine	= centerLine;

	if (_centerLine < 0)
	{
		_pos		= CallScintilla(view, SCI_GETCURRENTPOS, 0, 0);
		_selStart	= CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0);
		_selEnd		= CallScintilla(view, SCI_GETSELECTIONEND, 0, 0);

		const int line = CallScintilla(view, SCI_LINEFROMPOSITION, _pos, 0);

		_visibleLineOffset = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) -
				CallScintilla(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
	}

	LOGD("Store " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") + " view location\n");
}


void ViewLocation::restore()
{
	if (_view != MAIN_VIEW && _view != SUB_VIEW)
		return;

	if (_centerLine < 0)
	{
		const int caretLine = CallScintilla(_view, SCI_LINEFROMPOSITION, _pos, 0);
		const int firstVisibleLine = CallScintilla(_view, SCI_VISIBLEFROMDOCLINE, caretLine, 0) - _visibleLineOffset;

		CallScintilla(_view, SCI_ENSUREVISIBLEENFORCEPOLICY, caretLine, 0);
		CallScintilla(_view, SCI_SETSEL, _selStart, _selEnd);
		CallScintilla(_view, SCI_SETFIRSTVISIBLELINE, firstVisibleLine, 0);

		LOGD("Restore " + std::string(_view == MAIN_VIEW ? "MAIN" : "SUB") +
				" view location, caret doc line: " + std::to_string(caretLine) + ", visible doc line: " +
				std::to_string(CallScintilla(_view, SCI_DOCLINEFROMVISIBLE, firstVisibleLine, 0)) + "\n");
	}
	else
	{
		if (!isLineVisible(_view, _centerLine))
			centerAt(_view, _centerLine);

		LOGD("Restore " + std::string(_view == MAIN_VIEW ? "MAIN" : "SUB") +
				" view location, center doc line: " + std::to_string(_centerLine) + "\n");
	}
}


namespace // anonymous namespace
{

const int cBlinkCount		= 3;
const int cBlinkInterval_ms	= 100;

bool compareMode[2]		= { false, false };
int blankStyle[2]		= { 0, 0 };
bool endAtLastLine[2]	= { true, true };


void defineColor(int type, int color)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINE,	type, SC_MARK_BACKGROUND);
	CallScintilla(MAIN_VIEW,	SCI_MARKERSETBACK,	type, color);
	CallScintilla(MAIN_VIEW,	SCI_MARKERSETFORE,	type, 0);

	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINE,	type, SC_MARK_BACKGROUND);
	CallScintilla(SUB_VIEW,		SCI_MARKERSETBACK,	type, color);
	CallScintilla(SUB_VIEW,		SCI_MARKERSETFORE,	type, 0);
}


void defineXpmSymbol(int type, const char** xpm)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
}


void defineRgbaSymbol(int type, const unsigned char* rgba)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINERGBAIMAGE,	type, (LPARAM)rgba);
	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINERGBAIMAGE,	type, (LPARAM)rgba);
}


void setTextStyle(const ColorSettings& settings)
{
	CallScintilla(MAIN_VIEW, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT, INDIC_ROUNDBOX);
	CallScintilla(MAIN_VIEW, SCI_INDICSETFORE,	INDIC_HIGHLIGHT, settings.highlight);
	CallScintilla(MAIN_VIEW, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT, settings.alpha);

	CallScintilla(SUB_VIEW, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT, INDIC_ROUNDBOX);
	CallScintilla(SUB_VIEW, SCI_INDICSETFORE,	INDIC_HIGHLIGHT, settings.highlight);
	CallScintilla(SUB_VIEW, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT, settings.alpha);
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


int showArrowSymbol(int view, int line, bool down)
{
	const bool isRTL = isRTLwindow(getView(view));
	const unsigned char* rgba = down ?
			(isRTL ? icon_arrow_down_rtl : icon_arrow_down) : (isRTL ? icon_arrow_up_rtl : icon_arrow_up);

	CallScintilla(view,	SCI_MARKERDEFINERGBAIMAGE,	MARKER_ARROW_SYMBOL, (LPARAM)rgba);

	return CallScintilla(view, SCI_MARKERADD, line, MARKER_ARROW_SYMBOL);
}


void blinkLine(int view, int line)
{
	const int marker = CallScintilla(view, SCI_MARKERGET, line, 0) & MARKER_MASK_ALL;
	HWND hView = getView(view);

	for (int i = cBlinkCount; ;)
	{
		if (marker)
			clearMarks(view, line);
		else
			CallScintilla(view, SCI_MARKERADDSET, line, MARKER_MASK_BLANK);

		::UpdateWindow(hView);
		::Sleep(cBlinkInterval_ms);

		if (marker)
			CallScintilla(view, SCI_MARKERADDSET, line, marker);
		else
			CallScintilla(view, SCI_MARKERDELETE, line, MARKER_BLANK);

		::UpdateWindow(hView);

		if (--i == 0)
			break;

		::Sleep(cBlinkInterval_ms);
	}
}


void blinkRange(int view, int startPos, int endPos)
{
	ViewLocation loc(view);

	for (int i = cBlinkCount; ;)
	{
		CallScintilla(view, SCI_SETSEL, startPos, endPos);
		::UpdateWindow(getView(view));
		::Sleep(cBlinkInterval_ms);

		if (--i == 0)
			break;

		CallScintilla(view, SCI_SETSEL, startPos, startPos);
		::UpdateWindow(getView(view));
		::Sleep(cBlinkInterval_ms);
	}

	loc.restore();
}


void centerAt(int view, int line)
{
	CallScintilla(view, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);

	const int linesOnScreen = CallScintilla(view, SCI_LINESONSCREEN, 0, 0);
	const int firstVisible = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - linesOnScreen / 2;

	CallScintilla(view, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
}


void setNormalView(int view)
{
	if (compareMode[view])
	{
		compareMode[view] = false;

		CallScintilla(view, SCI_SETENDATLASTLINE, endAtLastLine[view], 0);

		CallScintilla(view, SCI_SETMARGINMASKN, 4, 0);
		CallScintilla(view, SCI_SETMARGINWIDTHN, 4, 0);

		CallScintilla(view, SCI_SETCARETLINEBACKALPHA, SC_ALPHA_NOALPHA, 0);
	}
}


void setCompareView(int view, int blankColor)
{
	if (!compareMode[view])
	{
		compareMode[view] = true;

		endAtLastLine[view] = (CallScintilla(view, SCI_GETENDATLASTLINE, 0, 0) != 0);
		CallScintilla(view, SCI_SETENDATLASTLINE, false, 0);

		CallScintilla(view, SCI_SETMARGINMASKN, 4, (LPARAM)(MARKER_MASK_SYMBOL | MARKER_MASK_ARROW));
		CallScintilla(view, SCI_SETMARGINWIDTHN, 4, 16);

		CallScintilla(view, SCI_SETCARETLINEBACKALPHA, 96, 0);
	}

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

	defineColor(MARKER_CHANGED_LINE,	settings.colors.changed);
	defineColor(MARKER_ADDED_LINE,		settings.colors.added);
	defineColor(MARKER_REMOVED_LINE,	settings.colors.deleted);
	defineColor(MARKER_MOVED_LINE,		settings.colors.moved);
	defineColor(MARKER_BLANK,			settings.colors.blank);

	defineRgbaSymbol(MARKER_CHANGED_SYMBOL,				icon_diff_rgba);
	defineRgbaSymbol(MARKER_ADDED_SYMBOL,				icon_add_rgba);
	defineRgbaSymbol(MARKER_REMOVED_SYMBOL,				icon_sub_rgba);
	defineRgbaSymbol(MARKER_MOVED_LINE_SYMBOL,			icon_moved_line);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_BEGIN_SYMBOL,	icon_moved_block_start);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_MID_SYMBOL,		icon_moved_block_middle);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_END_SYMBOL,		icon_moved_block_end);

	defineXpmSymbol(MARKER_MOVED_MULTIPLE_SYMBOL,	icon_moved_multiple_xpm);

	setTextStyle(settings.colors);
}


void markTextAsChanged(int view, int start, int length)
{
	if (length != 0)
	{
		const int curIndic = CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0);
		CallScintilla(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		CallScintilla(view, SCI_INDICATORFILLRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void clearChangedIndicator(int view, int start, int length)
{
	if (length != 0)
	{
		const int curIndic = CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0);
		CallScintilla(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		CallScintilla(view, SCI_INDICATORCLEARRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
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
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_BEGIN_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_MID_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_END_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_MULTIPLE_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);

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
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_LINE_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_BLOCK_BEGIN_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_BLOCK_MID_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_BLOCK_END_SYMBOL);
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
		if (isLineAnnotated(view, line))
			CallScintilla(view, SCI_ANNOTATIONSETTEXT, line, (LPARAM)NULL);
	}
}


int getPrevUnmarkedLine(int view, int startLine, int markMask)
{
	int prevUnmarkedLine = startLine;

	for (; (prevUnmarkedLine > 0) && isLineMarked(view, prevUnmarkedLine, markMask); --prevUnmarkedLine);

	return prevUnmarkedLine;
}


int getNextUnmarkedLine(int view, int startLine, int markMask)
{
	const int endLine = CallScintilla(view, SCI_GETLINECOUNT, 0, 0) - 1;
	int nextUnmarkedLine = startLine;

	for (; (nextUnmarkedLine < endLine) && isLineMarked(view, nextUnmarkedLine, markMask); ++nextUnmarkedLine);

	return nextUnmarkedLine;
}


bool isAdjacentAnnotation(int view, int line, bool down)
{
	if (down)
	{
		if (isLineAnnotated(view, line))
			return true;
	}
	else
	{
		if (line && isLineAnnotated(view, line - 1))
			return true;
	}

	return false;
}


bool isVisibleAdjacentAnnotation(int view, int line, bool down)
{
	if (down)
	{
		if (!isLineAnnotated(view, line))
			return false;

		if (CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) + getWrapCount(view, line) > getLastVisibleLine(view))
			return false;
	}
	else
	{
		if (!line || !isLineAnnotated(view, line - 1))
			return false;

		if (CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - 1 < getFirstVisibleLine(view))
			return false;
	}

	return true;
}


void addBlankSection(int view, int line, int length)
{
	if (length <= 0)
		return;

	std::vector<char> blank(length - 1, '\n');
	blank.push_back('\0');

	CallScintilla(view, SCI_ANNOTATIONSETTEXT, line - 1, (LPARAM)blank.data());
}
