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
#include "ScmHelper.h"

bool GetScmBaseFolder(const TCHAR* baseDirName, const TCHAR* currentDir, TCHAR* svnDir, unsigned svnDirSize)
{
	// search recursively upwards for a ".svn" folder

	TCHAR buffDir1[MAX_PATH] = {0};
	TCHAR buffDir2[MAX_PATH] = {0};

	_tcscpy_s(buffDir1, _countof(buffDir1), currentDir);

	while (true)
	{
		PathCombine(buffDir2, buffDir1, baseDirName);
		if (PathFileExists(buffDir2))
		{
			_tcscpy_s(svnDir, svnDirSize, buffDir2);
			return true;
		}

		PathCombine(buffDir2, buffDir1, TEXT(".."));
		if ((_tcslen(buffDir2) == 1) || !_tcscmp(buffDir2, buffDir1))
		{
			// we've searched until root, thus no more searching
			return false;
		}

		_tcscpy_s(buffDir1, _countof(buffDir1), buffDir2);
	}
}

void GetLocalScmPath(const TCHAR* curDir, const TCHAR* scmDir, const TCHAR* filename, TCHAR* scmFilePath)
{
	TCHAR buffDir[MAX_PATH] = {0};

	unsigned len = _tcslen(scmDir) - 4;
	if (_tcslen(curDir) > len)
	{
		_tcscpy_s(buffDir, _countof(buffDir), curDir + len);
	}
	else
	{
		buffDir[0] = 0;
	}
	PathCombine(scmFilePath, buffDir, filename);
	for (unsigned i = 0; i < _tcslen(scmFilePath); ++i)
	{
		if (scmFilePath[i] == '\\')
		{
			scmFilePath[i] = '/';
		}
	}
}

bool GetSvnBaseFile(const TCHAR* curDir, const TCHAR* svnDir, const TCHAR* filename,
		TCHAR* svnBaseFile, unsigned svnBaseFileSize)
{
	bool ret = false;
	TCHAR buffDir1[MAX_PATH] = {0};
	TCHAR buffDir2[MAX_PATH] = {0};

	// is it svn 1.7 or above?
	_tcscpy_s(buffDir1, _countof(buffDir1), svnDir);
	PathCombine(buffDir2, buffDir1, TEXT("wc.db"));
	if (PathFileExists(buffDir2))
	{
		if (InitSqlite())
		{
			sqlite3 *ppDb;
			if (sqlite3_open16(buffDir2, &ppDb) == SQLITE_OK)
			{
				sqlite3_stmt *pStmt;
				TCHAR svnFilePath[MAX_PATH];
				TCHAR statement[128];

				GetLocalScmPath(curDir, svnDir, filename, svnFilePath);

				_sntprintf_s(statement, _countof(statement), _TRUNCATE,
						TEXT("SELECT checksum FROM nodes_current WHERE local_relpath='%s';"), svnFilePath);

				if (sqlite3_prepare16_v2(ppDb, statement, -1, &pStmt, NULL) == SQLITE_OK)
				{
					if (sqlite3_step(pStmt) == SQLITE_ROW)
					{
						const TCHAR* checksum = (const TCHAR*)sqlite3_column_text16(pStmt, 0);
						if (checksum[0] != 0)
						{
							TCHAR buffer[128];
							_tcsncpy_s(buffer, _countof(buffer), checksum + 6, 3);
							_tcscpy_s(buffDir1, _countof(buffDir1), svnDir);
							PathCombine(buffDir2, buffDir1, TEXT("pristine"));
							PathCombine(buffDir1, buffDir2, buffer);
							_tcscpy_s(buffer, _countof(buffer), checksum + 6);
							PathCombine(buffDir2, buffDir1, buffer);
							_tcscat_s(buffDir2, _countof(buffDir2), TEXT(".svn-base"));
							if (PathFileExists(buffDir2))
							{
								_tcscpy_s(svnBaseFile, svnBaseFileSize, buffDir2);
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
			MessageBox(NULL, TEXT("Can't init sqlite"), TEXT("Compare Plugin"), MB_OK);
		}
	}
	else
	{
		// is it an old svn version?
		PathCombine(buffDir2, buffDir1, TEXT("text-base"));
		PathCombine(buffDir1, buffDir2, filename);
		_tcscat_s(buffDir1, _countof(buffDir1), TEXT(".svn-base"));
		if(PathFileExists(buffDir1))
		{
			_tcscpy_s(svnBaseFile, svnBaseFileSize, buffDir1);
			ret = true;
		}
	}

	return ret;
}

void TCharToChar(const wchar_t* src, char* dest, int size)
{
	WideCharToMultiByte(CP_ACP, 0, src, wcslen(src) + 1, dest , size, NULL, NULL);
}

HGLOBAL GetContentFromGitRepo(const TCHAR *gitDir, const TCHAR *gitFilePath, long *size)
{
	HGLOBAL hMem = NULL;

	char ansiGitDir[MAX_PATH];
	char ansiGitFilePath[MAX_PATH];
	TCharToChar(gitDir, ansiGitDir, sizeof(ansiGitDir));
	TCharToChar(gitFilePath, ansiGitFilePath, sizeof(ansiGitFilePath));

	if (InitLibGit2())
	{
		git_repository *repo;
		if (!git_repository_open(&repo, ansiGitDir))
		{
			git_index *index;
			if (!git_repository_index(&index, repo))
			{
				size_t at_pos;
				if (git_index_find(&at_pos, index, ansiGitFilePath) != GIT_ENOTFOUND)
				{
					const git_index_entry *e = git_index_get_byindex(index, at_pos);
					if (e)
					{
						git_blob *blob;
						if (!git_blob_lookup(&blob, repo, &e->oid))
						{
							long sizeBlob = (long)git_blob_rawsize(blob);
							if (sizeBlob)
							{
								const void * content = git_blob_rawcontent(blob);
								if (content)
								{
									hMem = GlobalAlloc(GMEM_FIXED, (SIZE_T)sizeBlob);
									if (hMem)
									{
										*size = sizeBlob;
										CopyMemory(hMem, content, (SIZE_T)*size);
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
	}
	else
	{
		MessageBox(NULL, TEXT("Can't init libgit2"), TEXT("Compare Plugin"), MB_OK);
	}

	return hMem;
}
