// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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
// along with this program.	 If not, see <https://www.gnu.org/licenses/>.


#pragma once

#include <windows.h>
#include <wchar.h>
#include <string>
#include "DockingFeature/Window.h"


class URLCtrl : public Window {
public:
	void create(HWND itemHandle, const wchar_t * link, COLORREF linkColor = RGB(0,0,255));
	void create(HWND itemHandle, int cmd, HWND msgDest = NULL);
	void destroy() override;

private:
	HCURSOR& loadHandCursor();
	void action();

protected :
	std::wstring	_URL;
	HFONT			_hfUnderlined	= nullptr;
	HCURSOR			_hCursor		= nullptr;

	HWND			_msgDest	= nullptr;
	unsigned long	_cmdID		= 0;

	WNDPROC		_oldproc		= nullptr;
	COLORREF	_linkColor		= RGB(0xFF, 0xFF, 0xFF);
	bool		_clicking		= false;

	static LRESULT CALLBACK URLCtrlProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		return ((URLCtrl *)(::GetWindowLongPtrW(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam);
	};

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};
