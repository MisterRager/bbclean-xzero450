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
*/ // PIDL.H

#ifndef __BBPIDL_H_
#define __BBPIDL_H_

struct _ITEMIDLIST;
struct IShellFolder;

// Free shell items
void SHMalloc_Free(void* whatever);

int GetIDListSize(const _ITEMIDLIST * pidl);

// join two lists, uses ordinary "malloc"
_ITEMIDLIST *joinIDlists(const _ITEMIDLIST *pidlA, const _ITEMIDLIST *pidlB);

// copy list, uses ordinary "malloc"
_ITEMIDLIST *duplicateIDlist(const _ITEMIDLIST * pidl);

BOOL sh_get_displayname(IShellFolder *piFolder, const _ITEMIDLIST *pidl, DWORD dwFlags, LPTSTR pszName);

// Get the Com Interface from pidl
IShellFolder* sh_get_folder_interface(const _ITEMIDLIST *pIDFolder);

// Get pidl from path, parsing SPECIAL folders, defaults to blackbox directory
_ITEMIDLIST *get_folder_pidl(const char *path);

// Get pidl from path, no parsing
_ITEMIDLIST *sh_getpidl(IShellFolder *pSF, const char *path);

// Get path from csidl, replacement for SHGetFolderPath
bool sh_getfolderpath(LPSTR szPath, UINT csidl);

// parsing SPECIAL folders and convert to string again, defaults to blackbox directory
char *replace_shellfolders(char *buffer, const char *path, bool search_path);

// retrieve the shell icon
HICON sh_geticon (const _ITEMIDLIST *pID, int iconsize);

// retrieve the default display name
void sh_getdisplayname (const _ITEMIDLIST *pID, char *buffer);

//===========================================================================
struct pidl_node
{
	struct pidl_node *next;
	_ITEMIDLIST *v;
};

// like get_folder_pidl, parse for concatenated multiple folders, like "STARTMENU|COMMON_STARTMENU"
struct pidl_node *get_folder_pidl_list(const char *paths);

// delete_that
void delete_pidl_list (struct pidl_node **ppList);

// and make a copy
struct pidl_node *copy_pidl_list(const struct pidl_node *old_pidl_list);

// support for ContextMenu, DragSource, DropTarget
bool sh_get_uiobject(
	const _ITEMIDLIST * pidl,
	_ITEMIDLIST ** ppidlPath,
	_ITEMIDLIST ** ppidlItem,
	struct IShellFolder **ppsfFolder,
	const struct _GUID riid,
	void **pObject
	);

//===========================================================================
extern "C" HRESULT WINAPI StrRetToBufA(
	struct _STRRET *pstr,
	const UNALIGNED _ITEMIDLIST *pidl,
	LPSTR pszBuf,
	UINT cchBuf
	);

//===========================================================================

#define NextID(pidl) ((LPITEMIDLIST)((BYTE*)pidl+pidl->mkid.cb))

//===========================================================================
#endif
