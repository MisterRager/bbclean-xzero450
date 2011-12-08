/*===================================================

	CONTROLTYPE_BUTTON CODE

===================================================*/
// create text setting menu maker function
// Global Include
#include "BBApi.h"
#include <shellapi.h>

//Parent Include
#include "ControlType_Button.h"

//Includes
#include "ControlMaster.h"
#include "MessageMaster.h"
#include "StyleMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"
#include "PluginMaster.h"

//Local variables
const int controltype_button_data_defaultheight = 32;
const int controltype_button_data_defaultwidth = 96;
const int controltype_button_data_minimumheight = 8;
const int controltype_button_data_minimumwidth = 8;
const int controltype_button_data_maximumheight = 1024;
const int controltype_button_data_maximumwidth = 1024;

//Agent definitions
enum CONTROLTYPE_BUTTON_AGENT {
    CONTROLTYPE_BUTTON_CAPTION,
    CONTROLTYPE_BUTTON_IMAGE,
	CONTROLTYPE_BUTTON_IMAGE_WHENPRESSED,
    CONTROLTYPE_BUTTON_AGENT_MOUSEDOWN,
    CONTROLTYPE_BUTTON_AGENT_MOUSEUP,
    CONTROLTYPE_BUTTON_AGENT_LMOUSEDOWN,
    CONTROLTYPE_BUTTON_AGENT_LMOUSEUP,
    CONTROLTYPE_BUTTON_AGENT_RMOUSEDOWN,
    CONTROLTYPE_BUTTON_AGENT_RMOUSEUP,
    CONTROLTYPE_BUTTON_AGENT_MMOUSEDOWN,
    CONTROLTYPE_BUTTON_AGENT_MMOUSEUP,
    CONTROLTYPE_BUTTON_AGENT_ONDROP
};
const int CONTROLTYPE_BUTTON_AGENTCOUNT = 12;

enum CONTROLTYPE_TWOSTATEBUTTON_AGENT {
    CONTROLTYPE_TWOSTATEBUTTON_AGENT_VALUECHANGE = 3, // First three agents are identical to those of Button
    CONTROLTYPE_TWOSTATEBUTTON_AGENT_PRESSED,
    CONTROLTYPE_TWOSTATEBUTTON_AGENT_UNPRESSED,
    CONTROLTYPE_TWOSTATEBUTTON_AGENT_ONDROP
};
const int CONTROLTYPE_TWOSTATEBUTTON_AGENTCOUNT = 7;

char *controltype_button_agentnames[] = {"Caption", "Image", "ImageWhenPressed", "MouseDown", "MouseUp", "LeftMouseDown", "LeftMouseUp","RightMouseDown", "RightMouseUp","MiddleMouseDown", "MiddleMouseUp","OnDrop" };
int controltype_button_agenttypes[] = {CONTROL_FORMAT_TEXT, CONTROL_FORMAT_IMAGE, CONTROL_FORMAT_IMAGE, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER,CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER,CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER,CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_DROP };
char *controltype_switchbutton_agentnames[] = {"Caption", "Image", "ImageWhenPressed", "Value", "Pressed", "Unpressed", "OnDrop" };
int controltype_switchbutton_agenttypes[] = {CONTROL_FORMAT_TEXT, CONTROL_FORMAT_IMAGE, CONTROL_FORMAT_IMAGE, CONTROL_FORMAT_BOOL, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_DROP};

const char *button_haligns[] = {"Center", "Left", "Right", NULL};
const char *button_valigns[] = {"Center", "Top", "Bottom", "TopWrap", NULL};

// Local functions
void controltype_button_updatesettings(controltype_button_details *details);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_button_startup()
{
	//Register this type with the ControlMaster
	control_registertype(
		"Button",                           //Name of control type
		true,                               //Can be parentless
		false,                              //Can parent
		true,                               //Can child
		CONTROL_ID_BUTTON,
		&controltype_button_create,         
		&controltype_button_destroy,
		&controltype_button_event,
		&controltype_button_notify,
		&controltype_button_message,
		&controltype_button_getdata,
		&controltype_button_getstringdata,
		&controltype_button_menu_context,
		&controltype_button_notifytype
		);

		//Register this type with the ControlMaster
	control_registertype(
		"SwitchButton",                     //Name of control type
		true,                               //Can be parentless
		false,                              //Can parent
		true,                               //Can child
		CONTROL_ID_SWITCHBUTTON,
		&controltype_switchbutton_create,           
		&controltype_button_destroy,
		&controltype_button_event,
		&controltype_button_notify,
		&controltype_button_message,
		&controltype_button_getdata,
		&controltype_button_getstringdata,
		&controltype_switchbutton_menu_context,
		&controltype_button_notifytype
		);

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_button_shutdown()
{
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_button_create(control *c)
{
	//No details are required at the moment for this data type
	controltype_button_details *details = new controltype_button_details;
	details->pressed = false;
	details->hilite = false; 
	details->is_on = false; 
	details->is_twostate = false;
	details->caption = NULL;
	details->halign = 0;
	details->valign = 0;
	controltype_button_updatesettings(details);

	for (int i = 0; i < CONTROLTYPE_BUTTON_AGENTCOUNT; i++)
	{
		details->agents[i] = NULL;
	}
	c->controldetails = (void *) details;

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_create_twostate
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_switchbutton_create(control *c)
{
	//No details are required at the moment for this data type
	controltype_button_details *details = new controltype_button_details;
	details->pressed = false;
	details->hilite = false; 
	details->is_on = false; 
	details->is_twostate = true;
	details->caption = NULL;
	details->halign = 0;
	details->valign = 0;
	controltype_button_updatesettings(details);

	for (int i = 0; i < CONTROLTYPE_BUTTON_AGENTCOUNT; i++)
		details->agents[i] = NULL;

	c->controldetails = (void *) details;
	return 0;   
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_button_destroy(control *c)
{
	//Get the details
	controltype_button_details *details = (controltype_button_details *) c->controldetails;
	
	//Delete the agents
	for (int i = 0; i < CONTROLTYPE_BUTTON_AGENTCOUNT; i++)
		agent_destroy(&details->agents[i]);

	//Destroy the window
	window_destroy(&c->windowptr);

	//Delete the details
	delete (controltype_button_details *) c->controldetails;

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT controltype_button_event(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	controltype_button_details *details = (controltype_button_details *) c->controldetails;

	switch (msg)
	{
		case WM_DROPFILES:
		{
			char buffer[MAX_PATH];
			variables_set(false, "DroppedFile", get_dragged_file(buffer, wParam));
			agent_notify(details->agents[ (details->is_twostate ? CONTROLTYPE_TWOSTATEBUTTON_AGENT_ONDROP : CONTROLTYPE_BUTTON_AGENT_ONDROP)], NOTIFY_CHANGE, NULL);
			break;
		}

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:

			//If this is a switchbutton that is not writable, break
			if (details->is_twostate == true
				&& details->agents[CONTROLTYPE_TWOSTATEBUTTON_AGENT_VALUECHANGE] != NULL
				&& details->agents[CONTROLTYPE_TWOSTATEBUTTON_AGENT_VALUECHANGE]->writable == false)
			{
				break;
			}
			
			//Notify the agents of events
			details->pressed = details->hilite = true;
			style_draw_invalidate(c);
			if (false == details->is_twostate)
			{
				//Action button - Mouse down - TRIGGER type
				int mouse_agent = CONTROLTYPE_BUTTON_AGENT_MOUSEDOWN;
				if	( ((msg == WM_LBUTTONDOWN) || (msg == WM_LBUTTONDBLCLK)) &&
					(details->agents[CONTROLTYPE_BUTTON_AGENT_LMOUSEDOWN] != NULL))
						mouse_agent = CONTROLTYPE_BUTTON_AGENT_LMOUSEDOWN;
				else
				if	( ((msg == WM_RBUTTONDOWN) || (msg == WM_RBUTTONDBLCLK)) &&
					(details->agents[CONTROLTYPE_BUTTON_AGENT_RMOUSEDOWN] != NULL))
						mouse_agent = CONTROLTYPE_BUTTON_AGENT_RMOUSEDOWN;
				else
				if	( ((msg == WM_MBUTTONDOWN) || (msg == WM_MBUTTONDBLCLK)) &&
					(details->agents[CONTROLTYPE_BUTTON_AGENT_MMOUSEDOWN] != NULL))
						mouse_agent = CONTROLTYPE_BUTTON_AGENT_MMOUSEDOWN;

				agent_notify(details->agents[mouse_agent], NOTIFY_CHANGE, NULL);
			}
			break;

		case WM_KILLFOCUS:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:

			//If this is a switchbutton that is not writable, break
			if (details->is_twostate == true
				&& details->agents[CONTROLTYPE_TWOSTATEBUTTON_AGENT_VALUECHANGE] != NULL
				&& details->agents[CONTROLTYPE_TWOSTATEBUTTON_AGENT_VALUECHANGE]->writable == false)
			{
				break;
			}

			if (details->pressed)
			{
				bool trigger = details->hilite;
				details->pressed = details->hilite = false;
				style_draw_invalidate(c);
				if (trigger)
				{
					//Notify the agents of events
					if (details->is_twostate)
					{
						details->is_on = false == details->is_on;
						//Value change - BOOL type
						agent_notify(details->agents[CONTROLTYPE_TWOSTATEBUTTON_AGENT_VALUECHANGE], NOTIFY_CHANGE, (void *) &details->is_on);
						//Pressed/Unpressed - TRIGGER type
						agent_notify(details->agents[(details->is_on ? CONTROLTYPE_TWOSTATEBUTTON_AGENT_PRESSED : CONTROLTYPE_TWOSTATEBUTTON_AGENT_UNPRESSED)], NOTIFY_CHANGE, NULL);
					}
					else
					{
						//Action button - Mouse up - TRIGGER type
						int mouse_agent = CONTROLTYPE_BUTTON_AGENT_MOUSEUP;
						if ( (msg == WM_LBUTTONUP) && (details->agents[CONTROLTYPE_BUTTON_AGENT_LMOUSEUP] != NULL)) mouse_agent = CONTROLTYPE_BUTTON_AGENT_LMOUSEUP;
						else if ( (msg == WM_RBUTTONUP) && (details->agents[CONTROLTYPE_BUTTON_AGENT_RMOUSEUP] != NULL)) mouse_agent = CONTROLTYPE_BUTTON_AGENT_RMOUSEUP;
						else if ( (msg == WM_MBUTTONUP) && (details->agents[CONTROLTYPE_BUTTON_AGENT_MMOUSEUP] != NULL)) mouse_agent = CONTROLTYPE_BUTTON_AGENT_MMOUSEUP;

						agent_notify(details->agents[mouse_agent], NOTIFY_CHANGE, NULL);

					}
				}
			}
			break;

		case WM_MOUSEMOVE:
			if (details->pressed)
			{
				RECT r; GetClientRect(hwnd, &r);
				POINT pt;
				pt.x = (short)LOWORD(lParam);
				pt.y = (short)HIWORD(lParam);
				bool over = PtInRect(&r, pt);
				if (over != details->hilite)
				{
					details->hilite = over;
					style_draw_invalidate(c);
				}
			}
			break;

		case WM_PAINT:
			styledrawinfo d;

			//Begin drawing
			if (style_draw_begin(c, d))
			{
				//Paint background according to the current style on the bitmap ...
				int style_index = STYLETYPE_NONE;
				switch (c->windowptr->style)
				{
				case STYLETYPE_TOOLBAR:
					style_index = (details->hilite || details->is_on) ? STYLETYPE_INSET : STYLETYPE_TOOLBAR; break;
				case STYLETYPE_INSET:
					style_index = (details->hilite || details->is_on) ? STYLETYPE_TOOLBAR : STYLETYPE_INSET; break;
				case STYLETYPE_FLAT:
					style_index = (details->hilite || details->is_on) ? STYLETYPE_SUNKEN : STYLETYPE_FLAT; break;
				case STYLETYPE_SUNKEN:
					style_index = (details->hilite || details->is_on) ? STYLETYPE_FLAT : STYLETYPE_SUNKEN; break;
				}
				
				if (c->windowptr->has_custom_style)
				{
					if (details->hilite || details->is_on)
						style_draw_box(
							d.rect,
							d.buffer,
							STYLETYPE_INSET,
							c->windowptr->is_bordered
						);
					else
						style_draw_box(
							d.rect,
							d.buffer,
							c->windowptr->styleptr,
							c->windowptr->is_bordered
						);

				}
				else
				style_draw_box(
					d.rect,
					d.buffer,
					style_index,
					c->windowptr->is_bordered
					);
				
				//Draw the image, dependant on state and availability of second image
				if (details->agents[CONTROLTYPE_BUTTON_IMAGE_WHENPRESSED] && (details->hilite || details->is_on))
				{
					d.apply_sat_hue = false == (details->hilite || details->is_on);
					agent_notify(details->agents[CONTROLTYPE_BUTTON_IMAGE_WHENPRESSED], NOTIFY_DRAW, &d);
				}
				else if (details->agents[CONTROLTYPE_BUTTON_IMAGE])
				{
					d.apply_sat_hue = false == (details->hilite || details->is_on);
					agent_notify(details->agents[CONTROLTYPE_BUTTON_IMAGE], NOTIFY_DRAW, &d);
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
						d.rect.top += vpad;
						d.rect.bottom -= vpad;
					}
					if (c->windowptr->has_custom_style)
						if (details->hilite || details->is_on)
						style_draw_text(
							d.rect,
							d.buffer,
							STYLETYPE_INSET,
							details->caption,
							details->settings,
							plugin_enableshadows
							);
						else
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
						style_index,
						details->caption,
						details->settings,
						plugin_enableshadows
						);
				}
			}
			//End drawing
			style_draw_end(d);
			break;

	}

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_button_notify(control *c, int notifytype, void *messagedata)
{
	//Variables
	controltype_button_details *details;
	bool *newvalueptr;

	//Get details
	details = (controltype_button_details *) c->controldetails;

	//Find out what to do
	switch (notifytype)
	{       
		case NOTIFY_NEEDUPDATE:
			//When we are told we need an update, figure out what to do
			//Update the caption first
			if (details->agents[CONTROLTYPE_BUTTON_CAPTION])
			{
				details->caption = (char *) agent_getdata(details->agents[CONTROLTYPE_BUTTON_CAPTION], DATAFETCH_VALUE_TEXT);
			}
			else
			{
				details->caption = NULL;
			}

			//Only two state buttons need updates at this point
			if (details->is_twostate)
			{
				//Get the value
				newvalueptr = (bool *)(agent_getdata(details->agents[CONTROLTYPE_TWOSTATEBUTTON_AGENT_VALUECHANGE], DATAFETCH_VALUE_BOOL));
				if (newvalueptr)
				{
					//Set the new value
					details->is_on = *newvalueptr;
				}
			}

			//Tell the button to draw itself again
			style_draw_invalidate(c);
			break;

		case NOTIFY_SAVE_CONTROL:
			//Save all local values
			config_write(config_get_control_setcontrolprop_c(c, "HAlign", button_haligns[details->halign]));
			config_write(config_get_control_setcontrolprop_c(c, "VAlign", button_valigns[details->valign]));
			int i;
			for (i = 0; i < CONTROLTYPE_BUTTON_AGENTCOUNT; i++)
				agent_notify(details->agents[i], NOTIFY_SAVE_AGENT, NULL);
			if (details->is_twostate)
				config_write(config_get_control_setcontrolprop_b(c, "Pressed", &details->is_on));
			break;

		case NOTIFY_DRAGACCEPT:
			DragAcceptFiles(c->windowptr->hwnd, NULL != details->agents[details->is_twostate ? CONTROLTYPE_TWOSTATEBUTTON_AGENT_ONDROP : CONTROLTYPE_BUTTON_AGENT_ONDROP]);
			break;

	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_button_message(control *c, int tokencount, char *tokens[])
{
	//Get the details
	controltype_button_details *details = (controltype_button_details *) c->controldetails;

	//If it's set control property
	if (tokencount == 6 && !stricmp(tokens[2], szBActionSetControlProperty))
	{
		//Only pressed, only for two state buttons
		if (details->is_twostate && !stricmp(tokens[4], "Pressed") && config_set_bool(tokens[5], &details->is_on))
		{
			style_draw_invalidate(c);
			return 0;
		}
		else if (!strcmp(tokens[4],"VAlign"))
		{
			int i;
			if (-1 != (i = get_string_index(tokens[5], button_valigns)))
			{
				details->valign = i;
				controltype_button_updatesettings(details);
				style_draw_invalidate(c);
				return 0;
			}
		}
		else if (!strcmp(tokens[4],"HAlign"))
		{
			int i;
			if (-1 != (i = get_string_index(tokens[5], button_haligns)))
			{
				details->halign = i;
				controltype_button_updatesettings(details);
				style_draw_invalidate(c);
				return 0;
			}
		}

		//Must be an error
		return 1;
	}

	//Must be an agent message?
	if (details->is_twostate)
		return agent_controlmessage(c, tokencount, tokens, CONTROLTYPE_TWOSTATEBUTTON_AGENTCOUNT, details->agents, controltype_switchbutton_agentnames, controltype_switchbutton_agenttypes);
	else
		return agent_controlmessage(c, tokencount, tokens, CONTROLTYPE_BUTTON_AGENTCOUNT, details->agents, controltype_button_agentnames, controltype_button_agenttypes);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *controltype_button_getdata(control *c, int datatype)
{
	controltype_button_details *details = (controltype_button_details *) c->controldetails;
	switch (datatype)
	{
		case DATAFETCH_VALUE_BOOL:
			if (details->is_twostate) return &details->is_on;
			break;
		case DATAFETCH_INT_DEFAULTHEIGHT:
			return (void *) &controltype_button_data_defaultheight;
		case DATAFETCH_INT_DEFAULTWIDTH:
			return (void *) &controltype_button_data_defaultwidth;
		case DATAFETCH_INT_MIN_WIDTH:
			return (void *) &controltype_button_data_minimumwidth;
		case DATAFETCH_INT_MIN_HEIGHT:
			return (void *) &controltype_button_data_minimumheight;
		case DATAFETCH_INT_MAX_WIDTH:
			return (void *) &controltype_button_data_maximumwidth;
		case DATAFETCH_INT_MAX_HEIGHT:
			return (void *) &controltype_button_data_maximumheight;
		case DATAFETCH_CONTENTSIZES:
		{			
			if (details->agents[CONTROLTYPE_BUTTON_IMAGE])
				return agent_getdata(details->agents[CONTROLTYPE_BUTTON_IMAGE], datatype);
			break;
		}
	}
	
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_getstringdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool controltype_button_getstringdata(control *c, char *buffer, char *propertyname)
{
	controltype_button_details *details = (controltype_button_details *) c->controldetails;
	if (details->is_twostate && !stricmp(propertyname,"Pressed"))
	{
		strcpy(buffer, (details->is_on ? "1" : "0"));
		return true;
	}
	else if (!stricmp(propertyname, "Caption"))
	{
		strcpy(buffer, details->caption);
	}
	//Broadcast value is a special text value that ALL controls should be able to broadcast
	//It contains a certain "value" that can be associated with a control.  Various things
	//such as broams can reference it.
	//For sliders, this value is the adjusted value using the min and max.
	else if (!stricmp(propertyname,"BroadcastValue"))
	{
		if (details->is_twostate)
		{
			sprintf(buffer, "%d", details->is_on);
		}
	}

	//No data to return - return false
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_button_menu_context(Menu *m, control *c)
{
	//Get the details
	controltype_button_details *details = (controltype_button_details *) c->controldetails;

	//Show the menu
	menu_controloptions(m, c, CONTROLTYPE_BUTTON_AGENTCOUNT, details->agents, "", controltype_button_agentnames, controltype_button_agenttypes);

	make_menuitem_nop(m, "");
	Menu *submenu = make_menu("Text Settings", c);
	make_menuitem_bol(submenu, "Left", config_getfull_control_setcontrolprop_c(c, "HAlign", "Left"), details->halign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setcontrolprop_c(c, "HAlign", "Center"), details->halign == 0);
	make_menuitem_bol(submenu, "Right", config_getfull_control_setcontrolprop_c(c, "HAlign", "Right"), details->halign == 2);
	make_menuitem_nop(submenu, "");
	make_menuitem_bol(submenu, "Top, Word Wrapped", config_getfull_control_setcontrolprop_c(c, "VAlign", "TopWrap"), details->valign == 3);
	make_menuitem_bol(submenu, "Top", config_getfull_control_setcontrolprop_c(c, "VAlign", "Top"), details->valign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setcontrolprop_c(c, "VAlign", "Center"), details->valign == 0);
	make_menuitem_bol(submenu, "Bottom", config_getfull_control_setcontrolprop_c(c, "VAlign", "Bottom"), details->valign == 2);
	make_submenu_item(m, "Text Settings", submenu);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_switchbutton_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_switchbutton_menu_context(Menu *m, control *c)
{
	//Get the details
	controltype_button_details *details = (controltype_button_details *) c->controldetails;

	//Show the menu
	menu_controloptions(m, c, CONTROLTYPE_TWOSTATEBUTTON_AGENTCOUNT, details->agents, "", controltype_switchbutton_agentnames, controltype_switchbutton_agenttypes);

	make_menuitem_nop(m, "");
	Menu* submenu = make_menu("Text Settings", c);
	make_menuitem_bol(submenu, "Left", config_getfull_control_setcontrolprop_c(c, "HAlign", "Left"), details->halign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setcontrolprop_c(c, "HAlign", "Center"), details->halign == 0);
	make_menuitem_bol(submenu, "Right", config_getfull_control_setcontrolprop_c(c, "HAlign", "Right"), details->halign == 2);
	make_menuitem_nop(submenu, "");
	make_menuitem_bol(submenu, "Top, Word Wrapped", config_getfull_control_setcontrolprop_c(c, "VAlign", "TopWrap"), details->valign == 3);
	make_menuitem_bol(submenu, "Top", config_getfull_control_setcontrolprop_c(c, "VAlign", "Top"), details->valign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setcontrolprop_c(c, "VAlign", "Center"), details->valign == 0);
	make_menuitem_bol(submenu, "Bottom", config_getfull_control_setcontrolprop_c(c, "VAlign", "Bottom"), details->valign == 2);
	make_submenu_item(m, "Text Settings", submenu);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_button_notifytype(int notifytype, void *messagedata)
{

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_updatesettings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_button_updatesettings(controltype_button_details *details)
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
