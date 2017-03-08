/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2013 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
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

#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include "SqliteHelper.h"

PSQLOPEN16			sqlite3_open16;
PSQLPREPARE16V2		sqlite3_prepare16_v2;
PSQLSTEP			sqlite3_step;
PSQLCOLUMNTEXT16	sqlite3_column_text16;
PSQLFINALZE			sqlite3_finalize;
PSQLCLOSE			sqlite3_close;

bool InitSQLite()
{
	static bool isInit = false;

	if (!isInit)
	{
		TCHAR dllPath[MAX_PATH];

		HMODULE hPlugin = ::GetModuleHandle(TEXT("ComparePlugin.dll"));
		if (!hPlugin)
			return false;

		::GetModuleFileName(hPlugin, (LPWSTR)dllPath, _countof(dllPath));
		::PathRemoveExtension(dllPath);
		_tcscat_s(dllPath, _countof(dllPath), TEXT("\\sqlite3.dll"));

		HMODULE ligSQLite = ::LoadLibrary(dllPath);
		if (!ligSQLite)
			return false;

		sqlite3_open16 = (PSQLOPEN16)::GetProcAddress(ligSQLite, "sqlite3_open16");
		if (!sqlite3_open16)
			return false;
		sqlite3_prepare16_v2 = (PSQLPREPARE16V2)::GetProcAddress(ligSQLite, "sqlite3_prepare16_v2");
		if (!sqlite3_prepare16_v2)
			return false;
		sqlite3_step = (PSQLSTEP)::GetProcAddress(ligSQLite, "sqlite3_step");
		if (!sqlite3_step)
			return false;
		sqlite3_column_text16 = (PSQLCOLUMNTEXT16)::GetProcAddress(ligSQLite, "sqlite3_column_text16");
		if (!sqlite3_column_text16)
			return false;
		sqlite3_finalize = (PSQLFINALZE)::GetProcAddress(ligSQLite, "sqlite3_finalize");
		if (!sqlite3_finalize)
			return false;
		sqlite3_close = (PSQLCLOSE)::GetProcAddress(ligSQLite, "sqlite3_close");
		if (!sqlite3_close)
			return false;

		isInit = true;
	}

	return true;
}
