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

#include "bbshell.h"
#include "win0x500.h"

#define false 0
#define true 1

const char *defaultrcPath(void);
int bbMB2WC(const char *src, WCHAR *wstr, int len);
int bbWC2MB(const WCHAR *src, char *str, int len);

/* ----------------------------------------------------------------------- */
int GetIDListSize(LPCITEMIDLIST pidl)
{
    int cb, c;
    if (NULL == pidl)
        return 0;
    c = sizeof(short);
    while (0 != (cb = cbID(pidl))) {
        c += cb;
        pidl = (LPCITEMIDLIST)((BYTE*)pidl + cb);
    }
    return c;
}

/* ----------------------------------------------------------------------- */
LPITEMIDLIST duplicateIDList(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlNew; int cb;
    if (NULL==pidl) return NULL;
    pidlNew = (LPITEMIDLIST)m_alloc(cb = GetIDListSize(pidl));
    memcpy(pidlNew, pidl, cb);
    return pidlNew;
}

/* ----------------------------------------------------------------------- */
LPITEMIDLIST joinIDLists(LPCITEMIDLIST pidlA, LPCITEMIDLIST pidlB)
{
    LPITEMIDLIST pidl; int cbA, cbB;
    cbA = GetIDListSize(pidlA);
    if (cbA) cbA -= sizeof(short);
    cbB = GetIDListSize(pidlB);
    pidl = (LPITEMIDLIST)m_alloc(cbA + cbB);
    memcpy(pidl, pidlA, cbA);
    memcpy((BYTE*)pidl + cbA, pidlB, cbB);
    return pidl;
}

void freeIDList(LPITEMIDLIST pidl)
{
    if (pidl)
        m_free(pidl);
}

void SHMalloc_Free(void * pidl)
{
    IMalloc *pMalloc;
    if (pidl && SUCCEEDED(SHGetMalloc(&pMalloc)))
    {
        COMCALL1(pMalloc, Free, pidl);
        COMCALL0(pMalloc, Release);
    }
}

int isEqualPIDL(LPCITEMIDLIST p1, LPCITEMIDLIST p2)
{
    int s1 = GetIDListSize(p1);
    int s2 = GetIDListSize(p2);
    return s1 == s2 && 0 == memcmp(p1, p2, s1);
}

/* ----------------------------------------------------------------------- */
#ifndef NAMELESS_MEMBER
#define NAMELESS_MEMBER(m) m
#endif

BOOL sh_getnameof(LPSHELLFOLDER piFolder, LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName)
{
    STRRET str;
    BOOL fDesktop = FALSE;
    BOOL fSuccess = TRUE;
    /* Check to see if a parent folder was specified.  If not, get a pointer */
    /* to the desktop folder. */
    if (NULL == piFolder)   
    {
        HRESULT hr = SHGetDesktopFolder(&piFolder);

        if (FAILED(hr))
            return (FALSE);

        fDesktop = TRUE;
    }
    /* Get the display name from the folder.  Then do any conversions necessary */
    /* depending on the type of string returned. */
    if (pidl && NOERROR == COMCALL3(piFolder, GetDisplayNameOf, pidl, dwFlags, &str))
    {
        /*StrRetToBufA(&str, pidl, pszName, MAX_PATH); */
        switch (str.uType)
        {
            case STRRET_WSTR:
                bbWC2MB(str.NAMELESS_MEMBER(pOleStr), pszName, MAX_PATH);
                SHMalloc_Free(str.NAMELESS_MEMBER(pOleStr));
                /*dbg_printf("WSTR: %s", pszName); */
                break;

            case STRRET_OFFSET:
                strcpy(pszName, (LPSTR)pidl + str.NAMELESS_MEMBER(uOffset));
                /*dbg_printf("OFFS: %s", pszName); */
                break;

            case STRRET_CSTR:
                strcpy(pszName, str.NAMELESS_MEMBER(cStr));
                /*dbg_printf("CSTR: %s", pszName); */
                break;

            default:
                fSuccess = FALSE;
                break;
        }
    }
    else
    {
        fSuccess = FALSE;
    }

    if (fDesktop)
        COMCALL0(piFolder, Release);

    return (fSuccess);
}

/* ----------------------------------------------------------------------- */
/* Function:    sh_get_folder_interface */
/* Purpose:     get the IShellfolder interface from PIDL */
/* ----------------------------------------------------------------------- */

struct IShellFolder* sh_get_folder_interface(LPCITEMIDLIST pIDFolder)
{
    struct IShellFolder* pShellFolder = NULL;
    struct IShellFolder* pThisFolder  = NULL;
    HRESULT hr;

    hr = SHGetDesktopFolder(&pShellFolder);
    if (NOERROR != hr)
        return NULL;

    if (NextID(pIDFolder) == pIDFolder)
        return pShellFolder;

    hr = COMCALL4(pShellFolder, BindToObject,
        pIDFolder,
        NULL,
        COMREF(IID_IShellFolder),
        (LPVOID*)&pThisFolder
        );

    COMCALL0(pShellFolder, Release);

    if (NOERROR != hr)
        return NULL;

    return pThisFolder;
}

/* ------------------------------------------------------------------------- */
/* support for DragSource / DropTarget / ContextMenu */

int sh_get_uiobject(
    LPCITEMIDLIST pidl,
    LPITEMIDLIST* ppidlPath,
    LPITEMIDLIST* ppidlItem,
    struct IShellFolder **ppsfFolder,
    const struct _GUID riid,
    void **pObject
    )
{
    LPITEMIDLIST p;
    HRESULT hr;

    *ppidlItem  = NULL;
    *ppsfFolder = NULL;
    *pObject    = NULL;

    p = *ppidlPath = duplicateIDList(pidl);
    if (NULL == p)
        return false;

    /*get last shitemid - the file */
    while (cbID(NextID(p)))
        p = NextID(p);

    *ppidlItem = duplicateIDList(p);
    cbID(p) = 0;

    if (NULL == (*ppsfFolder = sh_get_folder_interface(*ppidlPath)))
        return false;

    /*fake ITEMIDLIST - array for 1 file object */
    hr = COMCALL6(*ppsfFolder, GetUIObjectOf,
        (HWND)NULL,
        1, 
        (LPCITEMIDLIST*)ppidlItem, 
        COMREF(riid),
        NULL, 
        pObject
        );

    return SUCCEEDED(hr) && NULL != *pObject;
}

/* ----------------------------------------------------------------------- */
/* Function:    sh_getpidl */
/* Purpose:     get the PIDL for a file/folder by path-name. */
/* In:          pSF = folder interface (with relative paths) or NULL */
/* In:          path = file/folderpath to be parsed into an ITEMIDLIST. */
/* ----------------------------------------------------------------------- */

LPITEMIDLIST sh_getpidl (struct IShellFolder *pSF, const char *path)
{
    struct IShellFolder *mSF = pSF;
    ULONG chEaten;
    WCHAR wpath[MAX_PATH];
    LPITEMIDLIST pIDFolder;
    LPITEMIDLIST pID;
    char temp[MAX_PATH];
    HRESULT hr;

    if (NULL == mSF && NOERROR != SHGetDesktopFolder(&mSF))
        return NULL;

    replace_slashes(temp, path);

    MultiByteToWideChar(CP_ACP, 0, temp, -1, wpath, MAX_PATH);
    hr = COMCALL6(mSF, ParseDisplayName,
        NULL, 
        NULL, 
        wpath, 
        &chEaten, 
        &pIDFolder, 
        NULL
        );

    if (NULL == pSF)
        COMCALL0(mSF, Release);

    if (NOERROR!=hr)
        return NULL;

    pID = duplicateIDList(pIDFolder);
    SHMalloc_Free(pIDFolder);

    return pID;
}

/* ----------------------------------------------------------------------- */
/* Function:    sh_getfolderpath */
/* Purpose:     replacement for SHGetFolderPath, get path from CSIDL_ */
/* ----------------------------------------------------------------------- */

int sh_getfolderpath(char* szPath, UINT csidl)
{
    LPITEMIDLIST item;
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &item)))
    {
        BOOL result = SHGetPathFromIDList(item, szPath);
        SHMalloc_Free(item);
        return FALSE != result;
    }
    return false;
}

/* ----------------------------------------------------------------------- */
/* Function:    sh_get_icon_and_name */
/* Purpose: */
/* ----------------------------------------------------------------------- */

int sh_get_icon_and_name(LPCITEMIDLIST pID, HICON *pIcon, int iconsize, char *pName, int NameSize)
{
    static DWORD_PTR (WINAPI* pSHGetFileInfoW)(LPCWSTR,DWORD,SHFILEINFOW*,UINT,UINT);
    HIMAGELIST sysimgl;
    UINT cbfileinfo = SHGFI_PIDL;

    if (NULL == pSHGetFileInfoW) {
        if (GetVersion() & 0x80000000)
            *(DWORD_PTR*)&pSHGetFileInfoW = 1;
        else
            load_imp(&pSHGetFileInfoW, "shell32.dll", "SHGetFileInfoW");
    }

    if (pIcon) {
        cbfileinfo |= SHGFI_SYSICONINDEX | SHGFI_SHELLICONSIZE;
        if (iconsize < 20)
            cbfileinfo |= SHGFI_SMALLICON;
        if (iconsize >= 36)
            cbfileinfo |= SHGFI_LARGEICON;
        *pIcon = NULL;
    }

    if (pName) {
        cbfileinfo |= SHGFI_DISPLAYNAME;
        *pName = 0;
    }

    if (have_imp(pSHGetFileInfoW)) {
        SHFILEINFOW shinfo;
        shinfo.szDisplayName[0] = 0;
        sysimgl = (HIMAGELIST)pSHGetFileInfoW((LPWSTR)pID, 0, &shinfo, sizeof shinfo, cbfileinfo);
        if (sysimgl) {
            if (pName)
                bbWC2MB(shinfo.szDisplayName, pName, NameSize);
            if (pIcon)
                *pIcon = ImageList_GetIcon(sysimgl, shinfo.iIcon, ILD_NORMAL);
            return 1;
        }
    } else {
        SHFILEINFO shinfo;
        shinfo.szDisplayName[0] = 0;
        sysimgl = (HIMAGELIST)SHGetFileInfoA((LPCSTR)pID, 0, &shinfo, sizeof shinfo, cbfileinfo);
        if (sysimgl) {
            if (pName)
                strcpy_max(pName, shinfo.szDisplayName, NameSize);
            if (pIcon)
                *pIcon = ImageList_GetIcon(sysimgl, shinfo.iIcon, ILD_NORMAL);
            return 1;
        }
    }
    return 0;
}

/* ----------------------------------------------------------------------- */
/* Function:    sh_getdisplayname */
/* Purpose: */
/* ----------------------------------------------------------------------- */

char *sh_getdisplayname(LPCITEMIDLIST pID, char *buffer)
{
    sh_get_icon_and_name(pID, NULL, 0, buffer, MAX_PATH);
    return buffer;
}

/* ----------------------------------------------------------------------- */
/* Function:    sh_geticon */
/* Purpose: */
/* ----------------------------------------------------------------------- */

HICON sh_geticon(LPCITEMIDLIST pID, int iconsize)
{
    HICON hIcon;
    sh_get_icon_and_name(pID, &hIcon, iconsize, NULL, 0);
    return hIcon;
}

/* ----------------------------------------------------------------------- */
/* Function:    get_csidl */
/* Purpose:     for paths like 'APPDATA\myprog' - get the CSIDL index */
/*              of the special folder in front of a path and store the */
/*              rest of the path, if any, to the char** */
/* ----------------------------------------------------------------------- */

#define CSIDL_BLACKBOX  0x0032
#define CSIDL_CURTHEME  0x0033
#define LAST_CSIDL      0x0034
#define NO_CSIDL        -1

int get_csidl(const char **pPath)
{
    static const char idl[] = {
      "DESKTOP"                   /* 0x0000 <desktop> */
    "\0INTERNET"                  /* 0x0001 Internet Explorer (icon on desktop) */
    "\0PROGRAMS"                  /* 0x0002 Start Menu\Programs */
    "\0CONTROLS"                  /* 0x0003 My Computer\Control Panel */
    "\0PRINTERS"                  /* 0x0004 My Computer\Printers */
    "\0PERSONAL"                  /* 0x0005 My Documents */
    "\0FAVORITES"                 /* 0x0006 <user name>\Favorites */
    "\0STARTUP"                   /* 0x0007 Start Menu\Programs\Startup */
    "\0RECENT"                    /* 0x0008 <user name>\Recent */
    "\0SENDTO"                    /* 0x0009 <user name>\SendTo */
    "\0BITBUCKET"                 /* 0x000a <desktop>\Recycle Bin */
    "\0STARTMENU"                 /* 0x000b <user name>\Start Menu */
    "\0"                          /* 0x000c */
    "\0MYMUSIC"                   /* 0x000d My Documents\My Music */
    "\0"                          /* 0x000e */
    "\0"                          /* 0x000f */
    "\0DESKTOPDIRECTORY"          /* 0x0010 <user name>\Desktop */
    "\0DRIVES"                    /* 0x0011 My Computer */
    "\0NETWORK"                   /* 0x0012 Network Neighborhood */
    "\0NETHOOD"                   /* 0x0013 <user name>\nethood */
    "\0FONTS"                     /* 0x0014 windows\fonts */
    "\0TEMPLATES"                 /* 0x0015 */
    "\0COMMON_STARTMENU"          /* 0x0016 All Users\Start Menu */
    "\0COMMON_PROGRAMS"           /* 0X0017 All Users\Programs */
    "\0COMMON_STARTUP"            /* 0x0018 All Users\Startup */
    "\0COMMON_DESKTOPDIRECTORY"   /* 0x0019 All Users\Desktop */
    "\0APPDATA"                   /* 0x001a <user name>\Application Data */
    "\0PRINTHOOD"                 /* 0x001b <user name>\PrintHood */
    "\0LOCAL_APPDATA"             /* 0x001c <user name>\Local Settings\Applicaiton Data (non roaming) */
    "\0ALTSTARTUP"                /* 0x001d non localized startup */
    "\0COMMON_ALTSTARTUP"         /* 0x001e non localized common startup */
    "\0COMMON_FAVORITES"          /* 0x001f */
    "\0INTERNET_CACHE"            /* 0x0020 */
    "\0COOKIES"                   /* 0x0021 */
    "\0HISTORY"                   /* 0x0022 */
    "\0COMMON_APPDATA"            /* 0x0023 All Users\Application Data */
    "\0WINDOWS"                   /* 0x0024 GetWindowsDirectory() */
    "\0SYSTEM"                    /* 0x0025 GetSystemDirectory() */
    "\0PROGRAM_FILES"             /* 0x0026 C:\Program Files */
    "\0MYPICTURES"                /* 0x0027 C:\Program Files\My Pictures */
    "\0PROFILE"                   /* 0x0028 USERPROFILE */
    "\0SYSTEMX86"                 /* 0x0029 x86 system directory on RISC */
    "\0PROGRAM_FILESX86"          /* 0x002a x86 C:\Program Files on RISC */
    "\0PROGRAM_FILES_COMMON"      /* 0x002b C:\Program Files\Common */
    "\0PROGRAM_FILES_COMMONX86"   /* 0x002c x86 Program Files\Common on RISC */
    "\0COMMON_TEMPLATES"          /* 0x002d All Users\Templates */
    "\0COMMON_DOCUMENTS"          /* 0x002e All Users\Documents */
    "\0COMMON_ADMINTOOLS"         /* 0x002f All Users\Start Menu\Programs\Administrative Tools */
    "\0ADMINTOOLS"                /* 0x0030 <user name>\Start Menu\Programs\Administrative Tools */
    "\0CONNECTIONS"               /* 0x0031 Network and Dial-up Connections */

    /* --- other --- */
    "\0BLACKBOX"                  /* 0x0032 BLACKBOX HOME */
    "\0CURRENTTHEME"              /* 0x0033 */

    /* --- xoblite aliases --- */
    "\0PROGRAMFILES"              /* 0x0034 */
    "\0USERAPPDATA"               /* 0x0035 */
    "\0COMMONSTARTMENU"           /* 0x0036 */
    "\0."
    };

    static const char xob_ids[] = {
        0x0026, /*CSIDL_PROGRAM_FILES, */
        0x001a, /*CSIDL_APPDATA, */
        0x0016, /*CSIDL_COMMON_STARTMENU */
    };

    char buffer[MAX_PATH];
    const char *psub, *path, *cp;
    int k, l, c, id;

    if (NULL == (path = *pPath))
        return NO_CSIDL;

    /* let's see, if there is a subfolder */
    for (psub = path; 0 != (c = *psub); ++psub)
        if (IS_SLASH(c))
            break;

    l = psub - path;

    /* check xoblite/Litestep style special folders like $Blackbox$ */
    if (path[0]=='$' && l>=2 && path[l-1]=='$')
        /* we need upper letter case */
        path = strupr(extract_string(buffer, path+1, l -= 2));

    /* search the list above */
    if (l) {
        for (cp = idl, id = CSIDL_DESKTOP; '.' != *cp; id++, cp+=k+1) {
            k = strlen(cp);
            if (k == l && 0 == memcmp(path, cp, k)) {
                /* pointer to the subfolder, or NULL */
                *pPath = c ? psub : NULL;
                return (id < LAST_CSIDL) ? id : xob_ids[id - LAST_CSIDL];
            }
        }
    }
    return NO_CSIDL; /* not found */
}

/* ----------------------------------------------------------------------- */
/* Function: get_folder_pidl */
/* Purpose:  get the PIDL from path-string, with parsing for special */
/*           folders */
/* ----------------------------------------------------------------------- */

LPITEMIDLIST get_folder_pidl (const char *path)
{
    char temp[MAX_PATH], basedir[MAX_PATH], buffer[MAX_PATH];
    const char *tail_p, *rcdir;
    int id;
    LPITEMIDLIST pID1, pID;

    if (NULL == path)
        return NULL;

    fix_path(unquote(strcpy_max(temp, path, sizeof temp)));

    if (0 == temp[0])
        return NULL;

    if (is_absolute_path(temp)) {
        GetFullPathName(temp, sizeof buffer, buffer, NULL);
        return sh_getpidl(NULL, buffer);
    }

    tail_p = temp;
    id = get_csidl(&tail_p);

    if (NO_CSIDL == id || CSIDL_BLACKBOX == id || CSIDL_CURTHEME == id)
    {
        if (CSIDL_CURTHEME == id && !!(rcdir = defaultrcPath()))
            strcpy(basedir, rcdir);
        else
            get_exe_path(NULL, basedir, sizeof basedir);
        strcpy(temp, join_path(buffer, basedir, tail_p));
        GetFullPathName(temp, sizeof buffer, buffer, NULL);
        return sh_getpidl(NULL, buffer);
    }

    /* now for special folders, like CONTROLS */
    if (NOERROR != SHGetSpecialFolderLocation(NULL, id, &pID1))
        return NULL;

    if (NULL == tail_p) {
        pID = duplicateIDList(pID1);
    } else {
        struct IShellFolder* pThisFolder;
        pID = NULL;
        /* a subdirectory is specified, like APPDATA\microsoft */
        /* so get its local pidl and append it */
        pThisFolder = sh_get_folder_interface(pID1);
        if (pThisFolder) {
            LPITEMIDLIST pID2 = sh_getpidl(pThisFolder, tail_p+1);
            if (pID2) {
                pID = joinIDLists(pID1, pID2);
                freeIDList(pID2);
            }
            COMCALL0(pThisFolder, Release);
        }
    }

    SHMalloc_Free(pID1);
    return pID;
}

/* ----------------------------------------------------------------------- */
/* Function: get_folder_pidl_list */
/* Purpose:  get a list of pidls, specified by string, separated by '|' */
/* ----------------------------------------------------------------------- */

struct pidl_node *get_folder_pidl_list (const char *paths)
{
    struct pidl_node *new_pidl_list = NULL;
    char buffer[MAX_PATH];

    while (*NextToken(buffer, &paths, "|")) {
        LPITEMIDLIST pID = get_folder_pidl(buffer);
        if (pID) {
            append_node(&new_pidl_list, make_pidl_node(pID));
            freeIDList(pID);
        }
    }
    return new_pidl_list;
}

/* ------------------------------------------------------------------------- */
void delete_pidl_list (struct pidl_node **ppList)
{
    freeall(ppList);
}

/* ------------------------------------------------------------------------- */
struct pidl_node *copy_pidl_list(const struct pidl_node *old_pidl_list)
{
    struct pidl_node *new_pidl_list = NULL;
    const struct pidl_node *p;
    dolist (p, old_pidl_list)
        append_node(&new_pidl_list, make_pidl_node(first_pidl(p)));
    return new_pidl_list;
}

/* ------------------------------------------------------------------------- */
struct pidl_node *make_pidl_node(LPCITEMIDLIST pidl)
{
    struct pidl_node *p; int n;
    n = GetIDListSize(pidl);
    p = (struct pidl_node *)m_alloc(sizeof *p + n - sizeof p->stuff);
    memcpy(p->stuff, pidl, n);
    p->next = NULL;
    return p;
}

struct pidl_node *make_pidl_node2(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    struct pidl_node *p; int n1, n2;
    n1 = GetIDListSize(pidl1);
    if (n1) n1 -= sizeof(short);
    n2 = GetIDListSize(pidl2);
    p = (struct pidl_node *)m_alloc(sizeof *p + n1 + n2 - sizeof p->stuff);
    memcpy(p->stuff, pidl1, n1);
    memcpy((BYTE*)p->stuff + n1, pidl2, n2);
    p->next = NULL;
    return p;
}

int equal_pidl_list(struct pidl_node *p1, struct pidl_node *p2)
{
    int l1, l2;
    for (;p1 && p2; p1 = p1->next, p2 = p2->next) {
        l1 = GetIDListSize((LPCITEMIDLIST)p1->stuff);
        l2 = GetIDListSize((LPCITEMIDLIST)p2->stuff);
        if (l1 != l2 || 0 != memcmp(p1->stuff, p2->stuff, l1))
            break;
    }
    return p1 == p2;
}

/* ----------------------------------------------------------------------- */
/* Function:    replace_shellfolders */
/* Purpose:     parse for special folders and convert back to a path */
/*              string */
/* ----------------------------------------------------------------------- */

char *replace_shellfolders_from_base(
    char *buffer, const char *path, int search_path, const char *basepath)
{
    char temp[MAX_PATH];
    char basedir[MAX_PATH];
    const char *tail_p, *rcdir;
    int id;
    LPITEMIDLIST pID;
    HRESULT hr;

    fix_path(unquote(strcpy_max(temp, path, sizeof temp)));

    if (0 == temp[0] || is_absolute_path(temp))
        return strcpy(buffer, temp);

    tail_p = temp;
    id = get_csidl(&tail_p);

    if (NO_CSIDL == id && search_path)
        return strcpy(buffer, temp);

    if (NO_CSIDL == id || CSIDL_BLACKBOX == id || CSIDL_CURTHEME == id)
    {
        if (NULL == basepath || NO_CSIDL != id) {
            if (CSIDL_CURTHEME == id && !!(rcdir = defaultrcPath()))
                basepath = rcdir;
            else
                basepath = get_exe_path(NULL, basedir, sizeof basedir);
        }
        return join_path(buffer, basepath, tail_p);
    }

    /* special folders, like CONTROLS */
    hr = SHGetSpecialFolderLocation(NULL, id, &pID);
    if (NOERROR == hr) {
        /* returns also things like "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" */
        /* (unlike SHGetPathFromIDList) */
        BOOL result = sh_getnameof(NULL, pID, SHGDN_FORPARSING, basedir);
        SHMalloc_Free(pID);
        if (result)
            return join_path(buffer, basedir, tail_p);
    }

    return strcpy(buffer, temp);
}

char *replace_shellfolders(char *buffer, const char *path, int search_path)
{
    return replace_shellfolders_from_base(buffer, path, search_path, NULL);
}

/* ------------------------------------------------------------------------- */

#ifndef SFGAO_HIDDEN
#define SFGAO_HIDDEN 0x00080000L /* hidden object */
#endif

struct enum_files
{
    LPMALLOC pMalloc;
    LPCITEMIDLIST pIDFolder;
    struct IShellFolder* pThisFolder;
    LPENUMIDLIST pEnumIDList;
    LPITEMIDLIST pID;
};

int ef_getname(struct enum_files *ef, char *out)
{
    out[0] = 0;
    return sh_getnameof(ef->pThisFolder, ef->pID, SHGDN_NORMAL, out);
}

int ef_getpath(struct enum_files *ef, char *out)
{
    out[0] = 0;
    return sh_getnameof(ef->pThisFolder, ef->pID, SHGDN_FORPARSING, out);
}

ULONG ef_getattr(struct enum_files *ef, int *pAttr)
{
    ULONG uAttr = SFGAO_FOLDER|SFGAO_LINK|SFGAO_HIDDEN;
    HRESULT hr;
    *pAttr = 0;
    hr = COMCALL3(ef->pThisFolder, GetAttributesOf, 1, (LPCITEMIDLIST*)&ef->pID, &uAttr);
    if (FAILED(hr))
        return 0;
    if (uAttr & SFGAO_HIDDEN)
        *pAttr |= ef_hidden;
    if (uAttr & SFGAO_FOLDER)
        *pAttr |= ef_folder;
    if (uAttr & SFGAO_LINK)
        *pAttr |= ef_link;
    return 1;
}

int ef_getpidl(struct enum_files *ef, struct pidl_node **pp)
{
    *pp = make_pidl_node2(ef->pIDFolder, ef->pID);
    return 1;
}

int ef_next(struct enum_files *ef)
{
    ULONG nReturned;
    HRESULT hr;

    if (ef->pID)
        COMCALL1(ef->pMalloc, Free, ef->pID), ef->pID = NULL;

    hr = COMCALL3(ef->pEnumIDList, Next, 1, &ef->pID, &nReturned);

    if (S_FALSE == hr || 1 != nReturned)
        return 0;
    return 1;
}

void ef_close(struct enum_files *ef)
{
    if (ef->pID)
        COMCALL1(ef->pMalloc, Free, ef->pID);
    if (ef->pEnumIDList)
        COMCALL0(ef->pEnumIDList, Release);
    if (ef->pMalloc)
        COMCALL0(ef->pMalloc, Release);
    if (ef->pThisFolder)
        COMCALL0(ef->pThisFolder,Release);
    m_free(ef);
}

int ef_open(LPCITEMIDLIST pIDFolder, struct enum_files **pp)
{
    struct enum_files *ef = c_new(struct enum_files);

    /* nothing to do on NULL pidl's */
    if (NULL==pIDFolder)
        goto fail;

    ef->pIDFolder = pIDFolder;

    /* get a COM interface of the folder */
    ef->pThisFolder = sh_get_folder_interface(ef->pIDFolder);
    if (NULL == ef->pThisFolder)
        goto fail;

    SHGetMalloc(&ef->pMalloc);
    if (NULL == ef->pMalloc)
        goto fail;

    /* get the folders "EnumObjects" interface */
    COMCALL3(ef->pThisFolder, EnumObjects,
        NULL,
        SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN,
        &ef->pEnumIDList
        );

    if (NULL == ef->pEnumIDList)
        goto fail;

    *pp = ef;
    return 1;
fail:
    ef_close(ef);
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Functions: add/remove_change_notify_entry */
/* ------------------------------------------------------------------------- */

#ifndef SHCNF_ACCEPT_INTERRUPTS
#define SHCNF_ACCEPT_INTERRUPTS 0x0001 
#define SHCNF_ACCEPT_NON_INTERRUPTS 0x0002
#define SHCNF_NO_PROXY 0x8000
#endif

#ifndef SHCNE_DISKEVENTS
#define SHCNE_DISKEVENTS    0x0002381FL
#define SHCNE_GLOBALEVENTS  0x0C0581E0L /* Events that dont match pidls first */
#define SHCNE_ALLEVENTS     0x7FFFFFFFL
#define SHCNE_INTERRUPT     0x80000000L /* The presence of this flag indicates */
#endif

struct __SHChangeNotifyEntry
{
    LPCITEMIDLIST pidl;
    BOOL fRecursive;
};

UINT (WINAPI *pSHChangeNotifyRegister)(
    HWND hWnd, 
    DWORD dwFlags, 
    LONG wEventMask, 
    UINT uMsg, 
    DWORD cItems,
    struct __SHChangeNotifyEntry *lpItems
    );

BOOL (WINAPI *pSHChangeNotifyDeregister)(UINT ulID);

UINT add_change_notify_entry(HWND hwnd, LPCITEMIDLIST pidl)
{
    struct __SHChangeNotifyEntry E;
    UINT id_notify;

    if (load_imp(&pSHChangeNotifyRegister, "SHELL32.DLL", (const char*)2)) {
        E.pidl = pidl;
        E.fRecursive = FALSE;
        id_notify = pSHChangeNotifyRegister(
                hwnd,
                SHCNF_ACCEPT_INTERRUPTS|SHCNF_ACCEPT_NON_INTERRUPTS|SHCNF_NO_PROXY,
                SHCNE_ALLEVENTS,
                BB_FOLDERCHANGED,
                1,
                &E
                );
        return id_notify;
    } else {
        return 0;
    }
}

void remove_change_notify_entry(UINT id_notify)
{
    if (load_imp(&pSHChangeNotifyDeregister, "SHELL32.DLL", (const char*)4))
        pSHChangeNotifyDeregister(id_notify);
}

/* ------------------------------------------------------------------------- */
/* Function: exec_pidl */

int exec_pidl(LPCITEMIDLIST pidl, LPCSTR verb)
{
    char workdir[MAX_PATH];
    SHELLEXECUTEINFO sei;

    fix_path(get_exe_path(NULL, workdir, sizeof workdir));

    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
    /*sei.hwnd = NULL; */
    sei.lpVerb = verb;
    /*sei.lpFile = NULL; */
    /*sei.lpParameters = NULL; */
    sei.lpDirectory = workdir;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpIDList = (void*)pidl;
    return ShellExecuteEx(&sei);
}

/* ------------------------------------------------------------------------- */

