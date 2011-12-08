/* Tooltip */

#include "BBApi.h"
#include "Definitions.h"
#include "PluginMaster.h"
#include "Tooltip.h"


#define NUMBER_OF(array) (sizeof((array)) / sizeof((array)[0]))




static HWND tooltip_window = NULL;
bool tooltip_enabled = true;




static void
notice(const char* message)
{
	if (!plugin_suppresserrors)
		BBMessageBox(NULL, message, szAppName, MB_OK | MB_SYSTEMMODAL);
}








int
tooltip_startup(void)
{
	tooltip_window = CreateWindowEx( WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
	                            TOOLTIPS_CLASS,
	                            NULL,
	                            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
	                            0, 0, 0, 0,
	                            NULL,
	                            NULL,
	                            plugin_instance_plugin,
	                            NULL );

	if (tooltip_window == NULL) {
		notice("Failed to create tooltip");
		return !0;
	}

	return 0;
}


int
tooltip_shutdown(void)
{
	if (tooltip_window != NULL)
		DestroyWindow(tooltip_window);

	return 0;
}




int
tooltip_add(HWND hwnd)
{
	TOOLINFO ti;

	if (tooltip_window == NULL)
		return !0;

	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = hwnd;
	ti.uId = (UINT_PTR)hwnd;
	/* ti.rect */
	ti.hinst = NULL;
	ti.lpszText = LPSTR_TEXTCALLBACK;

	if (!SendMessage(tooltip_window, TTM_ADDTOOL, 0, (LPARAM)&ti)) {
		notice("Failed to add a control into tooltip");
		return !0;
	}

	return 0;
}


int
tooltip_del(HWND hwnd)
{
	TOOLINFO ti;

	if (tooltip_window == NULL)
		return !0;

	ti.cbSize = sizeof(ti);
	ti.hwnd = hwnd;
	ti.uId = (UINT_PTR)hwnd;
	SendMessage(tooltip_window, TTM_DELTOOL, 0, (LPARAM)&ti);

	return 0;
}




void
tooltip_update(NMTTDISPINFO* di, control* c)
{
	if ((c == NULL) || (!tooltip_enabled))
		return;

	if (di->hdr.hwndFrom == tooltip_window) {
		if (di->hdr.code == TTN_GETDISPINFO) {
			char buf[225];
			window* w = c->windowptr;
			sprintf(buf,"%s:%s [%d,%d] @ (%d,%d)",c->moduleptr->name,c->controlname, w->width, w->height, w->x, w->y);
			strncpy( di->szText, buf,
			         NUMBER_OF(di->szText) );
			di->szText[NUMBER_OF(di->szText) - 1] = '\0';
		}
	}
}




/* __END__ */
