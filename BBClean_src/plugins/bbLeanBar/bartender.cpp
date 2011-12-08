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


#include <windows.h>
#include <stdio.h>
#include <string>

#include "extenders\barextender.h"

using namespace std;

bool QueryExtensionDetails(const char* libname, const char* itemkey);


bool GetExtenderList() {

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	// Get the path of the modules, and the pattern
	char rcpath[MAX_PATH];
	GetModuleFileName(m_hInstance, rcpath, sizeof(rcpath));
	std::string strPath = rcpath;
	strPath = strPath.substr(0,strPath.find_last_of('\\')+1);
	std::string strPattern = strPath + "ext_*.dll";
	std::string strModule;

	char modkey;

	dbg_printf("Bartender is about to look for %s...", strPattern.c_str());

	hFind = FindFirstFile(strPattern.c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		while(true)
		{
			dbg_printf("Bartender found DLL %s...", FindFileData.cFileName);
			strModule = strPath + FindFileData.cFileName;
			if (QueryExtensionDetails(strModule.c_str(),&modkey)) {
				dbg_printf("Loaded key '%s' from module %s!",modkey,strModule.c_str());
			}
			if (FindNextFile(hFind, &FindFileData) == 0) break;
		}
	}
	else
	{
		return false;
	}
	FindClose(hFind);
	return true;
}


bool QueryExtensionDetails(const char* libname, const char* itemkey) {
	// First, load the module. A NULL is a bad thing, so if that happens we return
	// a false.
	HINSTANCE hPlugin = LoadLibrary(libname);
	if (hPlugin == NULL)
	{
		dbg_printf("The module %s failed to load",libname);
	}
	else
	{
		// Okay, We have a handle to the loaded library now, so we declare a variable
		// getExtenderInfoFunc as the type DLL_getExtenderInfo. We then find the proc
		// entry point of the "getExtenderInfo" function and shove it in there.
		DLL_getExtenderInfo getExtenderInfoFunc;
		getExtenderInfoFunc = (DLL_getExtenderInfo)GetProcAddress(hPlugin,"getExtenderInfo");
		if (getExtenderInfoFunc == NULL)
		{
			// Again, a NULL is not a good thing to get. If we got it here, it means that
			// the procedure entry point could not be found. So tell the user that.
			dbg_printf("Module %s loaded but entry point of getExtenderInfo not found!",libname);
			return false;
		}
		else
		{
			// Since we were successful, go on with polling the information from the
			// function we just imported.
			itemkey = getExtenderInfoFunc(EIF_KEY);
			return true;
		}
	}
	return false;
}
