Compare plugin for Notepad++
------------

A very useful diff plugin to show the difference between:
-  2 files (side by side)
-  Diff against Git (with the help of [libgit2](https://github.com/libgit2/libgit2) )
-  Diff against SVN (with the help of [sqlite](https://sqlite.org) )
-  Diff since last Save

Build Status
------------

- AppVeyor `VS2015`  [![Build status](https://ci.appveyor.com/api/projects/status/github/pnedev/compare-plugin?svg=true)](https://ci.appveyor.com/project/pnedev/compare-plugin)

Build Compare plugin for Notepad++ from source:
-------------------------------

 1. Open [`plugin_compare\compare-plugin\projects\2015\Compare.vcxproj`](https://github.com/pnedev/compare-plugin/blob/master/projects/2015/Compare.vcxproj)
 2. Build Compare plugin [like a normal Visual Studio project](https://msdn.microsoft.com/en-us/library/7s88b19e.aspx). Available platforms are x86 win32 and x64 for Unicode Release and Debug.
 3. CMake config is available and tested for the generators MinGW Makefiles, Visual Studio and NMake Makefiles

Installation:
----------

To install the plugin manually for usage with Notepad++, follow the insructions below based on your current Notepad++ version.

7.6.1+:
1. Create "ComparePlugin" folder in the plugins directory (`Notepad++ installation dir`)\Notepad++\Plugins.
2. Copy the contents of the [Latest Release](https://github.com/pnedev/compare-plugin/releases/latest) zip file into the newly created folder.
- ComparePlugin.dll : The core plugin.
- ComparePlugin\libgit2.dll and ComparePlugin\sqlite.dll : Provide the Diff against Git and SVN (Please use the correct library versions - x86 or x64).
3. Restart Notepad++.

Pre 7.6.1:
1. Copy the ComparePlugin.dll and ComparePlugin subfolder
into the plugins directory (`Notepad++ installation dir`)\Notepad++\Plugins.
- ComparePlugin.dll : The core plugin.
- ComparePlugin subfolder : Contains the libs libgit2.dll and sqlite.dll for the Diff against Git and SVN (Please use the correct library versions - x86 or x64).
2. Restart Notepad++.

Get Compare plugin for Notepad++ at the web:
-------------------------------

- via Notepad++ PluginManager
- from GitHub project link [Releases section](https://github.com/pnedev/compare-plugin/releases)
- manual download of continuous builds from [Appveyor](https://ci.appveyor.com/project/pnedev/compare-plugin/history)

Additional information:
----------

- Compare plugin for Notepad++ [Contributors](https://github.com/pnedev/compare-plugin/graphs/contributors)
- See also the [Notepad++ official site](http://notepad-plus-plus.org/) for more information.

Changelog:
----------

see [`ReleaseNotes.txt`](https://github.com/pnedev/compare-plugin/blob/master/ReleaseNotes.txt)
