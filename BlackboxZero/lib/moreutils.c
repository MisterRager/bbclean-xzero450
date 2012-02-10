/*
 ============================================================================

  This file is part of the bb4win_mod source code
  Copyright © 2001-2009 The Blackbox for Windows Development Team
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bb4win_mod
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bb4win_mod and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "bblib.h"
#include <winuser.h>

#define DWORD_PTR unsigned long

//===========================================================================
// API: FileExists
// Purpose: Checks for a files existance
//===========================================================================

int File_Exists(const char* szFileName)
{
    DWORD a = GetFileAttributes(szFileName);
    return (DWORD)-1 != a && 0 == (a & FILE_ATTRIBUTE_DIRECTORY);
}

//===========================================================================
// Function: locate_file
// Purpose:  load file from plugin directory
// In:       buffer, filename, extension (optional)
// Out:      full path
//===========================================================================

int locate_file(HINSTANCE hInstance, char *path, const char *fname, const char *ext)
{
	char *file_name_start;
	GetModuleFileName(hInstance, path, MAX_PATH);
	file_name_start = strrchr(path, '\\');
	if (file_name_start) ++file_name_start;
	else file_name_start = strchr(path, 0);
	if (stristr(fname, "."))
		strcpy(file_name_start, fname);
	else
		sprintf(file_name_start, "%s.%s", fname, ext);
	return File_Exists(path);
}

//===========================================================================
// Function: bbPlugin_LocateFile
// Purpose:  load rc file - either from plugin folder or from bb directory
// In:       handle to plugin, path buffer, size buffer, file|plugin name
// Out:      void
//===========================================================================

int bbPlugin_LocateFile(HINSTANCE hInstance, LPSTR lpPluginPath, DWORD nSize, LPCSTR lpString)
{
	HINSTANCE hInst;
	int i = 0;
	do
	{
		char *file_name_start;
		// First and third, we look for the config file
		// in the same folder as the plugin...
		hInst = hInstance;
		// second we check the blackbox directory
		if (i == 1) hInst = NULL;
		GetModuleFileName(hInst, lpPluginPath, nSize);
		file_name_start = strrchr(lpPluginPath, '\\');
		if (file_name_start) ++file_name_start;
		else file_name_start = strchr(lpPluginPath, 0);
		if (stristr(lpString, "."))
			strcpy(file_name_start, lpString);
		else
			sprintf(file_name_start, "%s.rc", lpString);
	} while (++i < 3 && 0 == File_Exists(lpPluginPath));
	return File_Exists(lpPluginPath);
}

//===========================================================================
int check_filetime(const char *fn, FILETIME *ft0)
{
    FILETIME ft;
    return get_filetime(fn, &ft) == 0 || CompareFileTime(&ft, ft0) != 0;// > 0;
}

/*----------------------------------------------------------------------------*/

unsigned int eightScale_down(unsigned int i)
{
	return i > 8 ? (i / 32 + 1) : iminmax(i, 0, 255);
}

unsigned int eightScale_up(unsigned int i)
{
	return i < 9 ? (i * 32 - 1) : iminmax(i, 0, 255);
}

/*----------------------------------------------------------------------------*/

// case insensitive string compare, up to length of second string
int my_substr_icmp(const char *a, const char *b)
{
	return memicmp(a, b, strlen(b));
}

//===========================================================================

int n_stricmp(const char **pp, const char *s)
{
	int n = (int)strlen (s);
	int i = memicmp(*pp, s, n);
	if (i) return i;
	i = (*pp)[n] - ' ';
	if (i > 0) return i;
	*pp += n;
	while (' '== **pp) ++*pp;
	return 0;
}

//===========================================================================
// helpers to remove excess info
int trim_address(char q[MAX_PATH], int is, int js)
{
	char *p; 
	p = (strchr(q, is));
	if (p != NULL)
	{
		p = strrev(strrchr(strrev(q), js));
		strcpy(q, (const char *)(p));
		return 1;
	}
	return 0;
}

//===========================================================================
// Function: substr_icmp
// Purpose:  strcmp for the second string as start of the first
//===========================================================================

int substr_icmp(const char *a, const char *b)
{
    return memicmp(a, b, strlen(b));
}

//===========================================================================
// Function: get_substring_index
// Purpose:  search for a start-string match in a string array
// In:       searchstring, array
// Out:      index or -1
//===========================================================================

int get_substring_index (const char *key, const char * const * string_array)
{
    int i;
    for (i=0; *string_array; i++, string_array++)
        if (0==substr_icmp(key, *string_array)) return i;
    return -1;
}

//===========================================================================
//const char *string_empty_or_null(const char *s)
//{
//    return NULL==s ? "<null>" : 0==*s ? "<empty>" : s;
//}

//===========================================================================

void draw_line_h(HDC hDC, int x1, int x2, int y, int w, COLORREF C)
{
    HGDIOBJ oldPen;
    if (0 == w) return;
    oldPen = SelectObject(hDC, CreatePen(PS_SOLID, 1, C));
    do
    {
        MoveToEx(hDC, x1, y, NULL);
        LineTo  (hDC, x2, y);
        ++y;
    } while (--w);
    DeleteObject(SelectObject(hDC, oldPen));
}

const char *get_delim(const char *path, int d)
{
    int nLen = strlen(path);
    int n = nLen;
    while (n) { if (path[--n] == d) return path + n; }
    return path + nLen;
}

//===========================================================================
// Function: add_slash
// Purpose:  add \ when not present
//===========================================================================

char *add_slash(char *d, const char *s)
{
    int l; memcpy(d, s, l = (int)strlen(s));
    if (l && !IS_SLASH(d[l-1])) d[l++] = '\\';
    d[l] = 0;
    return d;
}

//===========================================================================
// Function: is_relative_path
// Purpose:  check, if the path is relative
// In:
// Out:
//===========================================================================

int is_relative_path(const char *path)
{
    if (IS_SLASH(path[0])) return 0;
    if (strchr(path, ':')) return 0;
    return 1;
}

//===========================================================================
// Function: make_bb_path
// Purpose:  add the blackbox path as default
// In:
// Out:
//===========================================================================

char *make_bb_path(HINSTANCE h, char *dest, const char *src)
{
    dest[0]=0;
    if (is_relative_path(src))
		get_exe_path(h, dest, sizeof dest);
    return strcat(dest, src);
}

char* make_full_path(HINSTANCE h, char *buffer, const char *filename)
{
	buffer[0] = 0;
	if (NULL == strchr(filename, ':')) get_exe_path(h, buffer, sizeof buffer);
	return strcat(buffer, filename);
}

char *get_path(char *pszPath, int nMaxLen, const char *file)
{
    GetModuleFileName(NULL, pszPath, nMaxLen);
    *(char*)file_basename(pszPath) = 0;
	return strcat(pszPath, file);
}

//===========================================================================
// Function: 
// Purpose:  icon code
//===========================================================================

HICON GetIcon(HWND iWin)
{
	HICON hIcon = NULL;
	SendMessageTimeout(iWin, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG|SMTO_BLOCK, 300, (DWORD_PTR*)&hIcon);
	if (NULL == hIcon) hIcon = (HICON)(DWORD_PTR)GetClassLongPtr(iWin, GCLP_HICON);
	if (NULL == hIcon)
		SendMessageTimeout(iWin, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG|SMTO_BLOCK, 300, (DWORD_PTR*)&hIcon);
	if (NULL == hIcon) hIcon = (HICON)(DWORD_PTR)GetClassLongPtr(iWin, GCLP_HICONSM);
	if (NULL == hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
	return hIcon;
}

//===========================================================================
// Function: Settings_CreateShadowColor
// Purpose: creates a ShadowColor based on the existing textColor
// In:
// Out:
//===========================================================================

COLORREF Settings_CreateShadowColor(COLORREF textColor)
{
		if (textColor > 0xd0d0d0)
			return 0x000000;
		if (textColor < 0x303030)
			return 0xffffff;

		return 0xffffff - textColor;
}

//===========================================================================
// Function: FuzzyMatch
// Purpose: tests whether two colours are similar
// In: two colour values
// Out: true if diff <= 20%
//===========================================================================

int FuzzyMatch(COLORREF focus, COLORREF unfocus)
{
	int diff;
	
	if (focus == unfocus)
		return 1;

	if (focus > unfocus)
		diff = GetRValue(focus) - GetRValue(unfocus) + GetGValue(focus) - GetGValue(unfocus) + GetBValue(focus) - GetBValue(unfocus);
	else
		diff = GetRValue(unfocus) - GetRValue(focus) + GetGValue(unfocus) - GetGValue(focus) + GetBValue(unfocus) - GetBValue(focus);	

	if (diff < 73)
		return 1;

	return 0;
}

__inline int tobyte(int v)
{
    return ((v<0)?0:(v>255)?255:v);
}

COLORREF split(COLORREF c, int To)
{
	int r, g, b, d;
	r = GetRValue(c);
	g = GetGValue(c);
	b = GetBValue(c);
	d = To ? 4 :2;
	r += tobyte(r >> d);
	g += tobyte(g >> d);
	b += tobyte(b >> d);
	return RGB(r, g, b);
}

/*----------------------------------------------------------------------------*/
