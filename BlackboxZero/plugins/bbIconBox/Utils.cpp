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
#include "drawico.h"

#ifndef TTF_TRACK
#define TTF_TRACK 0x0020
#endif

// ----------------------------------------------
#if 1
// ----------------------------------------------
#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040   // Use the new dialog layout with the ability to resize
#endif
#ifndef BIF_USENEWUI
#define BIF_USENEWUI (BIF_NEWDIALOGSTYLE | BIF_EDITBOX)
#endif

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            POINT p; //GetCursorPos(&p);
            RECT m; GetMonitorRect(&p, &m, GETMON_WORKAREA|GETMON_FROM_POINT);
            RECT r; GetWindowRect(hwnd, &r);
            // center screen
            p.x = (m.left + m.right - r.right + r.left) / 2;
            p.y = (m.top + m.bottom - r.bottom + r.top) / 2;
            SetWindowPos(hwnd, NULL, p.x, p.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
            const char **info = (const char**)lpData;

            SetWindowText(hwnd, info[0]);
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)info[1]);
            break;
        }
    }
    return 0;
}

bool select_folder(HWND hwnd, const char *title, char *name, char *path)
{
    BROWSEINFO bi;
    LPITEMIDLIST p;
    LPMALLOC pMalloc = NULL;
    SHGetMalloc(&pMalloc);

    const char *info[2] = { title, path };

    bi.hwndOwner    = hwnd;
    bi.pidlRoot     = NULL;
    bi.pszDisplayName = name;
    bi.lpszTitle    = "Select Folder...";
    bi.ulFlags      = BIF_USENEWUI;
    bi.lpfn         = BrowseCallbackProc;
    bi.lParam       = (LPARAM)info;
    bi.iImage       = 0;
    p = SHBrowseForFolder(&bi);
    path[0] = 0;
    if (p)
    {
        SHGetPathFromIDList(p, path);
        pMalloc->Free(p);
    }
    pMalloc->Release();
    return NULL != p;
}

// ----------------------------------------------
#endif
// ----------------------------------------------

void EnumTray (TRAYENUMPROC lpEnumFunc, LPARAM lParam)
{
    for (int i = 0, s = GetTraySize(); i < s; i++)
        if (FALSE == lpEnumFunc(GetTrayIcon(i), lParam))
            break;
}

void EnumDesks (DESKENUMPROC lpEnumFunc, LPARAM lParam)
{
    DesktopInfo info;
    info.deskNames = NULL;
    GetDesktopInfo(&info);
    string_node *p = info.deskNames;
    for (int n = 0; n < info.ScreensX; n++)
    {
        DesktopInfo DI;
        DI.number = n;
        DI.deskNames = info.deskNames;
        DI.isCurrent = n == info.number;
        DI.name[0] = 0;
        if (p)
        {
            strcpy(DI.name, p->str);
            p = p->next;
        }
        if (FALSE == lpEnumFunc(&DI, lParam))
            break;
    }
}

void EnumTasks (TASKENUMPROC lpEnumFunc, LPARAM lParam)
{
    const struct tasklist *tl;
    dolist (tl, GetTaskListPtr())
        if (FALSE == lpEnumFunc(tl, lParam))
            break;
}

#ifndef XOB
void free_task_list(void) { }
void new_task_list(void) { }
#endif

// ----------------------------------------------

// ----------------------------------------------
#ifdef LINK_EXTRACT_ICON
// experimental stuff, pointed out by IronHead "The EmergeDesktopian".
// Nice method to extract 48px+ icons, anyway.

HICON extract_icon(IShellFolder *pFolder, LPCITEMIDLIST pidlRelative, int iconSize)
{
    char iconLocation[MAX_PATH];
    IExtractIcon *extractIcon;
    int iconIndex;
    UINT iconFlags;
    HICON icon = NULL;
    HICON icon2 = NULL;
    HRESULT hr;

    hr = pFolder->GetUIObjectOf(
        NULL, 1, &pidlRelative, IID_IExtractIcon, NULL, (void**)&extractIcon);

    if (FAILED(hr)) {
        //dbg_printf("failed GetUIObjectOf");
        return icon;
    }

    hr = extractIcon->GetIconLocation(GIL_FORSHELL, iconLocation, sizeof iconLocation, &iconIndex, &iconFlags);

    if (FAILED(hr)) {
        //dbg_printf("failed GetIconLocation");
        return icon;
    }

    //iconSize = 8 << iconSize;

    if (iconSize <= 16)
        extractIcon->Extract(iconLocation, iconIndex, &icon, &icon2, MAKELONG(16, 16));
    else if (iconSize <= 32)
        extractIcon->Extract(iconLocation, iconIndex, &icon, &icon2, MAKELONG(32, 16));
    else if (iconSize <= 48)
        extractIcon->Extract(iconLocation, iconIndex, &icon, &icon2, MAKELONG(48, 16));
    else
        extractIcon->Extract(iconLocation, iconIndex, &icon, &icon2, MAKELONG(64, 16));

    extractIcon->Release();

    //dbg_printf("GetIconLocation %x %x %x %x %s", iconFlags, icon, icon2, iconIndex, iconLocation);

    if (NULL == icon) {
        icon = icon2;
        if (NULL == icon) {
            icon = LoadIcon(NULL, IDI_APPLICATION);
    }}

    return icon;
}

// ----------------------------------------------

// ----------------------------------------------
// This one portion was figured out by IronHead of emergeDesktop Fame.

HICON EGGetIcon(WCHAR *file, UINT iconSize)
{
    HICON icon = NULL, tmpIcon = NULL;

    IShellFolder *deskFolder = NULL;
    IShellFolder *appObject = NULL;
    LPITEMIDLIST pidlLocal = NULL;
    LPITEMIDLIST pidlRelative = NULL;
    IExtractIcon *extractIcon = NULL;
    WCHAR iconLocation[MAX_PATH];
    int iconIndex = 0;
    UINT iconFlags = 0;
    HRESULT hr;

    if (MSILFree == NULL)
        MSILFree = (fnILFree)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), (LPCSTR)155);
//        MSILFree = (fnILFree)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), "ILFree");

    if (MSILClone == NULL)
        MSILClone = (fnILClone)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), (LPCSTR)18);
//        MSILClone = (fnILClone)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), "ILClone");

    if (MSILFindLastID == NULL)
        MSILFindLastID = (fnILFindLastID)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), (LPCSTR)16);
//        MSILFindLastID = (fnILFindLastID)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), "ILFindLastID");

    if (MSILRemoveLastID == NULL)
        MSILRemoveLastID = (fnILRemoveLastID)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), (LPCSTR)17);
//        MSILRemoveLastID = (fnILRemoveLastID)GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), "ILRemoveLastID");

    hr = SHGetDesktopFolder(&deskFolder);
    if (FAILED(hr))
        return icon;

    hr = deskFolder->ParseDisplayName(NULL, NULL, file, NULL, &pidlLocal, NULL);
    if (FAILED(hr))
    {
        deskFolder->Release();
        return icon;
    }

    pidlRelative = MSILClone(MSILFindLastID(pidlLocal));
    MSILRemoveLastID(pidlLocal);

    hr = deskFolder->BindToObject(pidlLocal, NULL, IID_IShellFolder, (void**)&appObject);
    if (FAILED(hr))
    {
        deskFolder->Release();
        MSILFree(pidlLocal);
        return icon;
    }

    MSILFree(pidlLocal);

    hr = appObject->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlRelative, IID_IExtractIcon, NULL,
        (void**)&extractIcon);
    if (FAILED(hr))
    {
        deskFolder->Release();
        appObject->Release();
        MSILFree(pidlRelative);
        return icon;
    }

    MSILFree(pidlRelative);

    hr = extractIcon->GetIconLocation(0, iconLocation, MAX_PATH, &iconIndex, &iconFlags);
    if (FAILED(hr))
    {
        deskFolder->Release();
        appObject->Release();
        return icon;
    }

    if (iconSize == 16)
        hr = extractIcon->Extract(iconLocation, iconIndex, &tmpIcon, &icon, MAKELONG(32, 16));
    else
        hr = extractIcon->Extract(iconLocation, iconIndex, &icon, &tmpIcon, MAKELONG(iconSize, 16));

    if (SUCCEEDED(hr))
        DestroyIcon(tmpIcon);

    extractIcon->Release();
    appObject->Release();
    deskFolder->Release();

    return icon;
}

int sh_get_icon_and_name(LPCITEMIDLIST pID, HICON *pIcon, int iconsize, char *szTip, int TipSize)
{
    *pIcon = extract_icon(pThisFolder, pID, iconsize);
    szTip[0] = 0;
    sh_get_displayname(pThisFolder, pID, SHGDN_NORMAL, szTip);
}

#endif
// ----------------------------------------------

