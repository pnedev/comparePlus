Compare plugin for Notepad++
------------

A very useful diff plugin to show the difference between:
-  2 files (side by side)
-  Diff against Git
-  Diff against SVN
-  Diff since last Save

Build Status
------------

AppVeyor `VS2013` and `VS2015`  [![Build status](https://ci.appveyor.com/api/projects/status/github/jsleroy/compare-plugin?svg=true)](https://ci.appveyor.com/project/jsleroy/compare-plugin)

Build Compare plugin for Notepad++ from source:
-------------------------------

 1. Open [`plugin_compare\compare-plugin\projects\2013\Compare.vcxproj`](https://github.com/jsleroy/compare-plugin/blob/master/projects/2013/Compare.vcxproj)
 2. Build Compare plugin [like a normal Visual Studio project](https://msdn.microsoft.com/en-us/library/7s88b19e.aspx).
 3. X64 builds currently just have beta status, report issues at [GitHub](https://github.com/jsleroy/compare-plugin/issues).

Installation:
----------

To install manually for usage with Notepad++, copy ComparePlugin.dll and ComparePlugin subfolder
into the plugins directory (`Notepad++ installation dir`)\Notepad++\Plugins.

Get Compare plugin for Notepad++ at the web:
-------------------------------

- via [PluginManager](http://docs.notepad-plus-plus.org/index.php/Plugin_Central)
- manual download from [Sourceforge](https://sourceforge.net/projects/npp-plugins/files/ComparePlugin/)
- manual download of beta versions from [Dropbox](https://www.dropbox.com/sh/f42cxkqppuapa1j/AADvlcmqoK_myRmoOpeL1yGMa?dl=0)
- manual download of continuous builds from [Appveyor](https://ci.appveyor.com/project/jsleroy/compare-plugin/history)

Additional information:
----------

- Compare plugin for Notepad++ [Contributors](https://github.com/jsleroy/compare-plugin/graphs/contributors)
- See also the [Notepad++ official site](http://notepad-plus-plus.org/) for more information.

Changelog:
----------

see [`ReleaseNotes.txt`](https://github.com/jsleroy/compare-plugin/blob/master/ReleaseNotes.txt)

TODOs:
----------

 1. Description of cmake build on mingw
 2. Correct changelog of 1.5.2, 1.5.6.8, 1.5.7
 3. Check broken Travis build
 4. Description of X64 build
 5. Description of dependency to libgit2.dll and sqlite.dll for plugin installation with current sources
 6. X64 version of libgit2.dll and sqlite.dll