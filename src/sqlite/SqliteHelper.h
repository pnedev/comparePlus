#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include "Compare.h"

#define sqlite3 HANDLE
#define sqlite3_stmt HANDLE
#define SQLITE_OK 0
#define SQLITE_ROW 100

typedef int (*PSQLOPEN16) (const void *filename, sqlite3 **ppDb);
typedef int (*PSQLPREPARE16V2) (sqlite3 *db, const void *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
typedef int (*PSQLSTEP) (sqlite3_stmt *pStmt);
typedef const void * (*PSQLCOLUMNTEXT16) (sqlite3_stmt *pStmt, int iCol);
typedef int (*PSQLFINALZE) (sqlite3_stmt *pStmt);
typedef int (*PSQLCLOSE) (sqlite3 *db);

extern PSQLOPEN16 sqlite3_open16;
extern PSQLPREPARE16V2 sqlite3_prepare16_v2;
extern PSQLSTEP sqlite3_step;
extern PSQLCOLUMNTEXT16 sqlite3_column_text16;
extern PSQLFINALZE sqlite3_finalize;
extern PSQLCLOSE sqlite3_close;

bool InitSqlite();

#endif