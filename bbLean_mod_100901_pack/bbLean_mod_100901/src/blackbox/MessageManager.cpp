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
#include "MessageManager.h"

struct wnd_entry
{
	struct wnd_entry* next;
	HWND hwnd;
};

struct MsgMap
{
	struct MsgMap *next;
	UINT msg;
	int count;
	struct wnd_entry *winlist;
};

static struct MsgMap *msgs;

//===========================================================================

void MessageManager_Init()
{
	msgs = NULL;
}

void MessageManager_Exit()
{
	// well, if all plugins behave correctly,
	// there should be nothing left to do here
	struct MsgMap *mm;
	dolist(mm, msgs) freeall(&mm->winlist);
	freeall(&msgs);
}

//===========================================================================
#if 0
static void dbg_msg(const char *id, HWND window, UINT msg)
{
	char buffer[256];
	GetClassName(window, buffer, 256);
	dbg_printf("%s: %s %d", id, buffer, msg);
}
#else
#define dbg_msg(id, window, msg)
#endif

//===========================================================================
static void AddRemoveMessages(HWND window, UINT* messages, bool add)
{
	UINT msg;
	while (0 != (msg = *messages++))
	{
		struct MsgMap *mm = (struct MsgMap *)assoc(msgs, (void*)msg);
		if (mm)
		{
			struct wnd_entry *w, **wp;
			for (wp = &mm->winlist; NULL != (w = *wp); )
				if (window == w->hwnd)
					*wp = w->next, m_free(w), --mm->count;
				else
					wp = &w->next;
		}
		else
		if (add)
		{
			mm = (struct MsgMap*)c_alloc(sizeof *mm);
			mm->msg = msg;
			//mm->winlist = NULL;
			//mm->next = NULL;
			//mm->count = 0;
			append_node (&msgs, mm);
		}

		if (add)
		{
			dbg_msg("add", window, msg);
			struct wnd_entry *w = (struct wnd_entry*)c_alloc(sizeof *w);
			w->hwnd = window;
			cons_node(&mm->winlist, w);
			++mm->count;
		}
		else
		if (mm)
		{
			dbg_msg("del", window, msg);
			if (NULL == mm->winlist) remove_item(&msgs, mm);
		}
	}
}

void MessageManager_AddMessages(HWND window, UINT* messages)
{
	AddRemoveMessages(window, messages, true);
}

void MessageManager_RemoveMessages(HWND window, UINT* messages)
{
	AddRemoveMessages(window, messages, false);
}

//====================

LRESULT MessageManager_SendMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct MsgMap *mm;
	dolist (mm, msgs) if (mm->msg == msg) goto found;
	return 0;
found:
	struct wnd_entry *w = mm->winlist;
	if (NULL == w) return 0;

	// make a local copy to be safe for messages being added or removed
	// while traversing the list...
	HWND hwnd_array[256], *pw = hwnd_array;
	do *pw++ = w->hwnd, w = w->next; while (w);

	LRESULT result = SendMessage(*--pw, msg, wParam, lParam);
	if (BB_DRAGTODESKTOP == msg)
		while (0 == result && pw > hwnd_array)
			result = SendMessage(*--pw, msg, wParam, lParam);
	else
		while (pw > hwnd_array)
			SendMessage(*--pw, msg, wParam, lParam);

	return result;
}

//===========================================================================
