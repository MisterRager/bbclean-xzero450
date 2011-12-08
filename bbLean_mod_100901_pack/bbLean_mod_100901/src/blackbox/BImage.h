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

#ifndef __BIMAGE_H
#define __BIMAGE_H

#ifndef B_HORIZONTAL

// bevelstyle
#define BEVEL_FLAT   0
#define BEVEL_RAISED 1
#define BEVEL_SUNKEN 2

// bevelposition
#define BEVEL1 1
#define BEVEL2 2

// Gradient types
#define B_HORIZONTAL        0
#define B_VERTICAL          1
#define B_DIAGONAL          2
#define B_CROSSDIAGONAL     3
#define B_PIPECROSS         4
#define B_ELLIPTIC          5
#define B_RECTANGLE         6
#define B_PYRAMID           7
#define B_SOLID             8
#define B_SPLITVERTICAL     9
#define B_MIRRORVERTICAL   10
#define B_MIRRORHORIZONTAL 11
#define B_SPLITHORIZONTAL  12

#endif

extern "C"
{
	void MakeGradient(
		HDC hDC,
		RECT rect,
		int type,
		COLORREF Color,
		COLORREF ColorTo,
		bool bInterlaced,
		int bevelStyle,
		int bevelPosition,
		int bevelWidth,
		COLORREF borderColour,
		int borderWidth
		);

	void CreateBorder(
		HDC hDC,
		RECT *prect,
		COLORREF borderColour,
		int borderWidth
		);

	HBITMAP MakeGradientBitmap(
		int width, int height,
		int type,
		COLORREF colour1,
		COLORREF colour2,
		bool bInterlaced,
		int bevelStyle,
		int bevelPosition
		);
};

//===========================================================================
#endif
