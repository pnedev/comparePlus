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

#include "SettingsDialog.h"

#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <uxtheme.h>


typedef HRESULT (WINAPI *ETDTProc) (HWND, DWORD);


UINT SettingsDialog::doDialog(UserSettings* settings)
{
	_Settings = settings;

	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK SettingsDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/)
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

			_ColorComboAdded.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_ADDED_COLOR));
			_ColorComboChanged.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_CHANGED_COLOR));
			_ColorComboMoved.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_MOVED_COLOR));
			_ColorComboRemoved.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_REMOVED_COLOR));
			_ColorComboHighlight.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_HIGHLIGHT_COLOR));

			HWND hUpDown = ::GetDlgItem(_hSelf, IDC_SPIN_CTL);
			::SendMessage(hUpDown, UDM_SETRANGE, 0L, MAKELONG(100, 0));

			SetParams();
		}
		break;

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDOK:
					GetParams();
					_Settings->markAsDirty();
					::EndDialog(_hSelf, IDOK);
				return TRUE;

				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
				return TRUE;

				case IDDEFAULT:
				{
					UserSettings settings;

					settings.OldFileIsFirst			= (bool) DEFAULT_OLD_IS_FIRST;
					settings.OldFileViewId			= DEFAULT_OLD_ON_LEFT ? MAIN_VIEW : SUB_VIEW;
					settings.CompareToPrev			= (bool) DEFAULT_COMPARE_TO_PREV;
					settings.DetectMovesLineMode	= (bool) DEFAULT_DETECT_MOVE_LINE_MODE;
					settings.EncodingsCheck			= (bool) DEFAULT_ENCODINGS_CHECK;
					settings.PromptToCloseOnMatch	= (bool) DEFAULT_PROMPT_CLOSE_ON_MATCH;
					settings.AlignReplacements		= (bool) DEFAULT_ALIGN_REPLACEMENTS;
					settings.WrapAround				= (bool) DEFAULT_WRAP_AROUND;
					settings.RecompareOnSave		= (bool) DEFAULT_RECOMPARE_ON_SAVE;
					settings.GotoFirstDiff			= (bool) DEFAULT_GOTO_FIRST_DIFF;
					settings.UpdateOnChange			= (bool) DEFAULT_UPDATE_ON_CHANGE;

					settings.colors.added     	= DEFAULT_ADDED_COLOR;
					settings.colors.changed   	= DEFAULT_CHANGED_COLOR;
					settings.colors.deleted   	= DEFAULT_DELETED_COLOR;
					settings.colors.moved     	= DEFAULT_MOVED_COLOR;
					settings.colors.highlight 	= DEFAULT_HIGHLIGHT_COLOR;
					settings.colors.alpha     	= DEFAULT_HIGHLIGHT_ALPHA;

					SetParams(&settings);
				}
				break;

				case IDC_COMBO_ADDED_COLOR:
					_ColorComboAdded.onSelect();
				break;

				case IDC_COMBO_CHANGED_COLOR:
					_ColorComboChanged.onSelect();
				break;

				case IDC_COMBO_MOVED_COLOR:
					_ColorComboMoved.onSelect();
				break;

				case IDC_COMBO_REMOVED_COLOR:
					_ColorComboRemoved.onSelect();
				break;

				case IDC_COMBO_HIGHLIGHT_COLOR:
					_ColorComboHighlight.onSelect();
				break;

				default:
				return FALSE;
			}
		}
		break;
	}

	return FALSE;
}


void SettingsDialog::SetParams(UserSettings* settings)
{
	if (settings == nullptr)
		settings = _Settings;

	Button_SetCheck(::GetDlgItem(_hSelf, settings->OldFileIsFirst ? IDC_FIRST_OLD : IDC_FIRST_NEW), BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->OldFileIsFirst ? IDC_FIRST_NEW : IDC_FIRST_OLD), BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->OldFileViewId == MAIN_VIEW ? IDC_OLD_LEFT : IDC_OLD_RIGHT),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->OldFileViewId == MAIN_VIEW ? IDC_OLD_RIGHT : IDC_OLD_LEFT),
			BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->CompareToPrev ? IDC_COMPARE_TO_PREV : IDC_COMPARE_TO_NEXT),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->CompareToPrev ? IDC_COMPARE_TO_NEXT : IDC_COMPARE_TO_PREV),
			BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->DetectMovesLineMode ? IDC_MOVE_LINE_BASED : IDC_MOVE_BLOCK_BASED),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->DetectMovesLineMode ? IDC_MOVE_BLOCK_BASED : IDC_MOVE_LINE_BASED),
			BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_ENCODING_CHECK),
			settings->EncodingsCheck ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_PROMPT_CLOSE_ON_MATCH),
			settings->PromptToCloseOnMatch ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_ALIGN_REPLACEMENTS),
			settings->AlignReplacements ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_WRAP_AROUND),
			settings->WrapAround ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_RECOMPARE_ON_SAVE),
			settings->RecompareOnSave ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_GOTO_FIRST_DIFF),
			settings->GotoFirstDiff ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_UPDATE_ON_CHANGE),
			settings->UpdateOnChange ? BST_CHECKED : BST_UNCHECKED);

	// Set current colors configured in option dialog
	_ColorComboAdded.setColor(settings->colors.added);
	_ColorComboMoved.setColor(settings->colors.moved);
	_ColorComboRemoved.setColor(settings->colors.deleted);
	_ColorComboChanged.setColor(settings->colors.changed);
	_ColorComboHighlight.setColor(settings->colors.highlight);

	// Set transparency
	::SetDlgItemInt(_hSelf, IDC_SPIN_BOX, settings->colors.alpha, FALSE);
}


void SettingsDialog::GetParams()
{
	_Settings->OldFileIsFirst		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_FIRST_OLD)) == BST_CHECKED);
	_Settings->OldFileViewId		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_OLD_LEFT)) == BST_CHECKED) ?
			MAIN_VIEW : SUB_VIEW;
	_Settings->CompareToPrev		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_TO_PREV)) == BST_CHECKED);
	_Settings->DetectMovesLineMode	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_MOVE_LINE_BASED)) == BST_CHECKED);
	_Settings->EncodingsCheck		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_ENCODING_CHECK)) == BST_CHECKED);
	_Settings->PromptToCloseOnMatch	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_PROMPT_CLOSE_ON_MATCH)) == BST_CHECKED);
	_Settings->AlignReplacements	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ALIGN_REPLACEMENTS)) == BST_CHECKED);
	_Settings->WrapAround			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_WRAP_AROUND)) == BST_CHECKED);
	_Settings->RecompareOnSave		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_RECOMPARE_ON_SAVE)) == BST_CHECKED);
	_Settings->GotoFirstDiff		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_GOTO_FIRST_DIFF)) == BST_CHECKED);
	_Settings->UpdateOnChange		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_UPDATE_ON_CHANGE)) == BST_CHECKED);

	// Get color chosen in dialog
	_ColorComboAdded.getColor((LPCOLORREF)&_Settings->colors.added);
	_ColorComboMoved.getColor((LPCOLORREF)&_Settings->colors.moved);
	_ColorComboRemoved.getColor((LPCOLORREF)&_Settings->colors.deleted);
	_ColorComboChanged.getColor((LPCOLORREF)&_Settings->colors.changed);
	_ColorComboHighlight.getColor((LPCOLORREF)&_Settings->colors.highlight);

	// Get transparency
	_Settings->colors.alpha = ::GetDlgItemInt(_hSelf, IDC_SPIN_BOX, NULL, FALSE);
}