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

set (win32_inc_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/include)
set (win32_lib_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/lib)

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
    src/NppAPI/StaticDialog.cpp
    src/AboutDlg/URLCtrl.cpp
    src/AboutDlg/AboutDialog.cpp
    src/SettingsDlg/ColorCombo.cpp
    src/SettingsDlg/ColorPopup.cpp
    src/SettingsDlg/SettingsDialog.cpp
    src/NavDlg/NavDialog.cpp
    src/ProgressDlg/ProgressDlg.cpp
    src/Engine/Engine.cpp
    src/Compare.cpp
    src/LibGit2/LibGit2Helper.cpp
    src/NPPHelpers.cpp
    src/LibHelpers.cpp
    src/SQLite/SqliteHelper.cpp
)

include_directories (
	${win32_inc_dir}
	src/
	src/NppAPI/
	src/XpmIcons/
	src/Engine/
	src/AboutDlg/
	src/SettingsDlg/
	src/NavDlg/
	src/ProgressDlg/
	src/SQLite/
	src/LibGit2/
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
