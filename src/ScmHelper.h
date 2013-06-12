#ifndef SCMHELPER_H
#define SCMHELPER_H

#include "Compare.h"
#include "Shlwapi.h"
#include "sqlite\SqliteHelper.h"

bool GetSvnFolder(TCHAR* currentDir, TCHAR* svnDir);
bool GetSvnBaseFile(TCHAR* curDir, TCHAR* svnDir, TCHAR* filename, TCHAR* svnBaseFile);

#endif