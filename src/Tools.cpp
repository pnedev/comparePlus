/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C) 2016-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "Tools.h"
#include <versionhelpers.h>


std::map<UINT_PTR, DelayedWork*> DelayedWork::workMap;


bool DelayedWork::post(UINT delay_ms)
{
	const bool isRunning = (_timerId != 0);

	_timerId = ::SetTimer(NULL, _timerId, delay_ms, timerCB);

	if (!isRunning && _timerId)
		workMap[_timerId] = this;

	return (_timerId != 0);
}


void DelayedWork::cancel()
{
	if (_timerId)
	{
		::KillTimer(NULL, _timerId);

		std::map<UINT_PTR, DelayedWork*>::iterator it = workMap.find(_timerId);
		if (it != workMap.end())
			workMap.erase(it);

		_timerId = 0;
	}
}


VOID CALLBACK DelayedWork::timerCB(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	std::map<UINT_PTR, DelayedWork*>::iterator it = workMap.find(idEvent);

	::KillTimer(NULL, idEvent);

	// Normally this shouldn't be the case
	if (it == workMap.end())
		return;

	DelayedWork* work = it->second;
	workMap.erase(it);

	// This is not a valid case
	if (!work)
		return;

	work->_timerId = 0;
	(*work)();
}


IFStreamLineGetter::IFStreamLineGetter(std::ifstream& ifs) : _ifs(ifs)
{
	if (ifs.good())
	{
		_readBuf.resize(cBuffSize);

		ifs.read(_readBuf.data(), _readBuf.size());

		_countRead = static_cast<size_t>(ifs.gcount());
	}
}


std::string IFStreamLineGetter::get()
{
	std::string lineStr;

	while (true)
	{
		const size_t pos = _readPos;

		if (lineStr.empty() || (lineStr.back() != '\r' && lineStr.back() != '\n'))
		{
			while (_readPos < _countRead &&
					*(_readBuf.data() + _readPos) != '\r' && *(_readBuf.data() + _readPos) != '\n')
				++_readPos;
		}

		while (_readPos < _countRead &&
				(*(_readBuf.data() + _readPos) == '\r' || *(_readBuf.data() + _readPos) == '\n'))
			++_readPos;

		lineStr.append(_readBuf.data() + pos, _readBuf.data() + _readPos);

		if (_readPos < _countRead || !_ifs.good())
			break;

		_ifs.read(_readBuf.data(), _readBuf.size());

		_countRead = static_cast<size_t>(_ifs.gcount());
		_readPos = 0;
	}

	return lineStr;
}


std::vector<wchar_t> getFromClipboard(bool addLeadingNewLine)
{
	std::vector<wchar_t> content;

	if (!::OpenClipboard(NULL))
		return content;

	HANDLE hData = ::GetClipboardData(CF_UNICODETEXT);

	if (hData != NULL)
	{
		wchar_t* pText = static_cast<wchar_t*>(::GlobalLock(hData));

		if (pText != NULL)
		{
			const size_t len = wcslen(pText) + 1;

			if (addLeadingNewLine)
			{
				content.resize(len + 1);
				content[0] = L'\n'; // Needed for selections alignment after comparing
				wcscpy_s(content.data() + 1, len, pText);
			}
			else
			{
				content.resize(len);
				wcscpy_s(content.data(), len, pText);
			}
		}

		::GlobalUnlock(hData);
	}

	::CloseClipboard();

	return content;
}


bool setToClipboard(const std::vector<wchar_t>& txt)
{
	if (txt.empty())
		return true;

	HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, txt.size() * sizeof(wchar_t));
	if (hglbCopy == nullptr)
		return false;

	// Lock the handle and copy the text to the buffer
	wchar_t* pStr = (wchar_t*)::GlobalLock(hglbCopy);
	if (!pStr)
	{
		::GlobalFree(hglbCopy);
		return false;
	}

	wcscpy_s(pStr, txt.size(), txt.data());
	::GlobalUnlock(hglbCopy);

	if (!::OpenClipboard(NULL))
	{
		::GlobalFree(hglbCopy);
		return false;
	}

	if (!::EmptyClipboard())
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}

	// Place the handle on the clipboard
	if (!::SetClipboardData(CF_UNICODETEXT, hglbCopy))
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}

	::CloseClipboard();

	return true;
}


void toLowerCase(std::vector<char>& text, int codepage)
{
	const int len = static_cast<int>(text.size());

	if (len == 0)
		return;

	const int wLen = ::MultiByteToWideChar(codepage, 0, text.data(), len, NULL, 0);

	std::vector<wchar_t> wText(wLen);

	::MultiByteToWideChar(codepage, 0, text.data(), len, wText.data(), wLen);

	wText.push_back(L'\0');
	::CharLowerW((LPWSTR)wText.data());
	wText.pop_back();

	::WideCharToMultiByte(codepage, 0, wText.data(), wLen, text.data(), len, NULL, NULL);
}


std::wstring MBtoWC(const char* mb, int len, int codepage)
{
	if (!mb || !len)
		return {};

	const int wLen = ::MultiByteToWideChar(codepage, 0, mb, len, NULL, 0);

	std::wstring str;
	str.resize(len > 0 ? wLen : wLen - 1);

	::MultiByteToWideChar(codepage, 0, mb, len, &str[0], wLen);

	return str;
}


std::string WCtoMB(const wchar_t* wc, int len, int codepage)
{
	if (!wc || !len)
		return {};

	const int l = ::WideCharToMultiByte(codepage, 0, wc, len, NULL, 0, NULL, NULL);

	std::string str;
	str.resize(len > 0 ? l : l - 1);

	::WideCharToMultiByte(codepage, 0, wc, len, &str[0], l, NULL, NULL);

	return str;
}


HFONT createFontFromSystemDefault(SysFont font, int size, bool underlined)
{
	HFONT hf {nullptr};

	NONCLIENTMETRICS ncm {0};
	ncm.cbSize = sizeof(NONCLIENTMETRICS);

#if (WINVER >= 0x0600)
	if (!IsWindows7OrGreater())
		ncm.cbSize -= sizeof(int);
#endif

	if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
	{
		LOGFONTW lf;

		switch (font)
		{
			case SysFont::Caption:
				lf = ncm.lfCaptionFont;
			break;

			case SysFont::SmallCaption:
				lf = ncm.lfSmCaptionFont;
			break;

			case SysFont::Menu:
				lf = ncm.lfMenuFont;
			break;

			case SysFont::Status:
				lf = ncm.lfStatusFont;
			break;

			case SysFont::Message:
				lf = ncm.lfMessageFont;
			break;

			default:
			return hf;
		}

		if (size)
		{
			HDC hdc = ::GetDC(nullptr);

			if (hdc)
			{
				lf.lfHeight = -::MulDiv(size, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);

				::ReleaseDC(nullptr, hdc);
			}
		}

		lf.lfUnderline = underlined ? TRUE : FALSE;

		hf = ::CreateFontIndirectW(&lf);
	}

	return hf;
}
