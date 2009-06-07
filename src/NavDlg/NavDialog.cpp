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
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
			return 0;
		}
		case WM_COMMAND:
		{
			break;
		}
	    case WM_PAINT:
		{
			PAINTSTRUCT ps;
            hdc = BeginPaint(hWnd, &ps);
            DrawRectangle(hdc);
            DisplayResults(hdc);
			EndPaint(hWnd, &ps);
            break;
		}
		case WM_NOTIFY:
		{
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
			break;
		}
		case WM_DESTROY:
		{
            PostQuitMessage(0); 
			break;
		}
		default:
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
	}

	return FALSE;
}

void NavDialog::DrawRectangle(HDC hdc)
{
    //HBRUSH hBrush = CreateSolidBrush(RGB(255,255,255));
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

void NavDialog::DisplayResults(HDC hdc)
{
    int marker = 0;
    int i = 0;
    
    int NavBarLength = rLeft.bottom - rLeft.top;

    /*
    SCI_GETLINECOUNT
    This returns the number of lines in the document. 
    An empty document contains 1 line. 
    A document holding only an end of line sequence has 2 lines.
    */
    int MaxDocLength;
    int doc1 = SendMessage(_nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0);
    int doc2 = SendMessage(_nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0);

    (doc1 > doc2) ? (MaxDocLength = doc1 - 1) : (MaxDocLength = doc2 - 1);

    int LineWidth = NavBarLength / MaxDocLength;

    /* Draw left doc results */
    for (i = 0; i < doc1; i++)
    {
        marker = SendMessage(_nppData._scintillaMainHandle, SCI_MARKERGET, (WPARAM)i, 0);
        if (marker != 0)
        {
            DrawLine(LineWidth, i, 0, marker);
        }
    }

    /* Draw right doc results */
    for (i = 0; i < doc2; i++)
    {
        int marker = SendMessage(_nppData._scintillaSecondHandle, SCI_MARKERGET, (WPARAM)i, 0);
        if (marker != 0)
        {
            DrawLine(LineWidth, i, 1, marker);
        }
    }
}

void NavDialog::DrawLine(int width, int line, bool view, int marker)
{
    HBRUSH hBrush;
    RECT r = {0};

    /* Colorize */
    if (marker & (1 << MARKER_BLANK_LINE))
    {  
        hBrush = CreateSolidBrush(blank);
    }
    else if (marker & (1 << MARKER_ADDED_LINE))
    {
        hBrush = CreateSolidBrush(added);
    }
    else if (marker & (1 << MARKER_CHANGED_LINE))
    {
        hBrush = CreateSolidBrush(changed);
    }
    else if (marker & (1 << MARKER_MOVED_LINE))
    {
        hBrush = CreateSolidBrush(moved);
    }
    else if (marker & (1 << MARKER_REMOVED_LINE))
    {
        hBrush = CreateSolidBrush(deleted);
    }
    else 
    {
        hBrush = CreateSolidBrush(RGB(255,255,255));
    }

    /* Left view */
    if (view == 0)
    {
        r.top    = rLeft.top + line * width;
        r.bottom = rLeft.top + (line + 1) * width;
        r.left   = rLeft.left + 1;
        r.right  = rLeft.right - 1;

        FillRect(hdc, &r, hBrush);
    }

    /* Right view */
    else
    {
        r.top    = rRight.top + line * width;
        r.bottom = rRight.top + (line + 1) * width;
        r.left   = rRight.left + 1;
        r.right  = rRight.right - 1;

        FillRect(hdc, &r, hBrush);
    }
}