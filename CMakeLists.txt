cmake_minimum_required (VERSION 2.8)

set (CMAKE_SYSTEM_NAME Windows)

set (CMAKE_SYSROOT "$ENV{HOME}/bin/cross")

set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)

set (toolchain_prefix   i686-w64-mingw32)

set (CMAKE_C_COMPILER   ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-gcc)
set (CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-g++)
set (CMAKE_RC_COMPILER  ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-windres)

set (win32_inc_dir /${toolchain_prefix}/include)
set (win32_lib_dir /${toolchain_prefix}/lib)

project (Compare)

set (defs
    -DUNICODE -D_UNICODE -DMINGW_HAS_SECURE_API=1 -D_WIN32 -DWIN32
    -D_WIN32_WINNT=0x0501 -DWIN32_LEAN_AND_MEAN -DNOCOMM -DNDEBUG
)

set (CMAKE_CXX_FLAGS
    "-std=c++11 -O3 -mwindows -mthreads -municode -Wall -Wno-unknown-pragmas"
)

set (CMAKE_MODULE_LINKER_FLAGS
    "-s"
)

set (project_rc_files
    src/Compare.rc
)

set (project_sources
    src/AboutDlg/AboutDialog.cpp
    src/ColorCombo.cpp
    src/ColorPopup.cpp
    src/Compare.cpp
    src/Engine.cpp
    src/libgit2/LibGit2Helper.cpp
    src/NavDlg/NavDialog.cpp
    src/NPPHelpers.cpp
    src/OptionDlg/OptionDialog.cpp
    src/AboutDlg/URLCtrl.cpp
    src/Common/StaticDialog.cpp
    src/ProgressDlg/CProgress.cpp
    src/ScmHelper.cpp
    src/sqlite/SqliteHelper.cpp
)

include_directories (
	${win32_inc_dir}
	src/
	src/Common
	src/mba
	src/AboutDlg
	src/OptionDlg
	src/NavDlg
	src/ProgressDlg
	src/sqlite
	src/libgit2
)

add_definitions (${defs})

find_library (comctl32
    NAMES libcomctl32.a
    PATHS ${win32_lib_dir}
)

find_library (comdlg32
    NAMES libcomdlg32.a
    PATHS ${win32_lib_dir}
)

find_library (shlwapi
    NAMES libshlwapi.a
    PATHS ${win32_lib_dir}
)

add_library (Compare MODULE ${project_rc_files} ${project_sources})

target_link_libraries (Compare ${comctl32} ${comdlg32} ${shlwapi})

install (FILES ${CMAKE_BINARY_DIR}/libCompare.dll
    DESTINATION "$ENV{HOME}/.wine/drive_c/Program Files/Notepad++/plugins"
    RENAME ComparePlugin.dll
)
