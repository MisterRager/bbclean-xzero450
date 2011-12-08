/*===================================================

	CONTROLTYPE_LABEL CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <shellapi.h>

//Parent Include
#include "ControlType_Label.h"

//Includes
#include "ControlMaster.h"
#include "StyleMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"

#include "DialogMaster.h"
#include "MessageMaster.h"
#include "SlitManager.h"
#include "PluginMaster.h"

//Local variables
const int controltype_label_data_defaultheight = 16;
const int controltype_label_data_defaultwidth = 128;
const int controltype_label_data_minimumheight = 8;
const int controltype_label_data_minimumwidth = 8;
const int controltype_label_data_maximumheight = 1024;
const int controltype_label_data_maximumwidth = 1024;
const int controltype_frame_data_defaultheight = 128;
const int controltype_frame_data_defaultwidth = 256;
const int controltype_frame_data_minimumheight = 8;
const int controltype_frame_data_minimumwidth = 8;
const int controltype_frame_data_maximumheight = 1024;
const int controltype_frame_data_maximumwidth = 1024;

const int FRAME_TITLEBAR_HEIGHT = 20;

const char *label_haligns[] = {"Center", "Left", "Right", NULL};
const char *label_valigns[] = {"Center", "Top", "Bottom", "TopWrap", NULL};

//Agents
char *controltype_label_agentnames[] = {"Caption", "Image", "RightMouseUp", "OnDrop"};
int controltype_label_agenttypes[] = {CONTROL_FORMAT_TEXT, CONTROL_FORMAT_IMAGE, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_DROP};

//Indexes
enum CONTROLTYPE_LABEL_AGENT {
	CONTROLTYPE_LABEL_AGENT_CAPTION,
	CONTROLTYPE_LABEL_AGENT_IMAGE,
	CONTROLTYPE_LABEL_AGENT_RIGHTMOUSEUP,
	CONTROLTYPE_LABEL_AGENT_ONDROP
};
const int CONTROLTYPE_LABEL_AGENT_COUNT = 4;

const char szBActionPlugin [] = "ExternalPlugin";
const char szBActionPluginSetProperty [] = "SetProperty";

//Local functions
void controltype_label_updatesettings(controltype_label_details *details);
int controltype_labelframe_create(control *c, bool isframe);
void controltype_frame_insertpluginmenu(control *c, Menu *m);
void controltype_frame_saveplugins(control *c);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_label_startup()
{
	//Register this type with the ControlMaster
	control_registertype(
		"Frame",                            //Name of control type
		true,                               //Can be parentless
		true,                               //Can parent
		true,                               //Can child
		CONTROL_ID_FRAME,
		&controltype_frame_create,          
		&controltype_label_destroy,
		&controltype_label_event,
		&controltype_label_notify,
		&controltype_label_message,
		&controltype_frame_getdata,
		&controltype_label_getstringdata,
		&controltype_label_menu_context,
		&controltype_label_notifytype
		);

	//Register this type with the ControlMaster
	control_registertype(
		"Label",                            //Name of control type
		true,                               //Can be parentless
		false,                              //Can parent
		true,                               //Can child
		CONTROL_ID_LABEL,
		&controltype_label_create,          
		&controltype_label_destroy,
		&controltype_label_event,
		&controltype_label_notify,
		&controltype_label_message,
		&controltype_label_getdata,
		&controltype_label_getstringdata,
		&controltype_label_menu_context,
		&controltype_label_notifytype
		);

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_label_shutdown()
{
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_label_create(control *c)
{
	return controltype_labelframe_create(c, false);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_frame_create(control *c)
{
	return controltype_labelframe_create(c, true);
}

//##################################################
//controltype_label_create
//##################################################
int controltype_labelframe_create(control *c, bool isframe)
{
	int i;

	//No details are required at the moment for this data type
	controltype_label_details *details = new controltype_label_details;
	details->caption = NULL;

	for (i = 0; i < CONTROLTYPE_LABEL_AGENT_COUNT; i++)
		details->agents[i] = NULL;

	details->halign = 0;
	details->has_titlebar = false;
	details->is_locked = false;

	details->plugin_info = NULL;
	details->module_info = NULL;

	details->is_frame = isframe;
	if (isframe) details->valign = 1;
	else details->valign = 0;
	controltype_label_updatesettings(details);

	c->controldetails = (void *) details;
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_label_destroy(control *c)
{
	int i;

	//Get the details
	controltype_label_details *details = (controltype_label_details *) c->controldetails;
	
	//Delete the agents
	for (i = 0; i < CONTROLTYPE_LABEL_AGENT_COUNT; i++)
		agent_destroy(&details->agents[i]);

	unloadPlugin(&details->module_info, NULL);

	//Destroy the window
	window_destroy(&c->windowptr);

	//Delete the details
	delete (controltype_label_details *) c->controldetails;

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT controltype_label_event(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	controltype_label_details *details = (controltype_label_details *) c->controldetails;

	switch (msg)
	{
		case WM_DROPFILES:
		{
			char buffer[MAX_PATH];
			variables_set(false, "DroppedFile", get_dragged_file(buffer, wParam));
			agent_notify(details->agents[CONTROLTYPE_LABEL_AGENT_ONDROP], NOTIFY_CHANGE, NULL);
			break;
		}

		case WM_PAINT:
			styledrawinfo d;

			//Begin drawing
			if (style_draw_begin(c, d))
			{
				//Paint background according to the current style on the bitmap ...
				if(c->windowptr->has_custom_style)
				style_draw_box(
					d.rect,
					d.buffer,
					c->windowptr->styleptr,
					c->windowptr->is_bordered
					);
				else 
				style_draw_box(
					d.rect,
					d.buffer,
					c->windowptr->style,
					c->windowptr->is_bordered
					);

				//Draw the title bar
				if (details->has_titlebar)
				{
					RECT titlebar = d.rect;
					int b = d.rect.top+FRAME_TITLEBAR_HEIGHT;
					titlebar.bottom = b < d.rect.bottom ? b : d.rect.bottom;
					style_draw_box(titlebar, d.buffer, STYLETYPE_TOOLBAR, c->windowptr->is_bordered);
				}

				//Draw the image
				if (details->agents[CONTROLTYPE_LABEL_AGENT_IMAGE])
				{
					agent_notify(details->agents[CONTROLTYPE_LABEL_AGENT_IMAGE], NOTIFY_DRAW, &d);
				}

				//Draw the caption
				if (details->caption)
				{
					int vpad = style_bevel_width + 1;
					if (c->windowptr->is_bordered) vpad += style_border_width;
					if (vpad < 2) vpad = 2;

					int hpad = vpad;
					if (0 == (details->settings & DT_CENTER)) hpad += 1;
					d.rect.left  += hpad;
					d.rect.right -= hpad;

					if (!(details->settings & DT_VCENTER))
					{
						if (details->has_titlebar)
							d.rect.top += (FRAME_TITLEBAR_HEIGHT-style_font_height)/2;
						else
							d.rect.top += vpad;

						d.rect.bottom -= vpad;
					}
					else
					{
						if (details->has_titlebar)
							d.rect.top += FRAME_TITLEBAR_HEIGHT;
					}

					if(c->windowptr->has_custom_style)
					style_draw_text(
						d.rect,
						d.buffer,
						c->windowptr->styleptr,
						details->caption,
						details->settings,
						plugin_enableshadows
						);
					else 
					style_draw_text(
						d.rect,
						d.buffer,
						c->windowptr->style,
						details->caption,
						details->settings,
						plugin_enableshadows
						);
				}
			}
			//End drawing
			style_draw_end(d);
			break;

		case WM_RBUTTONUP:
		case WM_NCRBUTTONUP:
			agent_notify(details->agents[CONTROLTYPE_LABEL_AGENT_RIGHTMOUSEUP], NOTIFY_CHANGE, NULL);
			break;

		case SLIT_ADD:
		case SLIT_REMOVE:
		case SLIT_UPDATE:
			SlitWndProc(c, hwnd, msg, wParam, lParam);
			break;

	}
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_label_notify(control *c, int notifytype, void *messagedata)
{
	//Variables
	controltype_label_details *details;
	int i;

	//Get details
	details = (controltype_label_details *) c->controldetails;

	//Find out what to do
	switch (notifytype)
	{       
		case NOTIFY_NEEDUPDATE:
			//When we are told we need an update, figure out what to do
			//Update the caption first
			if (details->agents[CONTROLTYPE_LABEL_AGENT_CAPTION])
			{
				details->caption = (char *) agent_getdata(details->agents[CONTROLTYPE_LABEL_AGENT_CAPTION], DATAFETCH_VALUE_TEXT);
			}
			else
			{
				details->caption = NULL;
			}

			//Tell the label to draw itself again
			style_draw_invalidate(c);
			break;

		case NOTIFY_SAVE_CONTROL:
			//Save all local values
			config_write(config_get_control_setcontrolprop_c(c, "HAlign", label_haligns[details->halign]));
			config_write(config_get_control_setcontrolprop_c(c, "VAlign", label_valigns[details->valign]));
			if (details->is_frame)
			{
				config_write(config_get_control_setcontrolprop_b(c, "HasTitleBar", &details->has_titlebar));
				config_write(config_get_control_setcontrolprop_b(c, "IsLocked", &details->is_locked));
			}

			for (i = 0; i < CONTROLTYPE_LABEL_AGENT_COUNT; i++)
				agent_notify(details->agents[i], NOTIFY_SAVE_AGENT, NULL);

			if (details->is_frame)
				controltype_frame_saveplugins(c);
			break;

		case NOTIFY_DRAGACCEPT:
			DragAcceptFiles(c->windowptr->hwnd, NULL != details->agents[CONTROLTYPE_LABEL_AGENT_ONDROP]);
			break;
		}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_label_message(control *c, int tokencount, char *tokens[])
{
	// token strings

	enum {
		T_NONE          = 0,
		T_STYLE         ,
		T_VALIGN        ,
		T_HALIGN        ,
		T_HASTITLE      ,
		T_LOCKPOS       ,

		T_PLUGINLOAD    ,
		T_PLUGINUNLOAD  ,
		T_PLUGINSETPOS  ,
		T_PLUGINSHOW    ,

		T_PLUGINABOUT
	};

	extern char szWPStyle [];

	static struct token_check controltype_label_property_tokens[] =
	{
		{ szWPStyle,            T_STYLE         , 1 },
		{ "VAlign",             T_VALIGN        , 1 },
		{ "HAlign",             T_HALIGN        , 1 },
		{ "HasTitleBar",        T_HASTITLE      , 1 },
		{ "IsLocked",           T_LOCKPOS       , 1 },
		{ NULL }
	};

	static struct token_check controltype_label_plugin_property_tokens[] =
	{
		{ "IsVisible",          T_PLUGINSHOW    , 1 },
		{ "Position",           T_PLUGINSETPOS  , 2 },
		{ NULL }
	};

	static struct token_check controltype_label_plugin_tokens[] =
	{
		{ "Load",               T_PLUGINLOAD    , 1 },
		{ "Unload",             T_PLUGINUNLOAD  , 1 },
		{ "About",              T_PLUGINABOUT   , 0 },
		{ szBActionPluginSetProperty, (int)controltype_label_plugin_property_tokens  , 2 },
		{ NULL }
	};

	static struct token_check controltype_label_message_tokens[] =
	{
		{ szBActionSetControlProperty, (int)controltype_label_property_tokens, 2 },
		{ szBActionPlugin, (int)controltype_label_plugin_tokens, 2 },
		{ NULL }
	};

	// -----------------
	//Get the details
	controltype_label_details *details = (controltype_label_details *) c->controldetails;

	//If set control details
	int i;
	int curtok = 2;
	switch (token_check(controltype_label_message_tokens, &curtok, tokencount, tokens))
	{
		// -----------------
		case T_STYLE:
		if (-1 != (i = get_string_index(tokens[curtok], szStyleNames)))
		{
			c->windowptr->style = i;
			style_draw_invalidate(c);
			return 0;
		}
		break;

		// -----------------
		case T_HALIGN:
		if (-1 != (i = get_string_index(tokens[curtok], label_haligns)))
		{
			details->halign = i;
			controltype_label_updatesettings(details);
			style_draw_invalidate(c);
			return 0;
		}
		break;

		// -----------------
		case T_VALIGN:
		if (-1 != (i = get_string_index(tokens[curtok], label_valigns)))
		{
			details->valign = i;
			controltype_label_updatesettings(details);
			style_draw_invalidate(c);
			return 0;
		}
		break;

		// -----------------
		case T_HASTITLE:
		if (details->is_frame && config_set_bool(tokens[curtok], &details->has_titlebar))
		{
			style_draw_invalidate(c);
			return 0;
		}
		break;

		// -----------------
		case T_LOCKPOS:
		if (details->is_frame && config_set_bool(tokens[curtok], &details->is_locked))
			return 0;
		break;

		// -----------------
		case T_PLUGINLOAD:
		if (details->is_frame)
		{
			char *plugin_name = tokens[curtok];
			if (0 == strcmp(plugin_name, "*browse*"))
			{
				// "open file" dialog
				plugin_name = dialog_file("Plugins\0*.dll\0", "Add Plugin" , NULL, ".dll", false);
				if (NULL == plugin_name)
				{
					//message_override = true;
					return 2;
				}
			}
			ModuleInfo * m = loadPlugin(&details->module_info, c->windowptr->hwnd, plugin_name);
			if (m)
			{
				variables_set(false, "LastPlugin", m->module_name);
				return 0;
			}
		}
		break;

		// -----------------
		case T_PLUGINUNLOAD:
			if (unloadPlugin(&details->module_info, tokens[curtok]))
				return 0;
		break;

		// -----------------
		case T_PLUGINSETPOS:
		{
			int x, y;
			if (config_set_int(tokens[curtok], &x)
			 && config_set_int(tokens[1+curtok], &y)
			 && plugin_setpos(details->plugin_info, tokens[-2+curtok], x, y)
				)
				return 0;

			return 0; // dont generate an error here...
		}
		break;

		// -----------------
		case T_PLUGINSHOW:
		{
			bool show;
			if (plugin_getset_show_state(details->plugin_info, tokens[-2+curtok],
				config_set_bool(tokens[curtok], &show) ? show : 2)
				)
				return 0;

			return 0; // dont generate an error here...
		}
		break;

		// -----------------
		case T_PLUGINABOUT:
		{
			aboutPlugins(details->module_info, c->controlname);
			return 0;
		}
		break;

		// -----------------
		//Must be an agent message
		default:
			return agent_controlmessage(c, tokencount, tokens, CONTROLTYPE_LABEL_AGENT_COUNT, details->agents, controltype_label_agentnames, controltype_label_agenttypes);
	}
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *controltype_label_getdata(control *c, int datatype)
{
	switch (datatype)
	{
		case DATAFETCH_INT_DEFAULTHEIGHT:
			return (void *) &controltype_label_data_defaultheight;
		case DATAFETCH_INT_DEFAULTWIDTH:
			return (void *) &controltype_label_data_defaultwidth;
		case DATAFETCH_INT_MIN_WIDTH:
			return (void *) &controltype_label_data_minimumwidth;
		case DATAFETCH_INT_MIN_HEIGHT:
			return (void *) &controltype_label_data_minimumheight;
		case DATAFETCH_INT_MAX_WIDTH:
			return (void *) &controltype_label_data_maximumwidth;
		case DATAFETCH_INT_MAX_HEIGHT:
			return (void *) &controltype_label_data_maximumheight;
		case DATAFETCH_CONTENTSIZES:
		{
			controltype_label_details *details = (controltype_label_details *) c->controldetails;
			if (details->agents[CONTROLTYPE_LABEL_AGENT_IMAGE])
				return agent_getdata(details->agents[CONTROLTYPE_LABEL_AGENT_IMAGE], datatype);
			break;
		}
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_getstringdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool controltype_label_getstringdata(control *c, char *buffer, char *propertyname)
{
	//No data to return - return false
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_frame_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *controltype_frame_getdata(control *c, int datatype)
{
	switch (datatype)
	{
		case DATAFETCH_INT_DEFAULTHEIGHT:
			return (void *) &controltype_frame_data_defaultheight;
		case DATAFETCH_INT_DEFAULTWIDTH:
			return (void *) &controltype_frame_data_defaultwidth;
		case DATAFETCH_INT_MIN_WIDTH:
			return (void *) &controltype_frame_data_minimumwidth;
		case DATAFETCH_INT_MIN_HEIGHT:
			return (void *) &controltype_frame_data_minimumheight;
		case DATAFETCH_INT_MAX_WIDTH:
			return (void *) &controltype_frame_data_maximumwidth;
		case DATAFETCH_INT_MAX_HEIGHT:
			return (void *) &controltype_frame_data_maximumheight;

		case DATAFETCH_CONTENTSIZES:
			return controltype_label_getdata(c, datatype);
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_label_menu_context(Menu *m, control *c)
{
	//Variables
	Menu *submenu;

	//Get the details
	controltype_label_details *details = (controltype_label_details *) c->controldetails;

	//Show the menu
	menu_controloptions(m, c, CONTROLTYPE_LABEL_AGENT_COUNT, details->agents, "", controltype_label_agentnames, controltype_label_agenttypes);

	make_menuitem_nop(m, "");

	submenu = make_menu("Text Settings", c);
	make_menuitem_bol(submenu, "Left", config_getfull_control_setcontrolprop_c(c, "HAlign", "Left"), details->halign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setcontrolprop_c(c, "HAlign", "Center"), details->halign == 0);
	make_menuitem_bol(submenu, "Right", config_getfull_control_setcontrolprop_c(c, "HAlign", "Right"), details->halign == 2);

	make_menuitem_nop(submenu, "");

	make_menuitem_bol(submenu, "Top, Word Wrapped", config_getfull_control_setcontrolprop_c(c, "VAlign", "TopWrap"), details->valign == 3);
	make_menuitem_bol(submenu, "Top", config_getfull_control_setcontrolprop_c(c, "VAlign", "Top"), details->valign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setcontrolprop_c(c, "VAlign", "Center"), details->valign == 0);
	make_menuitem_bol(submenu, "Bottom", config_getfull_control_setcontrolprop_c(c, "VAlign", "Bottom"), details->valign == 2);

	make_submenu_item(m, "Text Settings", submenu);

	if (details->is_frame)
	{
		bool temp;
		temp = !details->has_titlebar;
		make_menuitem_bol(m, "Has Title Bar", config_getfull_control_setcontrolprop_b(c, "HasTitleBar", &temp), false == temp);
		temp = !details->is_locked;
		make_menuitem_bol(m, "Lock Children", config_getfull_control_setcontrolprop_b(c, "IsLocked", &temp), false == temp);
		make_menuitem_nop(m, NULL);
		controltype_frame_insertpluginmenu(c, m);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_label_updatesettings(controltype_label_details *details)
{
	switch(details->valign)
	{
		case 0: details->settings = (DT_SINGLELINE | DT_VCENTER); break;
		case 1: details->settings = (DT_SINGLELINE | DT_TOP); break;
		case 2: details->settings = (DT_SINGLELINE | DT_BOTTOM); break;
		case 3: details->settings = (DT_WORDBREAK | DT_TOP ); break;    
	}   
	switch(details->halign)
	{
		case 0: details->settings = details->settings | DT_CENTER; break;
		case 1: details->settings = details->settings | DT_LEFT; break;
		case 2: details->settings = details->settings | DT_RIGHT; break;
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_label_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_label_notifytype(int notifytype, void *messagedata)
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// plugin stuff

extern char *config_masterbroam;

char *config_get_control_plugin_s(control *c, const char *key)
{   sprintf(config_masterbroam, "%s %s %s %s %s", szBBroam, szBEntityControl, szBActionPlugin, c->controlname, key); return config_masterbroam;  }
char *config_get_control_plugin_c(control *c, const char *key, const char * plug)
{   sprintf(config_masterbroam, "%s %s %s %s %s \"%s\"", szBBroam, szBEntityControl, szBActionPlugin, c->controlname, key, plug); return config_masterbroam;  }
char *config_get_control_plugin_prop_b(control *c, const char * plug, const char *key, bool val)
{   sprintf(config_masterbroam, "%s %s %s %s %s %s %s %s", szBBroam, szBEntityControl, szBActionPlugin, c->controlname, szBActionPluginSetProperty, plug, key, szBoolArray[val]); return config_masterbroam;  }
char *config_get_control_plugin_prop_ii(control *c, const char * plug, const char *key, int x, int y)
{   sprintf(config_masterbroam, "%s %s %s %s %s %s %s %d %d", szBBroam, szBEntityControl, szBActionPlugin, c->controlname, szBActionPluginSetProperty, plug, key, x, y); return config_masterbroam;  }

char *config_getfull_control_plugin_s(control *c, const char *key)
{   sprintf(config_masterbroam, "%s %s %s %s:%s %s", szBBroam, szBEntityControl, szBActionPlugin, c->moduleptr->name, c->controlname, key); return config_masterbroam;  }
char *config_getfull_control_plugin_c(control *c, const char *key, const char * plug)
{   sprintf(config_masterbroam, "%s %s %s %s:%s %s \"%s\"", szBBroam, szBEntityControl, szBActionPlugin, c->moduleptr->name, c->controlname, key, plug); return config_masterbroam;  }
char *config_getfull_control_plugin_prop_b(control *c, const char * plug, const char *key, bool val)
{   sprintf(config_masterbroam, "%s %s %s %s:%s %s %s %s %s", szBBroam, szBEntityControl, szBActionPlugin, c->moduleptr->name, c->controlname, szBActionPluginSetProperty, plug, key, szBoolArray[val]); return config_masterbroam;  }
char *config_getfull_control_plugin_prop_ii(control *c, const char * plug, const char *key, int x, int y)
{   sprintf(config_masterbroam, "%s %s %s %s:%s %s %s %s %d %d", szBBroam, szBEntityControl, szBActionPlugin, c->moduleptr->name, c->controlname, szBActionPluginSetProperty, plug, key, x, y); return config_masterbroam;  }


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_frame_insertpluginmenu
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_frame_insertpluginmenu(control *c, Menu *m)
{
	const char menu_title[] = "Plugins";
	Menu *sub = make_menu(menu_title, c);

	controltype_label_details *details = (controltype_label_details *) c->controldetails;
	make_menuitem_cmd(sub, "Load Plugin...", config_getfull_control_plugin_c(c, "Load", "*browse*"));
	static const char *submenu_titles[] = { "Unload Plugin", "Visibility" };
	for (int n = 0; n < 2; n++)
	{
		Menu *sub2 = make_menu(submenu_titles[n], c);
		ModuleInfo *mi = details->module_info;
		if (NULL == mi) make_menuitem_nop(sub2, "No plugins loaded");
		for (;mi; mi=mi->next)
		{
			if (0 == n)
			{
				make_menuitem_bol(sub2, mi->module_name,
					config_getfull_control_plugin_c(c, "Unload", mi->module_name), false);
			}
			else
			if (1 == n)
			{
				bool temp = !plugin_getset_show_state(details->plugin_info, mi->module_name, 3);
				make_menuitem_bol(sub2, mi->module_name,
					config_getfull_control_plugin_prop_b(c, mi->module_name, "IsVisible", temp), !temp);
			}
		}
		make_submenu_item(sub, submenu_titles[n], sub2);
	}
	if (details->module_info)
		make_menuitem_cmd(sub, "About Plugins", config_getfull_control_plugin_s(c, "About"));

	make_submenu_item(m, menu_title, sub);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_frame_saveplugins
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_frame_saveplugins(control *c)
{
	controltype_label_details *details = (controltype_label_details *) c->controldetails;
	// Save Plugins
	ModuleInfo *m = details->module_info;
	if (NULL == m) return;

	config_printf("!---- %s::ExternalPlugins ----", c->controlname);
	for (;m; m=m->next)
	{
		config_write(config_get_control_plugin_c(c, "Load", m->file_name));
		bool temp = plugin_getset_show_state(details->plugin_info, m->module_name, 3);
		config_write(config_get_control_plugin_prop_b(c, m->module_name, "IsVisible", temp));
		PluginInfo *p = details->plugin_info;
		for (;p; p=p->next)
		{
			if (p->hMO == m->hMO)
			config_write(config_get_control_plugin_prop_ii(c, p->module_name, "Position", p->xpos, p->ypos));
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool is_locked_frame(control *c)
{
	if (c && CONTROL_ID_FRAME == c->controltypeptr->id)
		return ((controltype_label_details *) c->controldetails)->is_locked;
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
