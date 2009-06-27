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

#ifndef NAV_DLG_H
#define NAV_DLG_H

#include "Compare.h"
#include "DockingDlgInterface.h"

#define SPACE   5

class NavDialog : public DockingDlgInterface
{
public:
	NavDialog(void);
	~NavDialog(void);

    void init(HINSTANCE hInst, NppData nppData);
	void destroy(void) {};
   	void doDialog(bool willBeShown = true);
    void SetColor(int added, int deleted, int changed, int moved, int blank);  
    void CreateBitmap(void);

protected:

	virtual BOOL CALLBACK run_dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

    HWND m_hWnd;

    LRESULT OnPaint(HWND hWnd);

    int m_AddedColor;
    int m_DeletedColor;
    int m_ChangedColor;
    int m_MovedColor;
    int m_BlankColor;

    HDC     m_hdc;
    HDC     m_hMemDC1;
    HDC     m_hMemDC2;

    HBITMAP m_hMemBMP1;
    HBITMAP m_hMemBMP2;

    BITMAP  m_hMemBMPInfo;
    SIZE    m_hMemBMPSize;

	NppData	_nppData;
	tTbData	_data;

    /* Internal use */
	RECT    m_rLeft;
    RECT    m_rRight;	

    int     m_TextLength;

    HPEN  m_BlankPencil;
    HPEN  m_AddedPencil;
    HPEN  m_ChangedPencil;
    HPEN  m_MovedPencil;
    HPEN  m_RemovedPencil;

    long  *m_ResultsDoc1;
    long  *m_ResultsDoc2;
};

#endif // NAV_DLG_H