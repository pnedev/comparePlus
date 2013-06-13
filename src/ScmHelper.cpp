#include "ScmHelper.h"

bool GetScmBaseFolder(TCHAR* baseDirName, TCHAR* currentDir, TCHAR* svnDir)
{
	// search recursively upwards for a ".svn" folder

	TCHAR buffDir1[MAX_PATH] = {0};
	TCHAR buffDir2[MAX_PATH] = {0};
	TCHAR buffDir3[MAX_PATH] = {0};

	lstrcpy(buffDir1, currentDir);

	while (true)
	{
		PathCombine(buffDir2, buffDir1, baseDirName);
		if (PathFileExists(buffDir2))
		{
			lstrcpy(svnDir, buffDir2);
			return true;
		}
	
		PathCombine(buffDir2, buffDir1, L"..");
		if ((lstrlen(buffDir2) == 1) || !lstrcmp(buffDir2, buffDir1))
		{
			// we've searched until root, thus no more searching
			return false;
		}

		lstrcpy(buffDir1, buffDir2);
	}
}

void GetLocalScmPath(TCHAR* curDir, TCHAR* scmDir, TCHAR* filename, TCHAR* scmFilePath)
{
	TCHAR buffDir[MAX_PATH] = {0};

	int len = lstrlen(scmDir) - 4;
	if (lstrlen(curDir) > len)
	{
		lstrcpy(buffDir, curDir + len);
	}
	else
	{
		buffDir[0] = 0;
	}
	PathCombine(scmFilePath, buffDir, filename);
	for (int i = 0; i < lstrlen(scmFilePath); i++)
	{
		if (scmFilePath[i] == '\\')
		{
			scmFilePath[i] = '/';
		}
	}
}

bool GetSvnBaseFile(TCHAR* curDir, TCHAR* svnDir, TCHAR* filename, TCHAR* svnBaseFile)
{
	bool ret = false;
	TCHAR buffDir1[MAX_PATH] = {0};
	TCHAR buffDir2[MAX_PATH] = {0};

	// is it svn 1.7 or above?
	lstrcpy(buffDir1, svnDir);
	PathCombine(buffDir2, buffDir1, L"wc.db");
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
		
				wsprintf(statement, L"SELECT checksum FROM nodes_current WHERE local_relpath='%s';", svnFilePath);

				if (sqlite3_prepare16_v2(ppDb, statement, -1, &pStmt, NULL) == SQLITE_OK)
				{
					if (sqlite3_step(pStmt) == SQLITE_ROW)
					{
						const TCHAR* checksum = (const TCHAR*)sqlite3_column_text16(pStmt, 0);
						if (checksum[0] != 0)
						{
							TCHAR buffer[128];
							lstrcpyn(buffer, checksum + 6, 3);
							lstrcpy(buffDir1, svnDir);
							PathCombine(buffDir2, buffDir1, L"pristine");
							PathCombine(buffDir1, buffDir2, buffer);
							lstrcpy(buffer, checksum + 6);
							PathCombine(buffDir2, buffDir1, buffer);
							lstrcat(buffDir2, L".svn-base");
							if (PathFileExists(buffDir2))
							{
								lstrcpy(svnBaseFile, buffDir2);
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
			MessageBox(NULL, L"Can't init sqlite", L"ComparePlugin", MB_OK);
		}
	}
	else
	{
		// is it an old svn version?
		PathCombine(buffDir2, buffDir1, L"text-base");
		PathCombine(buffDir1, buffDir2, filename);
		lstrcat(buffDir1, L".svn-base");
		if(PathFileExists(buffDir1))
		{
			lstrcpy(svnBaseFile, buffDir1);
			ret = true;
		}
	}

	return ret;
}

void TCharToChar(const wchar_t* src, char* dest, int size) 
{ 
	WideCharToMultiByte(CP_ACP, 0, src, wcslen(src) + 1, dest , size, NULL, NULL); 
}

HGLOBAL GetContentFromGitRepo(TCHAR *gitDir, TCHAR *gitFilePath, long *size)
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
		MessageBox(NULL, L"Can't init libgit2", L"ComparePlugin", MB_OK);
	}

	return hMem;
}
