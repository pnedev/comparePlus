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
    delete(m_ResultsDoc1);
    delete(m_ResultsDoc2);
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
    else
    {
        Do();
    }

    // Display
	display(willBeShown);
}

void NavDialog::Do(void)
{
    m_hdc = GetDC(m_hWnd);

    // Get max file length
    int doc1 = SendMessage(_nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0);
    int doc2 = SendMessage(_nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0);
    (doc1 > doc2) ? (m_TextLength = doc1) : (m_TextLength = doc2);

    // Create BMP used to store graphical representation
    m_hMemDC1    = ::CreateCompatibleDC(m_hdc);
    m_hMemDC2    = ::CreateCompatibleDC(m_hdc);
    m_hMemDCView = ::CreateCompatibleDC(m_hdc);

    m_hMemBMP1    = ::CreateCompatibleBitmap(m_hdc, 10, m_TextLength);
    m_hMemBMP2    = ::CreateCompatibleBitmap(m_hdc, 10, m_TextLength);
    m_hMemBMPView = ::CreateCompatibleBitmap(m_hdc, 10, m_TextLength);

    // Retrieve created BMP info (BMP1 == BMP2)
    GetObject(m_hMemBMP1, sizeof(m_hMemBMPInfo), &m_hMemBMPInfo);
    m_hMemBMPSize.cx = m_hMemBMPInfo.bmWidth;
    m_hMemBMPSize.cy = m_hMemBMPInfo.bmHeight; 

    // Attach BMP to a DC
    SelectObject(m_hMemDC1,    m_hMemBMP1);
    SelectObject(m_hMemDC2,    m_hMemBMP2);
    SelectObject(m_hMemDCView, m_hMemBMPView);

    // Release DC
    ReleaseDC(m_hWnd, m_hdc);

    // Create line array
    int marker = 0;

    m_ResultsDoc1 = new long[m_TextLength];
    m_ResultsDoc2 = new long[m_TextLength];

    for (int i = 0; i < doc1; i++)
    {
        marker = SendMessage(_nppData._scintillaMainHandle, SCI_MARKERGET, (WPARAM)i, 0);

        if      (marker & (1 << MARKER_BLANK_LINE))   m_ResultsDoc1[i] = MARKER_BLANK_LINE;
        else if (marker & (1 << MARKER_ADDED_LINE))   m_ResultsDoc1[i] = MARKER_ADDED_LINE;
        else if (marker & (1 << MARKER_CHANGED_LINE)) m_ResultsDoc1[i] = MARKER_CHANGED_LINE;
        else if (marker & (1 << MARKER_MOVED_LINE))   m_ResultsDoc1[i] = MARKER_MOVED_LINE;
        else if (marker & (1 << MARKER_REMOVED_LINE)) m_ResultsDoc1[i] = MARKER_REMOVED_LINE;
        else                                          m_ResultsDoc1[i] = -1;
    }

    for (int i = 0; i < doc2; i++)
    {
        marker = SendMessage(_nppData._scintillaSecondHandle, SCI_MARKERGET, (WPARAM)i, 0);

        if      (marker & (1 << MARKER_BLANK_LINE))   m_ResultsDoc2[i] = MARKER_BLANK_LINE;
        else if (marker & (1 << MARKER_ADDED_LINE))   m_ResultsDoc2[i] = MARKER_ADDED_LINE;
        else if (marker & (1 << MARKER_CHANGED_LINE)) m_ResultsDoc2[i] = MARKER_CHANGED_LINE;
        else if (marker & (1 << MARKER_MOVED_LINE))   m_ResultsDoc2[i] = MARKER_MOVED_LINE;
        else if (marker & (1 << MARKER_REMOVED_LINE)) m_ResultsDoc2[i] = MARKER_REMOVED_LINE;
        else                                          m_ResultsDoc2[i] = -1;
    }

    CreateBitmap();
}

BOOL CALLBACK NavDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
            // Here, I modify window styles (set H and V redraw)
            SetClassLong(hWnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);

            m_hWnd = hWnd;

            Do();

    		break;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
            //RECT rc = {0};
            //getClientRect(rc);
            //InvalidateRect(hWnd, &rc, TRUE);
			return 0;
		}
		case WM_COMMAND:
		{
			break;
		}
	    case WM_PAINT:
		{
            return OnPaint(hWnd);
		}

		case WM_NOTIFY:
		{
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
			break;
		}

		case WM_DESTROY:
		{
            //ReleaseDC(hWnd, hdc);

            // Delete objects
            DeleteDC(m_hMemDC1);
            DeleteDC(m_hMemDC2);
            DeleteDC(m_hMemDCView);
            DeleteObject(m_hMemBMP1);
            DeleteObject(m_hMemBMP2);
            DeleteObject(m_hMemBMPView);

            PostQuitMessage(0); 
			break;
		}
		default:
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
	}

	return FALSE;
}

void NavDialog::SetColor(int added, int deleted, int changed, int moved, int blank)
{
    m_AddedColor   = added;
    m_DeletedColor = deleted;
    m_ChangedColor = changed;
    m_MovedColor   = moved;
    m_BlankColor   = blank;
}

void NavDialog::CreateBitmap(void)
{
    HPEN whitePen = CreatePen(PS_SOLID, 1, RGB(255,255,255));

    // Create pencils
    m_BlankPencil   = CreatePen(PS_SOLID, 1, m_BlankColor);
    m_AddedPencil   = CreatePen(PS_SOLID, 1, m_AddedColor);
    m_ChangedPencil = CreatePen(PS_SOLID, 1, m_ChangedColor);
    m_MovedPencil   = CreatePen(PS_SOLID, 1, m_MovedColor);
    m_RemovedPencil = CreatePen(PS_SOLID, 1, m_DeletedColor);

    // Fill BMP background
    HBRUSH hBrush = CreateSolidBrush(RGB(255,255,255));
    RECT bmpRect;
    bmpRect.top = 0;
    bmpRect.left = 0;
    bmpRect.right = m_hMemBMPSize.cx;
    bmpRect.bottom = m_hMemBMPSize.cy;
    FillRect(m_hMemDC1, &bmpRect, hBrush);
    FillRect(m_hMemDC2, &bmpRect, hBrush);

    // Draw BMPs - For all lines in document 
    // Note: doc1 nb lines == doc2 nb lines when compared
    for (int i = 0; i < m_TextLength; i++)
    {
        // Choose a pencil to draw with
        switch(m_ResultsDoc1[i])
        {
        case MARKER_BLANK_LINE:
            SelectObject(m_hMemDC1, m_BlankPencil);
            break;
        case MARKER_ADDED_LINE:
            SelectObject(m_hMemDC1, m_AddedPencil);
            break;
        case MARKER_CHANGED_LINE:
            SelectObject(m_hMemDC1, m_ChangedPencil);
            break;
        case MARKER_MOVED_LINE:
            SelectObject(m_hMemDC1, m_MovedPencil);
            break;
        case MARKER_REMOVED_LINE:
            SelectObject(m_hMemDC1, m_RemovedPencil);
            break;
        default:
            SelectObject(m_hMemDC1, whitePen);
            break;
        }

        // Draw line for the first document
        MoveToEx(m_hMemDC1, 0, i, (LPPOINT) NULL); 
        LineTo(m_hMemDC1, m_hMemBMPSize.cx, i);

        // Choose a pencil to draw with
        switch(m_ResultsDoc2[i])
        {
        case MARKER_BLANK_LINE:
            SelectObject(m_hMemDC2, m_BlankPencil);
            break;
        case MARKER_ADDED_LINE:
            SelectObject(m_hMemDC2, m_AddedPencil);
            break;
        case MARKER_CHANGED_LINE:
            SelectObject(m_hMemDC2, m_ChangedPencil);
            break;
        case MARKER_MOVED_LINE:
            SelectObject(m_hMemDC2, m_MovedPencil);
            break;
        case MARKER_REMOVED_LINE:
            SelectObject(m_hMemDC2, m_RemovedPencil);
            break;
        default:
            SelectObject(m_hMemDC2, whitePen);
            break;
        }

        // Draw line for the first document
        MoveToEx(m_hMemDC2, 0, i, (LPPOINT) NULL); 
        LineTo(m_hMemDC2, m_hMemBMPSize.cx, i);
    }

    InvalidateRect(m_hWnd, NULL, TRUE);
}

LRESULT NavDialog::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	RECT        r;

    // Get current DC
    m_hdc = ::BeginPaint(hWnd, &ps);

    // Draw bars
    GetClientRect(hWnd, &r);

    long SzX = ((r.right - r.left) - 3 * SPACE) / 2;
    long SzY = (r.bottom - r.top) - 2 * SPACE;

    // Define left rectangle coordinates
    m_rLeft.top    = SPACE;    
    m_rLeft.left   = SPACE;
    m_rLeft.right  = m_rLeft.left + SzX;
    m_rLeft.bottom = m_rLeft.top + SzY;

    // Define right rectangle coordinates
    m_rRight.top    = SPACE;
    m_rRight.left   = m_rLeft.right + SPACE;
    m_rRight.right  = m_rRight.left + SzX;
    m_rRight.bottom = m_rRight.top + SzY;

    // Draw the two rectangles
    Rectangle(m_hdc, m_rLeft.left, m_rLeft.top, m_rLeft.right, m_rLeft.bottom);
    Rectangle(m_hdc, m_rRight.left, m_rRight.top, m_rRight.right, m_rRight.bottom);

    // Draw content

    // Set stretch mode
    SetStretchBltMode(m_hdc, COLORONCOLOR);

    int x, y, cx, cy;

    x  = m_rLeft.left + 1;
    y  = m_rLeft.top + 1;
    cx = m_rLeft.right - x - 1;
    cy = m_rLeft.bottom - y - 1;

    StretchBlt(m_hdc, x, y, cx, cy, m_hMemDC1, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

    x  = m_rRight.left + 1;
    y  = m_rRight.top + 1;
    cx = m_rRight.right - x - 1;
    cy = m_rRight.bottom - y - 1;

    StretchBlt(m_hdc, x, y, cx, cy, m_hMemDC2, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

    // Draw current view
    //SetStretchBltMode(m_hdc, COLORONCOLOR);
    //StretchBlt(m_hdc, x, y, cx, cy, m_hMemDCView, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCPAINT);

	::EndPaint(hWnd, &ps);

    return 0;
}

void NavDialog::DrawView(void)
{
    long start, end;
    RECT r;
    int x, y, cx, cy;

    GetClientRect(m_hWnd, &r);

    x  = r.left;
    y  = r.top;
    cx = r.right - x;
    cy = r.bottom - y;

    start = SendMessage(_nppData._scintillaMainHandle, SCI_GETFIRSTVISIBLELINE, 0, 0) + 1;
    end   = SendMessage(_nppData._scintillaMainHandle, SCI_LINESONSCREEN, 0, 0) + start;

    HBRUSH hBrush = CreateSolidBrush(RGB(255,0,0));
    RECT bmpRect;
    bmpRect.top = start;
    bmpRect.left = 0;
    bmpRect.right = cx;
    bmpRect.bottom = end;
    FillRect(m_hMemDCView, &bmpRect, hBrush);
    Rectangle(m_hMemDCView, 0, start, 10, end);

    InvalidateRect(m_hWnd, NULL, TRUE);
}