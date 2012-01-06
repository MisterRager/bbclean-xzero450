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

#ifndef _STYLESTRUCT_H_
#define _STYLESTRUCT_H_

/* Note: Do not change this structure. New items may be appended
   at the end, though. */

typedef struct StyleStruct
{
    StyleItem Toolbar;
    StyleItem ToolbarButton;
    StyleItem ToolbarButtonPressed;
    StyleItem ToolbarLabel;
    StyleItem ToolbarWindowLabel;
    StyleItem ToolbarClock;

    StyleItem MenuTitle;
    StyleItem MenuFrame;
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

    COLORREF windowFrameFocusColor;
    COLORREF windowFrameUnfocusColor;

    unsigned char menuAlpha;
    unsigned char toolbarAlpha;
    bool menuNoTitle;
    unsigned char reserved2;

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
    bool is_070;
    bool menuTitleLabel;

    StyleItem Slit;

} StyleStruct;

#define STYLESTRUCTSIZE ((SIZEOFPART(StyleStruct, Slit)+3) & ~3)

#endif //ndef _STYLESTRUCT_H_
