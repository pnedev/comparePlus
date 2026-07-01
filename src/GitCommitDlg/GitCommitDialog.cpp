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

#pragma comment (lib, "uxtheme")


#include "GitCommitDialog.h"

#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "NppHelpers.h"
#include "Strings.h"
#include "Tools.h"


UINT GitCommitDialog::doDialog(std::string& commit)
{
	_commit = &commit;

	UINT res = 0;

	if (isRTLwindow(_hParent))
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_GIT_COMMIT_DIALOG, &pMyDlgTemplate);

		if (hMyDlgTemplate)
		{
			res = (UINT)::DialogBoxIndirectParamW(_hInst, pMyDlgTemplate, _hParent, (DLGPROC)dlgProc, (LPARAM)this);
			::GlobalFree(hMyDlgTemplate);
		}
	}
	else
	{
		res = (UINT)::DialogBoxParamW(_hInst, MAKEINTRESOURCEW(IDD_GIT_COMMIT_DIALOG), _hParent,
				(DLGPROC)dlgProc, (LPARAM)this);
	}

	return res;
}


INT_PTR CALLBACK GitCommitDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			registerDlgForDarkMode(_hSelf);

			goToCenter();

			::EnableThemeDialogTexture(_hSelf, ETDT_ENABLETAB);

			// Dialog opens by default in english
			if (Strings::get().currentLocale() != "english")
				updateLocalization();
		}
		return TRUE;

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDOK:
				{
					HWND hCtrl = ::GetDlgItem(_hSelf, IDC_GIT_COMMIT_EDIT);

					const int len = ::GetWindowTextLengthW(hCtrl);
					if (len > 0)
					{
						std::wstring txt(len + 1, 0);
						::GetWindowTextW(hCtrl, txt.data(), len + 1);
						*_commit = WCtoMB(txt.c_str());

						::EndDialog(_hSelf, IDOK);
					}
					else
					{
						::EndDialog(_hSelf, IDCANCEL);
					}
				}
				return TRUE;

				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
				return TRUE;

				default:
				return FALSE;
			}
		}
		break;
	}

	return FALSE;
}


void GitCommitDialog::updateLocalization()
{
	const auto& str = Strings::get();

	::SetWindowTextW(_hSelf, (std::wstring(PLUGIN_NAME) + std::wstring(L"   ") + str["HDR_GIT_COMMIT"]).c_str());

	::SetDlgItemTextW(_hSelf, IDOK,		str["IDOK"].c_str());
	::SetDlgItemTextW(_hSelf, IDCANCEL,	str["IDCANCEL"].c_str());

	updateDlgCtrlTxt(_hSelf, IDC_GIT_COMMIT_MSG, str["IDC_GIT_COMMIT_MSG"].c_str());
}
