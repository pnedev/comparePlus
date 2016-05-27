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

#pragma once

#define GIT_ENOTFOUND -3
#define GIT_OID_RAWSZ 20

typedef struct git_repository git_repository;
typedef struct git_index git_index;
typedef struct git_blob git_blob;
typedef __time64_t git_time_t;
typedef __int64 git_off_t;

typedef struct {
	git_time_t seconds;
	unsigned int nanoseconds;
} git_index_time;

typedef struct git_oid {
	/** raw binary formatted id */
	unsigned char id[GIT_OID_RAWSZ];
} git_oid;

typedef struct git_index_entry {
	git_index_time ctime;
	git_index_time mtime;
	unsigned int dev;
	unsigned int ino;
	unsigned int mode;
	unsigned int uid;
	unsigned int gid;
	git_off_t file_size;
	git_oid oid;
	unsigned short flags;
	unsigned short flags_extended;
	char *path;
} git_index_entry;

typedef int (*PGITREPOSITORYOPEN) (git_repository **out, const char *path);
typedef int (*PGITREPOSITORYINDEX) (git_index **out, git_repository *repo);
typedef int (*PGITINDEXFIND) (size_t *at_pos, git_index *index, const char *path);
typedef const git_index_entry * (*PGITINDEXGETBYINDEX) (git_index *index, size_t n);
typedef int (*PGITBLOBLOOKUP) (git_blob **blob, git_repository *repo, const git_oid *id);
typedef git_off_t (*PGITBLOBRAWSIZE) (const git_blob *blob);
typedef const void * (*PGITBLOBRAWCONTENT) (const git_blob *blob);
typedef void (*PGITBLOBFREE) (const git_blob *blob);
typedef void (*PGITINDEXFREE) (git_index *index);
typedef void (*PGITREPOSITORYFREE) (git_repository *repo);

extern PGITREPOSITORYOPEN	git_repository_open;
extern PGITREPOSITORYINDEX	git_repository_index;
extern PGITINDEXFIND		git_index_find;
extern PGITINDEXGETBYINDEX	git_index_get_byindex;
extern PGITBLOBLOOKUP		git_blob_lookup;
extern PGITBLOBRAWSIZE		git_blob_rawsize;
extern PGITBLOBRAWCONTENT	git_blob_rawcontent;
extern PGITBLOBFREE			git_blob_free;
extern PGITINDEXFREE		git_index_free;
extern PGITREPOSITORYFREE	git_repository_free;

bool InitLibGit2();
