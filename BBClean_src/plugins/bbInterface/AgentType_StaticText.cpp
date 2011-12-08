/*===================================================

	AGENTTYPE_STATICTEXT CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
#include <stdlib.h>

//Parent Include
#include "AgentType_StaticText.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"

//Variables
const char* agenttype_statictext_commons[] = {
"OK",
"Cancel",
"Show Plugins",
"Hide Plugins",
"Play",
"Pause",
"Stop",
"Next",
"Previous",
"Open"
};

#define array_count(ary) (sizeof(ary) / sizeof(ary[0]))

const int agenttype_statictext_commoncount =
	array_count(agenttype_statictext_commons);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_statictext_startup()
{
	//Register this type with the ControlMaster
	agent_registertype(
		"Static Text",                      //Friendly name of agent type
		"StaticText",                       //Name of agent type
		CONTROL_FORMAT_TEXT,                //Control format
		true,
		&agenttype_statictext_create,           
		&agenttype_statictext_destroy,
		&agenttype_statictext_message,
		&agenttype_statictext_notify,
		&agenttype_statictext_getdata,
		&agenttype_statictext_menu_set,
		&agenttype_statictext_menu_context,
		&agenttype_statictext_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_statictext_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_statictext_create(agent *a, char *parameterstring)
{
	//Find out details about the string
	if (0 == * parameterstring)
		return 2; // no text, no agent!

	//Parse it for newline characters
	//Create the details
	agenttype_statictext_details *details = new agenttype_statictext_details;
	a->agentdetails = (void *)details;

	//Make the string
	details->text = new_string(parameterstring);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_statictext_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_statictext_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_statictext_details *details = (agenttype_statictext_details *) a->agentdetails;

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
//agenttype_statictext_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_statictext_message(agent *a, int tokencount, char *tokens[])
{
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_statictext_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_statictext_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_statictext_details *details;
	details = (agenttype_statictext_details *) a->agentdetails;

	switch(notifytype)
	{
		//case NOTIFY_CHANGE:
			//Eventually, we'll have to update the text every time an
			//update is made. But this isn't necessary now.

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, details->text));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_statictext_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_statictext_getdata(agent *a, int datatype)
{
	if (datatype == DATAFETCH_VALUE_TEXT)
	{
		agenttype_statictext_details *details;
		details = (agenttype_statictext_details *) a->agentdetails;
		return details->text;
	}
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_statictext_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_statictext_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	const char *text = a
		? ((agenttype_statictext_details *) a->agentdetails)->text : "";

	make_menuitem_str(
		m,
		"Entry:",
		config_getfull_control_setagent_s(c, action, "StaticText"),
		text
		);

	make_menuitem_nop(m, "");
	for (int i = 0; i < agenttype_statictext_commoncount; i++)
	{
		make_menuitem_bol(m, agenttype_statictext_commons[i], config_getfull_control_setagent_c(c, action, "StaticText", agenttype_statictext_commons[i]), 0 == strcmp(text, agenttype_statictext_commons[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_statictext_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_statictext_menu_context(Menu *m, agent *a)
{
	make_menuitem_nop(m, "No options available.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_statictext_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_statictext_notifytype(int notifytype, void *messagedata)
{

}

/*=================================================*/
