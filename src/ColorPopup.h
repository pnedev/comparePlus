/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef COLOR_POPUP_H
#define COLOR_POPUP_H

#include "Window.h"
#include "Resource.h"
#include "CompareResource.h"

class ColorPopup : public Window
{
public :
    ColorPopup() : Window(), isColorChooserLaunched(false) {};
	ColorPopup(COLORREF defaultColor) : Window(), isColorChooserLaunched(false), _color(defaultColor) {};
	~ColorPopup(){};

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
    COLORREF	_color;
	bool		isColorChooserLaunched;

	static BOOL CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif //COLOR_POPUP_H

