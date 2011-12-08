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

#ifndef __STYLESTRUCT_H
#define __STYLESTRUCT_H

typedef struct
{
    StyleItem Toolbar;
    StyleItem ToolbarButton;
    StyleItem ToolbarButtonPressed;
    StyleItem ToolbarLabel;
    StyleItem ToolbarWindowLabel;
    StyleItem ToolbarClock;

    StyleItem MenuTitle;
    StyleItem MenuFrame;
    StyleItem MenuFrameExt;
    StyleItem MenuHilite;

    StyleItem windowTitleFocus;
    StyleItem windowLabelFocus;
    StyleItem windowHandleFocus;
    StyleItem windowGripFocus;
    StyleItem windowButtonFocus;
    StyleItem windowButtonPressed;

    StyleItem windowTitleUnfocus;
    StyleItem windowLabelUnfocus;
    StyleItem windowHandleUnfocus;
    StyleItem windowGripUnfocus;
    StyleItem windowButtonUnfocus;

    StyleItem MenuSeparator;
    StyleItem MenuVolume;

    StyleItem windowButtonCloseFocus;
    StyleItem windowButtonClosePressed;
    StyleItem windowButtonCloseUnfocus;

    StyleItem MenuVolumeHilite;

    COLORREF windowFrameFocusColor;
    COLORREF windowFrameUnfocusColor;
    COLORREF reserved;

    COLORREF borderColor;
    int borderWidth;
    int bevelWidth;
    int frameWidth;
    int handleHeight;

    char menuBullet[16];
    char menuBulletPosition[16];
    char rootCommand[MAX_PATH+80];

    bool bulletUnix;
    bool metricsUnix;
} StyleStruct;

#endif
