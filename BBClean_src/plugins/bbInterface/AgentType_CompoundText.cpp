/*===================================================

	AGENTTYPE_COMPOUNDTEXT CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
#include <stdlib.h>

//Parent Include
#include "AgentType_CompoundText.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"

//Local variables
const int agenttype_compoundtext_subagentcount = AGENTTYPE_COMPOUNDTEXT_MAXAGENTS;

char *agenttype_compoundtext_agentdescriptions[AGENTTYPE_COMPOUNDTEXT_MAXAGENTS] =
{
	"Text1",
	"Text2",
	"Text3",
	"Text4",
	"Text5",
	"Text6",
	"Text7",
	"Text8",
	"Text9",
	"Text10"
};

const int agenttype_compoundtext_agenttypes[AGENTTYPE_COMPOUNDTEXT_MAXAGENTS] =
{
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT,
	CONTROL_FORMAT_TEXT
};

//Variables
const char* agenttype_compoundtext_commons[] = {
	"Usage: $",
	"CPU: $   MEM: $"
};

#define array_count(ary) (sizeof(ary) / sizeof(ary[0]))
const int agenttype_compoundtext_commoncount = array_count(agenttype_compoundtext_commons);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_compoundtext_startup()
{
	//Register this type with the ControlMaster
	agent_registertype(
		"Compound Text",                      //Friendly name of agent type
		"CompoundText",                       //Name of agent type
		CONTROL_FORMAT_TEXT,                //Control format
		true,
		&agenttype_compoundtext_create,           
		&agenttype_compoundtext_destroy,
		&agenttype_compoundtext_message,
		&agenttype_compoundtext_notify,
		&agenttype_compoundtext_getdata,
		&agenttype_compoundtext_menu_set,
		&agenttype_compoundtext_menu_context,
		&agenttype_compoundtext_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_compoundtext_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_compoundtext_create(agent *a, char *parameterstring)
{
	//Find out details about the string
	if (0 == * parameterstring)
		return 2; // no text, no agent!

	//Parse it for newline characters
	//Create the details
	agenttype_compoundtext_details *details = new agenttype_compoundtext_details;
	a->agentdetails = (void *)details;

	//Make the string
	details->text = new_string(parameterstring);

	//Nullify all agents
	for (int i = 0; i < AGENTTYPE_COMPOUNDTEXT_MAXAGENTS; i++) details->agents[i] = NULL;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_compoundtext_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_compoundtext_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_compoundtext_details *details = (agenttype_compoundtext_details *) a->agentdetails;

		//Delete all subagents
		for (int i = 0; i < AGENTTYPE_COMPOUNDTEXT_MAXAGENTS; i++) agent_destroy(&details->agents[i]);

		//Delete the text
		free_string(&details->text);

		//Delete the details
		delete details;    
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_compoundtext_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_compoundtext_message(agent *a, int tokencount, char *tokens[])
{
	if (!stricmp("Formatting", tokens[5]))
	{
		agenttype_compoundtext_details *details = (agenttype_compoundtext_details *) a->agentdetails;
		free_string(&details->text);
		if (*tokens[6]) details->text = new_string(tokens[6]);
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}

	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_compoundtext_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_compoundtext_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_compoundtext_details *details;
	details = (agenttype_compoundtext_details *) a->agentdetails;

	switch(notifytype)
	{
		//case NOTIFY_CHANGE:
			//Eventually, we'll have to update the text every time an
			//update is made. But this isn't necessary now.

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, details->text));
			
			//Save all child agents, if necessary
			for (int i = 0; i < AGENTTYPE_COMPOUNDTEXT_MAXAGENTS; i++) agent_notify(details->agents[i], NOTIFY_SAVE_AGENT, NULL);

			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_compoundtext_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_compoundtext_getdata(agent *a, int datatype)
{
	if (datatype == DATAFETCH_VALUE_TEXT)
	{
		//Get the details
		agenttype_compoundtext_details *details;
		details = (agenttype_compoundtext_details *) a->agentdetails;

		//Copy the characters of the text to the final output string
		int charindex = 0;
		char *currentoutput = details->finaltext;
		int agentindex = 0;
		while (details->text[charindex] != '\0')
		{
			//If we hit a $
			if (details->text[charindex] == '$')
			{
				//If we have an agent
				if (agentindex < AGENTTYPE_COMPOUNDTEXT_MAXAGENTS && details->agents[agentindex] != NULL)
				{
					char *agenttext = (char *) agent_getdata(details->agents[agentindex], DATAFETCH_VALUE_TEXT);
					strcpy(currentoutput, agenttext);
					currentoutput += strlen(agenttext);
				}
				else
				{
					strcpy(currentoutput, "[?]");
					currentoutput += 3;
				}
				agentindex++;
			}
			else
			{
				currentoutput[0] = details->text[charindex];
				currentoutput += 1;
			}

			charindex++;
		}

		//End the string
		currentoutput[0] = '\0';

		return details->finaltext;
	}
	else if (datatype == DATAFETCH_SUBAGENTS_NAMES_ARRAY)
	{
		return agenttype_compoundtext_agentdescriptions;
	}
	else if (datatype == DATAFETCH_SUBAGENTS_POINTERS_ARRAY)
	{
		agenttype_compoundtext_details *details;
		details = (agenttype_compoundtext_details *) a->agentdetails;
		return details->agents;
	}
	else if (datatype == DATAFETCH_SUBAGENTS_TYPES_ARRAY)
	{
		return (void *) agenttype_compoundtext_agenttypes;
	}
	else if (datatype == DATAFETCH_SUBAGENTS_COUNT)
	{
		return (void *) &agenttype_compoundtext_subagentcount;
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_compoundtext_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_compoundtext_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	const char *text = a
		? ((agenttype_compoundtext_details *) a->agentdetails)->text : "";

	make_menuitem_str(
		m,
		"Entry:",
		config_getfull_control_setagent_s(c, action, "CompoundText"),
		text
		);

	make_menuitem_nop(m, "");
	for (int i = 0; i < agenttype_compoundtext_commoncount; i++)
	{
		make_menuitem_bol(m, agenttype_compoundtext_commons[i], config_getfull_control_setagent_c(c, action, "CompoundText", agenttype_compoundtext_commons[i]), 0 == strcmp(text, agenttype_compoundtext_commons[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_compoundtext_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_compoundtext_menu_context(Menu *m, agent *a)
{
	agenttype_compoundtext_details *details = (agenttype_compoundtext_details *) a->agentdetails;

	make_menuitem_str(m, "Formatting", config_getfull_control_setagentprop_s(a->controlptr, a->agentaction, "Formatting"), details->text ? details->text : "");

	char namedot[1000];
	sprintf(namedot, "%s%s", a->agentaction, ".");	
	menu_controloptions(m, a->controlptr, AGENTTYPE_COMPOUNDTEXT_MAXAGENTS, details->agents, namedot, agenttype_compoundtext_agentdescriptions, agenttype_compoundtext_agenttypes);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_compoundtext_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_compoundtext_notifytype(int notifytype, void *messagedata)
{

}

/*=================================================*/
