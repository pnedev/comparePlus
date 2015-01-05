#include <tchar.h>
#include <Shlwapi.h>

void showError(_TCHAR* errMsg, ...)
{
	_TCHAR buffer[512];
	va_list argptr;
	va_start(argptr, errMsg);
	#pragma warning(disable: 4996)
	vswprintf(buffer, errMsg, argptr);
	#pragma warning(default: 4996)
	MessageBox(NULL, buffer, L"compare.exe", MB_OK | MB_ICONERROR);
}

#define quitWithError(errMsg, ...) \
{ \
	showError(errMsg, __VA_ARGS__); \
	return 1; \
}

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
	if (false)
	{
		return FALSE;
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
		PathAppend(nppPath, L"..\\..\\notepad.exe");
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
