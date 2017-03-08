/*
 * This file is part of Compare Plugin for Notepad++
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include <memory>


class ProgressDlg;
using progress_ptr = std::unique_ptr<ProgressDlg>;


class ProgressDlg
{
public:
	static progress_ptr& Open(const TCHAR* info = NULL);

	static progress_ptr& Get()
	{
		return Inst;
	}

	static void Close()
	{
		Inst.reset();
	}

    ~ProgressDlg();

	inline void SetInfo(const TCHAR *info) const
	{
		if (_hwnd)
			::SendMessage(_hPText, WM_SETTEXT, 0, (LPARAM)info);
	}

	inline bool IsCancelled() const
	{
		if (_hwnd)
			return (::WaitForSingleObject(_hActiveState, 0) != WAIT_OBJECT_0);
		return false;
	}

	unsigned NextPhase();
	bool SetMaxCount(unsigned max, unsigned phase = 0);
	bool SetCount(unsigned cnt, unsigned phase = 0);
	bool Advance(unsigned cnt = 1, unsigned phase = 0);

private:
    static const TCHAR cClassName[];
    static const int cBackgroundColor;
    static const int cPBwidth;
    static const int cPBheight;
    static const int cBTNwidth;
    static const int cBTNheight;

	static const int cPhases[];

	static progress_ptr Inst;

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
