/*
This file is part of Plugin Template Plugin for Notepad++
Copyright (C)2009

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "NavDialog.h"
#include "Resource.h"

NavDialog::NavDialog(void) : DockingDlgInterface(IDD_NAV_DIALOG)
{
}

NavDialog::~NavDialog(void)
{
}


void NavDialog::init(HINSTANCE hInst, NppData nppData)
{
	_nppData = nppData;
	DockingDlgInterface::init(hInst, nppData._nppHandle);
}


void NavDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_RIGHT;
        _data.pszName       = TEXT("NavBar");
		_data.pszModuleName	= getPluginFileName();
        _data.dlgID			= CMD_USE_NAV_BAR;

		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
	}
	display(willBeShown);
}

BOOL CALLBACK NavDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			break;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
            // Refresh display because if size change, drawing change too.
            InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case WM_COMMAND:
		{
			break;
		}
	    case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(_hSelf, &ps);
			//PaintWindow(hdc);	
            DrawRectangle(hdc);         
			EndPaint(_hSelf, &ps);			
		}
		case WM_NOTIFY:
		{
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
			break;
		}
		case WM_DESTROY:
		{
			break;
		}
		default:
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
	}

	return FALSE;
}

//RECT rct = {0};
//getClientRect(rct);

void NavDialog::DrawRectangle(HDC hdc)
{
    //HBRUSH hBrush = CreateSolidBrush(RGB(255,255,255));
	RECT rLeft;
    RECT rRight;
    RECT rc = {0};
    
    getClientRect(rc);

    int w = rc.right / 5;

    rLeft.top    = rc.top + 20;
    rLeft.bottom = rc.bottom - 20;
    rLeft.left   = w;
    rLeft.right  = 2 * w;

    rRight.top    = rc.top + 20;
    rRight.bottom = rc.bottom - 20;
    rRight.left   = 3 * w;
    rRight.right  = 4 * w;

    //r.left = rc.left + 10;
    //r.right = r.left + 10;
    //r.top = rc.top + 10;
    //r.bottom = rc.bottom - 10;

    //FillRect(hdc, &r, hBrush);   
    Rectangle(hdc, rLeft.left, rLeft.top, rLeft.right, rLeft.bottom);
    Rectangle(hdc, rRight.left, rRight.top, rRight.right, rRight.bottom);
}