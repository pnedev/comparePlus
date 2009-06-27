//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

//--------------------------------------------------------------------------
// Compare plugin by Ty Landercasper & Jean-sébastien Leroy
//--------------------------------------------------------------------------

//#define UNICODE

/* Rotate a value n bits to the left. */
#define UINT_BIT (sizeof (unsigned) * CHAR_BIT)
#define ROL(v, n) ((v) << (n) | (v) >> (UINT_BIT - (n)))

/* Given a hash value and a new character, return a new hash value. */
#define HASH(h, c) ((c) + ROL (h, 7))


#include "Compare.h"
#include "NPPHelpers.h"

TCHAR emptyLinesDoc[MAX_PATH];
#define MAXCOMPARE 50
int compareDocs[MAXCOMPARE];

int tempWindow = -1;
bool notepadVersionOk = false;
bool active = false;
blankLineList *lastEmptyLines=NULL;
int topLine = 0;

bool panelsOpened = false;

const TCHAR SVN_BASE[] = TEXT(".svn\\text-base");
const TCHAR SVN_END[] = TEXT(".svn-base");
const TCHAR PLUGIN_NAME[] = TEXT("Compare");
TCHAR iniFilePath[MAX_PATH];
const TCHAR sectionName[] = TEXT("Compare Settings");
const TCHAR addLinesOption[] = TEXT("Align Matches");
const TCHAR ignoreSpacesOption[] = TEXT("Include Spaces");
const TCHAR detectMovesOption[] = TEXT("Detect Move Blocks");

const TCHAR colorsSection[]        = TEXT("Colors");
const TCHAR addedColorOption[]     = TEXT("Added");
const TCHAR removedColorOption[]   = TEXT("Removed");
const TCHAR changedColorOption[]   = TEXT("Changed");
const TCHAR movedColorOption[]     = TEXT("Moved");
const TCHAR blankColorOption[]     = TEXT("Blank");
const TCHAR highlightColorOption[] = TEXT("Highlight");
const TCHAR highlightAlphaOption[] = TEXT("Alpha");
const TCHAR symbolsOption[]        = TEXT("Symbols");

const TCHAR localConfFile[] = TEXT("doLocalConf.xml");

TCHAR compareFilePath[MAX_PATH];
TCHAR compareFile[] = TEXT("Compare File");
NppData nppData;

FuncItem funcItem[NB_MENU_COMMANDS];

HANDLE g_hModule;

sUserSettings Settings;
AboutDialog   AboutDlg;
OptionDialog  OptionDlg;
NavDialog     NavDlg;

toolbarIcons  tbPrev;
toolbarIcons  tbNext;
toolbarIcons  tbFirst;
toolbarIcons  tbLast;

bool FirstRun = false;

void EmptyFunc(void) { };

int getCompare(int window)
{
    for(int i = 0; i < MAXCOMPARE; i++) if(compareDocs[i] == window) return i;
    return -1;
}

void removeCompare(int window)
{
    int val = getCompare(window);
    if(val != -1) compareDocs[val] = -1;
}

int setCompare(int window)
{
    int val = getCompare(window);
    if(val != -1) return val;
    for(int i = 0; i < MAXCOMPARE; i++)
    {
        if(compareDocs[i] == -1)
        {
            compareDocs[i] = window;
            return i;
        }
    }
    return -1;
}

void alignMatches()
{
    HMENU hMenu = GetMenu(nppData._nppHandle);
    Settings.AddLine = !Settings.AddLine;
    if (hMenu)
    {
        CheckMenuItem(hMenu, funcItem[6]._cmdID, MF_BYCOMMAND | (Settings.AddLine ? MF_CHECKED : MF_UNCHECKED));
    }      
}
void includeSpacing()
{
    HMENU hMenu = GetMenu(nppData._nppHandle);
    Settings.IncludeSpace = !Settings.IncludeSpace;
    if (hMenu)
    {
        CheckMenuItem(hMenu, funcItem[7]._cmdID, MF_BYCOMMAND | (Settings.IncludeSpace ? MF_CHECKED : MF_UNCHECKED));
    }  
}
void detectMoves()
{
    HMENU hMenu = GetMenu(nppData._nppHandle);
    Settings.DetectMove = !Settings.DetectMove;
    if (hMenu)
    {
        CheckMenuItem(hMenu, funcItem[8]._cmdID, MF_BYCOMMAND | (Settings.DetectMove ? MF_CHECKED : MF_UNCHECKED));
    }  
}

void compareLocal()
{
    TCHAR file[MAX_PATH];
    ::SendMessage(nppData._nppHandle,NPPM_GETCURRENTDIRECTORY,0,(LPARAM)file);
    if(file[0] != 0)
    {
        ::SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,0,(LPARAM)file);
    }
    openFile(file);
}
void compareBase()
{
    TCHAR directory[MAX_PATH];
    TCHAR filename[MAX_PATH];
    TCHAR file[MAX_PATH];
    ::SendMessage(nppData._nppHandle,NPPM_GETCURRENTDIRECTORY,0,(LPARAM)directory);
    ::SendMessage(nppData._nppHandle,NPPM_GETFILENAME,0,(LPARAM)filename);
    if(directory[0] != 0)
    {
        //svn
        lstrcpy(file,directory);
        PathAppend(file,SVN_BASE);
        PathAppend(file,filename);
        int length=lstrlen(file);
        lstrcpy(file+length,SVN_END);
        if(PathFileExists(file) == TRUE)
        {
            openFile(file);
            return;
        }
        ::MessageBox(nppData._nppHandle,TEXT("Can't locate SVN information"),TEXT("File Not Found"),MB_OK);
    }
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID /*lpReserved*/)
 {
    g_hModule = hModule;

    switch (reasonForCall)
    {
    case DLL_PROCESS_ATTACH:
        {
            funcItem[CMD_COMPARE]._pFunc = compare;
            lstrcpy(funcItem[CMD_COMPARE]._itemName, TEXT("Compare"));
            funcItem[CMD_COMPARE]._pShKey = new ShortcutKey;
            funcItem[CMD_COMPARE]._pShKey->_isAlt = true;
            funcItem[CMD_COMPARE]._pShKey->_isCtrl = false;
            funcItem[CMD_COMPARE]._pShKey->_isShift = false;
            funcItem[CMD_COMPARE]._pShKey->_key = 'D';

            funcItem[CMD_CLEAR_RESULTS]._pFunc = reset;
            lstrcpy(funcItem[CMD_CLEAR_RESULTS]._itemName, TEXT("Clear Results"));
            funcItem[CMD_CLEAR_RESULTS]._pShKey = new ShortcutKey;
            funcItem[CMD_CLEAR_RESULTS]._pShKey->_isAlt = true;
            funcItem[CMD_CLEAR_RESULTS]._pShKey->_isCtrl = true;
            funcItem[CMD_CLEAR_RESULTS]._pShKey->_isShift = false;
            funcItem[CMD_CLEAR_RESULTS]._pShKey->_key = 'D';

            funcItem[CMD_SEPARATOR_1]._pFunc = EmptyFunc;
            lstrcpy(funcItem[CMD_SEPARATOR_1]._itemName, TEXT("-----------"));
            funcItem[CMD_SEPARATOR_1]._pShKey = NULL;

            funcItem[CMD_COMPARE_LAST_SAVE]._pFunc = compareLocal;
            lstrcpy(funcItem[CMD_COMPARE_LAST_SAVE]._itemName, TEXT("Compare to last save"));
            funcItem[CMD_COMPARE_LAST_SAVE]._pShKey = new ShortcutKey;
            funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isAlt = true;
            funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isCtrl = false;
            funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isShift = false;
            funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_key = 'S';

            funcItem[CMD_COMAPRE_SVN_BASE]._pFunc = compareBase;
            lstrcpy(funcItem[CMD_COMAPRE_SVN_BASE]._itemName, TEXT("Compare against SVN base"));
            funcItem[CMD_COMAPRE_SVN_BASE]._pShKey = new ShortcutKey;
            funcItem[CMD_COMAPRE_SVN_BASE]._pShKey->_isAlt = true;
            funcItem[CMD_COMAPRE_SVN_BASE]._pShKey->_isCtrl = false;
            funcItem[CMD_COMAPRE_SVN_BASE]._pShKey->_isShift = false;
            funcItem[CMD_COMAPRE_SVN_BASE]._pShKey->_key = 'B';

            funcItem[CMD_SEPARATOR_2]._pFunc = EmptyFunc;
            lstrcpy(funcItem[CMD_SEPARATOR_2]._itemName, TEXT("------------"));
            funcItem[CMD_SEPARATOR_2]._pShKey = NULL;

            funcItem[CMD_ALIGN_MATCHES]._pFunc = alignMatches;
            lstrcpy(funcItem[CMD_ALIGN_MATCHES]._itemName, TEXT("Align Matches"));
            funcItem[CMD_ALIGN_MATCHES]._pShKey = NULL;

            funcItem[CMD_IGNORE_SPACING]._pFunc = includeSpacing;
            lstrcpy(funcItem[CMD_IGNORE_SPACING]._itemName, TEXT("Ignore Spacing"));
            funcItem[CMD_IGNORE_SPACING]._pShKey = NULL;

            funcItem[CMD_DETECT_MOVES]._pFunc = detectMoves;
            lstrcpy(funcItem[CMD_DETECT_MOVES]._itemName, TEXT("Detect Moves"));
            funcItem[CMD_DETECT_MOVES]._pShKey = NULL;

            funcItem[CMD_USE_NAV_BAR]._pFunc = ViewNavigationBar;
            lstrcpy(funcItem[CMD_USE_NAV_BAR]._itemName, TEXT("Navigation bar"));
            funcItem[CMD_USE_NAV_BAR]._pShKey = NULL;
            funcItem[CMD_USE_NAV_BAR]._init2Check = true;

            funcItem[CMD_SEPARATOR_3]._pFunc = EmptyFunc;
            lstrcpy(funcItem[CMD_SEPARATOR_3]._itemName, TEXT("-----------"));
            funcItem[CMD_SEPARATOR_3]._pShKey = NULL;

            funcItem[CMD_PREV]._pFunc = Prev;
            lstrcpy(funcItem[CMD_PREV]._itemName, TEXT("Previous"));
            funcItem[CMD_PREV]._pShKey = new ShortcutKey;
            funcItem[CMD_PREV]._pShKey->_isAlt = false;
            funcItem[CMD_PREV]._pShKey->_isCtrl = true;
            funcItem[CMD_PREV]._pShKey->_isShift = false;
            funcItem[CMD_PREV]._pShKey->_key = VK_PRIOR;

            funcItem[CMD_NEXT]._pFunc = Next;
            lstrcpy(funcItem[CMD_NEXT]._itemName, TEXT("Next"));
            funcItem[CMD_NEXT]._pShKey = new ShortcutKey;
            funcItem[CMD_NEXT]._pShKey->_isAlt = false;
            funcItem[CMD_NEXT]._pShKey->_isCtrl = true;
            funcItem[CMD_NEXT]._pShKey->_isShift = false;
            funcItem[CMD_NEXT]._pShKey->_key = VK_NEXT;

            funcItem[CMD_FIRST]._pFunc = First;
            lstrcpy(funcItem[CMD_FIRST]._itemName, TEXT("First"));
            funcItem[CMD_FIRST]._pShKey = new ShortcutKey;
            funcItem[CMD_FIRST]._pShKey->_isAlt = false;
            funcItem[CMD_FIRST]._pShKey->_isCtrl = true;
            funcItem[CMD_FIRST]._pShKey->_isShift = true;
            funcItem[CMD_FIRST]._pShKey->_key = VK_PRIOR;

            funcItem[CMD_LAST]._pFunc = Last;
            lstrcpy(funcItem[CMD_LAST]._itemName, TEXT("Last"));
            funcItem[CMD_LAST]._pShKey = new ShortcutKey;
            funcItem[CMD_LAST]._pShKey->_isAlt = false;
            funcItem[CMD_LAST]._pShKey->_isCtrl = true;
            funcItem[CMD_LAST]._pShKey->_isShift = true;
            funcItem[CMD_LAST]._pShKey->_key = VK_NEXT;

            funcItem[CMD_SEPARATOR_4]._pFunc = EmptyFunc;
            lstrcpy(funcItem[CMD_SEPARATOR_4]._itemName, TEXT("-----------"));
            funcItem[CMD_SEPARATOR_4]._pShKey = NULL;

            funcItem[CMD_OPTION]._pFunc = openOptionDlg;
            lstrcpy(funcItem[CMD_OPTION]._itemName, TEXT("Option"));
            funcItem[CMD_OPTION]._pShKey = NULL;

            funcItem[CMD_ABOUT]._pFunc = openAboutDlg;
            lstrcpy(funcItem[CMD_ABOUT]._itemName, TEXT("About"));
            funcItem[CMD_ABOUT]._pShKey = NULL;

            for(int i = 0; i < MAXCOMPARE; i++)
            {
                compareDocs[i]=-1;
            }

            TCHAR nppPath[MAX_PATH];
            GetModuleFileName((HMODULE)hModule, nppPath, sizeof(nppPath));

            // remove the module name : get plugins directory path
            PathRemoveFileSpec(nppPath);

            // cd .. : get npp executable path
            PathRemoveFileSpec(nppPath);

            // Make localConf.xml path
            TCHAR localConfPath[MAX_PATH];
            lstrcpy(localConfPath, nppPath);
            PathAppend(localConfPath, localConfFile);

            // Test if localConf.xml exist
            bool isLocal = (PathFileExists(localConfPath) == TRUE);

            if (isLocal) 
            {
                lstrcpy(iniFilePath, nppPath);
                lstrcpy(compareFilePath, nppPath);

                PathAppend(iniFilePath, TEXT("plugins\\config\\Compare.ini"));
                //PathAppend(compareFilePath, compareFile);
            }
            else 
            {
                ITEMIDLIST *pidl;
                SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
                SHGetPathFromIDList(pidl, iniFilePath);
                SHGetPathFromIDList(pidl, compareFilePath);

                PathAppend(iniFilePath, TEXT("Notepad++\\Compare.ini"));
                //PathAppend(compareFilePath, "Notepad++\\");
                //PathAppend(compareFilePath,compareFile);
            }

            loadSettings();
        }
        break;

    case DLL_PROCESS_DETACH:

        if (tbNext.hToolbarBmp)  ::DeleteObject(tbNext.hToolbarBmp);
        if (tbPrev.hToolbarBmp)  ::DeleteObject(tbPrev.hToolbarBmp);
        if (tbFirst.hToolbarBmp) ::DeleteObject(tbFirst.hToolbarBmp);
        if (tbLast.hToolbarBmp)  ::DeleteObject(tbLast.hToolbarBmp);

        saveSettings();
        OptionDlg.destroy();
        AboutDlg.destroy();
        NavDlg.destroy();

        // Don't forget to deallocate your shortcut here
        delete funcItem[0]._pShKey;
        delete funcItem[1]._pShKey;
        delete funcItem[3]._pShKey;
        delete funcItem[4]._pShKey;
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;

    AboutDlg.init((HINSTANCE)g_hModule, nppData);
    OptionDlg.init((HINSTANCE)g_hModule, nppData);
    NavDlg.init((HINSTANCE)g_hModule, nppData);
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
    return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
    *nbF = NB_MENU_COMMANDS;
    return funcItem;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif

void loadSettings(void)
{
    // Try loading previous color settings
    int colors = GetPrivateProfileInt(colorsSection, addedColorOption, -1, iniFilePath);

    // If there is no previous color settings, load default value
    if(colors == -1)
    {
        Settings.ColorSettings.added     = ::GetPrivateProfileInt(colorsSection, addedColorOption, DEFAULT_ADDED_COLOR, iniFilePath);
        Settings.ColorSettings.deleted   = ::GetPrivateProfileInt(colorsSection, removedColorOption, DEFAULT_DELETED_COLOR, iniFilePath);
        Settings.ColorSettings.changed   = ::GetPrivateProfileInt(colorsSection, changedColorOption, DEFAULT_CHANGED_COLOR, iniFilePath);
        Settings.ColorSettings.moved     = ::GetPrivateProfileInt(colorsSection, movedColorOption, DEFAULT_MOVED_COLOR, iniFilePath);
        Settings.ColorSettings.blank     = ::GetPrivateProfileInt(colorsSection, blankColorOption, DEFAULT_BLANK_COLOR, iniFilePath);
        Settings.ColorSettings.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorOption, DEFAULT_HIGHLIGHT_COLOR, iniFilePath);
        Settings.ColorSettings.alpha     = ::GetPrivateProfileInt(colorsSection, highlightAlphaOption, DEFAULT_HIGHLIGHT_ALPHA, iniFilePath);
    }
    // Else load stored color settings
    else
    {
        Settings.ColorSettings.added     = ::GetPrivateProfileInt(colorsSection, addedColorOption, DEFAULT_ADDED_COLOR, iniFilePath);
        Settings.ColorSettings.deleted   = ::GetPrivateProfileInt(colorsSection, removedColorOption, DEFAULT_DELETED_COLOR, iniFilePath);
        Settings.ColorSettings.changed   = ::GetPrivateProfileInt(colorsSection, changedColorOption, DEFAULT_CHANGED_COLOR, iniFilePath);
        Settings.ColorSettings.moved     = ::GetPrivateProfileInt(colorsSection, movedColorOption, DEFAULT_MOVED_COLOR, iniFilePath);
        Settings.ColorSettings.blank     = ::GetPrivateProfileInt(colorsSection, blankColorOption, DEFAULT_BLANK_COLOR, iniFilePath); 
        Settings.ColorSettings.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorOption, DEFAULT_HIGHLIGHT_COLOR, iniFilePath);
        Settings.ColorSettings.alpha     = ::GetPrivateProfileInt(colorsSection, highlightAlphaOption, DEFAULT_HIGHLIGHT_ALPHA, iniFilePath);
    }

    // Try loading behavior settings, else load default value
    Settings.AddLine      = ::GetPrivateProfileInt(sectionName, addLinesOption, 1, iniFilePath) == 1;
    Settings.IncludeSpace = ::GetPrivateProfileInt(sectionName, ignoreSpacesOption, 1, iniFilePath) == 1;
    Settings.DetectMove   = ::GetPrivateProfileInt(sectionName, detectMovesOption, 1, iniFilePath) == 1;
    Settings.OldSymbols   = ::GetPrivateProfileInt(sectionName, symbolsOption, 1, iniFilePath) == 1;

    // Compare 1.5.4 NavBar beta
    FirstRun = ::GetPrivateProfileInt(sectionName, TEXT("FirstRun"), 1, iniFilePath) == 1;
}

void saveSettings(void)
{
    TCHAR buffer[64];

    _itot_s(Settings.ColorSettings.added, buffer, 64, 10);
    ::WritePrivateProfileString(colorsSection, addedColorOption, buffer, iniFilePath);

    _itot_s(Settings.ColorSettings.deleted, buffer, 64, 10);
    ::WritePrivateProfileString(colorsSection, removedColorOption, buffer, iniFilePath);

    _itot_s(Settings.ColorSettings.changed, buffer, 64, 10);
    ::WritePrivateProfileString(colorsSection, changedColorOption, buffer, iniFilePath);

    _itot_s(Settings.ColorSettings.moved, buffer, 64, 10);
    ::WritePrivateProfileString(colorsSection, movedColorOption, buffer, iniFilePath);

    _itot_s(Settings.ColorSettings.blank, buffer, 64, 10);
    ::WritePrivateProfileString(colorsSection, blankColorOption, buffer, iniFilePath);

    _itot_s(Settings.ColorSettings.highlight, buffer, 64, 10);
    ::WritePrivateProfileString(colorsSection, highlightColorOption, buffer, iniFilePath);

    _itot_s(Settings.ColorSettings.alpha, buffer, 64, 10);
    ::WritePrivateProfileString(colorsSection, highlightAlphaOption, buffer, iniFilePath);

    ::WritePrivateProfileString(sectionName, addLinesOption, Settings.AddLine ? TEXT("1") : TEXT("0"), iniFilePath);
    ::WritePrivateProfileString(sectionName, ignoreSpacesOption, Settings.IncludeSpace ? TEXT("1") : TEXT("0"), iniFilePath);
    ::WritePrivateProfileString(sectionName, detectMovesOption, Settings.DetectMove ? TEXT("1") : TEXT("0"), iniFilePath);
    ::WritePrivateProfileString(sectionName, symbolsOption, Settings.OldSymbols ? TEXT("1") : TEXT("0"), iniFilePath);

    ::WritePrivateProfileString(sectionName, TEXT("FirstRun"), TEXT("0"), iniFilePath);
}

void openOptionDlg(void)
{
    if (OptionDlg.doDialog(&Settings) == IDOK)
    {
        saveSettings();
        if (active)
        {
            setStyles(Settings);
            
            NavDlg.SetColor(
                Settings.ColorSettings.added, 
                Settings.ColorSettings.deleted, 
                Settings.ColorSettings.changed, 
                Settings.ColorSettings.moved, 
                Settings.ColorSettings.blank);

            NavDlg.CreateBitmap();
        }
    }
}

void jumpChangedLines( bool direction )
{
    HWND CurView = NULL;
    int currentEdit;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);

    if (currentEdit != -1) 
    {
        CurView = (currentEdit == 0) ? (nppData._scintillaMainHandle) : (nppData._scintillaSecondHandle);
    }

    int sci_search_mask = (1 << MARKER_MOVED_LINE) | (1 << MARKER_CHANGED_LINE) | 
                          (1 << MARKER_ADDED_LINE) | (1 << MARKER_REMOVED_LINE) |
                          (1 << MARKER_BLANK_LINE);

	int posStart = ::SendMessage(CurView, SCI_GETCURRENTPOS, 0, 0 );
	int lineStart = ::SendMessage(CurView, SCI_LINEFROMPOSITION, posStart, 0 );
	int lineMax = ::SendMessage(CurView, SCI_GETLINECOUNT, 0, 0 );

	int currLine;
	int nextLine;
	int sci_marker_direction;

	if ( direction ) 
    {
		currLine = ( lineStart < lineMax ) ? ( lineStart + 1 ) : ( 0 );
		sci_marker_direction = SCI_MARKERNEXT;
	}
	else 
    {
		currLine = ( lineStart > 0 ) ? ( lineStart - 1 ) : ( lineMax );
		sci_marker_direction = SCI_MARKERPREVIOUS;
	}

	nextLine = ::SendMessage(CurView, sci_marker_direction, currLine, sci_search_mask );

	if ( nextLine < 0 ) 
    {
		currLine = ( direction ) ? ( 0 ) : ( lineMax );
		nextLine = ::SendMessage(CurView, sci_marker_direction, currLine, sci_search_mask );
	}

	::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0 );
	::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0 );
}

void Prev(void)
{
    if (active) jumpChangedLines(false);
}

void Next(void)
{
    if (active) jumpChangedLines(true);
}

void First(void)
{
    if (active)
    {
        HWND CurView = NULL;
        int currentEdit;
        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);

        if (currentEdit != -1) 
        {
            CurView = (currentEdit == 0) ? (nppData._scintillaMainHandle) : (nppData._scintillaSecondHandle);
        }

        int sci_search_mask = (1 << MARKER_MOVED_LINE) | (1 << MARKER_CHANGED_LINE) | 
                              (1 << MARKER_ADDED_LINE) | (1 << MARKER_REMOVED_LINE) |
                              (1 << MARKER_BLANK_LINE);

	    int posStart = ::SendMessage(CurView, SCI_GETCURRENTPOS, 0, 0 );
	    int lineStart = ::SendMessage(CurView, SCI_LINEFROMPOSITION, posStart, 0 );
	    int lineMax = ::SendMessage(CurView, SCI_GETLINECOUNT, 0, 0 );
	    int currLine = 0;
	    int nextLine = ::SendMessage(CurView, SCI_MARKERNEXT, currLine, sci_search_mask );
	    ::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0 );
	    ::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0 );
    }
}

void Last(void)
{
    if (active)
    {
        HWND CurView = NULL;
        int currentEdit;
        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);

        if (currentEdit != -1) 
        {
            CurView = (currentEdit == 0) ? (nppData._scintillaMainHandle) : (nppData._scintillaSecondHandle);
        }

        int sci_search_mask = (1 << MARKER_MOVED_LINE) | (1 << MARKER_CHANGED_LINE) | 
                              (1 << MARKER_ADDED_LINE) | (1 << MARKER_REMOVED_LINE) |
                              (1 << MARKER_BLANK_LINE);

	    int posStart = ::SendMessage(CurView, SCI_GETCURRENTPOS, 0, 0 );
	    int lineStart = ::SendMessage(CurView, SCI_LINEFROMPOSITION, posStart, 0 );
	    int lineMax = ::SendMessage(CurView, SCI_GETLINECOUNT, 0, 0 );
	    int nextLine = ::SendMessage(CurView, SCI_MARKERPREVIOUS, lineMax, sci_search_mask );
	    ::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0 );
	    ::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0 );
    }
}

void openAboutDlg(void)
{
    AboutDlg.doDialog();
}

void ViewNavigationBar(void)
{
    //NavDlg.doDialog(Settings.NavBar);
}

HWND getCurrentHScintilla(int which)
{
    return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
};


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (Message == WM_CREATE)
    {
        HMENU hMenu = ::GetMenu(nppData._nppHandle);
        ::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_1]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
        ::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_2]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
        ::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_3]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
        ::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_4]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
        ::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, MF_BYCOMMAND | MF_GRAYED);
        SendMessage(nppData._nppHandle, TB_ENABLEBUTTON, CMD_PREV, MAKELONG(FALSE, 0));
    }

    return TRUE;
}

HWND openTempFile(void)
    {
    /*if(PathFileExists(compareFilePath)==false){
    ofstream myfile;
    myfile.open (compareFilePath);
    myfile << "Blank File for compare use";
    myfile.close();
    }*/
    char original[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)original);
    HWND originalwindow = getCurrentWindow();	

    int result = ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
    HWND window = getCurrentWindow();		
    int win = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
    
    if(result == 0 || win != tempWindow)
    {
        ::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_NEW, (LPARAM)0);
        ::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)compareFilePath);
        tempWindow = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
    }	

    if(originalwindow == window)
    {
        SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
        panelsOpened = true;
    }

    result=::SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE, 0, (LPARAM)original);

    window = getOtherWindow();

    int pointer = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
    if(tempWindow != pointer)
    {
        window = getCurrentWindow();
        pointer = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
    }

    assert(tempWindow == pointer);

    //move focus to new document, or the other document will be marked as dirty
    ::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
    ::SendMessageA(window, SCI_SETREADONLY, 0, 0);
    ::SendMessageA(window, SCI_CLEARALL, 0, 0);

    return window;
}

void openFile(TCHAR *file)
{
    if(file == NULL || PathFileExists(file) == FALSE)
    {
        ::MessageBox(nppData._nppHandle, TEXT("No file to open"), TEXT("error"), MB_OK);
        return;
    }

    HWND window = openTempFile();

    //ifstream::pos_type size;
    ifstream myfile(file,ios::in|ios::ate| ios::binary);

    if(myfile.is_open())
    {		
        long size = myfile.tellg();
        char *memblock = new char [size+1];
        myfile.seekg (0);
        myfile.read (memblock, size);
        myfile.close();	

        memblock[size] = 0;
        ::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
        ::SendMessageA(window, SCI_APPENDTEXT, size, (LPARAM)memblock);	
        delete[] memblock;

        if(startCompare())
        {
            ::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
            ::SendMessageA(window, SCI_SETSAVEPOINT, 1, 0);
            ::SendMessageA(window, SCI_EMPTYUNDOBUFFER, 0, 0);
            ::SendMessageA(window, SCI_SETREADONLY, 1, 0);
            reset();
        }
        else
        {
            ::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
            ::SendMessageA(window, SCI_SETSAVEPOINT, 1, 0);
            ::SendMessageA(window, SCI_EMPTYUNDOBUFFER, 0, 0);
            ::SendMessageA(window, SCI_SETREADONLY, 1, 0);            
        }
    }
    //return;
}

void reset()
{
    if (active == true)
    {
        int doc1 = SendMessageA(nppData._scintillaMainHandle, SCI_GETDOCPOINTER, 0, 0);
        int doc2 = SendMessageA(nppData._scintillaSecondHandle, SCI_GETDOCPOINTER, 0, 0);

        int doc1Index = getCompare(doc1);
        int doc2Index = getCompare(doc2);
        int win = 3;

        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&win);
        HWND window = ::getCurrentHScintilla(win);

        if(doc1Index != -1)
        {
            clearWindow(nppData._scintillaMainHandle, true);
        }
        if(doc2Index != -1)
        {
            clearWindow(nppData._scintillaSecondHandle, true);
        }

        ::SendMessageA(window, SCI_GRABFOCUS, 0, (LPARAM)1);

        ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
        ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);	

        if(panelsOpened)
        {
            ::SendMessageA(nppData._scintillaSecondHandle, SCI_GRABFOCUS, 0, (LPARAM)0);
            SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
        }

        if(tempWindow!=-1)
        {
            ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
            window = getCurrentWindow();	
            int tempPointer = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
            
            if(tempPointer == tempWindow)
            {
                SendMessageA(window,SCI_EMPTYUNDOBUFFER,0,0);
                SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_CLOSE, 0);
            }
            tempWindow = -1;
        }

        // Remove margin mask
        ::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)0);
        ::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)0);

        // Remove margin
        ::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)0);
        ::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)0);

        removeCompare(doc1);
        removeCompare(doc2);

        panelsOpened = false;
        active = false;

        // Close NavBar
        NavDlg.doDialog(false);

        // Disable Prev/Next menu entry
        HMENU hMenu = ::GetMenu(nppData._nppHandle);
        ::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, MF_BYCOMMAND | MF_GRAYED);
    }
}

unsigned int getLineFromIndex(unsigned int *arr, int index, void * /*context*/)
{
    return arr[index];
}

int compareLines(unsigned int line1, unsigned int line2, void * /*context*/)
{

    if(line1 == line2) return 0;
    return -1;
}

int checkWords(diff_edit* e,chunk_info* chunk,chunk_info* otherChunk)
{
    Word *word=(Word*)varray_get(chunk->words,e->off);
    int start=word->line;
    word=(Word*)varray_get(chunk->words,e->off+e->len-1);
    int end=word->line;
    assert(start<=end);
    int line2=chunk->lineMappings[start];
    int len=e->len;
    int off=e->off;

    //if the beginning is not at the start of the line, than its definetely a change;
    if(line2==-1){
        //if this line is not matched to another line, don't bother checking it
        if(start==end){
            return chunk->changeCount;
        }else{
            //len=;
            e->len-=chunk->lineEndPos[start]-e->off;
            e->off=chunk->lineEndPos[start];
            start++;
        }
    }else if(e->off!=chunk->linePos[start]){
        struct diff_change *change=(diff_change*) varray_get(chunk->changes, chunk->changeCount++);
        struct diff_change *change2=(diff_change*) varray_get(otherChunk->changes, otherChunk->changeCount++);

        change2->line=line2;

        change2->len=0;
        change2->off=0;
        change2->matchedLine=chunk->lineStart+start;

        word=(Word*)varray_get(chunk->words,e->off);
        assert(word->line==start);
        change->off=word->pos;
        change->line=start;
        change->matchedLine=otherChunk->lineStart+line2;




        //multiline change or single line change
        if(start!=end){
            int len=chunk->lineEndPos[start]-e->off;
            assert(len>0);


            word=(Word*)varray_get(chunk->words,e->off+len-1);
            e->off=chunk->lineEndPos[start];
            e->len-=len;
            assert(word->length>0);
            assert(word->line==start);
            change->len=(word->pos+word->length)-change->off;
            assert(change->len>=0);

            start++;
        }else{
            int len=e->len;
            word=(Word*)varray_get(chunk->words,e->off+len-1);
            assert(word->length>0);
            assert(word->line==change->line);
            change->len=(word->pos+word->length)-change->off;
            assert(change->len>=0);
            return chunk->changeCount;
        }				
    }

    //if a change spans more than one line, all the middle lines are just inserts or deletes
    while(start!=end){
        //potentially a inserted line					
        e->off=chunk->lineEndPos[start];
        e->len-=(chunk->lineEndPos[start]-chunk->linePos[start]);
        start++;


    }
    line2=chunk->lineMappings[start];
    //if the change does not go to the end of the line than its definetely a change
    if(line2!=-1 && (e->off+e->len)<chunk->lineEndPos[start]){

        //todo recheck change because some of the diffs will be previous lines
        struct diff_change *change=(diff_change*) varray_get(chunk->changes, chunk->changeCount++);
        struct diff_change *change2=(diff_change*) varray_get(otherChunk->changes, otherChunk->changeCount++);
        //offset+=(direction*e->len);

        change2->line=line2;//getLineFromPos(chunk->mappings[e->off],otherChunk->lineEndPos,otherChunk->lineCount,false);				
        change2->matchedLine=chunk->lineStart+start;
        change2->len=0;
        change2->off=0;

        word=(Word*)varray_get(chunk->words,e->off);
        assert(word->line==start);
        change->off=word->pos;
        int len=e->len;
        word=(Word*)varray_get(chunk->words,e->off+len-1);
        assert(word->length>0);
        change->len=(word->pos+word->length)-change->off;

        change->line=start;	
        assert(word->line==change->line);
        assert(change->len>=0);
        change->matchedLine=otherChunk->lineStart+line2;
    }
    e->off=off;
    e->len=len;
    return chunk->changeCount;
}

wordType getWordType(char letter){
    switch(letter){
            case ' ':
            case '\t':
                return SPACECHAR;
            default:
                if((letter>='a' && letter<='z')|| (letter>='A' && letter<='Z') || (letter>='0' && letter<='9')){
                    return ALPHANUMCHAR;
                }
                return OTHERCHAR;

                break;
    }

}

int getWords(diff_edit* e, char** doc,chunk_info *chunk  ){

    varray *words=chunk->words;
    int wordIndex=0;
    chunk->lineEndPos=new int[chunk->lineCount];
    chunk->linePos=new int[chunk->lineCount];
    for(int line=0;line<(e->len);line++){
        string text=string("");
        wordType type=SPACECHAR;
        int len=0;
        chunk->linePos[line]=wordIndex;
        int i=0;
        unsigned int hash=0;
        for(i=0;doc[line+e->off][i]!=0;i++){

            char l=doc[line+e->off][i];
            wordType newType=getWordType(l);
            if(newType==type){
                text+=l;
                len++;
                hash=HASH(hash,l);
            }else{
                if(len>0){
                    //if(Settings.IncludeSpace || type!=SPACECHAR){
                    if(!Settings.IncludeSpace || type!=SPACECHAR){
                        Word *word=(Word*)varray_get(words,wordIndex++);
                        //word->text=text;
                        word->length=len;
                        word->line=line;
                        word->pos=i-len;
                        word->type=type;
                        word->hash=hash;
                    }
                }
                type=newType;
                text=l;
                len=1;
                hash=HASH(0,l);

            }

        }
        if(len>0){
            //if(Settings.IncludeSpace || type!=SPACECHAR){
            if(!Settings.IncludeSpace || type!=SPACECHAR){
                Word *word=(Word*)varray_get(words,wordIndex++);
                //word->text=text;
                word->length=len;
                word->line=line;
                word->pos=i-len;
                word->type=type;
                word->hash=hash;
            }
        }
        chunk->lineEndPos[line]=wordIndex;
    }
    return wordIndex;
}

Word *getWord(varray *words, int index, void * /*context*/)
{
    Word *word = (Word*)varray_get(words, index);
    return word;
}

int compareWord(Word *word1, Word *word2, void * /*context*/)
{
    //return word1->text.compare(word2->text);
    if(word1->hash == word2->hash) return 0;
    return 1;
}

bool compareWords(diff_edit* e1,diff_edit *e2,char** doc1,char** doc2){
    //return false;
    chunk_info chunk1;
    chunk1.lineCount=e1->len;
    chunk1.words=varray_new(sizeof(struct Word), NULL);
    chunk1.lineStart=e1->off;
    chunk1.count=getWords(e1,doc1,&chunk1);

    chunk1.lineMappings=new int[e1->len];
    for(int i=0;i<e1->len;i++){
        chunk1.lineMappings[i]=-1;
    }







    chunk_info chunk2;
    chunk2.lineCount=e2->len;
    chunk2.words=varray_new(sizeof(struct Word), NULL);
    chunk2.count=getWords(e2,doc2,&chunk2);
    chunk2.lineStart=e2->off;
    chunk2.lineMappings=new int[e2->len];
    for(int i=0;i<e2->len;i++){
        chunk2.lineMappings[i]=-1;
    }

    //Compare the two chunks
    int sn;
    struct varray *ses = varray_new(sizeof(struct diff_edit), NULL);

    //int result=(diff(chunk1.words, 0, chunk1.count, chunk2.words, 0, chunk2.count, (idx_fn)(getWord), (cmp_fn)(compareWord), NULL, 0, ses, &sn, NULL));

    diff(chunk1.words, 0, chunk1.count, chunk2.words, 0, chunk2.count, (idx_fn)(getWord), (cmp_fn)(compareWord), NULL, 0, ses, &sn, NULL);

    chunk1.changes = varray_new(sizeof(struct diff_change), NULL);
    chunk2.changes = varray_new(sizeof(struct diff_change), NULL);

    int offset=0;
    int **lineMappings1=new int*[chunk1.lineCount];
    for(int i=0;i<chunk1.lineCount;i++){
        lineMappings1[i]=new int[chunk2.lineCount];
        for(int j=0;j<chunk2.lineCount;j++){
            lineMappings1[i][j]=0;
        }
    }

    /// Use the MATCH results to syncronise line numbers
    /// count how many are on each line, than select the line with the most matches
    for (int i = 0; i < sn; i++) {
        struct diff_edit *e =(diff_edit*) varray_get(ses, i);
        //results[i].len=e->len;
        //results[i].off=e->off;
        //results[i].op=e->op;
        if(e->op==DIFF_DELETE){			
            offset-=e->len;	
        }else if(e->op==DIFF_INSERT){			
            offset+=e->len;	
        }else{

            for(int index=e->off;index<(e->off+e->len);index++){
                Word *word1=(Word*)varray_get(chunk1.words,index);
                Word *word2=(Word*)varray_get(chunk2.words,index+offset);
                if(word1->type!=SPACECHAR){
                    int line1a=word1->line;
                    int line2a=word2->line;

                    lineMappings1[line1a][line2a]+=word1->length;
                }


            }


        }
    }

    // go through each line, and select the line with the highest strength
    for(int i=0;i<chunk1.lineCount;i++){
        int line=-1;
        int max=0;
        for(int j=0;j<chunk2.lineCount;j++){
            if(lineMappings1[i][j]>max && (e2->moves==NULL||e2->moves[j]==-1) ){
                line=j;
                max=lineMappings1[i][j];
            }			
        }
        //make sure that the line isnt already matched to another line, and that enough of the line is matched to be significant
        int size=strlen(doc1[e1->off+i]);
        if(line!=-1 && chunk2.lineMappings[line]==-1 && max>(size/3) &&(e1->moves==NULL|| e1->moves[i]==-1)){
            chunk1.lineMappings[i]=line;
            chunk2.lineMappings[line]=i;
        }
    }
    //find all the differences between the lines
    chunk1.changeCount=0;
    chunk2.changeCount=0;

    for (int i = 0; i < sn; i++) {
        struct diff_edit *e =(diff_edit*) varray_get(ses, i);
        if(e->op==DIFF_DELETE){
            //Differences for Doc 1

            checkWords(e,&chunk1,&chunk2);

        }else if(e->op==DIFF_INSERT){			
            //Differences for Doc2
            checkWords(e,&chunk2,&chunk1);
        }
    }
    e1->changeCount=chunk1.changeCount;
    e1->changes=chunk1.changes;
    e2->changeCount=chunk2.changeCount;
    e2->changes=chunk2.changes;


#if CLEANUP
    for(int i=0;i<chunk1.lineCount;i++){
        delete[] lineMappings1[i];
    }
    delete[] lineMappings1;

    delete[] chunk1.lineMappings;
    delete[] chunk1.lineEndPos;
    delete[] chunk1.linePos;
    delete[] chunk2.lineMappings;
    delete[] chunk2.lineEndPos;
    delete[] chunk2.linePos;
    for(int i=chunk1.count;i>=0;i--){
        //Word *w=(Word*)varray_get(chunk1.words,i);
        //w->text.clear();		
        //w->text="";
        //delete w->text;
        varray_release(chunk1.words,i);
    }
    delete[] chunk1.words;
    for(int i=chunk2.count;i>=0;i--){
        //Word *w=(Word*)varray_get(chunk2.words,i);
        //w->text.clear();
        //w->text="";
        //delete w->text;
        varray_release(chunk2.words,i);
    }
    //varray_deinit(chunk2.words);
    delete[] chunk2.words;

    for(int i=sn;i>=0;i--){
        varray_release(ses,i);
    }
    delete ses;
#endif


    return chunk1.changeCount+chunk2.changeCount>0;
    //return true;
}


//change the blocks of diffs to one diff per line. revert a "Changed" line to a insert or delete line if there are no changes
//int setDiffLines(diff_edit *e,diff_edit changes[],int *i, int op,int altLocation){
int setDiffLines(diff_edit *e,diff_edit changes[],int *i, short op,int altLocation){
    int index=*i;
    int addedLines=0;
    //int startLocation = altLocation;
    for(int j=0;j<(e->len);j++){

        changes[index].set=e->set;
        changes[index].len=1;
        changes[index].op=e->op;
        changes[index].off=e->off+j;
        changes[index].changes=NULL;
        changes[index].changeCount=0;
        changes[index].moves=NULL;

        //see if line is already marked as move
        if(e->moves!=NULL && e->moves[j]!=-1){
            changes[index].op=DIFF_MOVE;
            changes[index].matchedLine=e->moves[j];
            changes[index].altLocation=altLocation;
            addedLines++;
        }else{
            for(int k=0;k<e->changeCount;k++){
                struct diff_change *change =(diff_change*) varray_get(e->changes, k);
                if(change->line==j){
                    changes[index].matchedLine=change->matchedLine;
                    changes[index].altLocation=altLocation;
                    if(altLocation!=change->matchedLine ){

                        int diff=altLocation-change->matchedLine;
                        altLocation=change->matchedLine;
                        for(int i=1;i<=j;i++){
                            if(changes[index-i].changes!=NULL){
                                break;
                            }
                            if(op==DIFF_DELETE){
                                changes[index-i].altLocation=change->matchedLine+diff;
                            }else{
                                changes[index-i].altLocation=change->matchedLine;
                            }
                        }
                        if(op==DIFF_INSERT){
                            altLocation+=diff;
                        }


                    }


                    //assert(altLocation==changes[index].matchedLine);
                    if(changes[index].changes==NULL){
                        changes[index].changes=varray_new(sizeof(struct diff_change), NULL);
                    }
                    struct diff_change *newChange =(diff_change*) varray_get(changes[index].changes, changes[index].changeCount++);
                    newChange->len=change->len;
                    newChange->off=change->off;							
                }
            }
            if(changes[index].changes==NULL){
                changes[index].op=op;
                changes[index].altLocation=altLocation;
                addedLines++;
            }else{
                if(op==DIFF_DELETE){
                    altLocation++;
                }else{
                    altLocation++;
                }
            }
        }

        index++;
    }
    *i=index;
    return addedLines;

}
//Move algorithm:
//scan for lines that are only in the other document once
//use one-to-one match as an anchor
//scan to see if the lines above and below anchor also match
diff_edit *find_anchor(int line,varray *ses,int sn,unsigned int *doc1,unsigned int *doc2, int *line2){
    diff_edit *insert=NULL;
    int matches=0;
    for(int i=0;i<sn;i++){
        diff_edit *e=(diff_edit*)varray_get(ses,i);
        if(e->op==DIFF_INSERT){
            for(int j=0;j<e->len;j++){
                //if(e->moves[j]==-1){
                if(compareLines(doc1[line],doc2[e->off+j],NULL)==0){
                    *line2=j;
                    insert=e;
                    matches++;
                }
                //}
            }
        }
    }
    if(matches!=1 || insert->moves[*line2]!=-1){
        return NULL;
    }
    matches=0;
    for(int i=0;i<sn;i++){
        diff_edit *e=(diff_edit*)varray_get(ses,i);
        if(e->op==DIFF_DELETE){
            for(int j=0;j<e->len;j++){
                //if(e->moves[j]==-1){
                if(compareLines(doc1[line],doc1[e->off+j],NULL)==0){
                    //line2=j;
                    //insert=e;
                    matches++;
                }
                //}
            }
        }
    }
    if(matches!=1){
        return NULL;
    }
    return insert;
}

void find_moves(varray *ses,int sn,unsigned int *doc1, unsigned int *doc2){
    //init moves arrays
    for(int i=0;i<sn;i++){
        diff_edit *e=(diff_edit*)varray_get(ses,i);
        if(e->op!=DIFF_MATCH){
            e->moves=new int[e->len];
            for(int j=0;j<e->len;j++){
                e->moves[j]=-1;
            }
        }else{
            e->moves=NULL;
        }
    }

    if(Settings.DetectMove==false){return;}
    for(int i=0;i<sn;i++){
        diff_edit *e=(diff_edit*)varray_get(ses,i);
        if(e->op==DIFF_DELETE){
            for(int j=0;j<e->len;j++){
                if(e->moves[j]==-1){
                    int line2;
                    diff_edit *match=find_anchor(e->off+j,ses,sn,doc1,doc2,&line2);
                    if(match!=NULL){
                        e->moves[j]=match->off+line2;
                        match->moves[line2]=e->off+j;
                        int d1=j-1;
                        int d2=line2-1;
                        while(d1>=0 && d2>=0 && e->moves[d1]==-1 && match->moves[d2]==-1 && compareLines(doc1[e->off+d1],doc2[match->off+d2],NULL)==0){
                            e->moves[d1]=match->off+d2;
                            match->moves[d2]=e->off+d1;
                            d1--;
                            d2--;
                        }
                        d1=j+1;
                        d2=line2+1;
                        while(d1<e->len && d2<match->len && e->moves[d1]==-1 && match->moves[d2]==-1 && compareLines(doc1[e->off+d1],doc2[match->off+d2],NULL)==0){
                            e->moves[d1]=match->off+d2;
                            match->moves[d2]=e->off+d1;
                            d1++;
                            d2++;
                        }
                    }
                }
            }
        }
    }
}


//algorithm borrowed from WinMerge
//if the line after the delete is the same as the first line of the delete, shift down
//basically cabbat -abb is the same as -bba
//since most languages start with unique lines and end with repetative lines(end,</node>, }, etc)
//we shift the differences down where its possible so the results will be cleaner

void shift_boundries(varray *ses,int sn,unsigned int *doc1,unsigned int *doc2,int doc1Length,int doc2Length){
    for (int i = 0; i < sn; i++) {
        struct diff_edit *e =(diff_edit*) varray_get(ses, i);
        struct diff_edit *e2=NULL;
        struct diff_edit *e3=NULL;
        int max1=doc1Length;
        int max2=doc2Length;
        int end2;
        if(e->op!=1){
            for(int j=i+1;j<sn;j++){
                e2=(diff_edit*) varray_get(ses, j);
                if(e2->op==e->op){
                    max1=e2->off;
                    max2=e2->off;
                    break;
                }
            }
        }
        if(e->op==DIFF_DELETE){
            e2=(diff_edit*) varray_get(ses, i+1);
            //if theres an insert after a delete, theres a potential match, so both blocks
            //need to be moved at the same time
            if(e2->op==DIFF_INSERT)
            {
                max2=doc2Length;
                for(int j=i+2;j<sn;j++){
                    e3=(diff_edit*) varray_get(ses, j);
                    if(e2->op==e3->op){
                        //max1=e2->off;
                        max2=e3->off;
                        break;
                    }
                }
                end2=e2->off+e2->len;
                i++;
                int end=e->off+e->len;
                while(end<max1 && end2<max2&& compareLines(doc1[e->off],doc1[end],NULL)==0&& compareLines(doc2[e2->off],doc2[end2],NULL)==0  ){
                    end++;
                    end2++;
                    e->off++;
                    e2->off++;
                }


            }else{
                int end=e->off+e->len;
                while(end<max1&& compareLines(doc1[e->off],doc1[end],NULL)==0  ){
                    end++;
                    e->off++;
                }
            }




        }else if(e->op==DIFF_INSERT){
            int end=e->off+e->len;
            while(end<max2&&compareLines(doc2[e->off],doc2[end],NULL)==0 ){
                end++;
                e->off++;
            }
        }
    }
}
unsigned int *computeHashes(char** doc,int docLength){
    unsigned int *hashes=new unsigned int[docLength];
    for(int i=0;i<docLength;i++){
        unsigned int hash=0;
        for(int j=0;doc[i][j]!=0;j++){
            if(doc[i][j]==' ' || doc[i][j]=='\t'){
                //if(Settings.IncludeSpace){
                if(!Settings.IncludeSpace){
                    hash=HASH(hash,doc[i][j]);
                }
            }else{
                hash=HASH(hash,doc[i][j]);
            }
        }
        hashes[i]=hash;
    }
    return hashes;
}

void clearEdit(diff_edit *e){
    if(e->moves!=NULL){
        delete[] e->moves;
    }
    if(e->changes!=NULL){
        for(int j=e->changeCount;j>=0;j--){
            varray_release(e->changes,j);
        }
        delete e->changes;
    }
}


void clearEdits(varray *ses,int sn){
    for(int i=sn;i>=0;i--){
        struct diff_edit *e =(diff_edit*) varray_get(ses, i);
        clearEdit(e);
        varray_release(ses,i);
        //delete e;
    }
    delete ses;
}


bool compareNew()
{
	//clear(true);
	wait();

	clearWindow(nppData._scintillaMainHandle, true);
	clearWindow(nppData._scintillaSecondHandle, true);
	
    //return false;
	
    active = true;

	int doc1Length;
	int *lineNum1;

	char **doc1 = getAllLines(nppData._scintillaMainHandle, &doc1Length, &lineNum1);

	if(doc1Length < 1)
    {
        return true;
    }

	int doc2Length;
	int *lineNum2;

	char **doc2 = getAllLines(nppData._scintillaSecondHandle, &doc2Length, &lineNum2);
	
    if(doc2Length < 1)
    {
        return true;
    }

	int	doc1Changed = 0;
	int	doc2Changed = 0;
	diff_edit *doc1Changes = NULL;
	diff_edit *doc2Changes = NULL;

	unsigned int *doc1Hashes = computeHashes(doc1, doc1Length);
	unsigned int *doc2Hashes = computeHashes(doc2, doc2Length);

	/* make diff */
	int sn;
	struct varray *ses = varray_new(sizeof(struct diff_edit), NULL);
	int result = (diff(doc1Hashes, 0, doc1Length, doc2Hashes, 0, doc2Length, (idx_fn)(getLineFromIndex), (cmp_fn)(compareLines), NULL, 0, ses, &sn, NULL));
	//int result = (diff(doc1, 0, doc1Length, doc2, 0, doc2Length, (idx_fn)(getLineFromIndex), (cmp_fn)(compareLines), NULL, 0, ses, &sn, NULL));
	int changeOffset = 0;
	//ready();
	//wait();
	shift_boundries(ses, sn, doc1Hashes, doc2Hashes, doc1Length, doc2Length);
	find_moves(ses, sn, doc1Hashes, doc2Hashes);
	/* - insert empty lines
	* - count changed lines
	*/
	doc1Changed = 0;
	doc2Changed = 0;

	for (int i = 0; i < sn; i++) 
    {
		struct diff_edit *e =(diff_edit*) varray_get(ses, i);
		if(e->op == DIFF_DELETE)
        {
			e->changeCount = 0;
			doc1Changed += e->len;
			struct diff_edit *e2 =(diff_edit*) varray_get(ses, i+1);
			e2->changeCount = 0;
			
            if(e2->op == DIFF_INSERT)
            {
				//see if the DELETE/INSERT COMBO includes changed lines or if its a completely new block
				if(compareWords(e, e2, doc1, doc2))
				{
					e->op = DIFF_CHANGE1;
					e2->op = DIFF_CHANGE2;
					doc2Changed += e2->len;
				}
                else
                {
					//addEmptyLines(nppData._scintillaSecondHandle, e->off+doc2Changed-changeOffset, e->len);	
				}

			}
            else
            {
				//addEmptyLines(nppData._scintillaSecondHandle, e->off+doc2Changed-changeOffset, e->len);	
			}
		}
        else if(e->op == DIFF_INSERT)
        {
			e->changeCount = 0;
			doc2Changed += e->len;
			//addEmptyLines(nppData._scintillaMainHandle, e->off+doc1Changed, e->len);
		}

	}

	int doc1CurrentChange = 0;
	int doc2CurrentChange = 0;
	changeOffset = 0;
	doc1Changes = new diff_edit[doc1Changed];
	doc2Changes = new diff_edit[doc2Changed];
	int doc1Offset = 0;
	int doc2Offset = 0;

	//switch from blocks of lines to one change per line. Change CHANGE to DELETE or INSERT if there are no changes on that line
	int added;

	for (int i = 0; i < sn; i++) 
    {
		struct diff_edit *e =(diff_edit*) varray_get(ses, i);
		e->set = i;

		switch(e->op)
        {
			case DIFF_CHANGE1:
			case DIFF_DELETE:
				added = setDiffLines(e, doc1Changes, &doc1CurrentChange, DIFF_DELETE, e->off + doc2Offset);
				doc2Offset -= added;
				doc1Offset += added;
				break;
			case DIFF_INSERT:
			case DIFF_CHANGE2:
				added = setDiffLines(e, doc2Changes, &doc2CurrentChange, DIFF_INSERT, e->off + doc1Offset);	
				doc1Offset -= added;
				doc2Offset += added;
				break;
		}
	}

	if (result != -1)
    {
		int textIndex;
		bool different = (doc1Changed > 0) || (doc2Changed > 0);
		
        for(int i = 0; i < doc1Changed; i++)
        {
			switch(doc1Changes[i].op)
            {
				case DIFF_DELETE:
					markAsRemoved(nppData._scintillaMainHandle, doc1Changes[i].off);					
					break;
				case DIFF_CHANGE1:
					markAsChanged(nppData._scintillaMainHandle, doc1Changes[i].off);
					textIndex = lineNum1[doc1Changes[i].off];

					for(int k = 0; k < doc1Changes[i].changeCount; k++)
                    {
						struct diff_change *change = (diff_change*)varray_get(doc1Changes[i].changes, k);
						markTextAsChanged(nppData._scintillaMainHandle, textIndex + change->off, change->len);
					}
					break;
				case DIFF_MOVE:					
					markAsMoved(nppData._scintillaMainHandle, doc1Changes[i].off);
					break;

			}
		}

		for(int i = 0; i < doc2Changed; i++)
        {
			switch(doc2Changes[i].op)
            {
				case DIFF_INSERT:					
					markAsAdded(nppData._scintillaSecondHandle, doc2Changes[i].off);						
					break;
				case DIFF_CHANGE2:
					markAsChanged(nppData._scintillaSecondHandle, doc2Changes[i].off);
					textIndex = lineNum2[doc2Changes[i].off];
					for(int k = 0; k < doc2Changes[i].changeCount; k++)
                    {
						struct diff_change *change=(diff_change*)varray_get(doc2Changes[i].changes, k);
						markTextAsChanged(nppData._scintillaSecondHandle, textIndex+change->off, change->len);
					}
					break;
				case DIFF_MOVE:					
					markAsMoved(nppData._scintillaSecondHandle,doc2Changes[i].off);										
					break;
			}

		}

		doc1Offset = 0;
		doc2Offset = 0;

		if(Settings.AddLine)
        {
			int length = 0;
			int off = -1;
			for(int i = 0; i < doc1Changed; i++)
            {
				switch(doc1Changes[i].op)
                {
					case DIFF_DELETE:
					case DIFF_MOVE:							
						if(doc1Changes[i].altLocation == off)
                        {
							length++;
						}
                        else
                        {
							addEmptyLines(nppData._scintillaSecondHandle, off + doc2Offset, length);
							doc2Offset += length;
							off = doc1Changes[i].altLocation;
							length = 1;						
						}
						//doc1Offset+=1;
						break;
				}
			}

			addEmptyLines(nppData._scintillaSecondHandle, off + doc2Offset, length);

			if(doc2Offset > 0)
            {
				clearUndoBuffer(nppData._scintillaSecondHandle);
			}

			length = 0;
			off = 0;
			doc1Offset = 0;
			//doc2Offset=0;

			for(int i = 0; i < doc2Changed; i++)
            {
				switch(doc2Changes[i].op)
                {
					case DIFF_INSERT:
					case DIFF_MOVE:							
						if(doc2Changes[i].altLocation == off)
                        {
							length++;
						}
                        else
                        {
							addEmptyLines(nppData._scintillaMainHandle, off + doc1Offset, length);
							doc1Offset += length;
							off = doc2Changes[i].altLocation;
							length = 1;						
						}
						//doc1Offset+=1;
						break;
				}
			}

			addEmptyLines(nppData._scintillaMainHandle, off + doc1Offset, length);

			if(doc1Offset > 0)
            {
				clearUndoBuffer(nppData._scintillaMainHandle);
			}

		}

		//clean up resources
#if CLEANUP

		for(int i = 0; i < doc1Length; i++)
        {
			if(*doc1[i] != 0)
            {
				delete[] doc1[i];
			}
            else
            {
				//delete doc1[i];
			}
		}
		delete[] doc1;
		delete[] lineNum1;
		
        for(int i = 0; i < doc2Length; i++)
        {
			if(*doc2[i] != 0)
            {
				delete[] doc2[i];
			}
            else
            {
				//delete doc2[i];
			}
		}
		delete[] doc2;
		delete lineNum2;

		delete[] doc1Hashes;
		delete[] doc2Hashes;

		clearEdits(ses, sn);

		for(int i = 0; i < doc1Changed; i++)
        {
			clearEdit(doc1Changes + (i));
		}
		delete[] doc1Changes;

		for(int i = 0; i < doc2Changed; i++)
        {
			clearEdit(doc2Changes+(i));
		}
		delete[] doc2Changes;

#endif // CLEANUP

        ready();

		if(!different)
        {
			::MessageBox(nppData._nppHandle, TEXT("Files Match"), TEXT("Results :"), MB_OK);
			return true;
		}

		::SendMessageA(nppData._scintillaMainHandle, SCI_SHOWLINES, 0, (LPARAM)1);
		::SendMessageA(nppData._scintillaSecondHandle, SCI_SHOWLINES, 0, (LPARAM)1);	
		return false;
	}
    return false;
}

bool startCompare()
{
    int win = 3;

    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&win);
    HWND window = ::getCurrentHScintilla(win);

    if(!IsWindowVisible(nppData._scintillaMainHandle) || !IsWindowVisible(nppData._scintillaSecondHandle))
    {	
        SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
        panelsOpened = true;
    }

    if(!IsWindowVisible(nppData._scintillaMainHandle) || !IsWindowVisible(nppData._scintillaSecondHandle))
    {	
        panelsOpened = false;
        ::MessageBox(nppData._nppHandle, TEXT("Nothing to compare!"), TEXT("Error"), MB_OK);
        return true;
    }

    if(!notepadVersionOk)
    {
        int version = ::SendMessage(nppData._nppHandle,NPPM_GETNPPVERSION, 0, 0);
        if(version > 0)
        {
            notepadVersionOk = true;
        }
    }

    if(Settings.AddLine && !notepadVersionOk)
    {
        ::MessageBox(nppData._nppHandle, TEXT("Notepad v4.5 or higher is required to line up matches. This feature will be turned off"), TEXT("Incorrect Version"), MB_OK);
        //return;
        Settings.AddLine = false;
        ::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_ALIGN_MATCHES]._cmdID, (LPARAM)Settings.AddLine);
    }

    SendMessageA(nppData._scintillaMainHandle, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
    SendMessageA(nppData._scintillaSecondHandle, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);

    setStyles(Settings);

    int doc1 = SendMessageA(nppData._scintillaMainHandle, SCI_GETDOCPOINTER, 0, 0);
    int doc2 = SendMessageA(nppData._scintillaSecondHandle, SCI_GETDOCPOINTER, 0, 0);

    setCompare(doc1);
    setCompare(doc2);

    /* sync pannels */
    HMENU hMenu = ::GetMenu(nppData._nppHandle);

    if ((::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0)
    {
        ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
    }

    if ((::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0)
    {
        ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);
    }

    ::SendMessageA(nppData._scintillaMainHandle, SCI_GOTOPOS, 1, 0);
    ::SendMessageA(nppData._scintillaSecondHandle, SCI_GOTOPOS, 1, 0);
    ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
    ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);
    ::SendMessageA(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);
    ::SendMessageA(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);

    bool result = compareNew();
    //return;
    ::SendMessageA(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);
    ::SendMessageA(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);
    ::SendMessageA(window, SCI_GRABFOCUS, 0, (LPARAM)1);

    // Save current N++ focus
    HWND hwnd = GetFocus();

    // Configure NavBar
    NavDlg.SetColor(
        Settings.ColorSettings.added, 
        Settings.ColorSettings.deleted, 
        Settings.ColorSettings.changed, 
        Settings.ColorSettings.moved, 
        Settings.ColorSettings.blank);

    // Display Navbar
    NavDlg.doDialog(true);

    // Restore N++ focus
    SetFocus(hwnd);

    // Enable Prev/Next menu entry
    ::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, MF_BYCOMMAND | MF_ENABLED);

    return result;
}

void compare()
{
    startCompare();
    ready();
}

void cleanEmptyLines(blankLineList *line)
{
    if(line->next != NULL)
    {
        cleanEmptyLines(line->next);
        delete line->next;
    }
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
    switch (notifyCode->nmhdr.code) 
    {
    case NPPN_TBMODIFICATION:
        {         
            //HMENU hMenu = ::GetMenu(nppData._nppHandle);

            tbNext.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_NEXT), IMAGE_BITMAP, 0, 0, (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
            tbPrev.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_PREV), IMAGE_BITMAP, 0, 0, (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
            tbFirst.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_FIRST), IMAGE_BITMAP, 0, 0, (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
            tbLast.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_LAST), IMAGE_BITMAP, 0, 0, (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));

			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_FIRST]._cmdID, (LPARAM)&tbFirst);
            ::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_PREV]._cmdID, (LPARAM)&tbPrev);
            ::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_NEXT]._cmdID, (LPARAM)&tbNext);
            ::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_LAST]._cmdID, (LPARAM)&tbLast);

            ::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_ALIGN_MATCHES]._cmdID, (LPARAM)Settings.AddLine);
            ::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACING]._cmdID, (LPARAM)Settings.IncludeSpace);
            ::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID, (LPARAM)Settings.DetectMove);

            if (FirstRun)
            {
                MessageBox(
                    NULL, 
		            TEXT("This is the first time you launch Compare 1.5.4.\r\n\r\n")
		            TEXT("This release include a comparison results graphical view that is still in beta version.\r\n")
                    TEXT("So please do not try to undock/move/resize it.\r\n")
		            TEXT("\r\n")
		            TEXT("Hopefully you find this to be a useful tool.  Enjoy!\r\n\r\n")
		            TEXT("Jean-Sébastien Leroy"),            
                    TEXT("Compare 1.5.4 - Beta version"), 
                    MB_ICONWARNING);
            }

            break;
        }
    case NPPN_FILEBEFORECLOSE:
    case NPPN_FILECLOSED:
    case NPPN_FILEBEFOREOPEN:
    case NPPN_FILEOPENED:

        notepadVersionOk = true;
        break;

    case NPPN_FILEBEFORESAVE:
        {
            notepadVersionOk=true;
            //char name[MAX_PATH];
            SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)emptyLinesDoc);
            //int win=3;
            //SendMessage(nppData._nppHandle,NPPM_GETCURRENTSCINTILLA,0,(LPARAM)&win);
            HWND window=getCurrentWindow();						
            int win=SendMessageA(window, SCI_GETDOCPOINTER, 0,0);

            if(getCompare(win)!=-1)
            {				
                topLine=SendMessageA(window,SCI_GETFIRSTVISIBLELINE,0,0);
                lastEmptyLines=removeEmptyLines(window,true);
            }
            else
            {
                lastEmptyLines=NULL;
            }

            //int k=1;
            break;
            //setCursor(SC_CURSORWAIT);
        }
    case NPPN_FILESAVED:
        {
            notepadVersionOk=true;
            TCHAR name[MAX_PATH];
            SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,0,(LPARAM)name);
            if(lastEmptyLines!=NULL&& lstrcmp(name,emptyLinesDoc)==0){
                HWND window=getCurrentWindow();
                ::addBlankLines(window,lastEmptyLines);
                int linesOnScreen=SendMessageA(window,SCI_LINESONSCREEN,0,0);

                int curPosBeg = ::SendMessageA(window, SCI_GETSELECTIONSTART, 0, 0);
                int curPosEnd = ::SendMessageA(window, SCI_GETSELECTIONEND, 0, 0);
                SendMessageA(window,SCI_GOTOLINE,topLine,0);
                SendMessageA(window,SCI_GOTOLINE,topLine+linesOnScreen-1,0);

                ::SendMessageA(window, SCI_SETSEL, curPosBeg, curPosEnd);
                cleanEmptyLines(lastEmptyLines);
                delete lastEmptyLines;
                lastEmptyLines=NULL;
                emptyLinesDoc[0]=0;

            }
            break;
            //setCursor(SC_CURSORNORMAL);
        }
        break;
    }
}