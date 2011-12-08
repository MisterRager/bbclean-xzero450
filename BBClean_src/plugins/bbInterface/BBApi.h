/*===================================================

	BLACKBOX API AND MISC DEFINITIONS

===================================================*/

#ifndef __BBAPI2_H_
#define __BBAPI2_H_

#define WINVER 0x0500

#define WS_EX_LAYERED   0x00080000
#define LWA_ALPHA       0x00000002
#define LWA_COLORKEY    1 

#if (_MSC_VER >= 1400) 

// overload strcpy and the like to new "secure" functions 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 1 
#pragma warning(disable:4996)
#endif


#include "blackbox\BBApi.h"

// Utility
void dbg_printf(const char *fmt, ...);

#define BB_DESKCLICK 10884
#define BB_DRAGTODESKTOP 10510
#define VALID_TEXTCOLOR (1<<3)

// wParam values for BB_WORKSPACE
#define BBWS_DESKLEFT           0
#define BBWS_DESKRIGHT          1
#define BBWS_GATHERWINDOWS      5
#define BBWS_MOVEWINDOWLEFT     6
#define BBWS_MOVEWINDOWRIGHT    7
#define BBWS_PREVWINDOW         8
#define BBWS_NEXTWINDOW         9

#define BBVERSION_LEAN 2
#define BBVERSION_XOB 1
#define BBVERSION_09X 0

#define BBI_MAX_LINE_LENGTH     4000
#define BBI_POSTCOMMAND         (WM_USER+10)

//-------------------------------------------------
#endif /* __BBAPI2_H_ */
