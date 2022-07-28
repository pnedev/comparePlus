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

#include "IgnoreRegexDialog.h"

#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <regex>

#include "NppHelpers.h"


UINT IgnoreRegexDialog::doDialog(UserSettings* settings)
{
	_Settings = settings;

	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_IGNORE_REGEX_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK IgnoreRegexDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			goToCenter();

			ETDTProc EnableDlgTheme =
					(ETDTProc)::SendMessage(_nppData._nppHandle, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);

			if (EnableDlgTheme != NULL)
				EnableDlgTheme(_hSelf, ETDT_ENABLETAB);

			SetParams();
		}
		break;

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDOK:
					if (GetParams())
					{
						_Settings->markAsDirty();
						::EndDialog(_hSelf, IDOK);
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


void IgnoreRegexDialog::SetParams()
{
	HWND hCtrl = ::GetDlgItem(_hSelf, IDC_IGNORE_REGEX);

	::SendMessage(hCtrl, EM_SETLIMITTEXT, cMaxRegexLen, 0);

	Edit_SetText(hCtrl, _Settings->IgnoreRegexStr.c_str());
	Edit_SetSel(hCtrl, 0, -1);
}


bool IgnoreRegexDialog::GetParams()
{
	HWND hCtrl = ::GetDlgItem(_hSelf, IDC_IGNORE_REGEX);

	int len = Edit_LineLength(hCtrl, 0);

	if (len > 0)
	{
		wchar_t buf[cMaxRegexLen + 1];

		Edit_GetLine(hCtrl, 0, buf, cMaxRegexLen);
		buf[len] = L'\0';

		if (isRegexValid(buf))
		{
			_Settings->IgnoreRegexStr = buf;
		}
		else
		{
			::SetFocus(hCtrl);

			return false;
		}
	}
	else
	{
		_Settings->IgnoreRegexStr = L"";
	}

	return true;
}


bool IgnoreRegexDialog::isRegexValid(const wchar_t* regexStr)
{
	try
	{
		std::wregex testRegex(regexStr, std::regex::ECMAScript);
	}
	catch (std::regex_error& err)
	{
		::MessageBoxA(nppData._nppHandle, err.what(), "ComparePlus Bad Regex", MB_OK | MB_ICONWARNING);

		return false;
	}

	return true;
}
