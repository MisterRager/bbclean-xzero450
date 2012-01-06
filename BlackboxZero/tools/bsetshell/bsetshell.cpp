/* ========================================================================

  bsetshell - a shell changer for Blackbox for Windows under 9x/ME/2k/XP

  This file is part of the bbLean source code
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2) For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================ */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define IDD_DLG1 1000
#define IDC_RBN1 1001
#define IDC_RBN2 1002
#define IDC_RBN3 1003
#define IDC_CHK1 1004
#define IDC_EDT1 1005
#define IDC_LOG1 1006
#define IDC_GRP1 1007
#define IDC_GRP2 1008
#define IDC_INFO 1009

#define APPNAME "bsetshell"

#ifndef RC_INVOKED
#define ST

ST char szBlackbox[MAX_PATH];
ST char shellpath[MAX_PATH];
ST HWND g_hDlg;

ST bool is_usingNT;
ST bool is_admin;
ST bool is_per_user;
ST bool is_blackbox;

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif
#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif
#ifndef GWLP_USERDATA
# define DWORD_PTR unsigned long
# define LONG_PTR long
#endif

/* ======================================================================== */

void dbg_printf (const char *fmt, ...)
{
    char buffer[4000];
    va_list arg;
    va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    strcat(buffer, "\n");
    OutputDebugString(buffer);
}

int message(int flg, const char *fmt, ...)
{
    char buffer[4000];
    va_list arg;
    va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    return MessageBox (g_hDlg, buffer, APPNAME, flg);
}

/* ======================================================================== */

ST char inimapstr       [] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\system.ini\\boot";
ST char sys_bootOption  [] = "SYS:Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
ST char usr_bootOption  [] = "USR:Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
ST char logonstr        [] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
ST char szExplorer      [] = "explorer.exe";

enum {
    A_RD = 1,
    A_WR = 2,
    A_DEL = 4,
    A_DW = 8,
    A_SZ = 16,
    A_TEST = A_WR|A_DEL
};

ST bool rw_reg(int action, HKEY root, const char *ckey, const char *cval, const char *cdata)
{
    HKEY k;
    LONG r;
    DWORD result, type, options;
    options = (action & A_RD) ? KEY_READ : KEY_WRITE | WRITE_OWNER;
/*
    // need both ?
    if (usingx64)
        options |= KEY_WOW64_64KEY ? KEY_WOW64_32KEY;
*/
    r = RegCreateKeyEx(root, ckey, 0, NULL, REG_OPTION_NON_VOLATILE,
        options, NULL, &k, &result);

    if (ERROR_SUCCESS == r) {
        if (A_RD & action) {
            if (A_DW & action) {
                result = sizeof(DWORD);
                r = RegQueryValueEx(k, cval, NULL, &type, (BYTE*)cdata, &result);
            } else if (A_SZ & action) {
                result = MAX_PATH;
                r = RegQueryValueEx(k, cval, NULL, &type, (BYTE*)cdata, &result);
            }
        } else {
            if (A_WR & action) {
                if (A_DW & action) {
                    r = RegSetValueEx(k, cval, 0, REG_DWORD, (const BYTE*)&cdata, sizeof(DWORD));
                } else if (A_SZ & action) {
                    r = RegSetValueEx(k, cval, 0, REG_SZ, (const BYTE*)cdata, strlen(cdata)+1);
                }
            }
            if (ERROR_SUCCESS == r) {
                if (A_DEL & action) {
                    r = RegDeleteValue(k, cval);
                }
            }
        }
        RegCloseKey(k);
    }

#if 0
    dbg_printf("(%d) %s %s %s %s %s",
        ERROR_SUCCESS == r,
        A_WR & action ? "WR" : A_DEL & action ? "DEL" : "RD",
        root == HKEY_LOCAL_MACHINE ? "LM" : "CU",
        ckey, cval, cdata);
#endif

    return ERROR_SUCCESS == r;
}                

/* read/write from/to system.ini (for windows 9x/me) */
ST bool rw_ini (int action, char *shell)
{
    char path[MAX_PATH];
    GetWindowsDirectory(path, sizeof(path));
    strcat(path, "\\SYSTEM.INI");
    if (action & A_RD)
        return 0 != GetPrivateProfileString("Boot", "Shell", "", shell, MAX_PATH, path);
    if (action & A_WR)
        return 0 != WritePrivateProfileString("Boot", "Shell", shell, path);
    return 0;
}

ST bool get_shell(char *buffer)
{
    buffer[0] = 0;
    if (0 == is_usingNT)
        return rw_ini(A_RD, buffer);
    else if (is_per_user)
        return rw_reg(A_RD|A_SZ, HKEY_CURRENT_USER, logonstr, "Shell", buffer);
    else
        return rw_reg(A_RD|A_SZ, HKEY_LOCAL_MACHINE, logonstr, "Shell", buffer);
}

ST bool get_per_user(void)
{
    char buffer[MAX_PATH];
    buffer[0] = 0;
    rw_reg(A_RD|A_SZ, HKEY_LOCAL_MACHINE, inimapstr, "Shell", buffer);
    return 0 == memicmp(buffer, "USR:", 4);
}

/* ======================================================================== */

ST INT_PTR CALLBACK dlgfunc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int id, f, option;
    char buffer[MAX_PATH], temp[2000];

    switch( msg ) {
    case WM_DESTROY:
        g_hDlg = NULL;
        return 0;

    case WM_INITDIALOG:
        g_hDlg = hDlg;
        option = 0;

        is_usingNT = 0 == (GetVersion() & 0x80000000);
        if (is_usingNT) {
            /* test if we could set in HKLM, if so we are admin */
            is_admin = rw_reg(A_TEST|A_SZ, HKEY_LOCAL_MACHINE, inimapstr, "bsetshell", "test");
            /* check if per user shell is set */
            is_per_user = get_per_user();
        } else {
            /* dont bother on win9x */
            is_per_user = 0;
            is_admin = 1;
        }

        /* bsetshell should be where blackbox.exe is. If so, get it's full path */
        if (0 == SearchPath(NULL, "blackbox.exe", NULL, MAX_PATH, szBlackbox, NULL))
            strcpy(szBlackbox, "(blackbox.exe not found)");

        if (0 == is_usingNT) {
            EnableWindow(GetDlgItem(hDlg, IDC_CHK1), FALSE);

        } else if (0 == is_admin) {
            /* if not admin, see if a shell is set for this user */
            is_per_user = rw_reg(A_RD|A_SZ, HKEY_CURRENT_USER, logonstr, "Shell", buffer);
            SetDlgItemText(hDlg, IDC_CHK1, "Set shell for this user &individually");
        }

        CheckDlgButton(hDlg, IDC_CHK1, is_per_user ? BST_CHECKED : BST_UNCHECKED);

display:
        if (is_usingNT) {
            if (0 == is_per_user) {
                strcpy(buffer, "Shell for all users");
                if (0 == is_admin)
                    strcat(buffer, " (run as admin to change)");

            } else {
                DWORD result = sizeof temp;
                temp[0] = 0;
                GetUserName(temp, &result);
                sprintf(buffer, "Shell for the current user (%s)", temp);
            }

            SetDlgItemText(hDlg, IDC_GRP1, buffer);
        }

        if (2 == option)
            strcpy(buffer, szBlackbox);
        else
        if (1 == option)
            strcpy(buffer, szExplorer);
        else
            get_shell(buffer);

        strlwr(strcpy(temp, buffer));

        if (0 == stricmp(temp, szBlackbox)) {
            id = IDC_RBN1;
        } else if (strstr(temp, "explorer")) {
            id = IDC_RBN2;
        } else if (temp[0]) {
            id = IDC_RBN3;
        } else {
            strcpy(buffer, "<not set>");
            id = 0;
        }

        CheckRadioButton(hDlg, IDC_RBN1, IDC_RBN3, id);
        for (f = IDC_RBN1; f <= IDC_RBN3; ++f)
            EnableWindow(GetDlgItem(hDlg, f), is_admin || is_per_user);

set_line:
        {
            HWND hLine = GetDlgItem(hDlg, IDC_EDT1);
            SetWindowText(hLine, buffer);
            SendMessage(hLine, EM_SETREADONLY, id != IDC_RBN3, 0);
        }
        return 1;


    case WM_COMMAND:
        id = LOWORD( wParam );
        switch(id) {
        case IDC_CHK1:
            is_per_user = 0 != IsDlgButtonChecked(hDlg, id);
            option = 0;
            goto display;

        case IDC_RBN1:
            option = 2;
            goto display;

        case IDC_RBN2:
            option = 1;
            goto display;

        case IDC_RBN3:
            buffer[0] = 0;
            goto set_line;

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return 1;

        case IDOK:
        case IDC_LOG1:
            /* okay, now really set it */
            GetDlgItemText(hDlg, IDC_EDT1, shellpath, sizeof shellpath);
            if (strcmp(shellpath, "explorer.exe")) {
                if (0 == shellpath[0]
                    || 0 == SearchPath(NULL, shellpath, ".exe", MAX_PATH, temp, NULL)) {
                    message(MB_OK, "Error: File does not exist: '%s'", shellpath);
                    return 1;
                }
                strcpy(shellpath, temp);
            }

            if (0 == is_usingNT) {
                /* win9x/me */
                GetShortPathName(temp, shellpath, MAX_PATH);
                f = rw_ini(A_WR, temp);
            } else {
                if (is_admin) {
                    /* set the boot option that indicates whether to look
                       for the shell in HKCU or HKLM */
                    if (is_per_user)
                        f = rw_reg(A_WR|A_SZ, HKEY_LOCAL_MACHINE, inimapstr, "Shell", usr_bootOption);
                    else
                        f = rw_reg(A_WR|A_SZ, HKEY_LOCAL_MACHINE, inimapstr, "Shell", sys_bootOption);

                    if (f) {
                        /* flush the IniFileMapping cache (NT based systems) */
                        WritePrivateProfileStringW(NULL, NULL, NULL, L"system.ini");
                    } else {
                        message(MB_OK, "Error: Could not set Boot Option.");
                        return 1;
                    }
                }

                if (is_per_user)
                    /* set shell in HKCU */
                    f = rw_reg(A_WR|A_SZ, HKEY_CURRENT_USER, logonstr, "Shell", shellpath);
                else
                    /* remove shell from HKCU */
                    f = 1, rw_reg(A_DEL, HKEY_CURRENT_USER, logonstr, "Shell", NULL);

                if (is_admin) {
                    if (is_per_user)
                        /* set explorer as default for all users */
                        rw_reg(A_WR|A_SZ, HKEY_LOCAL_MACHINE, logonstr, "Shell", szExplorer);
                    else
                        /* set our shell for all users */
                        f = rw_reg(A_WR|A_SZ, HKEY_LOCAL_MACHINE, logonstr, "Shell", shellpath);
                }
            }

            if (0 == f) {
                message(MB_OK, "Error: Could not set '%s' as shell.", shellpath);
                return 1;
            }

            if (0 == is_admin && is_per_user && 0 == get_per_user()) {
                id = 1;
            } else if (id == IDC_LOG1) {
                id = 2;
            } else if (0 == is_admin && 0 == is_per_user) {
                id = 3;
            } else {
                id = 4;
            }

            EndDialog(hDlg, id);
            return 1;
        }
    }
    return 0;
}

/* ======================================================================== */

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd
    )
{
    switch (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DLG1),
                NULL, (DLGPROC)dlgfunc, (LONG_PTR)hInstance)) {
    case 0:
        return 1;
    case 1:
        message(MB_OK,
            "'%s' has been set as shell."
            "\n"
            "\nTo make this work correctly, you need to run "APPNAME" as "
            "\nadministrator and enable individual shells too."
            , shellpath);
        break;
    case 2:
        ExitWindowsEx(EWX_LOGOFF, 0);
        break;
    case 3:
        message(MB_OK,
            "You will have the default system shell. Logout to apply.");
        break;
    case 4:
        message(MB_OK, "'%s' has been set as shell. Logout to apply.", shellpath);
        break;
    }
    return 0;
}

/* ======================================================================== */
#endif
