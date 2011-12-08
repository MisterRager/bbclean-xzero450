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
#include "Utils.h"
// --------------------------------------------------------------------------------
char *set_my_path(HINSTANCE hInstance, char *path, char *fname)
{
	int nLen = GetModuleFileName(hInstance, path, MAX_PATH);
	while (nLen && path[nLen-1] != '\\') nLen--;
	strcpy(path+nLen, fname);
	return path;
}

// --------------------------------------------------------------------------------
string_node* read_exclusions(char *fname, string_node *xlist)
{
	HANDLE hFile = CreateFile(
			fname,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);

	char fstr[MAX_PATH];
	if (hFile){ // Read mode
		while (ReadNextCommand(hFile, fstr, sizeof(fstr))){
			append_string_node(&xlist, fstr);
		}
		CloseHandle(hFile);
		return xlist;
	}
	return NULL;
}

// --------------------------------------------------------------------------------
bool find_exclusions(string_node *xlist, char* pszPath, char* pszClassName)
{
	string_node *sn;
	bool bExclude;
	dolist(sn, xlist){
		char *p = strchr(strchr(sn->str, ':') + 1, ':');
		if (p){
			*p = '\0';
			bExclude = !strcmpi(pszPath, sn->str) && !strcmpi(pszClassName, p+1);
			*p = ':';
		}
		else
			bExclude = !strcmpi(pszPath, sn->str);

		if (bExclude) return true;
	}
	return false;
}
// --------------------------------------------------------------------------------
bool ReadNextCommand(HANDLE hFile, LPSTR szBuffer, DWORD dwLength)
{
	while (read_next_line(hFile, szBuffer, dwLength))
	{
		char c = szBuffer[0];
		if (c && '#' != c && '!' != c) return true;
	}
	return false;
}

// --------------------------------------------------------------------------------
bool read_next_line(HANDLE hFile, LPSTR szBuffer, DWORD dwLength)
{
	if (hFile && _fgets(szBuffer, dwLength, hFile))
	{
		char *p, *q, c; p = q = szBuffer;
		while (0 != (c = *p) && IS_SPC(c)) p++;
		while (0 != (c = *p)) *q++ = IS_SPC(c) ? ' ' : c, p++;
		while (q > szBuffer && IS_SPC(q[-1])) q--;
		*q = 0;
		return true;
	}
	szBuffer[0] = 0;
	return false;
}

// --------------------------------------------------------------------------------
BOOL GetFileNameFromHwnd(HWND hWnd, LPTSTR lpszFileName, DWORD nSize)
{
	BOOL bResult = FALSE;
	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd , &dwProcessId);

	// process handle
	HANDLE hProcess;
	hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE, dwProcessId
	);
	if (hProcess)
	{
		HMODULE hModule;
		DWORD dwNeed;
		if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &dwNeed))
		{
			// get exe path from hModule
			if (GetModuleFileNameEx(hProcess, hModule, lpszFileName, nSize))
				bResult = TRUE;
		}
		// close process handle
		CloseHandle(hProcess) ;
	}
	GetLongPathName(lpszFileName, lpszFileName, nSize);
	return bResult;
}

// --------------------------------------------------------------------------------
char *_fgets(char *s, int size, HANDLE hFile){
	DWORD dwSize;
	ZeroMemory(s, size);
	for (int i = 0; i < size-1; i++){
		ReadFile(hFile, s, 1, &dwSize, NULL);
		if (dwSize == 0) return NULL;
		if (*s == '\n') break;
		*s++;
	}
	return s;
}

// --------------------------------------------------------------------------------
char *_strchr(const char *s, int c){
	do{ if (*++s == c) return (char*)s; }while(*s != '\0');
	return NULL;
}
