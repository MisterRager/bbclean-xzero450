/*===================================================

	AGENTTYPE_SYSTEM CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>

//Parent Include
#include "AgentType_System.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "MenuMaster.h"
#include "ConfigMaster.h"

//Define all the appropriate commands
struct agenttype_system_touple
{
	char *name;
	int msg;
	int wparam;
};

agenttype_system_touple agenttype_system_touples[] = {
	{"System Shutdown",         BB_SHUTDOWN,      0},
	{"System Reboot",           BB_SHUTDOWN,      1},
	{"System Logoff",           BB_SHUTDOWN,      2},
	{"System Hibernate",        BB_SHUTDOWN,      3},
	{"System Suspend",          BB_SHUTDOWN,      4},
	{"System Lock",             BB_SHUTDOWN,      5},
	{"Show Run Dialog",         BB_RUN,           0},

	{"Blackbox Quit",           BB_QUIT,          0},
	{"Blackbox Restart",        BB_RESTART,       0},
	{"Blackbox Reconfigure",    BB_RECONFIGURE,   0},
	{"Blackbox Toggle Tray",    BB_TOGGLETRAY,    0},
	{"Blackbox Toggle Plugins", BB_TOGGLEPLUGINS, 0},

	{"Blackbox Menu",           BB_MENU, 0},

	{"Workspace Left",          BB_WORKSPACE,     BBWS_DESKLEFT},
	{"Workspace Right",         BB_WORKSPACE,     BBWS_DESKRIGHT},
	{"Workspace Move Left",     BB_WORKSPACE,     BBWS_MOVEWINDOWLEFT  },
	{"Workspace Move Right",    BB_WORKSPACE,     BBWS_MOVEWINDOWRIGHT },
	{"Workspace Gather",        BB_WORKSPACE,     BBWS_GATHERWINDOWS},

	{"Previous Window",         BB_WORKSPACE,     BBWS_PREVWINDOW },
	{"Next Window",             BB_WORKSPACE,     BBWS_NEXTWINDOW }

};

#define array_count(ary) (sizeof(ary) / sizeof(ary[0]))

const int agenttype_system_touple_count
	= array_count(agenttype_system_touples);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_system_startup()
{
	//Register this type with the ControlMaster
	agent_registertype(
		"System/Shell",                 //Friendly name of agent type
		"System",                           //Name of agent type
		CONTROL_FORMAT_TRIGGER,             //Control type
		true,
		&agenttype_system_create,           
		&agenttype_system_destroy,
		&agenttype_system_message,
		&agenttype_system_notify,
		&agenttype_system_getdata,
		&agenttype_system_menu_set,
		&agenttype_system_menu_context,
		&agenttype_system_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_system_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_system_create(agent *a, char *parameterstring)
{
	//Figure out the command
	int index;
	for (index = 0; index < agenttype_system_touple_count; index++)
		if (!stricmp(agenttype_system_touples[index].name, parameterstring))
			goto found;

	return 1;

found:

	//Create the details
	agenttype_system_details *details = new agenttype_system_details;
	a->agentdetails = (void *) details;
	details->commandindex = index;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_system_destroy(agent *a)
{
	if (a->agentdetails)
	{
		delete (agenttype_system_details *) a->agentdetails;   
		a->agentdetails = NULL;
	}
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_system_message(agent *a, int tokencount, char *tokens[])
{
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_system_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_system_details *details = (agenttype_system_details *) a->agentdetails;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			//Send the message
			PostMessage(
				plugin_hwnd_blackbox,
				agenttype_system_touples[details->commandindex].msg,
				agenttype_system_touples[details->commandindex].wparam,
				0);
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, agenttype_system_touples[details->commandindex].name));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_system_getdata(agent *a, int datatype)
{
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_system_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	int set = -1;
	if (a)
	{
		agenttype_system_details *details = (agenttype_system_details *) a->agentdetails;
		set = details->commandindex;
	}

	//List all options
	for (int i = 0; i < agenttype_system_touple_count; i++)
	{
		make_menuitem_bol(m, agenttype_system_touples[i].name, config_getfull_control_setagent_c(c, action, "System", agenttype_system_touples[i].name), i == set);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_system_menu_context(Menu *m, agent *a)
{
	make_menuitem_nop(m, "No options available.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_system_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_system_notifytype(int notifytype, void *messagedata)
{

}

/*=================================================*/
