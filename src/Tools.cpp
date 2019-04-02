/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C) 2016 Pavel Nedev (pg.nedev@gmail.com)
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
