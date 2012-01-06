/* ==========================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  ========================================================================== */

#include "BB.h"
#include "Settings.h"
#include "bbshell.h"
#include "bbrc.h"
#include <tlhelp32.h>

#define ST static

//===========================================================================
// API: Tokenize
// Purpose: Put first token of 'string' (seperated by one of delims)
// Returns: pointer to the rest into 'string'
//===========================================================================

const char* Tokenize(const char* string, char* buf, const char* delims)
{
    NextToken(buf, &string, delims);
    return string;
}

//===========================================================================
// API: BBTokenize
// Purpose: Assigns a specified number of string variables and outputs the
//      remaining to a variable
//===========================================================================

int BBTokenize (
    const char* srcString,
    char **lpszBuffers,
    unsigned dwNumBuffers,
    char* szExtraParameters)
{
    const char *s = srcString;
    int stored = 0;
    unsigned c;

    //dbg_printf("BBTokenize [%d] <%s>", dwNumBuffers, srcString);
    for (c = 0; c < dwNumBuffers; ++c)
    {
        const char *a; int n; char *out;
        n = nexttoken(&a, &s, NULL);
        if (n) {
            if (('\''  == a[0] || '\"' == a[0]) && n >= 2 && a[n-1] == a[0])
                ++a, n -= 2; /* remove quotes */
            ++stored;
        }
        out = lpszBuffers[c];
        extract_string(out, a, imin(n, MAX_PATH-1));
    }
    if (szExtraParameters)
        strcpy_max(szExtraParameters, s, MAX_PATH);
    return stored;
}

//===========================================================================
// API: StrRemoveEncap
// Purpose: Removes the first and last characters of a string
//===========================================================================

char* StrRemoveEncap(char* string)
{
    int l = strlen(string);
    if (l >= 2)
        extract_string(string, string+1, l-2);
    return string;
}

//===========================================================================
// API: IsInString
// Purpose: Checks a given string to an occurance of the second string
//===========================================================================

bool IsInString(const char* inputString, const char* searchString)
{
    // xoblite-flavour plugins bad version test workaround
#ifdef __BBCORE__
    if (0 == strcmp(searchString, "bb") && 0 == strcmp(inputString, GetBBVersion()))
        return false;
#endif
    return NULL != stristr(inputString, searchString);
}

//===========================================================================
ST void fn_write_error(const char *filename)
{
    BBMessageBox(MB_OK, NLS2("$Error_WriteFile$",
        "Error: Could not open \"%s\" for writing."), filename);
}

ST struct rcreader_init g_rc =
{
    NULL,               // struct fil_list *rc_files;
    fn_write_error,     // void (*write_error)(const char *filename);
    true,               // char dos_eol;
    true,               // char translate_065;
    0                   // char found_last_value;
};

void bb_rcreader_init(void)
{
    init_rcreader(&g_rc);
}

//===========================================================================
// API: ReadValue
const char* ReadValue(const char* path, const char* szKey, long *ptr)
{
    return read_value(path, szKey, ptr);
}

// API: FoundLastValue
int FoundLastValue(void)
{
    return found_last_value();
}

// API: WriteValue
void WriteValue(const char* path, const char* szKey, const char* value)
{
    write_value(path, szKey, value);
}

//===========================================================================
// API: RenameSetting
//===========================================================================

bool RenameSetting(const char* path, const char* szKey, const char* new_keyword)
{
    return 0 != rename_setting(path, szKey, new_keyword);
}

//===========================================================================
// API: DeleteSetting
//===========================================================================

bool DeleteSetting(LPCSTR path, LPCSTR szKey)
{
    return 0 != delete_setting(path, szKey);
}

//===========================================================================
// API: ReadBool
//===========================================================================

bool ReadBool(const char* fileName, const char* szKey, bool bDefault)
{
    const char* szValue = read_value(fileName, szKey, NULL);
    if (szValue) {
        if (!stricmp(szValue, "true"))
            return true;
        if (!stricmp(szValue, "false"))
            return false;
    }
    return bDefault;
}

//===========================================================================
// API: ReadInt
//===========================================================================

int ReadInt(const char* fileName, const char* szKey, int nDefault)
{
    const char* szValue = read_value(fileName, szKey, NULL);
    return szValue ? atoi(szValue) : nDefault;
}

//===========================================================================
// API: ReadString
//===========================================================================

const char* ReadString(const char* fileName, const char* szKey, const char* szDefault)
{
    const char* szValue = read_value(fileName, szKey, NULL);
    return szValue ? szValue : szDefault;
}

//===========================================================================
// API: ReadColor
//===========================================================================

COLORREF ReadColor(const char* fileName, const char* szKey, const char* defaultColor)
{
    const char* szValue = szKey[0] ? read_value(fileName, szKey, NULL) : NULL;
    return ReadColorFromString(szValue ? szValue : defaultColor);
}

//===========================================================================
// API: WriteBool
//===========================================================================

void WriteBool(const char* fileName, const char* szKey, bool value)
{
    write_value(fileName, szKey, value ? "true" : "false");
}

//===========================================================================
// API: WriteInt
//===========================================================================

void WriteInt(const char* fileName, const char* szKey, int value)
{
    char buff[32];
    write_value(fileName, szKey, itoa(value, buff, 10));
}

//===========================================================================
// API: WriteString
//===========================================================================

void WriteString(const char* fileName, const char* szKey, const char* value)
{
    write_value(fileName, szKey, value);
}

//===========================================================================
// API: WriteColor
//===========================================================================

void WriteColor(const char* fileName, const char* szKey, COLORREF value)
{
    char buff[32];
    sprintf(buff, "#%06lx", (unsigned long)switch_rgb (value));
    write_value(fileName, szKey, buff);
}

//===========================================================================
// API: ParseItem
// Purpose: parses a given string and assigns settings to a StyleItem class
//===========================================================================

void ParseItem(const char* szItem, StyleItem *item)
{
    parse_item(szItem, item);
}

//===========================================================================
// API: FileExists
// Purpose: Checks for a files existance
//===========================================================================

bool FileExists(const char* szFileName)
{
    DWORD a = GetFileAttributes(szFileName);
    return (DWORD)-1 != a && 0 == (a & FILE_ATTRIBUTE_DIRECTORY);
}

//===========================================================================
// API: FileOpen
// Purpose: Opens file for parsing
//===========================================================================

FILE *FileOpen(const char* szPath)
{
#ifdef __BBCORE__
    // hack to prevent BBSlit from loading plugins, since they are
    // loaded by the built-in PluginManager.
    if (0 == strcmp(szPath, pluginrc_path))
        return NULL;
#endif
    return fopen(szPath, "rt");
}

//===========================================================================
// API: FileClose
// Purpose: Close selected file
//===========================================================================

bool FileClose(FILE *fp)
{
    return fp && 0 == fclose(fp);
}

//===========================================================================
// API: FileRead
// Purpose: Read's a line from given FILE and returns boolean on status
//===========================================================================

bool FileRead(FILE *fp, char* buffer)
{
    return 0 != read_next_line(fp, buffer, MAX_LINE_LENGTH);
}

//===========================================================================
// API: ReadNextCommand
// Purpose: Reads the next line of the file
//===========================================================================

bool ReadNextCommand(FILE *fp, char* szBuffer, unsigned dwLength)
{
    while (read_next_line(fp, szBuffer, dwLength)) {
        char c = szBuffer[0];
        if (c && '#' != c && '!' != c)
            return true;
    }
    return false;
}

//===========================================================================
// API: ReplaceEnvVars
// Purpose: parses a given string and replaces all %VAR% with the environment
//          variable value if such a value exists
//===========================================================================

void ReplaceEnvVars(char* string)
{
    replace_environment_strings(string, MAX_LINE_LENGTH);
}

//===========================================================================
// API: ReplaceShellFolders
// Purpose: replace shell folders in a string path, like DESKTOP\...
//===========================================================================

char* ReplaceShellFolders(char* string)
{
    return replace_shellfolders(string, string, false);
}

//===========================================================================

//===========================================================================
ST bool find_resource_file(char* pszOut, const char* filename, const char* basedir)
{
    char temp[MAX_PATH];

    if (defaultrc_path[0]) {
        join_path(pszOut,  defaultrc_path, file_basename(filename));
        if (FileExists(pszOut))
            return true;
    }
#ifndef BBTINY
    sprintf(temp, "APPDATA\\blackbox\\%s", file_basename(filename));
    if (FileExists(replace_shellfolders(pszOut, temp, false)))
        return true;
#endif
    if (is_absolute_path(filename))
        return FileExists(strcpy(pszOut, filename));

    if (basedir) {
        join_path(pszOut, basedir, filename);
        // save as default for below
        strcpy(temp, pszOut);
        if (FileExists(pszOut))
            return true;
    }
    replace_shellfolders(pszOut, filename, false);
    if (FileExists(pszOut))
        return true;
    if (basedir)
        strcpy(pszOut, temp);
    return false;
}

//===========================================================================
// API: FindRCFile
// Purpose: Lookup a configuration file
// In:  pszOut = the location where to put the result
// In:  const char* = filename to look for
// In:  HINSTANCE = plugin module handle or NULL
// Out: bool = true of found, FALSE otherwise
//===========================================================================

bool FindRCFile(char* pszOut, const char* filename, HINSTANCE module)
{
    char basedir[MAX_PATH];
    char file_rc[MAX_PATH];
    char temp[MAX_PATH];
    char *e;
    bool ret;

    if (NULL == module || 0 == GetModuleFileName(module, temp, MAX_PATH)) {
        ret = find_resource_file(pszOut, filename, NULL);

    } else {
        if (have_imp(pGetLongPathName))
            pGetLongPathName(temp, temp, MAX_PATH);
        file_directory(basedir, temp);
        strcpy(file_rc, filename);

        e = (char*)file_extension(file_rc);
        if (*e) {
            // file has extension
            ret = find_resource_file(pszOut, file_rc, basedir);
        } else {
            strcpy(e, ".rc"); // try file.rc
            ret = find_resource_file(pszOut, file_rc, basedir);
            if (!ret) {
                strcpy(temp, pszOut);
                strcpy(e, "rc"); // try filerc
                ret = find_resource_file(pszOut, file_rc, basedir);
                if (!ret) {
                    strcpy(pszOut, temp); // use file.rc as default
                }
            }
        }
    }
    //dbg_printf("FindRCFile (%d) %s -> %s", ret, filename, pszOut);
    return ret;
}

//===========================================================================
// API: ConfigFileExists
//===========================================================================

EXTERN_C API_EXPORT
const char* ConfigFileExists(const char* filename, const char* pluginDir)
{
    static char tempBuf[MAX_PATH];
    if (false == find_resource_file(tempBuf, filename, pluginDir))
        tempBuf[0] = 0;
    return tempBuf;
}

//===========================================================================
// API: Log
// Purpose: Appends given line to Blackbox.log file
//===========================================================================

void Log(const char* Title, const char* Line)
{
    log_printf((LOG_PLUGINS, "%s: %s", Title, Line));
}

//===========================================================================
// API: MBoxErrorFile
// Purpose: Gives a message box proclaming missing file
//===========================================================================

int MBoxErrorFile(const char* szFile)
{
    return BBMessageBox(MB_OK, NLS2("$Error_ReadFile$",
        "Error: Unable to open file \"%s\"."
        "\nPlease check location and try again."
        ), szFile);
}

//===========================================================================
// API: MBoxErrorValue
// Purpose: Gives a message box proclaming a given value
//===========================================================================

int MBoxErrorValue(const char* szValue)
{
    return BBMessageBox(MB_OK, NLS2("$Error_MBox$", "Error: %s"), szValue);
}

//===========================================================================
// API: BBExecute
// Purpose: A safe execute routine
// In: HWND = owner
// In: const char* = operation (eg. "open")
// In: const char* = command to run
// In: const char* = arguments
// In: const char* = directory to run from
// In: int = show status
// In: bool = suppress error messages
// Out: BOOL TRUE on success
//===========================================================================

#ifndef BBTINY
BOOL BBExecute(
    HWND Owner,
    const char* szVerb,
    const char* szFile,
    const char* szArgs,
    const char* szDirectory,
    int nShowCmd,
    int flags)
{
    SHELLEXECUTEINFO sei;
    char workdir[MAX_PATH];

    if (NULL == szDirectory || 0 == szDirectory[0])
        szDirectory = GetBlackboxPath(workdir, sizeof workdir);

    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = Owner;
    sei.lpVerb = szVerb;
    sei.lpParameters = szArgs;
    sei.lpDirectory = szDirectory;
    sei.nShow = nShowCmd;

    if (flags & RUN_ISPIDL) {
        sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
        sei.lpIDList = (void*)szFile;
    } else {
        sei.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI;
        sei.lpFile = szFile;
        if (NULL == szFile || 0 == szFile[0])
            goto skip;
    }

    if (ShellExecuteEx(&sei))
        return TRUE;

skip:
    if (0 == (flags & RUN_NOERRORS)) {
        char msg[200];
        BBMessageBox(MB_OK, NLS2("$Error_Execute$",
            "Error: Could not execute: %s\n(%s)"),
            szFile && szFile[0] ? szFile : NLS1("<empty>"),
            win_error(msg, sizeof msg));
    }

    return FALSE;
}
#endif

//===========================================================================

BOOL BBExecute_string(const char *line, int flags)
{
    char workdir[MAX_PATH];
    char file[MAX_PATH];
    const char *cmd, *args;
    char *cmd_temp = NULL;
    char *line_temp = NULL;
    int n, ret;

    workdir[0] = 0;
    if (flags & RUN_WINDIR)
        GetWindowsDirectory(workdir, sizeof workdir);
    else
        GetBlackboxPath(workdir, sizeof workdir);

    if (0 == (flags & RUN_NOSUBST))
        line = replace_environment_strings_alloc(&line_temp, line);

    if (flags & RUN_NOARGS) {
        cmd = line;
        args = NULL;
        strcpy(file, cmd);

    } else {
        for (args = line;; args += args[0] == ':')
        {
            cmd = args;
            NextToken(file, &args, NULL);
            if (file[0] != '-')
                break;

            // -hidden : run in a hidden window
            if (0 == strcmp(file+1, "hidden")) {
                flags |= RUN_HIDDEN;
                continue;
            }

            // -in <path> ; specify working directory
            if (0 == strcmp(file+1, "in")) {
                NextToken(file, &args, NULL);
                replace_shellfolders(workdir, file, true);
                continue;
            }

            // -workspace1 : specify workspace
            if (-1 != (n = get_workspace_number(file+1))) {
                SendMessage(BBhwnd, BB_SWITCHTON, 0, n);
                BBSleep(10);
                continue;
            }
            break;
        }
        if (0 == args[0])
            args = NULL;
    }

    if (0 == (flags & RUN_NOSUBST)) {
        n = '\"' == file[0];
        cmd_temp = (char*)m_alloc((args ? strlen(args) : 0) + MAX_PATH + 10);
        strcpy(cmd_temp, replace_shellfolders(file, file, true));
        if (n)
            quote_path(cmd_temp);
        if (args)
            sprintf(strchr(cmd_temp, 0), " %s", args);
        cmd = cmd_temp;
    }

    ret = -1 != run_process(cmd, workdir, flags);
    if (ret) {
        // dbg_printf("cmd (%d) <%s>", ret, cmd);
    } else {
        ret = BBExecute(NULL, NULL, file, args, workdir,
            flags & RUN_HIDDEN ? SW_HIDE : SW_SHOWNORMAL,
            flags & RUN_NOERRORS);
        // dbg_printf("exec (%d) <%s> <%s>", ret, file, args);
    }

    free_str(&cmd_temp);
    free_str(&line_temp);
    return ret;
}

//===========================================================================
// Function: BBExecute_pidl
//===========================================================================

int BBExecute_pidl(const char* verb, const void *pidl)
{
    if (NULL == pidl)
        return FALSE;

    return BBExecute(NULL, verb, (const char*)pidl,
        NULL, NULL, SW_SHOWNORMAL, RUN_ISPIDL|RUN_NOERRORS);
}

//===========================================================================
// API: IsAppWindow
// Purpose: checks given hwnd to see if it's an app
// This is used to populate the task list in case bb is started manually.
//===========================================================================

bool IsAppWindow(HWND hwnd)
{
    HWND hParent;

    if (!IsWindow(hwnd))
        return false;
    
    // if it is a WS_CHILD or not WS_VISIBLE, fail it
    if ((GetWindowLong(hwnd, GWL_STYLE)
            & (WS_CHILD|WS_VISIBLE|WS_DISABLED))
        != WS_VISIBLE)
        return false;

    // if the window is a WS_EX_TOOLWINDOW fail it
    if ((GetWindowLong(hwnd, GWL_EXSTYLE)
            & (WS_EX_TOOLWINDOW|WS_EX_APPWINDOW))
        == WS_EX_TOOLWINDOW)
        return false;

    // If this has a parent, then only accept this window
    // if the parent is not accepted
    hParent = GetParent(hwnd);
    if (NULL == hParent)
        hParent = GetWindow(hwnd, GW_OWNER);

    if (hParent && IsAppWindow(hParent))
        return false;

    if (0 == GetWindowTextLength(hwnd))
        return false;

    return true;
}

//===========================================================================
// API: SetTransparency
// Purpose: Wrapper, win9x conpatible
//===========================================================================

BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);

bool SetTransparency(HWND hwnd, BYTE alpha)
{
    LONG wStyle1, wStyle2;

    //dbg_window(hwnd, "alpha %d", alpha);
    if (!have_imp(pSetLayeredWindowAttributes))
        return false;

    wStyle1 = wStyle2 = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (alpha < 255)
        wStyle2 |= WS_EX_LAYERED;
    else
        wStyle2 &= ~WS_EX_LAYERED;

    if (wStyle2 != wStyle1)
        SetWindowLong(hwnd, GWL_EXSTYLE, wStyle2);

    if (wStyle2 & WS_EX_LAYERED)
        return 0 != pSetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);

    return true;
}

//*****************************************************************************

//*****************************************************************************
// multimon api, win 9x compatible

HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
BOOL     (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);
BOOL     (WINAPI* pEnumDisplayMonitors)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);

ST void get_mon_rect(HMONITOR hMon, RECT *s, RECT *w)
{
    if (hMon) {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (pGetMonitorInfoA(hMon, &mi)) {
            if (w) *w = mi.rcWork;
            if (s) *s = mi.rcMonitor;
            return;
        }
    }
    if (w) {
        SystemParametersInfo(SPI_GETWORKAREA, 0, w, 0);
    }
    if (s) {
        s->top = s->left = 0;
        s->right = GetSystemMetrics(SM_CXSCREEN);
        s->bottom = GetSystemMetrics(SM_CYSCREEN);
    }
}

//===========================================================================
// API: GetMonitorRect
//===========================================================================

/* Flags: */
#define GETMON_FROM_WINDOW 1    /* 'from' is HWND */
#define GETMON_FROM_POINT 2     /* 'from' is POINT* */
#define GETMON_FROM_MONITOR 4   /* 'from' is HMONITOR */
#define GETMON_WORKAREA 16      /* get working area rather than full screen */

HMONITOR GetMonitorRect(void *from, RECT *r, int flags)
{
    HMONITOR hMon = NULL;
    if (multimon && from)
        switch (flags & ~GETMON_WORKAREA) {
        case GETMON_FROM_WINDOW:
            hMon = pMonitorFromWindow((HWND)from, MONITOR_DEFAULTTONEAREST);
            break;
        case GETMON_FROM_POINT:
            hMon = pMonitorFromPoint(*(POINT*)from, MONITOR_DEFAULTTONEAREST);
            break;
        case GETMON_FROM_MONITOR:
            hMon = (HMONITOR)from;
            break;
        }
    if (flags & GETMON_WORKAREA)
        get_mon_rect(hMon, NULL, r);
    else
        get_mon_rect(hMon, r, NULL);

    return hMon;
}

//===========================================================================
// API: SetDesktopMargin
// Purpose:  Set a margin for e.g. toolbar, bbsystembar, etc
// In:       hwnd to associate the margin with, location, margin-width
//===========================================================================

struct dt_margins
{
    struct dt_margins *next;
    HWND hwnd;
    HMONITOR hmon;
    int location;
    int margin;
};

void update_screen_areas(struct dt_margins *dt_margins);

void SetDesktopMargin(HWND hwnd, int location, int margin)
{
    //dbg_printf("SDTM: %08x %d %d (%x)", hwnd, location, margin, pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST));

    static struct dt_margins *margin_list;
    struct dt_margins *p;

    if (BB_DM_RESET == location || Settings_disableMargins) {
        // reset everything
        if (NULL == margin_list)
            return;
        freeall(&margin_list);
    }
    else
    if (BB_DM_REFRESH == location) {
        // re-validate margins
        for (p = margin_list; p;)
        {
            struct dt_margins *n = p->next;
            if (FALSE == IsWindow(p->hwnd))
                remove_item(&margin_list, p);
            p = n;
        }
    }
    else
    if (hwnd) {
        // search for hwnd:
        p = (struct dt_margins *)assoc(margin_list, hwnd);
        if (margin) {
            if (NULL == p) { // insert a _new structure
                p = c_new(struct dt_margins);
                cons_node (&margin_list, p);
                p->hwnd = hwnd;
            }
            p->location = location;
            p->margin = margin;
            if (multimon)
                p->hmon = pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        } else {
            if (p)
                remove_item(&margin_list, p);
        }
    }

    update_screen_areas(margin_list);
}

//===========================================================================

struct _screen
{
    struct _screen *next;   /* this must come first */
    HMONITOR hMon;          /* this must be the second member */
    int index;
    RECT screen_rect;
    RECT work_rect;
    RECT new_rect;
    RECT custom_margins;
};

struct _screen_list
{
    struct _screen *pScr;
    struct _screen **ppScr;
    int index;
};

void get_custom_margin(RECT *pcm, int screen)
{
    char key[40];
    int x, i;
    static const char * const edges[4] = { "Left", "Top", "Right", "Bottom" };

    if (0 == screen)
        x = sprintf(key, "blackbox.desktop.margin");
    else
        x = sprintf(key, "blackbox.desktop.%d.margin", 1+screen);

    i = 0;
    do {
        strcpy(key+x, edges[i]);
        (&pcm->left)[i] = ReadInt(extensionsrcPath(NULL), key, -1);
    } while (++i < 4);
}

ST BOOL CALLBACK fnEnumMonProc(HMONITOR hMon, HDC hdcOptional, RECT *prcLimit, LPARAM dwData)
{
    struct _screen * s;
    struct _screen_list *i;
    int screen;

    s = c_new(struct _screen);
    s->hMon = hMon;
    get_mon_rect(hMon, &s->screen_rect, &s->work_rect);
    s->new_rect = s->screen_rect;

    i = (struct _screen_list *)dwData;
    *i->ppScr = s;
    i->ppScr = &s->next;

    screen = s->index = i->index++;
    if (0 == screen)
        s->custom_margins = Settings_desktopMargin;
    else
        get_custom_margin(&s->custom_margins, screen);

    // dbg_printf("EnumProc %x %d %d %d %d", hMon, s->screen_rect.left, s->screen_rect.top, s->screen_rect.right, s->screen_rect.bottom);
    return TRUE;
}

void update_screen_areas(struct dt_margins *dt_margins)
{
    struct _screen_list si;
    struct _screen *pS;

    si.pScr = NULL;
    si.ppScr = &si.pScr;
    si.index = 0;

    if (multimon)
        pEnumDisplayMonitors(NULL, NULL, fnEnumMonProc, (LPARAM)&si);
    else
        fnEnumMonProc(NULL, NULL, NULL, (LPARAM)&si);

    //dbg_printf("list: %d %d", listlen(si.pScr), si.index);
    if (false == Settings_fullMaximization) {
        struct dt_margins *p;
        // first loop through the set margins from plugins
        dolist (p, dt_margins) {
            pS = (struct _screen *)assoc(si.pScr, p->hmon); // get screen for this window
            if (pS) {
                RECT *n = &pS->new_rect;
                RECT *s = &pS->screen_rect;
                switch (p->location)
                {
                    case BB_DM_LEFT   : n->left     = imax(n->left  , s->left   + p->margin); break;
                    case BB_DM_TOP    : n->top      = imax(n->top   , s->top    + p->margin); break;
                    case BB_DM_RIGHT  : n->right    = imin(n->right , s->right  - p->margin); break;
                    case BB_DM_BOTTOM : n->bottom   = imin(n->bottom, s->bottom - p->margin); break;
                }
            }
        }
        // these may be overridden by fixed custom margins from extensions rc
        dolist (pS, si.pScr) {
            RECT *n = &pS->new_rect;
            RECT *s = &pS->screen_rect;
            RECT *m = &pS->custom_margins;
            if (-1 != m->left)     n->left     = s->left     + m->left    ;
            if (-1 != m->top)      n->top      = s->top      + m->top     ;
            if (-1 != m->right)    n->right    = s->right    - m->right   ;
            if (-1 != m->bottom)   n->bottom   = s->bottom   - m->bottom  ;
        }
    }

    // finally set the new margins, if changed
    dolist (pS, si.pScr) {
        RECT *n = &pS->new_rect;
        if (0 != memcmp(&pS->work_rect, n, sizeof(RECT))) {
            // dbg_printf("WA = %d %d %d %d", n->left, n->top, n->right, n->bottom);
            SystemParametersInfo(SPI_SETWORKAREA, 0, (PVOID)n, 0);
        }
    }

    freeall(&si.pScr);
}

//===========================================================================
// API: SnapWindowToEdge
// Purpose:Snaps a given windowpos at a specified distance
// In: WINDOWPOS* = WINDOWPOS recieved from WM_WINDOWPOSCHANGING
// In: int = distance to snap to
// In: bool = use screensize of workspace
//===========================================================================

/* Public flags for SnapWindowToEdge */
#define SNAP_FULLSCREEN 1  /* use full screen rather than workarea */
#define SNAP_NOPLUGINS  2 /* dont snap to other plugins */
#define SNAP_SIZING     4 /* window is resized (bottom-right sizing only) */

/* Private flags for SnapWindowToEdge */
#define SNAP_NOPARENT   8  /* dont snap to parent window edges */
#define SNAP_NOCHILDS  16  /* dont snap to child windows */
#define SNAP_TRIGGER   32  /* apply nTriggerDist instead of default */
#define SNAP_PADDING   64  /* Next arg points to the padding value */
#define SNAP_CONTENT  128  /* Next arg points to a SIZE struct */

// Structures
struct edges { int from1, from2, to1, to2, dmin, omin, d, o, def; };

struct snap_info { struct edges *h; struct edges *v;
    bool sizing; bool same_level; int pad; HWND self; HWND parent; };

// Local fuctions
//ST void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad);
ST void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad);
ST BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam);

void SnapWindowToEdge(WINDOWPOS* wp, LPARAM nDist, UINT flags, ...)
{
    struct edges h;
    struct edges v;
    struct snap_info si;
    int snapdist, padding;
    bool snap_plugins, sizing, snap_workarea;
    HWND self, parent;
    RECT r;
    va_list va;
    //int grid = 0;

    va_start(va, flags);
    snapdist = Settings_snapThreshold;
    padding = Settings_snapPadding;
    snap_plugins = Settings_snapPlugins;
    sizing = 0 != (flags & SNAP_SIZING);
    snap_workarea = 0 == (flags & SNAP_FULLSCREEN);
    if (flags & SNAP_NOPLUGINS)
        snap_plugins = false;
    if (flags & SNAP_TRIGGER)
        snapdist = nDist;
    if (flags & SNAP_PADDING)
        padding = va_arg(va, int);
    if (snapdist < 1)
        return;

    self = wp->hwnd;
    parent = NULL;

    if (WS_CHILD & GetWindowLong(self, GWL_STYLE))
        parent = GetParent(self);

    // ------------------------------------------------------
    // well, why is this here? Because some plugins call this
    // even if they reposition themselves rather than being
    // moved by the user.
    {
        static bool capture;
        if (GetCapture() == self)
            capture = true;
        else if (capture)
            capture = false;
        else
            return;
    }

    // ------------------------------------------------------

    si.h = &h, si.v = &v, si.sizing = sizing, si.same_level = true,
    si.pad = padding, si.self = self, si.parent = parent;

    h.dmin = v.dmin = h.def = v.def = snapdist;
    h.from1 = wp->x;
    h.from2 = h.from1 + wp->cx;
    v.from1 = wp->y;
    v.from2 = v.from1 + wp->cy;

    // ------------------------------------------------------
    // snap to grid

    /*if (grid > 1 && (parent || sizing))
    {
        snap_to_grid(&h, &v, sizing, grid, padding);
    }*/
    //else
    {
        // -----------------------------------------
        if (parent) {

            // snap to siblings
            EnumChildWindows(parent, SnapEnumProc, (LPARAM)&si);

            if (0 == (flags & SNAP_NOPARENT)) {
                // snap to frame edges
                GetClientRect(parent, &r);
                h.to1 = r.left;
                h.to2 = r.right;
                v.to1 = r.top;
                v.to2 = r.bottom;
                snap_to_edge(&h, &v, sizing, false, padding);
            }
        } else {

            // snap to top level windows
            if (snap_plugins)
                EnumThreadWindows(GetCurrentThreadId(), SnapEnumProc, (LPARAM)&si);

            // snap to screen edges
            GetMonitorRect(self, &r, snap_workarea ?
                GETMON_WORKAREA|GETMON_FROM_WINDOW : GETMON_FROM_WINDOW);
            h.to1 = r.left;
            h.to2 = r.right;
            v.to1 = r.top;
            v.to2 = r.bottom;
            snap_to_edge(&h, &v, sizing, false, 0);
        }

        // -----------------------------------------
        if (sizing) {
            if (flags & SNAP_CONTENT) { // snap to button icons
                SIZE * psize = va_arg(va, SIZE*);
                h.to2 = (h.to1 = h.from1) + psize->cx;
                v.to2 = (v.to1 = v.from1) + psize->cy;
                snap_to_edge(&h, &v, sizing, false, -2*padding);
            }

            if (0 == (flags & SNAP_NOCHILDS)) { // snap frame to childs
                si.same_level = false;
                si.pad = -padding;
                si.self = NULL;
                si.parent = self;
                EnumChildWindows(self, SnapEnumProc, (LPARAM)&si);
            }
        }
    }

    // -----------------------------------------
    // adjust the window-pos

    if (h.dmin < snapdist) {
        if (sizing)
            wp->cx += h.omin;
        else
            wp->x += h.omin;
    }

    if (v.dmin < snapdist) {
        if (sizing)
            wp->cy += v.omin;
        else
            wp->y += v.omin;
    }
}

//*****************************************************************************

ST BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam)
{
    struct snap_info *si = (struct snap_info *)lParam;
    LONG style = GetWindowLong(hwnd, GWL_STYLE);

    if (hwnd != si->self && (style & WS_VISIBLE))
    {
        HWND pw = (style & WS_CHILD) ? GetParent(hwnd) : NULL;
        if (pw == si->parent && false == Menu_IsA(hwnd))
        {
            RECT r;

            GetWindowRect(hwnd, &r);
            r.right -= r.left;
            r.bottom -= r.top;

            if (pw)
                ScreenToClient(pw, (POINT*)&r.left);

            if (false == si->same_level)
            {
                r.left += si->h->from1;
                r.top  += si->v->from1;
            }
            si->h->to2 = (si->h->to1 = r.left) + r.right;
            si->v->to2 = (si->v->to1 = r.top)  + r.bottom;
            snap_to_edge(si->h, si->v, si->sizing, si->same_level, si->pad);
        }
    }
    return TRUE;
}   

//*****************************************************************************
/*
ST void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad)
{
    for (struct edges *g = h;;g = v)
    {
        int o, d;
        if (sizing) o = g->from2 - g->from1 + pad; // relative to topleft
        else        o = g->from1 - pad; // absolute coords

        o = o % grid;
        if (o < 0) o += grid;

        if (o >= grid / 2)
            d = o = grid-o;
        else
            d = o, o = -o;

        if (d < g->dmin) g->dmin = d, g->omin = o;

        if (g == v) break;
    }
}
*/
//*****************************************************************************
ST void snap_to_edge(
    struct edges *h,
    struct edges *v,
    bool sizing,
    bool same_level,
    int pad)
{
    int o, d, n; struct edges *t;
    h->d = h->def; v->d = v->def;
    for (n = 2;;) // v- and h-edge
    {
        // see if there is any common edge,
        // i.e if the lower top is above the upper bottom.
        if ((v->to2 < v->from2 ? v->to2 : v->from2)
            >= (v->to1 > v->from1 ? v->to1 : v->from1))
        {
            if (same_level) // child to child
            {
                //snap to the opposite edge, with some padding between
                bool f = false;

                d = o = (h->to2 + pad) - h->from1;  // left edge
                if (d < 0) d = -d;
                if (d <= h->d)
                {
                    if (false == sizing)
                        if (d < h->d) h->d = d, h->o = o;
                    if (d < h->def) f = true;
                }

                d = o = h->to1 - (h->from2 + pad); // right edge
                if (d < 0) d = -d;
                if (d <= h->d)
                {
                    if (d < h->d) h->d = d, h->o = o;
                    if (d < h->def) f = true;
                }

                if (f)
                {
                    // if it's near, snap to the corner
                    if (false == sizing)
                    {
                        d = o = v->to1 - v->from1;  // top corner
                        if (d < 0) d = -d;
                        if (d < v->d) v->d = d, v->o = o;
                    }
                    d = o = v->to2 - v->from2;  // bottom corner
                    if (d < 0) d = -d;
                    if (d < v->d) v->d = d, v->o = o;
                }
            }
            else // child to frame
            {
                //snap to the same edge, with some bevel between
                if (false == sizing)
                {
                    d = o = h->to1 - (h->from1 - pad); // left edge
                    if (d < 0) d = -d;
                    if (d < h->d) h->d = d, h->o = o;
                }
                d = o = h->to2 - (h->from2 + pad); // right edge
                if (d < 0) d = -d;
                if (d < h->d) h->d = d, h->o = o;
            }
        }
        if (0 == --n) break;
        t = h; h = v, v = t;
    }

    if (false == sizing)// && false == same_level)
    {
        // snap to center
        for (n = 2;;) // v- and h-edge
        {
            if (v->d < v->dmin)
            {
                d = o = (h->to1 + h->to2)/2 - (h->from1 + h->from2)/2;
                if (d < 0) d = -d;
                if (d < h->d) h->d = d, h->o = o;
            }
            if (0 == --n) break;
            t = h; h = v, v = t;
        }
    }

    if (h->d < h->dmin) h->dmin = h->d, h->omin = h->o;
    if (v->d < v->dmin) v->dmin = v->d, v->omin = v->o;
}

//===========================================================================
// API: GetAppByWindow
// Purpose:
// In:
// Out:
//===========================================================================

// ToolHelp Function Pointers.
HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD,DWORD);
BOOL   (WINAPI *pModule32First)(HANDLE, LPMODULEENTRY32);
BOOL   (WINAPI *pModule32Next)(HANDLE, LPMODULEENTRY32);

// PSAPI Function Pointers.
DWORD  (WINAPI *pGetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD);
BOOL   (WINAPI *pEnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD);

int GetAppByWindow(HWND hwnd, char* processName)
{
    DWORD pid;
    HANDLE hPr;
    HMODULE hMod;
    DWORD cbNeeded;

    processName[0]=0;

    // determine the process id of the window handle
    GetWindowThreadProcessId(hwnd, &pid);

    if (have_imp(pGetModuleBaseName)
     && have_imp(pEnumProcessModules)) {
        hPr = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
        if (hPr) {
            if(pEnumProcessModules(hPr, &hMod, sizeof(hMod), &cbNeeded)) {
                pGetModuleBaseName(hPr, hMod, processName, MAX_PATH);
            }
            CloseHandle(hPr);
        }
    }
    else
    if (have_imp(pCreateToolhelp32Snapshot)
     && have_imp(pModule32First)
     && have_imp(pModule32Next)) {
        // grab all the modules associated with the process
        hPr = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
        // hMod = (HMODULE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        if (hPr != INVALID_HANDLE_VALUE)
        {
            DWORD_PTR base = (DWORD_PTR)-1;
            MODULEENTRY32 me;
            me.dwSize = sizeof(me);
            if (pModule32First(hPr, &me)) {
                do {
                    if ((DWORD_PTR)me.modBaseAddr < base)
                    {
                        strcpy(processName, me.szModule);
                        base = (DWORD_PTR)me.modBaseAddr;
                        //if (hMod == me.hModule)
                        if (base <= 0x00400000)
                            break;
                    }
                } while (pModule32Next(hPr, &me));
            }
            strlwr(processName);
            CloseHandle(hPr);
        }
    }

    // dbg_printf("appname = %s\n", processName);
    return strlen(processName);
}

//===========================================================================
