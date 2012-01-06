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

#include "../BB.h"
#include "../Settings.h"
#include "Menu.h"
#include <shellapi.h>
#include <shlobj.h>

static void exec_folder_click(LPCITEMIDLIST pidl);

// these are used during sorting
static int g_sortmode;
static int g_sortrev;
static struct IShellFolder *g_psf;

//===========================================================================
// SpecialFolderItem
//===========================================================================

SpecialFolderItem::SpecialFolderItem(
    const char* pszTitle,
    const char *path,
    struct pidl_node* pidl_list,
    const char  *pszExtra
    ) : FolderItem(NULL, pszTitle)
{
    m_ItemID = MENUITEM_ID_SF;
    m_pidl_list = path ? get_folder_pidl_list(path) : pidl_list;
    // command to apply to files, optional
    m_pszExtra = new_str(pszExtra);
    if (0 == pszTitle[0]) {
        char disp_name[MAX_PATH];
        replace_str(&m_pszTitle, m_pidl_list
            ? sh_getdisplayname(first_pidl(m_pidl_list), disp_name)
            : NLS0("Invalid Path"));
    }
}

SpecialFolderItem::~SpecialFolderItem()
{
    free_str(&m_pszExtra);
}

//====================================================
// This looks up whether a folder is already on screen

Menu *Menu::find_special_folder(struct pidl_node* p1)
{
    MenuList *ml;
    struct pidl_node *p2;
    if (p1) dolist (ml, Menu::g_MenuWindowList) {
        Menu *m = ml->m;
        if (m->m_MenuID == MENU_ID_SF) {
            p2 = m->m_pidl_list;
            if (p2 && equal_pidl_list(p1, p2))
                return m;
        }
    }
    return NULL;
}

//================================================

void SpecialFolderItem::ShowSubmenu(void)
{
    if (NULL == m_pSubmenu)
    {
        Menu *m = Menu::find_special_folder(m_pidl_list);
        if (m)
            m->incref();
        else
            m = new SpecialFolder(m_pszTitle, m_pidl_list, m_pszExtra);
        LinkSubmenu(m);
    }
    MenuItem::ShowSubmenu();
}

//================================================

void SpecialFolderItem::Invoke(int button)
{
    LPCITEMIDLIST pidl;

    pidl = GetPidl();

    if (INVOKE_PROP & button)
    {
        show_props(pidl);
        return;
    }

    if ((INVOKE_DBL|INVOKE_LEFT) == button)
    {
        m_pMenu->hide_on_click();
        exec_folder_click(pidl);
        return;
    }

    if (INVOKE_RIGHT & button)
    {
        ShowContextMenu(NULL, pidl);
        return;
    }

    if (INVOKE_DRAG & button)
    {
        m_pMenu->start_drag(NULL, pidl);
        return;
    }

    if (INVOKE_LEFT & button)
    {
        if (m_pSubmenu && (MENU_ID_SF != m_pSubmenu->m_MenuID))
            m_pMenu->HideChild(); // hide contextmenu
    }

    FolderItem::Invoke(button);
}

//===========================================================================
// SFInsert - An invisible item that expands into a file listing
//  when the menu is about to be shown (in Menu::Validate)
//===========================================================================

SFInsert::SFInsert(const char *path, const char  *pszExtra)
    : MenuItem("")
{
    m_ItemID = MENUITEM_ID_INSSF;
    m_pidl_list = get_folder_pidl_list(path);
    m_pszExtra = new_str(pszExtra);
    m_pLast = NULL;
    m_bNOP = true;
}

SFInsert::~SFInsert()
{
    free_str(&m_pszExtra);
}

void SFInsert::Measure(HDC hDC, SIZE *size)
{
    MenuItem *pNext, *p;
    size->cx = size->cy = 0;
    if (m_pLast)
        return;

    pNext = this->next;
    this->next = NULL;
    m_pMenu->m_pLastItem = this;
    m_pMenu->AddFolderContents(m_pidl_list, m_pszExtra);
    for (p = this->next; p; p = p->next)
        ++m_pMenu->m_itemcount;
    m_pLast = m_pMenu->m_pLastItem;
    m_pLast->next = pNext;
    if (NULL == m_pMenu->m_pidl_list) {
        m_pMenu->m_pidl_list = copy_pidl_list(m_pidl_list);
        m_pMenu->m_bIsDropTarg = true;
    }
}

void SFInsert::RemoveStuff(void)
{
    MenuItem *mi, *next = NULL;
    if (NULL == m_pLast)
        return;
    for (mi = this->next; mi; ) {
        next = mi->next;
        delete mi;
        --m_pMenu->m_itemcount;
        if (mi == m_pLast)
            break;
        mi = next;
    }
    this->next = next;
    m_pLast = NULL;
    m_pMenu->m_pActiveItem = NULL;
}

void SFInsert::Paint(HDC hDC)
{
}

//===========================================================================
// SpecialFolder
//===========================================================================

SpecialFolder::SpecialFolder(
    const char *pszTitle,
    const struct pidl_node *pidl_list,
    const char  *pszExtra) : Menu(pszTitle)
{
    m_MenuID = MENU_ID_SF;
    m_pidl_list = copy_pidl_list(pidl_list);
    m_pszExtra = new_str(pszExtra);
    m_bIsDropTarg = true;
    // fill menu
    UpdateFolder();
}

SpecialFolder::~SpecialFolder()
{
    free_str(&m_pszExtra);
}

void SpecialFolder::UpdateFolder(void)
{ 
    int flag;

    // delete_old items
    DeleteMenuItems();

    // load the folder contents
    flag = AddFolderContents(m_pidl_list, m_pszExtra);

    if (0 == (flag & 6)) {
        if (flag & 1)
            MakeMenuNOP(this, NLS0("No Files"));
        else
            MakeMenuNOP(this, NLS0("Invalid Path"));
    }
}

//===========================================================================
Menu *MakeFolderMenu(const char *title, const char* path, const char *cmd)
{
    char disp_name[MAX_PATH];
    struct pidl_node* pidl_list;
    Menu *m = NULL;

    pidl_list = get_folder_pidl_list(path);
    if (pidl_list)
        m = Menu::find_special_folder(pidl_list);

    if (m) {
        m->incref();
        m->SaveState();
    } else {
        if (NULL == title) {
            if (pidl_list) {
                sh_getdisplayname(first_pidl(pidl_list), disp_name);
                title = disp_name;
            } else {
                title = file_basename(unquote(strcpy_max(disp_name, path, sizeof disp_name)));
            }
        }
        m = new SpecialFolder(title, pidl_list, cmd);
    }
    delete_pidl_list(&pidl_list);
    return m;
}

//===========================================================================

//file extension priority sort
static int folder_compare(MenuItem** pm1, MenuItem** pm2)
{
    int f, x, y, z;
    MenuItem *m1, *m2;
    const char *a1, *b1, *a2, *b2;
    int sortmode = g_sortmode;
    HRESULT hr;

    if (g_sortrev)
        m2 = *pm1, m1 = *pm2;
    else
        m1 = *pm1, m2 = *pm2;

#ifndef BBTINY
    if (sortmode >= 2 && g_psf)
    {
        // use shell column-sort function
        LPCITEMIDLIST p1, p2, n1, n2;
        p1 = m1->GetPidl();
        p2 = m2->GetPidl();
        if (p1 && p2) {
            // get relative pidl
            while (cbID(n1=NextID(p1))) p1 = n1;
            while (cbID(n2=NextID(p2))) p2 = n2;
            hr = COMCALL3(g_psf, CompareIDs, sortmode-2, p1, p2);
            return (int)(short)HRESULT_CODE(hr);
        }}
#endif

    if (0 != (z = m2->m_nSortPriority - (f = m1->m_nSortPriority)))
        return z;

    a1 = m1->m_pszTitle;
    b1 = m2->m_pszTitle;
    if (1 != sortmode || f == M_SORT_NAME || f == M_SORT_FOLDER)
        return stricmp(a1, b1); // sort by name

    a2 = strrchr(a1,'.');
    b2 = strrchr(b1,'.');
    if (a2 == NULL)
        return (b2 == NULL) ? stricmp(a1, b1) : -1;
    else
    if (b2 == NULL)
        return 1;

    if (0 != (z = stricmp(a2,b2))) // sort by extension
        return z;

    x = a2-a1;
    y = b2-b1;
    if (0 != (z = memicmp(a1,b1,x<y?x:y)))
        return z;
    if (0 != (z = x-y))
        return z;
    return (int)((BYTE*)m2 - (BYTE*)m1);
}

int Menu::AddFolderContents(const struct pidl_node *pidl_list, const char *extra)
{
    const struct pidl_node *p = pidl_list;
    MenuItem *pItems = NULL;
    int flag = 0;
    int options = 0;

    if (extra && 0 == strcmp(extra, MM_THEME_BROAM)) {
        MenuItem *pItem = MakeMenuItem(this, "default", "@BBCore.theme default", false);
        MenuItemOption(pItem, BBMENUITEM_UPDCHECK);
        MakeMenuNOP(this, NULL);
        options |= LF_norecurse;
    }

    if (p) for (;;) {
        flag |= LoadFolder(&pItems, first_pidl(p), extra, options);
        p = p->next;
        if (NULL == p)
            break;
        options |= LF_join;
    }

    if (NULL == pItems)
        return flag;

    g_sortmode = m_sortmode ? m_sortmode : Settings_menu.sortByExtension;
    g_sortrev = m_sortrev;
    g_psf = NULL;

#ifndef BBTINY
    if (g_sortmode >= 2 && !(options & LF_join))
        g_psf = sh_get_folder_interface(first_pidl(pidl_list));
#endif

    Sort(&pItems, folder_compare);

    if (g_psf)
        COMCALL0(g_psf, Release), g_psf = NULL;

    if (pItems) for (;;) {
        MenuItem *next = pItems->next;
        AddMenuItem(pItems);
        if (!next)
            break;
#if 0
        if (pItems->m_nSortPriority != next->m_nSortPriority)
            MakeMenuNOP(this, NULL);
#endif
        pItems = next;
    }

    return flag;
}

/* ----------------------------------------------------------------------- */
// Function:    LoadFolder
// Purpose:     enumerate items in a folder, sort them, and insert them
//              into a menu
/* ----------------------------------------------------------------------- */

#ifndef BBTINY

static int is_controls(LPCITEMIDLIST pIDFolder)
{
     LPITEMIDLIST pIDCtrl;
     int ret = 0;
     if (NOERROR == SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pIDCtrl)) {
         ret = isEqualPIDL(pIDCtrl, pIDFolder);
         SHMalloc_Free(pIDCtrl);
     }
     return ret;
}

int LoadFolder(
    MenuItem **ppItems,
    LPCITEMIDLIST pIDFolder,
    const char *pszExtra,
    int options
    )
{
    struct enum_files *ef;

    char szDispName[MAX_PATH];
    char szFullPath[MAX_PATH];
    MenuItem* pItem;
    struct pidl_node *pidl_list;
    int attr, r;

    if (0 == ef_open(pIDFolder, &ef))
        return 0;

    if (usingVista && is_controls(pIDFolder))
        options |= LF_norecurse;

    r = 1; // folder exists
    while (ef_next(ef))
    {
        ef_getattr(ef, &attr);
        if ((attr & ef_hidden) && false == Settings_menu.showHiddenFiles)
            continue;

        ef_getname(ef, szDispName);
        ef_getpidl(ef, &pidl_list);

        if (pszExtra) {
            char *p = (char*)file_extension(szDispName);
            if (0 == stricmp(p, ".style"))
                *p = 0; // cut off .style file-extension
        }

        if (options & LF_join) {
            dolist (pItem, *ppItems) {
                if (0 == stricmp(pItem->m_pszTitle, szDispName)) {
                    //dbg_printf("join: %s %d", szDispName, 0 != (attr & ef_folder));
                    if (attr & ef_folder)
                        append_node(&pItem->m_pidl_list, pidl_list);
                    else
                        delete_pidl_list(&pidl_list);
                    break;
                }
            }
            if (pItem)
                continue;
        }

        if ((attr & ef_folder) && !(options & LF_norecurse)) {
            r |= 4; // contents include a folder
            pItem = new SpecialFolderItem(
                szDispName,
                NULL,
                pidl_list,
                pszExtra
                );

        } else if (pszExtra) {
            r |= 2; // contents include an item
            pItem = new CommandItem(
                NULL,
                szDispName,
                false
                );

            ef_getpath(ef, szFullPath);
            pItem->m_pszCommand = replace_arg1(pszExtra, szFullPath);
            pItem->m_ItemID |= MENUITEM_UPDCHECK;
            pItem->m_nSortPriority = M_SORT_NAME;
            pItem->m_pidl_list = pidl_list;

        } else {
            r |= 2; // contents include an item
            pItem = new CommandItem(
                NULL,
                szDispName,
                false
                );

            if (attr & ef_link)
                pItem->m_nSortPriority = M_SORT_NAME;
            pItem->m_pidl_list = pidl_list;
        }

        // add item to the list
        pItem->next = *ppItems, *ppItems = pItem;
    }
    ef_close(ef);
    return r;
}

//===========================================================================
#endif //ndef BBTINY
//===========================================================================

static void exec_folder_click(LPCITEMIDLIST pidl)
{
    char path[MAX_PATH], *tmp;
    const char *cmd = ReadString(extensionsrcPath(NULL),
        "blackbox.options.openFolderCommand", NULL);
    if (cmd) {
        if (sh_getnameof(NULL, pidl, SHGDN_FORPARSING, path)) {
            post_command(tmp = replace_arg1(cmd, path));
            m_free(tmp);
        }
    } else {
        BBExecute_pidl("explore", pidl);
    }
}

void show_props(LPCITEMIDLIST pidl)
{
    BBExecute_pidl("properties", pidl);
}

//===========================================================================

//===========================================================================
