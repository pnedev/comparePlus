/*
 * This file is part of Compare Plugin for Notepad++
 * Copyright (C) 2009
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <vector>
#include "NavDialog.h"
#include "NPPHelpers.h"
#include "resource.h"


const int NavDialog::cSpace = 2;
const int NavDialog::cMinSelectorHeight = 5;


NavDialog::NavDialog() : DockingDlgInterface(IDD_NAV_DIALOG),
	m_hMemDC1(NULL), m_hMemDC2(NULL), m_hMemBMP1(NULL), m_hMemBMP2(NULL)
{
	_data.hIconTab = NULL;
}


NavDialog::~NavDialog()
{
	if (_data.hIconTab)
		DestroyIcon(_data.hIconTab);
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
		_data.uMask			= DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		_data.pszName       = TEXT("Compare NavBar");
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= CMD_USE_NAV_BAR;
		_data.hIconTab		= (HICON)::LoadImage(GetModuleHandle(TEXT("ComparePlugin.dll")),
				MAKEINTRESOURCE(IDB_ICON), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);

		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
	}

	// Display
	display(willBeShown);

	if (willBeShown)
		Show();
	else
		Hide();
}


void NavDialog::Show()
{
	// Free resources if needed
	Hide();

	HDC hDC = ::GetDC(_hSelf);

    SetScalingFactor();

	// Create BMP used to store graphical representation
	m_hMemDC1	= ::CreateCompatibleDC(hDC);
	m_hMemDC2	= ::CreateCompatibleDC(hDC);

	m_hMemBMP1	= ::CreateCompatibleBitmap(hDC, 1, m_DocLineCount);
	m_hMemBMP2	= ::CreateCompatibleBitmap(hDC, 1, m_DocLineCount);

	// Retrieve created BMP info (BMP1 == BMP2)
	BITMAP hMemBMPInfo;
	::GetObject(m_hMemBMP1, sizeof(hMemBMPInfo), &hMemBMPInfo);

	m_hMemBMPSize.cx = hMemBMPInfo.bmWidth;
	m_hMemBMPSize.cy = hMemBMPInfo.bmHeight;

	// Attach BMP to a DC
	::SelectObject(m_hMemDC1, m_hMemBMP1);
	::SelectObject(m_hMemDC2, m_hMemBMP2);

	// Release DC
	::ReleaseDC(_hSelf, hDC);

	CreateBitmap();
}


void NavDialog::Hide()
{
	// Delete objects
	if (m_hMemDC1)
	{
		::DeleteDC(m_hMemDC1);
		m_hMemDC1 = NULL;
	}

	if (m_hMemDC2)
	{
		::DeleteDC(m_hMemDC2);
		m_hMemDC2 = NULL;
	}

	if (m_hMemBMP1)
	{
		::DeleteObject(m_hMemBMP1);
		m_hMemBMP1 = NULL;
	}

	if (m_hMemBMP2)
	{
		::DeleteObject(m_hMemBMP2);
		m_hMemBMP2 = NULL;
	}
}


void NavDialog::scrollView(short yPos)
{
	if (yPos < 0)
		yPos = 0;

	long current_line = (long)((double)yPos * m_ScaleFactorDocLines);

	HWND curView = getCurrentView();
	int LineVisible = SendMessage(curView, SCI_LINESONSCREEN, 0, 0);
	int LineStart = SendMessage(curView, SCI_GETFIRSTVISIBLELINE, 0, 0);
	int Delta = current_line - LineVisible / 2 - LineStart;

	SendMessage(curView, SCI_LINESCROLL, 0, (LPARAM)Delta);
	SendMessage(curView, SCI_GOTOLINE, (WPARAM)current_line, 0);
}


BOOL CALLBACK NavDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
			// Here, I modify window styles (set H and V redraw)
			SetClassLong(_hSelf, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);
		break;

		case WM_PAINT:
			OnPaint();
		break;

		case WM_DESTROY:
			Hide();
			::PostQuitMessage(0);
		break;

		case WM_LBUTTONDOWN:
			::SetCapture(_hSelf);
			scrollView(HIWORD(lParam));
		break;

		case WM_LBUTTONUP:
			::ReleaseCapture();
		break;

		case WM_MOUSEMOVE:
			if (::GetCapture() == _hSelf)
				scrollView(HIWORD(lParam));
		break;

		default:
			return DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
	}

	return FALSE;
}


void NavDialog::SetColor(const sColorSettings& colorSettings)
{
	_clr = colorSettings;
}


void NavDialog::SetLinePixel(long resultsDoc, int i, HDC hMemDC, int* lastDiffColor, int* lastDiffCounter)
{
	long color = _clr._default;

	// Choose a pencil to draw with
	switch (resultsDoc)
	{
		case MARKER_BLANK_LINE:   color = *lastDiffColor = _clr.blank;   *lastDiffCounter = m_minimumDiffHeight; break;
		case MARKER_ADDED_LINE:   color = *lastDiffColor = _clr.added;   *lastDiffCounter = m_minimumDiffHeight; break;
		case MARKER_CHANGED_LINE: color = *lastDiffColor = _clr.changed; *lastDiffCounter = m_minimumDiffHeight; break;
		case MARKER_MOVED_LINE:   color = *lastDiffColor = _clr.moved;   *lastDiffCounter = m_minimumDiffHeight; break;
		case MARKER_REMOVED_LINE: color = *lastDiffColor = _clr.deleted; *lastDiffCounter = m_minimumDiffHeight; break;

		default:
			if (*lastDiffCounter)
			{
				--(*lastDiffCounter);
				color = *lastDiffColor;
			}
		break;
	}

	// Draw line for the first document
	::SetPixel(hMemDC, 0, i, color);
}


void NavDialog::SetScalingFactor()
{
    // Get max file length
    m_LineCount1 = SendMessage(_nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0);
    m_LineCount2 = SendMessage(_nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0);

    m_DocLineCount = _MAX(m_LineCount1, m_LineCount2);

    // And one with adjusted line count (differs when we're in wrapped mode)
    m_VisibleLineCount = _MAX(
        SendMessage(_nppData._scintillaSecondHandle, SCI_VISIBLEFROMDOCLINE, m_LineCount1, 0),
        SendMessage(_nppData._scintillaSecondHandle, SCI_VISIBLEFROMDOCLINE, m_LineCount2, 0));

	RECT sideBarRect;
    ::GetClientRect(_hSelf, &sideBarRect);
    m_SideBarPartWidth = ((sideBarRect.right - sideBarRect.left) - 3 * cSpace) / 2;
    m_SideBarPartHeight = (sideBarRect.bottom - sideBarRect.top) - 2 * cSpace;

    m_ScaleFactorDocLines = (double)(m_DocLineCount) / (double)(m_SideBarPartHeight);
    m_ScaleFactorVisibleLines = (double)(m_VisibleLineCount) / (double)(m_SideBarPartHeight);

    m_minimumDiffHeight = (int)(1 / m_ScaleFactorDocLines);
}


void NavDialog::CreateBitmap()
{
	SetScalingFactor();

	// Create line arrays
	std::vector<long> m_ResultsDoc1(m_DocLineCount, -1);
	std::vector<long> m_ResultsDoc2(m_DocLineCount, -1);

    for (int i = 0; i < m_LineCount1; ++i)
	{
		int marker = SendMessage(_nppData._scintillaMainHandle, SCI_MARKERGET, (WPARAM)i, 0);
		if (!marker)
			continue;

		if      (marker & (1 << MARKER_BLANK_LINE))   m_ResultsDoc1[i] = MARKER_BLANK_LINE;
		else if (marker & (1 << MARKER_ADDED_LINE))   m_ResultsDoc1[i] = MARKER_ADDED_LINE;
		else if (marker & (1 << MARKER_CHANGED_LINE)) m_ResultsDoc1[i] = MARKER_CHANGED_LINE;
		else if (marker & (1 << MARKER_MOVED_LINE))   m_ResultsDoc1[i] = MARKER_MOVED_LINE;
		else if (marker & (1 << MARKER_REMOVED_LINE)) m_ResultsDoc1[i] = MARKER_REMOVED_LINE;
	}

    for (int i = 0; i < m_LineCount2; ++i)
	{
		int marker = SendMessage(_nppData._scintillaSecondHandle, SCI_MARKERGET, (WPARAM)i, 0);
		if (!marker)
			continue;

		if      (marker & (1 << MARKER_BLANK_LINE))   m_ResultsDoc2[i] = MARKER_BLANK_LINE;
		else if (marker & (1 << MARKER_ADDED_LINE))   m_ResultsDoc2[i] = MARKER_ADDED_LINE;
		else if (marker & (1 << MARKER_CHANGED_LINE)) m_ResultsDoc2[i] = MARKER_CHANGED_LINE;
		else if (marker & (1 << MARKER_MOVED_LINE))   m_ResultsDoc2[i] = MARKER_MOVED_LINE;
		else if (marker & (1 << MARKER_REMOVED_LINE)) m_ResultsDoc2[i] = MARKER_REMOVED_LINE;
	}

	// Fill BMP background
	HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

	RECT bmpRect;
	bmpRect.top = 0;
	bmpRect.left = 0;
	bmpRect.right = m_hMemBMPSize.cx;
	bmpRect.bottom = m_hMemBMPSize.cy;

	FillRect(m_hMemDC1, &bmpRect, hBrush);
	FillRect(m_hMemDC2, &bmpRect, hBrush);

    int lastDiffCounterLeft = 0;
    int lastDiffCounterRight = 0;

	int lastDiffColorLeft = _clr._default;
	int lastDiffColorRight = _clr._default;

    for (int i = 0; i < m_DocLineCount; ++i)
	{
		SetLinePixel(m_ResultsDoc1[i], i, m_hMemDC1, &lastDiffColorLeft, &lastDiffCounterLeft);
        SetLinePixel(m_ResultsDoc2[i], i, m_hMemDC2, &lastDiffColorRight, &lastDiffCounterRight);
	}

	InvalidateRect(_hSelf, NULL, TRUE);

	::DeleteObject(hBrush);
}


void NavDialog::OnPaint()
{
	SetScalingFactor();

	// If side bar is too small, don't draw anything
    if ((m_SideBarPartWidth < 5) || (m_SideBarPartHeight < 5))
		return;

	PAINTSTRUCT ps;

	// Get current DC
	HDC hDC = ::BeginPaint(_hSelf, &ps);

	// Define left rectangle coordinates
	m_rLeft.top    = cSpace;
	m_rLeft.left   = cSpace;
    m_rLeft.right = m_rLeft.left + m_SideBarPartWidth;
    m_rLeft.bottom = m_rLeft.top + m_SideBarPartHeight;

	// Define right rectangle coordinates
	m_rRight.top    = cSpace;
	m_rRight.left   = m_rLeft.right + cSpace;
    m_rRight.right = m_rRight.left + m_SideBarPartWidth;
    m_rRight.bottom = m_rRight.top + m_SideBarPartHeight;

	// SetStretchBltMode(hDC, HALFTONE);
	// SetBrushOrgEx(hDC, 0, 0, NULL);

	int x, y, cx, cy;

	// Draw bar border
    Rectangle(hDC, m_rLeft.left, m_rLeft.top, m_rLeft.right, m_rLeft.bottom);
    Rectangle(hDC, m_rRight.left, m_rRight.top, m_rRight.right, m_rRight.bottom);

	// Draw Left bar
	x  = m_rLeft.left + 1;
	y  = m_rLeft.top + 1;
	cx = m_rLeft.right - x - 1;
	cy = m_rLeft.bottom - y - 1;

	StretchBlt(hDC, x, y, cx, cy, m_hMemDC1, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

	// Draw Right bar
	x  = m_rRight.left + 1;
	y  = m_rRight.top + 1;
	cx = m_rRight.right - x - 1;
	cy = m_rRight.bottom - y - 1;

	StretchBlt(hDC, x, y, cx, cy, m_hMemDC2, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

    // Current doc view
    int firstDocLineScaled = SendMessage(_nppData._scintillaMainHandle, SCI_GETFIRSTVISIBLELINE, 0, 0);
    firstDocLineScaled = SendMessage(_nppData._scintillaMainHandle, SCI_DOCLINEFROMVISIBLE, firstDocLineScaled, 0);
    firstDocLineScaled = (int)((double)firstDocLineScaled / m_ScaleFactorDocLines);

    int linesOnScreenScaled = SendMessage(_nppData._scintillaMainHandle, SCI_LINESONSCREEN, 0, 0);
    linesOnScreenScaled = (int)((double)linesOnScreenScaled / m_ScaleFactorVisibleLines);

    x = m_rLeft.left - 1;
    y = _MAX(firstDocLineScaled, cSpace); // don't exceed the top border
    cx = m_rRight.right + 1;
    cy = y + linesOnScreenScaled;

    if (cy > m_rLeft.bottom)
    {
        // preserve the minimum height without exceeding the bottom border
        cy = m_rLeft.bottom;
        y = cy - linesOnScreenScaled;
    }

	// not too small or even invisible
	if (cy < y + cMinSelectorHeight)
		cy = y + cMinSelectorHeight;

    HPEN hPenView = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HBRUSH hBrushView = CreateHatchBrush(HS_VERTICAL, RGB(200, 200, 200));

    ::SelectObject(hDC, hPenView);
    ::SelectObject(hDC, hBrushView);

    int oldBkMode = GetBkMode(hDC);

    SetBkMode(hDC, TRANSPARENT);

    Rectangle(hDC, x, y, cx, cy);

    SetBkMode(hDC, oldBkMode);

    ::DeleteObject(hPenView);
    ::DeleteObject(hBrushView);

	::EndPaint(_hSelf, &ps);
}


void NavDialog::DrawView()
{
	InvalidateRect(_hSelf, NULL, TRUE);
}
