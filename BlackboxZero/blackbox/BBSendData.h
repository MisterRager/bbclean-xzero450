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

/*
  This file implements a common way to send and receive messages and data
  across processes.  It's used to send broams, style properties, to
  register messages, etc.  It is included in blackbox.exe as well as
  in other applications like bbStyleMaker, bbNote, ...

  Usage example: Set a style:
    BBSendData(
        FindWindow("BlackboxClass", "Blackbox"),
        BB_SETSTYLE,
        stylefile,
        1+strlen(stylefile)
        );
*/

struct bb_senddata
{
    WPARAM wParam;
    BYTE lParam_data[2000];
};

extern BOOL (WINAPI* pIsWow64Message)(VOID);

BOOL BBSendData(
    HWND hwnd,          // window
    UINT msg,           // the message to send
    WPARAM wParam,      // wParam
    LPCVOID lParam,     // lParam
    int lParam_size     // size of the data referenced by lParam
    )
{
    struct bb_senddata BBSD, *pBBSD;
    unsigned size;
    COPYDATASTRUCT cds;
    BOOL result = FALSE;
    BYTE *data;

    if (NULL == hwnd)
        return result;

    if (-1 == lParam_size)
        lParam_size = 1+strlen((const char*)lParam);

    size = lParam_size + offsetof(bb_senddata, lParam_data);

    // for speed, the local buffer is used when sufficient
    if (size <= sizeof BBSD)
        pBBSD = &BBSD;
    else
        pBBSD = (struct bb_senddata*)malloc(size);

    pBBSD->wParam = wParam;
    data = pBBSD->lParam_data;

#if defined _WIN64 && defined __BBCORE__
    if (have_imp(pIsWow64Message) && pIsWow64Message())
        data -= 4, size -= 4;
    // dbg_printf("1 IsWow64Message: %d %d", pIsWow64Message(), size);
#endif

    memcpy(data, lParam, lParam_size);
    cds.dwData = msg;
    cds.cbData = size;
    cds.lpData = (void*)pBBSD;
    result = (BOOL)SendMessage (hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
    if (pBBSD != &BBSD)
        free(pBBSD);
    return result;
}


//=====================================================
UINT BBReceiveData(HWND hwnd, LPARAM lParam, int (*fn) (
        HWND hwnd, UINT msg, WPARAM wParam, const void *data, unsigned data_size
        ))
{
    struct bb_senddata *pBBSD;
    UINT msg;
    WPARAM wParam;
    const BYTE *data;
    unsigned size;

    msg = (UINT)((COPYDATASTRUCT*)lParam)->dwData;
    if (msg < BB_MSGFIRST || msg >= BB_MSGLAST)
        return TRUE;

    pBBSD = (struct bb_senddata*)((COPYDATASTRUCT*)lParam)->lpData;
    wParam = pBBSD->wParam;
    data = pBBSD->lParam_data;
    size = ((PCOPYDATASTRUCT)lParam)->cbData - offsetof(bb_senddata, lParam_data);

#if defined _WIN64 && defined __BBCORE__
    if (have_imp(pIsWow64Message) && pIsWow64Message())
        data -= 4, size += 4, wParam = (DWORD)wParam;
    // dbg_printf("2 IsWow64Message: %d %d %x", pIsWow64Message(), size, msg);
#endif

    if (fn && fn(hwnd, msg, wParam, data, size))
        ;
    else if (BB_SENDDATA == msg)
        memcpy((void*)wParam, data, size);
    else
        SendMessage(hwnd, msg, wParam, (LPARAM)data);
    return TRUE;
}

//=====================================================
