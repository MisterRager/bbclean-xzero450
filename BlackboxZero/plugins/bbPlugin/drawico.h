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

void DrawIconSatnHue (
    HDC hDC,
    int px,
    int py,
    HICON IconHop,
    int size_x,
    int size_y,
    UINT istepIfAniCur, // index of frame in animated cursor
    HBRUSH hbrFlickerFreeDraw,  // handle to background brush
    UINT diFlags, // icon-drawing flags
    int apply_satnhue,
    int saturationValue,
    int hueIntensity
    );

