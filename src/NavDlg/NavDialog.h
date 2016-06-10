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
#include "Window.h"
#include "DockingDlgInterface.h"


class NavDialog : public DockingDlgInterface
{
public:
	NavDialog();
	~NavDialog();

	void init(HINSTANCE hInst);
	void destroy() {};

	void doDialog(bool show = true);

	void SetColors(const ColorSettings& colorSettings);
	void CreateBitmap();
	void Update();

protected:
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	static const int cSpace;

	void Show();
	void Hide();

	void SetScalingFactor();
	void FillViewBitmap(HWND view, HDC hMemDC);

	void scrollView(int x, int y);
	void onMouseWheel(int delta);
	void onPaint();

	tTbData	_data;

	ColorSettings m_clr;

	int		m_NavHalfWidth;
	int		m_NavHeight;

	float	m_HeightScaleFactor;

	HDC		m_hMemDC1;
	HDC		m_hMemDC2;
	HDC		m_hMemDC3;

	HBITMAP	m_hMemBMP1;
	HBITMAP	m_hMemBMP2;
	HBITMAP	m_hMemBMP3;

	SIZE	m_hMemBMPSize;
	SIZE	m_hMemSelBMPSize;

	int		m_FirstVisibleLine;
	int		m_MaxLineCount;

	bool	m_mouseOver;
};
