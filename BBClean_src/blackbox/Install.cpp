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

// perform the 'install / uninstall' options
// set / unset blackbox as the default shell

#include "BB.H"

extern bool usingx64;

enum { A_DEL, A_DW, A_SZ };

static int write_key(int action, HKEY root, const char *ckey, const char *cval, const char *cdata)
{
    HKEY k;
    DWORD result;
	int r;
	for(int i=0;i<(usingx64?2:1);++i)
	{
		r = RegCreateKeyEx(root, ckey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE|(i?KEY_WOW64_64KEY:KEY_WOW64_32KEY), NULL, &k, &result);
		if (ERROR_SUCCESS == r)
		{
			if (A_DEL==action)
				r = RegDeleteValue(k, cval);
			else
			if (A_DW==action)
				r = RegSetValueEx(k, cval, 0, REG_DWORD, (LPBYTE)&cdata, sizeof(DWORD));
			else
			if (A_SZ==action)
				r = RegSetValueEx(k, cval, 0, REG_SZ, (LPBYTE)cdata, strlen(cdata)+1);

			RegCloseKey(k);
		}
	}
    return r == ERROR_SUCCESS;
}                

static bool write_ini (char *boot)
{
    char szWinDir[MAX_PATH];
    GetWindowsDirectory(szWinDir, sizeof(szWinDir));
    strcat(szWinDir, "\\SYSTEM.INI");
    return WritePrivateProfileString("Boot", "Shell", boot, szWinDir);
}

static char inimapstr       [] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\system.ini\\boot";
static char sys_bootOption  [] = "SYS:Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
static char usr_bootOption  [] = "USR:Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
static char logonstr        [] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
static char explorer_option [] = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer";
static char szExplorer      [] = "explorer.exe";

static bool install_message(const char *msg, bool succ)
{
    if (succ)
        BBMessageBox(MB_OK, NLS2("$BBInstall_Success$",
            "%s has been set as shell.\nReboot or logoff to apply."), msg);
    else
        BBMessageBox(MB_OK, NLS2("$BBInstall_Failed$",
            "Error: Could not set %s as shell."), msg);
    return succ;
}

//====================
static bool install_nt(
    char *bootOption,
    char *defaultShell,
    char *userShell,
    bool setDesktopProcess
    )
{
    // First we open the boot options registry key...
    // ...and set the "Shell" registry value that defines if the shell setting should
    // be read from HKLM (for all users) or from HKCU (for the current user), see above...

    if (0==write_key(A_SZ,
        HKEY_LOCAL_MACHINE, inimapstr, "Shell", bootOption
        ))
        goto fail;

    // Next we set the HKLM shell registry key (this needs to be done at all times!)
    if (0==write_key(A_SZ,
        HKEY_LOCAL_MACHINE, logonstr, "Shell", defaultShell
        ))
        goto fail;

    // If installing for the current user only, we also need to set the
    // HKCU shell registry key, else delete_it ...

    if (0==write_key(userShell ? A_SZ : A_DEL,
        HKEY_CURRENT_USER, logonstr, "Shell", userShell
        ))
        if (userShell)
            goto fail;

    // ...as well as the DesktopProcess registry key that allows Explorer
    // to be used as file manager while using an alternative shell...

    if (0==write_key(A_DW,
        HKEY_CURRENT_USER, explorer_option, "DesktopProcess",
        setDesktopProcess ? (char*)1 : (char*)0
        ))
        goto fail;

    if (0==write_key(A_SZ,
        HKEY_CURRENT_USER, explorer_option, "BrowseNewProcess",
        setDesktopProcess ? "yes" : "no"
        ))
        goto fail;

    return true;
fail:
    return false;
}

//====================
bool installBlackbox(bool forceInstall)
{
	if (!forceInstall)
	{
		if (IDYES!=BBMessageBox(MB_YESNO, NLS2("$BBInstall_Query$",
			"Do you want to install Blackbox as your default shell?")))
			return false;
	}

	char szBlackbox[MAX_PATH];
	GetModuleFileName(NULL, szBlackbox, MAX_PATH);
	bool result = false;

	if (GetShortPathName(szBlackbox, szBlackbox, MAX_PATH))
	{
		//strcat(szBlackbox, " -startup");
		if (usingNT)
		{
			int answer;
			if (forceInstall)
			{
				answer = IDNO;
			} else {
				answer = BBMessageBox(MB_YESNOCANCEL, NLS2("$BBInstall_QueryAllOrCurrent$",
					"Do you want to install Blackbox as the default shell for All Users?"
					"\n(Selecting \"No\" will install for the Current User only)."
					));
			}

            if(IDCANCEL==answer)
                return false;

            if(IDYES==answer)
            {
                // If installing for all users the HKLM shell setting should point to Blackbox...
                result = install_nt(sys_bootOption, szBlackbox, NULL, false);
            }

            if(IDNO==answer)
            {
                // If installing only for the current user the HKLM shell setting should point to Explorer...
                // If installing for the current user only, we also need to set the HKCU shell registry key...

                // ...as well as the DesktopProcess registry key that allows Explorer
                // to be used as file manager while using an alternative shell...

                result = install_nt(usr_bootOption, szExplorer, szBlackbox, true);
            }
        }
        else // Windows9x/ME
        {
             result = write_ini(szBlackbox);
        }
    }

    return install_message("Blackbox", result);
}

//====================
bool uninstallBlackbox()
{
    if (IDYES != BBMessageBox(MB_YESNO, NLS2("$BBInstall_QueryUninstall$",
        "Do you want to reset explorer as your default shell?")))
        return false;

    bool result;

    if (usingNT)
    {
        result = install_nt(sys_bootOption, szExplorer, NULL, false);
    }
    else // Windows9x/ME
    {
        result = write_ini(szExplorer);
    }

    return install_message("Explorer", result);
}

//====================
