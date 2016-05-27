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

#include "LibHelpers.h"
#include <stdlib.h>
#include <shlwapi.h>
#include "Compare.h"


static void TCharToChar(const wchar_t* src, char* dest, int size)
{
	::WideCharToMultiByte(CP_ACP, 0, src, -1, dest, size, NULL, NULL);
}


// Search recursively upwards for the dirName folder
bool LocateDirUp(const TCHAR* dirName, const TCHAR* currentDir, TCHAR* fullDirPath, unsigned fullDirPathSize)
{
	TCHAR currPath[MAX_PATH];
	TCHAR testPath[MAX_PATH];

	_tcscpy_s(currPath, _countof(currPath), currentDir);

	while (!::PathIsRoot(currPath))
	{
		::PathCombine(testPath, currPath, dirName);
		if (::PathIsDirectory(testPath))
		{
			// found
			_tcscpy_s(fullDirPath, fullDirPathSize, testPath);
			return true;
		}

		// up one folder
		::PathCombine(testPath, currPath, TEXT(".."));
		_tcscpy_s(currPath, _countof(currPath), testPath);
	}

	return false;
}


void CreateRelativeFilePath(const TCHAR* currDir, const TCHAR* baseDir, const TCHAR* file, TCHAR* filePath)
{
	TCHAR tmp[MAX_PATH];

	filePath[0] = 0;

	// baseDir is supposed to be the revision control system data folder (.svn or .git for example)
	// so drop it (up one folder)
	::PathCombine(filePath, baseDir, TEXT(".."));
	::PathRelativePathTo(tmp, filePath, FILE_ATTRIBUTE_DIRECTORY, currDir, FILE_ATTRIBUTE_DIRECTORY);
	::PathCombine(filePath, tmp, file);

	for (int i = _tcslen(filePath) - 1; i >= 0; --i)
	{
		if (filePath[i] == TEXT('\\'))
			filePath[i] = TEXT('/');
	}
}


bool GetSvnFile(const TCHAR* currDir, const TCHAR* svnDir, const TCHAR* file, TCHAR* svnFile, unsigned svnFileSize)
{
	bool ret = false;
	TCHAR svnBase[MAX_PATH];

	::PathCombine(svnBase, svnDir, TEXT("wc.db"));

	// is it SVN 1.7 or above?
	if (::PathFileExists(svnBase))
	{
		if (!InitSQLite())
		{
			::MessageBox(nppData._nppHandle, TEXT("Failed to initialize SQLite"), TEXT("Compare Plugin"), MB_OK);
			return false;
		}

		sqlite3* ppDb;

		if (sqlite3_open16(svnBase, &ppDb) != SQLITE_OK)
			return false;

		TCHAR svnFilePath[MAX_PATH];
		CreateRelativeFilePath(currDir, svnDir, file, svnFilePath);

		TCHAR sqlQuery[MAX_PATH + 64];
		_sntprintf_s(sqlQuery, _countof(sqlQuery), _TRUNCATE,
				TEXT("SELECT checksum FROM nodes_current WHERE local_relpath='%s';"), svnFilePath);

		sqlite3_stmt* pStmt;

		if (sqlite3_prepare16_v2(ppDb, sqlQuery, -1, &pStmt, NULL) == SQLITE_OK)
		{
			if (sqlite3_step(pStmt) == SQLITE_ROW)
			{
				const TCHAR* checksum = (const TCHAR*)sqlite3_column_text16(pStmt, 0);

				if (checksum[0] != 0)
				{
					TCHAR tmp[MAX_PATH];
					TCHAR idx[128];

					_tcsncpy_s(idx, _countof(idx), checksum + 6, 2);

					::PathCombine(svnBase, svnDir, TEXT("pristine"));
					::PathCombine(tmp, svnBase, idx);

					_tcscpy_s(idx, _countof(idx), checksum + 6);

					::PathCombine(svnBase, tmp, idx);
					_tcscat_s(svnBase, _countof(svnBase), TEXT(".svn-base"));

					if (PathFileExists(svnBase))
					{
						_tcscpy_s(svnFile, svnFileSize, svnBase);
						ret = true;
					}
				}
			}

			sqlite3_finalize(pStmt);
		}

		sqlite3_close(ppDb);
	}
	else
	{
		TCHAR tmp[MAX_PATH];

		::PathCombine(tmp, svnDir, TEXT("text-base"));
		::PathCombine(svnBase, tmp, file);
		_tcscat_s(svnBase, _countof(svnBase), TEXT(".svn-base"));

		// Is it an old SVN version?
		if (::PathFileExists(svnBase))
		{
			_tcscpy_s(svnFile, svnFileSize, svnBase);
			ret = true;
		}
	}

	return ret;
}


HGLOBAL GetContentFromGitRepo(const TCHAR* gitDir, const TCHAR* gitFilePath)
{
	HGLOBAL hMem = NULL;

	char ansiGitDir[MAX_PATH];

	TCharToChar(gitDir, ansiGitDir, sizeof(ansiGitDir));

	if (!InitLibGit2())
	{
		::MessageBox(nppData._nppHandle, TEXT("Failed to initialize LibGit2"), TEXT("Compare Plugin"), MB_OK);
		return NULL;
	}

	git_repository* repo;

	if (!git_repository_open(&repo, ansiGitDir))
	{
		git_index* index;

		if (!git_repository_index(&index, repo))
		{
			size_t at_pos;
			char ansiGitFilePath[MAX_PATH];

			TCharToChar(gitFilePath, ansiGitFilePath, sizeof(ansiGitFilePath));

			if (git_index_find(&at_pos, index, ansiGitFilePath) != GIT_ENOTFOUND)
			{
				const git_index_entry* e = git_index_get_byindex(index, at_pos);

				if (e)
				{
					git_blob* blob;

					if (!git_blob_lookup(&blob, repo, &e->oid))
					{
						long sizeBlob = (long)git_blob_rawsize(blob);

						if (sizeBlob)
						{
							const void* content = git_blob_rawcontent(blob);

							if (content)
							{
								hMem = ::GlobalAlloc(GMEM_FIXED, (SIZE_T)sizeBlob + 1);

								if (hMem)
								{
									::CopyMemory(hMem, content, (SIZE_T)sizeBlob);
									*((char*)hMem + sizeBlob) = 0;
								}
							}
						}

						git_blob_free(blob);
					}
				}
			}

			git_index_free(index);
		}

		git_repository_free(repo);
	}

	return hMem;
}
