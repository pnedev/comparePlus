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
#include "NppHelpers.h"
#include "resource.h"
#include <windowsx.h>


const int NavDialog::cSpace = 2;
const int NavDialog::cMinSelectorHeight = 5;


NavDialog::NavDialog() : DockingDlgInterface(IDD_NAV_DIALOG),
	m_hMemDC1(NULL), m_hMemDC2(NULL), m_hMemBMP1(NULL), m_hMemBMP2(NULL), m_mouseOver(false)
{
	_data.hIconTab = NULL;
}


NavDialog::~NavDialog()
{
	Hide();

	if (_data.hIconTab)
		DestroyIcon(_data.hIconTab);
}


void NavDialog::init(HINSTANCE hInst)
{
	DockingDlgInterface::init(hInst, nppData._nppHandle);
}


void NavDialog::doDialog(bool show)
{
	if (!isCreated())
	{
		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		_data.pszName       = TEXT("Compare NavBar");
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= CMD_NAV_BAR;
		_data.hIconTab		= (HICON)::LoadImage(GetModuleHandle(TEXT("ComparePlugin.dll")),
				MAKEINTRESOURCE(IDB_ICON), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);

		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
	}

	// Display
	display(show);

	if (show)
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

	m_hMemBMP1	= ::CreateCompatibleBitmap(hDC, 1, m_NavHeight);
	m_hMemBMP2	= ::CreateCompatibleBitmap(hDC, 1, m_NavHeight);

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


void NavDialog::scrollView(int x, int y)
{
	y -= cSpace;
	if (y < 0 || y > m_NavHeight)
		return;

	HWND currView;

	if (cSpace < x && x < m_NavHalfWidth + cSpace)
		currView = nppData._scintillaMainHandle;
	else
		currView = nppData._scintillaSecondHandle;

	::SetFocus(currView);

	const int currLine = (int)((float)y * m_HeightScaleFactor);

	const int linesOnScreen = ::SendMessage(currView, SCI_LINESONSCREEN, 0, 0);
	const int firstVisibleLine = ::SendMessage(currView, SCI_VISIBLEFROMDOCLINE, currLine, 0) - linesOnScreen / 2;

	::SendMessage(currView, SCI_ENSUREVISIBLEENFORCEPOLICY, currLine, 0);
	::SendMessage(currView, SCI_SETFIRSTVISIBLELINE, firstVisibleLine, 0);
	::SendMessage(currView, SCI_GOTOLINE, currLine, 0);
}


void NavDialog::onMouseWheel(int delta)
{
	HWND currView = getCurrentView();

	const int firstVisibleLine = ::SendMessage(currView, SCI_GETFIRSTVISIBLELINE, 0, 0);
	int currLine = ::SendMessage(currView, SCI_GETCURRENTPOS, 0, 0);
	currLine = ::SendMessage(currView, SCI_POSITIONFROMLINE, currLine, 0);
	::SendMessage(currView, SCI_LINESCROLL, 0, firstVisibleLine - currLine - delta * 3);
}


BOOL CALLBACK NavDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
			::SetClassLong(_hSelf, GCL_STYLE, CS_HREDRAW | CS_VREDRAW | CS_PARENTDC);
		break;

		case WM_PAINT:
			OnPaint();
		break;

		case WM_NOTIFY:
		{
			LPNMHDR	pnmh = (LPNMHDR)lParam;

			if (pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_CLOSE)
				ViewNavigationBar();
		}
		break;

		case WM_LBUTTONDOWN:
			::SetCapture(_hSelf);
			scrollView(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

		case WM_LBUTTONUP:
			::ReleaseCapture();
		break;

		case WM_MOUSEMOVE:
			if (::GetCapture() == _hSelf)
				scrollView(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			else if (!m_mouseOver)
				m_mouseOver = true;
		break;

		case WM_MOUSELEAVE:
			if (m_mouseOver)
				m_mouseOver = false;
		break;

		case WM_MOUSEWHEEL:
			// if (m_mouseOver)
				// onMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / 120);
		break;

		default:
			return DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
	}

	return FALSE;
}


void NavDialog::SetColors(const ColorSettings& colorSettings)
{
	_clr = colorSettings;
}


void NavDialog::SetDocNavLine(int lineMark, int i, HDC hMemDC)
{
	int color;

	switch (lineMark)
	{
		case MARKER_BLANK_LINE:		color = _clr.blank;		break;
		case MARKER_ADDED_LINE:		color = _clr.added;		break;
		case MARKER_CHANGED_LINE:	color = _clr.changed;	break;
		case MARKER_MOVED_LINE:		color = _clr.moved;		break;
		case MARKER_REMOVED_LINE:	color = _clr.deleted;	break;
		default:					color = _clr._default;
	}

	i *= m_bmpLineHeight;
	for (int j = i + m_bmpLineHeight; j >= i; --j)
		::SetPixel(hMemDC, 0, j, color);
}


void NavDialog::SetScalingFactor()
{
    // Get max file length
    m_LineCount1 = ::SendMessage(nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0);
    m_LineCount2 = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0);

    m_MaxLineCount = _MAX(m_LineCount1, m_LineCount2);

	RECT navRect;
    ::GetClientRect(_hSelf, &navRect);

    m_NavHalfWidth = ((navRect.right - navRect.left) - 3 * cSpace) / 2;
    m_NavHeight = (navRect.bottom - navRect.top) - 2 * cSpace;

    m_bmpLineHeight = m_NavHeight / m_MaxLineCount;

	if (m_bmpLineHeight > 5)
		m_bmpLineHeight = 5;

	if (m_bmpLineHeight)
		m_NavHeight = m_bmpLineHeight * m_MaxLineCount;

    m_HeightScaleFactor = (float)(m_MaxLineCount) / (float)(m_NavHeight);
}


void NavDialog::CreateBitmap()
{
	// Create line arrays
	std::vector<int> doc1LineMark(m_MaxLineCount, -1);
	std::vector<int> doc2LineMark(m_MaxLineCount, -1);

    for (int i = 0; i < m_LineCount1; ++i)
	{
		int marker = SendMessage(nppData._scintillaMainHandle, SCI_MARKERGET, i, 0);
		if (!marker)
			continue;

		if      (marker & (1 << MARKER_BLANK_LINE))   doc1LineMark[i] = MARKER_BLANK_LINE;
		else if (marker & (1 << MARKER_ADDED_LINE))   doc1LineMark[i] = MARKER_ADDED_LINE;
		else if (marker & (1 << MARKER_CHANGED_LINE)) doc1LineMark[i] = MARKER_CHANGED_LINE;
		else if (marker & (1 << MARKER_MOVED_LINE))   doc1LineMark[i] = MARKER_MOVED_LINE;
		else if (marker & (1 << MARKER_REMOVED_LINE)) doc1LineMark[i] = MARKER_REMOVED_LINE;
	}

    for (int i = 0; i < m_LineCount2; ++i)
	{
		int marker = SendMessage(nppData._scintillaSecondHandle, SCI_MARKERGET, i, 0);
		if (!marker)
			continue;

		if      (marker & (1 << MARKER_BLANK_LINE))   doc2LineMark[i] = MARKER_BLANK_LINE;
		else if (marker & (1 << MARKER_ADDED_LINE))   doc2LineMark[i] = MARKER_ADDED_LINE;
		else if (marker & (1 << MARKER_CHANGED_LINE)) doc2LineMark[i] = MARKER_CHANGED_LINE;
		else if (marker & (1 << MARKER_MOVED_LINE))   doc2LineMark[i] = MARKER_MOVED_LINE;
		else if (marker & (1 << MARKER_REMOVED_LINE)) doc2LineMark[i] = MARKER_REMOVED_LINE;
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

    for (int i = 0; i < m_MaxLineCount; ++i)
	{
		SetDocNavLine(doc1LineMark[i], i, m_hMemDC1);
        SetDocNavLine(doc2LineMark[i], i, m_hMemDC2);
	}

	InvalidateRect(_hSelf, NULL, TRUE);

	::DeleteObject(hBrush);
}


void NavDialog::OnPaint()
{
	SetScalingFactor();

	// If side bar is too small, don't draw anything
    if ((m_NavHalfWidth < 5) || (m_NavHeight < 5))
		return;

	PAINTSTRUCT ps;

	// Get current DC
	HDC hDC = ::BeginPaint(_hSelf, &ps);

	// Define left rectangle coordinates
	RECT rLeft;
	rLeft.top		= cSpace;
	rLeft.left		= cSpace;
    rLeft.right		= rLeft.left + m_NavHalfWidth;
    rLeft.bottom	= rLeft.top + m_NavHeight;

	// Define right rectangle coordinates
	RECT rRight;
	rRight.top		= cSpace;
	rRight.left		= rLeft.right + cSpace;
    rRight.right	= rRight.left + m_NavHalfWidth;
    rRight.bottom	= rRight.top + m_NavHeight;

	int x, y, cx, cy;

	// Draw bar border
    ::Rectangle(hDC, rLeft.left, rLeft.top, rLeft.right, rLeft.bottom);
    ::Rectangle(hDC, rRight.left, rRight.top, rRight.right, rRight.bottom);

	// Draw Left bar
	x  = rLeft.left + 1;
	y  = rLeft.top + 1;
	cx = rLeft.right - x - 1;
	cy = rLeft.bottom - y - 1;

	::StretchBlt(hDC, x, y, cx, cy, m_hMemDC1, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

	// Draw Right bar
	x  = rRight.left + 1;
	y  = rRight.top + 1;
	cx = rRight.right - x - 1;
	cy = rRight.bottom - y - 1;

	::StretchBlt(hDC, x, y, cx, cy, m_hMemDC2, 0, 0, m_hMemBMPSize.cx, m_hMemBMPSize.cy, SRCCOPY);

	HWND currView = getCurrentView();

    int firstLineOnScreen = ::SendMessage(currView, SCI_GETFIRSTVISIBLELINE, 0, 0);
    int lastLineOnScreen = firstLineOnScreen + ::SendMessage(currView, SCI_LINESONSCREEN, 0, 0);

    firstLineOnScreen = ::SendMessage(currView, SCI_DOCLINEFROMVISIBLE, firstLineOnScreen, 0);
    lastLineOnScreen = ::SendMessage(currView, SCI_DOCLINEFROMVISIBLE, lastLineOnScreen, 0);

    firstLineOnScreen = (int)((float)firstLineOnScreen / m_HeightScaleFactor);
    lastLineOnScreen = (int)((float)lastLineOnScreen / m_HeightScaleFactor);

    x = rLeft.left - 1;
    y = _MAX(firstLineOnScreen, cSpace); // don't exceed the top border
    cx = rRight.right + 1;
    cy = y + (lastLineOnScreen - firstLineOnScreen);

	// preserve the minimum height without exceeding the bottom border
    if (cy > rLeft.bottom)
        cy = rLeft.bottom;

	// not too small or even invisible
	if (cy < y + cMinSelectorHeight)
		cy = y + cMinSelectorHeight;

    HPEN hPenView = ::CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HBRUSH hBrushView = ::CreateHatchBrush(HS_VERTICAL, RGB(200, 200, 200));

    ::SelectObject(hDC, hPenView);
    ::SelectObject(hDC, hBrushView);

    int oldBkMode = ::GetBkMode(hDC);

    ::SetBkMode(hDC, TRANSPARENT);

    ::Rectangle(hDC, x, y, cx, cy);

    ::SetBkMode(hDC, oldBkMode);

    ::DeleteObject(hPenView);
    ::DeleteObject(hBrushView);

	::EndPaint(_hSelf, &ps);
}


void NavDialog::Update()
{
	InvalidateRect(_hSelf, NULL, TRUE);
}
