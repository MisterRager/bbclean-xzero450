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
#ifndef _BBSHELL_H_
#define _BBSHELL_H_

#include "bblib.h"

#define NO_INTSHCUT_GUIDS
#define NO_SHDOCVW_GUIDS
#include <shlobj.h>
#include <shellapi.h>

#define cbID(pidl) (((LPITEMIDLIST)(pidl))->mkid.cb)
#define NextID(pidl) ((LPITEMIDLIST)((BYTE*)(pidl)+cbID(pidl)))

#ifdef __cplusplus
extern "C" {
#endif

/* Free shell items */
BBLIB_EXPORT void SHMalloc_Free(void* whatever);
BBLIB_EXPORT int GetIDListSize(LPCITEMIDLIST pidl);
/* join two lists, using "m_alloc" */
BBLIB_EXPORT LPITEMIDLIST joinIDLists(LPCITEMIDLIST pidlA, LPCITEMIDLIST pidlB);
/* copy list, using "m_alloc" */
BBLIB_EXPORT LPITEMIDLIST duplicateIDList(LPCITEMIDLIST pidl);
/* compare */
BBLIB_EXPORT int isEqualPIDL(LPCITEMIDLIST p1, LPCITEMIDLIST p2);
/* free list, using m_free */
BBLIB_EXPORT void freeIDList(LPITEMIDLIST p);
/* wrapper to IShellFolder->GetDisplayNameOf */
BBLIB_EXPORT BOOL sh_getnameof(struct IShellFolder *piFolder, LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName);
/* Get the Com Interface from pidl */
BBLIB_EXPORT struct IShellFolder* sh_get_folder_interface(LPCITEMIDLIST pIDFolder);
/* Get pidl from path, parsing SPECIAL folders, defaults to blackbox directory */
BBLIB_EXPORT LPITEMIDLIST get_folder_pidl(const char *path);
/* Get pidl from path, no parsing */
BBLIB_EXPORT LPITEMIDLIST sh_getpidl(struct IShellFolder *pSF, const char *path);
/* Get path from csidl, replacement for SHGetFolderPath */
BBLIB_EXPORT int sh_getfolderpath(char* szPath, UINT csidl);
/* parsing SPECIAL folders and convert to string again, defaults to basepath */
BBLIB_EXPORT char *replace_shellfolders_from_base(char *buffer, const char *path, int search_path, const char *basepath);
/* parsing SPECIAL folders and convert to string again, defaults to blackbox directory */
BBLIB_EXPORT char *replace_shellfolders(char *buffer, const char *path, int search_path);
/* retrieve the shell icon */
BBLIB_EXPORT HICON sh_geticon (LPCITEMIDLIST pID, int iconsize);
/* retrieve the default display name */
BBLIB_EXPORT char* sh_getdisplayname (LPCITEMIDLIST pID, char *buffer);
/* retrieve icon and display name */
BBLIB_EXPORT int sh_get_icon_and_name(LPCITEMIDLIST pID, HICON *pIcon, int iconsize, char *szTip, int NameSize);

/* support for ContextMenu, DragSource, DropTarget */
BBLIB_EXPORT int sh_get_uiobject(
    LPCITEMIDLIST pidl,
    LPITEMIDLIST* ppidlPath,
    LPITEMIDLIST* ppidlItem,
    struct IShellFolder **ppsfFolder,
    const struct _GUID riid,
    void **pObject
    );

/* lists of pidlists */
struct pidl_node
{
    struct pidl_node *next;
    short stuff[1];
};

#define first_pidl(p) ((LPCITEMIDLIST)(p)->stuff)

/* make above structure */
BBLIB_EXPORT struct pidl_node *make_pidl_node(LPCITEMIDLIST pidl);
/* make above structure from joined pidls */
BBLIB_EXPORT struct pidl_node *make_pidl_node2(LPCITEMIDLIST pidlA, LPCITEMIDLIST pidlB);
/* delete_that */
BBLIB_EXPORT void delete_pidl_list (struct pidl_node **ppList);
/* parse for concatenated multiple folders, like "STARTMENU|COMMON_STARTMENU" */
BBLIB_EXPORT struct pidl_node *get_folder_pidl_list(const char *paths);
/* make a copy */
BBLIB_EXPORT struct pidl_node *copy_pidl_list(const struct pidl_node *old_pidl_list);
/* are both equal ? */
BBLIB_EXPORT int equal_pidl_list(struct pidl_node *p1, struct pidl_node *p2);

/* enum_files */
enum ef_modes
{
    ef_folder = 1,
    ef_hidden = 2,
    ef_link = 4,
};

struct enum_files;

BBLIB_EXPORT int ef_getname(struct enum_files *ef, char *out);
BBLIB_EXPORT int ef_getpath(struct enum_files *ef, char *out);
BBLIB_EXPORT ULONG ef_getattr(struct enum_files *ef, int *pAttr);
BBLIB_EXPORT int ef_getpidl(struct enum_files *ef, struct pidl_node **pp);
BBLIB_EXPORT int ef_next(struct enum_files *ef);
BBLIB_EXPORT void ef_close(struct enum_files *ef);
BBLIB_EXPORT int ef_open(LPCITEMIDLIST pIDFolder, struct enum_files **ef);

/* folder changed notification register */
BBLIB_EXPORT UINT add_change_notify_entry(HWND hwnd, LPCITEMIDLIST pidl);
BBLIB_EXPORT void remove_change_notify_entry(UINT id_notify);
#define BB_FOLDERCHANGED 10897

/* shell execute pidl */
BBLIB_EXPORT int exec_pidl(LPCITEMIDLIST pidl, const char *verb);

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
# define COMCALL0(P,F) (P)->lpVtbl->F(P)
# define COMCALL1(P,F,A1) (P)->lpVtbl->F(P,A1)
# define COMCALL2(P,F,A1,A2) (P)->lpVtbl->F(P,A1,A2)
# define COMCALL3(P,F,A1,A2,A3) (P)->lpVtbl->F(P,A1,A2,A3)
# define COMCALL4(P,F,A1,A2,A3,A4) (P)->lpVtbl->F(P,A1,A2,A3,A4)
# define COMCALL5(P,F,A1,A2,A3,A4,A5) (P)->lpVtbl->F(P,A1,A2,A3,A4,A5)
# define COMCALL6(P,F,A1,A2,A3,A4,A5,A6) (P)->lpVtbl->F(P,A1,A2,A3,A4,A5,A6)
# define COMCALL7(P,F,A1,A2,A3,A4,A5,A6,A7) (P)->lpVtbl->F(P,A1,A2,A3,A4,A5,A6,A7)
# define COMREF(P) &P
#else
# define COMCALL0(P,F) (P)->F()
# define COMCALL1(P,F,A1) (P)->F(A1)
# define COMCALL2(P,F,A1,A2) (P)->F(A1,A2)
# define COMCALL3(P,F,A1,A2,A3) (P)->F(A1,A2,A3)
# define COMCALL4(P,F,A1,A2,A3,A4) (P)->F(A1,A2,A3,A4)
# define COMCALL5(P,F,A1,A2,A3,A4,A5) (P)->F(A1,A2,A3,A4,A5)
# define COMCALL6(P,F,A1,A2,A3,A4,A5,A6) (P)->F(A1,A2,A3,A4,A5,A6)
# define COMCALL7(P,F,A1,A2,A3,A4,A5,A6,A7) (P)->F(A1,A2,A3,A4,A5,A6,A7)
# define COMREF(P) P
#endif

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
#endif
