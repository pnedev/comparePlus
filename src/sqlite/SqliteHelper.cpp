#include "SqliteHelper.h"

bool bSqliteInitialized;
PSQLOPEN16 sqlite3_open16;
PSQLPREPARE16V2 sqlite3_prepare16_v2;
PSQLSTEP sqlite3_step;
PSQLCOLUMNTEXT16 sqlite3_column_text16;
PSQLFINALZE sqlite3_finalize;
PSQLCLOSE sqlite3_close;

bool InitSqlite()
{
	if (!bSqliteInitialized)
	{
		TCHAR buffer[MAX_PATH];

		// get sqlite3.dll path from plugin subfolder
		HMODULE mPlugin = GetModuleHandle(L"ComparePlugin.dll");
		if (!mPlugin) return false;
		int len = GetModuleFileName(mPlugin, (LPWSTR)buffer, MAX_PATH);
		buffer[len - 4] = 0;
		lstrcat(buffer, L"\\sqlite3.dll");

		HMODULE mSqlite = LoadLibrary(buffer);
		if (!mSqlite) return false;

		sqlite3_open16 = (PSQLOPEN16)GetProcAddress(mSqlite, "sqlite3_open16");
		if (!sqlite3_open16) return false;
		sqlite3_prepare16_v2 = (PSQLPREPARE16V2)GetProcAddress(mSqlite, "sqlite3_prepare16_v2");
		if (!sqlite3_prepare16_v2) return false;
		sqlite3_step = (PSQLSTEP)GetProcAddress(mSqlite, "sqlite3_step");
		if (!sqlite3_step) return false;
		sqlite3_column_text16 = (PSQLCOLUMNTEXT16)GetProcAddress(mSqlite, "sqlite3_column_text16");
		if (!sqlite3_column_text16) return false;
		sqlite3_finalize = (PSQLFINALZE)GetProcAddress(mSqlite, "sqlite3_finalize");
		if (!sqlite3_finalize) return false;
		sqlite3_close = (PSQLCLOSE)GetProcAddress(mSqlite, "sqlite3_close");
		if (!sqlite3_close) return false;

		bSqliteInitialized = true;
	}

	return true;
}
