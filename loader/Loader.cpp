#include <tchar.h>
#include <Shlwapi.h>
#include "Notepad_plus_msgs.h"

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

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
	_TCHAR buffer[256];
	GetClassName(hwnd, buffer, sizeof(buffer));
	if (!lstrcmp(buffer, L"Notepad++"))
	{
		int ver = SendMessage(hwnd, NPPM_GETNPPVERSION, 0, 0);
		if (HIWORD(ver) >= 6)
		{
            BOOL ret = TRUE; // TRUE = Fail

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
                if (!lstrcmp(buffer, L"Compare"))
                {
                    int iSubMenuItems = GetMenuItemCount(mii.hSubMenu);
                    for (int j = 0; j < iSubMenuItems; j++)
                    {
                        MENUITEMINFO smii;
                        smii.cbSize = sizeof(MENUITEMINFO);
                        smii.fMask = MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
                        smii.dwTypeData = buffer;
                        smii.cch = sizeof(buffer) - 1;
                        GetMenuItemInfo(mii.hSubMenu, i, TRUE, &smii);

                        // remove optional shortcuts
                        int k = 0;
                        while (buffer[k])
                        {
                            if (buffer[k] == '\t')
                            {
                                buffer[k] = 0;
                                break;
                            }
                            k++;
                        }
                        
                        if (!lstrcmp(buffer, L"Compare"))
                        {
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
	if (argc < 3)
		quitWithError(L"Missing commandline arguments:\ncompare.exe <file_path_1> <file_path_2>");
	if (!PathFileExists(argv[1]))
		quitWithError(L"Input file not found:\n'%s'", argv[1]);
	if (!PathFileExists(argv[2]))
		quitWithError(L"Input file not found:\n'%s'", argv[2]);

	// read .ini config
	_TCHAR iniPath[MAX_PATH];
	_TCHAR nppPath[MAX_PATH];
	_TCHAR buffer[2048];
	lstrcpy(iniPath, argv[0]);
	PathRemoveFileSpec(iniPath);
	PathAppend(iniPath, L"compare.ini");
	bool debug = (GetPrivateProfileInt(L"debug", L"output-debug-string", 0, iniPath) == 1);

	// get notepad++:
	// as configured in the .ini file
	// else as found via relative path
	// else as found via registry
	if (GetPrivateProfileString(L"notepad", L"path", NULL, buffer, sizeof(buffer), iniPath)) {
		ExpandEnvironmentStrings(buffer, nppPath, sizeof(nppPath));
		if (!PathFileExists(nppPath))
			quitWithError(L"Configured Notepad++ path not found:\n'%s'", nppPath);
	}
	else
	{
		lstrcpy(nppPath, argv[0]);
		PathRemoveFileSpec(nppPath);
		PathAppend(nppPath, L"..\\..\\notepad++.exe");
		if (!PathFileExists(nppPath))
		{
			HKEY hKey;
			DWORD lpcbData = sizeof(nppPath);
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Notepad++", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)nppPath, &lpcbData) == ERROR_SUCCESS)
				{
					PathAppend(nppPath, L"notepad++.exe");
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

	if (CreateProcess(NULL, buffer, NULL, NULL, FALSE, NULL, NULL, NULL, &suInfo, &procInfo)) {
		WaitForInputIdle(procInfo.hProcess, 7000);

		EnumThreadWindows(procInfo.dwThreadId, EnumThreadWndProc, NULL);

		CloseHandle(procInfo.hProcess);
		CloseHandle(procInfo.hThread);
	}

	return exitCode;
}
