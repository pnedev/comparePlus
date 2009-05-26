/*
this file is part of HexEdit Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#ifndef COLORCOMBO_H
#define COLORCOMBO_H

#include "Window.h"
#include "ColorPopup.h"

#if(WINVER <= 0x0400)
struct COMBOBOXINFO 
{
    int cbSize;
    RECT rcItem;
    RECT rcButton;
    DWORD stateButton;
    HWND hwndCombo;
    HWND hwndItem;
    HWND hwndList; 
};
#endif 


#define	CB_GETCOMBOBOXINFO	0x0164

class ColorCombo : public Window
{
public :
	ColorCombo() : Window(), _rgbCol(0), _pColorPopup(NULL) {
		::ZeroMemory(&_comboBoxInfo, sizeof(_comboBoxInfo));
	};
    ~ColorCombo () {};
	virtual void init(HINSTANCE hInst, HWND hNpp, HWND hCombo);
	virtual void destroy() {
		DestroyWindow(_hSelf);
	};

	void onSelect(void) {
		DrawColor();
	};

	void setColor(COLORREF rgbCol) {
		_rgbCol = rgbCol;
		::RedrawWindow(_comboBoxInfo.hwndItem, &_comboBoxInfo.rcItem, NULL, TRUE);
	};
	void getColor(LPCOLORREF p_rgbCol) {
		if (p_rgbCol != NULL) {
			*p_rgbCol = _rgbCol;
		}
	};

private:
	void DrawColor(HDC hDcExt = NULL);

private :
	HWND					_hNpp;
	COMBOBOXINFO			_comboBoxInfo;
    WNDPROC					_hDefaultComboProc;
    
    COLORREF                _rgbCol;
	ColorPopup*				_pColorPopup;

	/* Subclassing combo boxes */
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((ColorCombo *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
};

#endif // COLORCOMBO_H
