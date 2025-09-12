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

#include <windows.h>
#include <wchar.h>
#include <shellapi.h>
#include <cstdlib>
#include <string>

#include "AboutDialog.h"
#include "resource.h"
#include "DockingFeature/Window.h"

#include "NppHelpers.h"
#include "Tools.h"


static const wchar_t cDonate_URL[]	= L"https://www.paypal.com/paypalme/pnedev";
static const wchar_t cRepo_URL[]	= L"https://github.com/pnedev/comparePlus";
static const wchar_t cHelp_URL[]	= L"https://github.com/pnedev/comparePlus/blob/master/Help.md";


UINT AboutDialog::doDialog()
{
	return (UINT)::DialogBoxParamW(_hInst, MAKEINTRESOURCEW(IDD_ABOUT_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK AboutDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			registerDlgForDarkMode(_hSelf);

			goToCenter();

			wchar_t buildInfo[256];

			_snwprintf_s(buildInfo, _countof(buildInfo), _TRUNCATE, L"Build time:  %S, %S", __DATE__, __TIME__);

			::SetDlgItemTextW(_hSelf, IDC_BUILD_TIME, buildInfo);

			std::wstring libInfo = L"LibGit2 version:  ";

			if (_libGit2Ver.empty())
				libInfo += L"lib not found";
			else
				libInfo += _libGit2Ver.c_str();

			::SetDlgItemTextW(_hSelf, IDC_GITLIB_VER, libInfo.c_str());

			libInfo = L"SQLite3 version:  ";

			if (_sqlite3Ver.empty())
				libInfo += L"lib not found";
			else
				libInfo += _sqlite3Ver.c_str();

			::SetDlgItemTextW(_hSelf, IDC_SQLITE3_VER, libInfo.c_str());

			COLORREF linkColor = ::GetSysColor(COLOR_HOTLIGHT);

			if (isDarkMode())
			{
				auto dmColors = getNppDarkModeColors();
				if (dmColors)
					linkColor = dmColors->linkText;
			}

			_urlRepo.init(_hInst, _hSelf);
			_urlRepo.create(::GetDlgItem(_hSelf, IDC_REPO_URL), cRepo_URL, linkColor);
			_helpLink.init(_hInst, _hSelf);
			_helpLink.create(::GetDlgItem(_hSelf, IDC_HELP_URL), cHelp_URL, linkColor);

			return TRUE;
		}
		case WM_COMMAND :
		{
			switch (wParam)
			{
				case IDC_ABOUT_CLOSE_BUTTON :
				case IDCANCEL :
					::EndDialog(_hSelf, 0);
				return TRUE;

				case IDC_DONATE_BUTTON :
					::ShellExecuteW(NULL, L"open", cDonate_URL, NULL, NULL, SW_SHOWNORMAL);
				return TRUE;

				default :
				break;
			}
			break;
		}
	}

	return FALSE;
}
