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
#include "MessageManager.h"
#include <malloc.h>

struct winmap
{
    struct winmap* next;
    HWND hwnd;
};

struct MsgMap
{
    struct MsgMap *next;
    UINT_PTR msg;
    int count;
    int send_mode;
    struct winmap *winmap;
};

static struct MsgMap *msgs;

//===========================================================================
void MessageManager_Init(void)
{
}

//===========================================================================
void MessageManager_Exit(void)
{
    // Well, if all plugins behave correctly and unregister their messages,
    // there should be nothing left to do here
    struct MsgMap *mm;
    dolist (mm, msgs)
        freeall(&mm->winmap);
    freeall(&msgs);
}

//===========================================================================
// #include "other/BBMessages.cpp"

#ifdef LOG_BB_MESSAGES
static void dbg_msg(const char *action, HWND hwnd, UINT msg)
{
    char buffer[100];
    struct MsgMap *mm = (struct MsgMap *)assoc(msgs, (void*)msg);
    if (0 == GetClassName(hwnd, buffer, sizeof buffer))
        strcpy(buffer, "(invalid)");
    dbg_printf("%s: %s %s (%d/%d)",
        action,
        buffer,
        bb_str(msg, -1, -1),
        listlen(msgs), mm ? mm->count : 0
        );
}
#else
#define dbg_msg(action, hwnd, msg)
#endif

//===========================================================================
void MessageManager_Register(HWND hwnd, const UINT* messages, bool add)
{
    UINT msg;
    while (0 != (msg = *messages++))
    {
        struct MsgMap *mm = (struct MsgMap *)assoc(msgs, (void*)msg);
        if (mm) {
            if (remove_assoc(&mm->winmap, hwnd))
                --mm->count;

        } else if (add) {
            mm = c_new(struct MsgMap);
            mm->msg = msg;
            append_node (&msgs, mm);
            // these are the messages that expect return values and
            // are handled differently in 'MessageManager_Send()'
            mm->send_mode = BB_DRAGTODESKTOP == msg || BB_GETBOOL == msg;
        }

        if (add) {
            struct winmap *w = c_new(struct winmap);
            w->hwnd = hwnd;
            cons_node(&mm->winmap, w);
            ++mm->count;
            dbg_msg("add", hwnd, msg);

        } else if (mm) {
            if (NULL == mm->winmap)
                remove_item(&msgs, mm);
            dbg_msg("del", hwnd, msg);
        }
    }
}

//===========================================================================
LRESULT MessageManager_Send(UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct MsgMap *mm;
    struct winmap *w;
    LRESULT result;
    HWND hwnd_array[256], *pw;

    dolist (mm, msgs)
        if (mm->msg == msg)
            goto found;
    return 0;
found:
    w = mm->winmap;
    if (NULL == w)
        return 0;

    // make a local copy to be safe for messages being
    // added or removed while traversing the list:
    pw = hwnd_array;
    do {
        *pw = w->hwnd;
        dbg_msg("send", *pw, msg);
        w = w->next;
    } while (++pw < hwnd_array + 256 && w);

    result = SendMessage(*--pw, msg, wParam, lParam);
    if (mm->send_mode)
        while (0 == result && pw > hwnd_array)
            result = SendMessage(*--pw, msg, wParam, lParam);
    else
        while (pw > hwnd_array)
            SendMessage(*--pw, msg, wParam, lParam);

    return result;
}

//===========================================================================
