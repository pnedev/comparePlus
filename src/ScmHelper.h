#ifndef SCMHELPER_H
#define SCMHELPER_H

#include "Compare.h"
#include "Shlwapi.h"
#include "sqlite\SqliteHelper.h"
#include "libgit2\LibGit2Helper.h"

bool GetScmBaseFolder(TCHAR* baseDirName, TCHAR* currentDir, TCHAR* svnDir);
void GetLocalScmPath(TCHAR* curDir, TCHAR* scmDir, TCHAR* filename, TCHAR* scmFilePath);
bool GetSvnBaseFile(TCHAR* curDir, TCHAR* svnDir, TCHAR* filename, TCHAR* svnBaseFile);
HGLOBAL GetContentFromGitRepo(TCHAR *gitDir, TCHAR *gitFilePath, long *size);

#endif