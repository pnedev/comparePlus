/*
 * This file is part of Plugin Template Plugin for Notepad++
 * Copyright (C)2009
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
#include "UserSettings.h"
#include "Window.h"
#include "DockingDlgInterface.h"

#include <vector>


class NavDialog : public DockingDlgInterface
{
public:
	NavDialog();
	~NavDialog();

	void init(HINSTANCE hInst);
	void destroy() {};

	void Show();
	void Hide();

	void SetConfig(const UserSettings& settings);
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
		NavView() : m_hView(NULL), m_hViewDC(NULL), m_hSelDC(NULL), m_hViewBMP(NULL), m_hSelBMP(NULL) {}

		~NavView()
		{
			reset();
		}

		void init(HDC hDC);
		void reset();
		void create(const ColorSettings& colors, int reductionRatio = 0);
		void paint(HDC hDC, int xPos, int yPos, int width, int height, int hScale, int hOffset);

		bool updateFirstVisible();

		int maxBmpLines();
		int docToBmpLine(int docLine);
		int bmpToDocLine(int bmpLine);

		HWND	m_hView;

		HDC		m_hViewDC;
		HDC		m_hSelDC;

		HBITMAP	m_hViewBMP;
		HBITMAP	m_hSelBMP;

		int		m_firstVisible;
		int		m_lines;

		std::vector<int>	m_lineMap;
	};

	void doDialog();

	void CreateBitmap();
	void ShowScroller(RECT& r);

	void SetScalingFactor();
	void FillViewBitmap(HWND view, HDC hDC);

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
