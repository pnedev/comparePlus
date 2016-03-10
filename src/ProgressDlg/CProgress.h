#pragma once


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>


class CProgress
{
public:
    CProgress();
    ~CProgress();

    HWND Open(HWND hCallerWnd = NULL, const TCHAR* header = NULL);

    bool IsCancelled() const
	{
		if (_hwnd)
			return (::WaitForSingleObject(_hActiveState, 0) != WAIT_OBJECT_0);
		return false;
	}

	void SetInfo(const TCHAR *info) const
	{
		if (_hwnd)
			::SendMessage(_hPText, WM_SETTEXT, 0, (LPARAM)info);
	}

    void SetPercent(unsigned percent) const
	{
		if (_hwnd)
			::PostMessage(_hPBar, PBM_SETPOS, (WPARAM)percent, 0);
	}

    void Close();

private:
    static const TCHAR cClassName[];
    static const TCHAR cDefaultHeader[];
    static const int cBackgroundColor;
    static const int cPBwidth;
    static const int cPBheight;
    static const int cBTNwidth;
    static const int cBTNheight;

    static volatile LONG RefCount;
    static HINSTANCE HInst;

    static DWORD WINAPI threadFunc(LPVOID data);
    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    // Disable copy construction and operator=
    CProgress(const CProgress&);
    const CProgress& operator=(const CProgress&);

    BOOL thread();
    BOOL createProgressWindow();
    RECT adjustSizeAndPos(int width, int height);

	volatile HWND _hwnd;
    HWND _hCallerWnd;
    TCHAR _header[128];
    HANDLE _hThread;
    HANDLE _hActiveState;
	HWND _hPText;
    HWND _hPBar;
    HWND _hBtn;
};
