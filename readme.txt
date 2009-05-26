To install copy ComparePlugin.dll into the plugins directory C:\Program Files\Notepad++\Plugins

A couple of notes:

	Using or clearing the compare plugin will cause the undo buffer to be cleared. Unfortuntely, by adding lines, the editor no longer knows where the edit was, and would put it in a random spot

	When you save a file during a compare, all of the extra lines are removed before the save starts and added back in after the save is done. This may cause a delay, and slight movement in the scrollbar.
	
	In order to clear the change indicators, the entire document style is cleared, and than refreshed by unfolding all the lines. So all lines will be unfolded after a compare or clear.
	
	
Change Log:

1.5.3
    New Option and About menu entry, thanks to Jens.
    Colors used for comparison results are now configurable in Option menu (there is now no need to edit Compare.ini).
    Compare to last save shortcut move to Alt+S
    Compare against SVN base shortcut move to Alt+B
    
1.5.1 
    New colors. Thanks to Mark Baines for the suggestions
	Fixes ( Thanks to Todd Schmitt for the excellent QA):	
	Memory leak fixed
	Letters were getting cut off if the Formats weren't Windows
	Occasionaly the some letters were cut off at the end of the file
	Crash bug if there was a blank line at the beginning of the document
	
1.5
	"Align Matches", "Detect Moves", and "Ignore spaces" are now options
	"Show Changes since last save" option added in case you want to see the differences since you last saved, or if the file is modified outside of Notepad++ and you want to see those changes
	"Compare with SVN base" if the file is in a svn repository you can see the changes since you last committed
	There is now a colors section in the compare.ini to change the colors
	
	Fixed a bug in the change detection
	
	Compare checks to see if enough files are open before it checks if it has the right version.
	

1.4
	Fixed Issue where plugin would hang if you cleared a result that had an extra line at the end

	Fixed a bug where matches in the middle of insert and delete blocks would not line up properly

	Fixed bug that would delete a empty line on clear if there was a blank line under it

	Compares hashes of strings instead of the full string for some fairly significant speed improvements

	Changed lines are compared at the word level instead of the letter, which provides some more accurate results

	Spaces are no longer considered when comparing matches


1.3
	New feature!
	matches will now sync up with each other, so its easier to see the differences in the files. Much like how winmerge or some of the other major diff tools look. (thanks for the help Jens)

1.2.1
	Bug fix:
	Compare would hang if only one tab is open
	If compare moves a tab to another panel, than it will now move it back when the user selects Clear

	Optimization:
	When possible, changes are shifted to make the results more readable.
	
1.2
	By very, very popular demand: If two panels aren't already selected, the plugin will move the current tab into the second panel and than run the compare.

	Improved Moved detection.

	Vastly improved Changed detection. Only lines that were actually modified will show up as green, and the parts of the line that are different are highlighted.

	Horizontal and vertical syncronization are enabled by default. (thanks to jenslorenz)

	Fixed bug where files claimed to be identical if there was only one change. (Thanks to nicolasjolet)

1.1
	Fixed bug where display would not immediately show the changes after compare.

	Changed the way lines were marked to improve performance

	There is now a prompt to notify you if the two files are identical

	A description of each of the colors meaning was added to the about dialog box

	A very crude "moved" detection algorithm was added. So lines that are the same but just in a different place are now marked as grey

	
	
