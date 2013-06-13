#include "LibGit2Helper.h"

bool bLibGit2Initialized;
PGITREPOSITORYOPEN git_repository_open;
PGITREPOSITORYINDEX git_repository_index;
PGITINDEXFIND git_index_find;
PGITINDEXGETBYINDEX git_index_get_byindex;
PGITBLOBLOOKUP git_blob_lookup;
PGITBLOBRAWSIZE git_blob_rawsize;
PGITBLOBRAWCONTENT git_blob_rawcontent;
PGITBLOBFREE git_blob_free;
PGITINDEXFREE git_index_free;
PGITREPOSITORYFREE git_repository_free;

bool InitLibGit2()
{
	if (!bLibGit2Initialized)
	{
		TCHAR buffer[MAX_PATH];

		// get git2.dll path from plugin subfolder
		HMODULE mPlugin = GetModuleHandle(L"ComparePlugin.dll");
		if (!mPlugin) return false;
		int len = GetModuleFileName(mPlugin, (LPWSTR)buffer, MAX_PATH);
		buffer[len - 4] = 0;
		lstrcat(buffer, L"\\git2.dll");

		HMODULE mLibGit2 = LoadLibrary(buffer);
		if (!mLibGit2) return false;

		git_repository_open = (PGITREPOSITORYOPEN)GetProcAddress(mLibGit2, "git_repository_open");
		if (!git_repository_open) return false;
		git_repository_index = (PGITREPOSITORYINDEX)GetProcAddress(mLibGit2, "git_repository_index");
		if (!git_repository_index) return false;
		git_index_find = (PGITINDEXFIND)GetProcAddress(mLibGit2, "git_index_find");
		if (!git_index_find) return false;
		git_index_get_byindex = (PGITINDEXGETBYINDEX)GetProcAddress(mLibGit2, "git_index_get_byindex");
		if (!git_index_get_byindex) return false;
		git_blob_lookup = (PGITBLOBLOOKUP)GetProcAddress(mLibGit2, "git_blob_lookup");
		if (!git_blob_lookup) return false;
		git_blob_rawsize = (PGITBLOBRAWSIZE)GetProcAddress(mLibGit2, "git_blob_rawsize");
		if (!git_blob_rawsize) return false;
		git_blob_rawcontent = (PGITBLOBRAWCONTENT)GetProcAddress(mLibGit2, "git_blob_rawcontent");
		if (!git_blob_rawcontent) return false;
		git_blob_free = (PGITBLOBFREE)GetProcAddress(mLibGit2, "git_blob_free");
		if (!git_blob_free) return false;
		git_index_free = (PGITINDEXFREE)GetProcAddress(mLibGit2, "git_index_free");
		if (!git_index_free) return false;
		git_repository_free = (PGITREPOSITORYFREE)GetProcAddress(mLibGit2, "git_repository_free");
		if (!git_repository_free) return false;

		bLibGit2Initialized = true;
	}

	return true;
}
