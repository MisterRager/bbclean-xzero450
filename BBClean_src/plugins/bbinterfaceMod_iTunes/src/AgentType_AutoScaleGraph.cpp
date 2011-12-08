/*===================================================

	AGENTTYPE_GRAPH CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
//Parent Include
#include "AgentType_AutoScaleGraph.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "StyleMaster.h"
#include "DialogMaster.h"
#include "MessageMaster.h"
#include "MenuMaster.h"
#include "ColorMaster.h"

//Local variables
const int agenttype_autoscalegraph_subagentcount = AGENTTYPE_AUTOSCALEGRAPH_AGENTCOUNT;
#define AGENTTYPE_AUTOSCALEGRAPH_AGENT_VALUE 0
char *agenttype_autoscalegraph_agentdescriptions[AGENTTYPE_AUTOSCALEGRAPH_AGENTCOUNT] =
{
	"Value"
};

const int agenttype_autoscalegraph_agenttypes[AGENTTYPE_AUTOSCALEGRAPH_AGENTCOUNT] =
{
	CONTROL_FORMAT_DOUBLE
};

//Timer related stuff
VOID CALLBACK agenttype_autoscalegraph_timerproc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
int agenttype_autoscalegraph_counter = 0;
UINT_PTR agenttype_autoscalegraph_timerid;
bool agenttype_autoscalegraph_hastimer = false;
list *agenttype_autoscalegraph_agents;

double get_range_value(double value);
double ReadValueFromString(char * string);

#define AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPECOUNT 2
#define AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPE_FILL 0
#define AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPE_LINE 1
char *agenttype_autoscalegraph_charttypes[AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPECOUNT] =
{
	"FILLCHART",
	"LINECHART",
};

char *agenttype_autoscalegraph_friendlycharttypes[AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPECOUNT] =
{
	"Filled Chart",
	"Line Chart",
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_autoscalegraph_startup()
{
	//Create the list
	agenttype_autoscalegraph_agents = list_create();

	//Register this type with the ControlMaster
	agent_registertype(
		"AutoScaleGraph",                          //Friendly name of agent type
		"AutoScaleGraph",                          //Name of agent type
		CONTROL_FORMAT_IMAGE,               //Control type
		true,
		&agenttype_autoscalegraph_create,          
		&agenttype_autoscalegraph_destroy,
		&agenttype_autoscalegraph_message,
		&agenttype_autoscalegraph_notify,
		&agenttype_autoscalegraph_getdata,
		&agenttype_autoscalegraph_menu_set,
		&agenttype_autoscalegraph_menu_context,
		&agenttype_autoscalegraph_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_autoscalegraph_shutdown()
{
	if(agenttype_autoscalegraph_hastimer){
		agenttype_autoscalegraph_hastimer = false;
		KillTimer(NULL, agenttype_autoscalegraph_timerid);
	}
	//Destroy the internal tracking list
	if (agenttype_autoscalegraph_agents) list_destroy(agenttype_autoscalegraph_agents);
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_autoscalegraph_create(agent *a, char *parameterstring)
{
	//Get the chart type
	int charttype = -1;
	for (int  i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPECOUNT; i++)
	{
		if (!stricmp(parameterstring, agenttype_autoscalegraph_charttypes[i])) charttype = i; 
	}
	if (charttype == -1) return 1;

	//Create the details
	agenttype_autoscalegraph_details *details = new agenttype_autoscalegraph_details;
	a->agentdetails = details;
	details->charttype = charttype;
	details->range = details->user_range = 1.0;
	details->rangecount=0;
	details->use_custom_color = details->use_user_range = false;
	details->chartcolor = style_get_text_color(STYLETYPE_TOOLBAR);

	//Create a unique string to assign to this (just a number from a counter)
	char identifierstring[64];
	sprintf(identifierstring, "%ul", agenttype_autoscalegraph_counter);
	details->internal_identifier = new_string(identifierstring);

	//Nullify all agents
	for (int i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_AGENTCOUNT; i++) details->agents[i] = NULL;

	//Reset the history
	for (int i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_HISTORYLENGTH; i++) details->valuehistory[i] = 0.0;
	details->historyindex = 0;

	//Increment the counter
	agenttype_autoscalegraph_counter++;

	//Add this to our internal tracking list
	agent *oldagent; //Unused, but we have to pass it
	list_add(agenttype_autoscalegraph_agents, details->internal_identifier, (void *) a, (void **) &oldagent);

	if (!agenttype_autoscalegraph_hastimer)
	{
		agenttype_autoscalegraph_timerid = SetTimer(NULL, 0, 1000, agenttype_autoscalegraph_timerproc);
		agenttype_autoscalegraph_hastimer = true;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_autoscalegraph_destroy(agent *a)
{
	//Delete the details if possible
	if (a->agentdetails)
	{
		agenttype_autoscalegraph_details *details =(agenttype_autoscalegraph_details *) a->agentdetails;

		//Delete all subagents
		for (int i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_AGENTCOUNT; i++) agent_destroy(&details->agents[i]);		

		//Remove from tracking list
		if (details->internal_identifier != NULL)
		{
			list_remove(agenttype_autoscalegraph_agents, details->internal_identifier);
			free_string(&details->internal_identifier);
		}

		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_autoscalegraph_message(agent *a, int tokencount, char *tokens[])
{
	agenttype_autoscalegraph_details *details = (agenttype_autoscalegraph_details *) a->agentdetails;
	if (!stricmp("AutoScaleGraphType", tokens[5]))
	{
		int charttype = -1;
		for (int  i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPECOUNT; i++)
		{
			if (!stricmp(tokens[6], agenttype_autoscalegraph_charttypes[i])) charttype = i; 
		}
		if (charttype == -1) return 1;
		details->charttype = charttype;
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}
	else if (!stricmp("CustomChartColor", tokens[5]) && config_set_bool(tokens[6],&(details->use_custom_color)))
	{
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}
	else if (!stricmp("ChartColor",tokens[5])){
		details->use_custom_color = true;
		COLORREF colorval;
		if((colorval = ReadColorFromString(tokens[6])) != -1){
			details->chartcolor = colorval;
			control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		}
		return 0;
	}
	else if(!stricmp("AutoScale",tokens[5]) && config_set_bool(tokens[6],&(details->use_user_range)))
	{
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}
	else if(!stricmp("MaxRange",tokens[5]))
	{
		double temp = ReadValueFromString(tokens[6]);
		if(temp <= 0) return 1;
		details->use_user_range = true;
		details->user_range = temp;
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_autoscalegraph_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_autoscalegraph_details *details = (agenttype_autoscalegraph_details *) a->agentdetails;
	styledrawinfo *di;
	HPEN hpen;
	HGDIOBJ oldobj;
	int offset_left;
	int width;
	int offset_top;
	int offset_bottom;
	int height;
	int currenty;
	double *currentvalue;
	double finalvalue;
	COLORREF cr;
	char color[8];
	
	switch(notifytype)
	{
		case NOTIFY_TIMER:
			
			//Get the current value
			currentvalue = (double *) agent_getdata(details->agents[AGENTTYPE_AUTOSCALEGRAPH_AGENT_VALUE], DATAFETCH_VALUE_SCALE);
			finalvalue = *currentvalue;
			if(finalvalue > details->range){
				details->rangecount = 0;
				details->range = get_range_value(finalvalue);
			}else{
				details->rangecount++;
			}
			if(details->rangecount==180){
				details->range=finalvalue;
				int currenthistoryindex = details->historyindex;
				int maxi = 180;
				int ci;
				for (ci = 179; ci > 0; ci--){
					if(details->valuehistory[currenthistoryindex] > details->range){
						details->range=details->valuehistory[currenthistoryindex];
						maxi=ci;
					}
					currenthistoryindex--;
					if (currenthistoryindex < 0) currenthistoryindex = AGENTTYPE_AUTOSCALEGRAPH_HISTORYLENGTH - 1;
				}
				details->rangecount = 180-ci;
				details->range = get_range_value(details->range);
			}
			//Store this value in the value history			
			details->historyindex++;
			if (details->historyindex == AGENTTYPE_AUTOSCALEGRAPH_HISTORYLENGTH) details->historyindex = 0;
			details->valuehistory[details->historyindex] = finalvalue;

			//Notify the control it is time to redraw
			control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);

			break;

		case NOTIFY_DRAW:

			//Get set up for drawing the graph
			di = (styledrawinfo *) messagedata;
			cr = details->use_custom_color ? details->chartcolor : style_get_text_color(STYLETYPE_TOOLBAR);
			hpen = CreatePen(PS_SOLID, 1, cr);
			oldobj = SelectObject(di->buffer, hpen);

			//Prepare some variables for convenience
			offset_left = di->rect.left + 2;
			width = di->rect.right - di->rect.left - 4;
			offset_top = di->rect.top + 2;
			offset_bottom = di->rect.bottom - 2;
			height = di->rect.bottom - di->rect.top - 4;

			//Make sure we have a large enough area
			if (width > 2 && height > 2)
			{
				//Draw the graph
				int currenthistoryindex = details->historyindex;
				double real_value;
				double range = details->use_user_range ? details->user_range : details->range;
				if (details->charttype == AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPE_FILL)
				{
					for (int currentx = width; currentx >= offset_left; currentx--)
					{
						real_value = details->valuehistory[currenthistoryindex]/range;
						if(real_value>1.0) real_value=1.0;
						currenty = offset_bottom - (height*real_value);
						MoveToEx(di->buffer, currentx, offset_bottom, NULL);
						LineTo(di->buffer, currentx, currenty);
						currenthistoryindex--;
						if (currenthistoryindex < 0) currenthistoryindex = AGENTTYPE_AUTOSCALEGRAPH_HISTORYLENGTH - 1;
					}
				}
				else if (details->charttype == AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPE_LINE)
				{
					real_value = details->valuehistory[currenthistoryindex]/range;
					if(real_value>1.0) real_value=1.0;
					currenty = offset_bottom - (height*real_value);
					for (int currentx = width; currentx >= offset_left; currentx--)
					{	
						real_value = details->valuehistory[currenthistoryindex]/range;
						if(real_value>1.0) real_value=1.0;
						MoveToEx(di->buffer, currentx, currenty, NULL);
						currenty = offset_bottom - (height*real_value);
						LineTo(di->buffer, currentx-1, currenty);
						currenthistoryindex--;
						if (currenthistoryindex < 0) currenthistoryindex = AGENTTYPE_AUTOSCALEGRAPH_HISTORYLENGTH - 1;
					}
				}
			}

			//Cleanup
			SelectObject(di->buffer, oldobj);
			DeleteObject(hpen);
		
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, agenttype_autoscalegraph_charttypes[details->charttype]));
			if(details->use_user_range){
				char range[20];
				_snprintf(range,20,"%.0f",details->user_range);
				config_write(config_getfull_control_setagentprop_c(a->controlptr,a->agentaction,"MaxRange",range));
			}
			if(details->use_custom_color){
				sprintf(color,"#%06X",switch_rgb(details->chartcolor));
				config_write(config_getfull_control_setagentprop_c(a->controlptr,a->agentaction,"ChartColor",color));
			}
			//Save all child agents, if necessary
			for (int i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_AGENTCOUNT; i++) agent_notify(details->agents[i], NOTIFY_SAVE_AGENT, NULL);

			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_autoscalegraph_getdata(agent *a, int datatype)
{
	agenttype_autoscalegraph_details *details = (agenttype_autoscalegraph_details *) a->agentdetails;

	switch (datatype)
	{
		case DATAFETCH_SUBAGENTS_POINTERS_ARRAY:
			return details->agents;
		case DATAFETCH_SUBAGENTS_NAMES_ARRAY:
			return agenttype_autoscalegraph_agentdescriptions;
		case DATAFETCH_SUBAGENTS_TYPES_ARRAY:
			return (void *) agenttype_autoscalegraph_agenttypes;
		case DATAFETCH_SUBAGENTS_COUNT:
			return (void *) &agenttype_autoscalegraph_subagentcount;
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_autoscalegraph_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	for (int i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPECOUNT; i++)
	{
		make_menuitem_cmd(m, agenttype_autoscalegraph_friendlycharttypes[i], config_getfull_control_setagent_c(c, action, "AutoScaleGraph", agenttype_autoscalegraph_charttypes[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_autoscalegraph_menu_context(Menu *m, agent *a)
{
	Menu *submenu;
	agenttype_autoscalegraph_details *details = (agenttype_autoscalegraph_details *) a->agentdetails;

	for (int i = 0; i < AGENTTYPE_AUTOSCALEGRAPH_CHARTTYPECOUNT; i++)
	{
		make_menuitem_bol(m, agenttype_autoscalegraph_friendlycharttypes[i], config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "AutoScaleGraphType", agenttype_autoscalegraph_charttypes[i]), details->charttype == i);
	}
	make_menuitem_nop(m, NULL);

	char namedot[1000];
	sprintf(namedot, "%s%s", a->agentaction, ".");

	menu_controloptions(m, a->controlptr, AGENTTYPE_AUTOSCALEGRAPH_AGENTCOUNT, details->agents, namedot, agenttype_autoscalegraph_agentdescriptions, agenttype_autoscalegraph_agenttypes);

	make_menuitem_nop(m,NULL);
	submenu = make_menu("Auto Scale",a->controlptr);
	bool temp = !details->use_user_range;
	make_menuitem_bol(submenu,"Enable Auto Scale",config_getfull_control_setagentprop_b(a->controlptr,a->agentaction,"AutoScale",&temp),temp);
	if(!temp){
		char range[20];
		_snprintf(range,20,"%.0f",details->user_range);
		make_menuitem_str(submenu,"Max Range",config_getfull_control_setagentprop_s(a->controlptr,a->agentaction,"MaxRange"),range);
	}
	make_submenu_item(m,"Auto Scale",submenu);
	char color[8];
	sprintf(color,"#%06X",switch_rgb(details->chartcolor));
	temp = !(details->use_custom_color);
	make_menuitem_bol(m,"Custom Chart Color",config_getfull_control_setagentprop_b(a->controlptr,a->agentaction,"CustomChartColor",&temp),!temp);
	if(!temp){
		make_menuitem_str(
			m,
			"Chart Color",
			config_getfull_control_setagentprop_s(a->controlptr,a->agentaction,"ChartColor"),
			color
		);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_autoscalegraph_notifytype(int notifytype, void *messagedata)
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_autoscalegraph_timerproc
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void CALLBACK agenttype_autoscalegraph_timerproc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//If we have any agents
	if (agenttype_autoscalegraph_agents->first != NULL)
	{
		//Notify all agents it is time to update
		listnode *currentnode;
		dolist(currentnode, agenttype_autoscalegraph_agents)
		{
			agent_notify((agent *) currentnode->value, NOTIFY_TIMER, NULL);
		}
	}
	else
	{
		//If there are no more agents left, kill the timer
		agenttype_autoscalegraph_hastimer = false;
		KillTimer(NULL, agenttype_autoscalegraph_timerid);
	}
}

double get_range_value(double value){
	int i;
	for(i=0;value>2;i++){
		value/=2;
	}
	i++;
	return (double)(1<<i);
}

/*=================================================*/

double ReadValueFromString(char * string){
	double val=0;
	if(string == NULL) return -1.0;
	char *s = strlwr(string);
	while(*s == ' ') s++;
	char *p;
	for(p=s;*p!='\0';p++){
		if (isdigit(*p)) val = val * 10 + (*p) - '0';
		else if(*p == 'k') { val = val * 1024; break;}
		else if(*p == 'm') { val = val * 1024 * 1024; break;}
		else if(*p == 'g') { val = val * 1024 * 1024 * 1024; break;}
		else if(*p == 't') { val = val * 1024 * 1024 * 1024 * 1024; break;}
		else if(*p == '.') break;
		else return -1.0;
	}
	return val;
}


