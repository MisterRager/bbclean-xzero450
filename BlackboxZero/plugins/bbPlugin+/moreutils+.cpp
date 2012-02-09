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
#include <windows.h>
#include <shlobj.h>

#define IS_SLASH(c) ((c) == '\\' || (c) == '/')
#define array_count(ary) (sizeof(ary) / sizeof(ary[0]))
#define have_imp(pp) ((DWORD_PTR)pp > 1)
#define NOT_XOBLITE strlen(BBVersion) != 7 && strlen(BBVersion) != 5

OSVERSIONINFO osInfo;
bool         usingNT;

int imax(int a, int b)
{
    return a>b?a:b;
}

int imin(int a, int b)
{
    return a<b?a:b;
}

int iminmax(int a, int b, int c)
{
    if (a>c) a=c;
    if (a<b) a=b;
    return a;
}

char* stristr(const char *aa, const char *bb)
{
    const char *a, *b; int c, d;
    do {
        for (a = aa, b = bb;;++a, ++b) {
            if (0 == (c = *b))
                return (char*)aa;
            if (0 != (d = *a^c)
                && (d != 32 || (c |= 32) < 'a' || c > 'z'))
                break;
        }
    } while (*aa++);
    return NULL;
}

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
	GetModuleFileName(hInstance, path, MAX_PATH);
	char *file_name_start = strrchr(path, '\\');
	if (file_name_start) ++file_name_start;
	else file_name_start = strchr(path, 0);
	if (stristr(fname, "."))
		strcpy(file_name_start, fname);
	else
		sprintf(file_name_start, "%s.%s", fname, ext);
	return File_Exists(path);
}

//===========================================================================
// shell support forks

LPCSTR bbVersion()
{
	static BOOL(WINAPI*pMakeMenuSEP)(Menu *PluginMenu);	
	LPCSTR bbv = GetBBVersion();
	if (0 == _memicmp(bbv, "bbLean (boxCore", 14)) return "boxCore";
	if (0 == _memicmp(bbv, "bbLean 1.16 (bbClean", 20)) return "bbCleanNEB";
	if (0 == _memicmp(bbv, "bbLean|bb4win_mod", 17)) return "bb4win_mod";
	if (0 == (_memicmp(bbv, "bb", 2) + strlen(bbv) - 3)) return "xoblite";
	if (0 == _memicmp(bbv, "0", 1)) return "bb4win";
	*(FARPROC*)&pMakeMenuSEP = GetProcAddress((HINSTANCE)GetModuleHandle(NULL),"MakeMenuSEP");
	if (pMakeMenuSEP) return "bbLeanMod";
	if (0 == strlen(bbv) - 11) return bbv;
	return bbv;
}

int message_box(int flg, const char *fmt, ...)
{
    int (*pBBMessageBox)(int flg, LPCSTR fmt);
    *(FARPROC*)&pBBMessageBox = GetProcAddress(GetModuleHandle(NULL), "BBMessageBox");
    if (0 != pBBMessageBox)
        return pBBMessageBox(flg, fmt);
	char buff[16];
	sprintf(buff, "%s %s", bbVersion(), GetBBVersion());
	return MessageBox(NULL, fmt, buff, flg|MB_TOPMOST|MB_SETFOREGROUND);
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

int get_string_index (const char *key, const char * const * string_array)
{
    int i; const char *s;
    for (i=0; NULL != (s = *string_array); i++, string_array++)
        if (0==stricmp(key, s))
            return i;
    return -1;
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
const char *string_empty_or_null(const char *s)
{
    return NULL==s ? "<null>" : 0==*s ? "<empty>" : s;
}

//===========================================================================

char *replace_argument1(char *d, const char *s, const char *arg)
{
    char format[256];
	if (strlen(s) >= sizeof format)
		return d;
    char *p = strstr(strcpy(format, s), "%1");
    if (p) p[1] = 's';
    sprintf(d, format, arg);
    return d;
}

void draw_line_h(HDC hDC, int x1, int x2, int y, int w, COLORREF C)
{
    if (0 == w) return;
    HGDIOBJ oldPen = SelectObject(hDC, CreatePen(PS_SOLID, 1, C));
    do
    {
        MoveToEx(hDC, x1, y, NULL);
        LineTo  (hDC, x2, y);
        ++y;
    } while (--w);
    DeleteObject(SelectObject(hDC, oldPen));
}

//===========================================================================

int load_imp(void *pp, const char *dll, const char *proc)
{
    if (0 == *(DWORD_PTR*)pp) {
        HMODULE hm = GetModuleHandle(dll);
        if (NULL == hm)
            hm = LoadLibrary(dll);
        if (hm)
            *(FARPROC*)pp = GetProcAddress(hm, proc);
        if (0 == *(DWORD_PTR*)pp)
            *(DWORD_PTR*)pp = 1;
    }
    return have_imp(*(void**)pp);
}

int is_absolute_path(const char *path)
{
    const char *p, *q; char c;
    for (p = q = path; 0 != (c = *p);) {
        if (IS_SLASH(c))
            return p == q;
        ++p;
        if (':' == c) {
            q = p;
            if (':' == *p)
                return 1;
        }
    }
    return 0;
}

const char *file_basename(const char *path)
{
    int nLen = strlen(path);
    while (nLen && !IS_SLASH(path[nLen-1])) nLen--;
    return path + nLen;
}

char *file_extension(const char *name)
{
    char *e, *p = e = strchr(name = file_basename(name), 0);
    while (p > name)
        if (*--p == '.')
            return p;
    return e;
}

static DWORD (WINAPI *pGetLongPathName)(
    LPCTSTR lpszShortPath,
    LPTSTR lpszLongPath,
    DWORD cchBuffer);

char* get_exe_path(HINSTANCE h, char* pszPath, int nMaxLen)
{
    GetModuleFileName(h, pszPath, nMaxLen);
    if (load_imp(&pGetLongPathName, "KERNEL32.DLL", "GetLongPathNameA"))
        pGetLongPathName(pszPath, pszPath, nMaxLen);
    *(char*)file_basename(pszPath) = 0;
    return pszPath;
}

char *set_my_path(HINSTANCE h, char *dest, const char *fname)
{
    dest[0] = 0;
    if (0 == is_absolute_path(fname))
        get_exe_path(h, dest, MAX_PATH);
    return strcat(dest, fname);
}

char *get_path(char *pszPath, int nMaxLen, const char *file)
{
    GetModuleFileName(NULL, pszPath, nMaxLen);
    *(char*)file_basename(pszPath) = 0;
	return strcat(pszPath, file);
}

//===========================================================================
// Function: GetOSVersion - bb4win_mod
// Purpose: Retrieves info about the current OS & bit version
// In: None
// Out: int = Returns an integer indicating the OS & bit version
//===========================================================================

int GetOSVersion(void)
{
    ZeroMemory(&osInfo, sizeof(osInfo));
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    GetVersionEx(&osInfo);

	//64-bit OS test, when running as 32-bit under WoW
	BOOL bIs64BitOS= FALSE;
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle("kernel32"),"IsWow64Process");
	if (NULL != fnIsWow64Process)
		fnIsWow64Process(GetCurrentProcess(), &bIs64BitOS);
	/*usingx64 = bIs64BitOS;
	//64-bit OS test, if compiled as native 64-bit. In case we ever need it.
	if (!usingx64)
		usingx64=(sizeof(int)!=sizeof(void*));*/

    usingNT         = osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;

    if (usingNT)
		return ((osInfo.dwMajorVersion * 10) + osInfo.dwMinorVersion + (bIs64BitOS ? 5 : 0)); // NT 40; Win2kXP 50; Vista 60; etc.


	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        if (osInfo.dwMinorVersion >= 90)
            return 30; // Windows ME
        if (osInfo.dwMinorVersion >= 10)
            return 20; // Windows 98
    }
	return 10; // Windows 95
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
	if (focus == unfocus)
		return 1;

	int diff;

	if (focus > unfocus)
		diff = GetRValue(focus) - GetRValue(unfocus) + GetGValue(focus) - GetGValue(unfocus) + GetBValue(focus) - GetBValue(unfocus);
	else
		diff = GetRValue(unfocus) - GetRValue(focus) + GetGValue(unfocus) - GetGValue(focus) + GetBValue(unfocus) - GetBValue(focus);	

	if (diff < 73)
		return 1;

	return 0;
}

//===========================================================================
// Function: BBDrawText
// Purpose: draws text with(out) shadows
// In:
// Out:
//===========================================================================
#define _CopyOffsetRect(lprcDst, lprcSrc, dx, dy) (*lprcDst).left = (*lprcSrc).left + (dx), (*lprcDst).right = (*lprcSrc).right + (dx), (*lprcDst).top = (*lprcSrc).top + (dy), (*lprcDst).bottom = (*lprcSrc).bottom + (dy)

int BBDrawText(HDC hDC, const char *lpString, int nCount, RECT *lpRect, unsigned uFormat, StyleItem * pSI)
{
#ifdef NOT_XOBLITE  
	RECT Rs;
	COLORREF cr0;
	int i, j;
	bool outlineText = ReadBool(bbrcPath(), "session.outlineText:", false);

	if (outlineText || pSI->FontShadow)
	{
		cr0 = SetTextColor(hDC, pSI->OutlineColor);
		for (i = 0; i < (2 + pSI->ShadowX); i++){
			for (j = 0; j < (2 + pSI->ShadowY); j++){
				_CopyOffsetRect(&Rs, lpRect, i, j);
				DrawText(hDC, lpString, nCount, &Rs, uFormat);
			}
		}
	}

	if (pSI->ShadowXY)
	{
		if (outlineText || pSI->FontShadow)
		{
			cr0 = SetTextColor(hDC, pSI->ShadowColor);
			for (i = 0; i < 3; i++){
				for (j = 0; j < 3; j++){
					_CopyOffsetRect(&Rs, lpRect, i-pSI->ShadowX, j-pSI->ShadowY);
					DrawText(hDC, lpString, nCount, &Rs, uFormat);
				}
			}
		}
		else
		{
			cr0 = SetTextColor(hDC, pSI->ShadowColor);
			_CopyOffsetRect(&Rs, lpRect, pSI->ShadowX, pSI->ShadowY);
			DrawText(hDC, lpString, nCount, &Rs, uFormat);
		}
		SetTextColor(hDC, cr0);
	}
#endif
return DrawText(hDC, lpString, nCount, lpRect, uFormat);
}

COLORREF mixcolors(COLORREF c1, COLORREF c2, int f)
{
    int n = 255 - f;
    return RGB(
        (GetRValue(c1)*f+GetRValue(c2)*n)/255,
        (GetGValue(c1)*f+GetGValue(c2)*n)/255,
        (GetBValue(c1)*f+GetBValue(c2)*n)/255
        );
}

int get_fontheight(HFONT hFont)
{
    TEXTMETRIC TXM;
    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ other = SelectObject(hdc, hFont);
    int ret = 12;
    if (GetTextMetrics(hdc, &TXM))
        ret = TXM.tmHeight - TXM.tmExternalLeading;/*-TXM.tmInternalLeading;*/
    SelectObject(hdc, other);
    DeleteDC(hdc);
    return ret;
}

inline int tobyte(int v) {
    return ((v<0)?0:(v>255)?255:v);
}

COLORREF split(COLORREF c, bool To)
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

COLORREF switch_rgb (COLORREF c)
{
    return ((c&0x0000ff)<<16) | (c&0x00ff00) | ((c&0xff0000)>>16);
}

char* make_full_path(HINSTANCE h, char *buffer, const char *filename)
{
	buffer[0] = 0;
	if (NULL == strchr(filename, ':')) get_exe_path(h, buffer, sizeof buffer);
	return strcat(buffer, filename);
}

/*----------------------------------------------------------------------------*/
void *memset(void *d, int c, unsigned l)
{
	char *p = (char *)d;
	while (l) *p++ = c, --l;
	return d;
}

bool get_style(const char *style, StyleItem *si, const char *key)
{
	const char *s, *p;
	COLORREF c; int w;
	char fullkey[80], temp1[80], temp2[80], *r;

	memset(si, 0, sizeof *si);
	r = strchr(strcpy(fullkey, key), 0);
	s = style;

	strcpy(r, ".appearance:");
	p = ReadString(s, fullkey, NULL);
	if (p) {
		si->bordered = IsInString(p, "border");
	} else {
		strcpy(r, ":");
		p = ReadString(s, fullkey, NULL);
		if (NULL == p)
			return false;
		si->bordered = true;
	}
	ParseItem(p, si);

	if (B_SOLID != si->type || si->interlaced)
		strcpy(r, ".color1:");
	else
		strcpy(r, ".backgroundColor:");
	c = ReadColor(s, fullkey, NULL);
	if ((COLORREF)-1 == c) {
		strcpy(temp2, ReadString(s, "*backgroundColor:", NULL));
		strcpy(temp1, ReadString(s, "*.backgroundColor:", temp2));
		strcpy(temp2, ReadString(s, "*Color:", temp1));
		strcpy(temp1, ReadString(s, "*.Color:", temp2));
		strcpy(temp2, ReadString(s, "*Color1:", temp1));
		strcpy(temp1, ReadString(s, "*.Color1:", temp2));
		strcpy(r, ".color:");
		c = ReadColor(s, fullkey, temp1);
		if ((COLORREF)-1 == c)
			return false;
	}

	si->Color = si->ColorTo = c;
	if (B_SOLID != si->type || si->interlaced) {
		strcpy(r, ".color2:");
		c = ReadColor(s, fullkey, NULL);
		if ((COLORREF)-1 == c) {
			strcpy(temp2, ReadString(s, "*ColorTo:", NULL));
			strcpy(temp1, ReadString(s, "*.ColorTo:", temp2));
			strcpy(temp2, ReadString(s, "*Color2:", temp1));
			strcpy(temp1, ReadString(s, "*.Color2:", temp2));
			strcpy(r, ".colorTo:");
			c = ReadColor(s, fullkey, temp1);
			if ((COLORREF)-1 != c) 
				si->ColorTo = c;
		}
	}

	if (si->bordered) {
		strcpy(r, ".borderColor:");
		c = ReadColor(s, fullkey, NULL);
		if ((COLORREF)-1 != c)
			si->borderColor = c;
		else
			si->borderColor = ReadColor(s, "borderColor:", "black");

		strcpy(r, ".borderWidth:");
		w = ReadInt(s, fullkey, -100);
		if (-100 != w)
			si->borderWidth = w;
		else
			si->borderWidth = ReadInt(s, "borderWidth:", 1);
	}

	strcpy(r, ".marginWidth:");
	w = ReadInt(s, fullkey, -100);
	if (-100 != w)
		si->marginWidth = w;
	else
		si->marginWidth = ReadInt(s, "bevelWidth:", 2);
	return true;
}

//===========================================================================
// Function: select_folder
// Purpose: Locates a target folder|file
// In: BBhwnd, start location|szAppName, buffer for target folder, files?
// Out: bool
//===========================================================================
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
		case BFFM_INITIALIZED:
		{
			if (is_absolute_path((const char *)lpData))
				SendMessage(hwnd, BFFM_SETSELECTION, true, (LPARAM)lpData);
			else
				SetWindowText(hwnd, (char *)lpData);
			break;
		}
	}
	return 0;
}

bool select_folder(HWND hwnd, const char *title, char *path, bool files)
{
	BROWSEINFO bi;
	LPITEMIDLIST p;
	LPMALLOC pMalloc = NULL;
	SHGetMalloc(&pMalloc);

	bi.hwndOwner    = hwnd;
	bi.pidlRoot     = NULL;
	bi.lpszTitle    = "Select Folder...";
	bi.ulFlags      =  files ? BIF_USENEWUI | BIF_BROWSEINCLUDEFILES : BIF_USENEWUI;
	bi.lpfn         = BrowseCallbackProc;
	bi.lParam       = (LPARAM)title;
	p = SHBrowseForFolder(&bi);
	path[0] = 0;
	if (p)
	{
		SHGetPathFromIDList(p, path);
		pMalloc->Free(p);
	}
	pMalloc->Release();
	return NULL != p;
}

//===========================================================================
