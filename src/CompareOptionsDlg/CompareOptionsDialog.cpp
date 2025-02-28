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


#include "CompareOptionsDialog.h"

#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <boost/regex.hpp>

#include "NppHelpers.h"


UINT CompareOptionsDialog::doDialog(UserSettings* settings)
{
	_Settings = settings;

	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_COMPARE_OPTIONS_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK CompareOptionsDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			registerDlgForDarkMode(_hSelf);

			goToCenter();

			::EnableThemeDialogTexture(_hSelf, ETDT_ENABLETAB);

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

						return TRUE;
					}
				break;

				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
				return TRUE;

				case IDC_DETECT_SUB_BLOCK_DIFFS:
					Button_Enable(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS),
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_BLOCK_DIFFS)) == BST_CHECKED));
				break;

				case IDC_IGNORE_CHANGED_SPACES:
				{
					bool ignoreChangedSpaces =
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES)) == BST_CHECKED);

					if (ignoreChangedSpaces)
						Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES), BST_UNCHECKED);
				}
				break;

				case IDC_IGNORE_ALL_SPACES:
				{
					bool ignoreAllSpaces =
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES)) == BST_CHECKED);

					if (ignoreAllSpaces)
						Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES), BST_UNCHECKED);
				}
				break;

				case IDC_IGNORE_REGEX:
				{
					const bool ignoreRegex = (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX)) == BST_CHECKED);

					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_IGNORE),			ignoreRegex);
					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH),			ignoreRegex);
					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),	ignoreRegex &&
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH)) == BST_CHECKED));

					Edit_Enable(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX_STR), ignoreRegex);
				}
				break;

				case IDC_REGEX_MODE_IGNORE:
				case IDC_REGEX_MODE_MATCH:
					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH)) == BST_CHECKED));
				break;

				default:
				return FALSE;
			}
		}
		break;
	}

	return FALSE;
}


void CompareOptionsDialog::SetParams()
{
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DETECT_MOVES), _Settings->DetectMoves ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_BLOCK_DIFFS),
			_Settings->DetectSubBlockDiffs ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS),
			_Settings->DetectCharDiffs ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_EMPTY_LINES),
			_Settings->IgnoreEmptyLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_FOLDED_LINES),
			_Settings->IgnoreFoldedLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_HIDDEN_LINES),
			_Settings->IgnoreHiddenLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES),
			!_Settings->IgnoreAllSpaces && _Settings->IgnoreChangedSpaces ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES),
			_Settings->IgnoreAllSpaces ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CASE),  _Settings->IgnoreCase  ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX), _Settings->IgnoreRegex ? BST_CHECKED : BST_UNCHECKED);

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_IGNORE), !_Settings->InvertRegex ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH),   _Settings->InvertRegex ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),
			_Settings->InclRegexNomatchLines ? BST_CHECKED : BST_UNCHECKED);

	Button_Enable(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS),	_Settings->DetectSubBlockDiffs);
	Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_IGNORE),	_Settings->IgnoreRegex);
	Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH),	_Settings->IgnoreRegex);
	Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),
			_Settings->IgnoreRegex && _Settings->InvertRegex);

	HWND hCtrl = ::GetDlgItem(_hSelf, IDC_IGNORE_REGEX_STR);

	::SendMessage(hCtrl, EM_SETLIMITTEXT, cMaxRegexLen, 0);

	Edit_SetText(hCtrl, _Settings->IgnoreRegexStr.c_str());
	Edit_Enable(hCtrl, _Settings->IgnoreRegex);
}


bool CompareOptionsDialog::GetParams()
{
	_Settings->DetectMoves			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_MOVES)) == BST_CHECKED);
	_Settings->DetectSubBlockDiffs	=
			(Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_BLOCK_DIFFS)) == BST_CHECKED);
	_Settings->DetectCharDiffs		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS)) == BST_CHECKED);
	_Settings->IgnoreEmptyLines		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_EMPTY_LINES)) == BST_CHECKED);
	_Settings->IgnoreFoldedLines	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_FOLDED_LINES)) == BST_CHECKED);
	_Settings->IgnoreHiddenLines	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_HIDDEN_LINES)) == BST_CHECKED);
	_Settings->IgnoreChangedSpaces	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES)) == BST_CHECKED);
	_Settings->IgnoreAllSpaces		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES)) == BST_CHECKED);
	_Settings->IgnoreCase			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CASE)) == BST_CHECKED);
	_Settings->IgnoreRegex			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX)) == BST_CHECKED);

	if (_Settings->IgnoreRegex)
	{
		HWND hCtrl = ::GetDlgItem(_hSelf, IDC_IGNORE_REGEX_STR);

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

		_Settings->InvertRegex = (Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH)) == BST_CHECKED);

		if (_Settings->InvertRegex)
			_Settings->InclRegexNomatchLines =
					(Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES)) == BST_CHECKED);
	}

	return true;
}


bool CompareOptionsDialog::isRegexValid(const wchar_t* regexStr)
{
	try
	{
		boost::wregex testRegex(regexStr, boost::regex::perl);
	}
	catch (boost::regex_error& err)
	{
		::MessageBoxA(nppData._nppHandle, err.what(), "ComparePlus Bad Regex", MB_OK | MB_ICONWARNING);

		return false;
	}

	return true;
}
