#include <tchar.h>
#include <shlwapi.h>
#include <stdlib.h>
#include "../src/Common/Notepad_plus_msgs.h"

bool debug = 0;

void showError(TCHAR* errMsg, ...)
{
    TCHAR msg[512];
    TCHAR appName[MAX_PATH];

	va_list argptr;
	va_start(argptr, errMsg);
	#pragma warning(disable: 4996)
    vswprintf(msg, errMsg, argptr);
	#pragma warning(default: 4996)
    va_end(argptr);

    GetModuleFileName(NULL, appName, _countof(appName));
    PathStripPath(appName);

    MessageBox(NULL, msg, appName, MB_OK | MB_ICONERROR);
}

#define quitWithError(errMsg, ...) \
{ \
	showError(errMsg, __VA_ARGS__); \
	return 1; \
}

void log(TCHAR* errMsg, ...)
{
    if (debug)
    {
        TCHAR buffer[512];
        TCHAR msg[512];

        va_list argptr;
        va_start(argptr, errMsg);
        #pragma warning(disable: 4996)
        vswprintf(buffer, errMsg, argptr);
        #pragma warning(default: 4996)
        va_end(argptr);

        _sntprintf_s(msg, _countof(msg), _TRUNCATE, TEXT("Compare loader: %s"), buffer);

        OutputDebugString(msg);
    }
}


BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
	TCHAR buffer[256];
	GetClassName(hwnd, buffer, _countof(buffer));
    log(TEXT("Checking window: %s"), buffer);
	if (!_tcscmp(buffer, TEXT("Notepad++")))
	{
		int ver = SendMessage(hwnd, NPPM_GETNPPVERSION, 0, 0);
        log(TEXT("Checking N++ version: %d.%d"), HIWORD(ver), LOWORD(ver));
		if (HIWORD(ver) >= 6)
		{
            BOOL ret = TRUE; // TRUE = Fail

            log(TEXT("Searching for Compare plug-in menu"));
            HMENU hPluginMenu = (HMENU)SendMessage(hwnd, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
            int iMenuItems = GetMenuItemCount(hPluginMenu);
            for (int i = 0; i < iMenuItems; i++)
            {
                MENUITEMINFO mii;
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
                mii.dwTypeData = buffer;
                mii.cch = _countof(buffer) - 1;
                GetMenuItemInfo(hPluginMenu, i, TRUE, &mii);
                log(TEXT("Checking menu: %s"), buffer);
                if (!_tcscmp(buffer, TEXT("Compare")))
                {
                    log(TEXT("Searching for Compare plug-in sub menu item"));
                    int iSubMenuItems = GetMenuItemCount(mii.hSubMenu);
                    for (int j = 0; j < iSubMenuItems; j++)
                    {
                        MENUITEMINFO smii;
                        smii.cbSize = sizeof(smii);
                        smii.fMask = MIIM_ID | MIIM_STRING;
                        smii.dwTypeData = buffer;
                        smii.cch = _countof(buffer) - 1;
                        GetMenuItemInfo(mii.hSubMenu, i, TRUE, &smii);
                        log(TEXT("Checking sub menu item: %s"), buffer);

                        // remove optional shortcuts
                        int k = 0;
                        while (buffer[k])
                        {
                            if (buffer[k] == '\t')
                            {
                                buffer[k] = 0;
                                log(TEXT("Stripped sub menu item name: %s"), buffer);
                                break;
                            }
                            k++;
                        }

                        if (!_tcscmp(buffer, TEXT("Compare")))
                        {
                            log(TEXT("Sending command message to N++"));
                            SendMessage(hwnd, WM_COMMAND, smii.wID, 0);
                            ret = FALSE; // FALSE = OK
                            break;
                        }
                    }
                    break;
                }
            }

            return ret;
		}
	}
	return TRUE;
}

int _tmain(int argc, TCHAR* argv[])
{
	// read .ini config
	TCHAR iniPath[MAX_PATH];
	TCHAR nppPath[MAX_PATH];
	TCHAR buffer[2048];
	_tcscpy_s(iniPath, _countof(iniPath), argv[0]);
	PathRemoveFileSpec(iniPath);
	PathAppend(iniPath, TEXT("compare.ini"));
	debug = (GetPrivateProfileInt(TEXT("debug"), TEXT("output-debug-string"), 0, iniPath) == 1);

    // some checks
    if (argc < 3)
        quitWithError(TEXT("Missing commandline arguments:\ncompare.exe <file_path_1> <file_path_2>"));
    log(TEXT("%s %s %s", argv[0], argv[1], argv[2]));
    if (!PathFileExists(argv[1]))
        quitWithError(TEXT("Input file not found:\n'%s'"), argv[1]);
    if (!PathFileExists(argv[2]))
        quitWithError(TEXT("Input file not found:\n'%s'"), argv[2]);

	// get notepad++:
	// as configured in the .ini file
	// else as found via relative path
	// else as found via registry
	if (GetPrivateProfileString(TEXT("notepad"), TEXT("path"), NULL, buffer, _countof(buffer), iniPath))
	{
		ExpandEnvironmentStrings(buffer, nppPath, _countof(nppPath));
        log(TEXT("Using configured npp path: %s"), nppPath);
		if (!PathFileExists(nppPath))
			quitWithError(TEXT("Configured Notepad++ path not found:\n'%s'"), nppPath);
	}
	else
	{
		_tcscpy_s(nppPath, _countof(nppPath), argv[0]);
		PathRemoveFileSpec(nppPath);
		PathAppend(nppPath, TEXT("..\\..\\notepad++.exe"));
        log(TEXT("Trying relative npp path: %s"), nppPath);
		if (!PathFileExists(nppPath))
		{
			HKEY hKey;
			DWORD lpcbData = sizeof(nppPath);
            log(TEXT("Looking for npp path in registry (HKLM/SOFTWARE/Notepad++)"));
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Notepad++"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)nppPath, &lpcbData) == ERROR_SUCCESS)
				{
					PathAppend(nppPath, TEXT("notepad++.exe"));
                    log(TEXT("Trying npp path from registry: %s"), nppPath);
				}
				RegCloseKey(hKey);
			}
			if (!PathFileExists(nppPath))
			{
				quitWithError(TEXT("Notepad++.exe couldn't be found\n")
					TEXT("neither by relative path (..\\..\\notepad++.exe)\n")
					TEXT("nor via registry (HKEY_LOCAL_MACHINE\\SOFTWARE\\Notepad++)"));
			}
		}
	}

	STARTUPINFO suInfo;
	ZeroMemory(&suInfo, sizeof(suInfo));
	suInfo.cb = sizeof(suInfo);
	PROCESS_INFORMATION procInfo;
	ZeroMemory(&procInfo, sizeof(procInfo));
	int exitCode = 1;

	_sntprintf_s(buffer, _countof(buffer), _TRUNCATE, TEXT("\"%s\" -nosession -multiInst \"%s\" \"%s\""),
			nppPath, argv[1], argv[2]);
    log(TEXT("Creating process: %s"), buffer);

	if (CreateProcess(NULL, buffer, NULL, NULL, FALSE, NULL, NULL, NULL, &suInfo, &procInfo)) {
		WaitForInputIdle(procInfo.hProcess, 7000);

        log(TEXT("Searching for N++ main window"));
		EnumThreadWindows(procInfo.dwThreadId, EnumThreadWndProc, NULL);

		CloseHandle(procInfo.hProcess);
		CloseHandle(procInfo.hThread);
	}

	return exitCode;
}
