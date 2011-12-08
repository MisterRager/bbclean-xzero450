	-------------------------------------------------------------------

			bbLeanBar is a plugin for Blackbox for Windows

					Copyright 2004 grischka
				http://bb4win.sourceforge.net/bblean/


	  This program is free software, released under the GNU General
		Public License (GPL version 2 or later). For details see:

				http://www.fsf.org/licenses/gpl.html


	 THIS PROGRAM IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL,
	   BUT WITHOUT ANY WARRANTY, WITHOUT EVEN THE IMPLIED WARRANTY
		OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

	-------------------------------------------------------------------


	bbLeanBar is a configurable taskbar with tray and clock for
	Blackbox for Windows.


	mouse clicks on taskentry:
	--------------------------
	left                activate task
	shift-left:         zoom task into current workspace

	right               minimize task
	shift-right         close task

	double-left/middle  move task to next workspace
	with shift:         move task to previous workspace

	control-right       show options menu

	alt-left            toggle task display modes
	alt-right           toggle 'current tasks only' mode
	(Change these two option permanently from the menu)  


	mouse clicks on workspace label:
	--------------------------------
	left                next workspace
	right               previous workspace


	mouse clicks on clock:
	----------------------
	double left         open clock properties
	right               blackbox menu


	for the entire bar:
	-------------------
	control-left-drag           move the bar
	control-left-double-click   reset the bar



	Clock date formating:
	---------------------

	%a Abbreviated weekday name 
	%A Full weekday name 
	%b Abbreviated month name 
	%B Full month name 
	%c Date and time representation appropriate for locale 
	%d Day of month as decimal number (01 - 31) 
	%H Hour in 24-hour format (00 - 23) 
	%I Hour in 12-hour format (01 - 12) 
	%j Day of year as decimal number (001 - 366) 
	%m Month as decimal number (01 - 12) 
	%M Minute as decimal number (00 - 59) 
	%p Current locale's A.M./P.M. indicator for 12-hour clock 
	%S Second as decimal number (00 - 59) 
	%U Week of year as decimal number, with Sunday as first day
	%w Weekday as decimal number (0 - 6; Sunday is 0) 
	%W Week of year as decimal number, with Monday as first day
	%x Date representation for current locale 
	%X Time representation for current locale 
	%y Year without century, as decimal number (00 - 99) 
	%Y Year with century, as decimal number 
	%% Percent sign 
	%z Either the time-zone name or time zone abbreviation
	%Z same as above

	The # flag alters the behaviour of the above as follows:

	%#c Long date and time representation appropriate for locale. 
	%#x Long date representation, appropriate for locale. 
	%#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y
	   Remove leading zeros (if any). 
	%#a, %#A, %#b, %#B, %#p, %#X, %#z, %#Z, %#% #
	   flag is ignored 

	-------------------------------------------------------------------

