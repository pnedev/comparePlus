# this file is part of notepad++
# Copyright (C)2003 Don HO ( donho@altern.org )
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  
.SUFFIXES: .cpp
CPP = g++
CC = gcc

#UserDefineDialog_res.o 
OBJS = NppInsertPlugin.o

#
# the paths
TARGETDIR = ./bin


DLL = $(TARGETDIR)/NppInsertPlugin.dll

CFLAGS = -DBUILD_DLL

LDFLAGS = -export-dynamic,-Wl,
#LDFLAGS = -Wl,--out-implib,libtstdll.a
#LDFLAGS = -mwindows -lcomctl32 -lshlwapi -llibshell32

ALL: $(DLL)

$(DLL):	$(OBJS)
	$(CC) -shared -o $@ $(LDFLAGS) $< -lshlwapi


NppInsertPlugin.o : PluginInterface.h Scintilla.h NppInsertPlugin.cpp
	$(CC) -c $(CFLAGS) NppInsertPlugin.cpp -o $@

clean:
	rm -f $(OBJS) $(DLL)

