/*
 * This file is part of Compare Plugin for Notepad++
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

#pragma once


#include "Window.h"
#include "ColorPopup.h"


class ColorCombo : public Window
{
public :
	ColorCombo() : Window(), _rgbCol(0), _pColorPopup(NULL)
	{
		::ZeroMemory(&_comboBoxInfo, sizeof(_comboBoxInfo));
	};

	~ColorCombo () {};
	virtual void init(HINSTANCE hInst, HWND hNpp, HWND hCombo);
	virtual void destroy()
	{
		::DestroyWindow(_hSelf);
	};

	void onSelect(void)
	{
		DrawColor();
	};

	void setColor(COLORREF rgbCol)
	{
		_rgbCol = rgbCol;
		::RedrawWindow(_comboBoxInfo.hwndItem, &_comboBoxInfo.rcItem, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	};

	void getColor(LPCOLORREF p_rgbCol)
	{
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

	static LRESULT CALLBACK wndDefaultProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		return (((ColorCombo *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
};
