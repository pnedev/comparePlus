/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2017-2021 Pavel Nedev (pg.nedev@gmail.com)
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
const TCHAR UserSettings::alignAllMatchesSetting[]			= TEXT("align_all_matches");
const TCHAR UserSettings::markIgnoredLinesSetting[]			= TEXT("never_colorize_ignored_lines");
const TCHAR UserSettings::promptCloseOnMatchSetting[]		= TEXT("prompt_to_close_on_match");
const TCHAR UserSettings::wrapAroundSetting[]				= TEXT("wrap_around");
const TCHAR UserSettings::gotoFirstDiffSetting[]			= TEXT("go_to_first_on_recompare");
const TCHAR UserSettings::followingCaretSetting[]			= TEXT("following_caret");

const TCHAR UserSettings::detectMovesSetting[]				= TEXT("detect_moves");
const TCHAR UserSettings::detectCharDiffsSetting[]			= TEXT("detect_character_diffs");
const TCHAR UserSettings::bestSeqChangedLinesSetting[]		= TEXT("best_seq_changed_lines");
const TCHAR UserSettings::ignoreSpacesSetting[]				= TEXT("ignore_spaces");
const TCHAR UserSettings::ignoreEmptyLinesSetting[]			= TEXT("ignore_empty_lines");
const TCHAR UserSettings::ignoreCaseSetting[]				= TEXT("ignore_case");
const TCHAR UserSettings::showOnlySelSetting[]				= TEXT("show_only_selections");
const TCHAR UserSettings::showOnlyDiffSetting[]				= TEXT("show_only_diffs");
const TCHAR UserSettings::navBarSetting[]					= TEXT("navigation_bar");

const TCHAR UserSettings::reCompareOnChangeSetting[]		= TEXT("recompare_on_change");

const TCHAR UserSettings::statusTypeSetting[]				= TEXT("status_type");

const TCHAR UserSettings::colorsSection[]					= TEXT("color_settings");

const TCHAR UserSettings::addedColorSetting[]				= TEXT("added");
const TCHAR UserSettings::removedColorSetting[]				= TEXT("removed");
const TCHAR UserSettings::movedColorSetting[]				= TEXT("moved");
const TCHAR UserSettings::changedColorSetting[]				= TEXT("changed");
const TCHAR UserSettings::addHighlightColorSetting[]		= TEXT("added_highlight");
const TCHAR UserSettings::remHighlightColorSetting[]		= TEXT("removed_highlight");
const TCHAR UserSettings::highlightTranspSetting[]			= TEXT("transparency");

const TCHAR UserSettings::addedColorDarkSetting[]			= TEXT("added_dark");
const TCHAR UserSettings::removedColorDarkSetting[]			= TEXT("removed_dark");
const TCHAR UserSettings::movedColorDarkSetting[]			= TEXT("moved_dark");
const TCHAR UserSettings::changedColorDarkSetting[]			= TEXT("changed_dark");
const TCHAR UserSettings::addHighlightColorDarkSetting[]	= TEXT("added_highlight_dark");
const TCHAR UserSettings::remHighlightColorDarkSetting[]	= TEXT("removed_highlight_dark");
const TCHAR UserSettings::highlightTranspDarkSetting[]		= TEXT("transparency_dark");

const TCHAR UserSettings::changedThresholdSetting[]			= TEXT("changed_threshold_percentage");


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
	AlignAllMatches			= ::GetPrivateProfileInt(mainSection, alignAllMatchesSetting,
			DEFAULT_ALIGN_ALL_MATCHES, iniFile) != 0;
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

	DetectMoves			= ::GetPrivateProfileInt(mainSection, detectMovesSetting,			1, iniFile) != 0;
	DetectCharDiffs		= ::GetPrivateProfileInt(mainSection, detectCharDiffsSetting,		0, iniFile) != 0;
	BestSeqChangedLines	= ::GetPrivateProfileInt(mainSection, bestSeqChangedLinesSetting,	0, iniFile) != 0;
	IgnoreSpaces		= ::GetPrivateProfileInt(mainSection, ignoreSpacesSetting,			0, iniFile) != 0;
	IgnoreEmptyLines	= ::GetPrivateProfileInt(mainSection, ignoreEmptyLinesSetting,		0, iniFile) != 0;
	IgnoreCase			= ::GetPrivateProfileInt(mainSection, ignoreCaseSetting,			0, iniFile) != 0;
	ShowOnlyDiffs		= ::GetPrivateProfileInt(mainSection, showOnlyDiffSetting,			0, iniFile) != 0;
	ShowOnlySelections	= ::GetPrivateProfileInt(mainSection, showOnlySelSetting,			1, iniFile) != 0;
	UseNavBar			= ::GetPrivateProfileInt(mainSection, navBarSetting,				1, iniFile) != 0;

	RecompareOnChange	= ::GetPrivateProfileInt(mainSection, reCompareOnChangeSetting,	1, iniFile) != 0;

	SavedStatusType	= static_cast<StatusType>(::GetPrivateProfileInt(mainSection, statusTypeSetting,
			DEFAULT_STATUS_TYPE, iniFile));

	statusType = (SavedStatusType < STATUS_TYPE_END) ? SavedStatusType : static_cast<StatusType>(DEFAULT_STATUS_TYPE);

	colorsLight.added			= ::GetPrivateProfileInt(colorsSection, addedColorSetting,
			DEFAULT_ADDED_COLOR, iniFile);
	colorsLight.removed			= ::GetPrivateProfileInt(colorsSection, removedColorSetting,
			DEFAULT_REMOVED_COLOR, iniFile);
	colorsLight.moved			= ::GetPrivateProfileInt(colorsSection, movedColorSetting,
			DEFAULT_MOVED_COLOR, iniFile);
	colorsLight.changed			= ::GetPrivateProfileInt(colorsSection, changedColorSetting,
			DEFAULT_CHANGED_COLOR, iniFile);
	colorsLight.add_highlight	= ::GetPrivateProfileInt(colorsSection, addHighlightColorSetting,
			DEFAULT_HIGHLIGHT_COLOR, iniFile);
	colorsLight.rem_highlight	= ::GetPrivateProfileInt(colorsSection, remHighlightColorSetting,
			DEFAULT_HIGHLIGHT_COLOR, iniFile);
	colorsLight.transparency		= ::GetPrivateProfileInt(colorsSection, highlightTranspSetting,
			DEFAULT_HIGHLIGHT_TRANSP, iniFile);

	colorsDark.added			= ::GetPrivateProfileInt(colorsSection, addedColorDarkSetting,
			DEFAULT_ADDED_COLOR_DARK, iniFile);
	colorsDark.removed			= ::GetPrivateProfileInt(colorsSection, removedColorDarkSetting,
			DEFAULT_REMOVED_COLOR_DARK, iniFile);
	colorsDark.moved			= ::GetPrivateProfileInt(colorsSection, movedColorDarkSetting,
			DEFAULT_MOVED_COLOR_DARK, iniFile);
	colorsDark.changed			= ::GetPrivateProfileInt(colorsSection, changedColorDarkSetting,
			DEFAULT_CHANGED_COLOR_DARK, iniFile);
	colorsDark.add_highlight	= ::GetPrivateProfileInt(colorsSection, addHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_COLOR_DARK, iniFile);
	colorsDark.rem_highlight	= ::GetPrivateProfileInt(colorsSection, remHighlightColorDarkSetting,
			DEFAULT_HIGHLIGHT_COLOR_DARK, iniFile);
	colorsDark.transparency		= ::GetPrivateProfileInt(colorsSection, highlightTranspDarkSetting,
			DEFAULT_HIGHLIGHT_TRANSP_DARK, iniFile);

	ChangedThresholdPercent	= ::GetPrivateProfileInt(colorsSection, changedThresholdSetting,
			DEFAULT_CHANGED_THRESHOLD, iniFile);

	dirty = false;
}


void UserSettings::save()
{
	if (!dirty && (SavedStatusType == statusType))
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
	::WritePrivateProfileString(mainSection, alignAllMatchesSetting,
			AlignAllMatches ? TEXT("1") : TEXT("0"), iniFile);
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
	::WritePrivateProfileString(mainSection, detectCharDiffsSetting,
			DetectCharDiffs ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, bestSeqChangedLinesSetting,
			BestSeqChangedLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreSpacesSetting,
			IgnoreSpaces ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreEmptyLinesSetting,
			IgnoreEmptyLines ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreCaseSetting,
			IgnoreCase ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, showOnlyDiffSetting,
			ShowOnlyDiffs ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, showOnlySelSetting,
			ShowOnlySelections ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, navBarSetting,
			UseNavBar ? TEXT("1") : TEXT("0"), iniFile);

	::WritePrivateProfileString(mainSection, reCompareOnChangeSetting,
			RecompareOnChange ? TEXT("1") : TEXT("0"), iniFile);

	TCHAR buffer[64];

	SavedStatusType = statusType;

	_itot_s(static_cast<int>(SavedStatusType), buffer, 64, 10);
	::WritePrivateProfileString(mainSection, statusTypeSetting, buffer, iniFile);

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

	_itot_s(colorsLight.transparency, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightTranspSetting, buffer, iniFile);

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

	_itot_s(colorsDark.transparency, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightTranspDarkSetting, buffer, iniFile);

	_itot_s(ChangedThresholdPercent, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedThresholdSetting, buffer, iniFile);

	dirty = false;
}
