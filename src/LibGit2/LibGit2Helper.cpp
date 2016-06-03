/*
This file is part of Compare plugin for Notepad++
Copyright (C)2013 Jean-Sébastien Leroy (jean.sebastien.leroy@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include "LibGit2Helper.h"

PGITLIBVERSION			git_libgit2_version;
PGITREPOSITORYOPENEXT	git_repository_open_ext;
PGITREPOSITORYWORKDIR	git_repository_workdir;
PGITREPOSITORYINDEX		git_repository_index;
PGITINDEXGETBYPATH		git_index_get_bypath;
PGITBLOBLOOKUP			git_blob_lookup;
PGITBLOBRAWSIZE			git_blob_rawsize;
PGITBLOBRAWCONTENT		git_blob_rawcontent;
PGITBLOBFREE			git_blob_free;
PGITINDEXFREE			git_index_free;
PGITREPOSITORYFREE		git_repository_free;

bool InitLibGit2()
{
	static bool isInit = false;

	if (!isInit)
	{
		TCHAR dllPath[MAX_PATH];

		HMODULE hPlugin = ::GetModuleHandle(TEXT("ComparePlugin.dll"));
		if (!hPlugin)
			return false;

		::GetModuleFileName(hPlugin, (LPWSTR)dllPath, _countof(dllPath));
		::PathRemoveExtension(dllPath);
		_tcscat_s(dllPath, _countof(dllPath), TEXT("\\git2.dll"));

		HMODULE libGit2 = ::LoadLibrary(dllPath);
		if (!libGit2)
			return false;

		git_libgit2_version = (PGITLIBVERSION)::GetProcAddress(libGit2, "git_libgit2_version");
		if (!git_libgit2_version)
			return false;
		git_repository_open_ext = (PGITREPOSITORYOPENEXT)::GetProcAddress(libGit2, "git_repository_open_ext");
		if (!git_repository_open_ext)
			return false;
		git_repository_workdir = (PGITREPOSITORYWORKDIR)::GetProcAddress(libGit2, "git_repository_workdir");
		if (!git_repository_workdir)
			return false;
		git_repository_index = (PGITREPOSITORYINDEX)::GetProcAddress(libGit2, "git_repository_index");
		if (!git_repository_index)
			return false;
		git_index_get_bypath = (PGITINDEXGETBYPATH)::GetProcAddress(libGit2, "git_index_get_bypath");
		if (!git_index_get_bypath)
			return false;
		git_blob_lookup = (PGITBLOBLOOKUP)::GetProcAddress(libGit2, "git_blob_lookup");
		if (!git_blob_lookup)
			return false;
		git_blob_rawsize = (PGITBLOBRAWSIZE)::GetProcAddress(libGit2, "git_blob_rawsize");
		if (!git_blob_rawsize)
			return false;
		git_blob_rawcontent = (PGITBLOBRAWCONTENT)::GetProcAddress(libGit2, "git_blob_rawcontent");
		if (!git_blob_rawcontent)
			return false;
		git_blob_free = (PGITBLOBFREE)::GetProcAddress(libGit2, "git_blob_free");
		if (!git_blob_free)
			return false;
		git_index_free = (PGITINDEXFREE)::GetProcAddress(libGit2, "git_index_free");
		if (!git_index_free)
			return false;
		git_repository_free = (PGITREPOSITORYFREE)::GetProcAddress(libGit2, "git_repository_free");
		if (!git_repository_free)
			return false;

		isInit = true;
	}

	return true;
}
