/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2022-2025 Pavel Nedev (pg.nedev@gmail.com)
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

#pragma once

#define PARAM_TO_STR(X)			#X
#define TO_STR(X)				PARAM_TO_STR(X)


#define VER_COPYRIGHT			"Copyright (C) 2025\0"

#define PLUGIN_VERSION			2.0.0
#define VER_FILEVERSION			2,0,0,0
#define IS_PRERELEASE			1

#if (IS_PRERELEASE == 1)
#define VER_PRERELEASE	VS_FF_PRERELEASE
#else
#define VER_PRERELEASE	0
#endif

#ifdef _DEBUG
#define VER_DEBUG		VS_FF_DEBUG
#else
#define VER_DEBUG		0
#endif

#define VER_FILEFLAGS	(VER_PRERELEASE | VER_DEBUG)

#ifdef WIN64
#define VER_PRODUCT_STR		"ComparePlus (64-bit)\0"
#else
#define VER_PRODUCT_STR		"ComparePlus (32-bit)\0"
#endif


#define IDDEFAULT						3
#define IDD_ABOUT_DIALOG				101
#define IDD_COLOR_POPUP					102
#define IDD_SETTINGS_DIALOG				103
#define IDD_COMPARE_OPTIONS_DIALOG		104
#define IDD_VISUAL_FILTERS_DIALOG		105
#define IDD_NAV_DIALOG					106

#define IDB_DOCKING_ICON				115

#define IDB_SETFIRST					120
#define IDB_SETFIRST_RTL				121
#define IDB_COMPARE						122
#define IDB_COMPARE_LINES				123
#define IDB_CLEARCOMPARE				124
#define IDB_FIRST						125
#define IDB_LAST						126
#define IDB_PREV						127
#define IDB_NEXT						128
#define IDB_DIFFS_FILTERS				129
#define IDB_NAVBAR						130

#define IDB_SETFIRST_FL					140
#define IDB_SETFIRST_RTL_FL				141
#define IDB_COMPARE_FL					142
#define IDB_COMPARE_LINES_FL			143
#define IDB_CLEARCOMPARE_FL				144
#define IDB_FIRST_FL					145
#define IDB_LAST_FL						146
#define IDB_PREV_FL						147
#define IDB_NEXT_FL						148
#define IDB_DIFFS_FILTERS_FL			149
#define IDB_NAVBAR_FL					150

#define IDB_SETFIRST_FL_DM				160
#define IDB_SETFIRST_RTL_FL_DM			161
#define IDB_COMPARE_FL_DM				162
#define IDB_COMPARE_LINES_FL_DM			163
#define IDB_CLEARCOMPARE_FL_DM			164
#define IDB_FIRST_FL_DM					165
#define IDB_LAST_FL_DM					166
#define IDB_PREV_FL_DM					167
#define IDB_NEXT_FL_DM					168
#define IDB_DIFFS_FILTERS_FL_DM			169
#define IDB_NAVBAR_FL_DM				170

#define IDC_ABOUT_CLOSE_BUTTON			1001
#define IDC_DONATE_BUTTON				1002
#define IDC_COLOR_LIST					1003

#define IDC_BUILD_TIME					1010
#define IDC_EMAIL_LINK					1011
#define IDC_REPO_URL					1012
#define IDC_HELP_URL					1013

#define IDC_NEW_IN_SUB					1020
#define IDC_OLD_IN_SUB					1021
#define IDC_FIRST_NEW					1022
#define IDC_FIRST_OLD					1023
#define IDC_COMPARE_TO_PREV				1024
#define IDC_COMPARE_TO_NEXT				1025
#define IDC_DIFFS_SUMMARY				1026
#define IDC_COMPARE_OPTIONS				1027
#define IDC_STATUS_DISABLED				1028
#define IDC_ENCODINGS_CHECK				1029
#define IDC_SIZES_CHECK					1030
#define IDC_NEVER_MARK_IGNORED			1031
#define IDC_PROMPT_CLOSE_ON_MATCH		1032
#define IDC_WRAP_AROUND					1033
#define IDC_GOTO_FIRST_DIFF				1034
#define IDC_FOLLOWING_CARET				1035
#define IDC_COMBO_ADDED_COLOR			1036
#define IDC_COMBO_REMOVED_COLOR			1037
#define IDC_COMBO_MOVED_COLOR			1038
#define IDC_COMBO_CHANGED_COLOR			1039
#define IDC_COMBO_ADD_HIGHLIGHT_COLOR	1040
#define IDC_COMBO_REM_HIGHLIGHT_COLOR	1041
#define IDC_HIGHLIGHT_SPIN_BOX			1042
#define IDC_HIGHLIGHT_SPIN_CTL			1043
#define IDC_CARET_LINE_SPIN_BOX			1044
#define IDC_CARET_LINE_SPIN_CTL			1045
#define IDC_THRESHOLD_SPIN_BOX			1046
#define IDC_THRESHOLD_SPIN_CTL			1047
#define IDC_ENABLE_TOOLBAR				1048
#define IDC_SET_AS_FIRST_TB				1049
#define IDC_COMPARE_TB					1050
#define IDC_COMPARE_SELECTIONS_TB		1051
#define IDC_CLEAR_COMPARE_TB			1052
#define IDC_NAVIGATION_TB				1053
#define IDC_DIFFS_FILTERS_TB			1054
#define IDC_NAV_BAR_TB					1055

#define IDC_DETECT_MOVES				1070
#define IDC_DETECT_SUB_BLOCK_DIFFS		1071
#define IDC_DETECT_CHAR_DIFFS			1072
#define IDC_IGNORE_EMPTY_LINES			1073
#define IDC_IGNORE_FOLDED_LINES			1074
#define IDC_IGNORE_HIDDEN_LINES			1075
#define IDC_IGNORE_CHANGED_SPACES		1076
#define IDC_IGNORE_ALL_SPACES			1077
#define IDC_IGNORE_CASE					1078
#define IDC_IGNORE_REGEX				1079
#define IDC_IGNORE_REGEX_STR			1080
#define IDC_REGEX_MODE_IGNORE			1081
#define IDC_REGEX_MODE_MATCH			1082
#define IDC_REGEX_INCL_NOMATCH_LINES	1083

#define IDC_HIDE_MATCHES				1090
#define IDC_HIDE_ALL_DIFFS				1091
#define IDC_HIDE_NEW_LINES				1092
#define IDC_HIDE_CHANGED_LINES			1093
#define IDC_HIDE_MOVED_LINES			1094
#define IDC_SHOW_ONLY_SELECTIONS		1095

#define IDC_STATIC						-1

#define COLOR_POPUP_OK					10000
#define COLOR_POPUP_CANCEL				10001

// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        220
#define _APS_NEXT_COMMAND_VALUE         20001
#define _APS_NEXT_CONTROL_VALUE         1100
#define _APS_NEXT_SYMED_VALUE           110
#endif
#endif
