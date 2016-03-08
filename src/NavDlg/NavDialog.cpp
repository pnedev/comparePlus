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
#include "resource.h"

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
	m_hdc = GetDC(_hSelf);

    SetScalingFactor(_hSelf);

	// Create BMP used to store graphical representation
	m_hMemDC1    = ::CreateCompatibleDC(m_hdc);
	m_hMemDC2    = ::CreateCompatibleDC(m_hdc);

	m_hMemBMP1    = ::CreateCompatibleBitmap(m_hdc, 1, m_DocLineCount);
	m_hMemBMP2    = ::CreateCompatibleBitmap(m_hdc, 1, m_DocLineCount);

	// Retrieve created BMP info (BMP1 == BMP2)
	GetObject(m_hMemBMP1, sizeof(m_hMemBMPInfo), &m_hMemBMPInfo);
	m_hMemBMPSize.cx = m_hMemBMPInfo.bmWidth;
	m_hMemBMPSize.cy = m_hMemBMPInfo.bmHeight;

	// Attach BMP to a DC
	SelectObject(m_hMemDC1,    m_hMemBMP1);
	SelectObject(m_hMemDC2,    m_hMemBMP2);

	// Release DC
	ReleaseDC(_hSelf, m_hdc);

	// Create line array
	int marker = 0;

	m_ResultsDoc1 = new long[m_DocLineCount];
	m_ResultsDoc2 = new long[m_DocLineCount];

    for (int i = 0; i < m_LineCount1; i++)
	{
		marker = SendMessage(_nppData._scintillaMainHandle, SCI_MARKERGET, (WPARAM)i, 0);

		if      (marker & (1 << MARKER_BLANK_LINE))   m_ResultsDoc1[i] = MARKER_BLANK_LINE;
		else if (marker & (1 << MARKER_ADDED_LINE))   m_ResultsDoc1[i] = MARKER_ADDED_LINE;
		else if (marker & (1 << MARKER_CHANGED_LINE)) m_ResultsDoc1[i] = MARKER_CHANGED_LINE;
		else if (marker & (1 << MARKER_MOVED_LINE))   m_ResultsDoc1[i] = MARKER_MOVED_LINE;
		else if (marker & (1 << MARKER_REMOVED_LINE)) m_ResultsDoc1[i] = MARKER_REMOVED_LINE;
		else                                          m_ResultsDoc1[i] = -1;
	}

    for (int i = 0; i < m_LineCount2; i++)
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

BOOL CALLBACK NavDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	signed short yPos;
	int LineStart;
	int LineVisible;
	int Delta;
	HWND curView = NULL;
	int currentEdit = -1;

	switch (Message)
	{
		case WM_INITDIALOG:
		{
			// Here, I modify window styles (set H and V redraw)
			SetClassLong(_hSelf, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);
			ReadyToDraw = FALSE;
			break;
		}

		case WM_PAINT:
		{
			ReadyToDraw = TRUE;
			return OnPaint(_hSelf);
		}

		case WM_NOTIFY:
		{
			//return DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
			break;
		}

		case WM_DESTROY:
		{
			// Delete objects
			DeleteDC(m_hMemDC1);
			DeleteDC(m_hMemDC2);
			DeleteObject(m_hMemBMP1);
			DeleteObject(m_hMemBMP2);

			PostQuitMessage(0);
			break;
		}

		case WM_LBUTTONDOWN:
			SetCapture(_hSelf);
			yPos = HIWORD(lParam);
			if (yPos < 0) yPos = 0;
            current_line = (long)((double)yPos * m_ScaleFactorDocLines);
			::SendMessage(_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
			if (currentEdit != -1)
			{
				curView = (currentEdit == 0) ? (_nppData._scintillaMainHandle) : (_nppData._scintillaSecondHandle);
				LineVisible = SendMessageA(curView, SCI_LINESONSCREEN, 0, 0);
				LineStart = SendMessageA(curView, SCI_GETFIRSTVISIBLELINE, 0, 0);
				Delta = current_line - LineVisible / 2 - LineStart;
				SendMessageA(curView, SCI_LINESCROLL, 0, (LPARAM)Delta);
				SendMessageA(curView, SCI_GOTOLINE, (WPARAM)current_line, 0);
			}
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			if (GetCapture() == _hSelf)
			{
				yPos = HIWORD(lParam);
				if (yPos < 0) yPos = 0;
                const long next_line = (const long)((double)yPos * m_ScaleFactorDocLines);
				Delta = next_line - current_line;
				::SendMessage(_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
				if (currentEdit != -1)
				{
					curView = (currentEdit == 0) ? (_nppData._scintillaMainHandle) : (_nppData._scintillaSecondHandle);
					SendMessageA(curView, SCI_LINESCROLL, 0, (LPARAM)Delta);
					SendMessageA(curView, SCI_GOTOLINE, (WPARAM)next_line, 0);
					current_line = next_line;
				}
			}
			break;

		default:
			return DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
	}

	return FALSE;
}

void NavDialog::SetColor(int added, int deleted, int changed, int moved, int blank, int _default)
{
	m_AddedColor   = added;
	m_DeletedColor = deleted;
	m_ChangedColor = changed;
	m_MovedColor   = moved;
    m_BlankColor   = blank;
    m_DefaultColor = _default;
}

void NavDialog::SetLinePixel(long resultsDoc, int i, HDC hMemDC, int* m_lastDiffColor, int* m_lastDiffCounter)
{
	long color = 0;
	// Choose a pencil to draw with
	switch (resultsDoc)
	{
    case MARKER_BLANK_LINE:   color = *m_lastDiffColor = m_BlankColor;   *m_lastDiffCounter = m_minimumDiffHeight; break;
    case MARKER_ADDED_LINE:   color = *m_lastDiffColor = m_AddedColor;   *m_lastDiffCounter = m_minimumDiffHeight; break;
    case MARKER_CHANGED_LINE: color = *m_lastDiffColor = m_ChangedColor; *m_lastDiffCounter = m_minimumDiffHeight; break;
    case MARKER_MOVED_LINE:   color = *m_lastDiffColor = m_MovedColor;   *m_lastDiffCounter = m_minimumDiffHeight; break;
    case MARKER_REMOVED_LINE: color = *m_lastDiffColor = m_DeletedColor; *m_lastDiffCounter = m_minimumDiffHeight; break;
	default:
		if (*m_lastDiffCounter)
		{
			(*m_lastDiffCounter)--;
			color = *m_lastDiffColor;
		}
		else
		{
			color = m_DefaultColor;
		}
		break;
	}
	// Draw line for the first document
	SetPixel(hMemDC, 0, i, color);
}

void NavDialog::SetScalingFactor(HWND hWnd)
{
    // Get max file length
    m_LineCount1 = SendMessage(_nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0);
    m_LineCount2 = SendMessage(_nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0);
    m_DocLineCount = max(m_LineCount1, m_LineCount2);
    // And one with adjusted line count (differs when we're in wrapped mode)
    m_VisibleLineCount = max(
        SendMessage(_nppData._scintillaSecondHandle, SCI_VISIBLEFROMDOCLINE, m_LineCount1, 0),
        SendMessage(_nppData._scintillaSecondHandle, SCI_VISIBLEFROMDOCLINE, m_LineCount2, 0));

    GetClientRect(hWnd, &m_SideBarRect);
    m_SideBarPartWidth = ((m_SideBarRect.right - m_SideBarRect.left) - 3 * SPACE) / 2;
    m_SideBarPartHeight = (m_SideBarRect.bottom - m_SideBarRect.top) - 2 * SPACE;

    m_ScaleFactorDocLines = (double)(m_DocLineCount) / (double)(m_SideBarPartHeight);
    m_ScaleFactorVisibleLines = (double)(m_VisibleLineCount) / (double)(m_SideBarPartHeight);

    m_minimumDiffHeight = int((double)MIN_DIFF_HEIGHT / ((double)(m_SideBarPartHeight) / (double)(m_DocLineCount)));
}

void NavDialog::CreateBitmap(void)
{
	SetScalingFactor(_hSelf);

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
    m_lastDiffCounterLeft = 0;
    m_lastDiffCounterRight = 0;
	m_lastDiffColorLeft = m_DefaultColor;
	m_lastDiffColorRight = m_DefaultColor;
    for (int i = 0; i < max(m_DocLineCount - m_minimumDiffHeight, 1); i++)
	{
		SetLinePixel(m_ResultsDoc1[i], i, m_hMemDC1, &m_lastDiffColorLeft, &m_lastDiffCounterLeft);
        SetLinePixel(m_ResultsDoc2[i], i, m_hMemDC2, &m_lastDiffColorRight, &m_lastDiffCounterRight);
	}
    for (int i = m_DocLineCount - 1; i >= max(m_DocLineCount - m_minimumDiffHeight, 1); i--)
	{
        SetLinePixel(m_ResultsDoc1[i], i, m_hMemDC1, &m_lastDiffColorLeft, &m_lastDiffCounterLeft);
        SetLinePixel(m_ResultsDoc2[i], i, m_hMemDC2, &m_lastDiffColorRight, &m_lastDiffCounterRight);
	}

	InvalidateRect(_hSelf, NULL, TRUE);

	DeleteObject(hBrush);
}

LRESULT NavDialog::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;

	// Get current DC
	m_hdc = ::BeginPaint(hWnd, &ps);

	// Draw bars
	SetScalingFactor(hWnd);

	// If side bar is too small, don't draw anything
    if ((m_SideBarPartWidth < 5) || (m_SideBarPartHeight < 5)) return false;

	// Define left rectangle coordinates
	m_rLeft.top    = SPACE;
	m_rLeft.left   = SPACE;
    m_rLeft.right = m_rLeft.left + m_SideBarPartWidth;
    m_rLeft.bottom = m_rLeft.top + m_SideBarPartHeight;

	// Define right rectangle coordinates
	m_rRight.top    = SPACE;
	m_rRight.left   = m_rLeft.right + SPACE;
    m_rRight.right = m_rRight.left + m_SideBarPartWidth;
    m_rRight.bottom = m_rRight.top + m_SideBarPartHeight;

	//SetStretchBltMode(m_hdc, HALFTONE);

	int x, y, cx, cy;

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

    // Current doc view

    int firstVisibleLine = SendMessageA(_nppData._scintillaMainHandle, SCI_GETFIRSTVISIBLELINE, 0, 0);
    int firstDocLine = SendMessageA(_nppData._scintillaMainHandle, SCI_DOCLINEFROMVISIBLE, firstVisibleLine, 0);
    int firstDocLineScaled = (int)((double)firstDocLine / m_ScaleFactorDocLines);
    int linesOnScreen = SendMessageA(_nppData._scintillaMainHandle, SCI_LINESONSCREEN, 0, 0);
    int linesOnScreenScaled = (int)((double)linesOnScreen / m_ScaleFactorVisibleLines);

    x = m_rLeft.left - 1;
    y = max(firstDocLineScaled, SPACE); // don't exceed the top border
    cx = m_rRight.right + 1;
    cy = y + linesOnScreenScaled;
    if (cy > m_rLeft.bottom)
    {
        // preserve the minimum height without exceeding the bottom border
        cy = m_rLeft.bottom;
        y = cy - linesOnScreenScaled;
    }
    cy = max(cy, y + MIN_SELECTOR_HEIGHT); // not too small or even invisible

    HPEN hPenView = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HBRUSH hBrushView = CreateHatchBrush(HS_BDIAGONAL, RGB(255, 0, 0));
    SelectObject(m_hdc, hPenView);
    SelectObject(m_hdc, hBrushView);
    int oldBkMode = GetBkMode(m_hdc);
    SetBkMode(m_hdc, TRANSPARENT);
    Rectangle(m_hdc, x, y, cx, cy);
    SetBkMode(m_hdc, oldBkMode);
    DeleteObject(hPenView);
    DeleteObject(hBrushView);

	::EndPaint(hWnd, &ps);

	return 0;
}

void NavDialog::DrawView()
{
	InvalidateRect(_hSelf, NULL, TRUE);
}
