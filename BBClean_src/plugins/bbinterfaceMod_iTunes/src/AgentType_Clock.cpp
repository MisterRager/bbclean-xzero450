/*===================================================

	AGENTTYPE_CLOCK CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
#include <windows.h>
#include <time.h>
//Parent Include
#include "AgentType_Clock.h"


//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MessageMaster.h"
#include "ListMaster.h"



//Declare the function prototypes;
VOID CALLBACK agenttype_clock_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK agenttype_clock_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void agenttype_clock_update();
void agenttype_clock_updatevalue(agenttype_clock_details *details);

// Some windowing variables
HWND agenttype_clock_window;
bool agenttype_clock_windowclassregistered;

//Local primitives
unsigned long agenttype_clock_counter;
const char agenttype_clock_timerclass[] = "BBInterfaceAgentClock";

//A list of this type of agent
list *agenttype_clock_agents;

//local valuables
bool agenttype_clock_hastimer = false;
struct tm *ltp;

#define select_fmt(x) ((x) ? (x) : "%H:%M:%S")

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_clock_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_clock_startup()
{
	//Create the list
	agenttype_clock_agents = list_create();

	//Register the window class
	agenttype_clock_windowclassregistered = false;
	if (window_helper_register(agenttype_clock_timerclass, &agenttype_clock_event))
	{
		//Couldn't register the window
		BBMessageBox(NULL, "failed on register class", "test", MB_OK);
		return 1;
	}
	agenttype_clock_windowclassregistered = true;

	//Create the window
	agenttype_clock_window = window_helper_create(agenttype_clock_timerclass);
	if (!agenttype_clock_window)
	{
		//Couldn't create the window
		BBMessageBox(NULL, "failed on window", "test", MB_OK);
		return 1;
	}


	//If we got this far, we can successfully use this function
	//Register this type with the AgentMaster
	agent_registertype(
		"Clock",                   //Friendly name of agent type
		"Clock",                    //Name of agent type
		CONTROL_FORMAT_TEXT,      //Control type
		false,
		&agenttype_clock_create,
		&agenttype_clock_destroy,
		&agenttype_clock_message,
		&agenttype_clock_notify,
		&agenttype_clock_getdata,
		&agenttype_clock_menu_set,
		&agenttype_clock_menu_context,
		&agenttype_clock_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_clock_shutdown()
{
	if(agenttype_clock_hastimer){
		agenttype_clock_hastimer = false;
		KillTimer(agenttype_clock_window, 0);
	}
	//Destroy the internal tracking list
	if (agenttype_clock_agents) list_destroy(agenttype_clock_agents);
	
	//Destroy the window
	if (agenttype_clock_window) window_helper_destroy(agenttype_clock_window);

	//Unregister the window class
	if (agenttype_clock_windowclassregistered) window_helper_unregister(agenttype_clock_timerclass);


	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_clock_create(agent *a, char *parameterstring)
{


	if (0 == * parameterstring)
		return 2; // no param, no agent

	//Create the details
	agenttype_clock_details *details = new agenttype_clock_details;
	details->format = NULL;
	
	//Create a unique string to assign to this (just a number from a counter)
	char identifierstring[64];
	sprintf(identifierstring, "%ul", agenttype_clock_counter);
	details->internal_identifier = new_string(identifierstring);

	//Set the details
	a->agentdetails = (void *)details;

	//Add this to our internal tracking list
	agent *oldagent; //Unused, but we have to pass it
	list_add(agenttype_clock_agents, details->internal_identifier, (void *) a, (void **) &oldagent);

	//Increment the counter
	agenttype_clock_counter++;

	time_t systemTime;
	time(&systemTime);
	ltp = localtime(&systemTime);
	agenttype_clock_updatevalue(details);

	if (!agenttype_clock_hastimer)
	{
		SetTimer(agenttype_clock_window, 0, 1000, agenttype_clock_timercall);
		agenttype_clock_hastimer = true;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_clock_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_clock_details *details = (agenttype_clock_details *) a->agentdetails;

		//Delete from the tracking list
		list_remove(agenttype_clock_agents, details->internal_identifier);

		//Free the string
		free_string(&details->internal_identifier);
		free_string(&details->format);

		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_clock_message(agent *a, int tokencount, char *tokens[])
{
	agenttype_clock_details *details = (agenttype_clock_details *)a->agentdetails;
	if (!strcmp("ClockFormat",tokens[5]) && config_set_str(tokens[6],&details->format)){
		agenttype_clock_updatevalue(details);
		control_notify(a->controlptr,NOTIFY_NEEDUPDATE,NULL);
	}
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_clock_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_clock_details *details = (agenttype_clock_details *) a->agentdetails;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, "Clock"));
			config_write(config_get_control_setagentprop_c(a->controlptr, a->agentaction, "ClockFormat",select_fmt(details->format)));
			
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_clock_getdata(agent *a, int datatype)
{
	//Get the agent details
	agenttype_clock_details *details = (agenttype_clock_details *) a->agentdetails;

	return &details->timestr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_clock_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	make_menuitem_cmd(m, "Clock", config_getfull_control_setagent_c(c, action, "Clock", "Clock"));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_clock_menu_context(Menu *m, agent *a)
{
	agenttype_clock_details *details = (agenttype_clock_details *)a->agentdetails;
	make_menuitem_str(m, "Clock Format",config_getfull_control_setagentprop_s(a->controlptr, a->agentaction,"ClockFormat"),select_fmt(details->format));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_clock_notifytype(int notifytype, void *messagedata)
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_timercall
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VOID CALLBACK agenttype_clock_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//If there are agents left
	if (agenttype_clock_agents->first != NULL)
	{
		//get  value
		time_t systemTime;
		time(&systemTime);
		ltp = localtime(&systemTime);
		SYSTEMTIME lt;
		GetLocalTime(&lt);
		//Timer reset
		SetTimer(agenttype_clock_window, 0, 1100 - lt.wMilliseconds,agenttype_clock_timercall);
		agenttype_clock_update();
	}
	else
	{
		agenttype_clock_hastimer = false;
		KillTimer(hwnd, 0);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT CALLBACK agenttype_clock_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_update
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_clock_update()
{
	agent *currentagent;
	agenttype_clock_details *details;
	//Go through every agent
	listnode *currentnode;
	dolist(currentnode, agenttype_clock_agents)
	{
		//Get the agent
		currentagent = (agent *) currentnode->value;
		details = (agenttype_clock_details *)(currentagent->agentdetails);
		agenttype_clock_updatevalue(details);
		control_notify(currentagent->controlptr, NOTIFY_NEEDUPDATE, NULL);
	}
}

void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_clock_updatevalue
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_clock_updatevalue(agenttype_clock_details *details)
{
	int len = 0;
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = myInvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);
	len = strftime(
		details->timestr,
		sizeof(details->timestr), 
		select_fmt(details->format),
		ltp);
	if(len == 0) strcpy(details->timestr,"Invalid Format");
	_set_invalid_parameter_handler(oldHandler);
	
}

