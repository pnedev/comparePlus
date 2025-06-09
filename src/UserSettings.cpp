/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
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

#include "Compare.h"
#include "UserSettings.h"

#include <shlwapi.h>
#include <cstdlib>


const TCHAR UserSettings::mainSection[]						= TEXT("main_settings");

const TCHAR UserSettings::newFileViewSetting[]				= TEXT("new_in_sub_view");
const TCHAR UserSettings::firstIsNewSetting[]				= TEXT("set_first_as_new");
const TCHAR UserSettings::compareToPrevSetting[]			= TEXT("default_compare_to_prev");

const TCHAR UserSettings::encodingsCheckSetting[]			= TEXT("check_encodings");
const TCHAR UserSettings::sizesCheckSetting[]				= TEXT("check_sizes");
const TCHAR UserSettings::markIgnoredLinesSetting[]			= TEXT("never_colorize_ignored_lines");
const TCHAR UserSettings::promptCloseOnMatchSetting[]		= TEXT("prompt_to_close_on_match");
const TCHAR UserSettings::wrapAroundSetting[]				= TEXT("wrap_around");
const TCHAR UserSettings::gotoFirstDiffSetting[]			= TEXT("go_to_first_on_recompare");
const TCHAR UserSettings::followingCaretSetting[]			= TEXT("following_caret");

const TCHAR UserSettings::detectMovesSetting[]				= TEXT("detect_moves");
const TCHAR UserSettings::detectSubBlockDiffsSetting[]		= TEXT("detect_sub_block_diffs");
const TCHAR UserSettings::detectSubLineMovesSetting[]		= TEXT("detect_sub_line_moves");
const TCHAR UserSettings::detectCharDiffsSetting[]			= TEXT("detect_character_diffs");
const TCHAR UserSettings::ignoreEmptyLinesSetting[]			= TEXT("ignore_empty_lines");
const TCHAR UserSettings::ignoreFoldedLinesSetting[]		= TEXT("ignore_folded_lines");
const TCHAR UserSettings::ignoreHiddenLinesSetting[]		= TEXT("ignore_hidden_lines");
const TCHAR UserSettings::ignoreChangedSpacesSetting[]		= TEXT("ignore_changed_spaces");
const TCHAR UserSettings::ignoreAllSpacesSetting[]			= TEXT("ignore_all_spaces");
const TCHAR UserSettings::ignoreCaseSetting[]				= TEXT("ignore_case");
const TCHAR UserSettings::ignoreRegexSetting[]				= TEXT("ignore_regex");
const TCHAR UserSettings::invertRegexSetting[]				= TEXT("invert_regex");
const TCHAR UserSettings::inclRegexNomatchLinesSetting[]	= TEXT("incl_regex_nomatch_lines");
const TCHAR UserSettings::ignoreRegexStrSetting[]			= TEXT("ignore_regex_string");
const TCHAR UserSettings::hideMatchesSetting[]				= TEXT("hide_matches");
const TCHAR UserSettings::hideNewLinesSetting[]				= TEXT("hide_added_removed_lines");
const TCHAR UserSettings::hideChangedLinesSetting[]			= TEXT("hide_changed_lines");
const TCHAR UserSettings::hideMovedLinesSetting[]			= TEXT("hide_moved_lines");
const TCHAR UserSettings::showOnlySelSetting[]				= TEXT("show_only_selections");
const TCHAR UserSettings::navBarSetting[]					= TEXT("navigation_bar");

const TCHAR UserSettings::reCompareOnChangeSetting[]		= TEXT("recompare_on_change");

const TCHAR UserSettings::statusInfoSetting[]				= TEXT("status_info");

const TCHAR UserSettings::colorsSection[]					= TEXT("color_settings");

const TCHAR UserSettings::addedColorSetting[]				= TEXT("added");
const TCHAR UserSettings::removedColorSetting[]				= TEXT("removed");
const TCHAR UserSettings::movedColorSetting[]				= TEXT("moved");
const TCHAR UserSettings::changedColorSetting[]				= TEXT("changed");
const TCHAR UserSettings::addHighlightColorSetting[]		= TEXT("added_highlight");
const TCHAR UserSettings::remHighlightColorSetting[]		= TEXT("removed_highlight");
const TCHAR UserSettings::movHighlightColorSetting[]		= TEXT("moved_highlight");
const TCHAR UserSettings::highlightTranspSetting[]			= TEXT("highlight_transparency");
const TCHAR UserSettings::caretLineTranspSetting[]			= TEXT("caret_line_transparency");

const TCHAR UserSettings::addedColorDarkSetting[]			= TEXT("added_dark");
const TCHAR UserSettings::removedColorDarkSetting[]			= TEXT("removed_dark");
const TCHAR UserSettings::movedColorDarkSetting[]			= TEXT("moved_dark");
const TCHAR UserSettings::changedColorDarkSetting[]			= TEXT("changed_dark");
const TCHAR UserSettings::addHighlightColorDarkSetting[]	= TEXT("added_highlight_dark");
const TCHAR UserSettings::remHighlightColorDarkSetting[]	= TEXT("removed_highlight_dark");
const TCHAR UserSettings::movHighlightColorDarkSetting[]	= TEXT("moved_highlight_dark");
const TCHAR UserSettings::highlightTranspDarkSetting[]		= TEXT("highlight_transparency_dark");
const TCHAR UserSettings::caretLineTranspDarkSetting[]		= TEXT("caret_line_transparency_dark");

const TCHAR UserSettings::changedThresholdSetting[]			= TEXT("changed_threshold_percentage");

const TCHAR UserSettings::toolbarSection[]					= TEXT("toolbar_settings");

const TCHAR UserSettings::enableToolbarSetting[]			= TEXT("enable_toolbar");
const TCHAR UserSettings::setAsFirstTBSetting[]				= TEXT("set_as_first_tb");
const TCHAR UserSettings::compareTBSetting[]				= TEXT("compare_tb");
const TCHAR UserSettings::compareSelTBSetting[]				= TEXT("compare_selection_tb");
const TCHAR UserSettings::clearCompareTBSetting[]			= TEXT("clear_compare_tb");
const TCHAR UserSettings::navigationTBSetting[]				= TEXT("navigation_tb");
const TCHAR UserSettings::diffsFilterTBSetting[]			= TEXT("diffs_filter_tb");
const TCHAR UserSettings::navBarTBSetting[]					= TEXT("nav_bar_tb");


void UserSettings::load()
{
	TCHAR iniFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFile), (LPARAM)iniFile);

	::PathAppend(iniFile, TEXT("ComparePlus.ini"));

	NewFileViewId			= ::GetPrivateProfileInt(mainSection, newFileViewSetting,
			DEFAULT_NEW_IN_SUB_VIEW, iniFile) == 0 ? MAIN_VIEW : SUB_VIEW;
	FirstFileIsNew			= ::GetPrivateProfileInt(mainSection, firstIsNewSetting,
			DEFAULT_FIRST_IS_NEW, iniFile) != 0;
	CompareToPrev			= ::GetPrivateProfileInt(mainSection, compareToPrevSetting,
			DEFAULT_COMPARE_TO_PREV, iniFile) != 0;
	EncodingsCheck			= ::GetPrivateProfileInt(mainSection, encodingsCheckSetting,
			DEFAULT_ENCODINGS_CHECK, iniFile) != 0;
	SizesCheck				= ::GetPrivateProfileInt(mainSection, sizesCheckSetting,
			DEFAULT_SIZES_CHECK, iniFile) != 0;
	NeverMarkIgnored		= ::GetPrivateProfileInt(mainSection, markIgnoredLinesSetting,
			DEFAULT_NEVER_MARK_IGNORED, iniFile) != 0;
	FollowingCaret			= ::GetPrivateProfileInt(mainSection, followingCaretSetting,
			DEFAULT_FOLLOWING_CARET, iniFile) != 0;
	WrapAround				= ::GetPrivateProfileInt(mainSection, wrapAroundSetting,
			DEFAULT_WRAP_AROUND, iniFile) != 0;
	GotoFirstDiff			= ::GetPrivateProfileInt(mainSection, gotoFirstDiffSetting,
			DEFAULT_GOTO_FIRST_DIFF, iniFile) != 0;
	PromptToCloseOnMatch	= ::GetPrivateProfileInt(mainSection, promptCloseOnMatchSetting,
			DEFAULT_PROMPT_CLOSE_ON_MATCH, iniFile) != 0;

	DetectMoves				= ::GetPrivateProfileInt(mainSection, detectMovesSetting,			1, iniFile) != 0;
	DetectSubBlockDiffs		= ::GetPrivateProfileInt(mainSection, detectSubBlockDiffsSetting,	1, iniFile) != 0;
	DetectSubLineMoves		= ::GetPrivateProfileInt(mainSection, detectSubLineMovesSetting,	1, iniFile) != 0;
	DetectCharDiffs			= ::GetPrivateProfileInt(mainSection, detectCharDiffsSetting,		0, iniFile) != 0;
	IgnoreEmptyLines		= ::GetPrivateProfileInt(mainSection, ignoreEmptyLinesSetting,		0, iniFile) != 0;
	IgnoreFoldedLines		= ::GetPrivateProfileInt(mainSection, ignoreFoldedLinesSetting,		0, iniFile) != 0;
	IgnoreHiddenLines		= ::GetPrivateProfileInt(mainSection, ignoreHiddenLinesSetting,		0, iniFile) != 0;
	IgnoreChangedSpaces		= ::GetPrivateProfileInt(mainSection, ignoreChangedSpacesSetting,	0, iniFile) != 0;
	IgnoreAllSpaces			= ::GetPrivateProfileInt(mainSection, ignoreAllSpacesSetting,		0, iniFile) != 0;
	IgnoreCase				= ::GetPrivateProfileInt(mainSection, ignoreCaseSetting,			0, iniFile) != 0;
	IgnoreRegex				= ::GetPrivateProfileInt(mainSection, ignoreRegexSetting,			0, iniFile) != 0;
	InvertRegex				= ::GetPrivateProfileInt(mainSection, invertRegexSetting,			0, iniFile) != 0;
	InclRegexNomatchLines	= ::GetPrivateProfileInt(mainSection, inclRegexNomatchLinesSetting,	0, iniFile) != 0;

	TCHAR buf[1024];

	::GetPrivateProfileString(mainSection, ignoreRegexStrSetting, NULL, buf, _countof(buf), iniFile);

	IgnoreRegexStr = buf;

	HideMatches			= ::GetPrivateProfileInt(mainSection, hideMatchesSetting,			0, iniFile) != 0;
	HideNewLines		= ::GetPrivateProfileInt(mainSection, hideNewLinesSetting,			0, iniFile) != 0;
	HideChangedLines	= ::GetPrivateProfileInt(mainSection, hideChangedLinesSetting,		0, iniFile) != 0;
	HideMovedLines		= ::GetPrivateProfileInt(mainSection, hideMovedLinesSetting,		0, iniFile) != 0;
	ShowOnlySelections	= ::GetPrivateProfileInt(mainSection, showOnlySelSetting,			1, iniFile) != 0;

	UseNavBar			= ::GetPrivateProfileInt(mainSection, navBarSetting,				1, iniFile) != 0;
	RecompareOnChange	= ::GetPrivateProfileInt(mainSection, reCompareOnChangeSetting,		1, iniFile) != 0;

	StatusInfo = static_cast<StatusType>(::GetPrivateProfileInt(mainSection, statusInfoSetting,
			DEFAULT_STATUS_INFO, iniFile));

	if (StatusInfo >= STATUS_TYPE_END)
		StatusInfo = static_cast<StatusType>(DEFAULT_STATUS_INFO);

	colorsLight.added						= ::GetPrivateProfileInt(colorsSection, addedColorSetting,
			DEFAULT_ADDED_COLOR, iniFile);
	colorsLight.removed						= ::GetPrivateProfileInt(colorsSection, removedColorSetting,
			DEFAULT_REMOVED_COLOR, iniFile);
	colorsLight.moved						= ::GetPrivateProfileInt(colorsSection, movedColorSetting,
			DEFAULT_MOVED_COLOR, iniFile);
	colorsLight.changed						= ::GetPrivateProfileInt(colorsSection, changedColorSetting,
			DEFAULT_CHANGED_COLOR, iniFile);
	colorsLight.add_highlight				= ::GetPrivateProfileInt(colorsSection, addHighlightColorSetting,
			DEFAULT_HIGHLIGHT_COLOR, iniFile);
	colorsLight.rem_highlight				= ::GetPrivateProfileInt(colorsSection, remHighlightColorSetting,
			DEFAULT_HIGHLIGHT_COLOR, iniFile);
	colorsLight.mov_highlight				= ::GetPrivateProfileInt(colorsSection, movHighlightColorSetting,
			DEFAULT_HIGHLIGHT_MOVED_COLOR, iniFile);
	colorsLight.highlight_transparency		= ::GetPrivateProfileInt(colorsSection, highlightTranspSetting,
			DEFAULT_HIGHLIGHT_TRANSP, iniFile);
	colorsLight.caret_line_transparency		= ::GetPrivateProfileInt(colorsSection, caretLineTranspSetting,
			DEFAULT_CARET_LINE_TRANSP, iniFile);

	colorsDark.added						= ::GetPrivateProfileInt(colorsSection, addedColorDarkSetting,
			DEFAULT_ADDED_COLOR_DARK, iniFile);
	colorsDark.removed						= ::GetPrivateProfileInt(colorsSection, removedColorDarkSetting,
			DEFAULT_REMOVED_COLOR_DARK, iniFile);
	colorsDark.moved						= ::GetPrivateProfileInt(colorsSection, movedColorDarkSetting,
			DEFAULT_MOVED_COLOR_DARK, iniFile);
	colorsDark.changed						= ::GetPrivateProfileInt(colorsSection, changedColorDarkSetting,
			DEFAULT_CHANGED_COLOR_DARK, iniFile);
	colorsDark.add_highlight				= ::GetPrivateProfileInt(colorsSection, addHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_COLOR_DARK, iniFile);
	colorsDark.rem_highlight				= ::GetPrivateProfileInt(colorsSection, remHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_COLOR_DARK, iniFile);
	colorsDark.mov_highlight				= ::GetPrivateProfileInt(colorsSection, movHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_MOVED_COLOR_DARK, iniFile);
	colorsDark.highlight_transparency		= ::GetPrivateProfileInt(colorsSection, highlightTranspDarkSetting,
			DEFAULT_HIGHLIGHT_TRANSP_DARK, iniFile);
	colorsDark.caret_line_transparency		= ::GetPrivateProfileInt(colorsSection, caretLineTranspDarkSetting,
			DEFAULT_CARET_LINE_TRANSP_DARK, iniFile);

	ChangedThresholdPercent	= ::GetPrivateProfileInt(colorsSection, changedThresholdSetting,
			DEFAULT_CHANGED_THRESHOLD, iniFile);

	EnableToolbar		= ::GetPrivateProfileInt(toolbarSection, enableToolbarSetting,
			DEFAULT_ENABLE_TOOLBAR_TB, iniFile) != 0;
	SetAsFirstTB		= ::GetPrivateProfileInt(toolbarSection, setAsFirstTBSetting,
			DEFAULT_SET_AS_FIRST_TB, iniFile) != 0;
	CompareTB			= ::GetPrivateProfileInt(toolbarSection, compareTBSetting,
			DEFAULT_COMPARE_TB, iniFile) != 0;
	CompareSelTB		= ::GetPrivateProfileInt(toolbarSection, compareSelTBSetting,
			DEFAULT_COMPARE_SEL_TB, iniFile) != 0;
	ClearCompareTB		= ::GetPrivateProfileInt(toolbarSection, clearCompareTBSetting,
			DEFAULT_CLEAR_COMPARE_TB, iniFile) != 0;
	NavigationTB		= ::GetPrivateProfileInt(toolbarSection, navigationTBSetting,
			DEFAULT_NAVIGATION_TB, iniFile) != 0;
	DiffsFilterTB		= ::GetPrivateProfileInt(toolbarSection, diffsFilterTBSetting,
			DEFAULT_DIFFS_FILTER_TB, iniFile) != 0;
	NavBarTB			= ::GetPrivateProfileInt(toolbarSection, navBarTBSetting,
			DEFAULT_NAV_BAR_TB, iniFile) != 0;

	dirty = false;
}


void UserSettings::save()
{
	if (!dirty)
		return;

	TCHAR iniFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFile), (LPARAM)iniFile);

	if (::PathFileExists(iniFile) == FALSE)
	{
		if (::CreateDirectory(iniFile, NULL) == FALSE)
		{
			TCHAR msg[MAX_PATH + 128];

			_sntprintf_s(msg, _countof(msg), _TRUNCATE,
					TEXT("Notepad++ plugins config folder\n'%s'\ndoesn't exist and failed to be created.")
					TEXT("\nCannot write configuration file."), iniFile);
			::MessageBox(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

			return;
		}
	}

	::PathAppend(iniFile, TEXT("ComparePlus.ini"));

	// Make sure the ini config file has UNICODE encoding to be able to store ignore regex Unicode string
	{
		FILE* fp;

		_wfopen_s(&fp, iniFile, L"w, ccs=UNICODE");

		if (fp)
			fclose(fp);
	}

	if (!::WritePrivateProfileString(mainSection, newFileViewSetting,
		NewFileViewId == SUB_VIEW ? TEXT("1") : TEXT("0"), iniFile))
	{
		TCHAR msg[MAX_PATH + 64];

		_sntprintf_s(msg, _countof(msg), _TRUNCATE, TEXT("Failed to write\n'%s'\nconfiguration file."), iniFile);
		::MessageBox(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

		return;
	}

	::WritePrivateProfileString(mainSection, firstIsNewSetting,
			FirstFileIsNew ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, compareToPrevSetting,
			CompareToPrev ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, encodingsCheckSetting,
			EncodingsCheck ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, sizesCheckSetting,
			SizesCheck ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, markIgnoredLinesSetting,
			NeverMarkIgnored ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, followingCaretSetting,
			FollowingCaret ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, wrapAroundSetting,
			WrapAround ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, gotoFirstDiffSetting,
			GotoFirstDiff ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, promptCloseOnMatchSetting,
			PromptToCloseOnMatch ? TEXT("1") : TEXT("0"), iniFile);

	::WritePrivateProfileString(mainSection, detectMovesSetting,
			DetectMoves ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, detectSubBlockDiffsSetting,
			DetectSubBlockDiffs ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, detectSubLineMovesSetting,
			DetectSubLineMoves ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, detectCharDiffsSetting,
			DetectCharDiffs ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreEmptyLinesSetting,
			IgnoreEmptyLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreFoldedLinesSetting,
			IgnoreFoldedLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreHiddenLinesSetting,
			IgnoreHiddenLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreChangedSpacesSetting,
			IgnoreChangedSpaces ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreAllSpacesSetting,
			IgnoreAllSpaces ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreCaseSetting,
			IgnoreCase ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreRegexSetting,
			IgnoreRegex ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, invertRegexSetting,
			InvertRegex ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, inclRegexNomatchLinesSetting,
			InclRegexNomatchLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreRegexStrSetting,
			IgnoreRegexStr.c_str(), iniFile);
	::WritePrivateProfileString(mainSection, hideMatchesSetting,
			HideMatches ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, hideNewLinesSetting,
			HideNewLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, hideChangedLinesSetting,
			HideChangedLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, hideMovedLinesSetting,
			HideMovedLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, showOnlySelSetting,
			ShowOnlySelections ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, navBarSetting,
			UseNavBar ? TEXT("1") : TEXT("0"), iniFile);

	::WritePrivateProfileString(mainSection, reCompareOnChangeSetting,
			RecompareOnChange ? TEXT("1") : TEXT("0"), iniFile);

	TCHAR buffer[64];

	_itot_s(static_cast<int>(StatusInfo), buffer, 64, 10);
	::WritePrivateProfileString(mainSection, statusInfoSetting, buffer, iniFile);

	_itot_s(colorsLight.added, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addedColorSetting, buffer, iniFile);

	_itot_s(colorsLight.removed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, removedColorSetting, buffer, iniFile);

	_itot_s(colorsLight.moved, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movedColorSetting, buffer, iniFile);

	_itot_s(colorsLight.changed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedColorSetting, buffer, iniFile);

	_itot_s(colorsLight.add_highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addHighlightColorSetting, buffer, iniFile);

	_itot_s(colorsLight.rem_highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, remHighlightColorSetting, buffer, iniFile);

	_itot_s(colorsLight.mov_highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movHighlightColorSetting, buffer, iniFile);

	_itot_s(colorsLight.highlight_transparency, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightTranspSetting, buffer, iniFile);

	_itot_s(colorsLight.caret_line_transparency, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, caretLineTranspSetting, buffer, iniFile);

	_itot_s(colorsDark.added, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addedColorDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.removed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, removedColorDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.moved, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movedColorDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.changed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedColorDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.add_highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addHighlightColorDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.rem_highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, remHighlightColorDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.mov_highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movHighlightColorDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.highlight_transparency, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightTranspDarkSetting, buffer, iniFile);

	_itot_s(colorsDark.caret_line_transparency, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, caretLineTranspDarkSetting, buffer, iniFile);

	_itot_s(ChangedThresholdPercent, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedThresholdSetting, buffer, iniFile);

	::WritePrivateProfileString(toolbarSection, enableToolbarSetting,
			EnableToolbar ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(toolbarSection, setAsFirstTBSetting,
			SetAsFirstTB ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(toolbarSection, compareTBSetting,
			CompareTB ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(toolbarSection, compareSelTBSetting,
			CompareSelTB ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(toolbarSection, clearCompareTBSetting,
			ClearCompareTB ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(toolbarSection, navigationTBSetting,
			NavigationTB ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(toolbarSection, diffsFilterTBSetting,
			DiffsFilterTB ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(toolbarSection, navBarTBSetting,
			NavBarTB ? TEXT("1") : TEXT("0"), iniFile);

	dirty = false;
}
