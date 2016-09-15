#pragma once


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include <memory>


class ProgressDlg
{
public:
	static void Open(const TCHAR* msg);
	static bool IsCancelled();
	static void Close();

	static unsigned NextPhase();
	static bool SetMaxCount(unsigned max, unsigned phase = 0);
	static bool SetCount(unsigned cnt, unsigned phase = 0);
	static bool Advance(unsigned cnt = 1, unsigned phase = 0);

    ~ProgressDlg();

private:
    static const TCHAR cClassName[];
    static const int cBackgroundColor;
    static const int cPBwidth;
    static const int cPBheight;
    static const int cBTNwidth;
    static const int cBTNheight;

	static const int cPhases[];

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

    inline void setPos(unsigned pos) const
	{
		if (_hwnd)
			::PostMessage(_hPBar, PBM_SETPOS, (WPARAM)pos, 0);
	}

    void update();

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

	unsigned	_phase;
	unsigned	_phaseRange;
	unsigned	_phasePosOffset;
	unsigned	_max;
	unsigned	_count;

	unsigned	_pos;
};
