To install copy ComparePlugin.dll into the plugins directory C:\Program Files\Notepad++\Plugins

A couple of notes:
------------------

	Using or clearing the compare plugin will cause the undo buffer to be cleared. 
	Unfortuntely, by adding lines, the editor no longer knows where the edit was, 
	and would put it in a random spot

	When you save a file during a compare, all of the extra lines are removed before the save 
	starts and added back in after the save is done. 
	This may cause a delay, and slight movement in the scrollbar.
	
	In order to clear the change indicators, the entire document style is cleared, and than 
	refreshed by unfolding all the lines. 
	So all lines will be unfolded after a compare or clear.
	
Change Log:
-----------

1.5.6.2

    1. FIXED: Small changes not visible in navigation bar (in bigger files) (thx to Rolf P.)
    2. FIXED: Syntax highlighting broken after 'Compare' and 'Clear Results' (since N++ 6.2.1)
    3. FIXED: Change highlight not visible (since N++ 6.2.1)

1.5.6.1

    1. FIXED: Weird focus clipping while shutting down (no application gets focus again, when N++ is gone).

1.5.6

    1. NEW: "Previous" and "Next" commands now jumping blockwise instead of linewise.
    2. NEW: When comparing to last save or SVN base: Temp files now inherit the language highlighting from the original file.
    3. NEW: Marker icons for moved state.
    4. FIXED: Restoring of "Synchronize Horizontal Scrolling" check state after "Clear results".
    5. FIXED: Swapping of "Navigation bar" check state when N++ starts after it was closed with opened navigation bar.
    6. CHANGED: When comparing to last save or SVN base: Show temp files in first view (left side) instead of second view.
    7. CHANGED: More intuitive default highlighting colors (green=new, red=deleted, yellow=changed, blue=moved).
    8. CHANGED: Navigation bar background color now system's active caption color.

1.5.5

	1. Side bar can now be used to scroll.
	2. Fixed an issue causing N++ to crash when comparing R/O files.

1.5.4

	1. New side bar showing a graphical view of comparison results.
	2. New navigation keys (go to first/next/previous/last result).
	3. Compare released in both UNICODE and ANSI version.
	4. New markers icons.

1.5.3

	1. New Option and About menu entry, thanks to Jens.
	2. Colors used for comparison results are now configurable in Option menu 
      (there is now no need to edit Compare.ini).
	3. Compare to last save shortcut move to Alt+S
	4. Compare against SVN base shortcut move to Alt+B
	5. Bookmark and Compare marker bugs resolved, thanks to Thell

1.5.2
    
1.5.1 

    1. New colors. Thanks to Mark Baines for the suggestions
	2. Fixes ( Thanks to Todd Schmitt for the excellent QA):	
       Memory leak fixed
       Letters were getting cut off if the Formats weren't Windows
       Occasionaly the some letters were cut off at the end of the file
       Crash bug if there was a blank line at the beginning of the document
	
1.5

	1. "Align Matches", "Detect Moves", and "Ignore spaces" are now options
	2. "Show Changes since last save" option added in case you want to see the differences 
       since you last saved, or if the file is modified outside of Notepad++ and 
       you want to see those changes
	3. "Compare with SVN base" if the file is in a svn repository you can see the 
       changes since you last committed
	4. There is now a colors section in the compare.ini to change the colors
	5. Fixed a bug in the change detection
	6. Compare checks to see if enough files are open before it checks if it has the right version.
	

1.4
	
	1. Fixed Issue where plugin would hang if you cleared a result that had an extra line at the end
	2. Fixed a bug where matches in the middle of insert and delete blocks would not line up properly
	3. Fixed bug that would delete a empty line on clear if there was a blank line under it
	4. Compares hashes of strings instead of the full string for some fairly significant speed improvements
	5. Changed lines are compared at the word level instead of the letter, which provides some more accurate results
	6. Spaces are no longer considered when comparing matches


1.3
	
	1. New feature! Matches will now sync up with each other, so its easier to see the differences in the files. 
       Much like how winmerge or some of the other major diff tools look. (thanks for the help Jens)

1.2.1

	1. Bug fix:
       Compare would hang if only one tab is open
    2. If compare moves a tab to another panel, than it will now move it back when the user selects Clear
	3. Optimization:
       When possible, changes are shifted to make the results more readable.
	
1.2

	1. By very, very popular demand: If two panels aren't already selected, the plugin will move the current tab 
       into the second panel and than run the compare.
	2. Improved Moved detection.
	3. Vastly improved Changed detection. Only lines that were actually modified will show up as green, 
       and the parts of the line that are different are highlighted.
	4. Horizontal and vertical syncronization are enabled by default. (thanks to jenslorenz)
	5. Fixed bug where files claimed to be identical if there was only one change. (Thanks to nicolasjolet)

1.1

	1. Fixed bug where display would not immediately show the changes after compare.
	2. Changed the way lines were marked to improve performance
	3. There is now a prompt to notify you if the two files are identical
	4. A description of each of the colors meaning was added to the about dialog box
	5. A very crude "moved" detection algorithm was added. So lines that are the same but just 
       in a different place are now marked as grey

	
	
