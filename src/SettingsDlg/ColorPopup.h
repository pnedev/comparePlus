/*
 * this file is part of notepad++
 * Copyright (C)2003 Don HO < donho@altern.org >
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#pragma once


#include "DockingFeature/Window.h"
#include "resource.h"

class ColorPopup : public Window
{
public :
	ColorPopup() : Window(), _isColorChooserLaunched(false) {};
	ColorPopup(COLORREF defaultColor) : Window(), _isColorChooserLaunched(false), _color(defaultColor) {};
	~ColorPopup(){};

	// Bring base class virtual functions into scope to avoid hiding warnings
	using Window::init;

	void init(HINSTANCE hInst, HWND hParent, HWND hNpp) {
		_hNpp = hNpp;
		Window::init(hInst, hParent);
	}

	bool isCreated() const {
		return (_hSelf != NULL);
	};

	void create(int dialogID);

		void doDialog(POINT p) {
			if (!isCreated())
				create(IDD_COLOR_POPUP);
			::SetWindowPos(_hSelf, HWND_TOP, p.x, p.y, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW);
	};

	virtual void destroy() {
		::DestroyWindow(_hSelf);
	};
	COLORREF getSelColor(){return _color;};

private :
	HWND		_hNpp;
	RECT		_rc;
	bool		_isColorChooserLaunched;
	COLORREF	_color;

	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};
