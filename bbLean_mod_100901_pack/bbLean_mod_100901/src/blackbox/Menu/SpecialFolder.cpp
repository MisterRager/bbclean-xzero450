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

#include "../BB.h"
#include "../Settings.h"
#include "../Pidl.h"
#include "MenuMaker.h"
#include "Menu.h"
#include <shlobj.h>

//===========================================================================
// SpecialFolderItem
//===========================================================================

SpecialFolderItem::SpecialFolderItem(LPCSTR pszTitle, const char *pszPath, struct pidl_node* pidl_list, const char  *optional_command, const char* pszIcon)
    : FolderItem(NULL, pszTitle, pszIcon)
{
    m_pidl_list = pidl_list;
    m_ItemID    = MENUITEM_ID_SF;
    m_pszPath   = new_str(pszPath);
    // command to apply to files, optional
    m_pszExtra  = new_str(optional_command);
}

SpecialFolderItem::~SpecialFolderItem()
{
    delete_pidl_list(&m_pidl_list);
    free_str(&m_pszPath);
    free_str(&m_pszExtra);
}

const struct _ITEMIDLIST *SpecialFolderItem::check_pidl(void)
{
    if (NULL == m_pidl_list && m_pszPath)
        m_pidl_list = get_folder_pidl_list(m_pszPath);
    return m_pidl_list ? m_pidl_list->v : NULL;
}

//================================================
void SpecialFolderItem::ShowSubMenu(void)
{
    if (NULL == m_pSubMenu)
    {
        check_pidl();
        LinkSubmenu(new SpecialFolder(m_pszTitle, m_pidl_list, m_pszExtra));
    }

    MenuItem::ShowSubMenu();
}

//================================================
static void exec_folder_click(const struct _ITEMIDLIST * pidl)
{
    const char *p = ReadString(extensionsrcPath(), "blackbox.options.openFolderCommand:", NULL);
    if (p)
    {
        char path[MAX_PATH]; char buffer[MAX_PATH*2];
        if (sh_get_displayname(NULL, pidl, SHGDN_FORPARSING, path))
        {
            post_command(replace_argument1(buffer, p, path));
            return;
        }
    }
    exec_pidl(pidl, "explore", NULL);
}

void SpecialFolderItem::Invoke(int button)
{
    const struct _ITEMIDLIST * pidl = check_pidl();

    if ((INVOKE_DBL|INVOKE_LEFT) == button)
    {
        m_pMenu->hide_on_click();
        if (pidl) exec_folder_click(pidl);
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
        if (m_pSubMenu && (MENU_ID_SF != m_pSubMenu->m_MenuID))
            m_pMenu->HideChild(); // hide contextmenu
    }

    FolderItem::Invoke(button);
}

//===========================================================================
// SpecialFolder
//===========================================================================
SpecialFolder::SpecialFolder(const char *pszTitle, const struct pidl_node *pidl_list, const char  *optional_command)
    : Menu(pszTitle)
{
    m_MenuID    = MENU_ID_SF;   // ID
    m_pidl_list = NULL;         // the list of pidl
    m_notify    = 0;
    // create a copy of the SpecialFolderItem's pidl-list
    m_pidl_list = copy_pidl_list(pidl_list);
    m_pszExtra  = new_str(optional_command);
    // attach a copy of the (first) pidl to the TitleItem (for
    // "shift - right - click on title" - context menu.
    if (m_pidl_list) m_pMenuItems->m_pidl = duplicateIDlist(m_pidl_list->v);
    // fill menu
    UpdateFolder();
}

SpecialFolder::~SpecialFolder()
{
    // delete_the pidl-list and it's contents
    delete_pidl_list(&m_pidl_list);
    free_str(&m_pszExtra);
    // note: the drop-target is unregistered in the 'Menu' destructor
}

//================================================
void SpecialFolder::UpdateFolder(void)
{ 
    // ---------------------------------------
    // remember the active item as text

    MenuItem *ActiveItem = m_pActiveItem;
    char *active_item_text = NULL;

    m_pLastChild = m_pChild;
    if (m_pLastChild) ActiveItem = m_pLastChild->m_pParentItem;
    if (ActiveItem) active_item_text = new_str(ActiveItem->m_pszTitle);

    // delete_old items
    DeleteMenuItems();

    // load the folder contents
    MenuItem *Items = NULL; int r = 0;

    struct pidl_node *p;
    dolist (p, m_pidl_list)
        r |= MenuMaker_LoadFolder(&Items, p->v, m_pszExtra);

    if (Items) add_folder_contents(Items, NULL != m_pidl_list->next);
    else if (r) MakeMenuNOP(this, NLS0("No Files"));
    else MakeMenuNOP(this, NLS0("Invalid Path"));

    // ---------------------------------------
    // search by text the previously active item

    if (active_item_text)
    {
        dolist (ActiveItem, m_pMenuItems)
            if (0 == strcmp(active_item_text, ActiveItem->m_pszTitle))
                break;

        free_str(&active_item_text);
    }

    // ---------------------------------------
    // possibly reconnect to an open child-folder

    if (m_pLastChild)
    {
        if (ActiveItem)
        {
            m_pLastChild->incref();
            ActiveItem->LinkSubmenu(m_pLastChild);
            m_pLastChild->LinkToParentItem(ActiveItem);
        }
        else
        {
            m_pLastChild->Hide(); // lost child
        }
        m_pLastChild = NULL;
    }

    if (ActiveItem) ActiveItem->Active(2);
}

//===========================================================================
// merge folders - i.e. PROGRAMS|COMMON_PROGRAMS

void join_folders(SpecialFolderItem *i1st)
{
    SpecialFolderItem *i2nd;
    if (i1st) while (NULL != (i2nd = (SpecialFolderItem *)i1st->next))
    {
        if (0 == stricmp(i1st->m_pszTitle, i2nd->m_pszTitle))
        {
            if (i1st->m_ItemID == MENUITEM_ID_SF && i2nd->m_ItemID == MENUITEM_ID_SF)
            {
                // if both are folders
                if (i2nd->m_pidl_list->v)
                {
                    append_node(&i1st->m_pidl_list, i2nd->m_pidl_list);
                    i2nd->m_pidl_list = NULL;
                }
            join:
                i1st->next = i2nd->next, delete i2nd;
                continue;
            }

            if (i1st->m_ItemID != MENUITEM_ID_SF && i2nd->m_ItemID != MENUITEM_ID_SF)
            {
                // if both are not folders
                goto join;
            }
        }
        i1st = i2nd;
    }
}

//===========================================================================
//file extension priority sort

int SpecialFolder_Compare(MenuItem** m1, MenuItem** m2)
{
    int f,x,y,z; const char *a1,*b1,*a2,*b2;

    if (0 != (z = (*m2)->m_nSortPriority - (f = (*m1)->m_nSortPriority)))
        return z;

    a1=(*m1)->m_pszTitle;
    b1=(*m2)->m_pszTitle;

    if (false == Settings_menusExtensionSort || f == M_SORT_NAME || f == M_SORT_FOLDER)
        p1: return stricmp(a1, b1);

    a2=strrchr(a1,'.');
    b2=strrchr(b1,'.');

    if (a2 == NULL)
    {
        if (b2 == NULL) goto p1;
        return -1;
    }
    if (b2 == NULL) return 1;

    if (0!=(z=stricmp(a2,b2))) return z;
    x=a2-a1;
    y=b2-b1;
    if (0!=(z=memicmp(a1,b1,x<y?x:y))) return z;
    if (0!=(z=x-y)) return z;
    return (BYTE*)*m2 - (BYTE*)*m1;

}

MenuItem * MenuItem::Sort(int(*cmp_fn)(MenuItem**,MenuItem**))
{
    MenuItem *item; int n = 0;
    dolist(item, this) n++;  // get size
    if (n < 2) return this;
    MenuItem **a = (MenuItem**)m_alloc(n * sizeof item); // make array
    n = 0; dolist(item, this) a[n] = item, n++; // store pointers
    qsort(a, n, sizeof *a, (int(*)(const void*,const void*))cmp_fn);
    do a[--n]->next = item, item = a[n]; while (n); // make list
    m_free(a); // free array
    return item;
}

void Menu::add_folder_contents(MenuItem *pItems, bool join)
{
    pItems = pItems->Sort(SpecialFolder_Compare);
    if (join) join_folders((SpecialFolderItem *)pItems);

    while (pItems) pItems = AddMenuItem(pItems)->next;
}

//===========================================================================

//===========================================================================
void SpecialFolder::register_droptarget(bool set)
{
    if (set)
    {
        if (m_pidl_list)
        {
            m_droptarget = init_drop_targ(m_hwnd, m_pidl_list->v);
            m_notify = add_change_notify_entry(m_hwnd, m_pidl_list->v);
        }
    }
    else
    {
        if (m_notify)
            remove_change_notify_entry(m_notify), m_notify = 0;

        if (m_droptarget)
            exit_drop_targ(m_droptarget), m_droptarget = NULL;
    }
}

//===========================================================================

//===========================================================================
