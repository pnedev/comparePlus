/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2017 Pavel Nedev (pg.nedev@gmail.com)
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

#pragma once

#include "Compare.h"
#include "Window.h"
#include "DockingDlgInterface.h"

#include <vector>


class NavDialog : public DockingDlgInterface
{
public:
	NavDialog();
	~NavDialog();

	NavDialog(const NavDialog&) = delete;
	NavDialog& operator=(const NavDialog&) = delete;

	void init(HINSTANCE hInst);
	void destroy() {};

	void Show();
	void Hide();

	void SetColors(const ColorSettings& colors)
	{
		m_clr = colors;

		if (isVisible())
			Show();
	}

	void Update();

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	static const int cSpace;
	static const int cScrollerWidth;

	/**
	 *  \struct
	 *  \brief
	 */
	struct NavView
	{
		NavView() : m_view(0), m_hViewDC(NULL), m_hSelDC(NULL), m_hViewBMP(NULL), m_hSelBMP(NULL) {}

		~NavView()
		{
			reset();
		}

		void init(HDC hDC);
		void reset();
		void paint(HDC hDC, int xPos, int yPos, int width, int height, int hScale, int hOffset);

		void updateFirstVisible()
		{
			m_firstVisible = CallScintilla(m_view, SCI_GETFIRSTVISIBLELINE, 0, 0);
		}


		int maxBmpLines()
		{
			return (m_lineMap.empty() ? m_lines : static_cast<int>(m_lineMap.size()));
		}

		int bmpToDocLine(int bmpLine)
		{
			return (m_lineMap.empty() ? bmpLine :
				(bmpLine < static_cast<int>(m_lineMap.size())) ? m_lineMap[bmpLine] : m_lineMap.back());
		}

		int docToBmpLine(int docLine);

		int		m_view;

		HDC		m_hViewDC;
		HDC		m_hSelDC;

		HBITMAP	m_hViewBMP;
		HBITMAP	m_hSelBMP;

		int		m_firstVisible;
		int		m_lines;

		std::vector<int>	m_lineMap;
	};

	void doDialog();

	void createBitmap();
	void showScroller(RECT& r);

	void setScalingFactor();

	void setPos(int x, int y);
	void onMouseWheel(int rolls);
	int updateScroll();
	void onPaint();
	void adjustScroll(int offset);

	tTbData	_data;

	ColorSettings	m_clr;

	HINSTANCE		m_hInst;

	HWND		m_hScroll;

	bool		m_mouseOver;

	int			m_navViewWidth;
	int			m_navHeight;

	int			m_pixelsPerLine;
	int			m_maxBmpLines;

	NavView		m_view[2];
	NavView*	m_syncView;
};
