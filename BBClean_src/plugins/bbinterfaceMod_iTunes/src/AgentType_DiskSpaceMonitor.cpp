/*===================================================

	AGENTTYPE_DISKSPACEMONITOR CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
#include <windows.h>
#include <shlwapi.h>
#include <math.h>
//Parent Include
#include "AgentType_DiskSpaceMonitor.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MessageMaster.h"
#include "ListMaster.h"



//Declare the function prototypes;
VOID CALLBACK agenttype_diskspacemonitor_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK agenttype_diskspacemonitor_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void agenttype_diskspacemonitor_propogatenewvalues();
void agenttype_diskspacemonitor_updatevalue(agenttype_diskspacemonitor_details *d);

// Some windowing variables
HWND agenttype_diskspacemonitor_window;
bool agenttype_diskspacemonitor_windowclassregistered;
HMODULE agenttype_diskspacemonitor_ntdllmodule;

//Local primitives
unsigned long agenttype_diskspacemonitor_counter;
const char agenttype_diskspacemonitor_timerclass[] = "BBInterfaceAgenDiskSpaceMon";

//A list of this type of agent
list *agenttype_diskspacemonitor_agents;
bool agenttype_diskspacemonitor_hastimer = false;

//Declare the number of types
#define DISKSPACEMONITOR_NUMTYPES 5

//Array of string types - must have DISKSPACEMONITOR_NUMTYPES entries
enum DISKSPACEMONITOR_TYPE
{
	DISKSPACEMONITOR_TYPE_NONE = 0,
	DISKSPACEMONITOR_TYPE_DISKFREEVAL,
	DISKSPACEMONITOR_TYPE_DISKFREEPCT,
	DISKSPACEMONITOR_TYPE_DISKUSEVAL,
	DISKSPACEMONITOR_TYPE_DISKUSEPCT	
};

//Must match the enum ordering above! Must have DISKSPACEMONITOR_NUMTYPES entries
const char *agenttype_diskspacemonitor_types[] =
{
	"None", // Unused
	"DiskFree",
	"DiskFree(Pct.)",
	"DiskUse",
	"DiskUse(Pct.)"
};

//Must match the enum ordering above! Must have DISKSPACEMONITOR_NUMTYPES entries
const char *agenttype_diskspacemonitor_friendlytypes[] =
{
	"None", // Unused
	"Disk Free",
	"Disk Free(Percentage)",
	"Disk Used",
	"Disk Used(Percentage)"
};


//Must be the same size as the above array and enum


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_diskspacemonitor_startup()
{
	//Create the list
	agenttype_diskspacemonitor_agents = list_create();

	//Register the window class
	agenttype_diskspacemonitor_windowclassregistered = false;
	if (window_helper_register(agenttype_diskspacemonitor_timerclass, &agenttype_diskspacemonitor_event))
	{
		//Couldn't register the window
		BBMessageBox(NULL, "failed on register class", "test", MB_OK);
		return 1;
	}
	agenttype_diskspacemonitor_windowclassregistered = true;

	//Create the window
	agenttype_diskspacemonitor_window = window_helper_create(agenttype_diskspacemonitor_timerclass);
	if (!agenttype_diskspacemonitor_window)
	{
		//Couldn't create the window
		BBMessageBox(NULL, "failed on window", "test", MB_OK);
		return 1;
	}

	//If we got this far, we can successfully use this function
	//Register this type with the AgentMaster
	agent_registertype(
		"DiskSpace Monitor",                   //Friendly name of agent type
		"DiskSpaceMonitor",                    //Name of agent type
		CONTROL_FORMAT_SCALE|CONTROL_FORMAT_TEXT,				//Control type
		false,
		&agenttype_diskspacemonitor_create,
		&agenttype_diskspacemonitor_destroy,
		&agenttype_diskspacemonitor_message,
		&agenttype_diskspacemonitor_notify,
		&agenttype_diskspacemonitor_getdata,
		&agenttype_diskspacemonitor_menu_set,
		&agenttype_diskspacemonitor_menu_context,
		&agenttype_diskspacemonitor_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_diskspacemonitor_shutdown()
{
	if(agenttype_diskspacemonitor_hastimer){
		agenttype_diskspacemonitor_hastimer = false;
		KillTimer(agenttype_diskspacemonitor_window, 0);
	}
	//Destroy the internal tracking list
	if (agenttype_diskspacemonitor_agents) list_destroy(agenttype_diskspacemonitor_agents);
	//Destroy the window
	if (agenttype_diskspacemonitor_window) window_helper_destroy(agenttype_diskspacemonitor_window);

	//Unregister the window class
	if (agenttype_diskspacemonitor_windowclassregistered) window_helper_unregister(agenttype_diskspacemonitor_timerclass);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_diskspacemonitor_create(agent *a, char *parameterstring)
{


	if (0 == * parameterstring)
		return 2; // no param, no agent

	//Find the monitor type
	int monitor_type = DISKSPACEMONITOR_TYPE_NONE;
	for (int i = 1; i < DISKSPACEMONITOR_NUMTYPES; i++)
	{
		if (stricmp(agenttype_diskspacemonitor_types[i], parameterstring) == 0)
		{
			monitor_type = i;
			break;
		}
	}

	//If we didn't find a correct monitor type
	if (monitor_type == DISKSPACEMONITOR_TYPE_NONE)
	{
		//On an error
		if (!plugin_suppresserrors)
		{
			char buffer[1000];
			sprintf(buffer,	"There was an error setting the Disk Space Monitor agent:\n\nType \"%s\" is not a valid type.", parameterstring);
			BBMessageBox(NULL, buffer, szAppName, MB_OK|MB_SYSTEMMODAL);
		}
		return 1;
	}

	//Is this the first?
	bool first = (agenttype_diskspacemonitor_agents->first == NULL ? true : false);

	//Create the details
	agenttype_diskspacemonitor_details *details = new agenttype_diskspacemonitor_details;
	details->monitor_type = monitor_type;
	details->value=-1.0;
	details->previous_value = 0;
	details->path=NULL;
	strcpy(details->str_value,"");
	
	//Create a unique string to assign to this (just a number from a counter)
	char identifierstring[64];
	sprintf(identifierstring, "%ul", agenttype_diskspacemonitor_counter);
	details->internal_identifier = new_string(identifierstring);

	//Set the details
	a->agentdetails = (void *)details;
	agenttype_diskspacemonitor_updatevalue(details);
	control_notify(a->controlptr,NOTIFY_NEEDUPDATE,NULL);
	
	//Add this to our internal tracking list
	agent *oldagent; //Unused, but we have to pass it
	list_add(agenttype_diskspacemonitor_agents, details->internal_identifier, (void *) a, (void **) &oldagent);

	//Increment the counter
	agenttype_diskspacemonitor_counter++;

	if (!agenttype_diskspacemonitor_hastimer)
	{
		SetTimer(agenttype_diskspacemonitor_window, 0, 10000, agenttype_diskspacemonitor_timercall);
		agenttype_diskspacemonitor_hastimer = true;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_diskspacemonitor_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_diskspacemonitor_details *details = (agenttype_diskspacemonitor_details *) a->agentdetails;

		//Delete from the tracking list
		list_remove(agenttype_diskspacemonitor_agents, details->internal_identifier);

		//Free the string
		free_string(&details->internal_identifier);

		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_diskspacemonitor_message(agent *a, int tokencount, char *tokens[])
{
	agenttype_diskspacemonitor_details *details = (agenttype_diskspacemonitor_details *) a->agentdetails;
	if (!strcmp("MonitoringPath",tokens[5]) && config_set_str(tokens[6],&details->path)){
		details->value=-1.0;
		agenttype_diskspacemonitor_updatevalue(details);
		control_notify(a->controlptr,NOTIFY_NEEDUPDATE,NULL);
	}
	
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_diskspacemonitor_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_diskspacemonitor_details *details = (agenttype_diskspacemonitor_details *) a->agentdetails;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, agenttype_diskspacemonitor_types[details->monitor_type]));
			config_write(config_get_control_setagentprop_c(a->controlptr, a->agentaction, "MonitoringPath",details->path));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_diskspacemonitor_getdata(agent *a, int datatype)
{
	//Get the agent details
	agenttype_diskspacemonitor_details *details = (agenttype_diskspacemonitor_details *) a->agentdetails;

	switch (datatype)
	{
		case DATAFETCH_VALUE_SCALE:
			return &details->value;
			break;
		case DATAFETCH_VALUE_TEXT:
			return &details->str_value;
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_diskspacemonitor_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	//Add a menu item for every type
	for (int i = 1; i < DISKSPACEMONITOR_NUMTYPES; i++)
	{
		make_menuitem_cmd(m, agenttype_diskspacemonitor_friendlytypes[i], config_getfull_control_setagent_c(c, action, "DiskSpaceMonitor", agenttype_diskspacemonitor_types[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_diskspacemonitor_menu_context(Menu *m, agent *a)
{
	agenttype_diskspacemonitor_details *details = (agenttype_diskspacemonitor_details *)a->agentdetails;
	make_menuitem_str(m,"Monitoring  Path",config_getfull_control_setagentprop_s(a->controlptr,a->agentaction,"MonitoringPath"),details->path? details->path:"");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_diskspacemonitor_notifytype(int notifytype, void *messagedata)
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_timercall
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VOID CALLBACK agenttype_diskspacemonitor_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//If there are agents left
	if (agenttype_diskspacemonitor_agents->first != NULL)
	{
		agenttype_diskspacemonitor_propogatenewvalues();
	}
	else
	{
		agenttype_diskspacemonitor_hastimer = false;
		KillTimer(hwnd, 0);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT CALLBACK agenttype_diskspacemonitor_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_propogatenewvalues
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_diskspacemonitor_propogatenewvalues()
{
	//Declare variables
	agent *currentagent;
	listnode *currentnode;
	agenttype_diskspacemonitor_details *details;
	//Go through every agent
	dolist(currentnode, agenttype_diskspacemonitor_agents)
	{
		currentagent = (agent *) currentnode->value;
		details = (agenttype_diskspacemonitor_details *)currentagent->agentdetails;
		//initialize
		details->previous_value = details->value;
		details->value = -1.0;
	}

	dolist(currentnode, agenttype_diskspacemonitor_agents)
	{
		currentagent = (agent *) currentnode->value;
		details = (agenttype_diskspacemonitor_details *)currentagent->agentdetails;
		
		//Calculate a new value for this type
		agenttype_diskspacemonitor_updatevalue(details);

		//Tell the agent to update itself if the value has changed

		if (details->previous_value != details->value)
		{
			control_notify(currentagent->controlptr, NOTIFY_NEEDUPDATE, NULL);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_diskspacemonitor_getvalue
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_diskspacemonitor_updatevalue(agenttype_diskspacemonitor_details *d)
{
	//Multipurpose value
	int monitor_type = d->monitor_type;
	//If we already know its value, then return it
	if (d->value != -1) return;

	static ULONGLONG diskfree,disktotal;	
	char *path= d->path?d->path:"C:\\";
	GetDiskFreeSpaceEx(path,NULL,(ULARGE_INTEGER *)&disktotal,(ULARGE_INTEGER *)&diskfree);
	//Otherwise, figure it out
	switch(monitor_type)
	{
		case DISKSPACEMONITOR_TYPE_DISKFREEVAL:
			d->value = diskfree/(double)disktotal;
			StrFormatByteSize64( diskfree, d->str_value,sizeof(d->str_value));
			break;
		case DISKSPACEMONITOR_TYPE_DISKFREEPCT:
			d->value = diskfree/(double)disktotal;
			if(d->value > 1)
				d->value=1.0;
			sprintf(d->str_value, "%d%%", ((int)100*d->value));
			break;
		case DISKSPACEMONITOR_TYPE_DISKUSEVAL:
			d->value = (disktotal-diskfree)/(double)disktotal;
			StrFormatByteSize64( disktotal-diskfree, d->str_value,sizeof(d->str_value));
			break;	
		case DISKSPACEMONITOR_TYPE_DISKUSEPCT:
			d->value = (disktotal-diskfree)/(double)disktotal;
			if(d->value > 1)
				d->value=1.0;
			sprintf(d->str_value, "%d%%", ((int)100*d->value));
			break;
		default:
			d->value=0.0;
			strcpy(d->str_value,"Error"); //Should never happen
			break;
	}

	//We now know the value - return
	return;
}

