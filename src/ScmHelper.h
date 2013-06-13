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