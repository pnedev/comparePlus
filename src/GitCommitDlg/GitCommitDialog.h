/*
 * This file is part of ComparePlus plugin for Notepad++
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


#include <string>
#include "PluginInterface.h"
#include "DockingFeature/StaticDialog.h"
#include "resource.h"


class GitCommitDialog : public StaticDialog
{

public:
	GitCommitDialog(HINSTANCE hInst, HWND hWnd)
	{
		Window::init(hInst, hWnd);
	};

	~GitCommitDialog()
	{
		destroy();
	}

	UINT doDialog(std::string& commit);

	virtual void destroy() {};

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

private:
	void updateLocalization();

	std::string	*_commit;
};
