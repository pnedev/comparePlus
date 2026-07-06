/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2013 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
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
#include <windows.h>
#include <wchar.h>
#include <shlwapi.h>
#include <string>

#include "LibGit2Helper.h"
#include "NppHelpers.h"
#include "Tools.h"


namespace
{

inline std::wstring getDllPath()
{
	std::wstring dll = getPluginsHomePath();
	if (dll.empty())
		return dll;

	dll += L"\\ComparePlus\\libs\\git2.dll";

	return dll;
}

}


bool isGITlibFound()
{
	const std::wstring dllPath = getDllPath();

	if (dllPath.size())
		return fileExists(dllPath.c_str());

	return false;
}


std::unique_ptr<LibGit>	LibGit::Inst;


std::unique_ptr<LibGit>& LibGit::load()
{
	if (Inst)
		return Inst;

	const std::wstring dllPath = getDllPath();

	if (dllPath.empty())
		return Inst;

	HMODULE libGit2 = ::LoadLibraryW(dllPath.c_str());
	if (!libGit2)
		return Inst;

	Inst.reset(new LibGit);

	Inst->version = (PGITLIBVERSION)::GetProcAddress(libGit2, "git_libgit2_version");
	if (Inst->version)
	{
		int major, minor, rev;
		Inst->version(&major, &minor, &rev);

		if (major < 1 || (major == 1 && minor < 2)) // Don't accept old versions of the Git library
		{
			Inst.reset();
			return Inst;
		}

		Inst->_verStr = std::to_string(major);
		Inst->_verStr += '.';
		Inst->_verStr += std::to_string(minor);
		Inst->_verStr += '.';
		Inst->_verStr += std::to_string(rev);
	}
	else
	{
		Inst.reset();
		return Inst;
	}

	Inst->repository_open_ext = (PGITREPOSITORYOPENEXT)::GetProcAddress(libGit2, "git_repository_open_ext");
	if (!Inst->repository_open_ext)
	{
		Inst.reset();
		return Inst;
	}

	Inst->repository_workdir = (PGITREPOSITORYWORKDIR)::GetProcAddress(libGit2, "git_repository_workdir");
	if (!Inst->repository_workdir)
	{
		Inst.reset();
		return Inst;
	}
	Inst->repository_index = (PGITREPOSITORYINDEX)::GetProcAddress(libGit2, "git_repository_index");
	if (!Inst->repository_index)
	{
		Inst.reset();
		return Inst;
	}

	Inst->index_get_bypath = (PGITINDEXGETBYPATH)::GetProcAddress(libGit2, "git_index_get_bypath");
	if (!Inst->index_get_bypath)
	{
		Inst.reset();
		return Inst;
	}

	Inst->blob_lookup = (PGITBLOBLOOKUP)::GetProcAddress(libGit2, "git_blob_lookup");
	if (!Inst->blob_lookup)
	{
		Inst.reset();
		return Inst;
	}

	Inst->git_oid_fromstr = (PGITOIDFROMSTR)::GetProcAddress(libGit2, "git_oid_fromstrp");
	if (!Inst->git_oid_fromstr)
	{
		Inst.reset();
		return Inst;
	}

	Inst->reference_lookup = (PGITREFERENCELOOKUP)::GetProcAddress(libGit2, "git_reference_lookup");
	if (!Inst->reference_lookup)
	{
		Inst.reset();
		return Inst;
	}

	Inst->reference_peel = (PGITREFERENCEPEEL)::GetProcAddress(libGit2, "git_reference_peel");
	if (!Inst->reference_peel)
	{
		Inst.reset();
		return Inst;
	}

	Inst->object_id = (PGITOBJECTID)::GetProcAddress(libGit2, "git_object_id");
	if (!Inst->object_id)
	{
		Inst.reset();
		return Inst;
	}

	Inst->blob_filter_opt_init = (PGITBLOBFILTEROPTINIT)::GetProcAddress(libGit2, "git_blob_filter_options_init");
	if (!Inst->blob_filter_opt_init)
	{
		Inst.reset();
		return Inst;
	}

	Inst->blob_filter = (PGITBLOBFILTER)::GetProcAddress(libGit2, "git_blob_filter");
	if (!Inst->blob_filter)
	{
		Inst.reset();
		return Inst;
	}

	Inst->commit_lookup = (PGITCOMMITLOOKUP)::GetProcAddress(libGit2, "git_commit_lookup");
	if (!Inst->commit_lookup)
	{
		Inst.reset();
		return Inst;
	}

	Inst->commit_tree = (PGITCOMMITTREE)::GetProcAddress(libGit2, "git_commit_tree");
	if (!Inst->commit_tree)
	{
		Inst.reset();
		return Inst;
	}

	Inst->tree_entry_bypath = (PGITTREEENTRYBYPATH)::GetProcAddress(libGit2, "git_tree_entry_bypath");
	if (!Inst->tree_entry_bypath)
	{
		Inst.reset();
		return Inst;
	}

	Inst->tree_entry_to_object = (PGITTREEENTRYTOOBJ)::GetProcAddress(libGit2, "git_tree_entry_to_object");
	if (!Inst->tree_entry_to_object)
	{
		Inst.reset();
		return Inst;
	}

	Inst->blob_rawcontent = (PGITBLOBRAWCONTENT)::GetProcAddress(libGit2, "git_blob_rawcontent");
	if (!Inst->blob_rawcontent)
	{
		Inst.reset();
		return Inst;
	}

	Inst->blob_rawsize = (PGITBLOBRAWSIZE)::GetProcAddress(libGit2, "git_blob_rawsize");
	if (!Inst->blob_rawsize)
	{
		Inst.reset();
		return Inst;
	}

	Inst->buf_free = (PGITBUFFREE)::GetProcAddress(libGit2, "git_buf_dispose");
	if (!Inst->buf_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->blob_free = (PGITBLOBFREE)::GetProcAddress(libGit2, "git_blob_free");
	if (!Inst->blob_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->reference_free = (PGITREFERENCEFREE)::GetProcAddress(libGit2, "git_reference_free");
	if (!Inst->reference_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->index_free = (PGITINDEXFREE)::GetProcAddress(libGit2, "git_index_free");
	if (!Inst->index_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->commit_free = (PGITCOMMITFREE)::GetProcAddress(libGit2, "git_commit_free");
	if (!Inst->commit_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->tree_free = (PGITTREEFREE)::GetProcAddress(libGit2, "git_tree_free");
	if (!Inst->tree_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->tree_entry_free = (PGITTREEENTRYFREE)::GetProcAddress(libGit2, "git_tree_entry_free");
	if (!Inst->tree_entry_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->object_free = (PGITOBJFREE)::GetProcAddress(libGit2, "git_object_free");
	if (!Inst->object_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->repository_free = (PGITREPOSITORYFREE)::GetProcAddress(libGit2, "git_repository_free");
	if (!Inst->repository_free)
	{
		Inst.reset();
		return Inst;
	}

	Inst->init = (PGITLIBINIT)::GetProcAddress(libGit2, "git_libgit2_init");
	if (!Inst->init)
	{
		Inst.reset();
		return Inst;
	}

	// Get shutdown function address last to properly deinit library
	Inst->shutdown = (PGITLIBSHUTDOWN)::GetProcAddress(libGit2, "git_libgit2_shutdown");
	if (!Inst->shutdown)
	{
		Inst.reset();
		return Inst;
	}

	Inst->init();

	return Inst;
}


bool LibGit::commitOidFromName(git_repository* repo, const char* name, git_oid& out_oid) const
{
	if (!repo || !name)
		return false;

	bool ret = false;
	git_reference* ref = nullptr;

	std::string gitName(name);

	size_t pos = gitName.find_first_of('/');
	if (pos == std::string::npos)
	{
		if (reference_lookup(&ref, repo, (std::string("refs/heads/") + gitName).c_str()))
			reference_lookup(&ref, repo, (std::string("refs/tags/") + gitName).c_str());
	}
	else
	{
		pos = gitName.find_first_of('/', pos + 1);

		if (pos == std::string::npos)
			gitName.insert(0, "refs/remotes/");

		reference_lookup(&ref, repo, gitName.c_str());
	}

	if (ref)
	{
		git_object* obj = nullptr;

		// Peel to get the commit object
		if (!reference_peel(&obj, ref, GIT_OBJECT_COMMIT))
		{
			if (obj)
			{
				// Get the OID from the commit
				out_oid = *object_id(obj);
				ret = true;

				object_free(obj);
			}
		}

		reference_free(ref);
	}

	return ret;
}
