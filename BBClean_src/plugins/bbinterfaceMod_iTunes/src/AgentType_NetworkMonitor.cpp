/*===================================================

	AGENTTYPE_NETWORKMONITOR CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
#include <windows.h>
#include <iphlpapi.h>
//Parent Include
#include "AgentType_NetworkMonitor.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MessageMaster.h"
#include "ListMaster.h"



//Declare the function prototypes;
VOID CALLBACK agenttype_networkmonitor_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK agenttype_networkmonitor_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void agenttype_networkmonitor_propogatenewvalues();
void agenttype_networkmonitor_updatevalue(int monitor_interface_number);

// Some windowing variables
HWND agenttype_networkmonitor_window;
bool agenttype_networkmonitor_windowclassregistered;
bool agenttype_networkmonitor_hastimer = false;


//Local primitives
unsigned long agenttype_networkmonitor_counter;
const char agenttype_networkmonitor_timerclass[] = "BBInterfaceAgentNetworkMon";

void StrFormatByteSizeOrg(DWORD value,char c[]);

//A list of this type of agent
list *agenttype_networkmonitor_agents;

//Declare the number of types
#define AGENTTYPE_NETWORKMONITOR_NUMTYPES 3

enum NETWORKMONITOR_TYPES
{
	NETWORKMONITOR_TYPE_TOTAL = 0,
	NETWORKMONITOR_TYPE_DOWNLOAD,
	NETWORKMONITOR_TYPE_UPLOAD
};

const char *agenttype_networkmonitor_friendlytypes[] =
{
	"TOTAL",
	"Download",
	"Upload",
};


//Declare the number of Interfaces
#define AGENTTYPE_NETWORKMONITOR_NUMINTERFACES 10

//Array of string types - must have AGENTTYPE_NETWORKMONITOR_NUMINTERFACES entries
enum NETWORKMONITOR_INTERFACES
{
	NETWORKMONITOR_INTERFACE_ALL = 0,
	NETWORKMONITOR_INTERFACE_1,
	NETWORKMONITOR_INTERFACE_2,
	NETWORKMONITOR_INTERFACE_3,
	NETWORKMONITOR_INTERFACE_4,
	NETWORKMONITOR_INTERFACE_5,
	NETWORKMONITOR_INTERFACE_6,
	NETWORKMONITOR_INTERFACE_7,
	NETWORKMONITOR_INTERFACE_8,
	NETWORKMONITOR_INTERFACE_9
	
};

//Must match the enum ordering above! Must have AGENTTYPE_NETWORKMONITOR_NUMINTERFACES entries
const char *agenttype_networkmonitor_interface_numbers[] =
{
	"ALLInterfaces",
	"Interface1",
	"Interface2",
	"Interface3",
	"Interface4",
	"Interface5",
	"Interface6",
	"Interface7",
	"Interface8",
	"Interface9"
};

//Must match the enum ordering above! Must have AGENTTYPE_NETWORKMONITOR_NUMINTERFACES entries
const char *agenttype_networkmonitor_friendlyinterfaces[] =
{
	"ALL Interfaces",
	"Interface 1",
	"Interface 2",
	"Interface 3",
	"Interface 4",
	"Interface 5",
	"Interface 6",
	"Interface 7",
	"Interface 8",
	"Interface 9"
};


//Must be the same size as the above array and enum
double agenttype_networkmonitor_previous_values[AGENTTYPE_NETWORKMONITOR_NUMINTERFACES][AGENTTYPE_NETWORKMONITOR_NUMTYPES];
double agenttype_networkmonitor_values[AGENTTYPE_NETWORKMONITOR_NUMINTERFACES][AGENTTYPE_NETWORKMONITOR_NUMTYPES];

char agenttype_networkmonitor_textvalues[AGENTTYPE_NETWORKMONITOR_NUMINTERFACES][AGENTTYPE_NETWORKMONITOR_NUMTYPES][32];

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_networkmonitor_startup()
{
	//This code stolen from the bb4win SDK example

	//Initialize the usages to 0.0 to start
	for (int i = 0; i < AGENTTYPE_NETWORKMONITOR_NUMINTERFACES; i++)
	{
		for(int j = 0; j < AGENTTYPE_NETWORKMONITOR_NUMTYPES; j++)
		{
			agenttype_networkmonitor_previous_values[i][j] = -1.0;
			agenttype_networkmonitor_values[i][j] = 0.0;
			StrFormatByteSizeOrg(0,agenttype_networkmonitor_textvalues[i][j]);
		}
	}

	//Create the list
	agenttype_networkmonitor_agents = list_create();

	//Register the window class
	agenttype_networkmonitor_windowclassregistered = false;
	if (window_helper_register(agenttype_networkmonitor_timerclass, &agenttype_networkmonitor_event))
	{
		//Couldn't register the window
		BBMessageBox(NULL, "failed on register class", "test", MB_OK);
		return 1;
	}
	agenttype_networkmonitor_windowclassregistered = true;

	//Create the window
	agenttype_networkmonitor_window = window_helper_create(agenttype_networkmonitor_timerclass);
	if (!agenttype_networkmonitor_window)
	{
		//Couldn't create the window
		BBMessageBox(NULL, "failed on window", "test", MB_OK);
		return 1;
	}


		//If we got this far, we can successfully use this function
		//Register this type with the AgentMaster
	agent_registertype(
		"Network Monitor",                   //Friendly name of agent type
		"NetworkMonitor",                    //Name of agent type
		CONTROL_FORMAT_DOUBLE|CONTROL_FORMAT_TEXT,				//Control type
		false,
		&agenttype_networkmonitor_create,
		&agenttype_networkmonitor_destroy,
		&agenttype_networkmonitor_message,
		&agenttype_networkmonitor_notify,
		&agenttype_networkmonitor_getdata,
		&agenttype_networkmonitor_menu_set,
		&agenttype_networkmonitor_menu_context,
		&agenttype_networkmonitor_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_networkmonitor_shutdown()
{
	if(agenttype_networkmonitor_hastimer){
		agenttype_networkmonitor_hastimer = false;
		KillTimer(agenttype_networkmonitor_window, 0);
	}

	//Destroy the internal tracking list
	if (agenttype_networkmonitor_agents) list_destroy(agenttype_networkmonitor_agents);

	//Destroy the window
	if (agenttype_networkmonitor_window) window_helper_destroy(agenttype_networkmonitor_window);

	//Unregister the window class
	if (agenttype_networkmonitor_windowclassregistered) window_helper_unregister(agenttype_networkmonitor_timerclass);


	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_networkmonitor_create(agent *a, char *parameterstring)
{


	if (0 == * parameterstring)
		return 2; // no param, no agent

	//Find the monitor type
	int monitor_interface_number = AGENTTYPE_NETWORKMONITOR_NUMINTERFACES;
	for (int i = 0; i < AGENTTYPE_NETWORKMONITOR_NUMINTERFACES; i++)
	{
		if (stricmp(agenttype_networkmonitor_interface_numbers[i], parameterstring) == 0)
		{
			monitor_interface_number = i;
			break;
		}
	}

	//If we didn't find a correct monitor type
	if (monitor_interface_number == AGENTTYPE_NETWORKMONITOR_NUMINTERFACES)
	{
		//On an error
		if (!plugin_suppresserrors)
		{
			char buffer[1000];
			sprintf(buffer,	"There was an error setting the Network Monitor agent:\n\nType \"%s\" is not a valid type.", parameterstring);
			BBMessageBox(NULL, buffer, szAppName, MB_OK|MB_SYSTEMMODAL);
		}
		return 1;
	}

	//Create the details
	agenttype_networkmonitor_details *details = new agenttype_networkmonitor_details;
	details->monitor_interface_number = monitor_interface_number;

	//Create a unique string to assign to this (just a number from a counter)
	char identifierstring[64];
	sprintf(identifierstring, "%ul", agenttype_networkmonitor_counter);
	details->internal_identifier = new_string(identifierstring);
	details->monitor_types = NETWORKMONITOR_TYPE_TOTAL;

	//Set the details
	a->agentdetails = (void *)details;

	//Add this to our internal tracking list
	agent *oldagent; //Unused, but we have to pass it
	list_add(agenttype_networkmonitor_agents, details->internal_identifier, (void *) a, (void **) &oldagent);

	//Increment the counter
	agenttype_networkmonitor_counter++;

	if (!agenttype_networkmonitor_hastimer)
	{
		SetTimer(agenttype_networkmonitor_window, 0, 1000, agenttype_networkmonitor_timercall);
		agenttype_networkmonitor_hastimer = true;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_networkmonitor_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_networkmonitor_details *details = (agenttype_networkmonitor_details *) a->agentdetails;

		//Delete from the tracking list
		list_remove(agenttype_networkmonitor_agents, details->internal_identifier);

		//Free the string
		free_string(&details->internal_identifier);

		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_networkmonitor_message(agent *a, int tokencount, char *tokens[])
{
	agenttype_networkmonitor_details *details = (agenttype_networkmonitor_details *) a->agentdetails;
	          
	if (!stricmp("MonitorType", tokens[5]) && config_set_int(tokens[6],&details->monitor_types, 0,2)){
		control_notify(a->controlptr,NOTIFY_NEEDUPDATE,NULL);
	}
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_networkmonitor_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_networkmonitor_details *details = (agenttype_networkmonitor_details *) a->agentdetails;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, agenttype_networkmonitor_interface_numbers[details->monitor_interface_number]));
			config_write(config_get_control_setagentprop_i(a->controlptr,a->agentaction, "MonitorType",&details->monitor_types));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_networkmonitor_getdata(agent *a, int datatype)
{
	//Get the agent details
	agenttype_networkmonitor_details *details = (agenttype_networkmonitor_details *) a->agentdetails;

	switch (datatype)
	{
		case DATAFETCH_VALUE_SCALE:
			return &agenttype_networkmonitor_values[details->monitor_interface_number][details->monitor_types];
			break;
		case DATAFETCH_VALUE_TEXT:
			return &agenttype_networkmonitor_textvalues[details->monitor_interface_number][details->monitor_types];
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_networkmonitor_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	//Add a menu item for every type
	for (int i = 0; i < AGENTTYPE_NETWORKMONITOR_NUMINTERFACES; i++)
	{
		make_menuitem_cmd(m, agenttype_networkmonitor_friendlyinterfaces[i], config_getfull_control_setagent_c(c, action, "NetworkMonitor", agenttype_networkmonitor_interface_numbers[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_networkmonitor_menu_context(Menu *m, agent *a)
{
	agenttype_networkmonitor_details *details =  (agenttype_networkmonitor_details *)a->agentdetails;
	for (int i = 0; i < AGENTTYPE_NETWORKMONITOR_NUMTYPES; i++)
	{
		make_menuitem_bol(m, agenttype_networkmonitor_friendlytypes[i], config_getfull_control_setagentprop_i(a->controlptr, a->agentaction, "MonitorType", &i),i==details->monitor_types);
	}
	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_networkmonitor_notifytype(int notifytype, void *messagedata)
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_timercall
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VOID CALLBACK agenttype_networkmonitor_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//If there are agents left
	if (agenttype_networkmonitor_agents->first != NULL)
	{
		agenttype_networkmonitor_propogatenewvalues();
	}
	else
	{
		agenttype_networkmonitor_hastimer = false;
		KillTimer(hwnd, 0);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT CALLBACK agenttype_networkmonitor_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_propogatenewvalues
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_networkmonitor_propogatenewvalues()
{
	//Declare variables
	agent *currentagent;
	
	//Reset all values to unknown
	for (int i = 0; i < AGENTTYPE_NETWORKMONITOR_NUMINTERFACES; i++)
	{
		for (int j = 0; j < 3; j++){
			agenttype_networkmonitor_previous_values[i][j] = agenttype_networkmonitor_values[i][j];
			agenttype_networkmonitor_values[i][j] = -1.0;
		}
	}

	listnode *currentnode;
	//Go through every agent
	dolist(currentnode, agenttype_networkmonitor_agents)
	{
		//Get the agent
		currentagent = (agent *) currentnode->value;

		//Get the monitor type
		int monitor_interface_number = ((agenttype_networkmonitor_details *) (currentagent->agentdetails))->monitor_interface_number;

		//Calculate a new value for this type
		agenttype_networkmonitor_updatevalue(monitor_interface_number);

		//Tell the agent to update itself if the value has changed
		if (agenttype_networkmonitor_previous_values[monitor_interface_number][0] != agenttype_networkmonitor_values[monitor_interface_number][0])
		{
			control_notify(currentagent->controlptr, NOTIFY_NEEDUPDATE, NULL);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_networkmonitor_getvalue
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_networkmonitor_updatevalue(int monitor_interface_number)
{
	//Multipurpose value
	if (agenttype_networkmonitor_values[monitor_interface_number][0] != -1)
		return;

	
	//Get the Network Info
	static DWORD dwIn[AGENTTYPE_NETWORKMONITOR_NUMINTERFACES]={0} ,dwOut[AGENTTYPE_NETWORKMONITOR_NUMINTERFACES]={0};
	static DWORD d = 0;
	static PMIB_IFTABLE IfTable;
	DWORD dwOldIn, dwOldOut;

	
	if( d == 0 ){
		GetIfTable( NULL , &d, FALSE );
		IfTable = (PMIB_IFTABLE)new char[d] ;
	}

	//Otherwise, figure it out
	if( GetIfTable( (PMIB_IFTABLE)IfTable, &d, FALSE) == NO_ERROR ){
		dwOldIn = dwIn[monitor_interface_number];
		dwOldOut = dwOut[monitor_interface_number];
		if(monitor_interface_number == NETWORKMONITOR_INTERFACE_ALL){ //TOTAL
			dwIn[monitor_interface_number] = dwOut[monitor_interface_number] = 0;
			for( int i = 0; i < (int)IfTable->dwNumEntries; i++){
				dwIn[monitor_interface_number] += IfTable->table[i].dwInOctets;
				dwOut[monitor_interface_number] += IfTable->table[i].dwOutOctets;
			}
		}else{
			if((int)IfTable->dwNumEntries>monitor_interface_number-1){
				dwIn[monitor_interface_number] = IfTable->table[monitor_interface_number-1].dwInOctets;
				dwOut[monitor_interface_number] = IfTable->table[monitor_interface_number-1].dwOutOctets;
			}
		}
		DWORD dwInVal = (dwOldIn==0)?0:dwIn[monitor_interface_number]-dwOldIn;
		DWORD dwOutVal= (dwOldOut==0)?0:dwOut[monitor_interface_number]-dwOldOut;
		agenttype_networkmonitor_values[monitor_interface_number][NETWORKMONITOR_TYPE_DOWNLOAD] = (double)dwInVal;
		agenttype_networkmonitor_values[monitor_interface_number][NETWORKMONITOR_TYPE_UPLOAD] = (double)dwOutVal;
		agenttype_networkmonitor_values[monitor_interface_number][NETWORKMONITOR_TYPE_TOTAL] = (double)(dwInVal + dwOutVal);
		StrFormatByteSizeOrg(dwInVal,agenttype_networkmonitor_textvalues[monitor_interface_number][NETWORKMONITOR_TYPE_DOWNLOAD]);
		StrFormatByteSizeOrg(dwOutVal,agenttype_networkmonitor_textvalues[monitor_interface_number][NETWORKMONITOR_TYPE_UPLOAD]);
		StrFormatByteSizeOrg(dwInVal+dwOutVal,agenttype_networkmonitor_textvalues[monitor_interface_number][NETWORKMONITOR_TYPE_TOTAL]);
	}

	//In all monitor type cases, copy the double value to the string value
	//We now know the value - return
	return;
}

void StrFormatByteSizeOrg(DWORD value,char c[]){
	if(value<1024)
		sprintf(c,"%dBytes",value);
	else if(value<1024*1024)
		sprintf(c,"%.1fKB",value/1024.);
	else if(value<1024*1024*1024)
		sprintf(c,"%.1fMB",value/(1024*1024.));
}	
