/*
 ============================================================================

  This file is part of bbIconBox source code.
  bbIconBox is a plugin for Blackbox for Windows

  Copyright © 2004-2009 grischka
  http://bb4win.sf.net/bblean

  bbIconBox is free software, released under the GNU General Public License
  (GPL version 2).

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 ============================================================================
*/

#include "BBApi.h"
#include "win0x500.h"
#include "bblib.h"
#include "bbshell.h"
#include "bbPlugin.h"
#include "tooltips.h"
#include "drawico.h"
#include "bbshell.h"

#define MODE_FOLDER 0
#define MODE_TRAY 1
#define MODE_TASK 2
#define MODE_PAGER 3

#define TASK_RISE_TIMER 4
#define CHECK_MEM_TIMER 5
#define TASK_ACTIVATE_TIMER 6

#define MAX_TIPTEXT 200

#define BBIB_DELETE (WM_USER + 100)
#define BBIB_UPDATE (WM_USER + 101)

#define BBIB_PICK (WM_USER + 102)
#define BBIB_DRAG (WM_USER + 103)
#define BBIB_DROP (WM_USER + 104)

// ---------------------------------------------
// LoadFolder.cpp
struct Item
{
    struct Item * next;
    void *data;
    HICON hIcon;
    int index;
    bool active;
    bool is_folder;
    char szTip [MAX_TIPTEXT];
};

struct Folder
{
    struct Folder * next;
    struct Item * items;
    int mode;
    int desk;
    HWND task_over;
    UINT id_notify;
    struct pidl_node *pidl_list;
    char path[MAX_PATH];
    class CDropTarget *drop_target;
};

struct Desk
{
    HMONITOR mon;
    RECT mon_rect;
    RECT v_rect;

    struct winStruct *winList;
    int winCount;
    int index;
};

void ClearFolder(Folder*);
void LoadFolder(Folder *pFolder, int iconsize, HWND hwnd);

// ---------------------------------------------
// DropTarget.cpp

class CDropTarget *init_drop_targ(HWND hwnd);
void exit_drop_targ (class CDropTarget *m_dropTarget);
LPCITEMIDLIST indrag_get_pidl(HWND hwnd, POINT *);


// ---------------------------------------------
// utils.cpp

bool select_folder(HWND hwnd, const char *title, char *name, char *path);
HICON extract_icon(IShellFolder *pFolder, LPCITEMIDLIST pidlRelative, int iconSize);

// ---------------------------------------------
// winlist.cpp
struct winStruct
{
    struct winStruct *next;
    struct taskinfo info;
    HWND hwnd;
    int index;
    bool active;
    bool iconic;
};

void build_winlist(Desk *f, HWND hwnd);
void setup_ratio(Desk *f, HWND hwnd, int width, int height);
void free_winlist(Desk *f);
void get_virtual_rect(Desk *f, RECT *d, winStruct *w, RECT *r);
winStruct *get_winstruct(Desk *f, int index);

void free_task_list(void);
void new_task_list(void);

// ---------------------------------------------
/* experimental: */
typedef BOOL (*TASKENUMPROC)(const struct tasklist *, LPARAM);
void EnumTasks (TASKENUMPROC lpEnumFunc, LPARAM lParam);

/* experimental: */
typedef BOOL (*DESKENUMPROC)(const struct DesktopInfo *, LPARAM);
void EnumDesks (DESKENUMPROC lpEnumFunc, LPARAM lParam);

/* experimental: */
typedef BOOL (*TRAYENUMPROC)(const struct systemTray *, LPARAM);
void EnumTray (TRAYENUMPROC lpEnumFunc, LPARAM lParam);

// ---------------------------------------------
