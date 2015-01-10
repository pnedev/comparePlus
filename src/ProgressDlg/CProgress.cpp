#pragma comment (lib, "comctl32")


#define WIN32_LEAN_AND_MEAN
#include "CProgress.h"
#include <windowsx.h>
#include <commctrl.h>


const TCHAR CProgress::cClassName[]     = TEXT("CompareProgressClass");
const int CProgress::cBackgroundColor   = COLOR_3DFACE;
const int CProgress::cPBwidth           = 500;
const int CProgress::cPBheight          = 15;
const int CProgress::cBTNwidth          = 80;
const int CProgress::cBTNheight         = 25;


volatile LONG CProgress::RefCount = 0;


CProgress::CProgress(HINSTANCE hInst, HWND hParent, const TCHAR* header) :
    _hInst(hInst), _hParent(hParent), _isInit(false)
{
    if (::InterlockedIncrement(&RefCount) == 1)
    {
        WNDCLASS wc;

        ::SecureZeroMemory(&wc, sizeof(wc));
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = wndProc;
        wc.hInstance        = hInst;
        wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = ::GetSysColorBrush(cBackgroundColor);
        wc.lpszClassName    = cClassName;

        ::RegisterClass(&wc);

        INITCOMMONCONTROLSEX icex;

        ::SecureZeroMemory(&icex, sizeof(icex));
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC  = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;

        ::InitCommonControlsEx(&icex);
    }
    if (header)
		_tcscpy_s(_header, _tcslen(_header), header);
    else
		_tcscpy_s(_header, _tcslen(_header), TEXT("Compare in progress..."));
}


CProgress::~CProgress()
{
    Close();

    if (::InterlockedDecrement(&RefCount) == 0)
        ::UnregisterClass(cClassName, _hInst);
}


bool CProgress::Open()
{
    if (_isInit)
        return true;

    // Create manually reset non-signaled event
    _hActiveState = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!_hActiveState)
        return false;

    _hThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadFunc,
            (LPVOID)this, 0, NULL);
    if (!_hThread)
    {
        ::CloseHandle(_hActiveState);
        return false;
    }

    // Wait for the progress window to be created
    ::WaitForSingleObject(_hActiveState, INFINITE);

    // On progress window create fail
    if (!_isInit)
    {
        ::WaitForSingleObject(_hThread, INFINITE);
        ::CloseHandle(_hThread);
    }

    return _isInit;
}


bool CProgress::IsCancelled() const
{
    if (_isInit)
        return (::WaitForSingleObject(_hActiveState, 0) != WAIT_OBJECT_0);
    return false;
}


void CProgress::SetPercent(unsigned percent) const
{
    if (_isInit)
        ::SendNotifyMessage(_hPBar, PBM_SETPOS, (WPARAM)percent, 0);
}


void CProgress::Close()
{
    if (_isInit)
    {
        _isInit = false;
        ::SendMessage(_hwnd, WM_CLOSE, 0, 0);
        ::WaitForSingleObject(_hThread, INFINITE);
        ::CloseHandle(_hThread);
        ::CloseHandle(_hActiveState);
    }
}


DWORD CProgress::threadFunc(LPVOID data)
{
    CProgress* pw = static_cast<CProgress*>(data);
    return (DWORD)pw->thread();
}


int CProgress::thread()
{
    _isInit = createProgressWindow();
    ::SetEvent(_hActiveState);
    if (!_isInit)
        return -1;

    // Window message loop
    MSG msg;
    BOOL r;
    while ((r = ::GetMessage(&msg, NULL, 0, 0)) != 0 && r != -1)
        ::DispatchMessage(&msg);

	return r;
}


bool CProgress::createProgressWindow()
{
    _hwnd = ::CreateWindowEx(WS_EX_TOOLWINDOW,
            cClassName, _header, WS_OVERLAPPED, 10, 10, 10, 10,
            _hParent, NULL, _hInst, (LPVOID)this);
    if (!_hwnd)
        return false;

    int width = cPBwidth + 10;
    int height = cPBheight + cBTNheight + 15;
    RECT win = adjustSizeAndPos(width, height);
    ::MoveWindow(_hwnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);

    ::GetClientRect(_hwnd, &win);
    width = win.right - win.left;
    height = win.bottom - win.top;

    _hPBar = ::CreateWindowEx(0, PROGRESS_CLASS, TEXT("Progress Bar"),
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            5, 5, width - 10, cPBheight,
            _hwnd, NULL, _hInst, NULL);
    SendMessage(_hPBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    _hBtn = ::CreateWindowEx(0, TEXT("BUTTON"), TEXT("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_TEXT,
            (width - cBTNwidth) / 2, height - cBTNheight - 5,
            cBTNwidth, cBTNheight, _hwnd, NULL, _hInst, NULL);

    ::ShowWindow(_hwnd, SW_SHOW);
    ::UpdateWindow(_hwnd);

    return true;
}


RECT CProgress::adjustSizeAndPos(int width, int height)
{
    RECT win, maxWin;

    ::GetWindowRect(::GetDesktopWindow(), &maxWin);
    win = maxWin;
    win.right = win.left + width;
    win.bottom = win.top + height;

    ::AdjustWindowRectEx(&win, ::GetWindowLong(_hwnd, GWL_STYLE),
            FALSE, ::GetWindowLong(_hwnd, GWL_EXSTYLE));

    width = win.right - win.left;
    height = win.bottom - win.top;

    if (width < maxWin.right - maxWin.left)
    {
        win.left = (maxWin.left + maxWin.right - width) / 2;
        win.right = win.left + width;
    }
    else
    {
        win.left = maxWin.left;
        win.right = maxWin.right;
    }

    if (height < maxWin.right - maxWin.left)
    {
        win.top = (maxWin.top + maxWin.bottom - height) / 2;
        win.bottom = win.top + height;
    }
    else
    {
        win.top = maxWin.top;
        win.bottom = maxWin.bottom;
    }

    return win;
}


LRESULT APIENTRY CProgress::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
        case WM_CREATE:
        {
            CProgress* pw =
                (CProgress*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(pw));
            return 0;
        }

        case WM_SETFOCUS:
        {
            CProgress* pw =
                reinterpret_cast<CProgress*>(static_cast<LONG_PTR>
                        (::GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            ::SetFocus(pw->_hBtn);
            return 0;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) == BN_CLICKED)
            {
                CProgress* pw =
                    reinterpret_cast<CProgress*>(static_cast<LONG_PTR>
                            (::GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                ::ResetEvent(pw->_hActiveState);
                ::EnableWindow(pw->_hBtn, FALSE);
                return 0;
            }
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }

    return ::DefWindowProc(hwnd, umsg, wparam, lparam);
}
