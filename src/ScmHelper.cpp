#include "ScmHelper.h"

bool GetSvnFolder(TCHAR* currentDir, TCHAR* svnDir)
{
	// search recursively upwards for a ".svn" folder

	TCHAR buffDir1[MAX_PATH] = {0};
	TCHAR buffDir2[MAX_PATH] = {0};
	TCHAR buffDir3[MAX_PATH] = {0};

	lstrcpy(buffDir1, currentDir);

	while (true)
	{
		PathCombine(buffDir2, buffDir1, L".svn");
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

				// get the local svn path
				int len = lstrlen(svnDir) - 4;
				lstrcpy(buffDir1, curDir + len);
				PathCombine(svnFilePath, buffDir1, filename);
				for (int i = 0; i < lstrlen(svnFilePath); i++)
				{
					if (svnFilePath[i] == '\\')
					{
						svnFilePath[i] = '/';
					}
				}
		
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
