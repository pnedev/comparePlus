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
#include <tchar.h>
#include <shellapi.h>
#include <cstdlib>

#include "AboutDialog.h"
#include "resource.h"
#include "DockingFeature/Window.h"

#include "NppHelpers.h"


static const TCHAR cDonate_URL[]	= TEXT("https://www.paypal.com/paypalme/pnedev");
static const TCHAR cRepo_URL[]		= TEXT("https://github.com/pnedev/comparePlus");
static const TCHAR cHelp_URL[]		= TEXT("https://github.com/pnedev/comparePlus/blob/master/Help.md");


UINT AboutDialog::doDialog()
{
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), _hParent,
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

			TCHAR buildTimeStr[256];

			_sntprintf_s(buildTimeStr, _countof(buildTimeStr), _TRUNCATE,
					TEXT("Build time:    %s,  %s"), TEXT(__DATE__), TEXT(__TIME__));

			::SetDlgItemText(_hSelf, IDC_BUILD_TIME, buildTimeStr);

			_emailLink.init(_hInst, _hSelf);
			_emailLink.create(::GetDlgItem(_hSelf, IDC_EMAIL_LINK), TEXT("mailto:pg.nedev@gmail.com"));
			_urlRepo.init(_hInst, _hSelf);
			_urlRepo.create(::GetDlgItem(_hSelf, IDC_REPO_URL), cRepo_URL);
			_helpLink.init(_hInst, _hSelf);
			_helpLink.create(::GetDlgItem(_hSelf, IDC_HELP_URL), cHelp_URL);

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
					::ShellExecute(NULL, _T("open"), cDonate_URL, NULL, NULL, SW_SHOWNORMAL);
				return TRUE;

				default :
				break;
			}
			break;
		}
	}
	return FALSE;
}

