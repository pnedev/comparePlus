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


const wchar_t UserSettings::mainSection[]					= L"main_settings";

const wchar_t UserSettings::newFileViewSetting[]			= L"new_in_sub_view";
const wchar_t UserSettings::firstIsNewSetting[]				= L"set_first_as_new";
const wchar_t UserSettings::compareToPrevSetting[]			= L"default_compare_to_prev";

const wchar_t UserSettings::encodingsCheckSetting[]			= L"check_encodings";
const wchar_t UserSettings::sizesCheckSetting[]				= L"check_sizes";
const wchar_t UserSettings::markIgnoredLinesSetting[]		= L"never_colorize_ignored_lines";
const wchar_t UserSettings::promptCloseOnMatchSetting[]		= L"prompt_to_close_on_match";
const wchar_t UserSettings::wrapAroundSetting[]				= L"wrap_around";
const wchar_t UserSettings::gotoFirstDiffSetting[]			= L"go_to_first_on_recompare";
const wchar_t UserSettings::followingCaretSetting[]			= L"following_caret";

const wchar_t UserSettings::detectMovesSetting[]			= L"detect_moves";
const wchar_t UserSettings::detectSubBlockDiffsSetting[]	= L"detect_sub_block_diffs";
const wchar_t UserSettings::detectSubLineMovesSetting[]		= L"detect_sub_line_moves";
const wchar_t UserSettings::detectCharDiffsSetting[]		= L"detect_character_diffs";
const wchar_t UserSettings::ignoreEmptyLinesSetting[]		= L"ignore_empty_lines";
const wchar_t UserSettings::ignoreFoldedLinesSetting[]		= L"ignore_folded_lines";
const wchar_t UserSettings::ignoreHiddenLinesSetting[]		= L"ignore_hidden_lines";
const wchar_t UserSettings::ignoreChangedSpacesSetting[]	= L"ignore_changed_spaces";
const wchar_t UserSettings::ignoreAllSpacesSetting[]		= L"ignore_all_spaces";
const wchar_t UserSettings::ignoreEOLSetting[]				= L"ignore_eol";
const wchar_t UserSettings::ignoreCaseSetting[]				= L"ignore_case";
const wchar_t UserSettings::ignoreRegexSetting[]			= L"ignore_regex";
const wchar_t UserSettings::invertRegexSetting[]			= L"invert_regex";
const wchar_t UserSettings::inclRegexNomatchLinesSetting[]	= L"incl_regex_nomatch_lines";
const wchar_t UserSettings::ignoreRegexStrSetting[]			= L"ignore_regex_string";
const wchar_t UserSettings::hideMatchesSetting[]			= L"hide_matches";
const wchar_t UserSettings::hideNewLinesSetting[]			= L"hide_added_removed_lines";
const wchar_t UserSettings::hideChangedLinesSetting[]		= L"hide_changed_lines";
const wchar_t UserSettings::hideMovedLinesSetting[]			= L"hide_moved_lines";
const wchar_t UserSettings::showOnlySelSetting[]			= L"show_only_selections";
const wchar_t UserSettings::navBarSetting[]					= L"navigation_bar";

const wchar_t UserSettings::reCompareOnChangeSetting[]		= L"recompare_on_change";

const wchar_t UserSettings::statusInfoSetting[]				= L"status_info";

const wchar_t UserSettings::colorsSection[]					= L"color_settings";

const wchar_t UserSettings::addedColorSetting[]				= L"added";
const wchar_t UserSettings::removedColorSetting[]			= L"removed";
const wchar_t UserSettings::movedColorSetting[]				= L"moved";
const wchar_t UserSettings::changedColorSetting[]			= L"changed";
const wchar_t UserSettings::addHighlightColorSetting[]		= L"added_highlight";
const wchar_t UserSettings::remHighlightColorSetting[]		= L"removed_highlight";
const wchar_t UserSettings::movHighlightColorSetting[]		= L"moved_highlight";
const wchar_t UserSettings::highlightTranspSetting[]		= L"highlight_transparency";
const wchar_t UserSettings::caretLineTranspSetting[]		= L"caret_line_transparency";

const wchar_t UserSettings::addedColorDarkSetting[]			= L"added_dark";
const wchar_t UserSettings::removedColorDarkSetting[]		= L"removed_dark";
const wchar_t UserSettings::movedColorDarkSetting[]			= L"moved_dark";
const wchar_t UserSettings::changedColorDarkSetting[]		= L"changed_dark";
const wchar_t UserSettings::addHighlightColorDarkSetting[]	= L"added_highlight_dark";
const wchar_t UserSettings::remHighlightColorDarkSetting[]	= L"removed_highlight_dark";
const wchar_t UserSettings::movHighlightColorDarkSetting[]	= L"moved_highlight_dark";
const wchar_t UserSettings::highlightTranspDarkSetting[]	= L"highlight_transparency_dark";
const wchar_t UserSettings::caretLineTranspDarkSetting[]	= L"caret_line_transparency_dark";

const wchar_t UserSettings::changedThresholdSetting[]		= L"changed_threshold_percentage";

const wchar_t UserSettings::toolbarSection[]				= L"toolbar_settings";

const wchar_t UserSettings::enableToolbarSetting[]			= L"enable_toolbar";
const wchar_t UserSettings::setAsFirstTBSetting[]			= L"set_as_first_tb";
const wchar_t UserSettings::compareTBSetting[]				= L"compare_tb";
const wchar_t UserSettings::compareSelTBSetting[]			= L"compare_selection_tb";
const wchar_t UserSettings::clearCompareTBSetting[]			= L"clear_compare_tb";
const wchar_t UserSettings::navigationTBSetting[]			= L"navigation_tb";
const wchar_t UserSettings::diffsFilterTBSetting[]			= L"diffs_filter_tb";
const wchar_t UserSettings::navBarTBSetting[]				= L"nav_bar_tb";


void UserSettings::load()
{
	wchar_t ini[MAX_PATH];

	::SendMessageW(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(ini), (LPARAM)ini);

	::PathAppendW(ini, L"ComparePlus.ini");

	NewFileViewId			= ::GetPrivateProfileIntW(mainSection, newFileViewSetting,
			DEFAULT_NEW_IN_SUB_VIEW, ini) == 0 ? MAIN_VIEW : SUB_VIEW;
	FirstFileIsNew			= ::GetPrivateProfileIntW(mainSection, firstIsNewSetting,
			DEFAULT_FIRST_IS_NEW, ini) != 0;
	CompareToPrev			= ::GetPrivateProfileIntW(mainSection, compareToPrevSetting,
			DEFAULT_COMPARE_TO_PREV, ini) != 0;
	EncodingsCheck			= ::GetPrivateProfileIntW(mainSection, encodingsCheckSetting,
			DEFAULT_ENCODINGS_CHECK, ini) != 0;
	SizesCheck				= ::GetPrivateProfileIntW(mainSection, sizesCheckSetting,
			DEFAULT_SIZES_CHECK, ini) != 0;
	NeverMarkIgnored		= ::GetPrivateProfileIntW(mainSection, markIgnoredLinesSetting,
			DEFAULT_NEVER_MARK_IGNORED, ini) != 0;
	FollowingCaret			= ::GetPrivateProfileIntW(mainSection, followingCaretSetting,
			DEFAULT_FOLLOWING_CARET, ini) != 0;
	WrapAround				= ::GetPrivateProfileIntW(mainSection, wrapAroundSetting,
			DEFAULT_WRAP_AROUND, ini) != 0;
	GotoFirstDiff			= ::GetPrivateProfileIntW(mainSection, gotoFirstDiffSetting,
			DEFAULT_GOTO_FIRST_DIFF, ini) != 0;
	PromptToCloseOnMatch	= ::GetPrivateProfileIntW(mainSection, promptCloseOnMatchSetting,
			DEFAULT_PROMPT_CLOSE_ON_MATCH, ini) != 0;

	DetectMoves				= ::GetPrivateProfileIntW(mainSection, detectMovesSetting,			 1, ini) != 0;
	DetectSubBlockDiffs		= ::GetPrivateProfileIntW(mainSection, detectSubBlockDiffsSetting,	 1, ini) != 0;
	DetectSubLineMoves		= ::GetPrivateProfileIntW(mainSection, detectSubLineMovesSetting,	 1, ini) != 0;
	DetectCharDiffs			= ::GetPrivateProfileIntW(mainSection, detectCharDiffsSetting,		 0, ini) != 0;
	IgnoreEmptyLines		= ::GetPrivateProfileIntW(mainSection, ignoreEmptyLinesSetting,		 0, ini) != 0;
	IgnoreFoldedLines		= ::GetPrivateProfileIntW(mainSection, ignoreFoldedLinesSetting,	 0, ini) != 0;
	IgnoreHiddenLines		= ::GetPrivateProfileIntW(mainSection, ignoreHiddenLinesSetting,	 0, ini) != 0;
	IgnoreChangedSpaces		= ::GetPrivateProfileIntW(mainSection, ignoreChangedSpacesSetting,	 0, ini) != 0;
	IgnoreAllSpaces			= ::GetPrivateProfileIntW(mainSection, ignoreAllSpacesSetting,		 0, ini) != 0;
	IgnoreEOL				= ::GetPrivateProfileIntW(mainSection, ignoreEOLSetting,			 0, ini) != 0;
	IgnoreCase				= ::GetPrivateProfileIntW(mainSection, ignoreCaseSetting,			 0, ini) != 0;
	IgnoreRegex				= ::GetPrivateProfileIntW(mainSection, ignoreRegexSetting,			 0, ini) != 0;
	InvertRegex				= ::GetPrivateProfileIntW(mainSection, invertRegexSetting,			 0, ini) != 0;
	InclRegexNomatchLines	= ::GetPrivateProfileIntW(mainSection, inclRegexNomatchLinesSetting, 0, ini) != 0;

	wchar_t buf[1024];

	::GetPrivateProfileStringW(mainSection, ignoreRegexStrSetting, NULL, buf, _countof(buf), ini);

	IgnoreRegexStr = buf;

	HideMatches			= ::GetPrivateProfileIntW(mainSection, hideMatchesSetting,			0, ini) != 0;
	HideNewLines		= ::GetPrivateProfileIntW(mainSection, hideNewLinesSetting,			0, ini) != 0;
	HideChangedLines	= ::GetPrivateProfileIntW(mainSection, hideChangedLinesSetting,		0, ini) != 0;
	HideMovedLines		= ::GetPrivateProfileIntW(mainSection, hideMovedLinesSetting,		0, ini) != 0;
	ShowOnlySelections	= ::GetPrivateProfileIntW(mainSection, showOnlySelSetting,			1, ini) != 0;

	UseNavBar			= ::GetPrivateProfileIntW(mainSection, navBarSetting,				1, ini) != 0;
	RecompareOnChange	= ::GetPrivateProfileIntW(mainSection, reCompareOnChangeSetting,	1, ini) != 0;

	StatusInfo = static_cast<StatusType>(::GetPrivateProfileIntW(mainSection, statusInfoSetting,
			DEFAULT_STATUS_INFO, ini));

	if (StatusInfo >= STATUS_TYPE_END)
		StatusInfo = static_cast<StatusType>(DEFAULT_STATUS_INFO);

	colorsLight.added						= ::GetPrivateProfileIntW(colorsSection, addedColorSetting,
			DEFAULT_ADDED_COLOR, ini);
	colorsLight.removed						= ::GetPrivateProfileIntW(colorsSection, removedColorSetting,
			DEFAULT_REMOVED_COLOR, ini);
	colorsLight.moved						= ::GetPrivateProfileIntW(colorsSection, movedColorSetting,
			DEFAULT_MOVED_COLOR, ini);
	colorsLight.changed						= ::GetPrivateProfileIntW(colorsSection, changedColorSetting,
			DEFAULT_CHANGED_COLOR, ini);
	colorsLight.add_highlight				= ::GetPrivateProfileIntW(colorsSection, addHighlightColorSetting,
			DEFAULT_HIGHLIGHT_COLOR, ini);
	colorsLight.rem_highlight				= ::GetPrivateProfileIntW(colorsSection, remHighlightColorSetting,
			DEFAULT_HIGHLIGHT_COLOR, ini);
	colorsLight.mov_highlight				= ::GetPrivateProfileIntW(colorsSection, movHighlightColorSetting,
			DEFAULT_HIGHLIGHT_MOVED_COLOR, ini);
	colorsLight.highlight_transparency		= ::GetPrivateProfileIntW(colorsSection, highlightTranspSetting,
			DEFAULT_HIGHLIGHT_TRANSP, ini);
	colorsLight.caret_line_transparency		= ::GetPrivateProfileIntW(colorsSection, caretLineTranspSetting,
			DEFAULT_CARET_LINE_TRANSP, ini);

	colorsDark.added						= ::GetPrivateProfileIntW(colorsSection, addedColorDarkSetting,
			DEFAULT_ADDED_COLOR_DARK, ini);
	colorsDark.removed						= ::GetPrivateProfileIntW(colorsSection, removedColorDarkSetting,
			DEFAULT_REMOVED_COLOR_DARK, ini);
	colorsDark.moved						= ::GetPrivateProfileIntW(colorsSection, movedColorDarkSetting,
			DEFAULT_MOVED_COLOR_DARK, ini);
	colorsDark.changed						= ::GetPrivateProfileIntW(colorsSection, changedColorDarkSetting,
			DEFAULT_CHANGED_COLOR_DARK, ini);
	colorsDark.add_highlight				= ::GetPrivateProfileIntW(colorsSection, addHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_COLOR_DARK, ini);
	colorsDark.rem_highlight				= ::GetPrivateProfileIntW(colorsSection, remHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_COLOR_DARK, ini);
	colorsDark.mov_highlight				= ::GetPrivateProfileIntW(colorsSection, movHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_MOVED_COLOR_DARK, ini);
	colorsDark.highlight_transparency		= ::GetPrivateProfileIntW(colorsSection, highlightTranspDarkSetting,
			DEFAULT_HIGHLIGHT_TRANSP_DARK, ini);
	colorsDark.caret_line_transparency		= ::GetPrivateProfileIntW(colorsSection, caretLineTranspDarkSetting,
			DEFAULT_CARET_LINE_TRANSP_DARK, ini);

	ChangedThresholdPercent	= ::GetPrivateProfileIntW(colorsSection, changedThresholdSetting,
			DEFAULT_CHANGED_THRESHOLD, ini);

	EnableToolbar		= ::GetPrivateProfileIntW(toolbarSection, enableToolbarSetting,
			DEFAULT_ENABLE_TOOLBAR_TB, ini) != 0;
	SetAsFirstTB		= ::GetPrivateProfileIntW(toolbarSection, setAsFirstTBSetting,
			DEFAULT_SET_AS_FIRST_TB, ini) != 0;
	CompareTB			= ::GetPrivateProfileIntW(toolbarSection, compareTBSetting,
			DEFAULT_COMPARE_TB, ini) != 0;
	CompareSelTB		= ::GetPrivateProfileIntW(toolbarSection, compareSelTBSetting,
			DEFAULT_COMPARE_SEL_TB, ini) != 0;
	ClearCompareTB		= ::GetPrivateProfileIntW(toolbarSection, clearCompareTBSetting,
			DEFAULT_CLEAR_COMPARE_TB, ini) != 0;
	NavigationTB		= ::GetPrivateProfileIntW(toolbarSection, navigationTBSetting,
			DEFAULT_NAVIGATION_TB, ini) != 0;
	DiffsFilterTB		= ::GetPrivateProfileIntW(toolbarSection, diffsFilterTBSetting,
			DEFAULT_DIFFS_FILTER_TB, ini) != 0;
	NavBarTB			= ::GetPrivateProfileIntW(toolbarSection, navBarTBSetting,
			DEFAULT_NAV_BAR_TB, ini) != 0;

	dirty = false;
}


void UserSettings::save()
{
	if (!dirty)
		return;

	wchar_t ini[MAX_PATH];

	::SendMessageW(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(ini), (LPARAM)ini);

	if (::PathFileExistsW(ini) == FALSE)
	{
		if (::CreateDirectoryW(ini, NULL) == FALSE)
		{
			wchar_t msg[MAX_PATH + 128];

			_snwprintf_s(msg, _countof(msg), _TRUNCATE,
					L"Notepad++ plugins config folder\n'%s'\ndoesn't exist and failed to be created."
					L"\nCannot write configuration file.", ini);
			::MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

			return;
		}
	}

	::PathAppendW(ini, L"ComparePlus.ini");

	// Make sure the ini config file has UNICODE encoding to be able to store ignore regex Unicode string
	{
		FILE* fp;

		_wfopen_s(&fp, ini, L"w, ccs=UTF-8");

		if (fp)
			fclose(fp);
	}

	if (!::WritePrivateProfileStringW(mainSection, newFileViewSetting, NewFileViewId == SUB_VIEW ? L"1" : L"0", ini))
	{
		wchar_t msg[MAX_PATH + 64];

		_snwprintf_s(msg, _countof(msg), _TRUNCATE, L"Failed to write\n'%s'\nconfiguration file.", ini);
		::MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

		return;
	}

	::WritePrivateProfileStringW(mainSection, firstIsNewSetting,			FirstFileIsNew		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, compareToPrevSetting,			CompareToPrev		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, encodingsCheckSetting,		EncodingsCheck		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, sizesCheckSetting,			SizesCheck			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, markIgnoredLinesSetting,		NeverMarkIgnored	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, followingCaretSetting,		FollowingCaret		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, wrapAroundSetting,			WrapAround			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, gotoFirstDiffSetting,			GotoFirstDiff		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, promptCloseOnMatchSetting,	PromptToCloseOnMatch  ? L"1" : L"0", ini);

	::WritePrivateProfileStringW(mainSection, detectMovesSetting,			DetectMoves			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, detectSubBlockDiffsSetting,	DetectSubBlockDiffs	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, detectSubLineMovesSetting,	DetectSubLineMoves	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, detectCharDiffsSetting,		DetectCharDiffs		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreEmptyLinesSetting,		IgnoreEmptyLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreFoldedLinesSetting,		IgnoreFoldedLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreHiddenLinesSetting,		IgnoreHiddenLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreChangedSpacesSetting,	IgnoreChangedSpaces	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreAllSpacesSetting,		IgnoreAllSpaces		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreEOLSetting,				IgnoreEOL			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreCaseSetting,			IgnoreCase			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreRegexSetting,			IgnoreRegex			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, invertRegexSetting,			InvertRegex			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, inclRegexNomatchLinesSetting,	InclRegexNomatchLines ? L"1" : L"0", ini);

	::WritePrivateProfileStringW(mainSection, ignoreRegexStrSetting, IgnoreRegexStr.c_str(), ini);

	::WritePrivateProfileStringW(mainSection, hideMatchesSetting,			HideMatches			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, hideNewLinesSetting,			HideNewLines		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, hideChangedLinesSetting,		HideChangedLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, hideMovedLinesSetting,		HideMovedLines		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, showOnlySelSetting,			ShowOnlySelections	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, navBarSetting,				UseNavBar			  ? L"1" : L"0", ini);

	::WritePrivateProfileStringW(mainSection, reCompareOnChangeSetting,		RecompareOnChange	  ? L"1" : L"0", ini);

	wchar_t buffer[64];

	_itow_s(static_cast<int>(StatusInfo), buffer, 64, 10);
	::WritePrivateProfileStringW(mainSection, statusInfoSetting, buffer, ini);

	_itow_s(colorsLight.added, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addedColorSetting, buffer, ini);

	_itow_s(colorsLight.removed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, removedColorSetting, buffer, ini);

	_itow_s(colorsLight.moved, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movedColorSetting, buffer, ini);

	_itow_s(colorsLight.changed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, changedColorSetting, buffer, ini);

	_itow_s(colorsLight.add_highlight, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addHighlightColorSetting, buffer, ini);

	_itow_s(colorsLight.rem_highlight, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, remHighlightColorSetting, buffer, ini);

	_itow_s(colorsLight.mov_highlight, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movHighlightColorSetting, buffer, ini);

	_itow_s(colorsLight.highlight_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, highlightTranspSetting, buffer, ini);

	_itow_s(colorsLight.caret_line_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, caretLineTranspSetting, buffer, ini);

	_itow_s(colorsDark.added, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.removed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, removedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.moved, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.changed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, changedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.add_highlight, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addHighlightColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.rem_highlight, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, remHighlightColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.mov_highlight, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movHighlightColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.highlight_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, highlightTranspDarkSetting, buffer, ini);

	_itow_s(colorsDark.caret_line_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, caretLineTranspDarkSetting, buffer, ini);

	_itow_s(ChangedThresholdPercent, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, changedThresholdSetting, buffer, ini);

	::WritePrivateProfileStringW(toolbarSection, enableToolbarSetting,	EnableToolbar	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, setAsFirstTBSetting,	SetAsFirstTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, compareTBSetting,		CompareTB		? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, compareSelTBSetting,	CompareSelTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, clearCompareTBSetting,	ClearCompareTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, navigationTBSetting,	NavigationTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, diffsFilterTBSetting,	DiffsFilterTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, navBarTBSetting,		NavBarTB		? L"1" : L"0", ini);

	dirty = false;
}
