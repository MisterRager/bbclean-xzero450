/*
 ============================================================================

	Simple Label BarExtender for bbLeanBar 1.16 (bbClean)

 ============================================================================

	This file is part of the bbLean source code.

	Copyright © 2007 noccy
	http://dev.noccy.com/bbclean

	bbClean is free software, released under the GNU General Public License
	(GPL version 2 or later).

	http://www.fsf.org/licenses/gpl.html

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

 ============================================================================
*/

#include <string>
#include <bbapi.h>

#ifndef __BARLABELCTL_H
#define __BARLABELCTL_H

//
//  barLabel class. Hosts our delicate bar label
//
class barLabel
{
	public:
		barLabel();		// Constructor
		~barLabel();	// Destructor
		bool Initialize(const char* configString);
		HWND getLabelHandle();

	private:
		HWND CreateExtenderWindow();
		HWND hWndLabel;	// Window handle of our label

		// Options
		std::string barCaption;
		std::string barOnClick;
		std::string barWidth;
		std::string barID;
		std::string barStyle;
};

//
//  callback for label window
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


#endif // __BARLABELCTL_H
