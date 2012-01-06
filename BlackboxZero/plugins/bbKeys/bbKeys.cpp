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

#include "BBApi.h"
#include "bbversion.h"

const char szVersion     [] = "bbKeys "BBLEAN_VERSION;
const char szAppName     [] = "bbKeys";
const char szInfoVersion [] = BBLEAN_VERSION;
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = BBLEAN_RELDATE;
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";
const char szCopyright   [] = "2003-2009";

void about_box(void)
{
    char buffer[1000];
    sprintf(buffer, "%s - Copyright © %s %s", szVersion, szCopyright, szInfoEmail);
    MessageBox(NULL, buffer, szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
}

//===========================================================================

HWND hKeysWnd;
HWND BBhwnd;
HINSTANCE g_hInstance;
bool showlabel;
bool usingNT;
char rcpath[MAX_PATH];

struct HotkeyType
{
    struct HotkeyType *next;
    unsigned id;
    unsigned linenum;
    unsigned vkey, modifier, is_ExecCommand;
    char action[1];

} *g_hotKeys, *g_last_hotkey;

//===========================================================================
// Seems we dont really need this since on win9x/me we use the
// HSHELL_TASKMAN notification anyway, and on 2k/xp the winkey
// can be a hotkey, which seemse to work fine with the bits of
// timer sorcery below.

#if 0

bool set_kbdhook(bool set)
{
    return false;
}

#define DO_TIMERCHECK

//------------------------------------------------
#else
//------------------------------------------------

//#define TESTING

HHOOK g_mHook;
bool hook_is_set;
bool otherkey;
bool winkey;

LRESULT CALLBACK LLKeyboardProc (INT nCode, WPARAM wParam, LPARAM lParam)
{
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    if (HC_ACTION == nCode)
    {
        int vkCode = ((KBDLLHOOKSTRUCT *)lParam)->vkCode;
        int flags = ((KBDLLHOOKSTRUCT *)lParam)->flags;
#ifdef TESTING
        dbg_printf("hc_action vkey %x flags %x wp %x", vkCode, flags, wParam);
#endif
        if (VK_LWIN == vkCode || VK_RWIN == vkCode) {
            bool keydown = 0 == (0x80 & flags);
            if (keydown) {
                if (false == winkey) {
                    winkey = true;
                    otherkey = false;
                }
            } else {
                winkey = false;
                if (false == otherkey) {
                    PostMessage(hKeysWnd, BB_WINKEY, vkCode, 0);
                }
            }
            return 0;
        } else {
            otherkey = true;
        }
    }
    return CallNextHookEx (g_mHook, nCode, wParam, lParam);
}

//===========================================================================

bool set_kbdhook(bool set)
{
    bool r = false;
    if (hook_is_set != set) {
        if (set) {
            if (NULL == g_mHook)
                g_mHook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, g_hInstance, 0);
            r = NULL != g_mHook;
#ifdef TESTING
            dbg_printf("winkeyhook set (%d)", r);
#endif
        } else {
            if (g_mHook)
                r = FALSE != UnhookWindowsHookEx(g_mHook);
            g_mHook = NULL;
#ifdef TESTING
            dbg_printf("winkeyhook removed (%d)", r);
#endif
        }
        hook_is_set = set;
    } else {
        r = NULL != g_mHook;
    }
    return r;
}

#endif

//===========================================================================

const char* stristr(const char *aa, const char *bb)
{
    do {
        const char *a, *b; int c, d;
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

void getparam(char *from, const char *token, char *to, bool from_right)
{
    const char *p, *q; int l=0;
    if (NULL!=(p=stristr(from, token)))
    {
        p += strlen(token);
        while (' ' == *p)
            ++p;
        if ('(' == *p) {
            ++p;
            q = from_right ? strrchr(p,')') : strchr(p,')');
            if (q) memcpy(to, p, l = q-p);
        }
    }
    to[l]=0;
}

unsigned getvkey (const char *v)
{
    static const struct vkTable { const char* key; int vKey; } vkTable[] =
    {
        {"F1", VK_F1},
        {"F2", VK_F2},
        {"F3", VK_F3},
        {"F4", VK_F4},
        {"F5", VK_F5},
        {"F6", VK_F6},
        {"F7", VK_F7},
        {"F8", VK_F8},
        {"F9", VK_F9},
        {"F10", VK_F10},
        {"F11", VK_F11},
        {"F12", VK_F12},
        {"PRTSCN", VK_SNAPSHOT},
        {"PAUSE", VK_PAUSE},
        {"INSERT", VK_INSERT},
        {"DELETE", VK_DELETE},
        {"HOME", VK_HOME},
        {"END", VK_END},
        {"PAGEUP", VK_PRIOR},
        {"PAGEDOWN", VK_NEXT},
        {"LEFT", VK_LEFT},
        {"RIGHT", VK_RIGHT},
        {"DOWN", VK_DOWN},
        {"UP", VK_UP},
        {"TAB", VK_TAB},
        {"BACKSPACE", VK_BACK},
        {"SPACEBAR", VK_SPACE},
        {"APPS", VK_APPS},
        {"ENTER", VK_RETURN},
        {"NUM0", VK_NUMPAD0},
        {"NUM1", VK_NUMPAD1},
        {"NUM2", VK_NUMPAD2},
        {"NUM3", VK_NUMPAD3},
        {"NUM4", VK_NUMPAD4},
        {"NUM5", VK_NUMPAD5},
        {"NUM6", VK_NUMPAD6},
        {"NUM7", VK_NUMPAD7},
        {"NUM8", VK_NUMPAD8},
        {"NUM9", VK_NUMPAD9},
        {"MUL", VK_MULTIPLY},
        {"DIV", VK_DIVIDE},
        {"ADD", VK_ADD},
        {"SUB", VK_SUBTRACT},
        {"DEC", VK_DECIMAL},
        {"ESCAPE", VK_ESCAPE},
        {"LWIN", VK_LWIN},
        {"RWIN", VK_RWIN},
    /*
        {"VK_OEM_3", VK_OEM_3},
        {"VK_OEM_5", VK_OEM_5},
        {"VK_OEM_PLUS", VK_OEM_PLUS},
        {"VK_OEM_MINUS", VK_OEM_MINUS},
        {"VK_OEM_COMMA", VK_OEM_COMMA},
        {"VK_OEM_PERIOD", VK_OEM_PERIOD},
        {"NUMLOCK", VK_NUMLOCK},
        {"SCROLLOCK", VK_SCROLL},
        {"VOLUMEUP", VK_VOLUME_UP},
        {"VOLUMEDOWN", VK_VOLUME_DOWN},
    */
        { NULL, 0 }
    };

    const struct vkTable *t = vkTable;
    if (0 == v[1]) {
        unsigned c = (unsigned char)v[0];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            return c;
        return 0;
    }
    do if (!strcmp(t->key, v))
        return t->vKey;
    while ((++t)->key);

    if (v[0] == 'V' && v[1] == 'K' && v[2] >= '0' && v[2] <= '9') {
        if (v[2] == '0' && v[3] == 'X')
            return strtol(v+4, NULL, 16);
        return strtol(v+2, NULL, 10);
    }
    return 0;
}

unsigned getkey (const char *keystr, unsigned* pmod)
{
    char keytograb[100];
    char *p, *k, *s;
    unsigned vkey, modifier;

    vkey = modifier = 0;
    p = strupr(strcpy(keytograb, keystr));
    for (;;)
    {
        while (' ' == *p)
            ++p;
        if (0 == *p)
            break;
        k = s = strchr(p, '+');
        if (NULL == s)
            s = k = strchr(p, 0);
        else
            ++k;
        while(s > p && ' ' == s[-1])
            --s;
        *s = 0;

        if (0 == strcmp(p, "WIN"))
            modifier |= MOD_WIN;
        else
        if (0 == strcmp(p, "ALT"))
            modifier |= MOD_ALT;
        else
        if (0 == strcmp(p, "CTRL"))
            modifier |= MOD_CONTROL;
        else
        if (0 == strcmp(p, "SHIFT"))
            modifier |= MOD_SHIFT;
        else
            vkey = getvkey(p);
        p = k;
    }

    *pmod = modifier;
    return vkey;
}

bool read_line(char *buffer, int size, FILE *fp)
{
    char *p;
    if (NULL == fgets(buffer, size, fp))
        return false;
    p = buffer;
    while (*p && (unsigned char)*p <= ' ')
        ++p;
    if (p > buffer)
        p = strcpy(buffer, p);
    while (*p) {
        if ((unsigned char)*p <= ' ')
            *p = 32;
        ++p;
    }
    while (p > buffer && (unsigned char)p[-1] <= ' ')
        --p;
    *p = 0;
    return true;
}

//===========================================================================

void BBKeys_LoadHotkeys(HWND hwnd)
{
    FILE *fp;
    HotkeyType **ppHk, *h;
    int nID;
    unsigned linenum;
    int id, l;
    bool debug;

    if (false == FindRCFile(rcpath, "bbkeys", g_hInstance))
        return;

    fp = fopen(rcpath, "rb");
    if (NULL == fp)
        return;

    showlabel = true;
    debug = false;
    linenum = 0;
    nID = 1;
    ppHk = &g_hotKeys;

    for (;;)
    {
        char buffer[MAX_LINE_LENGTH];
        char keytograb[MAX_LINE_LENGTH];
        char action[MAX_LINE_LENGTH];
        char command[MAX_LINE_LENGTH];

        unsigned vkey;
        unsigned modifier;
        bool is_ExecCommand;
        char *k;

        if (false == read_line(buffer, sizeof buffer, fp)) {
            fclose(fp);
            break;
        }

        ++linenum;
        if (*buffer == '!' || *buffer == '#' || *buffer == 0)
            continue;

        if (*buffer == '-') {
            if (0 == stricmp("NOLABEL", buffer+1)) {
                showlabel = false;
                continue;
            }
            if (0 == stricmp("DEBUG", buffer+1)) {
                debug = true;
                continue;
            }
        }

        getparam(buffer, "KEYTOGRAB", keytograb, false);
        k = strchr(keytograb, 0);

        getparam(buffer, "WITHMODIFIER", k+1, false);
        if (k[1])
            k[0] = '+';

        getparam(buffer, "DOTHIS", command, true);
        is_ExecCommand = 0 != command[0];

        getparam(buffer, "WITHACTION", action, false == is_ExecCommand);
        if (0 == stricmp(action, "ExecCommand"))
            strcpy(action, command);

        if (0 == action[0])
            continue;

        vkey = getkey(keytograb, &modifier);
        if (0 == vkey) {
            BBMessageBox(MB_OK,
                "bbKeys.rc:%u: Invalid hotkey: %s -> %s",
                linenum, keytograb, action);
            continue;
        }

        id = nID;
        if (VK_LWIN == vkey || VK_RWIN == vkey) {
            if (GetUnderExplorer())
                continue;
            if (false == usingNT || set_kbdhook(true))
                id = 0;
            else
                modifier |= MOD_WIN;
        }

        for (h = g_hotKeys; h; h = h->next)
            if (h->vkey == vkey && h->modifier == modifier)
                break;

        if (h) {
            BBMessageBox(MB_OK,
                "bbKeys.rc:%u and %u: Hotkey defined twice: %s: '%s','%s'",
                h->linenum, linenum, keytograb, h->action, action);
            continue;
        }

        if (id) {
            if (0 == RegisterHotKey(hwnd, id, modifier, vkey)) {
                if (debug)
                    BBMessageBox(MB_OK,
                        "bbKeys.rc:%u: Unable to register hotkey: %s -> %s",
                        linenum, keytograb, action);
                continue;
            }
            ++nID;
        }

        l = strlen(action);
        h = (HotkeyType *)malloc(sizeof (HotkeyType) + l);
        h->id       = id;
        h->linenum  = linenum;
        h->modifier = modifier;
        h->vkey     = vkey;
        h->is_ExecCommand = is_ExecCommand;
        memcpy(h->action, action, 1+l);
        *(ppHk=&(*ppHk=h)->next)=NULL;

        //dbg_printf("hotkey: (%s) -> %s", keytograb, action);
    }
}

//===========================================================================
void BBKeys_FreeHotkeys(HWND hwnd)
{
    HotkeyType *n, *h = g_hotKeys;
    while (h)
    {
        if (h->id)
            UnregisterHotKey(hwnd, h->id);
        n = h->next;
        free(h);
        h = n;
    }
    g_last_hotkey = g_hotKeys = NULL;
}

//===========================================================================
void send_command(HotkeyType *h)
{
    char buffer[1024];
    const char *action = h->action;

    if (false == h->is_ExecCommand && action[0] != '@')
    {
        sprintf(buffer, "@BBCore.%s", action);
        action = buffer;
    }

    SendMessage(BBhwnd, BB_EXECUTEASYNC, 0, (LPARAM)action);

    if (showlabel)
    {
        sprintf(buffer, "BBKeys -> %s", h->action);
        SendMessage(BBhwnd, BB_SETTOOLBARLABEL, 0, (LPARAM)buffer);
    }
}

//===========================================================================

LRESULT CALLBACK HotkeyProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int msgs[] = {BB_RECONFIGURE, BB_WINKEY, BB_BROADCAST, 0};

    HotkeyType *h;
    unsigned modifier, vkey;

    switch (message)
    {
        case WM_CREATE:
            hKeysWnd = hwnd;
            SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            BBKeys_LoadHotkeys(hwnd);
            break;

        case WM_DESTROY:
            SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            BBKeys_FreeHotkeys(hwnd);
            break;

        case BB_RECONFIGURE:
            BBKeys_FreeHotkeys(hwnd);
            BBKeys_LoadHotkeys(hwnd);
            break;

        case BB_BROADCAST:
            if (0 == memicmp((LPCSTR)lParam, "@BBKeys.", 8))
            {
                lParam += 8;
                if (0 == stricmp((LPCSTR)lParam, "about"))
                {
                    about_box();
                    break;
                }
                if (0 == stricmp((LPCSTR)lParam, "editRC"))
                {
                    SendMessage(BBhwnd, BB_EDITFILE, (WPARAM)-1, (LPARAM)rcpath);
                    break;
                }
            }
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);

        case BB_WINKEY:
            modifier = 0;
            vkey = VK_LWIN;
            if (1 & GetAsyncKeyState(VK_RWIN)) vkey = VK_RWIN;
            if (0x8000 & GetAsyncKeyState(VK_SHIFT)) modifier |= MOD_SHIFT;
            if (0x8000 & GetAsyncKeyState(VK_CONTROL)) modifier |= MOD_CONTROL;
            if (0x8000 & GetAsyncKeyState(VK_MENU)) modifier |= MOD_ALT;
            for (h = g_hotKeys; h; h = h->next)
                if (vkey == h->vkey && modifier == h->modifier)
                {
                    send_command(h);
                    break;
                }
            break;

        case WM_HOTKEY:
            //dbg_printf("WM_HOTKEY %x %x", wParam, lParam);
            for (h = g_hotKeys; h; h = h->next)
                if (wParam == h->id) {
#ifdef DO_TIMERCHECK
                    if (VK_LWIN == h->vkey || VK_RWIN == h->vkey) {
                        g_last_hotkey = h;
                        SetTimer(hwnd, 2, 20, NULL);
                        break;
                    }
                    g_last_hotkey = NULL;
#endif
                    send_command(h);
                    break;
                }
            break;

        case WM_TIMER:
            if (2 == wParam)
            {
#ifdef DO_TIMERCHECK
                if (g_last_hotkey) {
                    // this is used for the winkey, it should fire only on
                    // key up, and only if no other key was pressed in between
                    static const unsigned char kbcheck[] = {
                        0x08, 0x0F,
                        0x15, 0x5A,
                        0x60, 0x9F,
                        0xA6, 0xFE,
                        0
                    };
                    const unsigned char *p = kbcheck;
                    unsigned v = g_last_hotkey->vkey, u;
                    do {
                        for (u = p[0]; u <= p[1]; ++u) {
                            if (u != v && (0x8000 & GetAsyncKeyState(u))) {
                                //dbg_printf("set %x", u);
                                g_last_hotkey = NULL;
                                goto ignore;
                            }
                        }
                    } while (*(p+=2));

                    if (0x8000 & GetAsyncKeyState(v))
                        break;
                    send_command(g_last_hotkey);
                }
        ignore:
#endif
                KillTimer(hwnd, wParam);
                break;
            }
            break;

    }
    return 0;
}

//===========================================================================

int beginPlugin(HINSTANCE hPluginInstance)
{
    if (BBhwnd)
    {
        MessageBox(NULL, "Dont load me twice!", szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
        return 1;
    }

    g_hInstance = hPluginInstance;
    usingNT = 0 == (GetVersion() & 0x80000000);
    BBhwnd = GetBBWnd();

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = HotkeyProc;    // our window procedure
    wc.hInstance = g_hInstance;     
    wc.lpszClassName = szAppName;   // our window class name

    if (!RegisterClass(&wc))
        return 1;

    CreateWindowEx(
        WS_EX_TOOLWINDOW,       // exstyles
        wc.lpszClassName,       // our window class name
        NULL,                   // use description for a window title
        WS_POPUP,
        0, 0,                   // position
        0, 0,                   // width & height of window
        NULL,                   // parent window
        NULL,                   // no menu
        g_hInstance,        // hInstance of DLL
        NULL
        );

    return 0;
}

void endPlugin(HINSTANCE hPluginInstance)
{
    DestroyWindow(hKeysWnd);
    UnregisterClass(szAppName, g_hInstance);
    set_kbdhook(false);
    BBhwnd = NULL;
}

LPCSTR pluginInfo(int field)
{
    switch (field)
    {
        default:
        case 0: return szVersion;
        case 1: return szAppName;
        case 2: return szInfoVersion;
        case 3: return szInfoAuthor;
        case 4: return szInfoRelDate;
        case 5: return szInfoLink;
        case 6: return szInfoEmail;
    }
}

//===========================================================================
