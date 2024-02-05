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

	#define LOG_ALGO	(1 << 0)
	#define LOG_SYNC	(1 << 1)
	#define LOG_NOTIF	(1 << 2)
	#define LOG_VISIT	(1 << 3)
	#define LOG_ALL		0xFF

	#define LOGD_GET_TIME \
		if (1) { \
			dLogTime_ms = ::GetTickCount(); \
		}

	#define LOGD(LOG_FILTER, STR) \
		if (DLOG & LOG_FILTER) { \
			const DWORD time_ms = ::GetTickCount(); \
			TCHAR file[MAX_PATH]; \
			char fileA[MAX_PATH * 2]; \
			::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, _countof(file), (LPARAM)file); \
			::WideCharToMultiByte(CP_UTF8, 0, file, -1, fileA, sizeof(fileA), NULL, NULL); \
			std::string tmp_str { std::to_string(time_ms - dLogTime_ms) }; \
			dLog += tmp_str; \
			if (tmp_str.size() < 5) dLog += " ms\t\t("; \
			else dLog += " ms\t("; \
			tmp_str = fileA; \
			dLog += tmp_str; \
			if (tmp_str.size() < 7) dLog += ")\t\t\t"; \
			else if (tmp_str.size() < 11) dLog += ")\t\t"; \
			else dLog += ")\t"; \
			dLog += (STR); \
		}

	#define LOGDIF(LOG_FILTER, COND, STR) \
		if ((DLOG & LOG_FILTER) && (COND)) \
			LOGD(LOG_FILTER, STR)

	#define LOGDB(LOG_FILTER, BUFFID, STR) \
		if (DLOG & LOG_FILTER) { \
			const DWORD time_ms = ::GetTickCount(); \
			TCHAR file[MAX_PATH]; \
			char fileA[MAX_PATH * 2]; \
			::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, BUFFID, (LPARAM)file); \
			::WideCharToMultiByte(CP_UTF8, 0, ::PathFindFileName(file), -1, fileA, sizeof(fileA), NULL, NULL); \
			std::string tmp_str { std::to_string(time_ms - dLogTime_ms) }; \
			dLog += tmp_str; \
			if (tmp_str.size() < 5) dLog += " ms\t\t("; \
			else dLog += " ms\t("; \
			tmp_str = fileA; \
			dLog += tmp_str; \
			if (tmp_str.size() < 7) dLog += ")\t\t\t"; \
			else if (tmp_str.size() < 11) dLog += ")\t\t"; \
			else dLog += ")\t"; \
			dLog += (STR); \
		}

	#define PRINT_DIFFS(INFO, DIFFS) \
		if (DLOG & LOG_ALGO) { \
			LOGD(LOG_ALGO, INFO "\n"); \
			for (const auto& d: DIFFS) { \
				LOGD(LOG_ALGO, "\t" + std::string((d.type == diff_type::DIFF_IN_1) ? "D1" : \
						(d.type == diff_type::DIFF_IN_2 ? "D2" : "M")) + \
						" off: " + std::to_string(d.off + 1) + " len: " + std::to_string(d.len) + "\n"); \
			} \
		}

	extern std::string	dLog;
	extern DWORD		dLogTime_ms;

#else

	#define LOGD_GET_TIME
	#define LOGD(LOG_FILTER, STR)
	#define LOGDIF(LOG_FILTER, COND, STR)
	#define LOGDB(LOG_FILTER, BUFFID, STR)
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
	CMD_CLIPBOARD_DIFF,
	CMD_SVN_DIFF,
	CMD_GIT_DIFF,
	CMD_SEPARATOR_2,
	CMD_BOOKMARK_DIFFS,
	CMD_BOOKMARK_ADD_REM,
	CMD_BOOKMARK_CHANGED,
	CMD_SEPARATOR_3,
	CMD_COMPARE_SUMMARY,
	CMD_SEPARATOR_4,
	CMD_DETECT_MOVES,
	CMD_DETECT_CHAR_DIFFS,
	CMD_SEPARATOR_5,
	CMD_IGNORE_EMPTY_LINES,
	CMD_IGNORE_FOLDED_LINES,
	CMD_IGNORE_CHANGED_SPACES,
	CMD_IGNORE_ALL_SPACES,
	CMD_IGNORE_CASE,
	CMD_IGNORE_REGEX,
	CMD_SEPARATOR_6,
	CMD_SHOW_ONLY_DIFF,
	CMD_SHOW_ONLY_SEL,
	CMD_NAV_BAR,
	CMD_SEPARATOR_7,
	CMD_AUTO_RECOMPARE,
	CMD_SEPARATOR_8,
	CMD_PREV,
	CMD_NEXT,
	CMD_FIRST,
	CMD_LAST,
	CMD_PREV_CHANGE_POS,
	CMD_NEXT_CHANGE_POS,
	CMD_SEPARATOR_9,
	CMD_SETTINGS,
	CMD_SEPARATOR_10,
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
#ifndef NDEBUG
	assert(viewNum >= 0 && viewNum < 2);
#endif

	return sciFunc(sciPtr[viewNum], uMsg, wParam, lParam);
}


void ToggleNavigationBar();
