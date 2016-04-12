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

#include "Compare.h"
#include "Engine.h"
#include "NPPHelpers.h"

#include "icon_add_16.h"
#include "icon_sub_16.h"
#include "icon_warning_16.h"
#include "icon_moved_16.h"


// don't use "INDIC_CONTAINER + 1" since it conflicts with DSpellCheck plugin
#define INDIC_HIGHLIGHT		INDIC_CONTAINER + 7


extern NppData nppData;


static const char strEOL[3][3] =
{
	"\r\n",
	"\r",
	"\n"
};


static const unsigned short lenEOL[3] = { 2, 1, 1 };


bool isSingleView()
{
	return (!IsWindowVisible(nppData._scintillaSecondHandle) || !IsWindowVisible(nppData._scintillaMainHandle));
}


HWND getView(int viewId)
{
	return (viewId == MAIN_VIEW) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}


int getCurrentViewId()
{
	return ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0);
}


HWND getCurrentView()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ?
			nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}


int getOtherViewId()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW;
}


HWND getOtherView()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ?
			nppData._scintillaSecondHandle : nppData._scintillaMainHandle;
}


int viewIdFromBuffId(int bufferID)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, bufferID, 0);
	return (index >> 30);
}


void activateBufferID(int bufferID)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, bufferID, 0);
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


static void setChangedStyle(HWND window, const sColorSettings& Settings)
{
	::SendMessage(window, SCI_INDICSETSTYLE, INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	::SendMessage(window, SCI_INDICSETFORE, INDIC_HIGHLIGHT, (LPARAM)Settings.highlight);
	::SendMessage(window, SCI_INDICSETALPHA, INDIC_HIGHLIGHT, (LPARAM)Settings.alpha);
}


static void setTextStyle(HWND window, const sColorSettings& Settings)
{
	setChangedStyle(window, Settings);
}


static void setTextStyles(const sColorSettings& Settings)
{
	setTextStyle(nppData._scintillaMainHandle, Settings);
	setTextStyle(nppData._scintillaSecondHandle, Settings);
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


void setStyles(sUserSettings& Settings)
{
    const int bg = ::SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0);

    Settings.ColorSettings._default = bg;

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

    Settings.ColorSettings.blank = r | (g << 8) | (b << 16);

    int MarginMask = (1 << MARKER_CHANGED_SYMBOL) |
					 (1 << MARKER_ADDED_SYMBOL) |
					 (1 << MARKER_REMOVED_SYMBOL) |
					 (1 << MARKER_MOVED_SYMBOL);

	::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINMASKN, 4, (LPARAM)MarginMask);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINMASKN, 4, (LPARAM)MarginMask);

	::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINWIDTHN, 4, 16);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINWIDTHN, 4, 16);

	setBlank(nppData._scintillaMainHandle,   Settings.ColorSettings.blank);
    setBlank(nppData._scintillaSecondHandle, Settings.ColorSettings.blank);

    defineColor(MARKER_ADDED_LINE,   Settings.ColorSettings.added);
    defineColor(MARKER_CHANGED_LINE, Settings.ColorSettings.changed);
    defineColor(MARKER_MOVED_LINE,   Settings.ColorSettings.moved);
    defineColor(MARKER_REMOVED_LINE, Settings.ColorSettings.deleted);

	DefineXpmSymbol(MARKER_ADDED_SYMBOL,   icon_add_16_xpm);
	DefineXpmSymbol(MARKER_REMOVED_SYMBOL, icon_sub_16_xpm);
	DefineXpmSymbol(MARKER_CHANGED_SYMBOL, icon_warning_16_xpm);
	DefineXpmSymbol(MARKER_MOVED_SYMBOL,   icon_moved_16_xpm);

    setTextStyles(Settings.ColorSettings);
}


void markAsBlank(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, MARKER_BLANK_LINE);
}


void markAsAdded(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_ADDED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_ADDED_LINE);
}


void markAsChanged(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_CHANGED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_CHANGED_LINE);
}


void markAsRemoved(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_REMOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_REMOVED_LINE);
}


void markAsMoved(HWND window, int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_MOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_MOVED_LINE);
}


void markTextAsChanged(HWND window, int start, int length)
{
	if(length!=0)
	{
		int curIndic = ::SendMessage(window, SCI_GETINDICATORCURRENT, 0, 0);
		::SendMessage(window, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		::SendMessage(window, SCI_INDICATORFILLRANGE, start, length);
		::SendMessage(window, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


DocLines_t getAllLines(HWND window, std::vector<int>& lineNum)
{
	int docLines = ::SendMessage(window, SCI_GETLINECOUNT, 0, 0);

	DocLines_t lines(docLines);
	lineNum.resize(docLines);

	for (int line = 0; line < docLines; ++line)
	{
		Sci_TextRange tr;
		tr.chrg.cpMin = ::SendMessage(window, SCI_POSITIONFROMLINE, line, 0);
		tr.chrg.cpMax = ::SendMessage(window, SCI_GETLINEENDPOSITION, line, 0);
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
	removeBlankLines(window, false);

	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE,   MARKER_CHANGED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_ADDED_LINE,     MARKER_ADDED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE,   MARKER_REMOVED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_MOVED_LINE,     MARKER_MOVED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_BLANK_LINE,     MARKER_BLANK_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, MARKER_CHANGED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL,   MARKER_ADDED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, MARKER_REMOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_MOVED_SYMBOL,   MARKER_MOVED_SYMBOL);

	// remove everything marked in markTextAsChanged():
	int curIndic = ::SendMessage(window, SCI_GETINDICATORCURRENT, 0, 0);
	int length = ::SendMessage(window, SCI_GETLENGTH, 0, 0);

	::SendMessage(window, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
	::SendMessage(window, SCI_INDICATORCLEARRANGE, 0, length);
	::SendMessage(window, SCI_SETINDICATORCURRENT, curIndic, 0);

	// reset syntax highlighting:
	::SendMessage(window, SCI_COLOURISE, 0, -1);
	::SendMessage(window, SCN_UPDATEUI, 0, 0);

	::SendMessage(window, SCI_SETMARGINMASKN, 4, 0);
	::SendMessage(window, SCI_SETMARGINWIDTHN, 4, 0);
}


void addBlankSection(HWND window, int line, int length)
{
	if (length <= 0)
		return;

	::SendMessage(window, SCI_SETUNDOCOLLECTION, FALSE, 0);

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

	::SendMessage(window, SCI_INSERTTEXT, posAdd, (LPARAM)buff.data());

	for (int i = 0; i < length; ++i)
		markAsBlank(window, line + i);

	::SendMessage(window, SCI_SETUNDOCOLLECTION, TRUE, 0);
}


int deleteBlankSection(HWND window, int line)
{
	const int eolLen = lenEOL[::SendMessage(window, SCI_GETEOLMODE, 0, 0)];
	const int lastLine = ::SendMessage(window, SCI_GETLINECOUNT, 0, 0) - 1;
	const int blankMask = 1 << MARKER_BLANK_LINE;

	int blankPos = ::SendMessage(window, SCI_POSITIONFROMLINE, line, 0);
	int blankLen = 0;
	int blankLines = 0;

	while ((line <= lastLine) && (::SendMessage(window, SCI_MARKERGET, line, 0) & blankMask))
	{
		::SendMessage(window, SCI_MARKERDELETE, line, MARKER_BLANK_LINE);

		const int lineLen = ::SendMessage(window, SCI_LINELENGTH, line, 0);

		// don't delete lines that actually have text in them
		if ((line < lastLine && lineLen > eolLen) || (line == lastLine && lineLen > 0))
			break;

		++blankLines;
		blankLen += lineLen;
		++line;
	}

	// if we're at the end of the document, we can't delete that line,
	// because it doesn't have any EOL characters to delete,
	// so we have to delete the EOL on the previous line
	if (line > lastLine)
	{
		blankPos -= eolLen;
		blankLen += eolLen;
	}

	if (blankLen > 0)
		::SendMessage(window, SCI_DELETERANGE, blankPos, blankLen);

	return blankLines;
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

	::SendMessage(window, SCI_SETUNDOCOLLECTION, FALSE, 0);

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

	::SendMessage(window, SCI_SETUNDOCOLLECTION, TRUE, 0);

	return blanks;
}
