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

#include "UserSettings.h"

#include <shlwapi.h>


const TCHAR UserSettings::mainSection[]					= TEXT("Main");

const TCHAR UserSettings::oldIsFirstSetting[]			= TEXT("Old is First");
const TCHAR UserSettings::oldFileOnLeftSetting[]		= TEXT("Old on Left");
const TCHAR UserSettings::compareToPrevSetting[]		= TEXT("Default Compare is to Prev");

const TCHAR UserSettings::detectMovesLineModeSetting[]	= TEXT("Detect Moves Line Mode");

const TCHAR UserSettings::encodingsCheckSetting[]		= TEXT("Check Encodings");
const TCHAR UserSettings::promptCloseOnMatchSetting[]	= TEXT("Prompt to Close on Match");
const TCHAR UserSettings::alignReplacementsSetting[]	= TEXT("Align Replacements");
const TCHAR UserSettings::wrapAroundSetting[]			= TEXT("Wrap Around");
const TCHAR UserSettings::reCompareOnSaveSetting[]		= TEXT("Re-Compare on Save");
const TCHAR UserSettings::gotoFirstDiffSetting[]		= TEXT("Go to First Diff");
const TCHAR UserSettings::updateOnChangeSetting[]		= TEXT("Update on Change");

const TCHAR UserSettings::ignoreSpacesSetting[]			= TEXT("Ignore Spaces");
const TCHAR UserSettings::ignoreCaseSetting[]			= TEXT("Ignore Case");
const TCHAR UserSettings::detectMovesSetting[]			= TEXT("Detect Moves");
const TCHAR UserSettings::navBarSetting[]				= TEXT("Navigation Bar");

const TCHAR UserSettings::colorsSection[]				= TEXT("Colors");

const TCHAR UserSettings::addedColorSetting[]			= TEXT("Added");
const TCHAR UserSettings::removedColorSetting[]			= TEXT("Removed");
const TCHAR UserSettings::changedColorSetting[]			= TEXT("Changed");
const TCHAR UserSettings::movedColorSetting[]			= TEXT("Moved");
const TCHAR UserSettings::highlightColorSetting[]		= TEXT("Highlight");
const TCHAR UserSettings::highlightAlphaSetting[]		= TEXT("Alpha");


void UserSettings::load()
{
	TCHAR iniFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFile), (LPARAM)iniFile);

	::PathAppend(iniFile, TEXT("ComparePlugin.ini"));

	OldFileIsFirst			= ::GetPrivateProfileInt(mainSection, oldIsFirstSetting,
			DEFAULT_OLD_IS_FIRST, iniFile) == 1;
	OldFileViewId			= ::GetPrivateProfileInt(mainSection, oldFileOnLeftSetting,
			DEFAULT_OLD_ON_LEFT, iniFile) == 1 ? MAIN_VIEW : SUB_VIEW;
	CompareToPrev			= ::GetPrivateProfileInt(mainSection, compareToPrevSetting,
			DEFAULT_COMPARE_TO_PREV, iniFile) == 1;
	DetectMovesLineMode		= ::GetPrivateProfileInt(mainSection, detectMovesLineModeSetting,
			DEFAULT_DETECT_MOVE_LINE_MODE, iniFile) == 1;
	EncodingsCheck			= ::GetPrivateProfileInt(mainSection, encodingsCheckSetting,
			DEFAULT_ENCODINGS_CHECK, iniFile) == 1;
	PromptToCloseOnMatch	= ::GetPrivateProfileInt(mainSection, promptCloseOnMatchSetting,
			DEFAULT_PROMPT_CLOSE_ON_MATCH, iniFile) == 1;
	AlignReplacements		= ::GetPrivateProfileInt(mainSection, alignReplacementsSetting,
			DEFAULT_ALIGN_REPLACEMENTS, iniFile) == 1;
	WrapAround				= ::GetPrivateProfileInt(mainSection, wrapAroundSetting,
			DEFAULT_WRAP_AROUND, iniFile) == 1;
	RecompareOnSave			= ::GetPrivateProfileInt(mainSection, reCompareOnSaveSetting,
			DEFAULT_RECOMPARE_ON_SAVE, iniFile) == 1;
	GotoFirstDiff			= ::GetPrivateProfileInt(mainSection, gotoFirstDiffSetting,
			DEFAULT_GOTO_FIRST_DIFF, iniFile) == 1;
	UpdateOnChange			= ::GetPrivateProfileInt(mainSection, updateOnChangeSetting,
			DEFAULT_UPDATE_ON_CHANGE, iniFile) == 1;

	IgnoreSpaces	= ::GetPrivateProfileInt(mainSection, ignoreSpacesSetting,	1, iniFile) == 1;
	IgnoreCase		= ::GetPrivateProfileInt(mainSection, ignoreCaseSetting,	0, iniFile) == 1;
	DetectMoves		= ::GetPrivateProfileInt(mainSection, detectMovesSetting,	1, iniFile) == 1;
	UseNavBar		= ::GetPrivateProfileInt(mainSection, navBarSetting,		1, iniFile) == 1;

	colors.added		= ::GetPrivateProfileInt(colorsSection, addedColorSetting,
			DEFAULT_ADDED_COLOR, iniFile);
	colors.deleted		= ::GetPrivateProfileInt(colorsSection, removedColorSetting,
			DEFAULT_DELETED_COLOR, iniFile);
	colors.changed		= ::GetPrivateProfileInt(colorsSection, changedColorSetting,
			DEFAULT_CHANGED_COLOR, iniFile);
	colors.moved		= ::GetPrivateProfileInt(colorsSection, movedColorSetting,
			DEFAULT_MOVED_COLOR, iniFile);
	colors.highlight	= ::GetPrivateProfileInt(colorsSection, highlightColorSetting,
			DEFAULT_HIGHLIGHT_COLOR, iniFile);
	colors.alpha		= ::GetPrivateProfileInt(colorsSection, highlightAlphaSetting,
			DEFAULT_HIGHLIGHT_ALPHA, iniFile);

	dirty = false;
}


void UserSettings::save()
{
	if (!dirty)
		return;

	TCHAR iniFile[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(iniFile), (LPARAM)iniFile);
	::PathAppend(iniFile, TEXT("ComparePlugin.ini"));

	::WritePrivateProfileString(mainSection, oldIsFirstSetting,
			OldFileIsFirst ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, oldFileOnLeftSetting,
			OldFileViewId == MAIN_VIEW ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, compareToPrevSetting,
			CompareToPrev ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, detectMovesLineModeSetting,
			DetectMovesLineMode ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, encodingsCheckSetting,
			EncodingsCheck ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, promptCloseOnMatchSetting,
			PromptToCloseOnMatch ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, alignReplacementsSetting,
			AlignReplacements ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, wrapAroundSetting,
			WrapAround ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, reCompareOnSaveSetting,
			RecompareOnSave ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, gotoFirstDiffSetting,
			GotoFirstDiff ? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, updateOnChangeSetting,
			UpdateOnChange ? TEXT("1") : TEXT("0"), iniFile);

	::WritePrivateProfileString(mainSection, ignoreSpacesSetting,	IgnoreSpaces	? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, ignoreCaseSetting,	    IgnoreCase		? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, detectMovesSetting,	DetectMoves		? TEXT("1") : TEXT("0"), iniFile);
	::WritePrivateProfileString(mainSection, navBarSetting,			UseNavBar		? TEXT("1") : TEXT("0"), iniFile);

	TCHAR buffer[64];

	_itot_s(colors.added, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addedColorSetting, buffer, iniFile);

	_itot_s(colors.deleted, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, removedColorSetting, buffer, iniFile);

	_itot_s(colors.changed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedColorSetting, buffer, iniFile);

	_itot_s(colors.moved, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movedColorSetting, buffer, iniFile);

	_itot_s(colors.highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightColorSetting, buffer, iniFile);

	_itot_s(colors.alpha, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightAlphaSetting, buffer, iniFile);

	dirty = false;
}
