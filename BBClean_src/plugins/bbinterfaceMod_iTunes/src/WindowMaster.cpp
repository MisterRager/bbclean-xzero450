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
#include "ColorMaster.h"

//Global variables
char szWPx                      [] = "X";
char szWPy                      [] = "Y";
char szWPwidth                  [] = "Width";
char szWPheight                 [] = "Height";
char szWPtransparency           [] = "Transparency";
char szWPworkspacenumber        [] = "WorkspaceNumber";
char szWPisvisible              [] = "IsVisible";
char szWPisontop                [] = "IsOnTop";
char szWPissnappy               [] = "IsSnappy";
char szWPisslitted              [] = "IsSlitted";
char szWPistransparent          [] = "IsTransparent";
char szWPistoggledwithplugins   [] = "IsToggledWithPlugins";
char szWPisonallworkspaces      [] = "IsOnAllWorkspaces";
char szWPisdetectfullscreen     [] = "DetectFullScreen";
char szWPisbordered             [] = "Border";
char szWPStyle                  [] = "Style";
char szWPStyleWhenPressed       [] = "StyleWhenPressed";
char szWPautohide               [] = "AutoHide";
char szWPmakeinvisible          [] = "MakeInvisible";
char szWPmakeinvisible_never    [] = "Never";
char szWPmakeinvisible_bbblur   [] = "BBLoseFocus";
char szWPmakeinvisible_winblur  [] = "WindowLoseFocus";
char szWPfontname               [] = "FontName";
char szWPfontname_pressed       [] = "PressedFontName";
char szWPfontheight             [] = "FontSize";
char szWPfontheight_pressed     [] = "PressedFontSize";
char szWPfontweight             [] = "FontBold";
char szWPfontweight_pressed     [] = "PressedFontBold";


//Local variables
bool window_is_pluginsvisible = true;

//Internal functions
void window_set_transparency(window *w);
bool window_test_autohide(window *w, POINT *pt);

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
	wc.lpszClassName    = szAppName;                    // our window class name
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wc.style            = CS_DBLCLKS;
	wc.cbWndExtra       = sizeof (control*); 

	if (!RegisterClass(&wc)) 
	{
		if (!plugin_suppresserrors) BBMessageBox(plugin_hwnd_blackbox, "Error registering window class", szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);
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
	UnregisterClass(szAppName, plugin_instance_plugin);

	
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
	windowpointer->is_detectfullscreen = false;
	windowpointer->is_bordered = true;
	windowpointer->style = STYLETYPE_TOOLBAR;
	windowpointer->styleptr = 0;
	windowpointer->has_custom_style = false;
	windowpointer->font = NULL;
	windowpointer->use_custom_font = false;
	strcpy(windowpointer->Fontname,"");
	windowpointer->FontHeight = 0;
	windowpointer->FontWeight = FW_NORMAL;
	windowpointer->nosavevalue = 0;

	windowpointer->makeinvisible = MAKEINVISIBLE_NEVER;
	windowpointer->autohide = false;
	windowpointer->useslit = false;
	windowpointer->is_slitted = false;
	windowpointer->is_transparent = false;
	windowpointer->transparency = 100;
	windowpointer->workspacenumber = 0;

	// other initialisation
	windowpointer->bitmap = NULL;
	windowpointer->is_moving = false;
	windowpointer->is_autohidden = false;

	//set button only properties
	if(c->controltypeptr->id == CONTROL_ID_BUTTON || c->controltypeptr->id ==CONTROL_ID_SWITCHBUTTON){
		windowpointer->is_button = true;
		windowpointer->bstyleptr = new struct ButtonStyleInfo;
		windowpointer->bstyleptr->style = STYLETYPE_DEFAULT;
		windowpointer->bstyleptr->styleptr = 0;
		windowpointer->bstyleptr->has_custom_style = false;
		windowpointer->bstyleptr->use_custom_font = false;
		windowpointer->bstyleptr->font = NULL;
		strcpy(windowpointer->bstyleptr->Fontname,"");
		windowpointer->bstyleptr->FontHeight = 0;
		windowpointer->bstyleptr->FontWeight = FW_NORMAL;
		windowpointer->bstyleptr->nosavevalue = 0;
	}else{
		windowpointer->is_button = false;
	}
	if(c->controltypeptr->id == CONTROL_ID_SLIDER){
		windowpointer->is_slider = true;
		windowpointer->sstyleptr = new struct SliderStyleInfo;
		windowpointer->sstyleptr->style = STYLETYPE_TOOLBAR;
		windowpointer->sstyleptr->styleptr = 0;
		windowpointer->sstyleptr->has_custom_style = false;
		windowpointer->sstyleptr->draw_inner = false;
		windowpointer->sstyleptr->in_style = STYLETYPE_INSET;
		windowpointer->sstyleptr->in_styleptr = 0;
		windowpointer->sstyleptr->in_has_custom_style = false;
	}else{
		windowpointer->is_slider = false;
	}
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
		szAppName,                      // our window class name
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
	if (w->use_custom_font) DeleteObject(w->font);
	
	//Free memory (button only settings)
	if (w->is_button){
		if(w->bstyleptr->has_custom_style) delete w->bstyleptr->styleptr;
		if(w->use_custom_font) DeleteObject(w->bstyleptr->font);
		delete w->bstyleptr;
	}
	
	//Free memory (Slider only settings)
	if (w->is_slider){
		if(w->sstyleptr->has_custom_style) delete w->sstyleptr->styleptr;
		if(w->sstyleptr->in_has_custom_style) delete w->sstyleptr->in_styleptr;
		delete w->sstyleptr;
	}
	
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
	Menu *submenu, *custommenu, *advancedmenu;
	int i; bool temp;

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
	custommenu = make_menu("Custom", c );
	for (i = 0; i < STYLE_COLOR_PROPERTY_COUNT; ++i)
	{
		if((w->nosavevalue&(1<<i))==0)
		{
			int colorval;
			switch(i)
			{
				case STYLE_COLOR_INDEX:
					colorval = w->has_custom_style ? w->styleptr->Color : style_get_copy(w->style).Color; 
					break;
				case STYLE_COLORTO_INDEX: 
					colorval = w->has_custom_style ? w->styleptr->ColorTo : style_get_copy(w->style).ColorTo;
					break;
				case STYLE_TEXTCOLOR_INDEX:
					colorval = w->has_custom_style ? w->styleptr->TextColor : style_get_copy(w->style).TextColor; 
					break;
			}
			char color[8];
			sprintf(color,"#%06X",switch_rgb(colorval)); //the bits need to be switched around here
			make_menuitem_str(
				custommenu,
				szStyleProperties[i],
				config_getfull_control_setwindowprop_s(c, szStyleProperties[i]),
				color
				);
		}
	}

	// Add the bevel setting menus as well.
	Menu* bevelmenu = make_menu("Bevel Style", c);
	int beveltype = w->has_custom_style ? w->styleptr->bevelstyle : style_get_copy(w->style).bevelstyle;
	if((w->nosavevalue&(1<<STYLE_BEVELTYPE_INDEX))==0){
		for (i=0; i<STYLE_BEVEL_TYPE_COUNT; ++i)
		{
			bool temp = (beveltype == i);
			make_menuitem_bol(bevelmenu,szBevelTypes[i],config_getfull_control_setwindowprop_c(c,szStyleProperties[STYLE_BEVELTYPE_INDEX],szBevelTypes[i]),temp);
		}
		make_submenu_item(custommenu,"Bevel Style",bevelmenu);
	}
	if (beveltype != 0 && (w->nosavevalue&(1<<STYLE_BEVELPOS_INDEX))==0)
		make_menuitem_int(
			custommenu,
			"Bevel Position",
			config_getfull_control_setwindowprop_s(c, szStyleProperties[STYLE_BEVELPOS_INDEX]),
			w->has_custom_style ? w->styleptr->bevelposition : style_get_copy(w->style).bevelposition,
			1,
			2);
//Border Options -------------------------------------------------------
	Menu *bordermenu = make_menu("Border",c);
	
	temp = !(w->has_custom_style ?
			w->styleptr->bordered : style_get_copy(w->style).bordered);
	make_menuitem_bol(bordermenu,"default",config_getfull_control_setwindowprop_c(c,"DefaultBorder","True"),temp);

	Menu *bordercustommenu = make_menu("Custom",c);

	if((w->nosavevalue&(1<<STYLE_BORDERWIDTH_INDEX))==0){
		int borderwidth = w->has_custom_style ? 
			(w->styleptr->bordered ? w->styleptr->borderWidth : *((int *)GetSettingPtr(SN_BORDERWIDTH))) :
			(style_get_copy(w->style).bordered ? style_get_copy(w->style).borderWidth : *((int *)GetSettingPtr(SN_BORDERWIDTH))); 
		make_menuitem_int(
			bordercustommenu,"Border Width",
			config_getfull_control_setwindowprop_s(c, szStyleProperties[STYLE_BORDERWIDTH_INDEX]),
			borderwidth,0,50);
	}
	if((w->nosavevalue&(1<<STYLE_BORDERCOLOR_INDEX))==0){
		int colorval = w->has_custom_style ? 
			(w->styleptr->bordered ? w->styleptr->borderColor : *((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))) :
			(style_get_copy(w->style).bordered ? style_get_copy(w->style).borderColor : *((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))); 
		char color[8];
		sprintf(color,"#%06X",switch_rgb(colorval));
		make_menuitem_str(
			bordercustommenu,
			"Border Color",
			config_getfull_control_setwindowprop_s(c,szStyleProperties[STYLE_BORDERCOLOR_INDEX]),
			color
		);
	}
	make_submenu_item(bordermenu,"Custom",bordercustommenu);
	make_submenu_item(custommenu,"Border",bordermenu);

//Shadow Options -------------------------------------------------------
	Menu *shadowmenu = make_menu("Text Shadow",c);
	
	Menu *shadowcolormenu = make_menu("Shadow Color",c);
	if((w->nosavevalue&(1<<STYLE_SHADOWCOLOR_INDEX))==0){
		
		temp =	((w->has_custom_style ? w->styleptr->validated  : style_get_copy(w->style).validated) & VALID_SHADOWCOLOR) == 0;
		make_menuitem_bol(shadowcolormenu,"disable",config_getfull_control_setwindowprop_c(c,szStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Disable"),temp);
 
		int colorval = w->has_custom_style ? w->styleptr->ShadowColor : style_get_copy(w->style).ShadowColor;
		temp = !temp && (colorval == CLR_INVALID); 
		make_menuitem_bol(shadowcolormenu,"auto",config_getfull_control_setwindowprop_c(c,szStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Auto"),temp);
		char color[8];
		sprintf(color,"#%06X",switch_rgb(colorval));
		make_menuitem_str(
			shadowcolormenu,
			"Custom Shadow Color",
			config_getfull_control_setwindowprop_s(c,szStyleProperties[STYLE_SHADOWCOLOR_INDEX]),
			color
		);
		make_submenu_item(shadowmenu,"Shadow Color",shadowcolormenu);
	}	
		
	if((w->nosavevalue&(1<<STYLE_SHADOWPOSX_INDEX))==0){
		make_menuitem_int(
			shadowmenu,"Position X",
			config_getfull_control_setwindowprop_s(c, szStyleProperties[STYLE_SHADOWPOSX_INDEX]),
			w->has_custom_style ? w->styleptr->ShadowX : style_get_copy(w->style).ShadowX,
			-100,100);
	}	
	if((w->nosavevalue&(1<<STYLE_SHADOWPOSY_INDEX))==0){
		make_menuitem_int(
			shadowmenu, "Position Y",
			config_getfull_control_setwindowprop_s(c, szStyleProperties[STYLE_SHADOWPOSY_INDEX]),
			w->has_custom_style ? w->styleptr->ShadowY : style_get_copy(w->style).ShadowY,
			-100, 100);
	}
	make_submenu_item(custommenu,"Text Shadow",shadowmenu);
	
// Slider Options ---------------------------------------------------------
	if(w->is_slider){

	//Slider Bar Style Menu------------------------------------------------
		Menu *sliderbarmenu = make_menu("Bar Style",c);
		for (i = 0; i < STYLE_COUNT - 1 ; ++i) // exclude "None"
			make_menuitem_bol(sliderbarmenu, szStyleNames[i], config_getfull_control_setwindowprop_c(c, "SliderBarStyle", szStyleNames[i]), (!w->sstyleptr->has_custom_style) && (w->sstyleptr->style == i) ); 
		Menu *sliderbarcustommenu = make_menu("Custom",c);
		char color[8];
		int colorval;
		for ( i = 0; i < 2; i++){ // Don't show TextColor menu
			if(i == 0){
				colorval = w->sstyleptr->has_custom_style ? w->sstyleptr->styleptr->Color : style_get_copy(w->sstyleptr->style).Color; 	
			}else{
				colorval = w->sstyleptr->has_custom_style ? w->sstyleptr->styleptr->ColorTo : style_get_copy(w->sstyleptr->style).ColorTo;
			}
			sprintf(color,"#%06X", switch_rgb(colorval)); 
			make_menuitem_str(
				sliderbarcustommenu,
				szStyleProperties[i],
				config_getfull_control_setwindowprop_s(c, (i == 0)?"SliderBarColor":"SliderBarColorTo"),
				color);
		}
		
		Menu* sliderbarbevelmenu = make_menu("Bevel Style", c);
		int beveltype = w->sstyleptr->has_custom_style ? w->sstyleptr->styleptr->bevelstyle : style_get_copy(w->sstyleptr->style).bevelstyle;
		for (i=0; i<STYLE_BEVEL_TYPE_COUNT; ++i)
			make_menuitem_bol(sliderbarbevelmenu,szBevelTypes[i],config_getfull_control_setwindowprop_c(c,"SliderBarBevel",szBevelTypes[i]),(beveltype == i));
		make_submenu_item(sliderbarcustommenu,"Bevel Style",sliderbarbevelmenu);
		
		if (beveltype != 0)
			make_menuitem_int(
				sliderbarcustommenu, "Bevel Position",
				config_getfull_control_setwindowprop_s(c, "SliderBarBevelPosition"),
				w->sstyleptr->has_custom_style ? w->sstyleptr->styleptr->bevelposition : style_get_copy(w->sstyleptr->style).bevelposition,
				1, 2);

		make_submenu_item(sliderbarmenu,"Custom",sliderbarcustommenu);
		
	//Slider Inner Style Menu----------------------------------------------
		Menu *sliderinnermenu = make_menu("Slider Inner Style",c);
		bool temp = !w->sstyleptr->draw_inner;
		make_menuitem_bol(sliderinnermenu,"Draw Inner Frame",config_getfull_control_setwindowprop_b(c,"DrawSliderInner",&temp),!temp);
		if(!temp){
			for (i = 0; i < STYLE_COUNT - 1; ++i)
				make_menuitem_bol(sliderinnermenu, szStyleNames[i], config_getfull_control_setwindowprop_c(c, "SliderInnerStyle", szStyleNames[i]), (!w->sstyleptr->in_has_custom_style) && (w->sstyleptr->in_style == i) ); 
			Menu *sliderinnercustommenu = make_menu("Custom",c);
			char color[8];
			int colorval;
			for ( i = 0; i < 2; i++){
				if(i == 0){
					colorval = w->sstyleptr->in_has_custom_style ? w->sstyleptr->in_styleptr->Color : style_get_copy(w->sstyleptr->in_style).Color; 	
				}else{
					colorval = w->sstyleptr->in_has_custom_style ? w->sstyleptr->in_styleptr->ColorTo : style_get_copy(w->sstyleptr->in_style).ColorTo;
				}
				sprintf(color,"#%06X", switch_rgb(colorval)); 
				make_menuitem_str(
					sliderinnercustommenu,
					szStyleProperties[i],
					config_getfull_control_setwindowprop_s(c, (i == 0)?"SliderInnerColor":"SliderInnerColorTo"),
					color);
			}
		
			Menu* sliderinnerbevelmenu = make_menu("Bevel Style", c);
			int beveltype = w->sstyleptr->in_has_custom_style ? w->sstyleptr->in_styleptr->bevelstyle : style_get_copy(w->sstyleptr->in_style).bevelstyle;
			for (i=0; i<STYLE_BEVEL_TYPE_COUNT; ++i)
				make_menuitem_bol(sliderinnerbevelmenu,szBevelTypes[i],config_getfull_control_setwindowprop_c(c,"SliderInnerBevel",szBevelTypes[i]),(beveltype == i));
			make_submenu_item(sliderinnercustommenu,"Bevel Style",sliderinnerbevelmenu);
		
			if (beveltype != 0)
				make_menuitem_int(
					sliderinnercustommenu, "Bevel Position",
					config_getfull_control_setwindowprop_s(c, "SliderInnerBevelPosition"),
					w->sstyleptr->in_has_custom_style ? w->sstyleptr->in_styleptr->bevelposition : style_get_copy(w->sstyleptr->in_style).bevelposition,
					1, 2);

			make_submenu_item(sliderinnermenu,"Custom",sliderinnercustommenu);
		}

		make_menuitem_nop(custommenu,NULL);
		make_submenu_item(custommenu,"Bar Style",sliderbarmenu);
		make_submenu_item(custommenu,"Inner Style",sliderinnermenu);
	}
// End Slider Options ------------------------------------------------------


// Advanced Options --------------------------------------------------------
	advancedmenu = make_menu("Advanced Option",c);
	Menu* advancedmenu2 = make_menu("Base Style",c);
	for (i = 0; i < STYLE_COUNT; ++i)
		make_menuitem_bol(advancedmenu2, szStyleNames[i], config_getfull_control_setwindowprop_c(c, "BaseStyle", szStyleNames[i]), (w->style == i) ); 
	Menu* advancedmenu3 = make_menu("Use Style Default Value",c);
	
	for (i = 0; i < STYLE_PROPERTY_COUNT; ++i){
		temp = ((w->nosavevalue & (1<<i))!=0); 
		make_menuitem_bol(advancedmenu3,szStyleProperties[i],config_getfull_control_setwindowprop_c(c,temp?"UseCustomValue":"UseStyleDefault",szStyleProperties[i]),temp);
	}
	
	make_submenu_item(advancedmenu,"Base Style",advancedmenu2);
	make_submenu_item(advancedmenu,"Use Style Default Value",advancedmenu3);
	make_menuitem_nop(custommenu,NULL);
	make_submenu_item(custommenu,"Advanced",advancedmenu);
	
	
	make_submenu_item(submenu, "Custom", custommenu);
	make_submenu_item(m, "Style", submenu);

// End Style Options -------------------------------------------------------


// Font Setting Menu -------------------------------------------------------
	if(!w->is_slider){  // Slider does not use font
		Menu* fonttopmenu = make_menu("Font",c);
		temp = !w->use_custom_font;
		make_menuitem_bol(fonttopmenu,"Use Custom Font",config_getfull_control_setwindowprop_b(c,"UseCustomFont",&temp),!temp);
		if(w->use_custom_font){
			char fontname[128];
			LOGFONT lf;
			GetObject(style_font,sizeof(lf),&lf);
			strcpy(fontname,lf.lfFaceName);
			if(strlen(w->Fontname)>0){
				strcpy(fontname,w->Fontname);
			}
	
			Menu* fontmenu = make_menu("Font Name", c);
			std::list<std::string>::iterator it = fontList.begin();
			const char *font;
			while(it != fontList.end())
			{
				font = it->c_str();
				make_menuitem_bol(fontmenu,font,config_getfull_control_setwindowprop_c(c,szWPfontname,font),(strcmp(font,fontname)==0));
				it++;
			}
			make_submenu_item(fonttopmenu,"Font",fontmenu);
		
			make_menuitem_int(
				fonttopmenu,
				"Font Size",
				config_getfull_control_setwindowprop_s(c,szWPfontheight),
				w->FontHeight,
				1,
				64);
		
			temp = !(w->FontWeight==FW_BOLD);
			make_menuitem_bol(
				fonttopmenu,
				"Font Bold",
				config_getfull_control_setwindowprop_b(c, szWPfontweight, &temp), 
				!temp 
			);
		}	
		make_submenu_item(m,"Font",fonttopmenu);
	}

	
	make_menuitem_nop(m,NULL);
	
	
// Pressed Button Style Menu -----------------------------------------------
	if(w->is_button){
		int stylenum = (w->bstyleptr->style==STYLETYPE_DEFAULT)?get_pressedstyle_index(w->style):w->bstyleptr->style;
		Menu *buttonsubmenu, *buttoncustommenu, *advancedbuttonmenu; 
	
		//Style options
		buttonsubmenu = make_menu("StyleWhenPressed", c);
	
		//First , Default Style
		make_menuitem_bol(buttonsubmenu, szPressedStyleNames[STYLETYPE_DEFAULT], config_getfull_control_setwindowprop_c(c, szWPStyleWhenPressed, szPressedStyleNames[STYLETYPE_DEFAULT]), (!w->bstyleptr->has_custom_style) && (w->bstyleptr->style == STYLETYPE_DEFAULT) ); 
		for (i = 0; i < PRESSED_STYLE_COUNT - 1 ; ++i)
			make_menuitem_bol(buttonsubmenu, szPressedStyleNames[i], config_getfull_control_setwindowprop_c(c, szWPStyleWhenPressed, szPressedStyleNames[i]), (!w->bstyleptr->has_custom_style) && (w->bstyleptr->style == i) ); 

		buttoncustommenu = make_menu("Custom", c );
		for (i = 0; i < STYLE_COLOR_PROPERTY_COUNT; ++i)
		{
			if((w->bstyleptr->nosavevalue&(1<<i))==0)
			{
				int colorval;
				switch(i)
				{
					case STYLE_COLOR_INDEX:
						colorval = w->bstyleptr->has_custom_style ?
							w->bstyleptr->styleptr->Color : 
							style_get_copy(stylenum).Color; 
						break;
					case STYLE_COLORTO_INDEX: 
						colorval = w->bstyleptr->has_custom_style ? 
							w->bstyleptr->styleptr->ColorTo : 
							style_get_copy(stylenum).ColorTo; 
						break;
					case STYLE_TEXTCOLOR_INDEX:
						colorval = w->bstyleptr->has_custom_style ? 
							w->bstyleptr->styleptr->TextColor : 
							style_get_copy(stylenum).TextColor; 
						break;
;
				}
				char color[8];
				sprintf(color,"#%06X",switch_rgb(colorval)); //the bits need to be switched around here
				
				make_menuitem_str(
					buttoncustommenu,
					szStyleProperties[i],
					config_getfull_control_setwindowprop_s(c, szPressedStyleProperties[i]),
					color
					);
			}
		}
	
		// Add the bevel setting menus as well.
		Menu* buttonbevelmenu = make_menu("Bevel Style", c);
		int beveltype = w->bstyleptr->has_custom_style ? w->bstyleptr->styleptr->bevelstyle : style_get_copy(stylenum).bevelstyle;
		if((w->bstyleptr->nosavevalue&(1<<STYLE_BEVELTYPE_INDEX))==0){
			for (int i=0; i<STYLE_BEVEL_TYPE_COUNT; ++i)
			{
				bool temp = (beveltype == i);
				make_menuitem_bol(buttonbevelmenu,szBevelTypes[i],config_getfull_control_setwindowprop_c(c,szPressedStyleProperties[STYLE_BEVELTYPE_INDEX],szBevelTypes[i]),temp);
			}
			make_submenu_item(buttoncustommenu,"Bevel Style",buttonbevelmenu);
		}
		if (beveltype != 0 && (w->bstyleptr->nosavevalue&(1<<STYLE_BEVELPOS_INDEX))==0)
			make_menuitem_int(
				buttoncustommenu,
				"Bevel Position",
				config_getfull_control_setwindowprop_s(c, szPressedStyleProperties[STYLE_BEVELPOS_INDEX]),
				w->bstyleptr->has_custom_style ? w->bstyleptr->styleptr->bevelposition : style_get_copy(stylenum).bevelposition,
				1,
				2);


//Border Options (Pressed)------------------------------------------------
		Menu *bordermenu = make_menu("Border",c);
	
		temp = !(w->bstyleptr->has_custom_style ?
				w->bstyleptr->styleptr->bordered : style_get_copy(stylenum).bordered);
		make_menuitem_bol(bordermenu,"default",config_getfull_control_setwindowprop_c(c,"DefaultBorderPressed","True"),temp);
	
		Menu *bordercustommenu = make_menu("Custom",c);
	
		if((w->bstyleptr->nosavevalue&(1<<STYLE_BORDERWIDTH_INDEX))==0){
			int borderwidth = 
					w->bstyleptr->has_custom_style ? 
						(
						 w->bstyleptr->styleptr->bordered ? 
						 	w->bstyleptr->styleptr->borderWidth :
							*((int *)GetSettingPtr(SN_BORDERWIDTH))
						) :
						(
						 style_get_copy(stylenum).bordered ? 
						 	style_get_copy(stylenum).borderWidth : 
							*((int *)GetSettingPtr(SN_BORDERWIDTH))
						); 
			make_menuitem_int(
				bordercustommenu,"Border Width",
				config_getfull_control_setwindowprop_s(c, szPressedStyleProperties[STYLE_BORDERWIDTH_INDEX]),
				borderwidth,0,50);
		}
		if((w->nosavevalue&(1<<STYLE_BORDERCOLOR_INDEX))==0){
			int colorval = 
					w->bstyleptr->has_custom_style ? 
						(
						 w->bstyleptr->styleptr->bordered ? 
						 	w->bstyleptr->styleptr->borderColor : 
							*((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))
						) :
						(
						 style_get_copy(stylenum).bordered ? 
						 	style_get_copy(stylenum).borderColor : 
							*((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))
						); 
			char color[8];
			sprintf(color,"#%06X",switch_rgb(colorval));
			make_menuitem_str(
				bordercustommenu,
				"Border Color",
				config_getfull_control_setwindowprop_s(c,szPressedStyleProperties[STYLE_BORDERCOLOR_INDEX]),
				color
			);
		}
		make_submenu_item(bordermenu,"Custom",bordercustommenu);
		make_submenu_item(buttoncustommenu,"Border",bordermenu);

//Shadow Options (Pressed)-----------------------------------------
		Menu *shadowmenu = make_menu("Text Shadow",c);
		Menu *shadowcolormenu = make_menu("Shadow Color",c);
		if((w->bstyleptr->nosavevalue&(1<<STYLE_SHADOWCOLOR_INDEX))==0){
			
			temp =	((w->bstyleptr->has_custom_style ? w->bstyleptr->styleptr->validated  : style_get_copy(stylenum).validated) & VALID_SHADOWCOLOR) == 0;
			make_menuitem_bol(shadowcolormenu,"disable",config_getfull_control_setwindowprop_c(c,szPressedStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Disable"),temp);
 
			int colorval = w->bstyleptr->has_custom_style ? w->bstyleptr->styleptr->ShadowColor : style_get_copy(stylenum).ShadowColor;
			temp = !temp && (colorval == CLR_INVALID); 
			make_menuitem_bol(shadowcolormenu,"auto",config_getfull_control_setwindowprop_c(c,szPressedStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Auto"),temp);
			char color[8];
			sprintf(color,"#%06X",switch_rgb(colorval));
			make_menuitem_str(
				shadowcolormenu,
				"Custom Shadow Color",
				config_getfull_control_setwindowprop_s(c,szPressedStyleProperties[STYLE_SHADOWCOLOR_INDEX]),
				color
			);
			make_submenu_item(shadowmenu,"Shadow Color",shadowcolormenu);
		}	
			
		if((w->bstyleptr->nosavevalue&(1<<STYLE_SHADOWPOSX_INDEX))==0){
			make_menuitem_int(
				shadowmenu,"Position X",
				config_getfull_control_setwindowprop_s(c, szPressedStyleProperties[STYLE_SHADOWPOSX_INDEX]),
				w->bstyleptr->has_custom_style ? w->bstyleptr->styleptr->ShadowX : style_get_copy(stylenum).ShadowX,
				-100,100);
		}	
		if((w->bstyleptr->nosavevalue&(1<<STYLE_SHADOWPOSY_INDEX))==0){
			make_menuitem_int(
				shadowmenu, "Position Y",
				config_getfull_control_setwindowprop_s(c, szPressedStyleProperties[STYLE_SHADOWPOSY_INDEX]),
				w->bstyleptr->has_custom_style ? w->bstyleptr->styleptr->ShadowY : style_get_copy(stylenum).ShadowY,
				-100, 100);
		}
		make_submenu_item(buttoncustommenu,"Text Shadow",shadowmenu);
	

		
	
		// Advanced Option (Pressed Button)
		advancedbuttonmenu = make_menu("Advanced Option",c);
		Menu* advancedbuttonmenu2 = make_menu("Base Style",c);
		
		make_menuitem_bol(advancedbuttonmenu2, szPressedStyleNames[STYLETYPE_DEFAULT], config_getfull_control_setwindowprop_c(c, "BaseStyle(Pressed)", szPressedStyleNames[STYLETYPE_DEFAULT]), (w->bstyleptr->style == STYLETYPE_DEFAULT) ); 
		for (i = 0; i < PRESSED_STYLE_COUNT - 1 ; ++i)
			make_menuitem_bol(advancedbuttonmenu2, szPressedStyleNames[i], config_getfull_control_setwindowprop_c(c, "BaseStyle(Pressed)", szPressedStyleNames[i]), (w->bstyleptr->style == i) ); 
		
		Menu* advancedbuttonmenu3 = make_menu("Use Style Default Value",c);
		
		for (i = 0; i < STYLE_PROPERTY_COUNT; ++i){
			temp = ((w->bstyleptr->nosavevalue & (1<<i))!=0); 
			make_menuitem_bol(advancedbuttonmenu3,szStyleProperties[i],config_getfull_control_setwindowprop_c(c,temp?"UseCustomValue(Pressed)":"UseStyleDefault(Pressed)",szStyleProperties[i]),temp);
		}
	
		make_submenu_item(advancedbuttonmenu,"Base Style",advancedbuttonmenu2);
		make_submenu_item(advancedbuttonmenu,"Use Style Default Value",advancedbuttonmenu3);
		make_menuitem_nop(buttoncustommenu,NULL);
		make_submenu_item(buttoncustommenu,"Advanced",advancedbuttonmenu);
			
		make_submenu_item(buttonsubmenu, "Custom", buttoncustommenu);
		make_submenu_item(m, "StyleWhenPressed", buttonsubmenu);

		Menu* pressedfonttopmenu = make_menu("FontWhenPressed",c);
		
		temp = !w->bstyleptr->use_custom_font;
		make_menuitem_bol(pressedfonttopmenu,"Use Custom Font",config_getfull_control_setwindowprop_b(c,"UseCustomFontWhenPressed",&temp),!temp);
		if(w->bstyleptr->use_custom_font){
		
			char fontname[128];
			LOGFONT lf;
			GetObject(style_font,sizeof(lf),&lf);
			strcpy(fontname,lf.lfFaceName);
			if(strlen(w->bstyleptr->Fontname)>0){
				strcpy(fontname,w->bstyleptr->Fontname);
			}
			Menu* buttonfontmenu = make_menu("Font Name", c);
			std::list<std::string>::iterator it = fontList.begin();
			const char *font;
			while(it != fontList.end())
			{
				font = it->c_str();
				make_menuitem_bol(buttonfontmenu,font,config_getfull_control_setwindowprop_c(c,szWPfontname_pressed,font),(strcmp(font,fontname)==0));
				it++;
				
			}
			make_submenu_item(pressedfonttopmenu,"Font",buttonfontmenu);
		
			make_menuitem_int(
				pressedfonttopmenu,
				"Font Size",
				config_getfull_control_setwindowprop_s(c,szWPfontheight_pressed),
				w->bstyleptr->FontHeight,
				1,
				64);
		
			temp = !(w->bstyleptr->FontWeight==FW_BOLD);
			make_menuitem_bol(
				pressedfonttopmenu,
				"Font Bold",
				config_getfull_control_setwindowprop_b(c, szWPfontweight_pressed, &temp), 
				!temp 
			);
		}
		make_submenu_item(m,"FontWhenPressed",pressedfonttopmenu);
		make_menuitem_nop(m,NULL);
	
	}

	//---------------end button pressed option
	
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
		temp = !w->is_detectfullscreen; make_menuitem_bol(m, "Detect Fullscreen App", config_getfull_control_setwindowprop_b(c, szWPisdetectfullscreen, &temp), !temp);
		temp = !w->is_toggledwithplugins; make_menuitem_bol(m, "Toggle With Plugins", config_getfull_control_setwindowprop_b(c, szWPistoggledwithplugins, &temp), !temp);
		temp = !w->is_onallworkspaces; make_menuitem_bol(m, "On All Workspaces", config_getfull_control_setwindowprop_b(c, szWPisonallworkspaces, &temp), !temp);

		if(!w->is_onallworkspaces){
			DesktopInfo di;
			GetDesktopInfo(&di);
			make_menuitem_int(m, "Workspace", config_getfull_control_setwindowprop_s(c, szWPworkspacenumber), w->workspacenumber, 0, di.ScreensX - 1);

		}
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
//
//save configuration to the setting file
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
	config_write(config_get_control_setwindowprop_c(c, szWPStyle, szStyleNames[w->style])); //The redundance in style specification allows for a possible "Disable custom styles" option, which could come in handy for some.


// save custom style settings
	if (w->has_custom_style)
	{
		for (int i = 0; i < STYLE_COLOR_PROPERTY_COUNT; ++i) //This part would be a likely candidate to be exported to another function.
		{
			int colorval;
			switch(i)
			{
				case STYLE_COLOR_INDEX: colorval = w->styleptr->Color; break;
				case STYLE_COLORTO_INDEX: colorval = w->styleptr->ColorTo; break;
				case STYLE_TEXTCOLOR_INDEX: colorval = w->styleptr->TextColor; break;
			}
			char color[8];
			sprintf(color,"#%06X",switch_rgb(colorval)); //the bits need to be switched around here
			if(((w->nosavevalue & (1<<i)) == 0)) // Don't save flag check
				config_write(config_get_control_setwindowprop_c(c, szStyleProperties[i], color));
		}
		if((w->nosavevalue & (1<<STYLE_BEVELTYPE_INDEX)) == 0)
			config_write(config_get_control_setwindowprop_c(c,szStyleProperties[STYLE_BEVELTYPE_INDEX],szBevelTypes[w->styleptr->bevelstyle]));
		if((w->styleptr->bevelstyle != 0) &&((w->nosavevalue & (1<<STYLE_BEVELPOS_INDEX)) == 0))
			config_write(config_get_control_setwindowprop_i(c,szStyleProperties[STYLE_BEVELPOS_INDEX],&w->styleptr->bevelposition));


		if(w->styleptr->bordered){ // custom border settings
			if((w->nosavevalue & (1<<STYLE_BORDERWIDTH_INDEX)) == 0){
				int val = (int)w->styleptr->borderWidth;
				config_write(config_get_control_setwindowprop_i(c,szStyleProperties[STYLE_BORDERWIDTH_INDEX],&val));
				
			}
			if((w->nosavevalue & (1<<STYLE_BORDERCOLOR_INDEX)) == 0){
				char color[8];
				sprintf(color,"#%06X",switch_rgb(w->styleptr->borderColor)); 
				config_write(config_get_control_setwindowprop_c(c, szStyleProperties[STYLE_BORDERCOLOR_INDEX], color));
			}
		}

		if((w->nosavevalue & (1<<STYLE_SHADOWCOLOR_INDEX)) == 0){
			if((w->styleptr->validated & VALID_SHADOWCOLOR) == 0)
				config_write(config_get_control_setwindowprop_c(c, szStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Disable"));
			else if(w->styleptr->ShadowColor == CLR_INVALID)
				config_write(config_get_control_setwindowprop_c(c, szStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Auto"));
			else{
				char color[8];
				sprintf(color,"#%06X",switch_rgb(w->styleptr->ShadowColor)); 
				config_write(config_get_control_setwindowprop_c(c, szStyleProperties[STYLE_SHADOWCOLOR_INDEX], color));
			}
		}
		if((w->nosavevalue & (1<<STYLE_SHADOWPOSX_INDEX)) == 0){
			int val = (int)w->styleptr->ShadowX;
			config_write(config_get_control_setwindowprop_i(c,szStyleProperties[STYLE_SHADOWPOSX_INDEX],&val));
		}
		if((w->nosavevalue & (1<<STYLE_SHADOWPOSY_INDEX)) == 0){
			int val = (int)w->styleptr->ShadowY;
			config_write(config_get_control_setwindowprop_i(c,szStyleProperties[STYLE_SHADOWPOSY_INDEX],&val));
		}

// slider only config settings ---------------------------------------------
		if (w->is_slider)
		{
			config_write(config_get_control_setwindowprop_c(c, "SliderBarStyle", szStyleNames[w->sstyleptr->style])); 
			if (w->sstyleptr->has_custom_style){
				for (int i = 0; i < STYLE_COLOR_PROPERTY_COUNT - 1; ++i) 
				{
					int colorval = (i==0)?w->sstyleptr->styleptr->Color: w->sstyleptr->styleptr->ColorTo;
					char color[8];
					sprintf(color,"#%06X",switch_rgb(colorval)); 
					config_write(config_get_control_setwindowprop_c(c, (i==0)?"SliderBarColor":"SliderBarColorTo", color));
				}
				int bevelstyle = w->sstyleptr->styleptr->bevelstyle;
				config_write(config_get_control_setwindowprop_c(c,"SliderBarBevel",szBevelTypes[bevelstyle]));
				if((bevelstyle != 0))
					config_write(config_get_control_setwindowprop_i(c,"SliderBarBevelPosition",&w->sstyleptr->styleptr->bevelposition));
			}
			config_write(config_get_control_setwindowprop_b(c, "DrawSliderInner", &w->sstyleptr->draw_inner));
			if(w->sstyleptr->draw_inner){
				config_write(config_get_control_setwindowprop_c(c, "SliderInnerStyle", szStyleNames[w->sstyleptr->in_style])); 
				if (w->sstyleptr->in_has_custom_style){
					for (int i = 0; i < STYLE_COLOR_PROPERTY_COUNT - 1; ++i) 
					{
						int colorval = (i==0)?w->sstyleptr->in_styleptr->Color: w->sstyleptr->in_styleptr->ColorTo;
						char color[8];
						sprintf(color,"#%06X",switch_rgb(colorval)); 
						config_write(config_get_control_setwindowprop_c(c, (i==0)?"SliderInnerColor":"SliderInnerColorTo", color));
					}
					int bevelstyle = w->sstyleptr->in_styleptr->bevelstyle;
					config_write(config_get_control_setwindowprop_c(c,"SliderInnerBevel",szBevelTypes[bevelstyle]));
					if((bevelstyle != 0))
						config_write(config_get_control_setwindowprop_i(c,"SliderInnerBevelPosition",&w->sstyleptr->in_styleptr->bevelposition));
				}
			

			}
		}
	}	
	if(w->use_custom_font){	
		char fontname[128];
		if(strlen(w->Fontname)){
			strcpy(fontname,w->Fontname);
		}else{
			LOGFONT lf;
			::GetObject(style_font,sizeof(lf),&lf);
			strcpy(fontname,lf.lfFaceName);
		}
		config_write(config_get_control_setwindowprop_c(c,szWPfontname,fontname));
		config_write(config_get_control_setwindowprop_i(c,szWPfontheight,&w->FontHeight));
		
		bool temp = (w->FontWeight==FW_BOLD);
		config_write(config_get_control_setwindowprop_b(c,szWPfontweight,&temp));
	}
	for(int j=w->nosavevalue , i=0; j > 0  ; i++ , j>>=1 ){
		if(j&1){
			config_write(config_get_control_setwindowprop_c(c,"UseStyleDefault",szStyleProperties[i]));
		}
	}
	
	
	//button only config settings
	if (w->is_button)
	{
		if(w->bstyleptr->style != STYLETYPE_DEFAULT)
			config_write(config_get_control_setwindowprop_c(c, szWPStyleWhenPressed, szPressedStyleNames[w->bstyleptr->style]));
		if(w->bstyleptr->has_custom_style)
		{	
			for (int i = 0; i < STYLE_COLOR_PROPERTY_COUNT; ++i)
			{
				int colorval;
				switch(i)
				{
					case STYLE_COLOR_INDEX: colorval = w->bstyleptr->styleptr->Color; break;
					case STYLE_COLORTO_INDEX: colorval = w->bstyleptr->styleptr->ColorTo; break;
					case STYLE_TEXTCOLOR_INDEX: colorval = w->bstyleptr->styleptr->TextColor; break;
				}
				char color[8];
				sprintf(color,"#%06X",switch_rgb(colorval)); //the bits need to be switched around here
				if(((w->bstyleptr->nosavevalue & (1<<i)) == 0)) // Don't save flag check
					config_write(config_get_control_setwindowprop_c(c, szPressedStyleProperties[i], color));
			}
			if((w->bstyleptr->nosavevalue & (1<<STYLE_BEVELTYPE_INDEX)) == 0)
				config_write(config_get_control_setwindowprop_c(c,szPressedStyleProperties[STYLE_BEVELTYPE_INDEX],szBevelTypes[w->bstyleptr->styleptr->bevelstyle]));
			if((w->bstyleptr->styleptr->bevelstyle != 0) &&((w->bstyleptr->nosavevalue & (1<<STYLE_BEVELPOS_INDEX)) == 0))
				config_write(config_get_control_setwindowprop_i(c,szPressedStyleProperties[STYLE_BEVELPOS_INDEX],&w->bstyleptr->styleptr->bevelposition));
			if(w->bstyleptr->styleptr->bordered){ // custom border settings
				if((w->bstyleptr->nosavevalue & (1<<STYLE_BORDERWIDTH_INDEX)) == 0){
					int val = (int)w->bstyleptr->styleptr->borderWidth;
					config_write(config_get_control_setwindowprop_i(c,szPressedStyleProperties[STYLE_BORDERWIDTH_INDEX],&val));
				}
				if((w->bstyleptr->nosavevalue & (1<<STYLE_BORDERCOLOR_INDEX)) == 0){
					char color[8];
					sprintf(color,"#%06X",switch_rgb(w->bstyleptr->styleptr->borderColor)); 
					config_write(config_get_control_setwindowprop_c(c, szPressedStyleProperties[STYLE_BORDERCOLOR_INDEX], color));
				}
			}



			if((w->bstyleptr->nosavevalue & (1<<STYLE_SHADOWCOLOR_INDEX)) == 0){
				if((w->bstyleptr->styleptr->validated & VALID_SHADOWCOLOR) == 0)
					config_write(config_get_control_setwindowprop_c(c, szPressedStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Disable"));
				else if(w->bstyleptr->styleptr->ShadowColor == CLR_INVALID)
					config_write(config_get_control_setwindowprop_c(c, szPressedStyleProperties[STYLE_SHADOWCOLOR_INDEX],"Auto"));
				else{
					char color[8];
					sprintf(color,"#%06X",switch_rgb(w->bstyleptr->styleptr->ShadowColor)); 
					config_write(config_get_control_setwindowprop_c(c, szPressedStyleProperties[STYLE_SHADOWCOLOR_INDEX], color));
				}
			}
			if((w->bstyleptr->nosavevalue & (1<<STYLE_SHADOWPOSX_INDEX)) == 0){
				int val = (int)w->bstyleptr->styleptr->ShadowX;
				config_write(config_get_control_setwindowprop_i(c,szPressedStyleProperties[STYLE_SHADOWPOSX_INDEX],&val));
			}
			if((w->bstyleptr->nosavevalue & (1<<STYLE_SHADOWPOSY_INDEX)) == 0){
				int val = (int)w->bstyleptr->styleptr->ShadowY;
				config_write(config_get_control_setwindowprop_i(c,szPressedStyleProperties[STYLE_SHADOWPOSY_INDEX],&val));
			}
	
		}
		if (w->bstyleptr->use_custom_font){
		
			char fontname[128];
			if(strlen(w->bstyleptr->Fontname)){
				strcpy(fontname,w->bstyleptr->Fontname);
			}else{
				LOGFONT lf;
				::GetObject(style_font,sizeof(lf),&lf);
				strcpy(fontname,lf.lfFaceName);
			}
			config_write(config_get_control_setwindowprop_c(c,szWPfontname_pressed,fontname));
			config_write(config_get_control_setwindowprop_i(c,szWPfontheight_pressed,&w->bstyleptr->FontHeight));
			
			bool temp = (w->bstyleptr->FontWeight==FW_BOLD);
			config_write(config_get_control_setwindowprop_b(c,szWPfontweight_pressed,&temp));
		}
		for(int j=w->bstyleptr->nosavevalue , i=0; j > 0  ; i++ , j>>=1 ){
			if(j&1){
				config_write(config_get_control_setwindowprop_c(c,"UseStyleDefault",szStyleProperties[i]));
			}
		}
	}
	
	if (!c->parentptr)
	{
		config_write(config_get_control_setwindowprop_b(c, szWPistoggledwithplugins, &w->is_toggledwithplugins));
		config_write(config_get_control_setwindowprop_b(c, szWPisonallworkspaces, &w->is_onallworkspaces));
		if(!w->is_onallworkspaces)
			config_write(config_get_control_setwindowprop_i(c, szWPworkspacenumber, &w->workspacenumber));
		config_write(config_get_control_setwindowprop_b(c, szWPisdetectfullscreen, &w->is_detectfullscreen));
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
		case BB_TASKSUPDATE:
			if(lParam == 0){
				window_update(w, false, false, true, false);
			}
			break;
			
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
	bool needs_fontupdate = false;
	bool needs_pressedfontupdate = false;
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
		if(w->has_custom_style) { 
			delete w->styleptr; 
			w->styleptr = 0; 
			w->has_custom_style = false; 
			w->nosavevalue = 0;
		} //get rid of previous custom style
			
		w->style = i; style_draw_invalidate(c); needs_transupdate = true;
	}
	else
	if (!stricmp(tokens[4], "BaseStyle") && -1 != (i = get_string_index(tokens[5], szStyleNames)))
	{
		if(!w->has_custom_style){
			w->has_custom_style = true;
			w->styleptr = new StyleItem; 
			*w->styleptr = style_get_copy(w->style);
			if(w->is_slider)	
				w->sstyleptr->draw_inner = (w->style == STYLETYPE_TOOLBAR);
		}
		w->style = i;
		style_set_customvalue(w->styleptr,i,w->nosavevalue);
		style_draw_invalidate(c);
		needs_transupdate = true;
	}
	else
	if (!stricmp(tokens[4], "UseCustomValue") && -1 != (i = get_string_index(tokens[5],szStyleProperties)))
	{
		w->nosavevalue &= ~(1<<i);
		if(!w->has_custom_style){
			w->has_custom_style = true;
			w->styleptr = new StyleItem; 
			*w->styleptr = style_get_copy(w->style);
			if(w->is_slider)	
				w->sstyleptr->draw_inner = (w->style == STYLETYPE_TOOLBAR);
		}
		style_draw_invalidate(c);
		needs_transupdate = true;
	}
	else
	if (!stricmp(tokens[4], "UseStyleDefault") && -1 != (i = get_string_index(tokens[5],szStyleProperties)))
	{
		w->nosavevalue |= (1<<i);
		if(!w->has_custom_style){
			w->has_custom_style = true;
			w->styleptr = new StyleItem; 
			*w->styleptr = style_get_copy(w->style);
			if(w->is_slider)	
				w->sstyleptr->draw_inner = (w->style == STYLETYPE_TOOLBAR);
		}
		StyleItem *tmpstyle;
		tmpstyle = new StyleItem;
		*tmpstyle = style_get_copy(w->style);
		switch(i){
			case STYLE_COLOR_INDEX:
				w->styleptr->Color = tmpstyle->Color;
				break;
			case STYLE_COLORTO_INDEX:
				w->styleptr->ColorTo = tmpstyle->ColorTo;
				break;
			case STYLE_TEXTCOLOR_INDEX:
				w->styleptr->TextColor = tmpstyle->TextColor;
				break;
			case STYLE_BEVELTYPE_INDEX:
				w->styleptr->bevelstyle = tmpstyle->bevelstyle;
				break;
			case STYLE_BEVELPOS_INDEX:
				w->styleptr->bevelposition = tmpstyle->bevelposition;
				break;
			case STYLE_BORDERWIDTH_INDEX:
				w->styleptr->borderWidth =
				(tmpstyle->bordered ? tmpstyle->borderWidth : *((int *)GetSettingPtr(SN_BORDERWIDTH)));
				if(w->nosavevalue & 1<<STYLE_BORDERCOLOR_INDEX){
					w->styleptr->bordered = false;
				}
				break;
			case STYLE_BORDERCOLOR_INDEX:
				w->styleptr->borderColor =
				(tmpstyle->bordered ? tmpstyle->borderColor : *((COLORREF *)GetSettingPtr(SN_BORDERCOLOR)));
				if(w->nosavevalue & 1<<STYLE_BORDERWIDTH_INDEX){
					w->styleptr->bordered = false;
				}
				break;
			case STYLE_SHADOWCOLOR_INDEX:
				w->styleptr->ShadowColor = tmpstyle->ShadowColor;
				w->styleptr->validated &= ~VALID_SHADOWCOLOR;
				w->styleptr->validated |= (tmpstyle->validated & VALID_SHADOWCOLOR);
				break;
			case STYLE_SHADOWPOSX_INDEX:
				w->styleptr->ShadowX = tmpstyle->ShadowX;
				break;
			case STYLE_SHADOWPOSY_INDEX:
				w->styleptr->ShadowY = tmpstyle->ShadowY;
				break;
		}
		delete tmpstyle;	
		style_draw_invalidate(c);
		needs_transupdate = true;
	}
	else
	if (!stricmp(tokens[4],"UseCustomFont") && config_set_bool(tokens[5], &w->use_custom_font))
	{
		if(!w->use_custom_font){
			DeleteObject(w->font);
			w->font = NULL;
		}
		style_draw_invalidate(c);
		needs_transupdate = true;
		needs_fontupdate = true;
	}
	else
	if (!stricmp(tokens[4],szWPfontname))
	{
		w->use_custom_font = true;
		strcpy(w->Fontname,tokens[5]);
		style_draw_invalidate(c); needs_transupdate = true; 
		needs_fontupdate = true;
	}
	else
	if (!stricmp(tokens[4],szWPfontheight))
	{
		w->use_custom_font = true;
		w->FontHeight=atoi(tokens[5]);
		style_draw_invalidate(c); needs_transupdate = true; 
		needs_fontupdate = true;
	}
	else
	if(!stricmp(tokens[4],szWPfontweight))
	{
		w->use_custom_font = true;
		bool temp;
		config_set_bool(tokens[5],&temp);
		w->FontWeight=temp?FW_BOLD:FW_NORMAL;
		style_draw_invalidate(c); needs_transupdate = true; 
		needs_fontupdate = true;
	}
	else
	if (-1 != (i = get_string_index(tokens[4], szStyleProperties)) )
	{
		if (!w->has_custom_style) {
			w->has_custom_style = true; w->styleptr = new StyleItem; *w->styleptr = style_get_copy(w->style);
			if(w->is_slider)	
				w->sstyleptr->draw_inner = (w->style == STYLETYPE_TOOLBAR);
		}
		COLORREF colorval;
		int beveltype,shadowpos;
		bool shadow_set = true;
		if ( (i<STYLE_COLOR_PROPERTY_COUNT) && -1 == ( colorval = ReadColorFromString(tokens[5])) ) errors = 1;
		if ( (i==STYLE_BEVELTYPE_INDEX) && -1 == ( beveltype = get_string_index(tokens[5], szBevelTypes)) ) errors = 1;
		if ( (i==STYLE_BEVELPOS_INDEX) && !config_set_int(tokens[5], &(w->styleptr->bevelposition), 1, 2)) errors = 1;
		if (i== STYLE_SHADOWCOLOR_INDEX){
			//Shadow color
			if(!stricmp(tokens[5],"Auto")){
				colorval = CLR_INVALID;
			}
			else if(!stricmp(tokens[5],"Disable")){
				shadow_set = false;
			}
			else if((colorval = ReadColorFromString(tokens[5]))==-1)
				errors = 1;
			
		}
		if ( (i==STYLE_BORDERWIDTH_INDEX) && !config_set_int(tokens[5], &(w->styleptr->borderWidth), 0, 50)) errors = 1;
		if ( (i==STYLE_BORDERCOLOR_INDEX) && -1 == ( colorval = ReadColorFromString(tokens[5])) ) errors = 1;

		if ( (i==STYLE_SHADOWPOSX_INDEX || i==STYLE_SHADOWPOSY_INDEX) && !config_set_int(tokens[5],&shadowpos,-100,100)) errors = 1;
		if (!errors)
		{	switch (i)
			{
				case STYLE_COLOR_INDEX:
					w->styleptr->Color = colorval; break;
				case STYLE_COLORTO_INDEX:
					w->styleptr->ColorTo = colorval; break;
				case STYLE_TEXTCOLOR_INDEX:
					w->styleptr->TextColor = colorval; break;
				case STYLE_BEVELTYPE_INDEX: w->styleptr->bevelstyle = beveltype;
					if (beveltype != 0 && w->styleptr->bevelposition == 0)
						w->styleptr->bevelposition = 1;
					break;
				case STYLE_BORDERWIDTH_INDEX:
					if(!w->styleptr->bordered){
						StyleItem *tmpstyle;
						tmpstyle = new StyleItem;
						*tmpstyle = style_get_copy(w->style);

						w->styleptr->bordered = true;
						w->styleptr->borderColor =
							(tmpstyle->bordered ?
							 	tmpstyle->borderColor : 
								*((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))
							);
						delete tmpstyle;	
					}
					break;
				case STYLE_BORDERCOLOR_INDEX:
					w->styleptr->borderColor = colorval; 
					if(!w->styleptr->bordered){
						StyleItem *tmpstyle;
						tmpstyle = new StyleItem;
						*tmpstyle = style_get_copy(w->style);
	
						w->styleptr->bordered = true;
						w->styleptr->borderWidth =
							(tmpstyle->bordered ? 
							 	tmpstyle->borderWidth : 
								*((int *)GetSettingPtr(SN_BORDERWIDTH))
							);
						delete tmpstyle;	
					}
					break;
				case STYLE_SHADOWCOLOR_INDEX:
					if(shadow_set){
						w->styleptr->validated |= VALID_SHADOWCOLOR; // set bit
						w->styleptr->ShadowColor = colorval;
					}else{
						w->styleptr->validated &= ~VALID_SHADOWCOLOR; // unset
					}
					break;
				case STYLE_SHADOWPOSX_INDEX:
					w->styleptr->ShadowX = (char)shadowpos;
					break;
				case STYLE_SHADOWPOSY_INDEX:
					w->styleptr->ShadowY = (char)shadowpos;
					break;

			}

			style_draw_invalidate(c); needs_transupdate = true; 
		}
	}
	else
	if (!stricmp(tokens[4], "DefaultBorder")){
		StyleItem *tmpstyle;
		tmpstyle = new StyleItem;
		*tmpstyle = style_get_copy(w->style);

		w->styleptr->borderWidth =
			(tmpstyle->bordered ? 
			 	tmpstyle->borderWidth : 
				*((int *)GetSettingPtr(SN_BORDERWIDTH))
			);
		w->styleptr->borderColor =
			(tmpstyle->bordered ?
			 	tmpstyle->borderColor : 
				*((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))
			);
		w->styleptr->bordered = false;

		delete tmpstyle;	
		style_draw_invalidate(c);
		needs_transupdate = true;
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
			{
				needs_stickyupdate = true;
				DesktopInfo di;
				GetDesktopInfo(&di);
				w->workspacenumber = di.number ;
			}
		else
		if (!stricmp(tokens[4], szWPworkspacenumber))
			{
				DesktopInfo di;
				GetDesktopInfo(&di);
				if (config_set_int(tokens[5], &w->workspacenumber, 0, di.ScreensX - 1 )){
					needs_stickyupdate = true;
				}

			}
		else
		if (!stricmp(tokens[4], szWPisdetectfullscreen) && config_set_bool(tokens[5], &w->is_detectfullscreen))
			{ 
				needs_posupdate = true; 
			}
		else
		if (!stricmp(tokens[4], szWPisontop) && config_set_bool(tokens[5], &w->is_ontop))
			{ 
				needs_posupdate = true; 
			}
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

	// Slider Option Settings
	if (errors && w->is_slider){
		errors = 0;
		if (!stricmp(tokens[4], "SliderBarStyle") && -1 != (i = get_string_index(tokens[5], szStyleNames)))
		{
			if (!w->has_custom_style) {
				w->has_custom_style = true; w->styleptr = new StyleItem; *w->styleptr = style_get_copy(w->style);
				w->sstyleptr->draw_inner = (w->style == STYLETYPE_TOOLBAR);
			}
				
			if(w->sstyleptr->has_custom_style) { 
				delete w->sstyleptr->styleptr; 
				w->sstyleptr->styleptr = 0; 
				w->sstyleptr->has_custom_style = false; 
			} 
			w->sstyleptr->style = i; style_draw_invalidate(c); needs_transupdate = true;
		}
		else
		if (!strncmp(tokens[4],"SliderBar",9)  && -1 != (i = get_string_index(&tokens[4][9], szStyleProperties)) )
		{
			if (!w->has_custom_style) {
				w->has_custom_style = true; w->styleptr = new StyleItem; *w->styleptr = style_get_copy(w->style);
				w->sstyleptr->draw_inner = (w->style == STYLETYPE_TOOLBAR);
			}
			if (!w->sstyleptr->has_custom_style) {
				w->sstyleptr->has_custom_style = true;
				w->sstyleptr->styleptr = new StyleItem; 
				*w->sstyleptr->styleptr = style_get_copy(w->sstyleptr->style);
			}
			COLORREF colorval;
			int beveltype;
			if ( (i<=STYLE_COLORTO_INDEX) && -1 == ( colorval = ReadColorFromString(tokens[5])) ) errors = 1;
			if ( i==STYLE_TEXTCOLOR_INDEX || i==STYLE_BORDERCOLOR_INDEX ) errors = 1;
			if ( (i==STYLE_BEVELTYPE_INDEX) && -1 == ( beveltype = get_string_index(tokens[5], szBevelTypes)) ) errors = 1;
			if ( (i==STYLE_BEVELPOS_INDEX) && !config_set_int(tokens[5], &(w->sstyleptr->styleptr->bevelposition), 1, 2)) errors = 1;
			if (!errors)
			{	switch (i)
				{	case STYLE_COLOR_INDEX: w->sstyleptr->styleptr->Color = colorval; break;
					case STYLE_COLORTO_INDEX: w->sstyleptr->styleptr->ColorTo = colorval; break;
					case STYLE_BEVELTYPE_INDEX: w->sstyleptr->styleptr->bevelstyle = beveltype;
						if (beveltype != 0 && w->sstyleptr->styleptr->bevelposition == 0)
							w->sstyleptr->styleptr->bevelposition = 1;
						break;
				}
				style_draw_invalidate(c); needs_transupdate = true; 
			}
		}
		else
		if (!stricmp(tokens[4],"DrawSliderInner") && config_set_bool(tokens[5], &(w->sstyleptr->draw_inner))){
			if(w->sstyleptr->draw_inner && !w->has_custom_style) {
				w->has_custom_style = true;
				w->styleptr = new StyleItem; 
				*w->styleptr = style_get_copy(w->style);
			}
			style_draw_invalidate(c); needs_transupdate = true;
		}
		else
		if (!stricmp(tokens[4], "SliderInnerStyle") && -1 != (i = get_string_index(tokens[5], szStyleNames)))
		{
			w->sstyleptr->draw_inner = true;
			if (!w->has_custom_style) {
				w->has_custom_style = true; w->styleptr = new StyleItem; *w->styleptr = style_get_copy(w->style);
			}
			if(w->sstyleptr->in_has_custom_style) { 
				delete w->sstyleptr->in_styleptr; 
				w->sstyleptr->in_styleptr = 0; 
				w->sstyleptr->in_has_custom_style = false; 
			} 
			w->sstyleptr->in_style = i; style_draw_invalidate(c); needs_transupdate = true;
		}

		else
		if (!strncmp(tokens[4],"SliderInner",11)  && -1 != (i = get_string_index(&tokens[4][11], szStyleProperties)) )
		{
			w->sstyleptr->draw_inner = true;
			if (!w->has_custom_style) {
				w->has_custom_style = true; w->styleptr = new StyleItem; *w->styleptr = style_get_copy(w->style);
			}
			if (!w->sstyleptr->in_has_custom_style) {
				w->sstyleptr->in_has_custom_style = true;
				w->sstyleptr->in_styleptr = new StyleItem; 
				*w->sstyleptr->in_styleptr = style_get_copy(w->sstyleptr->in_style);
			}
			COLORREF colorval;
			int beveltype;
			if ( (i<=STYLE_COLORTO_INDEX) && -1 == ( colorval = ReadColorFromString(tokens[5])) ) errors = 1;
			if ( i==STYLE_TEXTCOLOR_INDEX || i==STYLE_BORDERCOLOR_INDEX ) errors = 1;
			if ( (i==STYLE_BEVELTYPE_INDEX) && -1 == ( beveltype = get_string_index(tokens[5], szBevelTypes)) ) errors = 1;
			if ( (i==STYLE_BEVELPOS_INDEX) && !config_set_int(tokens[5], &(w->sstyleptr->in_styleptr->bevelposition), 1, 2)) errors = 1;
			if (!errors)
			{	switch (i)
				{	case STYLE_COLOR_INDEX:
						w->sstyleptr->in_styleptr->Color = colorval; break;
					case STYLE_COLORTO_INDEX:
						w->sstyleptr->in_styleptr->ColorTo = colorval; break;
					case STYLE_BEVELTYPE_INDEX: w->sstyleptr->in_styleptr->bevelstyle = beveltype;
						if (beveltype != 0 && w->sstyleptr->in_styleptr->bevelposition == 0)
							w->sstyleptr->in_styleptr->bevelposition = 1;
						break;
				}
	
				style_draw_invalidate(c); needs_transupdate = true; 
			}
		}
	}

	// Pressed Button Style settings 
	if (errors && w->is_button){
		errors = 0;
		int stylenum = (w->bstyleptr->style==STYLETYPE_DEFAULT)?get_pressedstyle_index(w->style):w->bstyleptr->style;
		if (!stricmp(tokens[4], szWPStyleWhenPressed) && -1 != (i = get_string_index(tokens[5], szPressedStyleNames)))
		{
			if(w->bstyleptr->has_custom_style) { 
				delete w->bstyleptr->styleptr; 
				w->bstyleptr->styleptr = 0; 
				w->bstyleptr->has_custom_style = false; 
				w->bstyleptr->nosavevalue = 0;
			} //get rid of previous custom style
			w->bstyleptr->style = i; style_draw_invalidate(c); needs_transupdate = true;
		}
		else
		if (!stricmp(tokens[4],"UseCustomFontWhenPressed") && config_set_bool(tokens[5], &w->bstyleptr->use_custom_font))
		{
			if(w->bstyleptr->use_custom_font){
				style_draw_invalidate(c);
				needs_transupdate = true;
				needs_pressedfontupdate = true;
			}
			else
			{
				DeleteObject(w->bstyleptr->font);
				w->bstyleptr->font = NULL;
		
				style_draw_invalidate(c);
				needs_transupdate = true;
				needs_pressedfontupdate = true;
			}
		}
		else
		if (!stricmp(tokens[4],szWPfontname_pressed))
		{
			w->bstyleptr->use_custom_font = true;
			strcpy(w->bstyleptr->Fontname,tokens[5]);
			style_draw_invalidate(c); needs_transupdate = true; 
			needs_pressedfontupdate = true;
		}
		else
		if (!stricmp(tokens[4],szWPfontheight_pressed))
		{
			w->bstyleptr->use_custom_font = true;
			w->bstyleptr->FontHeight=atoi(tokens[5]);
			style_draw_invalidate(c); needs_transupdate = true; 
			needs_pressedfontupdate = true;
		}
		else
		if(!stricmp(tokens[4],szWPfontweight_pressed))
		{
			w->bstyleptr->use_custom_font = true;
			bool temp;
			config_set_bool(tokens[5],&temp);
			w->bstyleptr->FontWeight=temp?FW_BOLD:FW_NORMAL;
			style_draw_invalidate(c); needs_transupdate = true; 
			needs_pressedfontupdate = true;
		}
		// button pressed style properties
		else
		if (-1 != (i = get_string_index(tokens[4], szPressedStyleProperties)) )
		{
			if (!w->bstyleptr->has_custom_style) {
				w->bstyleptr->has_custom_style = true;
				w->bstyleptr->styleptr = new StyleItem;
				*w->bstyleptr->styleptr = style_get_copy(stylenum);
			}
			COLORREF colorval;
			int beveltype,shadowpos;
			bool shadow_set = true;
			if ( (i<STYLE_COLOR_PROPERTY_COUNT) && -1 == ( colorval = ReadColorFromString(tokens[5])) ) errors = 1;
			if ( (i==STYLE_BEVELTYPE_INDEX) && -1 == ( beveltype = get_string_index(tokens[5], szBevelTypes)) ) errors = 1;
			if ( (i==STYLE_BEVELPOS_INDEX) && !config_set_int(tokens[5], &(w->bstyleptr->styleptr->bevelposition), 1, 2)) errors = 1;
			if ( (i==STYLE_BORDERWIDTH_INDEX) && !config_set_int(tokens[5], &(w->bstyleptr->styleptr->borderWidth), 0, 50)) errors = 1;
			if ( (i==STYLE_BORDERCOLOR_INDEX) && -1 == ( colorval = ReadColorFromString(tokens[5])) ) errors = 1;
			if (i== STYLE_SHADOWCOLOR_INDEX){
				//Shadow color
				if(!stricmp(tokens[5],"Auto"))	colorval = CLR_INVALID;
				else if(!stricmp(tokens[5],"Disable"))	shadow_set = false;
				else if((colorval = ReadColorFromString(tokens[5]))==-1) errors = 1;
			}
			if ( (i==STYLE_SHADOWPOSX_INDEX || i==STYLE_SHADOWPOSY_INDEX) && !config_set_int(tokens[5],&shadowpos,-100,100)) errors = 1;
			if (!errors)
			{	switch (i)
				{	case STYLE_COLOR_INDEX: 
						w->bstyleptr->styleptr->Color = colorval; break;
					case STYLE_COLORTO_INDEX:
						w->bstyleptr->styleptr->ColorTo = colorval; break;
					case STYLE_TEXTCOLOR_INDEX: 
						w->bstyleptr->styleptr->TextColor = colorval; break;
					case STYLE_BORDERWIDTH_INDEX:
						if(!w->bstyleptr->styleptr->bordered){
							StyleItem *tmpstyle;
							tmpstyle = new StyleItem;
							*tmpstyle = style_get_copy(stylenum);

							w->bstyleptr->styleptr->bordered = true;
							w->bstyleptr->styleptr->borderColor =
								(tmpstyle->bordered ?
								 	tmpstyle->borderColor : 
									*((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))
								);
							delete tmpstyle;
						}
						break;
					case STYLE_BORDERCOLOR_INDEX:
						w->bstyleptr->styleptr->borderColor = colorval; 
						if(!w->bstyleptr->styleptr->bordered){
							StyleItem *tmpstyle;
							tmpstyle = new StyleItem;
							*tmpstyle = style_get_copy(stylenum);

							w->bstyleptr->styleptr->bordered = true;
							w->bstyleptr->styleptr->borderWidth =
								(tmpstyle->bordered ? 
								 	tmpstyle->borderWidth : 
									*((int *)GetSettingPtr(SN_BORDERWIDTH))
								);
							delete tmpstyle;
						}
						break;
					case STYLE_BEVELTYPE_INDEX:
						w->bstyleptr->styleptr->bevelstyle = beveltype;
						if (beveltype != 0 && w->bstyleptr->styleptr->bevelposition == 0)
							w->bstyleptr->styleptr->bevelposition = 1;
						break;
					case STYLE_SHADOWCOLOR_INDEX:
						if(shadow_set){
							w->bstyleptr->styleptr->validated |= VALID_SHADOWCOLOR; // set bit
							w->bstyleptr->styleptr->ShadowColor = colorval;
						}else{
							w->bstyleptr->styleptr->validated &= ~VALID_SHADOWCOLOR; // unset
						}
						break;
					case STYLE_SHADOWPOSX_INDEX:
						w->bstyleptr->styleptr->ShadowX = (char)shadowpos;
						break;
					case STYLE_SHADOWPOSY_INDEX:
						w->bstyleptr->styleptr->ShadowY = (char)shadowpos;
						break;
				}
	
				style_draw_invalidate(c); needs_transupdate = true; 
			}
		}
		else
		if (!stricmp(tokens[4], "BaseStyle(Pressed)") && -1 != (i = get_string_index(tokens[5], szPressedStyleNames)))
		{
			if(!w->bstyleptr->has_custom_style){
				w->bstyleptr->has_custom_style = true;
				w->bstyleptr->styleptr = new StyleItem; 
				*w->bstyleptr->styleptr = style_get_copy(stylenum);
			}
			w->bstyleptr->style = i;
			style_set_customvalue(w->bstyleptr->styleptr,i,w->bstyleptr->nosavevalue);
			style_draw_invalidate(c);
			needs_transupdate = true;
		}
		else
		if (!stricmp(tokens[4], "UseCustomValue(Pressed)") && -1 != (i = get_string_index(tokens[5],szStyleProperties)))
		{
			w->bstyleptr->nosavevalue &= ~(1<<i);
			if(!w->bstyleptr->has_custom_style){
				w->bstyleptr->has_custom_style = true;
				w->bstyleptr->styleptr = new StyleItem; 
				*w->bstyleptr->styleptr = style_get_copy(stylenum);
			}
		}
		else
		if (!stricmp(tokens[4], "UseStyleDefault(Pressed)") && -1 != (i = get_string_index(tokens[5],szStyleProperties)))
		{
			w->bstyleptr->nosavevalue |= (1<<i);
			if(!w->bstyleptr->has_custom_style){
				w->bstyleptr->has_custom_style = true;
				w->bstyleptr->styleptr = new StyleItem; 
				*w->bstyleptr->styleptr = style_get_copy(stylenum);
			}
			StyleItem *tmpstyle;
			tmpstyle = new StyleItem;
			*tmpstyle = style_get_copy(stylenum);
			switch(i){
				case STYLE_COLOR_INDEX:
					w->bstyleptr->styleptr->Color = tmpstyle->Color;
					break;
				case STYLE_COLORTO_INDEX:
					w->bstyleptr->styleptr->ColorTo = tmpstyle->ColorTo;
					break;
				case STYLE_TEXTCOLOR_INDEX:
					w->bstyleptr->styleptr->TextColor = tmpstyle->TextColor;
					break;
				case STYLE_BEVELTYPE_INDEX:
					w->bstyleptr->styleptr->bevelstyle = tmpstyle->bevelstyle;
					break;
				case STYLE_BEVELPOS_INDEX:
					w->bstyleptr->styleptr->bevelposition = tmpstyle->bevelposition;
					break;
				case STYLE_BORDERWIDTH_INDEX:
					w->bstyleptr->styleptr->borderWidth =
						(tmpstyle->bordered ? 
						 	tmpstyle->borderWidth : 
							*((int *)GetSettingPtr(SN_BORDERWIDTH))
						);
					if(w->bstyleptr->nosavevalue & 1<<STYLE_BORDERCOLOR_INDEX){
						w->bstyleptr->styleptr->bordered = false;
					}
					break;
				case STYLE_BORDERCOLOR_INDEX:
					w->bstyleptr->styleptr->borderColor =
						(tmpstyle->bordered ?
						 	tmpstyle->borderColor : 
							*((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))
						);
					if(w->bstyleptr->nosavevalue & 1<<STYLE_BORDERWIDTH_INDEX){
						w->bstyleptr->styleptr->bordered = false;
					}
					break;
	
				case STYLE_SHADOWCOLOR_INDEX:
					w->bstyleptr->styleptr->ShadowColor = tmpstyle->ShadowColor;
					w->bstyleptr->styleptr->validated &= ~VALID_SHADOWCOLOR;
					w->bstyleptr->styleptr->validated |= (tmpstyle->validated & VALID_SHADOWCOLOR);
					break;
				case STYLE_SHADOWPOSX_INDEX:
					w->bstyleptr->styleptr->ShadowX = tmpstyle->ShadowX;
					break;
				case STYLE_SHADOWPOSY_INDEX:
					w->bstyleptr->styleptr->ShadowY = tmpstyle->ShadowY;
					break;

			}
			delete tmpstyle;	
			style_draw_invalidate(c);
			needs_transupdate = true;
		}
		else
		if (!stricmp(tokens[4], "DefaultBorderPressed")){
			StyleItem *tmpstyle;
			tmpstyle = new StyleItem;
			*tmpstyle = style_get_copy(stylenum);

			w->bstyleptr->styleptr->borderWidth =
				(tmpstyle->bordered ? 
				 	tmpstyle->borderWidth : 
					*((int *)GetSettingPtr(SN_BORDERWIDTH))
				);
			w->bstyleptr->styleptr->borderColor =
				(tmpstyle->bordered ?
				 	tmpstyle->borderColor : 
					*((COLORREF *)GetSettingPtr(SN_BORDERCOLOR))
				);
			w->bstyleptr->styleptr->bordered = false;

			delete tmpstyle;	
			style_draw_invalidate(c);
			needs_transupdate = true;
		}

		else
			errors = 1;
	}

	//Update fonts
	if(needs_fontupdate){
		if(w->font){
			DeleteObject(w->font);
		}
	        LOGFONT lf;
		::GetObject(style_font,sizeof(lf),&lf);
	        if(strlen(w->Fontname)){
	                strcpy(lf.lfFaceName, w->Fontname);
	        }
		if(w->FontHeight >0){
		        lf.lfHeight=w->FontHeight;
		}else{
			w->FontHeight = lf.lfHeight;
		}
	        lf.lfWeight=w->FontWeight;
	        w->font = ::CreateFontIndirect(&lf);
	}

	//Update pressed fonts
	if(needs_pressedfontupdate){
		if(w->bstyleptr->font){
			DeleteObject(w->bstyleptr->font);
		}
	        LOGFONT lf;
		::GetObject(style_font,sizeof(lf),&lf);
	        if(strlen(w->bstyleptr->Fontname)){
	                strcpy(lf.lfFaceName, w->bstyleptr->Fontname);
	        }
		if(w->bstyleptr->FontHeight >0){
		        lf.lfHeight=w->bstyleptr->FontHeight;
		}else{
			w->bstyleptr->FontHeight = lf.lfHeight;
		}
	        lf.lfWeight=w->bstyleptr->FontWeight;
	        w->bstyleptr->font = ::CreateFontIndirect(&lf);
	}


	//Update the window as necessary
	window_update(w, needs_posupdate, needs_transupdate, needs_visupdate, needs_stickyupdate);

	//Return
	return errors;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//reconfigure_customvalue
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void reconfigure_customvalue(control *c){
	window *w = c->windowptr;
	if(w->has_custom_style)
		style_set_customvalue(w->styleptr,w->style,w->nosavevalue);
	if(w->is_button && w->bstyleptr->has_custom_style)
		style_set_customvalue(w->bstyleptr->styleptr,w->bstyleptr->style,w->bstyleptr->nosavevalue);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//style_set_customvalue
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void style_set_customvalue(StyleItem *styleptr, int style, int nosavevalue)
{
	if(nosavevalue == 0) return;
	
	StyleItem *tmpstyle;
	tmpstyle = new StyleItem;
	*tmpstyle = style_get_copy(style);
	if(nosavevalue & 1)
		styleptr->Color = tmpstyle->Color;
	if(nosavevalue & 1<<STYLE_COLORTO_INDEX)
		styleptr->ColorTo = tmpstyle->ColorTo;
	if(nosavevalue & 1<<STYLE_TEXTCOLOR_INDEX)
		styleptr->TextColor = tmpstyle->TextColor;
	if(nosavevalue & 1<<STYLE_BORDERCOLOR_INDEX){
		styleptr->borderColor = tmpstyle->bordered ? 
			tmpstyle->borderColor : *((COLORREF *)GetSettingPtr(SN_BORDERCOLOR));
		if(nosavevalue & 1<<STYLE_BORDERWIDTH_INDEX){
			styleptr->bordered = false;
		}
	}
	if(nosavevalue & 1<<STYLE_BORDERWIDTH_INDEX){
		styleptr->borderWidth =	tmpstyle->bordered ? 
			tmpstyle->borderWidth : *((int *)GetSettingPtr(SN_BORDERWIDTH));
		if(nosavevalue & 1<<STYLE_BORDERCOLOR_INDEX){
			styleptr->bordered = false;
		}
	}
	if(nosavevalue & 1<<STYLE_BEVELTYPE_INDEX)
		styleptr->bevelstyle = tmpstyle->bevelstyle;
	if(nosavevalue & 1<<STYLE_BEVELPOS_INDEX)
		styleptr->bevelposition = tmpstyle->bevelposition;
	delete tmpstyle;	
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
	wc.lpszClassName    = classname;            // our window class name
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
	UnregisterClass(classname, plugin_instance_plugin);

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
		classname,                 // our window class name
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
	bool visible_fullscreen = !w->is_detectfullscreen
		|| !fullscreen_app_exist ||  fullscreen_app_hMon != GetMonitorRect(w->hwnd,NULL,GETMON_FROM_WINDOW); 
	bool visible = w->is_visible &&
		(window_is_pluginsvisible || false == w->is_toggledwithplugins)
		&& visible_fullscreen;

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
			if (w->is_onallworkspaces){
				MakeSticky(w->hwnd);
				SetTaskWorkspace(w->hwnd,w->workspacenumber);
				SendMessage(plugin_hwnd_blackbox, BB_WORKSPACE,BBWS_MAKESTICKY,(LPARAM)w->hwnd);
			}
			else{
				RemoveSticky(w->hwnd);
				SetTaskWorkspace(w->hwnd,w->workspacenumber);
				SendMessage(plugin_hwnd_blackbox, BB_WORKSPACE,BBWS_CLEARSTICKY,(LPARAM)w->hwnd);

			}
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

