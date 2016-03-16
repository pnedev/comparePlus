/*
This file is part of Compare plugin for Notepad++
Copyright (C)2011 Jean-Sébastien Leroy (jean.sebastien.leroy@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Compare.h"
#include "NPPHelpers.h"
#include "icon_add_16.h"
#include "icon_sub_16.h"
#include "icon_warning_16.h"
#include "icon_moved_16.h"

extern NppData nppData;
extern UINT	EOLtype;

//int MarkerStart   = 0;
//int removed       = MarkerStart + 7; // 24;
//int removedSymbol = MarkerStart + 6; // 23;
//int added         = MarkerStart + 5; // 22;
//int addedSymbol   = MarkerStart + 4; // 21;
//int changed       = MarkerStart + 3; // 20;
//int changedSymbol = MarkerStart + 2; // 19;
//int moved         = MarkerStart + 1; // 18;
//int blank         = MarkerStart;     // 17;

HWND getCurrentWindow()
{
	int win = 3;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&win);
	HWND window = (win == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	return window;
}

HWND getOtherWindow()
{
	int win = 3;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&win);
	HWND window = (win == 1) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	return window;
}


void defineColor(int type, int color)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETFORE,type, (LPARAM)0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETFORE,type, (LPARAM)0);
}
void defineSymbol(int type, int symbol)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINE, type, (LPARAM)symbol);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINE, type, (LPARAM)symbol);
}


void setChangedStyle(HWND window, sColorSettings Settings)
{
	::SendMessage(window, SCI_INDICSETSTYLE, INDIC_HIGHLIGHT, (LPARAM)INDIC_ROUNDBOX);
	::SendMessage(window, SCI_INDICSETFORE, INDIC_HIGHLIGHT, (LPARAM)Settings.highlight);
	::SendMessage(window, SCI_INDICSETALPHA, INDIC_HIGHLIGHT, (LPARAM)Settings.alpha);
}


void setTextStyle(HWND window, sColorSettings Settings)
{
	setChangedStyle(window, Settings);
}

void setTextStyles(sColorSettings Settings)
{
	setTextStyle(nppData._scintillaMainHandle, Settings);
	setTextStyle(nppData._scintillaSecondHandle, Settings);
}

void setCursor(int type)
{
	::SendMessage(nppData._scintillaMainHandle, SCI_SETCURSOR, type, (LPARAM)type);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETCURSOR, type, (LPARAM)type);
}

void wait()
{
	setCursor(SC_CURSORWAIT);
}

void ready()
{
	setCursor(SC_CURSORNORMAL);
}

void setBlank(HWND window, int color)
{
	SendMessage(window, SCI_MARKERDEFINE, MARKER_BLANK_LINE, (LPARAM)SC_MARK_BACKGROUND);
	SendMessage(window, SCI_MARKERSETBACK, MARKER_BLANK_LINE, (LPARAM)color);
	SendMessage(window, SCI_MARKERSETFORE, MARKER_BLANK_LINE, (LPARAM)color);
}

void DefineXpmSymbol(int type, const char **xpm)
{
	SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
	SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINEPIXMAP, type, (LPARAM)xpm);
}

void setStyles(sUserSettings* Settings)
{
    int bg = SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0);
    Settings->ColorSettings._default = bg;
    unsigned int r = bg & 0xFF;
    unsigned int g = bg >> 8 & 0xFF;
    unsigned int b = bg >> 16 & 0xFF;
    int colorShift = 0;
    if (((r + g + b) / 3) >= 128)
    {
        colorShift = -30;
    }
    else
    {
        colorShift = 30;
    }
    r = (r + colorShift) & 0xFF;
    g = (g + colorShift) & 0xFF;
    b = (b + colorShift) & 0xFF;
    Settings->ColorSettings.blank = r | (g << 8) | (b << 16);

    int MarginMask = (1 << MARKER_CHANGED_SYMBOL) |
					 (1 << MARKER_ADDED_SYMBOL) |
					 (1 << MARKER_REMOVED_SYMBOL) |
					 (1 << MARKER_MOVED_SYMBOL);

	::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)MarginMask);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)MarginMask);

	::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)16);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)16);

	setBlank(nppData._scintillaMainHandle,   Settings->ColorSettings.blank);
    setBlank(nppData._scintillaSecondHandle, Settings->ColorSettings.blank);

    defineColor(MARKER_ADDED_LINE,   Settings->ColorSettings.added);
    defineColor(MARKER_CHANGED_LINE, Settings->ColorSettings.changed);
    defineColor(MARKER_MOVED_LINE,   Settings->ColorSettings.moved);
    defineColor(MARKER_REMOVED_LINE, Settings->ColorSettings.deleted);

	DefineXpmSymbol(MARKER_ADDED_SYMBOL,   icon_add_16_xpm);
	DefineXpmSymbol(MARKER_REMOVED_SYMBOL, icon_sub_16_xpm);
	DefineXpmSymbol(MARKER_CHANGED_SYMBOL, icon_warning_16_xpm);
	DefineXpmSymbol(MARKER_MOVED_SYMBOL,   icon_moved_16_xpm);

    setTextStyles(Settings->ColorSettings);
}

void markAsBlank(HWND window,int line)
{
	::SendMessage(window, SCI_MARKERADD, line, MARKER_BLANK_LINE);
}

void markAsAdded(HWND window,int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_ADDED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_ADDED_LINE);
}
void markAsChanged(HWND window,int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_CHANGED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_CHANGED_LINE);
}
void markAsRemoved(HWND window,int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_REMOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_REMOVED_LINE);
}

void markAsMoved(HWND window,int line)
{
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_MOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)MARKER_MOVED_LINE);
}

void markTextAsChanged(HWND window,int start,int length)
{
	if(length!=0)
	{
		int curIndic = ::SendMessage(window, SCI_GETINDICATORCURRENT, 0, 0);
		::SendMessage(window, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		::SendMessage(window, SCI_INDICATORFILLRANGE, start, length);
		::SendMessage(window, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}

char **getAllLines(HWND window, int *length, int **lineNum)
{
	int docLines = SendMessage(window, SCI_GETLINECOUNT, 0, (LPARAM)0);
	char **lines = new char*[docLines];
	*lineNum = new int[docLines];
	int textCount = 0;

	for (int line = 0; line < docLines; line++)
	{
		int lineLength = SendMessage(window, SCI_LINELENGTH,line,  (LPARAM)0);
		(*lineNum)[line] = textCount;
		textCount += lineLength;

		int i = 0;
		lines[line] = new char[lineLength+1];

		if (lineLength > 0)
		{
			::SendMessage(window, SCI_GETLINE, line, (LPARAM)lines[line]);
			for(i = 0; i < lineLength && lines[line][i] != '\n' && lines[line][i] != '\r'; i++);
		}

		lines[line][i] = 0;
	}

	*length = docLines;

	return lines;
}

int deleteLine(HWND window,int line)
{
	int posAdd = ::SendMessage(window, SCI_POSITIONFROMLINE, line, 0);
	::SendMessage(window, SCI_SETTARGETSTART, posAdd, 0);
	int docLength = ::SendMessage(window, SCI_GETLINECOUNT, 0, 0)-1;
	int length = 0;//::SendMessage(window, SCI_LINELENGTH, line, 0);
	UINT EOLtype = ::SendMessage(window,SCI_GETEOLMODE,0,0);

	//::SendMessage(window,SCI_TARGETFROMSELECTION,0,0);
	int start = line;
	int lines = 0;
	int marker = SendMessage(window, SCI_MARKERGET, line, 0);
	//int blankMask=pow(2.0,blank);
	int blankMask = 1 << MARKER_BLANK_LINE;

	while((marker & blankMask) != 0)
	{
		unsigned int lineLength = ::SendMessage(window, SCI_LINELENGTH, line, 0);

		//don't delete lines that actually have text in them
		if(line < docLength && lineLength > lenEOL[EOLtype])
		{
			//lineLength-=lenEOL[EOLtype];
			break;
		}
		else if(line == docLength && lineLength > 0)
		{
			break;
		}
		lines++;
		length += lineLength;
		line++;
		marker = SendMessage(window, SCI_MARKERGET, line, 0);
	}


	//select the end of the lines, and unmark them so they aren't called for delete again
	::SendMessage(window, SCI_SETTARGETEND, posAdd+length, 0);
	for(int i=start;i<line;i++){
		::SendMessage(window, SCI_MARKERDELETE, i, MARKER_BLANK_LINE);
	}

	//if we're at the end of the document, we can't delete that line, because it doesn't have any EOL characters to delete
	//, so we have to delete the EOL on the previous line
	if(line>docLength){
		::SendMessage(window, SCI_SETTARGETSTART, posAdd-(lenEOL[EOLtype]), 0);
		length+=lenEOL[EOLtype];
	}
	if(length>0){
		::SendMessage(window, SCI_MARKERDELETE, line, MARKER_BLANK_LINE);
		::SendMessage(window, SCI_REPLACETARGET, 0, (LPARAM)"");
		return lines;
	}else{
		::SendMessage(window, SCI_MARKERDELETE, line, MARKER_BLANK_LINE);
		return 0;
	}

	//::SendMessage(window,SCI_TARGETFROMSELECTION,0,0);
	//::SendMessage(window, SCI_REPLACETARGET, 0, (LPARAM)"");
}

blankLineList *removeEmptyLines(HWND window,bool saveList)
{
	::SendMessage(window, SCI_SETUNDOCOLLECTION, FALSE, 0);
	blankLineList *list=NULL;
	//int curPosBeg = ::SendMessage(window, SCI_GETSELECTIONSTART, 0, 0);
	//int curPosEnd = ::SendMessage(window, SCI_GETSELECTIONEND, 0, 0);
	//double marker=pow(2.0,blank);
	//int line=SendMessage(window, SCI_MARKERNEXT, 0, (LPARAM)marker);
	int marker = 1 << MARKER_BLANK_LINE;
	int line=SendMessage(window, SCI_MARKERNEXT, 0, marker);
	while(line!=-1){
		int lines=deleteLine(window,line);
		if(lines>0&&saveList){
			//add to list
			blankLineList *newLine=new blankLineList;
			newLine->next=list;
			newLine->line=line;
			newLine->length=lines;
			list=newLine;
		}
		//line=SendMessage(window, SCI_MARKERNEXT, 0, marker);
		line=SendMessage(window, SCI_MARKERNEXT, 0, (LPARAM)marker);
	}

	//::SendMessage(window, SCI_SETSEL, curPosBeg, curPosEnd);
	::SendMessage(window, SCI_SETUNDOCOLLECTION, TRUE, 0);
	return list;
}

void clearUndoBuffer(HWND window){
	int modified=::SendMessage(window, SCI_GETMODIFY, 0, (LPARAM)0);
		::SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, (LPARAM)0);
		if(modified){
			::SendMessage(window, SCI_BEGINUNDOACTION, 0, (LPARAM)0);
			char fake[2];
			fake[1]=0;
			fake[0]=(char)::SendMessage(window, SCI_GETCHARAT, 0, (LPARAM)0);
			::SendMessage(window, SCI_SETTARGETSTART, 0, 0);
			::SendMessage(window, SCI_SETTARGETEND, 1, 0);
			::SendMessage(window, SCI_REPLACETARGET, 1, (LPARAM)fake);
			::SendMessage(window, SCI_ENDUNDOACTION, 0, (LPARAM)0);
		}
}

void clearWindow(HWND window, bool clearUndo)
{
	//int pos=SendMessage(window, SCI_MARKERDELETEALL, changed, (LPARAM)changed);

	clearUndo=(removeEmptyLines(window,false)!=NULL);

	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE,   (LPARAM)MARKER_CHANGED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_ADDED_LINE,     (LPARAM)MARKER_ADDED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE,   (LPARAM)MARKER_REMOVED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_MOVED_LINE,     (LPARAM)MARKER_MOVED_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_BLANK_LINE,     (LPARAM)MARKER_BLANK_LINE);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, (LPARAM)MARKER_CHANGED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL,   (LPARAM)MARKER_ADDED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, (LPARAM)MARKER_REMOVED_SYMBOL);
	::SendMessage(window, SCI_MARKERDELETEALL, MARKER_MOVED_SYMBOL,   (LPARAM)MARKER_MOVED_SYMBOL);

	// remove everything marked in markTextAsChanged():
	int curIndic = ::SendMessage(window, SCI_GETINDICATORCURRENT, 0, 0);
	int length = ::SendMessage(window, SCI_GETLENGTH, 0, 0);
	::SendMessage(window, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
	::SendMessage(window, SCI_INDICATORCLEARRANGE, 0, length);
	::SendMessage(window, SCI_SETINDICATORCURRENT, curIndic, 0);

	// reset syntax highlighting:
	::SendMessage(window, SCI_COLOURISE, 0, -1);

	//int topLine=SendMessage(window,SCI_GETFIRSTVISIBLELINE,0,0);
	//int linesOnScreen=SendMessage(window,SCI_LINESONSCREEN,0,0);

	//int curPosBeg = ::SendMessage(window, SCI_GETSELECTIONSTART, 0, 0);
	//int curPosEnd = ::SendMessage(window, SCI_GETSELECTIONEND, 0, 0);
	::SendMessage(window, SCN_UPDATEUI, 0, (LPARAM)0);
	//::SendMessage(window, SCI_SHOWLINES, 0, (LPARAM)1);

	if(clearUndo)
	{
		clearUndoBuffer(window);
	}

	/*if(returnToPos){
		SendMessage(window,SCI_GOTOLINE,topLine,0);
		SendMessage(window,SCI_GOTOLINE,topLine+linesOnScreen-1,0);

		::SendMessage(window, SCI_SETSEL, curPosBeg, curPosEnd);
	}*/

}

void addBlankLines(HWND window,blankLineList *list){
	::SendMessage(window, SCI_SETUNDOCOLLECTION, FALSE, 0);
	while(list!=NULL){
		addEmptyLines(window,list->line,list->length);
		list=list->next;
	}
	::SendMessage(window, SCI_SETUNDOCOLLECTION, TRUE, 0);
}

char *getAllText(HWND window,int *length){
	int docLength=SendMessage(window, SCI_GETLENGTH, 0, (LPARAM)0);
	char *text = new char[docLength+1];
	SendMessage(window, SCI_GETTEXT, docLength, (LPARAM)text);
	text[docLength]=0;
	*length=docLength;
	return text;

}

void addEmptyLines(HWND hSci, int offset, int length){
	if(length<=0){return;}
	::SendMessage(hSci, SCI_SETUNDOCOLLECTION, FALSE, 0);
	int posAdd=0;
	UINT EOLtype = ::SendMessage(hSci,SCI_GETEOLMODE,0,0);


	if(offset!=0){
		int docLines=SendMessage(hSci, SCI_GETLINECOUNT, 0, (LPARAM)0);
		posAdd= ::SendMessage(hSci, SCI_POSITIONFROMLINE, offset-1, 0);


		posAdd+=::SendMessage(hSci, SCI_LINELENGTH,offset-1,  (LPARAM)0)-lenEOL[EOLtype];
		if(offset==docLines){
			posAdd=SendMessage(hSci, SCI_GETLENGTH, 0, (LPARAM)0);
		}
		if(posAdd!=0){
			posAdd--;
		}else{
			posAdd=lenEOL[EOLtype]-1;
		}
	}

	::SendMessage(hSci, SCI_SETTARGETSTART, posAdd, 0);
	::SendMessage(hSci, SCI_SETTARGETEND, posAdd+1, 0);

	int blankLinesLength = lenEOL[EOLtype] * length + 1;
	int off = 0;
	char *buff = new char[blankLinesLength];
	int marker = 0;

	if(offset == 0)
	{
		marker = ::SendMessage(hSci, SCI_MARKERGET, 0, 0);
		::SendMessage(hSci, SCI_MARKERDELETE, 0, (LPARAM)-1);
		buff[blankLinesLength-1] = (char)SendMessage(hSci, SCI_GETCHARAT, posAdd, (LPARAM)0);
		off = 0;
	}
	else
	{
		buff[0] = (char)SendMessage(hSci, SCI_GETCHARAT, posAdd, (LPARAM)0);
		off = 1;
	}

	for(int j = 0; j < length; j++)
	{
		for(unsigned int i = 0; i < lenEOL[EOLtype]; i++)
		{
			buff[j * lenEOL[EOLtype] + i + off] = strEOL[EOLtype][i];
		}
	}

	::SendMessage(hSci, SCI_REPLACETARGET, blankLinesLength, (LPARAM)buff);

	for (int i = 0; i < length; i++)
	{
		markAsBlank(hSci, offset + i);
	}

	if(offset == 0)
	{
		SendMessage(hSci, SCI_MARKERADDSET, length, marker);
	}

#if CLEANUP
	delete[] buff;
#endif
	::SendMessage(hSci, SCI_SETUNDOCOLLECTION, TRUE, 0);
}
//
//
//
//
//void addEmptyLines(HWND hSci, int offset, int length){
//	if(length<=0){return;}
//	::SendMessage(hSci, SCI_SETUNDOCOLLECTION, FALSE, 0);
//	int curPosBeg = ::SendMessage(hSci, SCI_GETSELECTIONSTART, 0, 0);
//	int curPosEnd = ::SendMessage(hSci, SCI_GETSELECTIONEND, 0, 0);
//	int posAdd=0;
//
//
//	if(offset!=0){
//		int docLines=SendMessage(hSci, SCI_GETLINECOUNT, 0, (LPARAM)0);
//		posAdd= ::SendMessage(hSci, SCI_POSITIONFROMLINE, offset-1, 0);
//		posAdd+=::SendMessage(hSci, SCI_LINELENGTH,offset-1,  (LPARAM)0)-lenEOL[EOLtype];
//		if(offset==docLines){
//			posAdd=SendMessage(hSci, SCI_GETLENGTH, 0, (LPARAM)0);
//		}
//	}
//	//::SendMessage(hSci, SCI_SETTARGETSTART, posAdd, 0);
//	//::SendMessage(hSci, SCI_SETTARGETEND, posAdd, 0);
//	::SendMessage(hSci, SCI_SETSEL, posAdd, posAdd);
//
//	int marker;
//	if(offset==0){
//		marker=::SendMessage(hSci, SCI_MARKERGET, 0, 0);
//		::SendMessage(hSci, SCI_MARKERDELETE, 0, (LPARAM)-1);
//	}
//
//
//	for (int i = 0; i < length; i++){
//		::SendMessage(hSci, SCI_ADDTEXT, lenEOL[EOLtype], (LPARAM)strEOL[EOLtype]);
//
//		if(curPosBeg>posAdd){
//			curPosBeg+=lenEOL[EOLtype];
//		}
//		if(curPosEnd>posAdd){
//			curPosEnd+=lenEOL[EOLtype];
//		}
//
//		markAsBlank(hSci,offset+i);
//
//	}
//	if(offset==0){
//		SendMessage(hSci, SCI_MARKERADDSET, length, marker);
//
//	}
//	//	::SendMessage(hSci, SCI_SETTARGETSTART, posAdd, 0);
//	//int docLength=::SendMessage(window, SCI_GETLINECOUNT, 0, 0);
//	//int length=::SendMessage(window, SCI_LINELENGTH, line, 0);
//	//::SendMessage(hSci, SCI_SETTARGETEND, posAdd+lenEOL[EOLtype]*length+lenEOL[EOLtype], 0);
//	//::SendMessage(hSci, SCI_LINESSPLIT, 1, 0);
//	//::SendMessage(hSci, SCI_STARTSTYLING, posAdd, 255);
//	//::SendMessage(hSci, SCI_SETSTYLING,lenEOL[EOLtype]*length+lenEOL[EOLtype] , blank);
//
//	::SendMessage(hSci, SCI_SETSEL, curPosBeg, curPosEnd);
//	::SendMessage(hSci, SCI_SETUNDOCOLLECTION, TRUE, 0);
//}
