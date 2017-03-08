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

#include "ColorCombo.h"
#include "resource.h"


void ColorCombo::init(HINSTANCE hInst, HWND hNpp, HWND hCombo)
{
	_hNpp	= hNpp;
	Window::init(hInst, hNpp);

	/* subclass combo to get edit messages */
	_comboBoxInfo.cbSize = sizeof(_comboBoxInfo);
	::SendMessage(hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&_comboBoxInfo);
	::SetWindowLongPtr(_comboBoxInfo.hwndItem, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	_hDefaultComboProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_comboBoxInfo.hwndItem, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndDefaultProc)));
}


LRESULT ColorCombo::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case COLOR_POPUP_OK:
		{
			setColor((COLORREF)wParam);

			_pColorPopup->destroy();
			delete _pColorPopup;
			_pColorPopup = NULL;
			return TRUE;
		}
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			RECT		rc;
			POINT		pt;
			::GetWindowRect(hwnd, &rc);
			pt.x = rc.left;
			pt.y = rc.bottom;

			if (_pColorPopup == NULL) {
				_pColorPopup = new ColorPopup(_rgbCol);
				_pColorPopup->init(_hInst, hwnd, _hNpp);
				_pColorPopup->doDialog(pt);
			}
			return TRUE;
		}
		case COLOR_POPUP_CANCEL:
		case WM_DESTROY:
		{
			if (_pColorPopup != NULL) {
				_pColorPopup->destroy();
				delete _pColorPopup;
				_pColorPopup = NULL;
			}
			break;
		}
		case WM_PAINT:
		{
			LRESULT lpRet = ::CallWindowProc(_hDefaultComboProc, hwnd, Message, wParam, lParam);
			DrawColor((HDC)wParam);
			return lpRet;
		}
		default :
			break;
	}
	return ::CallWindowProc(_hDefaultComboProc, hwnd, Message, wParam, lParam);
}


void ColorCombo::DrawColor(HDC hDcExt)
{
	HDC		hDc			= NULL;
	HBRUSH	hBrush		= ::CreateSolidBrush(_rgbCol);

	if (hDcExt == NULL) {
		hDc	= ::GetWindowDC(_comboBoxInfo.hwndCombo);
	} else {
		hDc = hDcExt;
	}

	/* draw item */
	::FillRect(hDc, &_comboBoxInfo.rcItem, hBrush);

	/* draw selection on focus */
	if (_comboBoxInfo.hwndCombo == ::GetFocus())
	{
		RECT	rc	= _comboBoxInfo.rcItem;
		::InflateRect(&rc, -1, -1);
		::DrawFocusRect(hDc, &rc);
	}

	::DeleteObject(hBrush);

	if (hDcExt == NULL) {
		::ReleaseDC(_comboBoxInfo.hwndCombo, hDc);
	}
}


