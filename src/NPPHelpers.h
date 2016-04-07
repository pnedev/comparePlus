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


// Forward declarations
struct sUserSettings;


struct BlankSection
{
	BlankSection(unsigned int line, unsigned int len) : startLine(line), length(len) {}

	unsigned int startLine;
	unsigned int length;
};


using BlankSections_t = std::vector<BlankSection>;
using DocLines_t = std::vector<std::vector<char>>;


bool isSingleView();

HWND getView(int viewId);
int getCurrentViewId();
HWND getCurrentView();
int getOtherViewId();
HWND getOtherView();

int viewIdFromBuffId(int bufferID);
void activateBufferID(int bufferID);

void markTextAsChanged(HWND window, int start, int length);
void markAsMoved(HWND window, int line);
void markAsRemoved(HWND window, int line);
void markAsChanged(HWND window, int line);
void markAsAdded(HWND window, int line);
void markAsBlank(HWND window, int line);

void setStyles(sUserSettings& Settings);
void setBlank(HWND window, int color);

void defineSymbol(int type, int symbol);
void defineColor(int type, int color);
void clearWindow(HWND window);

DocLines_t getAllLines(HWND window, std::vector<int>& lineNum);

void addBlankSection(HWND window, int line, int length);
int deleteBlankSection(HWND window, int line);

void addBlankLines(HWND window, const BlankSections_t& blanks);
BlankSections_t removeBlankLines(HWND window, bool saveBlanks);
