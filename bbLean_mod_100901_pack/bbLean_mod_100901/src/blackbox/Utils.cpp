/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
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

#include "BB.h"
#include "Settings.h"
#include <time.h>
#include <shlobj.h>
#include <shellapi.h>

//===========================================================================
// API: BBMessageBox
// Purpose:  standard BB-MessageBox
//===========================================================================

int BBMessageBox(int flg, const char *fmt, ...)
{
    char buffer[10000], *msg = buffer, *caption = "bbLean", *p;
    va_list arg; va_start(arg, fmt); vsprintf (buffer, fmt, arg);
    if ('#' == msg[0] && NULL != (p = strchr(msg+1, msg[0])))
        caption = msg+1, *p=0, msg = p+1;

    //return MessageBox (BBhwnd, msg, caption, flg | MB_SYSTEMMODAL | MB_SETFOREGROUND);

    MSGBOXPARAMS mp; ZeroMemory(&mp, sizeof mp);
    mp.cbSize = sizeof mp;
    mp.hInstance = hMainInstance;
    //mp.hwndOwner = NULL;
    mp.lpszText = msg;
    mp.lpszCaption = caption;
    mp.dwStyle = flg | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_USERICON;
    mp.lpszIcon = MAKEINTRESOURCE(IDI_BLACKBOX);
    MessageBeep(0);
    return MessageBoxIndirect(&mp);
}

//===========================================================================
// Function: BBRegisterClass
// Purpose:  Register a window class, display error on failure
//===========================================================================

BOOL BBRegisterClass (WNDCLASS *pWC)
{
    if(RegisterClass(pWC)) return 1;
    BBMessageBox(MB_OK, NLS2("$BBError_RegisterClass$",
        "Error: Could not register \"%s\" window class."), pWC->lpszClassName);
    return 0;
}

//===========================================================================
// Function: rgb, switch_rgb
// Purpose: macro replacement and rgb adjust
//===========================================================================

// COLORREF rgb (unsigned r,unsigned g,unsigned b)
// {
//     return RGB(r,g,b);
// }

// COLORREF switch_rgb (COLORREF c)
// {
//     return (c&0x0000ff)<<16 | (c&0x00ff00) | (c&0xff0000)>>16;
// }

COLORREF mixcolors(COLORREF c1, COLORREF c2, int f)
{
	int n = 255 - f;
	return RGB(
		(GetRValue(c1)*f+GetRValue(c2)*n)>>8,
		(GetGValue(c1)*f+GetGValue(c2)*n)>>8,
		(GetBValue(c1)*f+GetBValue(c2)*n)>>8
		);
}

COLORREF shadecolor(COLORREF c, int f)
{
    int r = (int)GetRValue(c) + f;
    int g = (int)GetGValue(c) + f;
    int b = (int)GetBValue(c) + f;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return RGB(r, g, b);
}

// unsigned greyvalue(COLORREF c)
// {
//     unsigned r = GetRValue(c);
//     unsigned g = GetGValue(c);
//     unsigned b = GetBValue(c);
//     return (r*79 + g*156 + b*21) / 256;
// }

void draw_line_h(HDC hDC, int x1, int x2, int y, int w, COLORREF C)
{
    HGDIOBJ oldPen = SelectObject(hDC, CreatePen(PS_SOLID, 1, C));
    while (w--){
        MoveToEx(hDC, x1, y, NULL);
        LineTo  (hDC, x2, y);
        ++y;
    }
    DeleteObject(SelectObject(hDC, oldPen));
}

/*----------------------------------------------------------------------------*/

// int imax(int a, int b) {
//     return a>b?a:b;
// }

// int imin(int a, int b) {
//     return a<b?a:b;
// }

// int iminmax(int a, int b, int c) {
//     if (a<b) a=b;
//     if (a>c) a=c;
//     return a;
// }

// bool is_alpha(int c)
// {
//     return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z';
// }

// bool is_num(int c)
// {
//     return c >= '0' && c <= '9';
// }

// bool is_alnum(int c)
// {
//     return is_alpha(c) || is_num(c);
// }


//===========================================================================
char *extract_string(char *dest, const char *src, int n)
{
    memcpy(dest, src, n);
    dest[n] = 0;
    return dest;
}

//===========================================================================
// Function: strcpy_max
// Purpose:  copy a string with maximal length (a safer strncpy)
//===========================================================================

char *strcpy_max(char *dest, const char *src, size_t maxlen)
{
    return extract_string(dest, src, imin(strlen(src), maxlen-1));
}

//===========================================================================
// Function: stristr
// Purpose:  ignore case strstr
//===========================================================================

const char* stristr(const char *aa, const char *bb)
{
    const char *a, *b; char c, d;
    do
    {
        for (a = aa, b = bb;;++a, ++b)
        {
            if (0 == (c = *b)) return aa;
            if (0 != (d = *a^c))
                if (d != 32 || (c |= 32) < 'a' || c > 'z')
                    break;
        }
    } while (*aa++);

    return NULL;
}

//===========================================================================
const char *string_empty_or_null(const char *s)
{
    return NULL==s ? "<null>" : 0==*s ? "<empty>" : s;
}

//===========================================================================
// Function: unquote
// Purpose:  remove quotes from a string (if any)
//===========================================================================

char* unquote(char *d, const char *s)
{
    int l = strlen(s);
    if (l >= 2 && (s[0] == '\"' || s[0] == '\'') && s[l-1] == s[0])
        s++, l-=2;
    return extract_string(d, s, l);
}

//===========================================================================
// Function: get_ext
// Purpose: get extension part of filename
// In:      filepath, extension to query
// Out:     * to extension or to terminating 0
//===========================================================================

const char *get_ext(const char *path)
{
    int nLen = strlen(path);
    int n = nLen;
    while (n) { if (path[--n] == '.') return path + n; }
    return path + nLen;
}

//===========================================================================
// Function: get_file
// Purpose:  get the pointer to the filename
// In:
// Out:
//===========================================================================

const char *get_file(const char *path)
{
    int nLen = strlen(path);
    while (nLen && !IS_SLASH(path[nLen-1])) nLen--;
    return path + nLen;
}

//===========================================================================
// Function: get_file
// Purpose:  get the pointer to the filename
// In:
// Out:
//===========================================================================

char *get_directory(char *buffer, const char *path)
{
    if (is_alpha(path[0]) && ':' == path[1] && IS_SLASH(path[2]))
    {
        const char *f = get_file(path);
        if (f > &path[3]) --f;
        return extract_string(buffer, path, f-path);
    }
    buffer[0] = 0;
    return buffer;
}

//===========================================================================
// Function: get_relative_path
// Purpose:  get the sub-path, if the path is in the blackbox folder,
// In:       path to check
// Out:      pointer to subpath or full path otherwise.
//===========================================================================

const char *get_relative_path(const char *p)
{
    char home[MAX_PATH];
    GetBlackboxPath(home, MAX_PATH);
    int n = strlen(home);
    if (0 == memicmp(p, home, n)) return p + n;
    return p;
}

//===========================================================================
// Function: is_relative_path
// Purpose:  check, if the path is relative
// In:
// Out:
//===========================================================================

bool is_relative_path(const char *path)
{
    if (IS_SLASH(path[0])) return false;
    if (strchr(path, ':')) return false;
    return true;
}

//===========================================================================

char *replace_slashes(char *buffer, const char *path)
{
    const char *p = path; char *b = buffer; char c;
    do *b++ = '/' == (c = *p++) ? '\\' : c; while (c);
    return buffer;
}


//===========================================================================
// Function: add_slash
// Purpose:  add \ when not present
//===========================================================================

char *add_slash(char *d, const char *s)
{
    int l; memcpy(d, s, l = strlen(s));
    if (l && !IS_SLASH(d[l-1])) d[l++] = '\\';
    d[l] = 0;
    return d;
}

//===========================================================================
// Function: make_bb_path
// Purpose:  add the blackbox path as default
// In:
// Out:
//===========================================================================

char *make_bb_path(char *dest, const char *src)
{
    dest[0]=0;
    if (is_relative_path(src))
        GetBlackboxPath(dest, MAX_PATH);
    return strcat(dest, src);
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

int get_substring_index (const char *key, const char **string_array)
{
    int i;
    for (i=0; *string_array; i++, string_array++)
        if (0==substr_icmp(key, *string_array)) return i;
    return -1;
}

//===========================================================================
// Function: get_string_index
// Purpose:  search for a match in a string array
// In:       searchstring, array
// Out:      index or -1
//===========================================================================

int get_string_index (const char *key, const char **string_array)
{
    const char **s;
    for (s = string_array; *s; ++s)
        if (0==stricmp(key, *s)) return s - string_array;
    return -1;
}

//===========================================================================

int get_false_true(const char *arg)
{
    if (arg)
    {
        if (0==stricmp(arg, "true")) return true;
        if (0==stricmp(arg, "false")) return false;
    } return -1;
}

const char *false_true_string(bool f)
{
    return f ? "true" : "false";
}

void set_bool(bool *v, const char *arg)
{
    int f = get_false_true(arg);
    *v = -1 == f ? false == *v : f;
}

//===========================================================================

char *replace_argument1(char *d, const char *s, const char *arg)
{
    char format[256];
    char *p = strstr(strcpy(format, s), "%1");
    if (p) p[1] = 's';
    sprintf(d, format, arg);
    return d;
}

//===========================================================================
// Function: arrow_bullet
// Purpose:  draw the triangle bullet
// In:       HDC, position x,y, direction -1 or 1
// Out:
//===========================================================================

void arrow_bullet (HDC buf, int x, int y, int d)
{
    int s = mStyle.bulletUnix ? 1 : 2; int e = d;

    if (Settings_arrowUnix)
        x-=d*s, d+=d, e = 0;
    else
        s++, x-=d*s/2, d = 0;

    for (int i=-s, j=0; i<=s; i++)
    {
        j+=d;
        MoveToEx(buf, x,   y+i, NULL);
        j+=e;
        LineTo  (buf, x+j, y+i);
        if (0==i) d=-d, e=-e;
    }
}


// //===========================================================================
// // Function: BitBltRect
// //===========================================================================
// void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r)
// {
//     BitBlt(
//         hdc_to,
//         r->left, r->top, r->right-r->left, r->bottom-r->top,
//         hdc_from,
//         r->left, r->top,
//         SRCCOPY
//         );
// }

//===========================================================================
// Function: get_fontheight
//===========================================================================
int get_fontheight(HFONT hFont)
{
    TEXTMETRIC TXM;
    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ other = SelectObject(hdc, hFont);
    GetTextMetrics(hdc, &TXM);
    SelectObject(hdc, other);
    DeleteDC(hdc);
    return TXM.tmHeight;
}

//===========================================================================

//===========================================================================
int get_filetime(const char *fn, FILETIME *ft)
{
    WIN32_FIND_DATA data_bb;
    HANDLE h = FindFirstFile(fn, &data_bb);
    if (INVALID_HANDLE_VALUE==h) return 0;
    FindClose(h);
    *ft = data_bb.ftLastWriteTime;
    return 1;
}

int check_filetime(const char *fn, FILETIME *ft0)
{
    FILETIME ft;
    return get_filetime(fn, &ft) == 0 || CompareFileTime(&ft, ft0) != 0;// > 0;
}

//===========================================================================
// logging support
//===========================================================================
static HANDLE hlog_file;

void log_printf(int flag, const char *fmt, ...)
{
    if ((Settings_LogFlag & flag) || 0 == (flag & 0x7FFF))
    {
        if (NULL == hlog_file)
        {
            char log_path[MAX_PATH];
            hlog_file = CreateFile(
                make_bb_path(log_path, "blackbox.log"),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

            SetFilePointer(hlog_file, 0, NULL, FILE_END);
            char date[32]; _strdate(date);
            char time[10]; _strtime(time);
            log_printf(flag, "\nStarting Log %s %s\n", date, time);
        }

        char buffer[4096]; buffer[0] = 0;
        if ('\n' != *fmt)
        {
            if (0 == (0x8000 & flag)) _strtime(buffer);
            strcat(buffer, "  ");
        }
        va_list arg; va_start(arg, fmt);
        vsprintf (buffer+strlen(buffer), fmt, arg);
        strcat(buffer, "\n");
        DWORD written; WriteFile(hlog_file, buffer, strlen(buffer), &written, NULL);
    }
}

void reset_logging(void)
{
    if (hlog_file && 0 == Settings_LogFlag)
        CloseHandle(hlog_file), hlog_file = NULL;
}

//===========================================================================

//===========================================================================
void dbg_printf (const char *fmt, ...)
{
    char buffer[4096];
    va_list arg; va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    strcat(buffer, "\n");
    OutputDebugString(buffer);
}

void dbg_window(HWND window, const char *fmt, ...)
{
    char buffer[4096];
    int x = GetClassName(window, buffer, sizeof buffer);
    x += sprintf(buffer+x, " <%lX>: ", (DWORD)window);
    va_list arg; va_start(arg, fmt);
    vsprintf (buffer+x, fmt, arg);
    strcat(buffer, "\n");
    OutputDebugString(buffer);
}

//===========================================================================

//===========================================================================
// API: GetAppByWindow
// Purpose:
// In:
// Out:
//===========================================================================
#include <tlhelp32.h>

// ToolHelp Function Pointers.
HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD,DWORD);
BOOL   (WINAPI *pModule32First)(HANDLE, LPMODULEENTRY32);
BOOL   (WINAPI *pModule32Next)(HANDLE, LPMODULEENTRY32);

// PSAPI Function Pointers.
DWORD  (WINAPI *pGetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD);
BOOL   (WINAPI *pEnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD);

int GetAppByWindow(HWND Window, LPSTR processName)
{
    processName[0]=0;
    DWORD pid;
    HANDLE hPr;

    GetWindowThreadProcessId(Window, &pid); // determine the process id of the window handle

    if (pCreateToolhelp32Snapshot && pModule32First && pModule32Next)
    {
        // grab all the modules associated with the process
        hPr = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
        if (hPr != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32 me;
            HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(Window, GWLP_HINSTANCE);

            me.dwSize = sizeof(me);
            if (pModule32First(hPr, &me))
            do
                if (me.hModule == hi)
                {
                    strcpy(processName, me.szModule);
                    break;
                }
            while (pModule32Next(hPr, &me));
            CloseHandle(hPr);
        }
    }
    else
    if (pGetModuleBaseName && pEnumProcessModules)
    {
        HMODULE hMod; DWORD cbNeeded;
        hPr = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
        if (hPr != NULL)
        {
            if(pEnumProcessModules(hPr, &hMod, sizeof(hMod), &cbNeeded))
            {
                pGetModuleBaseName(hPr, hMod, processName, MAX_PATH);
            }
            CloseHandle(hPr);
        }
    }

    // dbg_printf("appname = %s\n", processName);
    return strlen(processName);
}

//===========================================================================

//===========================================================================
// Function: EditBox
// Purpose: Display a single line editcontrol
// In:
// Out:
//===========================================================================
BOOL CALLBACK dlgproc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static char *buffer;
    switch( msg )
    {
    case WM_INITDIALOG:
        SetWindowText (hDlg, ((char**)lParam)[0]);
        SetDlgItemText(hDlg, 401, ((char**)lParam)[1]);
        SetDlgItemText(hDlg, 402, ((char**)lParam)[2]);
        buffer = ((char**)lParam)[3];
        MakeSticky(hDlg);
        {
            POINT p; GetCursorPos(&p);
            RECT m; GetMonitorRect(&p, &m, GETMON_WORKAREA|GETMON_FROM_POINT);
            RECT r; GetWindowRect(hDlg, &r);
    #if 0
            // at cursor
            r.right -= r.left; r.bottom -= r.top;
            p.x = iminmax(p.x - r.right / 2,  m.left, m.right - r.right);
            p.y = iminmax(p.y - 10,  m.top, m.bottom - r.bottom);
    #else
            // center screen
            p.x = (m.left + m.right - r.right + r.left) / 2;
            p.y = (m.top + m.bottom - r.bottom + r.top) / 2;
    #endif
            SetWindowPos(hDlg, NULL, p.x, p.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
        }
        return 1;

    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
        SetForegroundWindow(hDlg);
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ))
        {
        case IDOK:
            GetDlgItemText (hDlg, 402, buffer, 256);
        case IDCANCEL:
            RemoveSticky(hDlg);
            EndDialog(hDlg, LOWORD(wParam));
            return 1;
        }
        break;
    }
    return 0;
}

int EditBox(const char *caption, const char *message, const char *initvalue, char *buffer)
{
    return DialogBoxParam(
        NULL, MAKEINTRESOURCE(400), NULL, (DLGPROC)dlgproc, (LPARAM)&caption);
}

//===========================================================================
// Function:  SetOnTop
// Purpose:   bring a window on top in case it is visible and not on top anyway
//===========================================================================

void SetOnTop (HWND hwnd)
{
    if (hwnd && IsWindowVisible(hwnd) && !(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
        SetWindowPos(hwnd,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING
            );
}

//===========================================================================
// Function: exec_pidl
//===========================================================================

int exec_pidl(const _ITEMIDLIST *pidl, LPCSTR verb, LPCSTR arguments)
{
    char szFullName[MAX_PATH];
    if (NULL == verb && SHGetPathFromIDList(pidl, szFullName))
        return BBExecute_command(szFullName, arguments, false);

    SHELLEXECUTEINFO sei;
    ZeroMemory(&sei,sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = verb
        ? SEE_MASK_IDLIST | SEE_MASK_FLAG_NO_UI
        : SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI
        ;
    //sei.hwnd = NULL;
    sei.lpVerb = verb;
    //sei.lpFile = NULL;
    sei.lpParameters = arguments;
    //sei.lpDirectory = NULL;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpIDList = (void*)pidl;
    return ShellExecuteEx(&sei);
}

//===========================================================================
// Function:
// Purpose:
// In:
// Out:
//===========================================================================

#ifndef SHCNF_ACCEPT_INTERRUPTS
struct _SHChangeNotifyEntry
{
    const _ITEMIDLIST *pidl;
    BOOL fRecursive;
};
#define SHCNF_ACCEPT_INTERRUPTS 0x0001 
#define SHCNF_ACCEPT_NON_INTERRUPTS 0x0002
#define SHCNF_NO_PROXY 0x8000
#endif

#ifndef SHCNE_DISKEVENTS
#define SHCNE_DISKEVENTS    0x0002381FL
#define SHCNE_GLOBALEVENTS  0x0C0581E0L // Events that dont match pidls first
#define SHCNE_ALLEVENTS     0x7FFFFFFFL
#define SHCNE_INTERRUPT     0x80000000L // The presence of this flag indicates
#endif

extern UINT (WINAPI *pSHChangeNotifyRegister)(
    HWND hWnd, 
    DWORD dwFlags, 
    LONG wEventMask, 
    UINT uMsg, 
    DWORD cItems,
    struct _SHChangeNotifyEntry *lpItems
    );

extern BOOL (WINAPI *pSHChangeNotifyDeregister)(UINT ulID);

//===========================================================================
// Function:
// Purpose:
// In:
// Out:
//===========================================================================

UINT add_change_notify_entry(HWND hwnd, const _ITEMIDLIST *pidl)
{
    struct _SHChangeNotifyEntry E;
    E.pidl = pidl;
    E.fRecursive = FALSE;
    return pSHChangeNotifyRegister(
        hwnd,
        SHCNF_ACCEPT_INTERRUPTS|SHCNF_ACCEPT_NON_INTERRUPTS|SHCNF_NO_PROXY,
        SHCNE_ALLEVENTS,
        BB_FOLDERCHANGED,
        1,
        &E
        );
}

void remove_change_notify_entry(UINT id_notify)
{
    pSHChangeNotifyDeregister(id_notify);
}

//===========================================================================
// Function:
// Purpose:
// In:
// Out:
//===========================================================================

#ifdef BBOPT_SUPPORT_NLS

struct nls {
    struct nls *next;
    unsigned hash;
    int k;
    char *translation;
    char key[1];
};

static struct nls *pNLS;
bool nls_loaded;

void free_nls(void)
{
    struct nls *t;
    dolist (t, pNLS) free_str(&t->translation);
    freeall(&pNLS);
    nls_loaded = false;
}

static int decode_escape(char *d, const char *s)
{
    char c, e, *d0 = d; do
    {
        c = *s++;
        if ('\\' == c)
        {
            e = *s++;
            if ('n' == e) c = '\n';
            else
            if ('r' == e) c = '\r';
            else
            if ('t' == e) c = '\t';
            else
            if ('\"' == e || '\'' == e || '\\' == e) c = e;
            else --s;
        }
        *d++ = c;
    } while(c);
    return d - d0 - 1;
}

// -------------------------------------------

static void load_nls(void)
{
    const char *lang_file =
        ReadString(extensionsrcPath(), "blackbox.options.language:", NULL);
    if (NULL == lang_file) return;

    char full_path[MAX_PATH];
    FILE *fp = fopen (make_bb_path(full_path, lang_file), "rb");
    if (NULL == fp) return;

    char line[4000], key[200], new_text[4000], *np; int nl;
    key[0] = 0;

    new_text[0] = 0; np = new_text; nl = 0;
    for (;;)
    {
        bool eof = false == read_next_line(fp, line, sizeof line);
        char *s = line, c = *s;
        if ('$' == c || eof)
        {
            if (key[0] && new_text[0])
            {
                struct nls *t = (struct nls *)c_alloc(sizeof *t + strlen(key));
                t->hash = calc_hash(t->key, key, &t->k);
                t->translation = new_str(new_text);
                cons_node(&pNLS, t);
            }
            if (eof) break;
            if (' ' == s[1]) s += 2;
            decode_escape(key, s);
            new_text[0] = 0; np = new_text; nl = 0;
            continue;
        }

        if ('#' == c || '!' == c) continue;

        if ('\0' != c)
        {
            while (nl--) *np++ = '\n';
            np += decode_escape(np, s);
            nl = 0;
        }

        nl ++;
    }
    fclose(fp);
    reverse_list(&pNLS);
}

// -------------------------------------------
const char *nls2a(const char *i, const char *p)
{
    if (false == nls_loaded)
    {
        load_nls();
        nls_loaded = true;
    }

    if (pNLS)
    {
        char buffer[256]; int k; unsigned hash;
        hash = calc_hash(buffer, i, &k);
        struct nls *t;
        dolist (t, pNLS)
            if (t->hash==hash && 0==memcmp(buffer, t->key, 1+k))
                return t->translation;
    }
    return p;
}

const char *nls2b(const char *p)
{
    const char *e;
    if (*p != '$' || NULL == (e = strchr(p+1, *p)))
        return p;

    ++e;
    char buffer[256];
    return nls2a(extract_string(buffer, p, e-p), e);
}

const char *nls1(const char *p)
{
    return nls2a(p, p);
}

#endif

//===========================================================================
// Function: BBDrawText
// Purpose: draw text with shadow and/or outline
// In:
// Out:
//===========================================================================
int BBDrawText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, StyleItem* pSI){
    if (pSI->validated & VALID_OUTLINECOLOR){ // draw shadow with outline
        if (pSI->validated & VALID_SHADOWCOLOR){ // draw shadow
            SetTextColor(hDC, pSI->ShadowColor);
            _OffsetRect(lpRect,  2,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
            _OffsetRect(lpRect,  0,  1); DrawText(hDC, lpString, nCount, lpRect, uFormat);
            _OffsetRect(lpRect,  0,  1); DrawText(hDC, lpString, nCount, lpRect, uFormat);
            _OffsetRect(lpRect, -1,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
            _OffsetRect(lpRect, -1,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
            _OffsetRect(lpRect,  0, -2);
        }
        SetTextColor(hDC, pSI->OutlineColor);
        _OffsetRect(lpRect,  1,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect,  0,  1); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect, -1,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect, -1,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect,  0, -1); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect,  0, -1); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect,  2,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect, -1,  0); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect,  0,  1);
    }
    else if (pSI->validated & VALID_SHADOWCOLOR){ // draw shadow
        SetTextColor(hDC, pSI->ShadowColor);
        _OffsetRect(lpRect,  1,  1); DrawText(hDC, lpString, nCount, lpRect, uFormat);
        _OffsetRect(lpRect, -1, -1);
    }
    // draw text
    SetTextColor(hDC, pSI->TextColor);
    return DrawText(hDC, lpString, nCount, lpRect, uFormat);
}

//===========================================================================
// Function: GetIconFromHWND
// Purpose:
// In: WINDOW HANDLE
// Out: ICON HANDLE
//===========================================================================
HICON GetIconFromHWND(HWND hWnd){
	HICON hIcon = NULL;
	SendMessageTimeout(hWnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIcon);
	if (!hIcon){
		hIcon = (HICON)GetClassLong(hWnd, GCLP_HICONSM);
		if (!hIcon){
			SendMessageTimeout(hWnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIcon);
			if (!hIcon){
				hIcon = (HICON)GetClassLong(hWnd, GCLP_HICON);
			}
		}
	}
	return hIcon;
}

