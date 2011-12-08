/*
 ============================================================================

	BarExtender declarations for bbLeanBar 1.16 (bbClean)

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

#ifndef __BAREXTENDER_H
#define __BAREXTENDER_H

#define DLLEXPORT __declspec(dllexport)

#include <windows.h>

enum {
	EIF_NAME = 0,
	EIF_KEY,
	EIF_MULTIUSE,
	EIF_DYNAMICWIDTH
};

extern "C" {

	//
	//	GetExtenderInfo returns the information on an extender
	//
	DLLEXPORT LPCSTR getExtenderInfo(int field);
	typedef LPCSTR (*DLL_getExtenderInfo)(int field);

	//
	//	CreateExtender should return the hWnd of the created extender window
	//	or 0 if the function failed
	//
	DLLEXPORT HWND createExtender(const char* configString);
	typedef HWND (*DLL_createExtender)(const char* configString);

	//
	//	getWidth returns the width that the extender wishes to allocate from
	//	the bar.
	//
	DLLEXPORT int getWidth();
	typedef int (*DLL_getWidth)();

	//
	//	DestroyExtender destroys the extender
	//
	DLLEXPORT int destroyExtender(HWND extHandle);
	typedef int (*DLL_destroyExtender)(HWND extHandle);

}

#endif /* __BAREXTENDER_H */

