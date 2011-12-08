#ifndef BBInterface_Tooltip_h
#define BBInterface_Tooltip_h

#include <commctrl.h>


extern bool tooltip_enabled;

int tooltip_startup(void);
int tooltip_shutdown(void);

int tooltip_add(HWND);
int tooltip_del(HWND);

void tooltip_update(NMTTDISPINFO*, control*);

#endif
