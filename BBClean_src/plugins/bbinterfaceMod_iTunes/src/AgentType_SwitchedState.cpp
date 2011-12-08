/*===================================================

	AGENTTYPE_SWITCHEDSTATE CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>

//Parent Include
#include "AgentType_SwitchedState.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"
#include "MessageMaster.h"

//Local variables
const int agenttype_switchedstate_subagentcount = 3;
#define AGENTTYPE_SWITCHEDSTATE_AGENT_BOOL 0
#define AGENTTYPE_SWITCHEDSTATE_AGENT_WHENTRUE 1
#define AGENTTYPE_SWITCHEDSTATE_AGENT_WHENFALSE 2
char *agenttype_switchedstate_agentdescriptions[3] =
{
	"Value",
	"ResultWhenTrue",
	"ResultWhenFalse"
};

//The agent types arrays
#define AGENTTYPE_SWITCHEDSTATE_TYPECOUNT 3
#define AGENTTYPE_SWITCHEDSTATE_TYPE_TEXT 0
#define AGENTTYPE_SWITCHEDSTATE_TYPE_IMAGE 1
#define AGENTTYPE_SWITCHEDSTATE_TYPE_TRIGGER 2

const int agenttype_switchedstate_agenttypes[][AGENTTYPE_SWITCHEDSTATE_TYPECOUNT] =
{
	{CONTROL_FORMAT_BOOL, CONTROL_FORMAT_TEXT, CONTROL_FORMAT_TEXT},
    {CONTROL_FORMAT_BOOL, CONTROL_FORMAT_IMAGE, CONTROL_FORMAT_IMAGE},
    {CONTROL_FORMAT_BOOL, CONTROL_FORMAT_TRIGGER, CONTROL_FORMAT_TRIGGER}
};

const char *agenttype_switchedstate_typenames[AGENTTYPE_SWITCHEDSTATE_TYPECOUNT] =
{
	"TEXT",
	"IMAGE",
	"TRIGGER"
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_switchedstate_startup()
{
	//Register this type with the ControlMaster
	agent_registertype(
		"Switched State Value",              //Friendly name of agent type
		"SwitchedState",                    //Name of agent type
		CONTROL_FORMAT_TEXT|CONTROL_FORMAT_TRIGGER|CONTROL_FORMAT_IMAGE,   //Control type
		false,
		&agenttype_switchedstate_create,            
		&agenttype_switchedstate_destroy,
		&agenttype_switchedstate_message,
		&agenttype_switchedstate_notify,
		&agenttype_switchedstate_getdata,
		&agenttype_switchedstate_menu_set,
		&agenttype_switchedstate_menu_context,
		&agenttype_switchedstate_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_switchedstate_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_switchedstate_create(agent *a, char *parameterstring)
{
	if (0 == * parameterstring)	return 2; // no param, no agent

	//Figure out the type
	int agenttype = -1;
	for (int i = 0; i < AGENTTYPE_SWITCHEDSTATE_TYPECOUNT; i++)
	{
		if (stricmp(parameterstring, agenttype_switchedstate_typenames[i]) == 0) agenttype = i;
	}
	if (agenttype == -1) return 1;

	//Create the details
	agenttype_switchedstate_details *details = new agenttype_switchedstate_details;
	a->agentdetails = (void *)details;
	details->datatype = agenttype;

	//Void all agents
	for (int  i = 0; i < 3; i++) details->agents[i] = NULL;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_switchedstate_destroy(agent *a)
{
	if (a->agentdetails)
	{		
		agenttype_switchedstate_details *details = (agenttype_switchedstate_details *) a->agentdetails;

		//Destroy all child agents
		for (int  i = 0; i < 3; i++) agent_destroy(&details->agents[i]);

		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_switchedstate_message(agent *a, int tokencount, char *tokens[])
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_switchedstate_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_switchedstate_details *details = (agenttype_switchedstate_details *) a->agentdetails;

	//Declare variables
	bool *boolptr;

	switch(notifytype)
	{
		case NOTIFY_DRAW:
		case NOTIFY_CHANGE:
			//Get the boolean value
			boolptr = (bool *) agent_getdata(details->agents[AGENTTYPE_SWITCHEDSTATE_AGENT_BOOL], DATAFETCH_VALUE_BOOL);

			//Do the appropriate action
			if (boolptr != NULL && *boolptr == true) return agent_notify(details->agents[AGENTTYPE_SWITCHEDSTATE_AGENT_WHENTRUE], notifytype, messagedata);
			else return agent_notify(details->agents[AGENTTYPE_SWITCHEDSTATE_AGENT_WHENFALSE], notifytype, messagedata);

			break;

		case NOTIFY_SAVE_AGENT:

			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, agenttype_switchedstate_typenames[details->datatype]));

			//Save all child agents, if necessary
			for (int i = 0; i < 3; i++) agent_notify(details->agents[i], NOTIFY_SAVE_AGENT, NULL);
			break;
	}

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_switchedstate_getdata(agent *a, int datatype)
{
	agenttype_switchedstate_details *details = (agenttype_switchedstate_details *) a->agentdetails;

	bool *boolptr;

	switch (datatype)
	{
		case DATAFETCH_VALUE_TEXT:
			//Get the boolean value
			boolptr = (bool *) agent_getdata(details->agents[AGENTTYPE_SWITCHEDSTATE_AGENT_BOOL], DATAFETCH_VALUE_BOOL);

			//If the boolean value is true...
			if (boolptr != NULL && *boolptr == true) return agent_getdata(details->agents[AGENTTYPE_SWITCHEDSTATE_AGENT_WHENTRUE], datatype);
			else return agent_getdata(details->agents[AGENTTYPE_SWITCHEDSTATE_AGENT_WHENFALSE], datatype);

			break;

		case DATAFETCH_SUBAGENTS_POINTERS_ARRAY:
			return details->agents;
		case DATAFETCH_SUBAGENTS_NAMES_ARRAY:
			return agenttype_switchedstate_agentdescriptions;
		case DATAFETCH_SUBAGENTS_TYPES_ARRAY:
			return (void *) agenttype_switchedstate_agenttypes[details->datatype];
		case DATAFETCH_SUBAGENTS_COUNT:
			return (void *) &agenttype_switchedstate_subagentcount;
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_switchedstate_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	char temp[32] = "";
	switch(controlformat)
	{
		case CONTROL_FORMAT_TEXT: strcpy(temp, agenttype_switchedstate_typenames[AGENTTYPE_SWITCHEDSTATE_TYPE_TEXT]); break;
		case CONTROL_FORMAT_IMAGE: strcpy(temp, agenttype_switchedstate_typenames[AGENTTYPE_SWITCHEDSTATE_TYPE_IMAGE]); break;
		case CONTROL_FORMAT_TRIGGER: strcpy(temp, agenttype_switchedstate_typenames[AGENTTYPE_SWITCHEDSTATE_TYPE_TRIGGER]); break;
		default: 
			make_menuitem_nop(m, "Invalid type.");
			break;
	}

	make_menuitem_cmd(m, "Set", config_getfull_control_setagent_c(c, action, "SwitchedState", temp));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_switchedstate_menu_context(Menu *m, agent *a)
{
	//Get the agent details
	agenttype_switchedstate_details *details = (agenttype_switchedstate_details *) a->agentdetails;

	char namedot[1000];
	sprintf(namedot, "%s%s", a->agentaction, ".");

	menu_controloptions(m, a->controlptr, 3, details->agents, namedot, agenttype_switchedstate_agentdescriptions, agenttype_switchedstate_agenttypes[details->datatype]);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_switchedstate_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_switchedstate_notifytype(int notifytype, void *messagedata)
{

}

/*=================================================*/
