/*
 * This file is part of Compare Plugin for Notepad++
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	HWND			_HSource;

	/* for eMail */
	URLCtrl			_emailLinkJSL;
	URLCtrl			_emailLinkPND;
	URLCtrl			_urlOriginalRepo;
	URLCtrl			_urlPNDRepo;
	URLCtrl			_helpLink;
};
