/* ------------------------------------------------------------------------- */
/*
  This file is part of the bbLean source code
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
*/
/* ------------------------------------------------------------------------- */
/* windows functions */

#include "bblib.h"
#include "win0x500.h"

void dbg_printf (const char *fmt, ...)
{
    char buffer[4000];
    va_list arg;
    int x;
    va_start(arg, fmt);
    x = vsprintf (buffer, fmt, arg);
    strcpy(buffer+x, "\n");
    OutputDebugString(buffer);
}

void dbg_window(HWND hwnd, const char *fmt, ...)
{
    char buffer[4000];
    int x;
    va_list arg;
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    x = GetClassName(hwnd, buffer, 80);
    x += sprintf(buffer+x, " hwnd:%lx pid:%ld ", (DWORD)(DWORD_PTR)hwnd, pid);
    va_start(arg, fmt);
    vsprintf (buffer+x, fmt, arg);
    strcat(buffer, "\n");
    OutputDebugString(buffer);
}

int _load_imp(void *pp, const char *dll, const char *proc)
{
    HMODULE hm = GetModuleHandle(dll);
    if (NULL == hm)
        hm = LoadLibrary(dll);
    if (hm)
        *(FARPROC*)pp = GetProcAddress(hm, proc);
    return 0 != *(DWORD_PTR*)pp;
}

int load_imp(void *pp, const char *dll, const char *proc)
{
    if (0 == *(DWORD_PTR*)pp && !_load_imp(pp, dll, proc))
        *(DWORD_PTR*)pp = 1;
    return have_imp(*(void**)pp);
}

void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r)
{
    BitBlt(
        hdc_to,
        r->left, r->top, r->right-r->left, r->bottom-r->top,
        hdc_from,
        r->left, r->top,
        SRCCOPY
        );
}

HWND GetRootWindow(HWND hwnd)
{
    HWND pw; HWND dw = GetDesktopWindow();
    while (NULL != (pw = GetParent(hwnd)) && dw != pw)
        hwnd = pw;
    return hwnd;
}

int is_bbwindow(HWND hwnd)
{
    return GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId();
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

int get_filetime(const char *fn, FILETIME *ft)
{
    WIN32_FIND_DATA data_bb;
    HANDLE h = FindFirstFile(fn, &data_bb);
    if (INVALID_HANDLE_VALUE==h) {
        ft->dwLowDateTime = ft->dwHighDateTime = 0;
        return 0;
    }
    FindClose(h);
    *ft = data_bb.ftLastWriteTime;
    return 1;
}

int diff_filetime(const char *fn, FILETIME *ft0)
{
    FILETIME ft;
    get_filetime(fn, &ft);
    return CompareFileTime(&ft, ft0) != 0;
}

unsigned long getfileversion(const char *path)
{
    char temp[MAX_PATH]; DWORD dwHandle = 0, result = 0;
    UINT bytes = GetFileVersionInfoSize(strcpy(temp, path), &dwHandle);
    if (bytes) {
        char *buffer = (char*)m_alloc(bytes);
        if (GetFileVersionInfo(temp, 0, bytes, buffer)) {
            void *value;
            char subblock[2] = "\\";
            if (VerQueryValue(buffer, subblock, &value, &bytes)) {
                VS_FIXEDFILEINFO *vs = (VS_FIXEDFILEINFO*)value;
                result = ((vs->dwFileVersionLS & 0xFF) >> 0)
                    | ((vs->dwFileVersionLS & 0xFF0000) >> 8)
                    | ((vs->dwFileVersionMS & 0xFF) << 16)
                    | ((vs->dwFileVersionMS & 0xFF0000) << 8)
                    ;
            }}
        m_free(buffer);
    }
    /* dbg_printf("version number of %s %08x", path, result); */
    return result;
}

const char *replace_environment_strings_alloc(char **out, const char *src)
{
    int len, r;
    char *buf;

    *out = NULL;
    if (0 == strchr(src, '%'))
        return src;
    len = strlen(src) + 100;
    for (;;) {
        buf = (char*)m_alloc(len);
        r = ExpandEnvironmentStrings(src, buf, len);
        if (r && r <= len)
            return *out = buf;
        m_free(buf);
        if (!r)
            return src;
        len = r;
    }
}

char* replace_environment_strings(char* src, int max_size)
{
    char *tmp;
    replace_environment_strings_alloc(&tmp, src);
    if (tmp) {
        strcpy_max(src, tmp, max_size);
        m_free(tmp);
    }
    return src;
}

char* win_error(char *msg, int msgsize)
{
    char *p;
    if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        msg, msgsize, NULL))
        msg[0] = 0;

    /* strip \r\n */
    p = strchr(msg, 0);
    while (p > msg && (unsigned char)p[-1] <= ' ')
        --p;
    *p = 0;
    return msg;
}

void ForceForegroundWindow(HWND theWin)
{
    DWORD ThreadID1, ThreadID2;
    int attach;
    HWND fw;

    fw = GetForegroundWindow();
    if(theWin == fw)
        return; /* Nothing to do if already in foreground */
    ThreadID1 = GetWindowThreadProcessId(fw, NULL);
    ThreadID2 = GetCurrentThreadId();
    /* avoid attaching to a hanging message-queue */
    attach = ThreadID1 != ThreadID2 && 0 == is_frozen(fw);
    if (attach)
        AttachThreadInput(ThreadID1, ThreadID2, TRUE);
    SetForegroundWindow(theWin);
    if (attach)
        AttachThreadInput(ThreadID1, ThreadID2, FALSE);
}

void SetOnTop (HWND hwnd)
{
    if (IsWindow(hwnd)
     && IsWindowVisible(hwnd)
     && !(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
        SetWindowPos(hwnd,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING
            );
}

int is_frozen(HWND hwnd)
{
    DWORD_PTR dwres;
    return 0 == SendMessageTimeout(hwnd, WM_NULL, 0, 0,
        SMTO_ABORTIFHUNG|SMTO_NORMAL, 300, &dwres);
}

HWND window_under_mouse(void)
{
    POINT pt;
    GetCursorPos(&pt);
    return GetRootWindow(WindowFromPoint(pt));
}


/* ------------------------------------------------------------------------- */
/* Function: BBWait */
/* Purpose: wait for some obj and/or delay, dispatch messages in between */
/* ------------------------------------------------------------------------- */

int BBWait(int delay, unsigned nObj, HANDLE *pObj)
{
    DWORD t_end, t_wait, tick, r;
    int quit = 0;

    t_end = (delay > 0) ? GetTickCount() + delay : 0;
    do {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (WM_QUIT == msg.message) {
                quit = 1;
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        r = WAIT_TIMEOUT;
        if (delay > 0) {
            tick = GetTickCount();
            if (tick >= t_end)
                break;
            t_wait = t_end - tick;
        } else if (nObj) {
            t_wait = INFINITE;
        } else {
            break;
        }
        r = MsgWaitForMultipleObjects(nObj, pObj, FALSE, t_wait, QS_ALLINPUT);
    } while (r == WAIT_OBJECT_0 + nObj);

    if (quit)
        PostQuitMessage(0);
    if (r == WAIT_TIMEOUT)
        return -1;
    if (r >= WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + nObj)
        return r - WAIT_OBJECT_0;
    return -2;
}

/* ------------------------------------------------------------------------- */
/* API: BBSleep */
/* Purpose: pause for the given delay while blackbox remains responsive */
/* ------------------------------------------------------------------------- */

void BBSleep(unsigned millisec)
{
    BBWait(millisec, 0, NULL);
}

/* ------------------------------------------------------------------------- */
/* Function: run_process */
/* Purpose: low level process spawn, optionally wait for completion */
/* ------------------------------------------------------------------------- */

int run_process(const char *cmd, const char *dir, int flags)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD retcode = 0;
    char *buf;
    BOOL r;

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    if (flags & RUN_HIDDEN) {
        si.wShowWindow = SW_HIDE;
        si.dwFlags = STARTF_USESHOWWINDOW;
    }

    buf = new_str(cmd);
    r = CreateProcess(NULL, buf,
        NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, dir, &si, &pi);
    m_free(buf);

    if (FALSE == r)
        return -1;

    if (flags & RUN_WAIT) {
        BBWait(0, 1, &pi.hProcess);
        GetExitCodeProcess(pi.hProcess, (DWORD*)&retcode);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return retcode;
}

/* ------------------------------------------------------------------------- */
