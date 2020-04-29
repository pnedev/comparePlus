/*
 * This file is part of ComparePlus plugin for Notepad++
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

#include <cassert>

#include <windows.h>
#include <tchar.h>

#include "Notepad_plus_msgs.h"
#include "Scintilla.h"
#include "menuCmdID.h"
#include "PluginInterface.h"
#include "UserSettings.h"


#ifdef DLOG

	#include <string>
	#include <shlwapi.h>

	#define LOGD_GET_TIME \
		for (;;) { \
			dLogTime_ms = ::GetTickCount(); \
			break; \
		}

	#define LOGD(STR) \
		for (;;) { \
			const DWORD time_ms = ::GetTickCount(); \
			TCHAR file[MAX_PATH]; \
			char fileA[MAX_PATH]; \
			::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, _countof(file), (LPARAM)file); \
			::WideCharToMultiByte(CP_ACP, 0, file, -1, fileA, sizeof(fileA), NULL, NULL); \
			if (dLogTime_ms) dLog += "+ "; \
			std::string tmp_str { std::to_string(time_ms - dLogTime_ms) }; \
			dLog += tmp_str; \
			if (tmp_str.size() < 3) dLog += " ms\t\t- "; \
			else dLog += " ms\t- "; \
			tmp_str = fileA; \
			dLog += tmp_str; \
			if (tmp_str.size() < 4) dLog += " -\t\t\t\t"; \
			else if (tmp_str.size() < 8) dLog += " -\t\t\t"; \
			else if (tmp_str.size() < 12) dLog += " -\t\t"; \
			else dLog += " -\t"; \
			dLog += (STR); \
			break; \
		}

	#define LOGDIF(COND, STR) \
		if (COND) \
			LOGD(STR)

	#define LOGDB(BUFFID, STR) \
		for (;;) { \
			const DWORD time_ms = ::GetTickCount(); \
			TCHAR file[MAX_PATH]; \
			char fileA[MAX_PATH]; \
			::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, BUFFID, (LPARAM)file); \
			::WideCharToMultiByte(CP_ACP, 0, ::PathFindFileName(file), -1, fileA, sizeof(fileA), NULL, NULL); \
			if (dLogTime_ms) dLog += "+ "; \
			std::string tmp_str { std::to_string(time_ms - dLogTime_ms) }; \
			dLog += tmp_str; \
			if (tmp_str.size() < 3) dLog += " ms\t\t- "; \
			else dLog += " ms\t- "; \
			tmp_str = fileA; \
			dLog += tmp_str; \
			if (tmp_str.size() < 4) dLog += " -\t\t\t\t"; \
			else if (tmp_str.size() < 8) dLog += " -\t\t\t"; \
			else if (tmp_str.size() < 12) dLog += " -\t\t"; \
			else dLog += " -\t"; \
			dLog += (STR); \
			break; \
		}

	#define PRINT_DIFFS(INFO, DIFFS) \
		for (;;) { \
			LOGD(INFO "\n"); \
			for (const auto& d: DIFFS) { \
				LOGD("\t" + std::string((d.type == diff_type::DIFF_IN_1) ? "D1" : \
						(d.type == diff_type::DIFF_IN_2 ? "D2" : "M")) + \
						" off: " + std::to_string(d.off + 1) + " len: " + std::to_string(d.len) + "\n"); \
			} \
			break; \
		}

	extern std::string	dLog;
	extern DWORD		dLogTime_ms;

#else

	#define LOGD_GET_TIME
	#define LOGD(STR)
	#define LOGDIF(COND, STR)
	#define LOGDB(BUFFID, STR)
	#define PRINT_DIFFS(INFO, DIFFS)

#endif


enum MENU_COMMANDS
{
	CMD_SET_FIRST = 0,
	CMD_COMPARE,
	CMD_COMPARE_SEL,
	CMD_FIND_UNIQUE,
	CMD_FIND_UNIQUE_SEL,
	CMD_CLEAR_ACTIVE,
	CMD_CLEAR_ALL,
	CMD_SEPARATOR_1,
	CMD_LAST_SAVE_DIFF,
	CMD_SVN_DIFF,
	CMD_GIT_DIFF,
	CMD_SEPARATOR_2,
	CMD_CHAR_HIGHLIGHTING,
	CMD_DIFFS_BASED_LINE_CHANGES,
	CMD_SEPARATOR_3,
	CMD_IGNORE_SPACES,
	CMD_IGNORE_LINE_NUMBERS,
	CMD_IGNORE_EMPTY_LINES,
	CMD_IGNORE_CASE,
	CMD_DETECT_MOVES,
	CMD_SEPARATOR_4,
	CMD_SHOW_ONLY_DIFF,
	CMD_SHOW_ONLY_SEL,
	CMD_NAV_BAR,
	CMD_SEPARATOR_5,
	CMD_AUTO_RECOMPARE,
	CMD_SEPARATOR_6,
	CMD_PREV,
	CMD_NEXT,
	CMD_FIRST,
	CMD_LAST,
	CMD_SEPARATOR_7,
	CMD_SETTINGS,
	CMD_SEPARATOR_8,
	CMD_ABOUT,
	NB_MENU_COMMANDS
};


extern const TCHAR PLUGIN_NAME[];

extern NppData		nppData;
extern SciFnDirect	sciFunc;
extern sptr_t		sciPtr[2];

extern UserSettings	Settings;


inline LRESULT CallScintilla(int viewNum, unsigned int uMsg, uptr_t wParam, sptr_t lParam)
{
	assert(viewNum >= 0 && viewNum < 2);

	return sciFunc(sciPtr[viewNum], uMsg, wParam, lParam);
}


void ToggleNavigationBar();
