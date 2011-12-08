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

#ifndef __BBPLUGINMANAGER_H
#define __BBPLUGINMANAGER_H


void PluginManager_Init();
void PluginManager_Exit();
void PluginManager_aboutPlugins();
void PluginManager_handleBroam(const char *submessage);

extern FILETIME PluginManager_FT;

struct plugins
{
    struct plugins *next;
    HMODULE hmodule;
    const char* (*pluginInfo)(int);
    int (*beginPlugin)(HINSTANCE);
    int (*endPlugin)(HINSTANCE);
    int (*beginSlitPlugin)(HINSTANCE, HWND);
    int (*beginPluginEx)(HINSTANCE, HWND);
    bool is_comment;
    bool is_slit;
    bool enabled;
    bool useslit;
    bool inslit;
    char name[80];
    char fullpath[1];
};

extern struct plugins *bbplugins;
extern HWND hSlit;


#endif // __BBPLUGINMANAGER_H
