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
        _data.pszName       = TEXT("Nav Bar");
		_data.pszModuleName	= getPluginFileName();
        _data.dlgID			= CMD_USE_NAV_BAR;

		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
    }
    // Display
	display(willBeShown);

    if (willBeShown) Do();
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

    m_hMemBMP1    = ::CreateCompatibleBitmap(m_hdc, 1, m_TextLength);
    m_hMemBMP2    = ::CreateCompatibleBitmap(m_hdc, 1, m_TextLength);
    m_hMemBMPView = ::CreateCompatibleBitmap(m_hdc, 1, m_TextLength);

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
            ReadyToDraw = FALSE;
    		break;
		}

	    case WM_PAINT:
		{
            ReadyToDraw = TRUE;
            return OnPaint(hWnd);
		}

		case WM_NOTIFY:
		{
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
			break;
		}

		case WM_DESTROY:
		{
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
    long color = 0;

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
        case MARKER_BLANK_LINE:   color = m_BlankColor;     break;
        case MARKER_ADDED_LINE:   color = m_AddedColor;     break;
        case MARKER_CHANGED_LINE: color = m_ChangedColor;   break;
        case MARKER_MOVED_LINE:   color = m_MovedColor;     break;
        case MARKER_REMOVED_LINE: color = m_DeletedColor;   break;
        default:                  color = RGB(255,255,255); break;
        }

        // Draw line for the first document
        SetPixel(m_hMemDC1, 0, i, color);

        // Choose a pencil to draw with
        switch(m_ResultsDoc2[i])
        {
        case MARKER_BLANK_LINE:   color = m_BlankColor;     break;
        case MARKER_ADDED_LINE:   color = m_AddedColor;     break;
        case MARKER_CHANGED_LINE: color = m_ChangedColor;   break;
        case MARKER_MOVED_LINE:   color = m_MovedColor;     break;
        case MARKER_REMOVED_LINE: color = m_DeletedColor;   break;
        default:                  color = RGB(255,255,255); break;
        }

        // Draw line for the first document
        SetPixel(m_hMemDC2, 0, i, color);
    }

    InvalidateRect(m_hWnd, NULL, TRUE);

    DeleteObject(hBrush);
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

    // If side bar is too small, don't draw anything
    if ((SzX < 5) || (SzX < 5)) return false;

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

    SetStretchBltMode(m_hdc, COLORONCOLOR);

    int x, y, cx, cy;

    // Current doc view

    x  = r.left;
    y  = r.top + SPACE;
    cx = r.right - x;
    cy = r.bottom - y - SPACE;

    StretchBlt(m_hdc, x, y, cx, cy, m_hMemDCView, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

    // Draw bar border
    Rectangle(m_hdc, m_rLeft.left, m_rLeft.top, m_rLeft.right, m_rLeft.bottom);
    Rectangle(m_hdc, m_rRight.left, m_rRight.top, m_rRight.right, m_rRight.bottom);

    // Draw Left bar

    x  = m_rLeft.left + 1;
    y  = m_rLeft.top + 1;
    cx = m_rLeft.right - x - 1;
    cy = m_rLeft.bottom - y - 1;

    StretchBlt(m_hdc, x, y, cx, cy, m_hMemDC1, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

    // Draw Right bar

    x  = m_rRight.left + 1;
    y  = m_rRight.top + 1;
    cx = m_rRight.right - x - 1;
    cy = m_rRight.bottom - y - 1;

    StretchBlt(m_hdc, x, y, cx, cy, m_hMemDC2, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

	::EndPaint(hWnd, &ps);

    return 0;
}

void NavDialog::DrawView(long start, long end)
{    
    RECT bmpRect;
    bmpRect.top = 0;
    bmpRect.left = 0;
    bmpRect.right = m_hMemBMPSize.cx;
    bmpRect.bottom = m_hMemBMPSize.cy;

	HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    FillRect(m_hMemDCView, &bmpRect, hBrush);

    for(long i = start; i <= end; i++) SetPixel(m_hMemDCView, 0, i, RGB(0,0,0));

    InvalidateRect(m_hWnd, NULL, TRUE);

    DeleteObject(hBrush);
}