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
	//(doc1 > doc2) ? (m_TextLength = doc1) : (m_TextLength = doc2);
	m_TextLength = max(doc1, doc2);

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
			//return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
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

		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
			yPos = HIWORD(lParam);
			if (yPos < 0) yPos = 0;
			current_line = (long)((double)yPos * m_ScaleFactor);
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
			if (GetCapture() == hWnd) 
			{ 
				yPos = HIWORD(lParam);
				if (yPos < 0) yPos = 0;
				const long next_line = (const long)((double)yPos * m_ScaleFactor);
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
	m_DefaultColor = GetSysColor(COLOR_ACTIVECAPTION);
}

void NavDialog::SetLinePixel(long resultsDoc, int i, HDC hMemDC, int* m_lastDiffColor)
{
	long color = 0;
	// Choose a pencil to draw with
	switch (resultsDoc)
	{
	case MARKER_BLANK_LINE:   color = *m_lastDiffColor = m_BlankColor;   m_lastDiffCounter = m_lastDiffCounterInit; break;
	case MARKER_ADDED_LINE:   color = *m_lastDiffColor = m_AddedColor;   m_lastDiffCounter = m_lastDiffCounterInit; break;
	case MARKER_CHANGED_LINE: color = *m_lastDiffColor = m_ChangedColor; m_lastDiffCounter = m_lastDiffCounterInit; break;
	case MARKER_MOVED_LINE:   color = *m_lastDiffColor = m_MovedColor;   m_lastDiffCounter = m_lastDiffCounterInit; break;
	case MARKER_REMOVED_LINE: color = *m_lastDiffColor = m_DeletedColor; m_lastDiffCounter = m_lastDiffCounterInit; break;
	default:
		if (m_lastDiffCounter)
		{
			m_lastDiffCounter--;
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
	GetClientRect(hWnd, &m_scaleRect);
	m_SzX = ((m_scaleRect.right - m_scaleRect.left) - 3 * SPACE) / 2;
	m_SzY = (m_scaleRect.bottom - m_scaleRect.top) - 2 * SPACE;
	m_ScaleFactor = (double)(m_TextLength) / (double)(m_SzY);
	m_lastDiffCounterInit = min(MIN_DIFF_HEIGHT, int((double)MIN_DIFF_HEIGHT * m_ScaleFactor));
}

void NavDialog::CreateBitmap(void)
{
	SetScalingFactor(m_hWnd);

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
	m_lastDiffCounter = 0;
	m_lastDiffColorLeft = m_DefaultColor;
	m_lastDiffColorRight = m_DefaultColor;
	for (int i = 0; i < max(m_TextLength - MIN_DIFF_HEIGHT, 1); i++)
	{
		SetLinePixel(m_ResultsDoc1[i], i, m_hMemDC1, &m_lastDiffColorLeft);
		SetLinePixel(m_ResultsDoc2[i], i, m_hMemDC2, &m_lastDiffColorRight);
	}
	for (int i = m_TextLength - 1; i >= max(m_TextLength - MIN_DIFF_HEIGHT, 1); i--)
	{
		SetLinePixel(m_ResultsDoc1[i], i, m_hMemDC1, &m_lastDiffColorLeft);
		SetLinePixel(m_ResultsDoc2[i], i, m_hMemDC2, &m_lastDiffColorRight);
	}

	InvalidateRect(m_hWnd, NULL, TRUE);

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
	if ((m_SzX < 5) || (m_SzY < 5)) return false;

	// Define left rectangle coordinates
	m_rLeft.top    = SPACE;    
	m_rLeft.left   = SPACE;
	m_rLeft.right = m_rLeft.left + m_SzX;
	m_rLeft.bottom = m_rLeft.top + m_SzY;

	// Define right rectangle coordinates
	m_rRight.top    = SPACE;
	m_rRight.left   = m_rLeft.right + SPACE;
	m_rRight.right = m_rRight.left + m_SzX;
	m_rRight.bottom = m_rRight.top + m_SzY;

	SetStretchBltMode(m_hdc, WHITEONBLACK);

	int x, y, cx, cy;

	// Current doc view

	x  = m_scaleRect.left;
	y  = m_scaleRect.top + SPACE;
	cx = m_scaleRect.right - x;
	cy = m_scaleRect.bottom - y - SPACE;

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

	for(long i = start; i <= max(end, start + MIN_DIFF_HEIGHT); i++)
		SetPixel(m_hMemDCView, 0, i, RGB(255,0,0));

	InvalidateRect(m_hWnd, NULL, TRUE);

	DeleteObject(hBrush);
}