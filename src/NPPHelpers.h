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

#ifndef NPPHELPERS_H
#define NPPHELPERS_H

HWND getCurrentWindow();
HWND getOtherWindow();
void markTextAsChanged(HWND window,int start,int length);
void markAsMoved(HWND window,int line);
void markAsRemoved(HWND window,int line);
void markAsChanged(HWND window,int line);
void markAsAdded(HWND window,int line);
void markAsBlank(HWND window,int line);
void setStyles(sUserSettings Settings);
void setBlank(HWND window,int color);
void ready();
void wait();
void setCursor(int type);
void setTextStyles(sColorSettings Settings);
void setTextStyle(HWND window, sColorSettings Settings);
void setChangedStyle(HWND window, int color);
void defineSymbol(int type,int symbol);
void defineColor(int type,int color);
void clearWindow(HWND window,bool clearUndo);
void clearUndoBuffer(HWND window);
blankLineList *removeEmptyLines(HWND window,bool saveList);
int deleteLine(HWND window,int line);
char **getAllLines(HWND window,int *length, int **lineNum);
void addBlankLines(HWND window,blankLineList *list);
void addEmptyLines(HWND hSci, int offset, int length);

#endif
