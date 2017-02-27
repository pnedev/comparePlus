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


#pragma once


#include "PluginInterface.h"
#include "StaticDialog.h"
#include "URLCtrl.h"

class AboutDialog : public StaticDialog
{

public:
	AboutDialog() : StaticDialog() {};

	void init(HINSTANCE hInst, NppData nppDataParam)
	{
		_nppData = nppDataParam;
		Window::init(hInst, nppDataParam._nppHandle);
	};

	void doDialog();

	virtual void destroy() {
		_emailLinkJSL.destroy();
		_emailLinkPND.destroy();
		_urlOriginalRepo.destroy();
		_urlPNDRepo.destroy();
		_helpLink.destroy();
	};


protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	/* Handles */
	NppData			_nppData;
	HWND			_HSource;

	/* for eMail */
	URLCtrl			_emailLinkJSL;
	URLCtrl			_emailLinkPND;
	URLCtrl			_urlOriginalRepo;
	URLCtrl			_urlPNDRepo;
	URLCtrl			_helpLink;
};
