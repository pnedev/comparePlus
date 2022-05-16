cmake_minimum_required (VERSION 2.8)

set (CMAKE_SYSTEM_NAME Windows)

if (UNIX OR MINGW)
	set (CMAKE_SYSROOT "/usr")

	set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
	set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
	set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)

	if (WIN64)
		set (toolchain_prefix   x86_64-w64-mingw32)
	else ()
		set (toolchain_prefix   i686-w64-mingw32)
	endif ()

	set (CMAKE_C_COMPILER   ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-gcc)
	set (CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-g++)
	set (CMAKE_RC_COMPILER  ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-windres)

	set (win32_inc_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/include)
	set (win32_lib_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/lib)
endif ()

project (ComparePlus)

if (UNIX OR MINGW)
	set (defs
		-DUNICODE -D_UNICODE -DMINGW_HAS_SECURE_API=1 -D_WIN32 -DWIN32
		-D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOCOMM
	)

	if (NOT DEBUG)
		set (defs ${defs} -DNDEBUG)
	endif ()

	if (WIN64)
		set (defs ${defs} -DWIN64)
	endif ()

	if (MULTITHREAD)
		set (defs ${defs} -DMULTITHREAD)
	endif ()

	if (DLOG)
		set (defs ${defs} -DDLOG)
	endif ()

	if (OTHER_MOVED_ICONS)
		set (defs ${defs} -DOTHER_MOVED_ICONS)
	endif ()

	set (CMAKE_CXX_FLAGS
		"-std=c++14 -O3 -static-libgcc -static-libstdc++ -Wall -Wno-unknown-pragmas"
	)

	set (CMAKE_MODULE_LINKER_FLAGS
		"-s"
	)

	set (CMAKE_SHARED_MODULE_PREFIX "")
else ()
	set (defs
		-DUNICODE -D_UNICODE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES -D_WIN32 -DWIN32
		-D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOCOMM -DMULTITHREAD
	)

	set (CMAKE_CXX_FLAGS
		"/EHsc /LD /MT /MP /W4"
	)
endif ()

set (project_rc_files
	src/Compare.rc
)

set (project_sources
	src/NppAPI/DockingFeature/StaticDialog.cpp
	src/AboutDlg/URLCtrl.cpp
	src/AboutDlg/AboutDialog.cpp
	src/SettingsDlg/ColorCombo.cpp
	src/SettingsDlg/ColorPopup.cpp
	src/SettingsDlg/SettingsDialog.cpp
	src/NavDlg/NavDialog.cpp
	src/ProgressDlg/ProgressDlg.cpp
	src/Engine/Engine.cpp
	src/Tools.cpp
	src/UserSettings.cpp
	src/Compare.cpp
	src/LibGit2/LibGit2Helper.cpp
	src/NppHelpers.cpp
	src/LibHelpers.cpp
	src/SQLite/SqliteHelper.cpp
)

include_directories (
	${win32_inc_dir}
	src/
	src/NppAPI/
	src/NppAPI/DockingFeature/
	src/Icons/
	src/Engine/
	src/AboutDlg/
	src/SettingsDlg/
	src/NavDlg/
	src/ProgressDlg/
	src/SQLite/
	src/LibGit2/
)

add_definitions (${defs})

add_library (ComparePlus MODULE ${project_rc_files} ${project_sources})

if (UNIX OR MINGW)
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

	find_library (msimg32
		NAMES libmsimg32.a
		PATHS ${win32_lib_dir}
	)

	target_link_libraries (ComparePlus ${comctl32} ${comdlg32} ${shlwapi} ${msimg32})

	set (INSTALL_PATH
		"$ENV{HOME}/wine/drive_c/Program Files/Notepad++/plugins/ComparePlus"
	)

	install (FILES ${CMAKE_BINARY_DIR}/ComparePlus.dll
		DESTINATION "${INSTALL_PATH}"
	)
else ()
	target_link_libraries (ComparePlus comctl32 comdlg32 shlwapi msimg32)

	set (INSTALL_PATH
		"${PROJECT_SOURCE_DIR}/Notepad++/plugins/ComparePlus"
	)

	install (TARGETS ComparePlus
		DESTINATION "${INSTALL_PATH}"
	)
endif ()

message ("Install destination: ${INSTALL_PATH}")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	install (DIRECTORY libs/x64/
		DESTINATION "${INSTALL_PATH}/ComparePlus"
	)
else()
	install (DIRECTORY libs/x86/
		DESTINATION "${INSTALL_PATH}/ComparePlus"
	)
endif()
