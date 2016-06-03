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
#include <cstring>
#include "Compare.h"
#include "SQLite/SqliteHelper.h"
#include "LibGit2/LibGit2Helper.h"


namespace // anonymous namespace
{

void CharToTChar(const char* src, wchar_t* dest, int destCharsCount)
{
	::MultiByteToWideChar(CP_ACP, 0, src, -1, dest, destCharsCount);
}


void TCharToChar(const wchar_t* src, char* dest, int destCharsCount)
{
	::WideCharToMultiByte(CP_ACP, 0, src, -1, dest, destCharsCount, NULL, NULL);
}


void RepoSubPath(const TCHAR* fullFilePath, const TCHAR* baseDir, TCHAR* filePath, unsigned filePathSize)
{
	TCHAR basePath[MAX_PATH];
	TCHAR fullPath[MAX_PATH];

	filePath[0] = 0;

	_tcscpy_s(fullPath, _countof(fullPath), baseDir);
	for (int i = _tcslen(fullPath) - 1; i >= 0; --i)
	{
		if (fullPath[i] == TEXT('/'))
			fullPath[i] = TEXT('\\');
	}

	// basePath is supposed to be the revision control system data folder (.svn or .git for example)
	// so drop it (up one folder)
	::PathCombine(basePath, fullPath, TEXT(".."));

	_tcscpy_s(fullPath, _countof(fullPath), fullFilePath);
	for (int i = _tcslen(fullPath) - 1; i >= 0; --i)
	{
		if (fullPath[i] == TEXT('/'))
			fullPath[i] = TEXT('\\');
	}

	int relativePathPos = _tcslen(basePath);

	if (!_tcsncmp(fullPath, basePath, relativePathPos))
	{
		for (int i = _tcslen(fullPath) - 1; i >= relativePathPos; --i)
		{
			if (fullPath[i] == TEXT('\\'))
				fullPath[i] = TEXT('/');
		}

		if (fullPath[relativePathPos] == TEXT('/'))
			++relativePathPos;

		_tcscpy_s(filePath, filePathSize, &fullPath[relativePathPos]);
	}
}


// Search recursively upwards for the dirName folder
bool LocateDirUp(const TCHAR* dirName, const TCHAR* currentDir, TCHAR* fullDirPath, unsigned fullDirPathSize)
{
	TCHAR testPath[MAX_PATH];

	_tcscpy_s(fullDirPath, fullDirPathSize, currentDir);

	while (!::PathIsRoot(fullDirPath))
	{
		::PathCombine(testPath, fullDirPath, dirName);
		if (::PathIsDirectory(testPath))
		{
			// found
			_tcscpy_s(fullDirPath, fullDirPathSize, testPath);
			return true;
		}

		// up one folder
		::PathCombine(testPath, fullDirPath, TEXT(".."));
		_tcscpy_s(fullDirPath, fullDirPathSize, testPath);
	}

	return false;
}

} // anonymous namespace


bool GetSvnFile(const TCHAR* fullFilePath, TCHAR* svnFile, unsigned svnFileSize)
{
	TCHAR svnDir[MAX_PATH];
	TCHAR svnBase[MAX_PATH];

	_tcscpy_s(svnBase, _countof(svnBase), fullFilePath);
	::PathRemoveFileSpec(svnBase);

	bool ret = LocateDirUp(TEXT(".svn"), svnBase, svnDir, _countof(svnDir));

	if (ret)
	{
		ret = false;

		::PathCombine(svnBase, svnDir, TEXT("wc.db"));

		// is it SVN 1.7 or above?
		if (::PathFileExists(svnBase))
		{
			if (!InitSQLite())
			{
				::MessageBox(nppData._nppHandle, TEXT("Failed to initialize SQLite - operation aborted."),
						TEXT("Compare Plugin"), MB_OK);
				return false;
			}

			sqlite3* ppDb;

			if (sqlite3_open16(svnBase, &ppDb) == SQLITE_OK)
			{
				TCHAR svnFilePath[MAX_PATH];
				RepoSubPath(fullFilePath, svnDir, svnFilePath, _countof(svnFilePath));

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
		}
		else
		{
			TCHAR tmp[MAX_PATH];

			::PathCombine(tmp, svnDir, TEXT("text-base"));

			const TCHAR* file = ::PathFindFileName(fullFilePath);

			::PathCombine(svnBase, tmp, file);
			_tcscat_s(svnBase, _countof(svnBase), TEXT(".svn-base"));

			// Is it an old SVN version?
			if (::PathFileExists(svnBase))
			{
				_tcscpy_s(svnFile, svnFileSize, svnBase);
				ret = true;
			}
		}
	}

	if (!ret)
		::MessageBox(nppData._nppHandle, TEXT("No SVN data found."), TEXT("Compare Plugin"), MB_OK);

	return ret;
}


std::vector<char> GetGitFileContent(const TCHAR* fullFilePath)
{
	std::vector<char> gitFileContent;

	if (!InitLibGit2())
	{
		::MessageBox(nppData._nppHandle, TEXT("Failed to initialize LibGit2 - operation aborted."),
				TEXT("Compare Plugin"), MB_OK);
		return gitFileContent;
	}

	git_repository* repo = NULL;

	char ansiGitFilePath[MAX_PATH];

	{
		TCharToChar(fullFilePath, ansiGitFilePath, sizeof(ansiGitFilePath));

		if (!git_repository_open_ext(&repo, ansiGitFilePath, 0, NULL))
		{
			const char* ansiGitDir = git_repository_path(repo);

			TCHAR gitDir[MAX_PATH];
			CharToTChar(ansiGitDir, gitDir, _countof(gitDir));

			TCHAR gitFile[MAX_PATH];
			RepoSubPath(fullFilePath, gitDir, gitFile, _countof(gitFile));

			TCharToChar(gitFile, ansiGitFilePath, sizeof(ansiGitFilePath));
		}
	}

	if (repo)
	{
		git_index* index;

		if (!git_repository_index(&index, repo))
		{
			size_t at_pos;

			if (git_index_find(&at_pos, index, ansiGitFilePath) != GIT_ENOTFOUND)
			{
				const git_index_entry* e = git_index_get_byindex(index, at_pos);

				if (e)
				{
					git_blob* blob;

					if (!git_blob_lookup(&blob, repo, &e->oid))
					{
						long blobSize = (long)git_blob_rawsize(blob);

						if (blobSize)
						{
							const void* content = git_blob_rawcontent(blob);

							if (content)
							{
								gitFileContent.resize(blobSize + 1, 0);
								std::memcpy(gitFileContent.data(), content, blobSize);
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

	if (gitFileContent.empty())
		::MessageBox(nppData._nppHandle, TEXT("No Git data found."), TEXT("Compare Plugin"), MB_OK);

	return gitFileContent;
}
