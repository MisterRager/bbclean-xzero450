/*===================================================

	CONTROLTYPE_SLIDER CODE

===================================================*/

// Global Include
#include "BBApi.h"

//Parent Include
#include "ControlType_Slider.h"

//Includes
#include "ControlMaster.h"
#include "StyleMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"

//Local variables
const int controltype_slider_data_defaultheight = 24;
const int controltype_slider_data_defaultwidth = 256;
const int controltype_slider_data_minimumheight = 12;
const int controltype_slider_data_minimumwidth = 16;
const int controltype_slider_data_maximumheight = 1024;
const int controltype_slider_data_maximumwidth = 1024;

//Definitions
const int SLIDER_OUTERBORDERMARGIN = 3;
const int SLIDER_INNERBORDERMARGIN = 4;
const int SLIDER_MINIMUMWIDTH = 2;

const int slider_appearance_fillbar = 0;
const int slider_appearance_scrollbar = 1;
const int slider_appearance_trackknob = 2;

//Internally defined functions
void controltype_slider_updatetrack(control *c, controltype_slider_details *details, RECT &windowrect);
void controltype_slider_updateknob(control *c, controltype_slider_details *details, RECT &windowrect);
void controltype_slider_onmouse(control *c, controltype_slider_details *details, int mouse_x, int mouse_y);

//Agents
enum CONTROLTYPE_SLIDER_AGENT {
    CONTROLTYPE_SLIDER_VALUE,
	CONTROLTYPE_SLIDER_AGENT_ONCHANGE,
	CONTROLTYPE_SLIDER_AGENT_ONGRAB,    
    CONTROLTYPE_SLIDER_AGENT_ONRELEASE
};
const int CONTROLTYPE_SLIDER_AGENTCOUNT = 4;

char *controltype_slider_agentnames[] = {"Value", "OnChange", "OnGrab", "OnRelease"};
int controltype_slider_agenttypes[] = {CONTROL_FORMAT_SCALE, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_slider_startup()
{
	//Register this type with the ControlMaster
	control_registertype(
		"Slider",                           //Name of control type
		true,                               //Can be parentless
		false,                              //Can parent
		true,                               //Can child
		CONTROL_ID_SLIDER,
		&controltype_slider_create,         
		&controltype_slider_destroy,
		&controltype_slider_event,
		&controltype_slider_notify,
		&controltype_slider_message,
		&controltype_slider_getdata,
		&controltype_slider_getstringdata,
		&controltype_slider_menu_context,
		&controltype_slider_notifytype
		);

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_slider_shutdown()
{
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_slider_create(control *c)
{
	//No details are required at the moment for this data type
	controltype_slider_details *details = new controltype_slider_details;
	
	//Set all default values
	details->value = 0.5;
	details->dragging = false;

	details->reversed = false;
	details->vertical = false;
	details->appearance = slider_appearance_fillbar;

	details->track_needsupdate = true;
	details->knob_needsupdate = true;

	details->broadcast_value_minimum = 0;
	details->broadcast_value_maximum = 100;

	details->ignore_killfocus = false;
	details->focus_lost = false;

	//Nullify all agents
	for (int i = 0; i < CONTROLTYPE_SLIDER_AGENTCOUNT; i++) details->agents[i] = NULL;
	
	//Set the control details
	c->controldetails = (void *) details;

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_slider_destroy(control *c)
{
	//Get the details
	controltype_slider_details *details = (controltype_slider_details *) c->controldetails;

	//Destroy the agents
	for (int i = 0; i < CONTROLTYPE_SLIDER_AGENTCOUNT; i++) agent_destroy(&details->agents[i]);

	//Destroy the window
	window_destroy(&c->windowptr);

	//No details are required at the moment for this data type
	delete (controltype_slider_details *) c->controldetails;

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT controltype_slider_event(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	controltype_slider_details *details = (controltype_slider_details *) c->controldetails;
	int mouse_x, mouse_y;

	switch (msg)
	{
		case WM_LBUTTONDOWN:
			if (details->agents[CONTROLTYPE_SLIDER_VALUE] != NULL && details->agents[CONTROLTYPE_SLIDER_VALUE]->writable == false) break;			

			if (details->dragging != true)
			{
				//Notify the OnGrab agent of this event
				agent_notify(details->agents[CONTROLTYPE_SLIDER_AGENT_ONGRAB], NOTIFY_CHANGE, NULL);
				
				//Set focus back to this window in case we lost it
				if (details->focus_lost == true) SetActiveWindow(c->windowptr->hwnd);
				
				//We're now dragging
				details->dragging = true;
				
				//Do the mouse event				
				mouse_x = (short)LOWORD(lParam);
				mouse_y = (short)HIWORD(lParam);
				controltype_slider_onmouse(c, details, mouse_x, mouse_y);

				//Redraw the window
				style_draw_invalidate(c);
			}
			break;
	
		case WM_MOUSEMOVE:
			if (!details->dragging) break;
			if (details->agents[CONTROLTYPE_SLIDER_VALUE] != NULL && details->agents[CONTROLTYPE_SLIDER_VALUE]->writable == false) break;
			style_draw_invalidate(c);

			mouse_x = (short)LOWORD(lParam);
			mouse_y = (short)HIWORD(lParam);
			controltype_slider_onmouse(c, details, mouse_x, mouse_y);
			break;

		case WM_ACTIVATEAPP:
			//If another app has gained focus, we won't get the lbuttonup message
			if (wParam != FALSE) break; //If it is being ACTIVATED, rather then deactivated, stop processing
		case WM_LBUTTONUP:
			if (details->dragging != false)
			{
				//We're now not dragging anymore
				details->dragging = false;

				//Notify the OnRelease agent of this event
				agent_notify(details->agents[CONTROLTYPE_SLIDER_AGENT_ONRELEASE], NOTIFY_CHANGE, NULL);
			}

			break;

		case WM_PAINT:
			styledrawinfo d;

			//Begin drawing
			if (style_draw_begin(c, d))
			{
				//Paint background according to the current style on the bitmap ...
				if(c->windowptr->has_custom_style){
					style_draw_box(
						d.rect,
						d.buffer,
						c->windowptr->styleptr,
						c->windowptr->is_bordered
						);
				}else{
					
					style_draw_box(
						d.rect,
						d.buffer,
						c->windowptr->style,
						c->windowptr->is_bordered
						);
				}
				//Update components if necessary
				if (details->track_needsupdate) controltype_slider_updatetrack(c, details, d.rect);
				if (details->knob_needsupdate) controltype_slider_updateknob(c, details, d.rect);
				
				if (c->windowptr->has_custom_style && c->windowptr->sstyleptr->draw_inner){
					if(c->windowptr->sstyleptr->in_has_custom_style)
						style_draw_box(details->track, d.buffer, c->windowptr->sstyleptr->in_styleptr, false);
					else
						style_draw_box(details->track, d.buffer, c->windowptr->sstyleptr->in_style, false);
				}else if (!c->windowptr->has_custom_style && STYLETYPE_TOOLBAR == c->windowptr->style)
					style_draw_box(details->track, d.buffer, STYLETYPE_INSET, false);
				if (c->windowptr->has_custom_style){
					if(c->windowptr->sstyleptr->has_custom_style)
						style_draw_box(details->knob, d.buffer, c->windowptr->sstyleptr->styleptr, c->windowptr->is_bordered);
					else
						style_draw_box(details->knob, d.buffer, c->windowptr->sstyleptr->style, c->windowptr->is_bordered);
				}
				else		
					style_draw_box(details->knob, d.buffer, STYLETYPE_TOOLBAR, c->windowptr->is_bordered);
			}
			//End drawing
			style_draw_end(d);
			break;
	}
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_slider_notify(control *c, int notifytype, void *messagedata)
{
	//Variables
	controltype_slider_details *details;
	double *newvalueptr;

	//Get the details
	details = (controltype_slider_details *) c->controldetails;

	switch (notifytype)
	{
		case NOTIFY_RESIZE:
			//On resize, let the control know
			details->track_needsupdate = true;
			details->knob_needsupdate = true;
			break;

		case NOTIFY_NEEDUPDATE:         
			//When we are told we need an update, figure out what to do
			//If we're dragging, forget updates
			if (details->dragging) break;       

			//Get the value
			newvalueptr = (double *)(agent_getdata(details->agents[CONTROLTYPE_SLIDER_VALUE], DATAFETCH_VALUE_SCALE));
			if (newvalueptr)
			{           
				//Record the old value
				double oldvalue = details->value;

				//Get the new value
				details->value = *newvalueptr;

				//If it has changed
				if (details->value != oldvalue)
				{
					//Notify the OnChange agent to do what it needs to do
					agent_notify(details->agents[CONTROLTYPE_SLIDER_AGENT_ONCHANGE], NOTIFY_CHANGE, NULL);

					//We need to update the knob
					details->knob_needsupdate = true;
				}
			}

			//Tell the window to draw itself again          
			style_draw_invalidate(c);
			break;

		case NOTIFY_SAVE_CONTROL:
			//Save the details of this particular slider
			config_write(config_get_control_setcontrolprop_d(c, "Value", &details->value));
			config_write(config_get_control_setcontrolprop_b(c, "Vertical", &details->vertical));
			config_write(config_get_control_setcontrolprop_b(c, "Reversed", &details->reversed));
			config_write(config_get_control_setcontrolprop_i(c, "Appearance", &details->appearance));
			config_write(config_get_control_setcontrolprop_i(c, "BroadcastValueMinimum", &details->broadcast_value_minimum));
			config_write(config_get_control_setcontrolprop_i(c, "BroadcastValueMaximum", &details->broadcast_value_maximum));

			//Save the agent details
			for (int i = 0; i < CONTROLTYPE_SLIDER_AGENTCOUNT; i++)	agent_notify(details->agents[i], NOTIFY_SAVE_AGENT, NULL);

			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int controltype_slider_message(control *c, int tokencount, char *tokens[])
{
	//Get the details
	controltype_slider_details *details = (controltype_slider_details *) c->controldetails;

	//If it's set control property
	if (tokencount == 6 && !stricmp(tokens[2], szBActionSetControlProperty))
	{
		//Check all settable values
		if (
			   (!stricmp(tokens[4], "Value") && config_set_double_expr(tokens[5], &details->value, 0.0, 1.0))
			|| (!stricmp(tokens[4], "Reversed") && config_set_bool(tokens[5], &details->reversed))
			|| (!stricmp(tokens[4], "Vertical") && config_set_bool(tokens[5], &details->vertical))
			|| (!stricmp(tokens[4], "Appearance") && config_set_int(tokens[5], &details->appearance, 0, 2))
			|| (!stricmp(tokens[4], "BroadcastValueMinimum") && config_set_int(tokens[5], &details->broadcast_value_minimum, -32767, 32767))
			|| (!stricmp(tokens[4], "BroadcastValueMaximum") && config_set_int(tokens[5], &details->broadcast_value_maximum, -32767, 32767))
			)
		{
			details->track_needsupdate = true;
			details->knob_needsupdate = true;
			controltype_slider_notify(c, NOTIFY_NEEDUPDATE, NULL);
			style_draw_invalidate(c);
			return 0;
		}

		//Must be an error
		return 1;
	}

	//Must be an agent message
	return agent_controlmessage(c, tokencount, tokens, CONTROLTYPE_SLIDER_AGENTCOUNT, details->agents, controltype_slider_agentnames, controltype_slider_agenttypes);

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *controltype_slider_getdata(control *c, int datatype)
{
	//Get the details
	controltype_slider_details *details = (controltype_slider_details *) c->controldetails;

	switch (datatype)
	{
		case DATAFETCH_VALUE_SCALE:
			return &details->value;
		case DATAFETCH_INT_DEFAULTHEIGHT:
			return (void *) &controltype_slider_data_defaultheight;
		case DATAFETCH_INT_DEFAULTWIDTH:
			return (void *) &controltype_slider_data_defaultwidth;
		case DATAFETCH_INT_MIN_WIDTH:
			return (void *) &controltype_slider_data_minimumwidth;
		case DATAFETCH_INT_MIN_HEIGHT:
			return (void *) &controltype_slider_data_minimumheight;
		case DATAFETCH_INT_MAX_WIDTH:
			return (void *) &controltype_slider_data_maximumwidth;
		case DATAFETCH_INT_MAX_HEIGHT:
			return (void *) &controltype_slider_data_maximumheight;
	}
	
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_getstringdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool controltype_slider_getstringdata(control *c, char *buffer, char *propertyname)
{
	controltype_slider_details *details = (controltype_slider_details *) c->controldetails;
	if (!stricmp(propertyname,"Value"))
	{
		sprintf(buffer, "%lf", details->value);
		return true;
	}
	//Broadcast value is a special text value that ALL controls should be able to broadcast
	//It contains a certain "value" that can be associated with a control.  Various things
	//such as broams can reference it.
	//For sliders, this value is the adjusted value using the min and max.
	else if (!stricmp(propertyname,"BroadcastValue"))
	{
		long broadcastvalue = (long) (details->value * (((long) details->broadcast_value_maximum) - ((long) details->broadcast_value_minimum)) + details->broadcast_value_minimum);
		sprintf(buffer, "%d", broadcastvalue);
	}

	//No data to return - return false
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_slider_menu_context(Menu *m, control *c)
{
	//Variables
	Menu *submenu1;
	bool temp;

	//Get the details
	controltype_slider_details *details = (controltype_slider_details *) c->controldetails;

	//Show the menu
	menu_controloptions(m, c, CONTROLTYPE_SLIDER_AGENTCOUNT, details->agents, "", controltype_slider_agentnames, controltype_slider_agenttypes);

	make_menuitem_nop(m, "");

	temp = !details->reversed; make_menuitem_bol(m, "Values Reversed", config_getfull_control_setcontrolprop_b(c, "Reversed", &temp), !temp);

	make_menuitem_nop(m, "");

	make_menuitem_int(m, "Broadcast Value Minimum", config_getfull_control_setcontrolprop_s(c, "BroadcastValueMinimum"), details->broadcast_value_minimum, -32767, 32767);
	make_menuitem_int(m, "Broadcast Value Maximum", config_getfull_control_setcontrolprop_s(c, "BroadcastValueMaximum"), details->broadcast_value_maximum, -32767, 32767);	

	make_menuitem_nop(m, "");

	submenu1 = make_menu("Appearance", c);
	temp = false; make_menuitem_bol(submenu1, "Horizontal", config_getfull_control_setcontrolprop_b(c, "Vertical", &temp), !details->vertical);
	temp = true; make_menuitem_bol(submenu1, "Vertical", config_getfull_control_setcontrolprop_b(c, "Vertical", &temp), details->vertical);
	
	make_menuitem_nop(submenu1, "");

	make_menuitem_bol(submenu1, "Fill Bar", config_getfull_control_setcontrolprop_i(c, "Appearance", &slider_appearance_fillbar), details->appearance == slider_appearance_fillbar);
	make_menuitem_bol(submenu1, "Scroll Bar", config_getfull_control_setcontrolprop_i(c, "Appearance", &slider_appearance_scrollbar), details->appearance == slider_appearance_scrollbar);
	make_menuitem_bol(submenu1, "Track and Knob", config_getfull_control_setcontrolprop_i(c, "Appearance", &slider_appearance_trackknob), details->appearance == slider_appearance_trackknob);
	
	make_submenu_item(m, "Appearance", submenu1);

	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_slider_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controltype_slider_notifytype(int notifytype, void *messagedata)
{
	
}

//##################################################
//controltype_slider_updatetrack
//##################################################
void controltype_slider_updatetrack(control *c, controltype_slider_details *details, RECT &windowrect)
{
	details->track.left = windowrect.left + SLIDER_OUTERBORDERMARGIN;
	details->track.right = windowrect.right - SLIDER_OUTERBORDERMARGIN;
	details->track.bottom = windowrect.bottom - SLIDER_OUTERBORDERMARGIN;
	details->track.top = windowrect.top + SLIDER_OUTERBORDERMARGIN;
	if (details->vertical)
	{
		details->track_length = (windowrect.bottom - windowrect.top) - 2*SLIDER_OUTERBORDERMARGIN;
		details->knob_maxlength = windowrect.bottom - windowrect.top - 2*SLIDER_INNERBORDERMARGIN;
	}
	else
	{
		details->track_length = (windowrect.right - windowrect.left) - 2*SLIDER_OUTERBORDERMARGIN;
		details->knob_maxlength = windowrect.right - windowrect.left - 2*SLIDER_INNERBORDERMARGIN;
	}

	details->track_clickable = details->track;
	details->track_clickable_length = details->track_length;
	details->track_countable = details->track;
	details->track_countable_length = details->track_length;

	if (details->appearance >= slider_appearance_scrollbar)
	{
		if (details->appearance == slider_appearance_scrollbar)
		{
			details->knob_length = details->track_length / 20;
			if (details->knob_length < 4) details->knob_length = 4;
		}       
		else
		{
			details->knob_length = 8;
			if (details->knob_length > details->track_length) details->knob_length = 4;         
		}

		int halflength = details->knob_length / 2;
		if (details->vertical)
		{
			details->track_countable.top += halflength;
			details->track_countable.bottom -= halflength;
		}
		else
		{
			details->track_countable.left += halflength;
			details->track_countable.right -= halflength;
		}
		details->track_countable_length -= 2*halflength;

		if (details->appearance == slider_appearance_trackknob)
		{
			//Make the track half it's girth
			int partgirth;
			if (details->vertical)
			{
				partgirth = (windowrect.right - windowrect.left) / 6;
				if (partgirth > 1)
				{
					details->track.left += partgirth;
					details->track.right -= partgirth;
				}
			}
			else
			{
				partgirth = (windowrect.bottom - windowrect.top) / 6;
				if (partgirth > 1)
				{
					details->track.top += partgirth;
					details->track.bottom -= partgirth;
				}
			}
		}
	}

	details->knob_needsupdate = true;
	details->track_needsupdate = false;
}

//##################################################
//controltype_slider_updateknob
//##################################################
void controltype_slider_updateknob(control *c, controltype_slider_details *details, RECT &windowrect)
{
	double realvalue = details->value;
	if (details->reversed) realvalue = 1.0 - realvalue;

	if (details->appearance == slider_appearance_fillbar)
	{
		details->knob_length = (int) (details->knob_maxlength * realvalue + 0.49);
	
		if (details->vertical)
		{
			details->knob.left = windowrect.left + SLIDER_INNERBORDERMARGIN;
			details->knob.right = windowrect.right - SLIDER_INNERBORDERMARGIN;
			details->knob.bottom = windowrect.bottom - SLIDER_INNERBORDERMARGIN;
			details->knob.top = details->knob.bottom - details->knob_length;
		}
		else
		{
			details->knob.left = windowrect.left + SLIDER_INNERBORDERMARGIN;
			details->knob.right = details->knob.left + details->knob_length;
			details->knob.bottom = windowrect.bottom - SLIDER_INNERBORDERMARGIN;
			details->knob.top = windowrect.top + SLIDER_INNERBORDERMARGIN;
		}
	}
	else if (details->appearance >= slider_appearance_scrollbar)
	{
		int adjusted_track_length = details->knob_maxlength - details->knob_length + 2;
		int offset = (int)(realvalue * adjusted_track_length) - 1;

		if (details->vertical)
		{
			details->knob.left = windowrect.left + SLIDER_OUTERBORDERMARGIN;
			details->knob.right = windowrect.right - SLIDER_OUTERBORDERMARGIN;
			details->knob.bottom = windowrect.bottom - SLIDER_INNERBORDERMARGIN - offset;
			details->knob.top = details->knob.bottom - details->knob_length;
		}
		else
		{
			details->knob.left = windowrect.left + SLIDER_INNERBORDERMARGIN + offset;
			details->knob.right = details->knob.left + details->knob_length;
			details->knob.bottom = windowrect.bottom - SLIDER_OUTERBORDERMARGIN;
			details->knob.top = windowrect.top + SLIDER_OUTERBORDERMARGIN;
		}
	}
	
	details->knob_needsupdate = false;
}

//##################################################
//controltype_slider_onmouse
//##################################################
void controltype_slider_onmouse(control *c, controltype_slider_details *details, int mouse_x, int mouse_y)
{
	//Chose to ignore it if out of track
	const int d = 20; //gr

	//Record the previous value
	double oldvalue = details->value;

	if (mouse_x < details->track_clickable.left-d
		|| mouse_x >= details->track_clickable.right+d
		|| mouse_y < details->track_clickable.top-d
		|| mouse_y >= details->track.bottom+d)
		return;

	//Figure out what value was clicked
	if (details->vertical) details->value = (double) (-(mouse_y - details->track_countable.bottom)) / (double) (details->track_countable_length);
	else details->value = (double) (mouse_x - details->track_countable.left) / (double) (details->track_countable_length);

	if (details->reversed) details->value = 1.0 - details->value;

	if (details->value < 0.0) details->value = 0.0;
	if (details->value > 1.0) details->value = 1.0;
		
	//We'll need to update the knob and offset now
	details->knob_needsupdate = true;

	//Notify the agent that a value change event has occurred
	agent_notify(details->agents[CONTROLTYPE_SLIDER_VALUE], NOTIFY_CHANGE, (void *) &details->value);

	//Notify the OnChange agent to do what it needs to do
	if (details->value != oldvalue)
	{
		agent_notify(details->agents[CONTROLTYPE_SLIDER_AGENT_ONCHANGE], NOTIFY_CHANGE, NULL);
	}
}
