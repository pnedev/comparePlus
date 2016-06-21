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

#pragma comment (lib, "msimg32")


#include "NavDialog.h"
#include "NppHelpers.h"
#include "resource.h"
#include <windowsx.h>
#include <commctrl.h>


const int NavDialog::cSpace = 2;
const int NavDialog::cScrollerWidth = 15;


void NavDialog::NavView::init(HDC hDC)
{
	// Create bitmaps used to store graphical representation
	m_hViewDC	= ::CreateCompatibleDC(hDC);
	m_hSelDC	= ::CreateCompatibleDC(hDC);

	m_lines	= ::SendMessage(m_hView, SCI_GETLINECOUNT, 0, 0);

	m_hViewBMP	= ::CreateCompatibleBitmap(hDC, 1, m_lines);
	m_hSelBMP	= ::CreateCompatibleBitmap(hDC, 1, 1);

	BITMAP bmpInfo;
	::GetObject(m_hViewBMP, sizeof(bmpInfo), &bmpInfo);
	::GetObject(m_hSelBMP, sizeof(bmpInfo), &bmpInfo);

	// Attach bitmaps to the DC
	::SelectObject(m_hViewDC, m_hViewBMP);
	::SelectObject(m_hSelDC, m_hSelBMP);
}


void NavDialog::NavView::reset()
{
	if (m_hViewDC)
	{
		::DeleteDC(m_hViewDC);
		m_hViewDC = NULL;
	}

	if (m_hSelDC)
	{
		::DeleteDC(m_hSelDC);
		m_hSelDC = NULL;
	}

	if (m_hViewBMP)
	{
		::DeleteObject(m_hViewBMP);
		m_hViewBMP = NULL;
	}

	if (m_hSelBMP)
	{
		::DeleteObject(m_hSelBMP);
		m_hSelBMP = NULL;
	}
}


void NavDialog::NavView::create(const ColorSettings& colors)
{
	RECT bmpRect = { 0 };

	bmpRect.right = 1;
	bmpRect.bottom = 1;

	HBRUSH hBrush = ::CreateSolidBrush(RGB(0, 0, 0));

	::FillRect(m_hSelDC, &bmpRect, hBrush);

	::DeleteObject(hBrush);

	bmpRect.right = 1;
	bmpRect.bottom = m_lines;

	hBrush = ::CreateSolidBrush(colors._default);

	::FillRect(m_hViewDC, &bmpRect, hBrush);

	::DeleteObject(hBrush);

	for (int i = 0; i < m_lines; ++i)
	{
		int marker = ::SendMessage(m_hView, SCI_MARKERGET, i, 0);
		if (!marker)
			continue;

		if      (marker & (1 << MARKER_BLANK_LINE))   marker = colors.blank;
		else if (marker & (1 << MARKER_ADDED_LINE))   marker = colors.added;
		else if (marker & (1 << MARKER_CHANGED_LINE)) marker = colors.changed;
		else if (marker & (1 << MARKER_MOVED_LINE))   marker = colors.moved;
		else if (marker & (1 << MARKER_REMOVED_LINE)) marker = colors.deleted;
		else
			continue;

		::SetPixel(m_hViewDC, 0, i, marker);
	}
}


void NavDialog::NavView::paint(HDC hDC, int xPos, int yPos, int width, int height, int hScale, int hOffset)
{
	height = _MIN((m_lines - hOffset) * hScale, height);
	if (height <= 0)
		return;

	RECT r;
	r.left		= xPos;
	r.top		= yPos;
	r.right		= r.left + width + 2;
	r.bottom	= r.top + height + 2;

	// Draw view border
	::Rectangle(hDC, r.left, r.top, r.right, r.bottom);

	// Fill view
	::StretchBlt(hDC, r.left + 1, r.top + 1, width, height, m_hViewDC, 0, hOffset, 1, height / hScale, SRCCOPY);

	int firstLine	= ::SendMessage(m_hView, SCI_DOCLINEFROMVISIBLE, m_firstLine, 0);
	int lastLine	= m_firstLine + ::SendMessage(m_hView, SCI_LINESONSCREEN, 0, 0);

	lastLine	= ::SendMessage(m_hView, SCI_DOCLINEFROMVISIBLE, lastLine, 0);

	firstLine	*= hScale;
	lastLine	*= hScale;

	r.top		= firstLine + yPos - hOffset;
	r.bottom	= lastLine + yPos - hOffset + 2;

	::Rectangle(hDC, r.left, r.top, r.right, r.bottom);

	BLENDFUNCTION blend = { 0 };
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 20;

	::AlphaBlend(hDC, r.left + 1, r.top + 1, width, lastLine - firstLine, m_hSelDC, 0, 0, 1, 1, blend);
}


bool NavDialog::NavView::updateFirstLine()
{
	const int firstLine = ::SendMessage(m_hView, SCI_GETFIRSTVISIBLELINE, 0, 0);

	if (firstLine != m_firstLine)
	{
		m_firstLine = firstLine;
		return true;
	}

	return false;
}


NavDialog::NavDialog() : DockingDlgInterface(IDD_NAV_DIALOG),
	m_hScroll(NULL), m_mouseOver(false)
{
	_data.hIconTab = NULL;
}


NavDialog::~NavDialog()
{
	Hide();

	if (_data.hIconTab)
		::DestroyIcon(_data.hIconTab);
}


void NavDialog::init(HINSTANCE hInst)
{
	m_hInst = hInst;

	DockingDlgInterface::init(hInst, nppData._nppHandle);

	m_view[0].m_hView = nppData._scintillaMainHandle;
	m_view[1].m_hView = nppData._scintillaSecondHandle;
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

	if (show)
		Show();
	else
		Hide();
}


void NavDialog::SetColors(const ColorSettings& colorSettings)
{
	m_clr = colorSettings;

	if (isVisible())
		CreateBitmap();
}


void NavDialog::Update()
{
	if (!isVisible())
		return;

	// Bitmap needs to be recreated
	if ((m_view[0].m_lines != ::SendMessage(m_view[0].m_hView, SCI_GETLINECOUNT, 0, 0)) ||
		(m_view[1].m_lines != ::SendMessage(m_view[1].m_hView, SCI_GETLINECOUNT, 0, 0)))
		Show();
	else if (m_view[0].updateFirstLine() || m_view[1].updateFirstLine())
		::InvalidateRect(_hSelf, NULL, FALSE);
}


void NavDialog::Show()
{
	HWND hwnd = ::GetFocus();

	// Free resources if needed
	Hide();

	HDC hDC = ::GetDC(_hSelf);

	m_view[0].init(hDC);
	m_view[1].init(hDC);

	// Release DC
	::ReleaseDC(_hSelf, hDC);

	CreateBitmap();

	display(true);

	::SetFocus(hwnd);
}


void NavDialog::Hide()
{
	display(false);

	m_view[0].reset();
	m_view[1].reset();
}


void NavDialog::CreateBitmap()
{
	m_view[0].create(m_clr);
	m_view[1].create(m_clr);

	SetScalingFactor();
}


void NavDialog::ShowScroller(RECT& r)
{
	const int x = r.right - cSpace - cScrollerWidth;
	const int y = cSpace;
	const int w = cScrollerWidth;
	const int h = m_navHeight;

	if (m_hScroll)
		::MoveWindow(m_hScroll, x, y, w, h, TRUE);
	else
		m_hScroll = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY, WC_SCROLLBAR, NULL, WS_CHILD | SBS_VERT,
				x, y, w, h, _hSelf, NULL, m_hInst, NULL);

	SCROLLINFO si = { 0 };
	si.cbSize	= sizeof(si);
	si.fMask	= SIF_RANGE | SIF_PAGE;
	si.nMin		= 0;
	si.nMax		= m_maxLines * m_pixelsPerLine - 1;
	si.nPage	= h;

	::SetScrollInfo(m_hScroll, SB_CTL, &si, TRUE);

	m_navViewWidth = ((r.right - r.left) - 4 * cSpace - cScrollerWidth - 4) / 2;

	::ShowScrollBar(m_hScroll, SB_CTL, TRUE);
}


void NavDialog::SetScalingFactor()
{
	m_view[0].m_lines = ::SendMessage(m_view[0].m_hView, SCI_GETLINECOUNT, 0, 0);
	m_view[1].m_lines = ::SendMessage(m_view[1].m_hView, SCI_GETLINECOUNT, 0, 0);

	m_view[0].m_firstLine = ::SendMessage(m_view[0].m_hView, SCI_GETFIRSTVISIBLELINE, 0, 0);
	m_view[1].m_firstLine = ::SendMessage(m_view[1].m_hView, SCI_GETFIRSTVISIBLELINE, 0, 0);

	m_maxLines = _MAX(m_view[0].m_lines, m_view[1].m_lines);
	m_syncView = (m_maxLines == m_view[0].m_lines) ? &m_view[0] : &m_view[1];

	RECT r;
	::GetClientRect(_hSelf, &r);

	m_navViewWidth = ((r.right - r.left) - 3 * cSpace - 4) / 2;
	m_navHeight = (r.bottom - r.top) - 2 * cSpace - 2;

	m_pixelsPerLine = m_navHeight / m_maxLines;

	if (m_pixelsPerLine == 0)
	{
		m_pixelsPerLine = 1;

		ShowScroller(r);
	}
	else
	{
		if (m_pixelsPerLine > 5)
			m_pixelsPerLine = 5;

		m_navHeight = m_pixelsPerLine * m_maxLines;

		if (m_hScroll)
			::ShowScrollBar(m_hScroll, SB_CTL, FALSE);
	}

	::InvalidateRect(_hSelf, NULL, TRUE);
}


void NavDialog::setPos(int x, int y)
{
	y -= (cSpace + 2);
	if (y < 0 || x < cSpace || x > (2 * m_navViewWidth + 2 * cSpace + 4))
		return;

	HWND currView;

	const int scrollOffset = (m_hScroll && ::IsWindowVisible(m_hScroll)) ? ::GetScrollPos(m_hScroll, SB_CTL) : 0;

	if (x < m_navViewWidth + cSpace + 2)
	{
		if (y > _MIN((m_view[0].m_lines - scrollOffset) * m_pixelsPerLine, m_navHeight))
			return;
		currView = m_view[0].m_hView;
	}
	else
	{
		if (y > _MIN((m_view[1].m_lines - scrollOffset) * m_pixelsPerLine, m_navHeight))
			return;
		currView = m_view[1].m_hView;
	}

	::SetFocus(currView);

	const int currLine = (y + scrollOffset) / m_pixelsPerLine;

	const int linesOnScreen = ::SendMessage(currView, SCI_LINESONSCREEN, 0, 0);
	const int firstLine = ::SendMessage(currView, SCI_VISIBLEFROMDOCLINE, currLine, 0) - linesOnScreen / 2;

	::SendMessage(currView, SCI_ENSUREVISIBLEENFORCEPOLICY, currLine, 0);
	::SendMessage(currView, SCI_SETFIRSTVISIBLELINE, firstLine, 0);
	::SendMessage(currView, SCI_GOTOLINE, currLine, 0);
}


void NavDialog::onMouseWheel(int steps)
{
	const int linesOnScreen = ::SendMessage(m_syncView->m_hView, SCI_LINESONSCREEN, 0, 0);
	int firstLine = m_syncView->m_firstLine;

	firstLine -= steps * linesOnScreen;

	::SendMessage(m_syncView->m_hView, SCI_SETFIRSTVISIBLELINE, firstLine, 0);
}


int NavDialog::updateScroll()
{
	if (m_hScroll && ::IsWindowVisible(m_hScroll))
	{
		const int linesOnScreen = ::SendMessage(m_syncView->m_hView, SCI_LINESONSCREEN, 0, 0);

		int currentScroll = ::GetScrollPos(m_hScroll, SB_CTL);

		if (currentScroll > m_syncView->m_firstLine)
		{
			currentScroll -= m_navHeight;

			if (currentScroll < 0)
				currentScroll = 0;

			::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
		}
		else if (currentScroll + m_navHeight - 1 < m_syncView->m_firstLine + linesOnScreen)
		{
			currentScroll += m_navHeight;

			if (currentScroll > m_maxLines * m_pixelsPerLine - 1)
				currentScroll = m_maxLines * m_pixelsPerLine - m_navHeight;

			::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
		}

		return currentScroll;
	}

	return 0;
}


void NavDialog::onPaint()
{
	// If side bar is too small, don't draw anything
	if ((m_navViewWidth < 5) || (m_navHeight < 5))
		return;

	const int scrollOffset = updateScroll();

	HPEN hPenView = ::CreatePen(PS_SOLID, 1, RGB(180, 180, 180));

	PAINTSTRUCT ps;

	// Get current DC
	HDC hDC = ::BeginPaint(_hSelf, &ps);

	::SelectObject(hDC, hPenView);
	::SelectObject(hDC, ::GetStockObject(NULL_BRUSH));

	if (m_view[0].m_lines > scrollOffset)
		m_view[0].paint(hDC, cSpace, cSpace, m_navViewWidth, m_navHeight, m_pixelsPerLine, scrollOffset);
	else
		ps.fErase = TRUE;

	if (m_view[1].m_lines > scrollOffset)
		m_view[1].paint(hDC, m_navViewWidth + 2 * cSpace + 2, cSpace,
				m_navViewWidth, m_navHeight, m_pixelsPerLine, scrollOffset);
	else
		ps.fErase = TRUE;

	::DeleteObject(hPenView);

	::EndPaint(_hSelf, &ps);
}


BOOL CALLBACK NavDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		break;

		case WM_SIZE:
		case WM_MOVE:
			if (isVisible())
				SetScalingFactor();
		break;

		case WM_PAINT:
			onPaint();
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
			setPos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

		case WM_LBUTTONUP:
			::ReleaseCapture();
		break;

		case WM_MOUSEMOVE:
			if (::GetCapture() == _hSelf)
				setPos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			else if (!m_mouseOver)
				m_mouseOver = true;
		break;

		case WM_MOUSELEAVE:
			if (m_mouseOver)
				m_mouseOver = false;
		break;

		case WM_MOUSEWHEEL:
			if (m_mouseOver)
				onMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / 120);
		break;

		default:
			return DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
	}

	return FALSE;
}
