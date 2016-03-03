#include <tchar.h>
#include <Shlwapi.h>
#include "..\src\Common\Notepad_plus_msgs.h"

bool debug = 0;

void showError(_TCHAR* errMsg, ...)
{
    _TCHAR msg[512];
    _TCHAR appName[MAX_PATH];

	va_list argptr;
	va_start(argptr, errMsg);
	#pragma warning(disable: 4996)
    vswprintf(msg, errMsg, argptr);
	#pragma warning(default: 4996)
    va_end(argptr);

    GetModuleFileName(NULL, appName, sizeof(appName));
    PathStripPath(appName);

    MessageBox(NULL, msg, appName, MB_OK | MB_ICONERROR);
}

#define quitWithError(errMsg, ...) \
{ \
	showError(errMsg, __VA_ARGS__); \
	return 1; \
}

void log(_TCHAR* errMsg, ...)
{
    if (debug)
    {
        _TCHAR buffer[512];
        _TCHAR msg[512];

        va_list argptr;
        va_start(argptr, errMsg);
        #pragma warning(disable: 4996)
        vswprintf(buffer, errMsg, argptr);
        #pragma warning(default: 4996)
        va_end(argptr);

        wsprintf(msg, L"Compare loader: %s", buffer);

        OutputDebugString(msg);
    }
}


BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
	_TCHAR buffer[256];
	GetClassName(hwnd, buffer, sizeof(buffer));
    log(L"Checking window: %s", buffer);
	if (!lstrcmp(buffer, L"Notepad++"))
	{
		int ver = SendMessage(hwnd, NPPM_GETNPPVERSION, 0, 0);
        log(L"Checking N++ version: %d.%d", HIWORD(ver), LOWORD(ver));
		if (HIWORD(ver) >= 6)
		{
            BOOL ret = TRUE; // TRUE = Fail

            log(L"Searching for Compare plug-in menu");
            HMENU hPluginMenu = (HMENU)SendMessage(hwnd, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
            int iMenuItems = GetMenuItemCount(hPluginMenu);
            for (int i = 0; i < iMenuItems; i++)
            {
                MENUITEMINFO mii;
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
                mii.dwTypeData = buffer;
                mii.cch = sizeof(buffer) - 1;
                GetMenuItemInfo(hPluginMenu, i, TRUE, &mii);
                log(L"Checking menu: %s", buffer);
                if (!lstrcmp(buffer, L"Compare"))
                {
                    log(L"Searching for Compare plug-in sub menu item");
                    int iSubMenuItems = GetMenuItemCount(mii.hSubMenu);
                    for (int j = 0; j < iSubMenuItems; j++)
                    {
                        MENUITEMINFO smii;
                        smii.cbSize = sizeof(MENUITEMINFO);
                        smii.fMask = MIIM_ID | MIIM_STRING;
                        smii.dwTypeData = buffer;
                        smii.cch = sizeof(buffer) - 1;
                        GetMenuItemInfo(mii.hSubMenu, i, TRUE, &smii);
                        log(L"Checking sub menu item: %s", buffer);

                        // remove optional shortcuts
                        int k = 0;
                        while (buffer[k])
                        {
                            if (buffer[k] == '\t')
                            {
                                buffer[k] = 0;
                                log(L"Stripped sub menu item name: %s", buffer);
                                break;
                            }
                            k++;
                        }

                        if (!lstrcmp(buffer, L"Compare"))
                        {
                            log(L"Sending command message to N++");
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

int _tmain(int argc, _TCHAR* argv[])
{
	// read .ini config
	_TCHAR iniPath[MAX_PATH];
	_TCHAR nppPath[MAX_PATH];
	_TCHAR buffer[2048];
	lstrcpy(iniPath, argv[0]);
	PathRemoveFileSpec(iniPath);
	PathAppend(iniPath, L"compare.ini");
	debug = (GetPrivateProfileInt(L"debug", L"output-debug-string", 0, iniPath) == 1);

    // some checks
    if (argc < 3)
        quitWithError(L"Missing commandline arguments:\ncompare.exe <file_path_1> <file_path_2>");
    log(L"%s %s %s", argv[0], argv[1], argv[2]);
    if (!PathFileExists(argv[1]))
        quitWithError(L"Input file not found:\n'%s'", argv[1]);
    if (!PathFileExists(argv[2]))
        quitWithError(L"Input file not found:\n'%s'", argv[2]);

	// get notepad++:
	// as configured in the .ini file
	// else as found via relative path
	// else as found via registry
	if (GetPrivateProfileString(L"notepad", L"path", NULL, buffer, sizeof(buffer), iniPath)) {
		ExpandEnvironmentStrings(buffer, nppPath, sizeof(nppPath));
        log(L"Using configured npp path: %s", nppPath);
		if (!PathFileExists(nppPath))
			quitWithError(L"Configured Notepad++ path not found:\n'%s'", nppPath);
	}
	else
	{
		lstrcpy(nppPath, argv[0]);
		PathRemoveFileSpec(nppPath);
		PathAppend(nppPath, L"..\\..\\notepad++.exe");
        log(L"Trying relative npp path: %s", nppPath);
		if (!PathFileExists(nppPath))
		{
			HKEY hKey;
			DWORD lpcbData = sizeof(nppPath);
            log(L"Looking for npp path in registry (HKLM/SOFTWARE/Notepad++)");
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Notepad++", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)nppPath, &lpcbData) == ERROR_SUCCESS)
				{
					PathAppend(nppPath, L"notepad++.exe");
                    log(L"Trying npp path from registry: %s", nppPath);
				}
				RegCloseKey(hKey);
			}
			if (!PathFileExists(nppPath))
			{
				quitWithError(L"Notepad++.exe couldn't be found\n"
					L"neither by relative path (..\\..\\notepad++.exe)\n"
					L"nor via registry (HKEY_LOCAL_MACHINE\\SOFTWARE\\Notepad++)");
			}
		}
	}

	STARTUPINFO suInfo;
	ZeroMemory(&suInfo, sizeof(suInfo));
	suInfo.cb = sizeof(suInfo);
	PROCESS_INFORMATION procInfo;
	ZeroMemory(&procInfo, sizeof(procInfo));
	int exitCode = 1;

	wsprintf(buffer, L"\"%s\" -nosession -multiInst \"%s\" \"%s\"", nppPath, argv[1], argv[2]);
    log(L"Creating process: %s", buffer);

	if (CreateProcess(NULL, buffer, NULL, NULL, FALSE, NULL, NULL, NULL, &suInfo, &procInfo)) {
		WaitForInputIdle(procInfo.hProcess, 7000);

        log(L"Searching for N++ main window");
		EnumThreadWindows(procInfo.dwThreadId, EnumThreadWndProc, NULL);

		CloseHandle(procInfo.hProcess);
		CloseHandle(procInfo.hThread);
	}

	return exitCode;
}
