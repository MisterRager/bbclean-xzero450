/*---------------------------------------------------------------------------------
 bbWheelHook (© 2006-2008 nocd5)
 ----------------------------------------------------------------------------------
 bbWheelHook is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbWheelHook is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/
#ifndef _UTILS_H
#define _UTILS_H
// ================================================================================
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0500
#endif

#ifdef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <psapi.h>
#include "Tinylist.h"
// --------------------------------------------------------------------------------
#define IS_SPC(c) ((unsigned char)(c) <= 32)
// --------------------------------------------------------------------------------
#define strcpy  lstrcpy
#define strchr  _strchr
#define strcmpi lstrcmpi
// --------------------------------------------------------------------------------
char *set_my_path(HINSTANCE hInstance, char *path, char *fname);
string_node *read_exclusions(char *fname, string_node *xlist);
bool find_exclusions(string_node *xlist, char* pszPath, char* pszClassName);
bool ReadNextCommand(HANDLE hFile, LPSTR szBuffer, DWORD dwLength);
bool read_next_line(HANDLE hFile, LPSTR szBuffer, DWORD dwLength);
BOOL GetFileNameFromHwnd(HWND hWnd, LPTSTR lpszFileName, DWORD nSize);
char *_fgets(char *s, int size, HANDLE hFile);
char *_strchr(const char *s, int c);
// ================================================================================
#endif
