#pragma once


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>


class CProgress
{
public:
    CProgress(HINSTANCE hInst, HWND hParent = NULL,
            const TCHAR* header = NULL);
    ~CProgress();

    bool Open();
    bool IsCancelled() const;
    void SetPercent(unsigned percent) const;
    void Close();

private:
    static const TCHAR cClassName[];
    static const int cBackgroundColor;
    static const int cPBwidth;
    static const int cPBheight;
    static const int cBTNwidth;
    static const int cBTNheight;

    static volatile LONG RefCount;

    static DWORD threadFunc(LPVOID data);
    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    // Disable copy construction and operator=
    CProgress(const CProgress&);
    const CProgress& operator=(const CProgress&);

    int thread();
    bool createProgressWindow();
    RECT adjustSizeAndPos(int width, int height);

    TCHAR _header[128];

    HINSTANCE _hInst;
    HWND _hParent;
    volatile bool _isInit;
    HANDLE _hThread;
    HANDLE _hActiveState;
    HWND _hwnd;
    HWND _hPBar;
    HWND _hBtn;
};
