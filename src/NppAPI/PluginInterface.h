// This file is part of Notepad++ project
// Copyright (C)2025 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.



// For more comprehensive information on plugin communication, please refer to the following resource:
// https://npp-user-manual.org/docs/plugin-communication/

#pragma once

#include "Scintilla.h"
#include "Notepad_plus_msgs.h"


typedef const wchar_t * (__cdecl * PFUNCGETNAME)();

struct NppData
{
	HWND _nppHandle = nullptr;
	HWND _scintillaMainHandle = nullptr;
	HWND _scintillaSecondHandle = nullptr;
};

typedef void (__cdecl * PFUNCSETINFO)(NppData);
typedef void (__cdecl * PFUNCPLUGINCMD)();
typedef void (__cdecl * PBENOTIFIED)(SCNotification *);
typedef LRESULT (__cdecl * PMESSAGEPROC)(UINT Message, WPARAM wParam, LPARAM lParam);


struct ShortcutKey
{
	bool _isCtrl = false;
	bool _isAlt = false;
	bool _isShift = false;
	UCHAR _key = 0;
};

const int menuItemSize = 64;

struct FuncItem
{
	wchar_t _itemName[menuItemSize] = { '\0' };
	PFUNCPLUGINCMD _pFunc = nullptr;
	int _cmdID = 0;
	bool _init2Check = false;
	ShortcutKey *_pShKey = nullptr;
};

typedef FuncItem * (__cdecl * PFUNCGETFUNCSARRAY)(int *);

// You should implement (or define an empty function body) those functions which are called by Notepad++ plugin manager
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const wchar_t * getName();
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *);
extern "C" __declspec(dllexport) void beNotified(SCNotification *);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam);

// This API return always true now, since Notepad++ isn't compiled in ANSI mode anymore
extern "C" __declspec(dllexport) BOOL isUnicode();

