/*===================================================

	AGENTTYPE_RUN CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>

//Parent Include
#include "AgentType_Run.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "DialogMaster.h"
#include "MessageMaster.h"
#include "MenuMaster.h"

//Constants

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_run_startup()
{
	//Register this type with the ControlMaster
	agent_registertype(
		"Run/Open",                 //Friendly name of agent type
		"Run",                      //Name of agent type
		CONTROL_FORMAT_TRIGGER|CONTROL_FORMAT_DROP, //Control type
		true,
		&agenttype_run_create,          
		&agenttype_run_destroy,
		&agenttype_run_message,
		&agenttype_run_notify,
		&agenttype_run_getdata,
		&agenttype_run_menu_set,
		&agenttype_run_menu_context,
		&agenttype_run_notifytype
		);


	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_run_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_run_create(agent *a, char *parameterstring)
{
	if (0 == * parameterstring)
		return 2; // no param, no agent

	//Create the details
	agenttype_run_details *details = new agenttype_run_details;
	a->agentdetails = (void *) details;
	details->command = NULL;
	details->arguments = NULL;
	details->workingdir = NULL;

	//Copy the parameter string
	if (!stricmp(parameterstring, "*browse*"))
	{
		//Get the file
		char *file = dialog_file(szFilterAll, "Select File", NULL, NULL, false);
		if (file)
		{
			details->command = new_string(file);
		}
		else
		{
			//message_override = true;
			return 2;
		}
	}
	else
	{
		details->command = new_string(parameterstring);
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_run_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_run_details *details = (agenttype_run_details *) a->agentdetails;
		free_string(&details->command);
		free_string(&details->arguments);
		free_string(&details->workingdir);
		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_run_message(agent *a, int tokencount, char *tokens[])
{
	if (!stricmp("Arguments", tokens[5]))
	{
		agenttype_run_details *details = (agenttype_run_details *) a->agentdetails;
		free_string(&details->arguments);
		if (*tokens[6]) details->arguments = new_string(tokens[6]);
		return 0;
	}
	if (!stricmp("WorkingDir", tokens[5]))
	{
		agenttype_run_details *details = (agenttype_run_details *) a->agentdetails;
		free_string(&details->workingdir);
		if (*tokens[6]) details->workingdir = new_string(tokens[6]);
		return 0;
	}
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_run_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_run_details *details;
	details = (agenttype_run_details *) a->agentdetails;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
		{
			const char *arguments = details->arguments;
			if (a->format & CONTROL_FORMAT_DROP) // OnDrop
			{
				if (NULL == arguments) arguments = "$DroppedFile$";
			}
			char buffer[BBI_MAX_LINE_LENGTH];
			if (arguments) arguments = message_preprocess(strcpy(buffer, arguments), a->controlptr->moduleptr);
			shell_exec(details->command, arguments, details->workingdir);
			break;
		}
		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, details->command));
			//Write arguments
			if (details->arguments) config_write(config_get_control_setagentprop_c(a->controlptr, a->agentaction, "Arguments", details->arguments));
			if (details->workingdir) config_write(config_get_control_setagentprop_c(a->controlptr, a->agentaction, "WorkingDir", details->workingdir));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_run_getdata(agent *a, int datatype)
{
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_run_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	make_menuitem_str(m, "Entry:", config_getfull_control_setagent_s(c, action, "Run"),
		a ? ((agenttype_run_details *) a->agentdetails)->command : "");
	make_menuitem_cmd(m, "Browse...", config_getfull_control_setagent_c(c, action, "Run", "*browse*"));

	make_menuitem_nop(m, "");
	make_menuitem_cmd(m, "Notepad", config_getfull_control_setagent_c(c, action, "Run", "notepad.exe"));
	make_menuitem_cmd(m, "Calculator", config_getfull_control_setagent_c(c, action, "Run", "calc.exe"));
	make_menuitem_cmd(m, "Command Prompt", config_getfull_control_setagent_c(c, action, "Run", "cmd.exe"));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_run_menu_context(Menu *m, agent *a)
{
	//Get the agent details
	agenttype_run_details *details = (agenttype_run_details *) a->agentdetails;

	//Arguments
	make_menuitem_str(m, "Arguments", config_getfull_control_setagentprop_s(a->controlptr, a->agentaction, "Arguments"),
		details->arguments ? details->arguments : "");

	make_menuitem_str(m, "Working Dir", config_getfull_control_setagentprop_s(a->controlptr, a->agentaction, "WorkingDir"),
		details->workingdir ? details->workingdir : "");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_run_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_run_notifytype(int notifytype, void *messagedata)
{

}


/*=================================================*/
