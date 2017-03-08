/*
 * This file is part of Compare Plugin for Notepad++
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <windows.h>

#include "Notepad_plus_msgs.h"
#include "Scintilla.h"
#include "menuCmdID.h"
#include "PluginInterface.h"


enum MENU_COMMANDS
{
	CMD_SET_FIRST = 0,
	CMD_COMPARE,
	CMD_COMPARE_LINES,
	CMD_CLEAR_ACTIVE,
	CMD_CLEAR_ALL,
	CMD_SEPARATOR_1,
	CMD_LAST_SAVE_DIFF,
	CMD_SVN_DIFF,
	CMD_GIT_DIFF,
	CMD_SEPARATOR_2,
	CMD_IGNORE_SPACES,
	CMD_IGNORE_CASE,
	CMD_DETECT_MOVES,
	CMD_NAV_BAR,
	CMD_SEPARATOR_3,
	CMD_PREV,
	CMD_NEXT,
	CMD_FIRST,
	CMD_LAST,
	CMD_SEPARATOR_4,
	CMD_SETTINGS,
	CMD_SEPARATOR_5,
	CMD_ABOUT,
	NB_MENU_COMMANDS
};


extern NppData nppData;


void ViewNavigationBar();
