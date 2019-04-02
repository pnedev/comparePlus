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

#include "AboutDialog.h"
#include "resource.h"
#include "Window.h"


static const TCHAR cDonate_URL[]	= TEXT("https://paypal.me/pnedev");
static const TCHAR cHelp_URL[]		= TEXT("https://github.com/pnedev/compare-plugin/blob/master/Help.md");


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
			goToCenter();

			_emailLinkJSL.init(_hInst, _hSelf);
			_emailLinkJSL.create(::GetDlgItem(_hSelf, IDC_EMAIL_LINK_JSL), TEXT("mailto:jean.sebastien.leroy@gmail.com"));
			_emailLinkPND.init(_hInst, _hSelf);
			_emailLinkPND.create(::GetDlgItem(_hSelf, IDC_EMAIL_LINK_PND), TEXT("mailto:pg.nedev@gmail.com"));
			_urlOriginalRepo.init(_hInst, _hSelf);
			_urlOriginalRepo.create(::GetDlgItem(_hSelf, IDC_ORIGINAL_REPO_URL), NULL);
			_urlPNDRepo.init(_hInst, _hSelf);
			_urlPNDRepo.create(::GetDlgItem(_hSelf, IDC_LATEST_DEV_REPO_URL), NULL);
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

