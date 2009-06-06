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

class NavDialog : public DockingDlgInterface
{
public:
	NavDialog(void);
	~NavDialog(void);

    void init(HINSTANCE hInst, NppData nppData);

	void destroy(void) {};

   	void doDialog(bool willBeShown = true);

protected:

	virtual BOOL CALLBACK run_dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

	NppData	_nppData;
	tTbData	_data;
	
};

#endif // NAV_DLG_H