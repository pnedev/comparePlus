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

#pragma once

#include "Compare.h"
#include "Window.h"
#include "DockingDlgInterface.h"


class NavDialog : public DockingDlgInterface
{
public:
	NavDialog();
	~NavDialog();

	void init(HINSTANCE hInst, NppData nppData);
	void destroy() {};

    void SetColor(const sColorSettings& colorSettings);
	void CreateBitmap();
    void DrawView();

	void doDialog(bool willBeShown = true);

protected:
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	static const int cSpace;
	static const int cMinSelectorHeight;

	void Show();
	void Hide();

    void SetLinePixel(long resultsDoc, int i, HDC hMemDC, int* lastDiffColor, int* lastDiffCounter);
	void SetScalingFactor();

	void scrollView(short yPos);

	void OnPaint();

	NppData	_nppData;
	tTbData	_data;

	sColorSettings _clr;

	int m_minimumDiffHeight;

    long m_SideBarPartWidth;
    long m_SideBarPartHeight;

    double m_ScaleFactorDocLines;
    double m_ScaleFactorVisibleLines;

	HDC     m_hMemDC1;
	HDC     m_hMemDC2;

	HBITMAP m_hMemBMP1;
	HBITMAP m_hMemBMP2;

	SIZE    m_hMemBMPSize;

	/* Internal use */
	RECT    m_rLeft;
	RECT    m_rRight;

    int     m_DocLineCount;
    int     m_VisibleLineCount;

    int     m_LineCount1;
    int     m_LineCount2;
};
