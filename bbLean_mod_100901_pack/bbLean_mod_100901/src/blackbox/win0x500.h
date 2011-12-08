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

// compiler and version dependent additional defines for windows

#ifndef __WIN0X500_H_
#define __WIN0X500_H_

//===========================================================================
#ifdef __BORLANDC__
//===========================================================================

#define LWA_COLORKEY                        0x00000001
#define ULW_COLORKEY                        0x00000001
#define LWA_ALPHA                           0x00000002
#define ULW_ALPHA                           0x00000002
#define ULW_OPAQUE                          0x00000004

#define SPI_GETACTIVEWINDOWTRACKING         0x1000
#define SPI_SETACTIVEWINDOWTRACKING         0x1001
#define SPI_GETMENUANIMATION                0x1002
#define SPI_SETMENUANIMATION                0x1003
#define SPI_GETCOMBOBOXANIMATION            0x1004
#define SPI_SETCOMBOBOXANIMATION            0x1005
#define SPI_GETLISTBOXSMOOTHSCROLLING       0x1006
#define SPI_SETLISTBOXSMOOTHSCROLLING       0x1007
#define SPI_GETGRADIENTCAPTIONS             0x1008
#define SPI_SETGRADIENTCAPTIONS             0x1009
#define SPI_GETKEYBOARDCUES                 0x100A
#define SPI_SETKEYBOARDCUES                 0x100B
#define SPI_GETMENUUNDERLINES               SPI_GETKEYBOARDCUES
#define SPI_SETMENUUNDERLINES               SPI_SETKEYBOARDCUES
#define SPI_GETACTIVEWNDTRKZORDER           0x100C
#define SPI_SETACTIVEWNDTRKZORDER           0x100D
#define SPI_GETHOTTRACKING                  0x100E
#define SPI_SETHOTTRACKING                  0x100F
#define SPI_GETMENUFADE                     0x1012
#define SPI_SETMENUFADE                     0x1013
#define SPI_GETSELECTIONFADE                0x1014
#define SPI_SETSELECTIONFADE                0x1015
#define SPI_GETTOOLTIPANIMATION             0x1016
#define SPI_SETTOOLTIPANIMATION             0x1017
#define SPI_GETTOOLTIPFADE                  0x1018
#define SPI_SETTOOLTIPFADE                  0x1019
#define SPI_GETCURSORSHADOW                 0x101A
#define SPI_SETCURSORSHADOW                 0x101B

#define SPI_GETUIEFFECTS                    0x103E
#define SPI_SETUIEFFECTS                    0x103F

#define SPI_GETFOREGROUNDLOCKTIMEOUT        0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT        0x2001
#define SPI_GETACTIVEWNDTRKTIMEOUT          0x2002
#define SPI_SETACTIVEWNDTRKTIMEOUT          0x2003
#define SPI_GETFOREGROUNDFLASHCOUNT         0x2004
#define SPI_SETFOREGROUNDFLASHCOUNT         0x2005
#define SPI_GETCARETWIDTH                   0x2006
#define SPI_SETCARETWIDTH                   0x2007

#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_CMONITORS            80
#define SM_SAMEDISPLAYFORMAT    81

//===========================================================================
#endif // __BORLANDC__
//===========================================================================



//===========================================================================
#if defined (_MSC_VER) && (_MSC_VER <= 1200)
//===========================================================================

struct _SHChangeNotifyEntry;

#define NIM_SETFOCUS    0x00000003
#define NIM_SETVERSION  0x00000004
#define NOTIFYICON_VERSION 3
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010
#define NIS_HIDDEN      0x00000001
#define NIS_SHAREDICON  0x00000002

#define CMF_EXTENDEDVERBS 0x00000100      // rarely used verbs

//----------------------------------------

#define CSIDL_COMMON_APPDATA            0x0023        // All Users\Application Data
#define CSIDL_WINDOWS                   0x0024        // GetWindowsDirectory()
#define CSIDL_SYSTEM                    0x0025        // GetSystemDirectory()
#define CSIDL_PROGRAM_FILES             0x0026        // C:\Program Files
#define CSIDL_MYPICTURES                0x0027        // C:\Program Files\My Pictures
#define CSIDL_PROFILE                   0x0028        // USERPROFILE
#define CSIDL_SYSTEMX86                 0x0029        // x86 system directory on RISC
#define CSIDL_PROGRAM_FILESX86          0x002a        // x86 C:\Program Files on RISC
#define CSIDL_PROGRAM_FILES_COMMON      0x002b        // C:\Program Files\Common
#define CSIDL_PROGRAM_FILES_COMMONX86   0x002c        // x86 Program Files\Common on RISC
#define CSIDL_COMMON_TEMPLATES          0x002d        // All Users\Templates
#define CSIDL_COMMON_DOCUMENTS          0x002e        // All Users\Documents
#define CSIDL_COMMON_ADMINTOOLS         0x002f        // All Users\Start Menu\Programs\Administrative Tools
#define CSIDL_ADMINTOOLS                0x0030        // <user name>\Start Menu\Programs\Administrative Tools
#define CSIDL_CONNECTIONS               0x0031        // Network and Dial-up Connections

//===========================================================================
#endif // _MSC_VER
//===========================================================================



//===========================================================================
#ifdef __GNUC__
//===========================================================================

// redundant with gcc 3.4.2
/*
#define SFGAO_HIDDEN    0x00080000L // hidden object
#define OFN_DONTADDTORECENT 0x02000000
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010
#define NIS_HIDDEN      0x00000001
#define NIS_SHAREDICON  0x00000002
#define NIM_SETVERSION  0x00000004

#define MONITOR_DEFAULTTONEAREST 2
#define CMF_CANRENAME 16
#define ENDSESSION_LOGOFF 0x80000000
#define CMF_EXTENDEDVERBS   0x00000100  // rarely used verbs

#define SPI_SETFOREGROUNDFLASHCOUNT 0x2005
#define SPI_SETACTIVEWNDTRKTIMEOUT 8195
//#define SPI_SETACTIVEWINDOWTRACKING 4097
//#define SPI_SETACTIVEWNDTRKZORDER 4109
*/

//===========================================================================
#endif // __GNUC__
//===========================================================================

#ifndef GWLP_USERDATA
#define GWLP_USERDATA GWL_USERDATA
#define GWLP_WNDPROC GWL_WNDPROC
#define GWLP_HINSTANCE GWL_HINSTANCE
#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong
#define DWORD_PTR DWORD
#define UINT_PTR UINT
#define LONG_PTR LONG
#endif
#ifndef GCLP_HICON
#define GCLP_HICON GCL_HICON
#define GCLP_HICONSM GCL_HICONSM
#endif

#ifndef LWA_ALPHA
#define LWA_ALPHA       0x00000002
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED   0x00080000
#endif

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP 0x020C
#endif

#ifndef XBUTTON1
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#endif

#ifndef XBUTTON3
#define XBUTTON3 0x0004
#endif

//#define MK_ALT  32

#ifndef CDSIZEOF_STRUCT
#define CDSIZEOF_STRUCT(structname,member) ((int)&((structname*)0)->member + sizeof(((structname*)0)->member))
#endif

#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400 CDSIZEOF_STRUCT(OPENFILENAMEA,lpTemplateName)
#endif

#endif // __BBGCC_W500_H_
