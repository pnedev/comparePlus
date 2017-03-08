/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2013 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017 Pavel Nedev (pg.nedev@gmail.com)
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

#include <stdlib.h>
#include <shlwapi.h>
#include <cstring>

#include "Compare.h"
#include "LibHelpers.h"
#include "SQLite/SqliteHelper.h"
#include "LibGit2/LibGit2Helper.h"


namespace // anonymous namespace
{

void TCharToChar(const wchar_t* src, char* dest, int destCharsCount)
{
	::WideCharToMultiByte(CP_ACP, 0, src, -1, dest, destCharsCount, NULL, NULL);
}


void RelativePath(const char* fullFilePath, const char* baseDir, char* filePath, unsigned filePathSize)
{
	filePath[0] = 0;

	char fullPath[MAX_PATH];

	strcpy_s(fullPath, _countof(fullPath), fullFilePath);
	for (int i = strlen(fullPath) - 1; i >= 0; --i)
	{
		if (fullPath[i] == '\\')
			fullPath[i] = '/';
	}

	char basePath[MAX_PATH];

	strcpy_s(basePath, sizeof(basePath), baseDir);
	for (int i = strlen(basePath) - 1; i >= 0; --i)
	{
		if (basePath[i] == '\\')
			basePath[i] = '/';
	}

	int relativePathPos = strlen(basePath);

	if (!strncmp(fullPath, basePath, relativePathPos))
	{
		if (fullPath[relativePathPos] == '/')
			++relativePathPos;

		strcpy_s(filePath, filePathSize, &fullPath[relativePathPos]);
	}
}


void RelativePath(const wchar_t* fullFilePath, const wchar_t* baseDir, wchar_t* filePath, unsigned filePathSize)
{
	filePath[0] = 0;

	wchar_t fullPath[MAX_PATH];

	wcscpy_s(fullPath, _countof(fullPath), fullFilePath);
	for (int i = wcslen(fullPath) - 1; i >= 0; --i)
	{
		if (fullPath[i] == L'\\')
			fullPath[i] = L'/';
	}

	wchar_t basePath[MAX_PATH];

	wcscpy_s(basePath, _countof(basePath), baseDir);
	for (int i = wcslen(basePath) - 1; i >= 0; --i)
	{
		if (basePath[i] == L'\\')
			basePath[i] = L'/';
	}

	int relativePathPos = wcslen(basePath);

	if (!wcsncmp(fullPath, basePath, relativePathPos))
	{
		if (fullPath[relativePathPos] == L'/')
			++relativePathPos;

		wcscpy_s(filePath, filePathSize, &fullPath[relativePathPos]);
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
			return true;

		// up one folder
		::PathCombine(testPath, fullDirPath, TEXT(".."));
		_tcscpy_s(fullDirPath, fullDirPathSize, testPath);
	}

	return false;
}

} // anonymous namespace


bool GetSvnFile(const TCHAR* fullFilePath, TCHAR* svnFile, unsigned svnFileSize)
{
	TCHAR svnTop[MAX_PATH];
	TCHAR svnBase[MAX_PATH];

	_tcscpy_s(svnBase, _countof(svnBase), fullFilePath);
	::PathRemoveFileSpec(svnBase);

	bool ret = LocateDirUp(TEXT(".svn"), svnBase, svnTop, _countof(svnTop));

	if (ret)
	{
		ret = false;

		TCHAR dotSvnIdx[MAX_PATH];

		::PathCombine(dotSvnIdx, svnTop, TEXT(".svn"));
		::PathCombine(svnBase, dotSvnIdx, TEXT("wc.db"));

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
				RelativePath(fullFilePath, svnTop, svnBase, _countof(svnBase));

				TCHAR sqlQuery[MAX_PATH + 64];
				_sntprintf_s(sqlQuery, _countof(sqlQuery), _TRUNCATE,
						TEXT("SELECT checksum FROM nodes_current WHERE local_relpath='%s';"), svnBase);

				sqlite3_stmt* pStmt;

				if (sqlite3_prepare16_v2(ppDb, sqlQuery, -1, &pStmt, NULL) == SQLITE_OK)
				{
					if (sqlite3_step(pStmt) == SQLITE_ROW)
					{
						const TCHAR* checksum = (const TCHAR*)sqlite3_column_text16(pStmt, 0);

						if (checksum[0] != 0)
						{
							TCHAR idx[128];

							_tcsncpy_s(idx, _countof(idx), checksum + 6, 2);

							::PathCombine(svnBase, dotSvnIdx, TEXT("pristine"));
							::PathCombine(dotSvnIdx, svnBase, idx);

							_tcscpy_s(idx, _countof(idx), checksum + 6);

							::PathCombine(svnBase, dotSvnIdx, idx);
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
			::PathCombine(svnTop, dotSvnIdx, TEXT("text-base"));

			const TCHAR* file = ::PathFindFileName(fullFilePath);

			::PathCombine(svnBase, svnTop, file);
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

	std::unique_ptr<LibGit>& gitLib = LibGit::load();
	if (!gitLib)
	{
		::MessageBox(nppData._nppHandle, TEXT("Failed to initialize LibGit2 - operation aborted."),
				TEXT("Compare Plugin"), MB_OK);
		return gitFileContent;
	}

	git_repository* repo = NULL;

	char ansiGitFilePath[MAX_PATH];

	{
		char ansiPath[MAX_PATH];

		TCharToChar(fullFilePath, ansiPath, sizeof(ansiPath));
		::PathRemoveFileSpecA(ansiPath);

		if (!gitLib->repository_open_ext(&repo, ansiPath, 0, NULL))
		{
			const char* ansiGitDir = gitLib->repository_workdir(repo);

			//reinit with fullFilePath after modification by PathRemoveFileSpecA(), needed to get the relative path
			TCharToChar(fullFilePath, ansiPath, sizeof(ansiPath));

			RelativePath(ansiPath, ansiGitDir, ansiGitFilePath, sizeof(ansiGitFilePath));
		}
	}

	if (repo)
	{
		git_index* index;

		if (!gitLib->repository_index(&index, repo))
		{
			const git_index_entry* e = gitLib->index_get_bypath(index, ansiGitFilePath, 0);

			if (e)
			{
				git_blob* blob;

				if (!gitLib->blob_lookup(&blob, repo, &e->id))
				{
					git_buf gitBuf = { 0 };

					if (!gitLib->blob_filtered_content(&gitBuf, blob, ansiGitFilePath, 1))
					{
						gitFileContent.resize(gitBuf.size + 1, 0);
						std::memcpy(gitFileContent.data(), gitBuf.ptr, gitBuf.size);

						gitLib->buf_free(&gitBuf);
					}

					gitLib->blob_free(blob);
				}
			}

			gitLib->index_free(index);
		}

		gitLib->repository_free(repo);
	}

	if (gitFileContent.empty())
		::MessageBox(nppData._nppHandle, TEXT("No Git data found."), TEXT("Compare Plugin"), MB_OK);

	return gitFileContent;
}
