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


#include "SettingsDialog.h"

#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <uxtheme.h>

#include "NppHelpers.h"


static const int c_Highlight_transp_min = 0;
static const int c_Highlight_transp_max = 100;
static const int c_Caret_line_transp_min = 0;
static const int c_Caret_line_transp_max = 100;
static const int c_Threshold_perc_min = 1;
static const int c_Threshold_perc_max = 99;


UINT SettingsDialog::doDialog(UserSettings* settings)
{
	_Settings = settings;

	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK SettingsDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			registerDlgForDarkMode(_hSelf);

			goToCenter();

			::EnableThemeDialogTexture(_hSelf, ETDT_ENABLETAB);

			_ColorComboAdded.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_ADDED_COLOR));
			_ColorComboRemoved.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_REMOVED_COLOR));
			_ColorComboMoved.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_MOVED_COLOR));
			_ColorComboChanged.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_CHANGED_COLOR));
			_ColorComboAddHighlight.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_ADD_HIGHLIGHT_COLOR));
			_ColorComboRemHighlight.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_REM_HIGHLIGHT_COLOR));
			_ColorComboMovHighlight.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_MOV_HIGHLIGHT_COLOR));

			HWND hCtrl = ::GetDlgItem(_hSelf, IDC_HIGHLIGHT_SPIN_BOX);
			::SendMessage(hCtrl, EM_SETLIMITTEXT, 3L, 0);

			hCtrl = ::GetDlgItem(_hSelf, IDC_CARET_LINE_SPIN_BOX);
			::SendMessage(hCtrl, EM_SETLIMITTEXT, 3L, 0);

			hCtrl = ::GetDlgItem(_hSelf, IDC_THRESHOLD_SPIN_BOX);
			::SendMessage(hCtrl, EM_SETLIMITTEXT, 2L, 0);

			hCtrl = ::GetDlgItem(_hSelf, IDC_HIGHLIGHT_SPIN_CTL);
			::SendMessage(hCtrl, UDM_SETRANGE, 0L, MAKELONG(c_Highlight_transp_max, c_Highlight_transp_min));

			hCtrl = ::GetDlgItem(_hSelf, IDC_CARET_LINE_SPIN_CTL);
			::SendMessage(hCtrl, UDM_SETRANGE, 0L, MAKELONG(c_Caret_line_transp_max, c_Caret_line_transp_min));

			hCtrl = ::GetDlgItem(_hSelf, IDC_THRESHOLD_SPIN_CTL);
			::SendMessage(hCtrl, UDM_SETRANGE, 0L, MAKELONG(c_Threshold_perc_max, c_Threshold_perc_min));

			if (isRTLwindow(nppData._nppHandle))
			{
				SetDlgItemText(_hSelf, IDC_NEW_IN_SUB, TEXT("New file in left view"));
				SetDlgItemText(_hSelf, IDC_OLD_IN_SUB, TEXT("Old file in left view"));
			}

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

					settings.FirstFileIsNew			= (bool) DEFAULT_FIRST_IS_NEW;
					settings.NewFileViewId			= DEFAULT_NEW_IN_SUB_VIEW ? SUB_VIEW : MAIN_VIEW;
					settings.CompareToPrev			= (bool) DEFAULT_COMPARE_TO_PREV;
					settings.EncodingsCheck			= (bool) DEFAULT_ENCODINGS_CHECK;
					settings.SizesCheck				= (bool) DEFAULT_SIZES_CHECK;
					settings.NeverMarkIgnored		= (bool) DEFAULT_NEVER_MARK_IGNORED;
					settings.FollowingCaret			= (bool) DEFAULT_FOLLOWING_CARET;
					settings.WrapAround				= (bool) DEFAULT_WRAP_AROUND;
					settings.GotoFirstDiff			= (bool) DEFAULT_GOTO_FIRST_DIFF;
					settings.PromptToCloseOnMatch	= (bool) DEFAULT_PROMPT_CLOSE_ON_MATCH;

					if (isDarkMode())
					{
						settings.useDarkColors();

						settings.colors().added						= DEFAULT_ADDED_COLOR_DARK;
						settings.colors().removed					= DEFAULT_REMOVED_COLOR_DARK;
						settings.colors().moved						= DEFAULT_MOVED_COLOR_DARK;
						settings.colors().changed					= DEFAULT_CHANGED_COLOR_DARK;
						settings.colors().add_highlight				= DEFAULT_HIGHLIGHT_COLOR_DARK;
						settings.colors().rem_highlight				= DEFAULT_HIGHLIGHT_COLOR_DARK;
						settings.colors().mov_highlight				= DEFAULT_HIGHLIGHT_MOVED_COLOR_DARK;
						settings.colors().highlight_transparency	= DEFAULT_HIGHLIGHT_TRANSP_DARK;
						settings.colors().caret_line_transparency	= DEFAULT_CARET_LINE_TRANSP_DARK;
					}
					else
					{
						settings.useLightColors();

						settings.colors().added						= DEFAULT_ADDED_COLOR;
						settings.colors().removed					= DEFAULT_REMOVED_COLOR;
						settings.colors().moved						= DEFAULT_MOVED_COLOR;
						settings.colors().changed					= DEFAULT_CHANGED_COLOR;
						settings.colors().add_highlight				= DEFAULT_HIGHLIGHT_COLOR;
						settings.colors().rem_highlight				= DEFAULT_HIGHLIGHT_COLOR;
						settings.colors().mov_highlight				= DEFAULT_HIGHLIGHT_MOVED_COLOR;
						settings.colors().highlight_transparency	= DEFAULT_HIGHLIGHT_TRANSP;
						settings.colors().caret_line_transparency	= DEFAULT_CARET_LINE_TRANSP;
					}

					settings.ChangedThresholdPercent	= DEFAULT_CHANGED_THRESHOLD;

					settings.EnableToolbar	= (bool) DEFAULT_ENABLE_TOOLBAR_TB;
					settings.SetAsFirstTB	= (bool) DEFAULT_SET_AS_FIRST_TB;
					settings.CompareTB		= (bool) DEFAULT_COMPARE_TB;
					settings.CompareSelTB	= (bool) DEFAULT_COMPARE_SEL_TB;
					settings.ClearCompareTB	= (bool) DEFAULT_CLEAR_COMPARE_TB;
					settings.NavigationTB	= (bool) DEFAULT_NAVIGATION_TB;
					settings.DiffsFilterTB	= (bool) DEFAULT_DIFFS_FILTER_TB;
					settings.NavBarTB		= (bool) DEFAULT_NAV_BAR_TB;

					SetParams(&settings);
				}
				break;

				case IDC_COMBO_ADDED_COLOR:
					_ColorComboAdded.onSelect();
				break;

				case IDC_COMBO_REMOVED_COLOR:
					_ColorComboRemoved.onSelect();
				break;

				case IDC_COMBO_MOVED_COLOR:
					_ColorComboMoved.onSelect();
				break;

				case IDC_COMBO_CHANGED_COLOR:
					_ColorComboChanged.onSelect();
				break;

				case IDC_COMBO_ADD_HIGHLIGHT_COLOR:
					_ColorComboAddHighlight.onSelect();
				break;

				case IDC_COMBO_REM_HIGHLIGHT_COLOR:
					_ColorComboRemHighlight.onSelect();
				break;

				case IDC_COMBO_MOV_HIGHLIGHT_COLOR:
					_ColorComboMovHighlight.onSelect();
				break;

				case IDC_ENABLE_TOOLBAR:
				{
					bool enableToolbar = (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_TOOLBAR)) == BST_CHECKED);

					Button_Enable(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB),		enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_TB),				enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB),	enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB),		enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB),			enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB),		enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB),				enableToolbar);
				}
				break;

				default:
				return FALSE;
			}
		}
		break;

		case WM_NOTIFY:
		{
			if (((LPNMHDR)lParam)->code == UDN_DELTAPOS)
			{
				constexpr int cStep = 5;

				LPNMUPDOWN lpnmud = (LPNMUPDOWN) lParam;

				int newPos = lpnmud->iPos / cStep;

				if (lpnmud->iDelta < 0)
				{
					if ((lpnmud->iPos % cStep) == 0)
						--newPos;
				}
				else
				{
					++newPos;
				}

				lpnmud->iDelta = (newPos * cStep) - lpnmud->iPos;
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

	Button_SetCheck(::GetDlgItem(_hSelf, settings->FirstFileIsNew ? IDC_FIRST_NEW : IDC_FIRST_OLD),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->FirstFileIsNew ? IDC_FIRST_OLD : IDC_FIRST_NEW),
			BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->NewFileViewId == MAIN_VIEW ? IDC_OLD_IN_SUB : IDC_NEW_IN_SUB),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->NewFileViewId == MAIN_VIEW ? IDC_NEW_IN_SUB : IDC_OLD_IN_SUB),
			BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->CompareToPrev ? IDC_COMPARE_TO_PREV : IDC_COMPARE_TO_NEXT),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->CompareToPrev ? IDC_COMPARE_TO_NEXT : IDC_COMPARE_TO_PREV),
			BST_UNCHECKED);

	if (settings->StatusInfo == StatusType::DIFFS_SUMMARY)
	{
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY),	BST_CHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_STATUS_DISABLED),	BST_UNCHECKED);
	}
	else if (settings->StatusInfo == StatusType::COMPARE_OPTIONS)
	{
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS),	BST_CHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_STATUS_DISABLED),	BST_UNCHECKED);
	}
	else
	{
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_STATUS_DISABLED),	BST_CHECKED);
	}

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_ENCODINGS_CHECK),
			settings->EncodingsCheck ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_SIZES_CHECK),
			settings->SizesCheck ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_NEVER_MARK_IGNORED),
			settings->NeverMarkIgnored ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_PROMPT_CLOSE_ON_MATCH),
			settings->PromptToCloseOnMatch ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_WRAP_AROUND),
			settings->WrapAround ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_GOTO_FIRST_DIFF),
			settings->GotoFirstDiff ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_FOLLOWING_CARET),
			settings->FollowingCaret ? BST_CHECKED : BST_UNCHECKED);

	// Set current colors configured in option dialog
	_ColorComboAdded.setColor(settings->colors().added);
	_ColorComboRemoved.setColor(settings->colors().removed);
	_ColorComboMoved.setColor(settings->colors().moved);
	_ColorComboChanged.setColor(settings->colors().changed);
	_ColorComboAddHighlight.setColor(settings->colors().add_highlight);
	_ColorComboRemHighlight.setColor(settings->colors().rem_highlight);
	_ColorComboMovHighlight.setColor(settings->colors().mov_highlight);

	if (settings->colors().highlight_transparency < c_Highlight_transp_min)
		settings->colors().highlight_transparency = c_Highlight_transp_min;
	else if (settings->colors().highlight_transparency > c_Highlight_transp_max)
		settings->colors().highlight_transparency = c_Highlight_transp_max;

	// Set highlight transparency
	::SetDlgItemInt(_hSelf, IDC_HIGHLIGHT_SPIN_BOX, settings->colors().highlight_transparency, FALSE);

	if (settings->colors().caret_line_transparency < c_Caret_line_transp_min)
		settings->colors().caret_line_transparency = c_Caret_line_transp_min;
	else if (settings->colors().caret_line_transparency > c_Caret_line_transp_max)
		settings->colors().caret_line_transparency = c_Caret_line_transp_max;

	// Set caret line transparency
	::SetDlgItemInt(_hSelf, IDC_CARET_LINE_SPIN_BOX, settings->colors().caret_line_transparency, FALSE);

	if (settings->ChangedThresholdPercent < c_Threshold_perc_min)
		settings->ChangedThresholdPercent = c_Threshold_perc_min;
	else if (settings->ChangedThresholdPercent > c_Threshold_perc_max)
		settings->ChangedThresholdPercent = c_Threshold_perc_max;

	// Set changed threshold percentage
	::SetDlgItemInt(_hSelf, IDC_THRESHOLD_SPIN_BOX, settings->ChangedThresholdPercent, FALSE);

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_TOOLBAR),
			settings->EnableToolbar ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB),
			settings->SetAsFirstTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_TB),
			settings->CompareTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB),
			settings->CompareSelTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB),
			settings->ClearCompareTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB),
			settings->NavigationTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB),
			settings->DiffsFilterTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB),
			settings->NavBarTB ? BST_CHECKED : BST_UNCHECKED);

	Button_Enable(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB),		settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_TB),				settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB),	settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB),		settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB),			settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB),		settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB),				settings->EnableToolbar);
}


void SettingsDialog::GetParams()
{
	_Settings->FirstFileIsNew		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_FIRST_NEW)) == BST_CHECKED);
	_Settings->NewFileViewId		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NEW_IN_SUB)) == BST_CHECKED) ?
			SUB_VIEW : MAIN_VIEW;
	_Settings->CompareToPrev		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_TO_PREV)) == BST_CHECKED);

	if (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY)) == BST_CHECKED)
		_Settings->StatusInfo = StatusType::DIFFS_SUMMARY;
	else if (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS)) == BST_CHECKED)
		_Settings->StatusInfo = StatusType::COMPARE_OPTIONS;
	else
		_Settings->StatusInfo = StatusType::STATUS_DISABLED;

	_Settings->EncodingsCheck		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ENCODINGS_CHECK)) == BST_CHECKED);
	_Settings->SizesCheck			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_SIZES_CHECK)) == BST_CHECKED);
	_Settings->NeverMarkIgnored		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NEVER_MARK_IGNORED)) == BST_CHECKED);
	_Settings->PromptToCloseOnMatch	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_PROMPT_CLOSE_ON_MATCH)) == BST_CHECKED);
	_Settings->WrapAround			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_WRAP_AROUND)) == BST_CHECKED);
	_Settings->GotoFirstDiff		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_GOTO_FIRST_DIFF)) == BST_CHECKED);
	_Settings->FollowingCaret		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_FOLLOWING_CARET)) == BST_CHECKED);

	// Get color chosen in dialog
	_ColorComboAdded.getColor((LPCOLORREF)&_Settings->colors().added);
	_ColorComboRemoved.getColor((LPCOLORREF)&_Settings->colors().removed);
	_ColorComboMoved.getColor((LPCOLORREF)&_Settings->colors().moved);
	_ColorComboChanged.getColor((LPCOLORREF)&_Settings->colors().changed);
	_ColorComboAddHighlight.getColor((LPCOLORREF)&_Settings->colors().add_highlight);
	_ColorComboRemHighlight.getColor((LPCOLORREF)&_Settings->colors().rem_highlight);
	_ColorComboMovHighlight.getColor((LPCOLORREF)&_Settings->colors().mov_highlight);

	// Get highlight transparency
	_Settings->colors().highlight_transparency = ::GetDlgItemInt(_hSelf, IDC_HIGHLIGHT_SPIN_BOX, NULL, FALSE);
	int setting = _Settings->colors().highlight_transparency;

	if (_Settings->colors().highlight_transparency < c_Highlight_transp_min)
		_Settings->colors().highlight_transparency = c_Highlight_transp_min;
	else if (_Settings->colors().highlight_transparency > c_Highlight_transp_max)
		_Settings->colors().highlight_transparency = c_Highlight_transp_max;

	if (setting != _Settings->colors().highlight_transparency)
		::SetDlgItemInt(_hSelf, IDC_HIGHLIGHT_SPIN_BOX, _Settings->colors().highlight_transparency, FALSE);

	// Get caret line transparency
	_Settings->colors().caret_line_transparency = ::GetDlgItemInt(_hSelf, IDC_CARET_LINE_SPIN_BOX, NULL, FALSE);
	setting = _Settings->colors().caret_line_transparency;

	if (_Settings->colors().caret_line_transparency < c_Caret_line_transp_min)
		_Settings->colors().caret_line_transparency = c_Caret_line_transp_min;
	else if (_Settings->colors().caret_line_transparency > c_Caret_line_transp_max)
		_Settings->colors().caret_line_transparency = c_Caret_line_transp_max;

	if (setting != _Settings->colors().caret_line_transparency)
		::SetDlgItemInt(_hSelf, IDC_CARET_LINE_SPIN_BOX, _Settings->colors().caret_line_transparency, FALSE);

	// Get changed threshold percentage
	_Settings->ChangedThresholdPercent = ::GetDlgItemInt(_hSelf, IDC_THRESHOLD_SPIN_BOX, NULL, FALSE);
	setting = _Settings->ChangedThresholdPercent;

	if (_Settings->ChangedThresholdPercent < c_Threshold_perc_min)
		_Settings->ChangedThresholdPercent = c_Threshold_perc_min;
	else if (_Settings->ChangedThresholdPercent > c_Threshold_perc_max)
		_Settings->ChangedThresholdPercent = c_Threshold_perc_max;

	if (setting != _Settings->ChangedThresholdPercent)
		::SetDlgItemInt(_hSelf, IDC_THRESHOLD_SPIN_BOX, _Settings->ChangedThresholdPercent, FALSE);

	_Settings->EnableToolbar	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_TOOLBAR)) == BST_CHECKED);
	_Settings->SetAsFirstTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB)) == BST_CHECKED);
	_Settings->CompareTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_TB)) == BST_CHECKED);
	_Settings->CompareSelTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB)) == BST_CHECKED);
	_Settings->ClearCompareTB	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB)) == BST_CHECKED);
	_Settings->NavigationTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB)) == BST_CHECKED);
	_Settings->DiffsFilterTB	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB)) == BST_CHECKED);
	_Settings->NavBarTB			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB)) == BST_CHECKED);
}