//this file is part of Hex Edit Plugin for Notepad++
//Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef OPTION_DEFINE_H
#define OPTION_DEFINE_H

#include "StaticDialog.h"
#include "Resource.h"
#include "Compare.h"
#include "ColorCombo.h"
#include <vector>
#include <string>

using namespace std;

class OptionDialog : public StaticDialog
{

public:
	OptionDialog() : StaticDialog() {};
    
    void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

    void doDialog(struct sColorSettings * Settings);

    virtual void destroy() {};

protected :
	BOOL CALLBACK run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void SetParams(void);
	BOOL GetParams(void);

private:
	/* Handles */
	NppData			_nppData;
    HWND			_HSource;

    // Combo color picker
	ColorCombo _ColorComboAdded;
	ColorCombo _ColorComboMoved;
	ColorCombo _ColorComboRemoved;
	ColorCombo _ColorComboChanged;
	ColorCombo _ColorComboBlank;

    struct sColorSettings* _ColorSettings;
};

#endif // OPTION_DEFINE_H
