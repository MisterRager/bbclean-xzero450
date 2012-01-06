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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    static const char szAppName[] = "bsetbg";

    BOOL                bRet;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    DWORD               dwRet;
    MSG                 msg;

    char                bsetroot_command[4000];
    char                *p;

    if (0 == *lpCmdLine || 0==memcmp(lpCmdLine, "-help", 2))
    {
        MessageBox(NULL,
            "bsetbg is a tool for Blackbox for Windows"
            "\nCopyright © 2001-2003 The Blackbox for Windows DevTeam"
            "\nCopyright © 2004-2009 grischka"
            "\nThis bsetbg passes switches and arguments to bsetroot"
            , szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
        return 1;
    }

    GetModuleFileName(NULL, bsetroot_command, MAX_PATH);
    p = strrchr(bsetroot_command, '\\');
    if (p) ++p;
    else p = bsetroot_command;
    strcat(strcpy(p, "bsetroot.exe "), lpCmdLine);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    bRet = CreateProcess (NULL, bsetroot_command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (FALSE == bRet)
    {
        MessageBox(NULL, "Error: bsetbg.exe could not find bsetroot.exe\t",
            szAppName, MB_OK|MB_ICONERROR|MB_TOPMOST|MB_SETFOREGROUND);
        return 1;
    }

    CloseHandle (pi.hThread);

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));

    WaitForSingleObject (pi.hProcess, INFINITE);
    GetExitCodeProcess  (pi.hProcess, (DWORD*)&dwRet);
    CloseHandle (pi.hProcess);
    return dwRet;
}

//===========================================================================
