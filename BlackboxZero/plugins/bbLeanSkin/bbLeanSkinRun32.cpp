/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003-2009 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include <windows.h>
#include "engine/hookinfo.h"

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    HANDLE hAck32;

    while (' ' == *szCmdLine)
        ++szCmdLine;

    if (0 == *szCmdLine) {
        MessageBox(NULL, "For internal usage only.", "bbLeanSkinRun32.exe",
            MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
        return 1;
    }
    if (atoi(szCmdLine) != EntryFunc(ENGINE_GETVERSION)) {
        MessageBox(NULL, "Version mismatch: bbLeanSkinEng32.dll", "bbLeanSkinRun32.exe",
            MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
        return 1;
    }

    // OutputDebugString("Start Engine32");
    hAck32 = OpenEvent(EVENT_ALL_ACCESS, FALSE, BBLEANSKIN_RUN32EVENT);
    if (NULL == hAck32)
        return 1;

    EntryFunc(ENGINE_SETHOOKS);
    PulseEvent(hAck32);
    for (;;)
    {
        MSG msg;
        DWORD r;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        r = MsgWaitForMultipleObjects(1, &hAck32, FALSE, INFINITE, QS_ALLINPUT);
        if (r != WAIT_OBJECT_0 + 1)
            break;
    }
    EntryFunc(ENGINE_UNSETHOOKS);
    CloseHandle(hAck32);

    // OutputDebugString("Stop Engine32");
    return 0;
}

//===========================================================================
