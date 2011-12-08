/*===================================================

	WINDOW MASTER CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <shellapi.h>
#include <stdlib.h>

//Parent Include
#include "WindowMaster.h"

//Includes
#include "Definitions.h"
#include "PluginMaster.h"
#include "StyleMaster.h"
#include "MenuMaster.h"
#include "ConfigMaster.h"
#include "SnapWindow.h"
#include "Tooltip.h"

//Global variables
char szWPx                      [] = "X";
char szWPy                      [] = "Y";
char szWPwidth                  [] = "Width";
char szWPheight                 [] = "Height";
char szWPtransparency           [] = "Transparency";
char szWPisvisible              [] = "IsVisible";
char szWPisontop                [] = "IsOnTop";
char szWPissnappy               [] = "IsSnappy";
char szWPisslitted              [] = "IsSlitted";
char szWPistransparent          [] = "IsTransparent";
char szWPistoggledwithplugins   [] = "IsToggledWithPlugins";
char szWPisonallworkspaces      [] = "IsOnAllWorkspaces";
char szWPisbordered             [] = "Border";
char szWPStyle                  [] = "Style";
char szWPautohide               [] = "AutoHide";
char szWPmakeinvisible          [] = "MakeInvisible";
char szWPmakeinvisible_never    [] = "Never";
char szWPmakeinvisible_bbblur   [] = "BBLoseFocus";
char szWPmakeinvisible_winblur  [] = "WindowLoseFocus";

//Local variables
bool window_is_pluginsvisible = true;

//Internal functions
void window_set_transparency(window *w);
bool window_test_autohide(window *w, POINT *pt);
COLORREF switch_rgb (COLORREF c);
COLORREF ReadColorFromString(char * string);

const int TIMER_AUTOHIDE = 1;
const int TIMER_MAKEINVISIBLE = 2;

const int AUTOHIDE_DELAY = 300;

const int MAKEINVISIBLE_NEVER = 0;
const int MAKEINVISIBLE_BBLOSEFOCUS = 1;
const int MAKEINVISIBLE_WINLOSEFOCUS = 2;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_startup()
{
	//Define the window class
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc      = window_event;                 // our window procedure
	wc.hInstance        = plugin_instance_plugin;       // hInstance of .dll
	wc.lpszClassName    = (LPCWSTR)szAppName;                    // our window class name
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wc.style            = CS_DBLCLKS;
	wc.cbWndExtra       = sizeof (control*); 

	if (!RegisterClass(&wc)) 
	{
		if (!plugin_suppresserrors) MessageBox(plugin_hwnd_blackbox, (LPCWSTR)"Error registering window class", (LPCWSTR)szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}
		
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_shutdown()
{
	//Unregister the window class
	UnregisterClass((LPCWSTR)szAppName, plugin_instance_plugin);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_create(control *c)
{
	window *windowpointer;
	windowpointer = new window;

	//Set up window properties
	windowpointer->controlptr = c;
	c->windowptr = windowpointer; 

	//Default settings
	windowpointer->x = 10;
	windowpointer->y = 10;
	windowpointer->width = 100;
	windowpointer->height = 100;
	windowpointer->is_visible = true;
	windowpointer->is_ontop = true;
	windowpointer->is_toggledwithplugins = true;
	windowpointer->is_onallworkspaces = true;
	windowpointer->is_bordered = true;
	windowpointer->style = STYLETYPE_TOOLBAR;
	windowpointer->styleptr = 0;
	windowpointer->has_custom_style = false;

	windowpointer->makeinvisible = MAKEINVISIBLE_NEVER;
	windowpointer->autohide = false;
	windowpointer->useslit = false;
	windowpointer->is_slitted = false;
	windowpointer->is_transparent = false;
	windowpointer->transparency = 100;

	// other initialisation
	windowpointer->bitmap = NULL;
	windowpointer->is_moving = false;
	windowpointer->is_autohidden = false;

	//Load settings
	void *datafetch;
	//Get the default height
	datafetch = (c->controltypeptr->func_getdata)(c, DATAFETCH_INT_DEFAULTHEIGHT);
	if (datafetch) windowpointer->height = *((int *) datafetch);

	datafetch = (c->controltypeptr->func_getdata)(c, DATAFETCH_INT_DEFAULTWIDTH);
	if (datafetch) windowpointer->width = *((int *) datafetch);

	// Setup creation params
	UINT exStyle;
	UINT Style;
	HWND hwnd_parent;

	if (c->parentptr)
	{
		exStyle = WS_EX_TOOLWINDOW;
		Style = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE;
		hwnd_parent = c->parentptr->windowptr->hwnd;
	}
	else
	{
		exStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
		Style = WS_POPUP|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE;
		hwnd_parent = NULL;
	}

	//Create the window
	HWND hwnd = CreateWindowEx(
		exStyle,                        // window ex-style
		(LPCWSTR)szAppName,                      // our window class name
		NULL,                           // NULL -> does not show up in task manager!
		Style,                          // window parameters
		windowpointer->x,               // x position
		windowpointer->y,               // y position
		windowpointer->width,           // window width
		windowpointer->height,          // window height
		hwnd_parent,                    // parent window
		NULL,                           // no menu
		plugin_instance_plugin,         // hInstance of .dll
		(void*)c                        // control pointer
		);

	// Check to make sure it's okay
	if (NULL == hwnd)
	{                          
		if (!plugin_suppresserrors)
			BBMessageBox(0, "Error creating window", szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);

		delete windowpointer;
		c->windowptr = NULL;
		return 1;
	}

	window_update(windowpointer, false, true, false, true);
	if (c->parentptr) SetWindowPos(hwnd, HWND_TOP, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOSENDCHANGING);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_destroy(window **pw)
{
	window *w = *pw;
	if (NULL == w) return 0;

	//Free memory for custom style
	if (w->has_custom_style) delete w->styleptr;

	//If the window is in the slit, remove it!
	if (w->is_slitted)
		SendMessage(plugin_hwnd_slit, SLIT_REMOVE, 0, (LPARAM) w->hwnd);

	//Remove sticky property
	RemoveSticky(w->hwnd);

	// Destroy the hwnd...
	DestroyWindow(w->hwnd);

	// Clear cached bitmap
	if (w->bitmap)
		DeleteObject(w->bitmap);

	//Destroy the window
	delete w;

	// Zero out pointer
	*pw = NULL;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void window_menu_context(Menu *m, control *c)
{
	window *w = c->windowptr;
	Menu *submenu, *submenu2; int i; bool temp;

	// NOTE: Comment out in proper version for now.
	/*
	make_menuitem_int(m, "X", config_getfull_control_setwindowprop_s(c, szWPx), w->x, -10, 3000);
	make_menuitem_int(m, "Y", config_getfull_control_setwindowprop_s(c, szWPy), w->y, -10, 3000);
	make_menuitem_int(m, "Width", config_getfull_control_setwindowprop_s(c, szWPwidth), w->width, 1, 1000);
	make_menuitem_int(m, "Height", config_getfull_control_setwindowprop_s(c, szWPheight), w->height, 1, 1000);
	make_menuitem_nop(m, NULL);
	*/
	


	//Style options
	submenu = make_menu("Style", c);
	for (i = 0; i < STYLE_COUNT; ++i)
		make_menuitem_bol(submenu, szStyleNames[i], config_getfull_control_setwindowprop_c(c, "Style", szStyleNames[i]), (!w->has_custom_style) && (w->style == i) ); //None of these are possible if a custom style is used. Subject to change with the rest of the implementation.
	submenu2 = make_menu("Custom", c );
	for (i = 0; i < STYLE_COLOR_PROPERTY_COUNT; ++i)
	{
		int colorval;
		switch(i)
		{
			case 0: colorval = w->has_custom_style ? w->styleptr->Color : style_get_copy(w->style).Color; break;
			case 1: colorval = w->has_custom_style ? w->styleptr->ColorTo : style_get_copy(w->style).ColorTo; break;
			case 2: colorval = w->has_custom_style ? w->styleptr->TextColor : style_get_copy(w->style).TextColor; break;
		}
		char color[8];
		sprintf(color,"#%06X",switch_rgb(colorval)); //the bits need to be switched around here
			make_menuitem_str(
				submenu2,
				szStyleProperties[i],
				config_getfull_control_setwindowprop_s(c, szStyleProperties[i]),
				color
				);

	}

	// Add the bevel setting menus as well.
	Menu* bevelmenu = make_menu("Bevel Style", c);
	int beveltype = w->has_custom_style ? w->styleptr->bevelstyle : style_get_copy(w->style).bevelstyle;
	for (int i=0; i<STYLE_BEVEL_TYPE_COUNT; ++i)
	{
		bool temp = (beveltype == i);
		make_menuitem_bol(bevelmenu,szBevelTypes[i],config_getfull_control_setwindowprop_c(c,szStyleProperties[STYLE_BEVELTYPE_INDEX],szBevelTypes[i]),temp);
	}
	make_submenu_item(submenu2,"Bevel Style",bevelmenu);
	if (beveltype != 0)
		make_menuitem_int(
			submenu2,
			"Bevel Position",
			config_getfull_control_setwindowprop_s(c, szStyleProperties[STYLE_BEVELPOS_INDEX]),
			w->has_custom_style ? w->styleptr->bevelposition : style_get_copy(w->style).bevelposition,
			1,
			2);

	make_submenu_item(submenu, "Custom", submenu2);
	make_submenu_item(m, "Style", submenu);

	temp = !w->is_bordered;
	make_menuitem_bol(m, "Border", config_getfull_control_setwindowprop_b(c, szWPisbordered, &temp), !temp);

	temp = !w->is_visible; make_menuitem_bol(m, "Visible", config_getfull_control_setwindowprop_b(c, szWPisvisible, &temp), !temp);

	if (!c->parentptr && plugin_hwnd_slit)
	{
		temp = !w->useslit;
		make_menuitem_bol(m, "In Slit", config_getfull_control_setwindowprop_b(c, szWPisslitted, &temp), !temp);
	}

	if (!c->parentptr && (!w->useslit || NULL == plugin_hwnd_slit))
	{
		//Do the "Make Invisible" property
		make_menuitem_nop(m, NULL);
		submenu = make_menu("Make Invisible", c);
		temp = (w->makeinvisible == MAKEINVISIBLE_NEVER); make_menuitem_bol(submenu, "Never", config_getfull_control_setwindowprop_c(c, szWPmakeinvisible, szWPmakeinvisible_never), temp);
		temp = (w->makeinvisible == MAKEINVISIBLE_WINLOSEFOCUS); make_menuitem_bol(submenu, "When control loses focus", config_getfull_control_setwindowprop_c(c, szWPmakeinvisible, szWPmakeinvisible_winblur), temp);
		temp = (w->makeinvisible == MAKEINVISIBLE_BBLOSEFOCUS); make_menuitem_bol(submenu, "When BlackBox loses focus", config_getfull_control_setwindowprop_c(c, szWPmakeinvisible, szWPmakeinvisible_bbblur), temp);
		make_submenu_item(m, "Make Invisible", submenu);


		make_menuitem_nop(m, NULL);
		temp = !w->autohide; make_menuitem_bol(m, "AutoHide", config_getfull_control_setwindowprop_b(c, szWPautohide, &temp), !temp);
		temp = !w->is_ontop; make_menuitem_bol(m, "Always On Top", config_getfull_control_setwindowprop_b(c, szWPisontop, &temp), !temp);
		temp = !w->is_toggledwithplugins; make_menuitem_bol(m, "Toggle With Plugins", config_getfull_control_setwindowprop_b(c, szWPistoggledwithplugins, &temp), !temp);
		temp = !w->is_onallworkspaces; make_menuitem_bol(m, "On All Workspaces", config_getfull_control_setwindowprop_b(c, szWPisonallworkspaces, &temp), !temp);

		if (plugin_using_modern_os)
		{
			make_menuitem_nop(m, NULL);
			temp = !w->is_transparent; make_menuitem_bol(m, "Transparent", config_getfull_control_setwindowprop_b(c, szWPistransparent, &temp), !temp);
			make_menuitem_int(m, "Transparency", config_getfull_control_setwindowprop_s(c, szWPtransparency), w->transparency, 0, 100);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void window_save_control(control *c)
{
	window *w = c->windowptr;
	config_write(config_get_control_setwindowprop_i(c, szWPx, &w->x));
	config_write(config_get_control_setwindowprop_i(c, szWPy, &w->y));
	config_write(config_get_control_setwindowprop_i(c, szWPwidth, &w->width));
	config_write(config_get_control_setwindowprop_i(c, szWPheight, &w->height));
	config_write(config_get_control_setwindowprop_b(c, szWPisbordered, &w->is_bordered));
	config_write(config_get_control_setwindowprop_b(c, szWPisvisible, &w->is_visible));
	config_write(config_get_control_setwindowprop_c(c, szWPStyle, szStyleNames[w->style]));
	if (w->has_custom_style) //The redundance in style specification allows for a possible "Disable custom styles" option, which could come in handy for some.
	{	for (int i = 0; i < STYLE_COLOR_PROPERTY_COUNT; ++i) //This part would be a likely candidate to be exported to another function.
		{
			int colorval;
			switch(i)
			{	case 0: colorval = w->styleptr->Color; break;
				case 1: colorval = w->styleptr->ColorTo; break;
				case 2: colorval = w->styleptr->TextColor; break;
			}
			char color[8];
			sprintf(color,"#%06X",switch_rgb(colorval)); //the bits need to be switched around here
			config_write(config_get_control_setwindowprop_c(c, szStyleProperties[i], color));
		}
		config_write(config_get_control_setwindowprop_c(c,szStyleProperties[STYLE_BEVELTYPE_INDEX],szBevelTypes[w->styleptr->bevelstyle]));
		if (w->styleptr->bevelstyle != 0) config_write(config_get_control_setwindowprop_i(c,szStyleProperties[STYLE_BEVELPOS_INDEX],&w->styleptr->bevelposition));
	}
	if (!c->parentptr)
	{
		config_write(config_get_control_setwindowprop_b(c, szWPistoggledwithplugins, &w->is_toggledwithplugins));
		config_write(config_get_control_setwindowprop_b(c, szWPisonallworkspaces, &w->is_onallworkspaces));
		config_write(config_get_control_setwindowprop_b(c, szWPisontop, &w->is_ontop));
		config_write(config_get_control_setwindowprop_b(c, szWPautohide, &w->autohide));
		config_write(config_get_control_setwindowprop_b(c, szWPisslitted, &w->useslit));
		config_write(config_get_control_setwindowprop_b(c, szWPistransparent, &w->is_transparent));
		config_write(config_get_control_setwindowprop_i(c, szWPtransparency, &w->transparency));

		char *makeinvisiblestring;
		switch (w->makeinvisible)
		{
			case MAKEINVISIBLE_BBLOSEFOCUS: makeinvisiblestring = szWPmakeinvisible_bbblur; break;
			case MAKEINVISIBLE_WINLOSEFOCUS: makeinvisiblestring = szWPmakeinvisible_winblur; break;
			default: makeinvisiblestring = szWPmakeinvisible_never; break;
		}
		config_write(config_get_control_setwindowprop_c(c, szWPmakeinvisible, makeinvisiblestring));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void window_save()
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LRESULT CALLBACK window_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	control *controlpointer = (control *)GetWindowLong(hwnd, 0);
	bool is_locked_frame(control *c);

	if (NULL == controlpointer)
	{
		if (WM_NCCREATE == msg)
		{
			// ---------------------------------------------------
			// bind the window to the control structure
			controlpointer = (control*)((CREATESTRUCT*)lParam)->lpCreateParams;
			SetWindowLong(hwnd, 0, (LONG) controlpointer);
			controlpointer->windowptr->hwnd = hwnd;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	// get the structure from the window extra data
	window *w = controlpointer->windowptr;

	//Interpret the message
	switch(msg)
	{
		// ---------------------------------------------------
		// Set window on top on left desk click
		case BB_DESKCLICK:
			if (lParam == 0)
			if (plugin_click_raise
			 && NULL == controlpointer->parentptr
			 && false == w->is_slitted
			 && false == w->is_ontop)
				SetWindowPos(hwnd, HWND_TOP, 0,0,0,0,
					SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOSENDCHANGING);
			break;

		// ---------------------------------------------------
		// Set a flag when the user is moving/sizing the window
		case WM_ENTERSIZEMOVE:
			w->is_moving = true;
			w->is_sizing = false != (GetKeyState(VK_MENU) & 0xF0);
			break;

		case WM_EXITSIZEMOVE:
			w->is_moving = w->is_sizing = false;
			if (w->is_slitted)
				PostMessage(plugin_hwnd_slit, SLIT_UPDATE, 0, (LPARAM) w->hwnd);

			w->is_autohidden = window_test_autohide(w, NULL);
			break;

		// ---------------------------------------------------
		//If dragging the window, snap if necessary
		case WM_WINDOWPOSCHANGING:
			if (w->is_moving && plugin_snapwindows && !(GetKeyState(VK_SHIFT) & 0xF0))
			{
				int *sizes = w->is_sizing
					? (int*)controlpointer->controltypeptr->func_getdata(controlpointer, DATAFETCH_CONTENTSIZES)
					: NULL;
				snap_windows((WINDOWPOS*)lParam, w->is_sizing, sizes);
			}
			break;

		// ---------------------------------------------------
		// If the window moved, record new coordinates
		case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS* windowpos = (WINDOWPOS*)lParam;
			if (0 == (windowpos->flags & SWP_NOMOVE))
			{
				if (false == w->is_slitted && false == w->is_autohidden)
				{
					w->x = windowpos->x;
					w->y = windowpos->y;
				}
				style_check_transparency_workaround(hwnd);
			}
			//...and sizes
			if (0 == (windowpos->flags & SWP_NOSIZE))
			{
				if (w->width != windowpos->cx || w->height != windowpos->cy)
				{
					w->width  = windowpos->cx;
					w->height = windowpos->cy;
					style_draw_invalidate(controlpointer);
					// notify control
					(controlpointer->controltypeptr->func_notify)(controlpointer, NOTIFY_RESIZE, NULL);
				}
			}
			break;
		}

		// ---------------------------------------------------
		// limit tracking sizes

		case WM_GETMINMAXINFO:
			LPMINMAXINFO mmi;
			mmi=(LPMINMAXINFO)lParam;
			mmi->ptMinTrackSize.x = *((int *)(controlpointer->controltypeptr->func_getdata)(controlpointer, DATAFETCH_INT_MIN_WIDTH));
			mmi->ptMinTrackSize.y = *((int *)(controlpointer->controltypeptr->func_getdata)(controlpointer, DATAFETCH_INT_MIN_HEIGHT));
			/*
			An odd bug occurs here.
			Setting ptMaxTrackSize.x works find.
			Setting ptMaxTrackSize.y immediately causes a crash.

			This occurs even with constants.
			For sanity's sake, I will not limit the maximum size.
			There's no real practical reason to do so, I'll just
			leave it to the user.
			
			mmi->ptMaxTrackSize.x=600;
			mmi->ptMaxTrackSize.y=600;
			*/
			break;

		// ---------------------------------------------------
		// mouse ...
		case WM_NCHITTEST:
			if ((GetKeyState(VK_SHIFT) & 0xF0)
				&& controlpointer->parentptr
				&& CONTROL_ID_FRAME != controlpointer->controltypeptr->id)
			{
				return HTTRANSPARENT;
			}
			else
			if (GetKeyState(VK_MENU) & 0xF0)
			{
				if (false == is_locked_frame(controlpointer)
					&& false == is_locked_frame(controlpointer->parentptr))
						return HTBOTTOMRIGHT;
			}
			else
			if (GetKeyState(VK_CONTROL) & 0xF0)
			{
				if (false == is_locked_frame(controlpointer->parentptr))
					return HTCAPTION;
				else
				if (controlpointer->parentptr)
					return HTTRANSPARENT;
			}
			else
			{
				// can drag a frame without holding control.
				if (NULL == controlpointer->parentptr
					&& CONTROL_ID_FRAME == controlpointer->controltypeptr->id)
					return HTCAPTION;
			}
			return HTCLIENT;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
			SetFocus(hwnd);
			SetCapture(hwnd);
			goto pass_to_control;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
			ReleaseCapture();
			goto pass_to_control;

		case WM_RBUTTONUP:
			ReleaseCapture();

		case WM_NCRBUTTONUP:
			if (false == (GetKeyState(VK_CONTROL) & 0xF0))
				goto pass_to_control;

			menu_control(controlpointer, true);
			break;
			
		//--------------------------------------
		//Automatically make invisible on blur
		case WM_ACTIVATEAPP:
			if (wParam == FALSE && w->makeinvisible == MAKEINVISIBLE_BBLOSEFOCUS)
			{
				w->is_visible = false;
				window_update(w, false, false, true, false);
			}
			goto pass_to_control;
			break;

		case WM_ACTIVATE:
			if (wParam == WA_INACTIVE && w->makeinvisible == MAKEINVISIBLE_WINLOSEFOCUS)
			{
				//We have to do a 10 millisecond delay before making the window invisible here.
				//This allows other events in the queue - most specifically, popup menu - to
				//occur.  Otherwise, bringing up the popup menu is impossible (window disappears,
				//and popup menu event is cancelled).
				SetTimer(hwnd, TIMER_MAKEINVISIBLE, 10, NULL);
			}
			goto pass_to_control;
			break;

		//--------------------------------------
		// autohide
		case WM_MOUSEMOVE:
			// on mouseover, unhide the window and start a poll-timer
			if (!w->is_autohidden)
				goto pass_to_control;

		case WM_NCMOUSEMOVE:
			if (w->is_autohidden && false==(GetAsyncKeyState(VK_CONTROL) & 0x8000))
			{
				w->is_autohidden = false;
				if (false == w->is_slitted)
				{
					window_update(w, true, true, false, false);
					SetTimer(hwnd, TIMER_AUTOHIDE, AUTOHIDE_DELAY, NULL);
				}
			}
			break;

		// on timer, check if the mouse is still over...
		case WM_TIMER:
			if (wParam == TIMER_AUTOHIDE)
			{
				bool hide_it = window_test_autohide(w, NULL);
				if (hide_it)
				{
					POINT pt; RECT rct;
					GetCursorPos(&pt);
					GetWindowRect(hwnd, &rct);
					if (PtInRect(&rct, pt) || (0x8000 & GetKeyState(VK_LBUTTON)))
						break;
				}
				w->is_autohidden = hide_it;
				KillTimer(hwnd, wParam);
				window_update(w, true, true, false, false);
				break;
			}
			else if (wParam == TIMER_MAKEINVISIBLE)
			{
				w->is_visible = false;
				window_update(w, false, false, true, false);
				KillTimer(hwnd, wParam);
			}
			goto pass_to_control;


		// ---------------------------------------------------
		// Handled entirely by control
		case WM_PAINT:
		case WM_KILLFOCUS:
		case SLIT_ADD:
		case SLIT_REMOVE:
		case SLIT_UPDATE:
		case WM_DROPFILES:
		pass_to_control:
			return control_event(controlpointer, hwnd, msg, wParam, lParam);

		// ---------------------------------------------------
		// no alt-F4
		case WM_CLOSE:
			break;

		case WM_ERASEBKGND:
			return TRUE;

		// ---------------------------------------------------
		// For tooltip
		case WM_NOTIFY:
			tooltip_update((NMTTDISPINFO*)lParam, controlpointer);
			break;

		case WM_CREATE:
			tooltip_add(hwnd);
			break;

		case WM_DESTROY:
			tooltip_del(hwnd);
			break;

		// ---------------------------------------------------
		// bring window into foreground on sizing/moving-start
		case WM_NCLBUTTONDOWN:
			SetWindowPos(hwnd, HWND_TOP, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOSENDCHANGING);
			UpdateWindow(hwnd);
			// fall through

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_message(int tokencount, char *tokens[], bool from_core, module* caller)
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_message_setproperty
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_message_setproperty(control *c, int tokencount, char *tokens[])
{
	//Variables
	bool needs_slitupdate = false;
	bool needs_posupdate = false;
	bool needs_transupdate = false;
	bool needs_visupdate = false;
	bool needs_stickyupdate = false;
	int errors = 0;
	int i;

	window *w = c->windowptr;

	if (tokencount == 5)
	{
		if (!stricmp(tokens[4], "Detach") && control_make_parentless(c))
		{
			window_make_child(w, NULL);
			needs_posupdate = needs_transupdate = needs_visupdate = needs_stickyupdate = true;
		}
		else
			errors = 1;
	}
	else
	//Six tokens required
	if (tokencount != 6)
		errors = 1;
	else
	//Figure out which property to set, and what to do once it's set
	if (!stricmp(tokens[4], szWPx) && config_set_int_expr(tokens[5], &w->x))
		{ needs_posupdate = true; }
	else
	if (!stricmp(tokens[4], szWPy) && config_set_int_expr(tokens[5], &w->y))
		{ needs_posupdate = true; }
	else
	if (!stricmp(tokens[4], szWPwidth) && config_set_int_expr(tokens[5], &w->width))
		{ style_draw_invalidate(c); needs_posupdate = true; }
	else
	if (!stricmp(tokens[4], szWPheight) && config_set_int_expr(tokens[5], &w->height))
		{ style_draw_invalidate(c); needs_posupdate = true; }
	else
	if (!stricmp(tokens[4], szWPisbordered) && config_set_bool(tokens[5], &w->is_bordered))
		{ style_draw_invalidate(c); }
	else
	if (!stricmp(tokens[4], szWPStyle) && -1 != (i = get_string_index(tokens[5], szStyleNames)))
	{
		if(w->has_custom_style) { delete w->styleptr; w->styleptr = 0; w->has_custom_style = false; } //get rid of previous custom style
		w->style = i; style_draw_invalidate(c); needs_transupdate = true;
	}
	else
	if (-1 != (i = get_string_index(tokens[4], szStyleProperties)) )
	{
		if (!w->has_custom_style) {
			w->has_custom_style = true; w->styleptr = new StyleItem; *w->styleptr = style_get_copy(w->style);
		}
		COLORREF colorval;
		int beveltype;
		if ( (i<3) && -1 == ( colorval = ReadColorFromString(tokens[5])) ) errors = 1;
		if ( (i==3) && -1 == ( beveltype = get_string_index(tokens[5], szBevelTypes)) ) errors = 1;
		if ( (i==4) && !config_set_int(tokens[5], &(w->styleptr->bevelposition), 1, 2)) errors = 1;
		if (!errors)
		{	switch (i)
			{	case 0: w->styleptr->Color = colorval; break;
				case 1: w->styleptr->ColorTo = colorval; break;
				case 2: w->styleptr->TextColor = colorval; break;
				case 3: w->styleptr->bevelstyle = beveltype;
					if (beveltype != 0 && w->styleptr->bevelposition == 0)
						w->styleptr->bevelposition = 1;
					break;
			}

			style_draw_invalidate(c); needs_transupdate = true; 
		}
	}
	else
	if (!stricmp(tokens[4], szWPisvisible) && config_set_bool(tokens[5], &w->is_visible))
		{ needs_visupdate = true; }
	else
	if (!c->parentptr)
	{
		if (!stricmp(tokens[4], szWPtransparency) && config_set_int(tokens[5], &w->transparency, 0, 100))
			{ needs_transupdate = true; }
		else
		if (!stricmp(tokens[4], szWPisonallworkspaces) && config_set_bool(tokens[5], &w->is_onallworkspaces))
			{ needs_stickyupdate = true; }
		else
		if (!stricmp(tokens[4], szWPisontop) && config_set_bool(tokens[5], &w->is_ontop))
			{ needs_posupdate = true; }
		else
		if (!stricmp(tokens[4], szWPisslitted) && config_set_bool(tokens[5], &w->useslit))
			{ needs_slitupdate = true; }
		else
		if (!stricmp(tokens[4], szWPistoggledwithplugins) && config_set_bool(tokens[5], &w->is_toggledwithplugins))
			{ needs_visupdate = true; }
		else
		if (!stricmp(tokens[4], szWPistransparent) && config_set_bool(tokens[5], &w->is_transparent))
			{ needs_transupdate = true; }
		else
		if (!stricmp(tokens[4], szWPautohide) && config_set_bool(tokens[5], &w->autohide))
			{ needs_posupdate = needs_transupdate = true; w->is_autohidden = window_test_autohide(w, NULL); }
		else
		if (!stricmp(tokens[4], "AttachTo") && control_make_childof(c, tokens[5]))
		{
			window_make_child(w, c->parentptr->windowptr);
			needs_posupdate = needs_transupdate = needs_visupdate = needs_stickyupdate = true;
		}
		else
		if (!stricmp(tokens[4], szWPmakeinvisible))
		{			
			if (!stricmp(tokens[5], szWPmakeinvisible_winblur)) w->makeinvisible = MAKEINVISIBLE_WINLOSEFOCUS;
			else if (!stricmp(tokens[5], szWPmakeinvisible_bbblur)) w->makeinvisible = MAKEINVISIBLE_BBLOSEFOCUS;
			else w->makeinvisible = MAKEINVISIBLE_NEVER;
		}
		else
			errors = 1;
	}
	else
		errors = 1;

	//Update the window as necessary
	window_update(w, needs_posupdate, needs_transupdate, needs_visupdate, needs_stickyupdate);

	//Return
	return errors;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_pluginsvisible
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void window_pluginsvisible(bool isvisible)
{
	window_is_pluginsvisible = isvisible;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_helper_register
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_helper_register(const char *classname, WNDPROC callbackfunc)
{
	//Create a window to recieve events
	//Define the window class
	WNDCLASS wc;
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc      = callbackfunc; // our window procedure
	wc.hInstance        = plugin_instance_plugin;       // hInstance of .dll
	wc.lpszClassName    = (LPCWSTR)classname;            // our window class name
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wc.style            = CS_DBLCLKS;

	//Register the class
	if (!RegisterClass(&wc)) return 1;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_helper_unregister
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_helper_unregister(const char *classname)
{
	//Unregister the class
	UnregisterClass((LPCWSTR)classname, plugin_instance_plugin);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_helper_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HWND window_helper_create(const char *classname)
{
	HWND hwnd;
	//Create a window
	hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,          // window style
		(LPCWSTR)classname,                 // our window class name
		NULL,                      // NULL -> does not show up in task manager!
		WS_POPUP,                  // window parameters
		0,                         // x position
		0,                         // y position
		0,                         // window width
		0,                         // window height
		NULL,                      // parent window
		NULL,                      // no menu
		plugin_instance_plugin,    // hInstance of .dll
		NULL);

	//Check to make sure it's okay
	if (!hwnd)
	{                
		return NULL;
	}

	return hwnd;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_helper_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int window_helper_destroy(HWND hwnd)
{
	DestroyWindow(hwnd);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//window_update
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void window_update(window *w, bool position, bool transparency, bool visibility, bool sticky)
{
	bool visible = w->is_visible &&
		(window_is_pluginsvisible || false == w->is_toggledwithplugins);

	bool useslit = w->useslit && NULL!=plugin_hwnd_slit && visible && NULL == w->controlptr->parentptr;

	if (useslit != w->is_slitted)
		position = transparency = true;

	// ------------------------------------------
	if (false == useslit && w->is_slitted)
	{
		SendMessage(plugin_hwnd_slit, SLIT_REMOVE, 0, (LPARAM) w->hwnd);
		w->is_slitted = false;
	}

	// ------------------------------------------
	if (position)
	{
		bool set_pos    = false == useslit;
		bool set_zorder = set_pos && NULL == w->controlptr->parentptr;

		POINT pt = { w->x, w->y };
		bool can_hide = window_test_autohide(w, &pt);

		SetWindowPos(
			w->hwnd,
			set_zorder ? (can_hide || w->is_ontop ? HWND_TOPMOST : HWND_NOTOPMOST) : NULL,
			pt.x,
			pt.y,
			w->width,
			w->height,
			set_zorder
				? SWP_NOACTIVATE|SWP_NOSENDCHANGING
				:
			set_pos
				? SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSENDCHANGING
				: SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSENDCHANGING
			);
	}

	// ------------------------------------------
	if (visible != (FALSE!=IsWindowVisible(w->hwnd)))
	{
		ShowWindow(w->hwnd, visible ? SW_SHOWNA : SW_HIDE);
		if (visible) SetActiveWindow(w->hwnd);
	}

	// ------------------------------------------
	if (useslit)
	{
		if (false == w->is_slitted)
		{
			w->is_slitted = true;
			window_set_transparency(w);
			SendMessage(plugin_hwnd_slit, SLIT_ADD, 0, (LPARAM) w->hwnd);
		}
		else
		if (position)
		{
			SendMessage(plugin_hwnd_slit, SLIT_UPDATE, 0, (LPARAM) w->hwnd);
		}
	}

	// ------------------------------------------
	else //if (false == useslit)
	{
		//If we need a transparency update
		if (transparency)
			window_set_transparency(w);       

		//If we need a sticky update
		if (sticky)
		{
			if (w->is_onallworkspaces)
				MakeSticky(w->hwnd);
			else
				RemoveSticky(w->hwnd);
		}
	}

	// ------------------------------------------
}

//##################################################
//window_set_transparency
//##################################################
void window_set_transparency(window *w)
{
	if (false == plugin_using_modern_os)
		return;

	int transvalue = iminmax(w->transparency * 255 / 100, 0, 255);

	if (false == w->is_transparent)
		transvalue = 255;

	if (w->is_autohidden)
		transvalue = 8;

	if (w->is_slitted)
		transvalue = 255;

	// transparency (win2k+ only)
	style_set_transparency(w->hwnd, (BYTE)transvalue, w->style == STYLETYPE_NONE);
}

//##################################################
bool window_test_autohide(window *w, POINT *pt)
{
	if (false == w->autohide || w->is_slitted)
		return false;

	int x = w->x;
	int y = w->y;

	RECT scrn;
	get_mon_rect(get_monitor(w->hwnd), &scrn);

	if (x == scrn.left) x = 1-w->width;
	else
	if (x == scrn.right - w->width) x = scrn.right - 1;
	else
	if (y == scrn.top) y = 1 - w->height;
	else
	if (y == scrn.bottom - w->height) y = scrn.bottom - 1;
	else
		return false;

	if (pt && w->is_autohidden)
		pt->x = x, pt->y = y;

	return true;
}

//##################################################

void window_make_child(window *w, window *pw)
{
	HWND hwnd = w->hwnd;
	HWND parent_hwnd = pw ? pw->hwnd : NULL;
	RECT r; GetWindowRect(hwnd, &r);

	SetParent(hwnd, parent_hwnd);
	SetWindowLong(hwnd, GWL_STYLE, (GetWindowLong(hwnd, GWL_STYLE) & ~(WS_POPUP|WS_CHILD)) | (parent_hwnd ? WS_CHILD : WS_POPUP));

	if (parent_hwnd)
	{
		ScreenToClient(parent_hwnd, (POINT*)&r.left);
		w->x = iminmax(r.left - 4, 2, pw->width - 2 - w->width);
		w->y = iminmax(r.top - 4, 2, pw->height - 2 - w->height);
	}
	else
	{
		w->x = r.left + 4;
		w->y = r.top + 4;
	}
}

//##################################################

//===========================================================================
// Function: ParseLiteralColor
// Purpose: Parses a given literal colour and returns the hex value
// In: LPCSTR = color to parse (eg. "black", "white")
// Out: COLORREF (DWORD) of rgb value
// (old)Out: LPCSTR = literal hex value
//===========================================================================

static struct litcolor1 { char *cname; COLORREF cref; } litcolor1_ary[] = {

    { "ghostwhite", RGB(248,248,255) },
    { "whitesmoke", RGB(245,245,245) },
    { "gainsboro", RGB(220,220,220) },
    { "floralwhite", RGB(255,250,240) },
    { "oldlace", RGB(253,245,230) },
    { "linen", RGB(250,240,230) },
    { "antiquewhite", RGB(250,235,215) },
    { "papayawhip", RGB(255,239,213) },
    { "blanchedalmond", RGB(255,235,205) },
    { "bisque", RGB(255,228,196) },
    { "peachpuff", RGB(255,218,185) },
    { "navajowhite", RGB(255,222,173) },
    { "moccasin", RGB(255,228,181) },
    { "cornsilk", RGB(255,248,220) },
    { "ivory", RGB(255,255,240) },
    { "lemonchiffon", RGB(255,250,205) },
    { "seashell", RGB(255,245,238) },
    { "honeydew", RGB(240,255,240) },
    { "mintcream", RGB(245,255,250) },
    { "azure", RGB(240,255,255) },
    { "aliceblue", RGB(240,248,255) },
    { "lavender", RGB(230,230,250) },
    { "lavenderblush", RGB(255,240,245) },
    { "mistyrose", RGB(255,228,225) },
    { "white", RGB(255,255,255) },
    { "black", RGB(0,0,0) },
    { "darkslategray", RGB(47,79,79) },
    { "dimgray", RGB(105,105,105) },
    { "slategray", RGB(112,128,144) },
    { "lightslategray", RGB(119,136,153) },
    { "gray", RGB(190,190,190) },
    { "lightgray", RGB(211,211,211) },
    { "midnightblue", RGB(25,25,112) },
    { "navy", RGB(0,0,128) },
    { "navyblue", RGB(0,0,128) },
    { "cornflowerblue", RGB(100,149,237) },
    { "darkslateblue", RGB(72,61,139) },
    { "slateblue", RGB(106,90,205) },
    { "mediumslateblue", RGB(123,104,238) },
    { "lightslateblue", RGB(132,112,255) },
    { "mediumblue", RGB(0,0,205) },
    { "royalblue", RGB(65,105,225) },
    { "blue", RGB(0,0,255) },
    { "dodgerblue", RGB(30,144,255) },
    { "deepskyblue", RGB(0,191,255) },
    { "skyblue", RGB(135,206,235) },
    { "lightskyblue", RGB(135,206,250) },
    { "steelblue", RGB(70,130,180) },
    { "lightsteelblue", RGB(176,196,222) },
    { "lightblue", RGB(173,216,230) },
    { "powderblue", RGB(176,224,230) },
    { "paleturquoise", RGB(175,238,238) },
    { "darkturquoise", RGB(0,206,209) },
    { "mediumturquoise", RGB(72,209,204) },
    { "turquoise", RGB(64,224,208) },
    { "cyan", RGB(0,255,255) },
    { "lightcyan", RGB(224,255,255) },
    { "cadetblue", RGB(95,158,160) },
    { "mediumaquamarine", RGB(102,205,170) },
    { "aquamarine", RGB(127,255,212) },
    { "darkgreen", RGB(0,100,0) },
    { "darkolivegreen", RGB(85,107,47) },
    { "darkseagreen", RGB(143,188,143) },
    { "seagreen", RGB(46,139,87) },
    { "mediumseagreen", RGB(60,179,113) },
    { "lightseagreen", RGB(32,178,170) },
    { "palegreen", RGB(152,251,152) },
    { "springgreen", RGB(0,255,127) },
    { "lawngreen", RGB(124,252,0) },
    { "green", RGB(0,255,0) },
    { "chartreuse", RGB(127,255,0) },
    { "mediumspringgreen", RGB(0,250,154) },
    { "greenyellow", RGB(173,255,47) },
    { "limegreen", RGB(50,205,50) },
    { "yellowgreen", RGB(154,205,50) },
    { "forestgreen", RGB(34,139,34) },
    { "olivedrab", RGB(107,142,35) },
    { "darkkhaki", RGB(189,183,107) },
    { "khaki", RGB(240,230,140) },
    { "palegoldenrod", RGB(238,232,170) },
    { "lightgoldenrodyellow", RGB(250,250,210) },
    { "lightyellow", RGB(255,255,224) },
    { "yellow", RGB(255,255,0) },
    { "gold", RGB(255,215,0) },
    { "lightgoldenrod", RGB(238,221,130) },
    { "goldenrod", RGB(218,165,32) },
    { "darkgoldenrod", RGB(184,134,11) },
    { "rosybrown", RGB(188,143,143) },
    { "indianred", RGB(205,92,92) },
    { "saddlebrown", RGB(139,69,19) },
    { "sienna", RGB(160,82,45) },
    { "peru", RGB(205,133,63) },
    { "burlywood", RGB(222,184,135) },
    { "beige", RGB(245,245,220) },
    { "wheat", RGB(245,222,179) },
    { "sandybrown", RGB(244,164,96) },
    { "tan", RGB(210,180,140) },
    { "chocolate", RGB(210,105,30) },
    { "firebrick", RGB(178,34,34) },
    { "brown", RGB(165,42,42) },
    { "darksalmon", RGB(233,150,122) },
    { "salmon", RGB(250,128,114) },
    { "lightsalmon", RGB(255,160,122) },
    { "orange", RGB(255,165,0) },
    { "darkorange", RGB(255,140,0) },
    { "coral", RGB(255,127,80) },
    { "lightcoral", RGB(240,128,128) },
    { "tomato", RGB(255,99,71) },
    { "orangered", RGB(255,69,0) },
    { "red", RGB(255,0,0) },
    { "hotpink", RGB(255,105,180) },
    { "deeppink", RGB(255,20,147) },
    { "pink", RGB(255,192,203) },
    { "lightpink", RGB(255,182,193) },
    { "palevioletred", RGB(219,112,147) },
    { "maroon", RGB(176,48,96) },
    { "mediumvioletred", RGB(199,21,133) },
    { "violetred", RGB(208,32,144) },
    { "magenta", RGB(255,0,255) },
    { "violet", RGB(238,130,238) },
    { "plum", RGB(221,160,221) },
    { "orchid", RGB(218,112,214) },
    { "mediumorchid", RGB(186,85,211) },
    { "darkorchid", RGB(153,50,204) },
    { "darkviolet", RGB(148,0,211) },
    { "blueviolet", RGB(138,43,226) },
    { "purple", RGB(160,32,240) },
    { "mediumpurple", RGB(147,112,219) },
    { "thistle", RGB(216,191,216) },

    { "darkgray", RGB(169,169,169) },
    { "darkblue", RGB(0,0,139) },
    { "darkcyan", RGB(0,139,139) },
    { "darkmagenta", RGB(139,0,139) },
    { "darkred", RGB(139,0,0) },
    { "lightgreen", RGB(144,238,144) }
    };

static struct litcolor4 { char *cname; COLORREF cref[4]; } litcolor4_ary[] = {

    { "snow", { RGB(255,250,250), RGB(238,233,233), RGB(205,201,201), RGB(139,137,137) }},
    { "seashell", { RGB(255,245,238), RGB(238,229,222), RGB(205,197,191), RGB(139,134,130) }},
    { "antiquewhite", { RGB(255,239,219), RGB(238,223,204), RGB(205,192,176), RGB(139,131,120) }},
    { "bisque", { RGB(255,228,196), RGB(238,213,183), RGB(205,183,158), RGB(139,125,107) }},
    { "peachpuff", { RGB(255,218,185), RGB(238,203,173), RGB(205,175,149), RGB(139,119,101) }},
    { "navajowhite", { RGB(255,222,173), RGB(238,207,161), RGB(205,179,139), RGB(139,121,94) }},
    { "lemonchiffon", { RGB(255,250,205), RGB(238,233,191), RGB(205,201,165), RGB(139,137,112) }},
    { "cornsilk", { RGB(255,248,220), RGB(238,232,205), RGB(205,200,177), RGB(139,136,120) }},
    { "ivory", { RGB(255,255,240), RGB(238,238,224), RGB(205,205,193), RGB(139,139,131) }},
    { "honeydew", { RGB(240,255,240), RGB(224,238,224), RGB(193,205,193), RGB(131,139,131) }},
    { "lavenderblush", { RGB(255,240,245), RGB(238,224,229), RGB(205,193,197), RGB(139,131,134) }},
    { "mistyrose", { RGB(255,228,225), RGB(238,213,210), RGB(205,183,181), RGB(139,125,123) }},
    { "azure", { RGB(240,255,255), RGB(224,238,238), RGB(193,205,205), RGB(131,139,139) }},
    { "slateblue", { RGB(131,111,255), RGB(122,103,238), RGB(105,89,205), RGB(71,60,139) }},
    { "royalblue", { RGB(72,118,255), RGB(67,110,238), RGB(58,95,205), RGB(39,64,139) }},
    { "blue", { RGB(0,0,255), RGB(0,0,238), RGB(0,0,205), RGB(0,0,139) }},
    { "dodgerblue", { RGB(30,144,255), RGB(28,134,238), RGB(24,116,205), RGB(16,78,139) }},
    { "steelblue", { RGB(99,184,255), RGB(92,172,238), RGB(79,148,205), RGB(54,100,139) }},
    { "deepskyblue", { RGB(0,191,255), RGB(0,178,238), RGB(0,154,205), RGB(0,104,139) }},
    { "skyblue", { RGB(135,206,255), RGB(126,192,238), RGB(108,166,205), RGB(74,112,139) }},
    { "lightskyblue", { RGB(176,226,255), RGB(164,211,238), RGB(141,182,205), RGB(96,123,139) }},
    { "slategray", { RGB(198,226,255), RGB(185,211,238), RGB(159,182,205), RGB(108,123,139) }},
    { "lightsteelblue", { RGB(202,225,255), RGB(188,210,238), RGB(162,181,205), RGB(110,123,139) }},
    { "lightblue", { RGB(191,239,255), RGB(178,223,238), RGB(154,192,205), RGB(104,131,139) }},
    { "lightcyan", { RGB(224,255,255), RGB(209,238,238), RGB(180,205,205), RGB(122,139,139) }},
    { "paleturquoise", { RGB(187,255,255), RGB(174,238,238), RGB(150,205,205), RGB(102,139,139) }},
    { "cadetblue", { RGB(152,245,255), RGB(142,229,238), RGB(122,197,205), RGB(83,134,139) }},
    { "turquoise", { RGB(0,245,255), RGB(0,229,238), RGB(0,197,205), RGB(0,134,139) }},
    { "cyan", { RGB(0,255,255), RGB(0,238,238), RGB(0,205,205), RGB(0,139,139) }},
    { "darkslategray", { RGB(151,255,255), RGB(141,238,238), RGB(121,205,205), RGB(82,139,139) }},
    { "aquamarine", { RGB(127,255,212), RGB(118,238,198), RGB(102,205,170), RGB(69,139,116) }},
    { "darkseagreen", { RGB(193,255,193), RGB(180,238,180), RGB(155,205,155), RGB(105,139,105) }},
    { "seagreen", { RGB(84,255,159), RGB(78,238,148), RGB(67,205,128), RGB(46,139,87) }},
    { "palegreen", { RGB(154,255,154), RGB(144,238,144), RGB(124,205,124), RGB(84,139,84) }},
    { "springgreen", { RGB(0,255,127), RGB(0,238,118), RGB(0,205,102), RGB(0,139,69) }},
    { "green", { RGB(0,255,0), RGB(0,238,0), RGB(0,205,0), RGB(0,139,0) }},
    { "chartreuse", { RGB(127,255,0), RGB(118,238,0), RGB(102,205,0), RGB(69,139,0) }},
    { "olivedrab", { RGB(192,255,62), RGB(179,238,58), RGB(154,205,50), RGB(105,139,34) }},
    { "darkolivegreen", { RGB(202,255,112), RGB(188,238,104), RGB(162,205,90), RGB(110,139,61) }},
    { "khaki", { RGB(255,246,143), RGB(238,230,133), RGB(205,198,115), RGB(139,134,78) }},
    { "lightgoldenrod", { RGB(255,236,139), RGB(238,220,130), RGB(205,190,112), RGB(139,129,76) }},
    { "lightyellow", { RGB(255,255,224), RGB(238,238,209), RGB(205,205,180), RGB(139,139,122) }},
    { "yellow", { RGB(255,255,0), RGB(238,238,0), RGB(205,205,0), RGB(139,139,0) }},
    { "gold", { RGB(255,215,0), RGB(238,201,0), RGB(205,173,0), RGB(139,117,0) }},
    { "goldenrod", { RGB(255,193,37), RGB(238,180,34), RGB(205,155,29), RGB(139,105,20) }},
    { "darkgoldenrod", { RGB(255,185,15), RGB(238,173,14), RGB(205,149,12), RGB(139,101,8) }},
    { "rosybrown", { RGB(255,193,193), RGB(238,180,180), RGB(205,155,155), RGB(139,105,105) }},
    { "indianred", { RGB(255,106,106), RGB(238,99,99), RGB(205,85,85), RGB(139,58,58) }},
    { "sienna", { RGB(255,130,71), RGB(238,121,66), RGB(205,104,57), RGB(139,71,38) }},
    { "burlywood", { RGB(255,211,155), RGB(238,197,145), RGB(205,170,125), RGB(139,115,85) }},
    { "wheat", { RGB(255,231,186), RGB(238,216,174), RGB(205,186,150), RGB(139,126,102) }},
    { "tan", { RGB(255,165,79), RGB(238,154,73), RGB(205,133,63), RGB(139,90,43) }},
    { "chocolate", { RGB(255,127,36), RGB(238,118,33), RGB(205,102,29), RGB(139,69,19) }},
    { "firebrick", { RGB(255,48,48), RGB(238,44,44), RGB(205,38,38), RGB(139,26,26) }},
    { "brown", { RGB(255,64,64), RGB(238,59,59), RGB(205,51,51), RGB(139,35,35) }},
    { "salmon", { RGB(255,140,105), RGB(238,130,98), RGB(205,112,84), RGB(139,76,57) }},
    { "lightsalmon", { RGB(255,160,122), RGB(238,149,114), RGB(205,129,98), RGB(139,87,66) }},
    { "orange", { RGB(255,165,0), RGB(238,154,0), RGB(205,133,0), RGB(139,90,0) }},
    { "darkorange", { RGB(255,127,0), RGB(238,118,0), RGB(205,102,0), RGB(139,69,0) }},
    { "coral", { RGB(255,114,86), RGB(238,106,80), RGB(205,91,69), RGB(139,62,47) }},
    { "tomato", { RGB(255,99,71), RGB(238,92,66), RGB(205,79,57), RGB(139,54,38) }},
    { "orangered", { RGB(255,69,0), RGB(238,64,0), RGB(205,55,0), RGB(139,37,0) }},
    { "red", { RGB(255,0,0), RGB(238,0,0), RGB(205,0,0), RGB(139,0,0) }},
    { "deeppink", { RGB(255,20,147), RGB(238,18,137), RGB(205,16,118), RGB(139,10,80) }},
    { "hotpink", { RGB(255,110,180), RGB(238,106,167), RGB(205,96,144), RGB(139,58,98) }},
    { "pink", { RGB(255,181,197), RGB(238,169,184), RGB(205,145,158), RGB(139,99,108) }},
    { "lightpink", { RGB(255,174,185), RGB(238,162,173), RGB(205,140,149), RGB(139,95,101) }},
    { "palevioletred", { RGB(255,130,171), RGB(238,121,159), RGB(205,104,137), RGB(139,71,93) }},
    { "maroon", { RGB(255,52,179), RGB(238,48,167), RGB(205,41,144), RGB(139,28,98) }},
    { "violetred", { RGB(255,62,150), RGB(238,58,140), RGB(205,50,120), RGB(139,34,82) }},
    { "magenta", { RGB(255,0,255), RGB(238,0,238), RGB(205,0,205), RGB(139,0,139) }},
    { "orchid", { RGB(255,131,250), RGB(238,122,233), RGB(205,105,201), RGB(139,71,137) }},
    { "plum", { RGB(255,187,255), RGB(238,174,238), RGB(205,150,205), RGB(139,102,139) }},
    { "mediumorchid", { RGB(224,102,255), RGB(209,95,238), RGB(180,82,205), RGB(122,55,139) }},
    { "darkorchid", { RGB(191,62,255), RGB(178,58,238), RGB(154,50,205), RGB(104,34,139) }},
    { "purple", { RGB(155,48,255), RGB(145,44,238), RGB(125,38,205), RGB(85,26,139) }},
    { "mediumpurple", { RGB(171,130,255), RGB(159,121,238), RGB(137,104,205), RGB(93,71,139) }},
    { "thistle", { RGB(255,225,255), RGB(238,210,238), RGB(205,181,205), RGB(139,123,139) }}
    };


COLORREF ParseLiteralColor(LPCSTR colour)
{
    int i, n; unsigned l; char *p, c, buf[32];
    l = strlen(colour) + 1;
    if (l > 2 && l < sizeof buf)
    {
        memcpy(buf, colour, l); //strlwr(buf);
        while (NULL!=(p=strchr(buf,' '))) strcpy(p, p+1);
        if (NULL!=(p=strstr(buf,"grey"))) p[2]='a';
        if (0==memcmp(buf,"gray", 4) && (c=buf[4]) >= '0' && c <= '9')
        {
            i = atoi(buf+4);
            if (i >= 0 && i <= 100)
            {
                i = (i * 255 + 50) / 100;
                return RGB(i,i,i);
            }
        }
        i = *(p = &buf[l-2]) - '1';
        if (i>=0 && i<4)
        {
            *p=0; --l;
            struct litcolor4 *cp4=litcolor4_ary;
            n = sizeof(litcolor4_ary) / sizeof(*cp4);
            do { if (0==memcmp(buf, cp4->cname, l)) return cp4->cref[i]; cp4++; }
            while (--n);
        }
        else
        {
            struct litcolor1 *cp1=litcolor1_ary;
            n = sizeof(litcolor1_ary) / sizeof(*cp1);
            do { if (0==memcmp(buf, cp1->cname, l)) return cp1->cref; cp1++; }
            while (--n);
        }
    }
    return (COLORREF)-1;
}


COLORREF switch_rgb (COLORREF c)
{ return (c&0x0000ff)<<16 | (c&0x00ff00) | (c&0xff0000)>>16; }
//===========================================================================
// Function: ReadColorFromString
// Purpose: parse a literal or hexadezimal color string
// --- Straight from the bblean source.

COLORREF ReadColorFromString(char * string)
{
    if (NULL == string) return (COLORREF)-1;
    char rgbstr[7];
    char *s = strlwr(string);
    if ('#'==*s) s++;
    for (;;)
    {
        COLORREF cr = 0; char *d, c;
        // check if its a valid hex number
        for (d = s; (c = *d) != 0; ++d)
        {
            cr <<= 4;
            if (c >= '0' && c <= '9') cr |= c - '0';
            else
            if (c >= 'a' && c <= 'f') cr |= c - ('a'-10);
            else goto check_rgb;
        }

        if (d - s == 3) // #AB4 short type colors
            cr = (cr&0xF00)<<12 | (cr&0xFF0)<<8 | (cr&0x0FF)<<4 | cr&0x00F;

        return switch_rgb(cr);

check_rgb:
        // check if its an "rgb:12/ee/4c" type string
//        s = stub;
        if (0 == memcmp(s, "rgb:", 4))
        {
            int j=3; s+=4; d = rgbstr;
            do {
                d[0] = *s && '/'!=*s ? *s++ : '0';
                d[1] = *s && '/'!=*s ? *s++ : d[0];
                d+=2; if ('/'==*s) ++s;
            } while (--j);
            *d=0; s = rgbstr;
            continue;
        } else
		// Check for an "rgb10:123/45/255" type string
       if (0 == memcmp(s, "rgb10:", 6))
        {
            s+=6;
			int cval[3];
			if (sscanf(s,"%d/%d/%d",cval,cval+1,cval+2) == 3)
			{
				sprintf(rgbstr,"%02x%02x%02x",cval[0]%256,cval[1]%256,cval[2]%256);
				s = rgbstr;
				continue;
			}
        }

        // must be one of the literal color names (or is invalid)
        return ParseLiteralColor(s);
    }
}
