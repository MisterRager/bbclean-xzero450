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

#include "../barextender.h"
#include "labelctl.h"

barLabel *thisLabel;

const char szVersion     [] = "1.0";
const char szName        [] = "Simple Label";
const char szKey         [] = "label";
const char szMultiUse    [] = "false";
const char szDynamicWidth[] = "false";

//
// getExtenderInfo returns information about the extender to bbLeanBar.
//
DLLEXPORT LPCSTR getExtenderInfo(int field) {
	switch(field) {
		case EIF_NAME: return szName;
		case EIF_KEY: return szKey;
		case EIF_MULTIUSE: return szMultiUse;
		case EIF_DYNAMICWIDTH: return szDynamicWidth;
	}
	return(NULL);
}

//
// createExtender creates an extender with the specified config string.
//
DLLEXPORT HWND createExtender(const char* configString) {
	thisLabel = new barLabel();
	thisLabel->Initialize(configString);
	return(thisLabel->getLabelHandle());
}

//
// destroyExtender destroys the extender again. Reconfiguring will do this.
//
DLLEXPORT int destroyExtender(HWND extHandle) {
	delete thisLabel;
	return(0);
}

//
// getWidth returns the width of the extender. If this value is 0 the extender
// is set to automatic width, and bbleanbar should be left to make a decision.
//
DLLEXPORT int getWidth() {
	return(0);
}
