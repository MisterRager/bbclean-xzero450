/*---------------------------------------------------------------------------------
 SystemBarEx (© 2004 Slade Taylor [bladestaylor@yahoo.com])
 ----------------------------------------------------------------------------------
 based on BBSystemBar 1.2 (© 2002 Chris Sutcliffe (ironhead) [ironhead@rogers.com])
 ----------------------------------------------------------------------------------
 SystemBarEx is a plugin for Blackbox for Windows.  For more information,
 please visit [http://bb4win.org] or [http://sourceforge.net/projects/bb4win].
 ----------------------------------------------------------------------------------
 SystemBarEx is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
#endif

#define VC_EXTRALEAN

//-------------------------------
#include "../../blackbox/BBApi.h"
//-------------------------------

#include <shellapi.h>
//#include <stdlib.h>
#include <time.h>

#ifdef MAX_LINE_LENGTH
    #undef MAX_LINE_LENGTH
#endif
#define MAX_LINE_LENGTH                 320

#define MINI_MAX_LINE_LENGTH            100

#ifndef BB_REDRAWGUI
    #define BB_REDRAWGUI                10881
#endif

#ifndef F_CO3a
    #define F_CO3a                      8
#endif

#ifndef BBRG_TOOLBAR
    #define BBRG_TOOLBAR                1
#endif

#ifndef BBRG_PRESSED
    #define BBRG_PRESSED                32
#endif
