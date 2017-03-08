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
#include "resource.h"
#include "UserSettings.h"
#include "ColorCombo.h"

#include <vector>
#include <string>


using namespace std;


class SettingsDialog : public StaticDialog
{

public:
	SettingsDialog() : StaticDialog() {};

	void init(HINSTANCE hInst, NppData nppDataParam)
	{
		_nppData = nppDataParam;
		Window::init(hInst, nppDataParam._nppHandle);
	};

	UINT doDialog(UserSettings* settings);

	virtual void destroy() {};

protected :
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

	void SetParams(UserSettings* settings = nullptr);
	void GetParams();

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
	ColorCombo _ColorComboHighlight;

	struct UserSettings* _Settings;
};
