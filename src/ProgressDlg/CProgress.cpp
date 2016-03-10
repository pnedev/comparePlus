#pragma comment (lib, "comctl32")


#include "CProgress.h"
#include <windowsx.h>
#include <stdlib.h>


const TCHAR CProgress::cClassName[]     = TEXT("OperationProgressClass");
const TCHAR CProgress::cDefaultHeader[] = TEXT("Operation progress...");
const int CProgress::cBackgroundColor   = COLOR_3DFACE;
const int CProgress::cPBwidth           = 600;
const int CProgress::cPBheight          = 10;
const int CProgress::cBTNwidth          = 80;
const int CProgress::cBTNheight         = 25;


volatile LONG CProgress::RefCount = 0;
HINSTANCE CProgress::HInst = NULL;


CProgress::CProgress() : _hwnd(NULL), _hCallerWnd(NULL)
{
    if (::InterlockedIncrement(&RefCount) == 1)
    {
        ::GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_PIN, cClassName, &HInst);

        WNDCLASSEX wcex;

        ::SecureZeroMemory(&wcex, sizeof(wcex));
        wcex.cbSize           = sizeof(wcex);
        wcex.style            = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc      = wndProc;
        wcex.hInstance        = HInst;
        wcex.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground    = ::GetSysColorBrush(cBackgroundColor);
        wcex.lpszClassName    = cClassName;

        ::RegisterClassEx(&wcex);

        INITCOMMONCONTROLSEX icex;

        ::SecureZeroMemory(&icex, sizeof(icex));
        icex.dwSize = sizeof(icex);
        icex.dwICC  = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;

        ::InitCommonControlsEx(&icex);
    }
}


CProgress::~CProgress()
{
    Close();

    if (::InterlockedDecrement(&RefCount) == 0)
        ::UnregisterClass(cClassName, HInst);
}


HWND CProgress::Open(HWND hCallerWnd, const TCHAR* header)
{
    if (_hwnd)
        return _hwnd;

    // Create manually reset non-signaled event
    _hActiveState = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!_hActiveState)
        return NULL;

    _hCallerWnd = hCallerWnd;

	for (HWND hwnd = _hCallerWnd; hwnd; hwnd = ::GetParent(hwnd))
		::UpdateWindow(hwnd);

    if (header)
        _tcscpy_s(_header, _countof(_header), header);
    else
        _tcscpy_s(_header, _countof(_header), cDefaultHeader);

    _hThread = ::CreateThread(NULL, 0, threadFunc, this, 0, NULL);
    if (!_hThread)
    {
        ::CloseHandle(_hActiveState);
        return NULL;
    }

    // Wait for the progress window to be created
    ::WaitForSingleObject(_hActiveState, INFINITE);

    // On progress window create fail
    if (!_hwnd)
    {
        ::WaitForSingleObject(_hThread, INFINITE);
        ::CloseHandle(_hThread);
		::CloseHandle(_hActiveState);
    }

    return _hwnd;
}


void CProgress::Close()
{
    if (_hwnd)
    {
		::PostMessage(_hwnd, WM_CLOSE, 0, 0);
		_hwnd = NULL;

        ::WaitForSingleObject(_hThread, INFINITE);
        ::CloseHandle(_hThread);
        ::CloseHandle(_hActiveState);
    }
}


DWORD WINAPI CProgress::threadFunc(LPVOID data)
{
    CProgress* pw = static_cast<CProgress*>(data);
    return (DWORD)pw->thread();
}


BOOL CProgress::thread()
{
    BOOL r = createProgressWindow();
    ::SetEvent(_hActiveState);
    if (!r)
        return r;

    // Window message loop
    MSG msg;
    while ((r = ::GetMessage(&msg, NULL, 0, 0)) != 0 && r != -1)
        ::DispatchMessage(&msg);

	return r;
}


BOOL CProgress::createProgressWindow()
{
	_hwnd = ::CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_OVERLAPPEDWINDOW,
            cClassName, _header, WS_POPUP | WS_CAPTION,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, HInst, (LPVOID)this);
    if (!_hwnd)
        return FALSE;

    int width = cPBwidth + 10;
    int height = cPBheight + cBTNheight + 35;
    RECT win = adjustSizeAndPos(width, height);
    ::MoveWindow(_hwnd, win.left, win.top, win.right - win.left, win.bottom - win.top, TRUE);

    ::GetClientRect(_hwnd, &win);
    width = win.right - win.left;
    height = win.bottom - win.top;

	_hPText = ::CreateWindowEx(0, TEXT("STATIC"), TEXT(""),
			WS_CHILD | WS_VISIBLE | BS_TEXT | SS_PATHELLIPSIS,
			5, 5, width - 10, 20, _hwnd, NULL, HInst, NULL);

    _hPBar = ::CreateWindowEx(0, PROGRESS_CLASS, TEXT("Progress Bar"),
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            5, 25, width - 10, cPBheight,
            _hwnd, NULL, HInst, NULL);
    SendMessage(_hPBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    _hBtn = ::CreateWindowEx(0, TEXT("BUTTON"), TEXT("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_TEXT,
            (width - cBTNwidth) / 2, height - cBTNheight - 5,
            cBTNwidth, cBTNheight, _hwnd, NULL, HInst, NULL);

	HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	if (hf)
	{
		::SendMessage(_hPText, WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
		::SendMessage(_hBtn, WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
	}

    ::ShowWindow(_hwnd, SW_SHOWNORMAL);
    ::UpdateWindow(_hwnd);

    return TRUE;
}


RECT CProgress::adjustSizeAndPos(int width, int height)
{
	RECT maxWin;
	maxWin.left		= ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	maxWin.top		= ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	maxWin.right	= ::GetSystemMetrics(SM_CXVIRTUALSCREEN) + maxWin.left;
	maxWin.bottom	= ::GetSystemMetrics(SM_CYVIRTUALSCREEN) + maxWin.top;

	POINT center;

	if (_hCallerWnd)
	{
		RECT biasWin;
		::GetWindowRect(_hCallerWnd, &biasWin);
		center.x = (biasWin.left + biasWin.right) / 2;
		center.y = (biasWin.top + biasWin.bottom) / 2;
	}
	else
	{
		center.x = (maxWin.left + maxWin.right) / 2;
		center.y = (maxWin.top + maxWin.bottom) / 2;
	}

	RECT win = maxWin;
	win.right = win.left + width;
	win.bottom = win.top + height;

	::AdjustWindowRectEx(&win, ::GetWindowLongPtr(_hwnd, GWL_STYLE), FALSE, ::GetWindowLongPtr(_hwnd, GWL_EXSTYLE));

	width = win.right - win.left;
	height = win.bottom - win.top;

	if (width < maxWin.right - maxWin.left)
	{
		win.left = center.x - width / 2;
		if (win.left < maxWin.left)
			win.left = maxWin.left;
		win.right = win.left + width;
		if (win.right > maxWin.right)
		{
			win.right = maxWin.right;
			win.left = win.right - width;
		}
	}
	else
	{
		win.left = maxWin.left;
		win.right = maxWin.right;
	}

	if (height < maxWin.bottom - maxWin.top)
	{
		win.top = center.y - height / 2;
		if (win.top < maxWin.top)
			win.top = maxWin.top;
		win.bottom = win.top + height;
		if (win.bottom > maxWin.bottom)
		{
			win.bottom = maxWin.bottom;
			win.top = win.bottom - height;
		}
	}
	else
	{
		win.top = maxWin.top;
		win.bottom = maxWin.bottom;
	}

	return win;
}


LRESULT APIENTRY CProgress::wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
        case WM_CREATE:
        {
			CProgress* pw =(CProgress*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(pw));
            return 0;
        }

        case WM_SETFOCUS:
        {
			CProgress* pw =	reinterpret_cast<CProgress*>(static_cast<LONG_PTR>
					(::GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            ::SetFocus(pw->_hBtn);
            return 0;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) == BN_CLICKED)
            {
                CProgress* pw = reinterpret_cast<CProgress*>(static_cast<LONG_PTR>
						(::GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                ::ResetEvent(pw->_hActiveState);
                ::EnableWindow(pw->_hBtn, FALSE);
				pw->SetInfo(TEXT("Cancelling operation, please wait..."));
                return 0;
            }
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }

    return ::DefWindowProc(hwnd, umsg, wparam, lparam);
}
