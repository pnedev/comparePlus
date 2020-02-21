/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2009
 * Copyright (C)2017-2019 Pavel Nedev (pg.nedev@gmail.com)
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


#define NOMINMAX

#include "Compare.h"
#include "NavDialog.h"
#include "NppHelpers.h"
#include "resource.h"

#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>


const int NavDialog::cSpace = 2;
const int NavDialog::cScrollerWidth = 15;


void NavDialog::NavView::init(HDC hDC)
{
	// Create bitmaps used to store graphical representation
	m_hViewDC	= ::CreateCompatibleDC(hDC);
	m_hSelDC	= ::CreateCompatibleDC(hDC);

	m_lines	= CallScintilla(m_view, SCI_GETLINECOUNT, 0, 0);

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
	m_lineMap.clear();

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


void NavDialog::NavView::paint(HDC hDC, int xPos, int yPos, int width, int height, int hScale, int hOffset)
{
	const int usefulHeight = (maxBmpLines() - hOffset) * hScale;
	const bool emptyArea = (height - usefulHeight) > 0;

	int h = emptyArea ? usefulHeight : height;
	if (h <= 0)
		return;

	RECT r;
	r.left		= xPos;
	r.top		= yPos;
	r.right		= r.left + width + 2;
	r.bottom	= r.top + h + 2;

	// Draw view border
	::Rectangle(hDC, r.left, r.top, r.right, r.bottom);

	if (emptyArea)
	{
		HBRUSH bkBrush = ::GetSysColorBrush(COLOR_3DFACE);

		RECT bkRect;

		bkRect.left		= r.left;
		bkRect.right	= r.right;
		bkRect.top		= r.bottom;
		bkRect.bottom	= r.top + height + 2;

		::FillRect(hDC, &bkRect, bkBrush);

		::DeleteObject(bkBrush);
	}

	// Fill view
	::StretchBlt(hDC, r.left + 1, r.top + 1, width, h, m_hViewDC, 0, hOffset, 1, h / hScale, SRCCOPY);

	int firstVisible	= CallScintilla(m_view, SCI_DOCLINEFROMVISIBLE, m_firstVisible, 0);
	int lastVisible		= m_firstVisible + CallScintilla(m_view, SCI_LINESONSCREEN, 0, 0);

	lastVisible = CallScintilla(m_view, SCI_DOCLINEFROMVISIBLE, lastVisible, 0);

	if (firstVisible == lastVisible)
		++lastVisible;

	firstVisible	= docToBmpLine(firstVisible);
	lastVisible		= docToBmpLine(lastVisible);

	h /= hScale;

	// Selector is out of scope so don't draw it
	if (firstVisible > hOffset + h || lastVisible < hOffset)
		return;

	if (firstVisible < hOffset)
		firstVisible = hOffset;

	if (lastVisible > hOffset + h)
		lastVisible = hOffset + h;

	firstVisible	*= hScale;
	lastVisible		*= hScale;

	r.top		= firstVisible + yPos - hOffset;
	r.bottom	= lastVisible + yPos - hOffset + 2;

	::Rectangle(hDC, r.left, r.top, r.right, r.bottom);

	BLENDFUNCTION blend = { 0 };
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 40;

	::AlphaBlend(hDC, r.left + 1, r.top + 1, width, lastVisible - firstVisible, m_hSelDC, 0, 0, 1, 1, blend);
}


int NavDialog::NavView::docToBmpLine(int docLine) const
{
	if (m_lineMap.empty())
		return docLine;

	const int max = static_cast<int>(m_lineMap.size());

	for (int i = 0; i < max; ++i)
	{
		if (docLine <= m_lineMap[i])
			return i + 1;
	}

	return max;
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

	m_view[0].m_view = MAIN_VIEW;
	m_view[1].m_view = SUB_VIEW;

	if (isRTLwindow(nppData._nppHandle))
		std::swap(m_view[0].m_view, m_view[1].m_view);
}


void NavDialog::doDialog()
{
	if (!isCreated())
	{
		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		_data.pszName       = TEXT("ComparePlus NavBar");
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= CMD_NAV_BAR;
		_data.hIconTab		= (HICON)::LoadImage(GetModuleHandle(TEXT("ComparePlus.dll")),
				MAKEINTRESOURCE(IDB_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
	}
}


void NavDialog::Update()
{
	if (!isVisible())
		return;

	// Bitmap needs to be recreated
	if ((m_view[0].m_lines != CallScintilla(m_view[0].m_view, SCI_GETLINECOUNT, 0, 0)) ||
		(m_view[1].m_lines != CallScintilla(m_view[1].m_view, SCI_GETLINECOUNT, 0, 0)))
	{
		Show();
	}
	else
	{
		m_view[0].updateFirstVisible();
		m_view[1].updateFirstVisible();

		updateScroll();

		::InvalidateRect(_hSelf, NULL, FALSE);
	}
}


void NavDialog::Show()
{
	HWND hwnd = ::GetFocus();

	doDialog();

	// Free resources if needed
	m_view[0].reset();
	m_view[1].reset();

	HDC hDC = ::GetDC(_hSelf);

	m_view[0].init(hDC);
	m_view[1].init(hDC);

	// Release DC
	::ReleaseDC(_hSelf, hDC);

	createBitmap();

	display(true);

	::SetFocus(hwnd);
}


void NavDialog::Hide()
{
	HWND hwnd = ::GetFocus();

	display(false);

	m_view[0].reset();
	m_view[1].reset();

	::SetFocus(hwnd);
}


void NavDialog::createBitmap()
{
	RECT r;
	::GetClientRect(_hSelf, &r);

	const int maxLines	= std::max(m_view[0].m_lines, m_view[1].m_lines);
	const int maxHeight	= (r.bottom - r.top) - 2 * cSpace - 2;

	int reductionRatio = maxLines / maxHeight;

	if (reductionRatio && (maxLines % maxHeight))
		++reductionRatio;

	{
		RECT bmpRect = { 0 };

		bmpRect.right = 1;
		bmpRect.bottom = 1;

		HBRUSH hBrush = ::CreateSolidBrush(m_clr._default ^ 0xFFFFFF);

		::FillRect(m_view[0].m_hSelDC, &bmpRect, hBrush);
		::FillRect(m_view[1].m_hSelDC, &bmpRect, hBrush);

		::DeleteObject(hBrush);

		hBrush = ::CreateSolidBrush(m_clr._default);

		bmpRect.bottom = m_view[0].m_lines;
		::FillRect(m_view[0].m_hViewDC, &bmpRect, hBrush);

		bmpRect.bottom = m_view[1].m_lines;
		::FillRect(m_view[1].m_hViewDC, &bmpRect, hBrush);

		::DeleteObject(hBrush);

		m_view[0].m_lineMap.clear();
		m_view[1].m_lineMap.clear();

		int skipLine = reductionRatio;
		int prevMarker = m_clr._default;
		int bmpLine = 0;

		for (int i = 0; i < m_view[0].m_lines; ++i)
		{
			int marker = CallScintilla(m_view[0].m_view, SCI_MARKERGET, i, 0);
			if (!marker && !reductionRatio)
				continue;

			if (marker & MARKER_MASK_CHANGED)		marker = m_clr.changed;
			else if (marker & MARKER_MASK_ADDED)	marker = m_clr.added;
			else if (marker & MARKER_MASK_REMOVED)	marker = m_clr.removed;
			else if (marker & MARKER_MASK_MOVED)	marker = m_clr.moved;
			else if (reductionRatio)				marker = m_clr._default;
			else
				continue;

			if (reductionRatio)
			{
				if (prevMarker == marker)
					--skipLine;

				if (prevMarker != marker || !skipLine)
				{
					skipLine = reductionRatio;
					prevMarker = marker;

					m_view[0].m_lineMap.push_back(i);

					::SetPixel(m_view[0].m_hViewDC, 0, bmpLine++, marker);
				}
			}
			else
			{
				::SetPixel(m_view[0].m_hViewDC, 0, i, marker);
			}
		}

		skipLine = reductionRatio;
		prevMarker = m_clr._default;
		bmpLine = 0;

		for (int i = 0; i < m_view[1].m_lines; ++i)
		{
			int marker = CallScintilla(m_view[1].m_view, SCI_MARKERGET, i, 0);
			if (!marker && !reductionRatio)
				continue;

			if (marker & MARKER_MASK_CHANGED)		marker = m_clr.changed;
			else if (marker & MARKER_MASK_ADDED)	marker = m_clr.added;
			else if (marker & MARKER_MASK_REMOVED)	marker = m_clr.removed;
			else if (marker & MARKER_MASK_MOVED)	marker = m_clr.moved;
			else if (reductionRatio)				marker = m_clr._default;
			else
				continue;

			if (reductionRatio)
			{
				if (prevMarker == marker)
					--skipLine;

				if (prevMarker != marker || !skipLine)
				{
					skipLine = reductionRatio;
					prevMarker = marker;

					m_view[1].m_lineMap.push_back(i);

					::SetPixel(m_view[1].m_hViewDC, 0, bmpLine++, marker);
				}
			}
			else
			{
				::SetPixel(m_view[1].m_hViewDC, 0, i, marker);
			}
		}
	}

	setScalingFactor();
}


void NavDialog::showScroller(RECT& r)
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
	si.nMax		= m_maxBmpLines * m_pixelsPerLine - 1;
	si.nPage	= h;

	::SetScrollInfo(m_hScroll, SB_CTL, &si, TRUE);

	m_navViewWidth = ((r.right - r.left) - 4 * cSpace - cScrollerWidth - 4) / 2;

	::ShowScrollBar(m_hScroll, SB_CTL, TRUE);
}


void NavDialog::setScalingFactor()
{
	RECT r;
	::GetClientRect(_hSelf, &r);

	// Happens when minimizing N++ window because WM_SIZE notification is received but window is minimized?!?!
	if (r.bottom - r.top == 0)
		return;

	m_view[0].m_lines = CallScintilla(m_view[0].m_view, SCI_GETLINECOUNT, 0, 0);
	m_view[1].m_lines = CallScintilla(m_view[1].m_view, SCI_GETLINECOUNT, 0, 0);

	m_view[0].updateFirstVisible();
	m_view[1].updateFirstVisible();

	m_maxBmpLines = std::max(m_view[0].maxBmpLines(), m_view[1].maxBmpLines());
	m_syncView = (m_maxBmpLines == m_view[0].maxBmpLines()) ? &m_view[0] : &m_view[1];

	m_navViewWidth = ((r.right - r.left) - 3 * cSpace - 4) / 2;
	m_navHeight = (r.bottom - r.top) - 2 * cSpace - 2;

	m_pixelsPerLine = m_navHeight / m_maxBmpLines;

	if (m_pixelsPerLine == 0)
	{
		m_pixelsPerLine = 1;

		showScroller(r);
	}
	else
	{
		if (m_pixelsPerLine > 5)
			m_pixelsPerLine = 5;

		m_navHeight = m_pixelsPerLine * m_maxBmpLines;

		if (m_hScroll)
			::ShowScrollBar(m_hScroll, SB_CTL, FALSE);
	}

	updateScroll();
	updateDockingDlg();

	if (isVisible())
		::InvalidateRect(_hSelf, NULL, TRUE);
}


void NavDialog::setPos(int x, int y)
{
	y -= (cSpace + 2);
	if (y < 0 || x < cSpace || x > (2 * m_navViewWidth + 2 * cSpace + 4))
		return;

	NavView* currentView;

	const int scrollOffset = (m_hScroll && ::IsWindowVisible(m_hScroll)) ? ::GetScrollPos(m_hScroll, SB_CTL) : 0;

	if (x < m_navViewWidth + cSpace + 2)
	{
		if (y > std::min((m_view[0].maxBmpLines() - scrollOffset) * m_pixelsPerLine, m_navHeight))
			return;
		currentView = &m_view[0];
	}
	else
	{
		if (y > std::min((m_view[1].maxBmpLines() - scrollOffset) * m_pixelsPerLine, m_navHeight))
			return;
		currentView = &m_view[1];
	}

	const int currentLine = currentView->bmpToDocLine((y + scrollOffset) / m_pixelsPerLine);

	if (!isLineVisible(currentView->m_view, currentLine))
		centerAt(currentView->m_view, currentLine);

	if (Settings.FollowingCaret)
	{
		::SetFocus(getView(currentView->m_view));

		if (Settings.ShowOnlyDiffs || Settings.ShowOnlySelections)
			gotoClosestUnhiddenLine(currentView->m_view, currentLine);
		else
			CallScintilla(currentView->m_view, SCI_SETEMPTYSELECTION,
					getLineStart(currentView->m_view, currentLine), 0);

		::UpdateWindow(getView(currentView->m_view));
	}
}


void NavDialog::onMouseWheel(int rolls)
{
	const int linesOnScreen	= CallScintilla(m_syncView->m_view, SCI_LINESONSCREEN, 0, 0);
	const int lastVisible	= CallScintilla(m_syncView->m_view, SCI_VISIBLEFROMDOCLINE, m_syncView->m_lines, 0);

	int firstVisible = m_syncView->m_firstVisible - rolls * linesOnScreen;

	if (firstVisible < 0)
		firstVisible = 0;
	else if (firstVisible > lastVisible - linesOnScreen)
		firstVisible = lastVisible - linesOnScreen;

	CallScintilla(m_syncView->m_view, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
}


int NavDialog::updateScroll()
{
	if (m_hScroll && ::IsWindowVisible(m_hScroll))
	{
		const int firstVisible1 =
				CallScintilla(m_view[0].m_view, SCI_DOCLINEFROMVISIBLE, m_view[0].m_firstVisible, 0);
		const int firstVisible2 =
				CallScintilla(m_view[1].m_view, SCI_DOCLINEFROMVISIBLE, m_view[1].m_firstVisible, 0);

		int lastVisible1 = m_view[0].m_firstVisible + CallScintilla(m_view[0].m_view, SCI_LINESONSCREEN, 0, 0);
		int lastVisible2 = m_view[1].m_firstVisible + CallScintilla(m_view[1].m_view, SCI_LINESONSCREEN, 0, 0);

		lastVisible1 = CallScintilla(m_view[0].m_view, SCI_DOCLINEFROMVISIBLE, lastVisible1, 0);
		lastVisible2 = CallScintilla(m_view[1].m_view, SCI_DOCLINEFROMVISIBLE, lastVisible2, 0);

		const int firstVisible	= std::min(firstVisible1, firstVisible2);
		const int lastVisible	= std::max(lastVisible1, lastVisible2);

		const NavView* const firstVisibleSyncView	= (firstVisible == firstVisible1) ? &m_view[0] : &m_view[1];
		const NavView* const lastVisibleSyncView	= (lastVisible == lastVisible1) ? &m_view[0] : &m_view[1];

		int currentScroll = ::GetScrollPos(m_hScroll, SB_CTL);

		if (firstVisibleSyncView->bmpToDocLine(currentScroll) > firstVisible)
		{
			currentScroll = firstVisibleSyncView->docToBmpLine(firstVisible);

			::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
		}
		else if (lastVisibleSyncView->bmpToDocLine(currentScroll + m_navHeight - 1) < lastVisible)
		{
			currentScroll = lastVisibleSyncView->docToBmpLine(lastVisible) - m_navHeight;

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

	const int scrollOffset = (m_hScroll && ::IsWindowVisible(m_hScroll)) ? ::GetScrollPos(m_hScroll, SB_CTL) : 0;

	HPEN hPenView = ::CreatePen(PS_SOLID, 1, RGB(180, 180, 180));

	PAINTSTRUCT ps;

	// Get current DC
	HDC hDC = ::BeginPaint(_hSelf, &ps);

	::SelectObject(hDC, hPenView);
	::SelectObject(hDC, ::GetStockObject(NULL_BRUSH));

	if (m_view[0].maxBmpLines() > scrollOffset)
		m_view[0].paint(hDC, cSpace, cSpace, m_navViewWidth, m_navHeight, m_pixelsPerLine, scrollOffset);

	if (m_view[1].maxBmpLines() > scrollOffset)
		m_view[1].paint(hDC, m_navViewWidth + 2 * cSpace + 2, cSpace,
				m_navViewWidth, m_navHeight, m_pixelsPerLine, scrollOffset);

	::DeleteObject(hPenView);

	::EndPaint(_hSelf, &ps);
}


void NavDialog::adjustScroll(int offset)
{
	::SetFocus(_hSelf);

	int currentScroll = ::GetScrollPos(m_hScroll, SB_CTL) + offset;

	if (currentScroll < 0)
		currentScroll = 0;
	else if (currentScroll > m_maxBmpLines * m_pixelsPerLine - 1)
		currentScroll = m_maxBmpLines * m_pixelsPerLine - m_navHeight;

	::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
	::InvalidateRect(_hSelf, NULL, FALSE);
}


INT_PTR CALLBACK NavDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		break;

		case WM_PAINT:
			onPaint();
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

		case WM_VSCROLL:
		{
			switch (LOWORD(wParam))
			{
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
				{
					const int currentScroll = HIWORD(wParam);

					::SetFocus(_hSelf);

					::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
					::InvalidateRect(_hSelf, NULL, FALSE);
				}
				break;

				case SB_PAGEDOWN:
					adjustScroll(m_navHeight);
				break;

				case SB_PAGEUP:
					adjustScroll(-m_navHeight);
				break;

				case SB_LINEDOWN:
					adjustScroll(1);
				break;

				case SB_LINEUP:
					adjustScroll(-1);
				break;
			}
		}
		break;

		case WM_SIZE:
		case WM_MOVE:
			if (isVisible())
				setScalingFactor();
		break;

		case WM_NOTIFY:
		{
			LPNMHDR	pnmh = (LPNMHDR)lParam;

			if (pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_CLOSE)
			{
				ToggleNavigationBar();
			}
			else if ((pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_FLOAT) ||
					(pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_DOCK))
			{
				setScalingFactor();
				::SetFocus(getView(m_syncView->m_view));
			}
		}
		break;

		default:
			return DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
	}

	return FALSE;
}
