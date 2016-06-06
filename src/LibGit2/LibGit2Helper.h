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

#define GIT_OID_RAWSZ	20


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


typedef struct {
	char   *ptr;
	size_t asize, size;
} git_buf;


typedef int (*PGITLIBVERSION) (int *major, int *minor, int *rev);
typedef int (*PGITREPOSITORYOPENEXT) (git_repository **out, const char *path,
		unsigned int flags, const char *ceiling_dirs);
typedef const char* (*PGITREPOSITORYWORKDIR) (git_repository *repo);
typedef int (*PGITREPOSITORYINDEX) (git_index **out, git_repository *repo);
typedef const git_index_entry* (*PGITINDEXGETBYPATH) (git_index *index, const char *path, int stage);
typedef int (*PGITBLOBLOOKUP) (git_blob **blob, git_repository *repo, const git_oid *id);
typedef int (*PGITBLOBFILTERCONTENT) (git_buf *out, git_blob *blob, const char *as_path, int check_for_bin_data);
typedef void (*PGITBUFFREE) (git_buf *buf);
typedef void (*PGITBLOBFREE) (const git_blob *blob);
typedef void (*PGITINDEXFREE) (git_index *index);
typedef void (*PGITREPOSITORYFREE) (git_repository *repo);


extern PGITLIBVERSION			git_libgit2_version;
extern PGITREPOSITORYOPENEXT	git_repository_open_ext;
extern PGITREPOSITORYWORKDIR	git_repository_workdir;
extern PGITREPOSITORYINDEX		git_repository_index;
extern PGITINDEXGETBYPATH		git_index_get_bypath;
extern PGITBLOBLOOKUP			git_blob_lookup;
extern PGITBLOBFILTERCONTENT	git_blob_filtered_content;
extern PGITBUFFREE				git_buf_free;
extern PGITBLOBFREE				git_blob_free;
extern PGITINDEXFREE			git_index_free;
extern PGITREPOSITORYFREE		git_repository_free;


bool InitLibGit2();
