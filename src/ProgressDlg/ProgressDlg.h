#pragma once


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include <memory>


class ProgressDlg
{
public:
	static void Open(const TCHAR* msg);
	static bool Update(int mid);
	static bool IsCancelled();
	static void Close();

    ~ProgressDlg();

private:
    static const TCHAR cClassName[];
    static const int cBackgroundColor;
    static const int cPBwidth;
    static const int cPBheight;
    static const int cBTNwidth;
    static const int cBTNheight;

	static std::unique_ptr<ProgressDlg> Inst;

    static DWORD WINAPI threadFunc(LPVOID data);
    static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

    ProgressDlg();

    // Disable copy construction and operator=
    ProgressDlg(const ProgressDlg&);
    const ProgressDlg& operator=(const ProgressDlg&);

    HWND create();
    void cancel();
    void destroy();

    inline bool cancelled() const
	{
		if (_hwnd)
			return (::WaitForSingleObject(_hActiveState, 0) != WAIT_OBJECT_0);
		return false;
	}

	inline void setInfo(const TCHAR *info) const
	{
		if (_hwnd)
			::SendMessage(_hPText, WM_SETTEXT, 0, (LPARAM)info);
	}

    inline void setPercent(unsigned percent) const
	{
		if (_hwnd)
			::PostMessage(_hPBar, PBM_SETPOS, (WPARAM)percent, 0);
	}

    BOOL thread();
    BOOL createProgressWindow();
    RECT adjustSizeAndPos(int width, int height);

    HINSTANCE		_hInst;
	volatile HWND	_hwnd;
    HANDLE			_hThread;
    HANDLE			_hActiveState;
	HWND			_hPText;
    HWND			_hPBar;
    HWND			_hBtn;
    HHOOK			_hKeyHook;

	int				_max;
	int				_count;
};
