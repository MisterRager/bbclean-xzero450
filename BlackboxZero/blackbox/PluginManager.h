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

#ifndef _BBPLUGINMANAGER_H_
#define _BBPLUGINMANAGER_H_

void PluginManager_Init(void);
void PluginManager_Exit(void);
void PluginManager_aboutPlugins(void);
int PluginManager_handleBroam(const char *submessage);
Menu* PluginManager_GetMenu(const char *text, char *menu_id, bool pop, int mode);
int PluginManager_RCChanged(void);

#define SUB_PLUGIN_LOAD 1
#define SUB_PLUGIN_SLIT 2

#endif // __BBPLUGINMANAGER_H
