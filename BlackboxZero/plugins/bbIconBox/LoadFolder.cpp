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

#include "bbIconBox.h"

Item *join_folders(Item *mi);

void del_item(Item *item, bool is_file)
{
    if (is_file) {
        struct pidl_node *pidl = (struct pidl_node *)item->data;
        delete_pidl_list(&pidl);
        if (item->hIcon)
            DestroyIcon(item->hIcon);
    }
    delete item;
}

void ClearFolder(Folder *pFolder)
{
    Item *item = pFolder->items;
    while (item) {
        Item* next = item->next;
        del_item(item, MODE_FOLDER == pFolder->mode);
        item = next;
    }
    pFolder->items = NULL;

    if (pFolder->id_notify) {
        remove_change_notify_entry(pFolder->id_notify);
        pFolder->id_notify = 0;
    }

    delete_pidl_list(&pFolder->pidl_list);
}

//////////////////////////////////////////////////////////////////////
// Function:    LoadFolder
// Purpose:     enumerate items in a folder, sort them, and insert them
//              into a menu
//////////////////////////////////////////////////////////////////////

void LoadFolder(Folder *pFolder, int iconsize, HWND hwnd)
{
    struct pidl_node *pidl_list;
    LPCITEMIDLIST pIDFolder;

    pidl_list = get_folder_pidl_list (pFolder->path);
    pFolder->pidl_list = pidl_list;
    if (NULL==pidl_list)
        return;

    pIDFolder = first_pidl(pidl_list);
    pFolder->id_notify = add_change_notify_entry(hwnd, pIDFolder);

    Item **ppItems = &pFolder->items;
    while (pidl_list)
    {
        LPCITEMIDLIST pIDFolder = first_pidl(pidl_list);

        struct enum_files *ef;
        if (ef_open(pIDFolder, &ef))
        {
            while (ef_next(ef))
            {
                int attr;
                struct pidl_node *pidl;

                if (0 == ef_getpidl(ef, &pidl))
                    continue;
                ef_getattr(ef, &attr);

                Item *item = new Item;
                item->is_folder = 0 != (attr & ef_folder);
                item->data = pidl;
                sh_get_icon_and_name(first_pidl(pidl), &item->hIcon, iconsize, item->szTip, sizeof item->szTip);

                // add item to the list
                item -> next = *ppItems, *ppItems = item;
            }
            ef_close(ef);
        }
        pidl_list = pidl_list -> next;
    }
    pFolder->items = join_folders(pFolder->items);
}

// ----------------------------------------------
#define m_pszTitle szTip

int cmp_fn(Item** m1, Item** m2)
{
    return stricmp((*m1)->m_pszTitle, (*m2)->m_pszTitle);
}

// Not ideal - convert list to array, sort it, convert to list again
Item * Sort(Item *MI)
{
    Item *i, **a; int n;
    n=0; dolist(i, MI) n++;
    a = (Item**)m_alloc(n*sizeof(Item*));
    n=0; dolist(i, MI) a[n]=i, n++;
    qsort(a, n, sizeof(Item*), (int(*)(const void*,const void*))cmp_fn);
    for (i=NULL;n;) a[--n]->next=i, i=a[n];
    m_free(a);
    return i;
}

// ----------------------------------------------
Item *join_folders(Item *items)
{
    items = Sort(items);
    Item *mi = items, *mn;
    if (mi) while (NULL != (mn = mi->next))
    {
        // compare first with second
        if (mi->m_pszTitle[0] && 0 == stricmp(mi->m_pszTitle, mn->m_pszTitle)) 
        {
            // set list ptr to the 3rd item, which becomes the second now
            mi->next = mn->next;

            // delete the second menu item
            del_item(mn, true);
        } 
        else 
        {
            // the second is now the first
            mi = mn;
        }
    }
    return items;
}

// ----------------------------------------------

