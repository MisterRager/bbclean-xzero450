/*===================================================

	AGENTTYPE_BROAM CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>

//Parent Include
#include "AgentType_Broam.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"
#include "MessageMaster.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_broam_startup()
{
	//Register this type with the ControlMaster
	agent_registertype(
		"BlackBox Bro@m",                   //Friendly name of agent type
		"Bro@m",                            //Name of agent type
		CONTROL_FORMAT_TRIGGER|CONTROL_FORMAT_DROP,  //Control type
		true,
		&agenttype_broam_create,            
		&agenttype_broam_destroy,
		&agenttype_broam_message,
		&agenttype_broam_notify,
		&agenttype_broam_getdata,
		&agenttype_broam_menu_set,
		&agenttype_broam_menu_context,
		&agenttype_broam_notifytype
		);

	//Register this type with the ControlMaster
	//This is actually just a farce - this is really the Bro@m type again
	//We use this to generate friendly menus for BBI Controls
	//Otherwise, it is exactly the same as Bro@m
	agent_registertype(
		"BBInterface Control",              //Friendly name of agent type
		"BBInterfaceControl",               //Name of agent type
		CONTROL_FORMAT_TRIGGER|CONTROL_FORMAT_DROP,  //Control type
		true,
		&agenttype_broam_create,            
		&agenttype_broam_destroy,
		&agenttype_broam_message,
		&agenttype_broam_notify,
		&agenttype_broam_getdata,
		&agenttype_broam_bbicontrols_menu_set,
		&agenttype_broam_menu_context,
		&agenttype_broam_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_broam_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_broam_create(agent *a, char *parameterstring)
{
	if (0 == * parameterstring)
		return 2; // no param, no agent

	//Create the details
	agenttype_broam_details *details = new agenttype_broam_details;
	a->agentdetails = (void *)details;

	//Copy the parameter string
	details->command = new_string(parameterstring);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_broam_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_broam_details *details = (agenttype_broam_details *) a->agentdetails;
		free_string(&details->command);
		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_broam_message(agent *a, int tokencount, char *tokens[])
{
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_broam_details *details = (agenttype_broam_details *) a->agentdetails;
	char *buffer, *varptr, *endptr;
	bool gotdata;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			varptr = NULL;

			//For now, just look for $BroadcastValue$ symbol
			//This will be replaced by the controls broadcast value
			varptr = strstr(details->command, "$BroadcastValue$");
			if (varptr != NULL)
			{
				//Build a stage for this new string
				char *stage = new char[MAX_LINE_LENGTH];

				//Copy characters from the command to the stage
				strncpy(stage, details->command, varptr - details->command);
				stage[varptr - details->command] = '\0';
				endptr = stage + strlen(stage);

				//Get the broadcast value from the agent
				gotdata = control_getstringdata(a->controlptr, endptr, "BroadcastValue");

				//Add the rest of the string
				strcat(stage, varptr + strlen("$BroadcastValue$"));				

				//Copy the final to the buffer
				buffer = new_string(stage);				

				//Delete the stage
				delete stage;
			}
			else
			{
				buffer = new_string(details->command);
			}
			
			// NOTE: To make broams local to modules, check if the message is a BBI broam,
			// change the current module to the one the control the agent belongs to is in,
			// process the message, then revert.
			message_interpret(buffer,false,a->controlptr->moduleptr);

			// NOTE: Is a->controlptr guaranteed to work even with agents that are not on top of the agent hierachy? (compound agents, for example)
			// Not too sure on this change, so...
			// PostMessage(message_window, BBI_POSTCOMMAND, 0, (LPARAM) buffer);
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, details->command));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_broam_getdata(agent *a, int datatype)
{
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	make_menuitem_str(m, "Entry:", config_getfull_control_setagent_s(c, action, "Bro@m"),
		a ? ((agenttype_broam_details *) a->agentdetails)->command : "");

	make_menuitem_cmd(m, "@BBShowPlugins", config_getfull_control_setagent_c(c, action, "Bro@m", "@BBShowPlugins"));
	make_menuitem_cmd(m, "@BBHidePlugins", config_getfull_control_setagent_c(c, action, "Bro@m", "@BBHidePlugins"));
	make_menuitem_cmd(m, "@BBInterface Plugin About", config_getfull_control_setagent_c(c, action, "Bro@m", "@BBInterface Plugin About"));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_bbicontrols_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_bbicontrols_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	//Declare variables
	listnode *ln, *ln2;
	Menu *sub, *sub2;
	int n;
	const bool btrue = true;
	const bool bfalse = false;
	char broamcommand[1024];

	// NOTE: a better solution would be to have a boolean toggle for the controls...

	sub = make_menu("Global",c);
	n = 0;
	dolist (ln2, globalmodule.controllist)
	{
		//Get the control
		control *thiscontrol = (control *) ln2->value;

		sub2 = make_menu("Visibility", thiscontrol);
		strcpy(broamcommand, config_getfull_control_setwindowprop_b(thiscontrol, szWPisvisible, &btrue));
		make_menuitem_cmd(sub2, "Visible", config_get_control_setagent_c(c, action, "BBInterfaceControl", broamcommand));
		strcpy(broamcommand, config_getfull_control_setwindowprop_b(thiscontrol, szWPisvisible, &bfalse));
		make_menuitem_cmd(sub2, "Invisible", config_get_control_setagent_c(c, action, "BBInterfaceControl", broamcommand));

		make_submenu_item(sub,thiscontrol->controlname,sub2);
		++n;
	}
	if (n==0) make_menuitem_nop(sub,"(None.)");
	make_submenu_item(m,"Global",sub);

	dolist (ln, modulelist)
	{       
		module *mod = (module *) ln->value;
		sub = make_menu(mod->name,c);
		n = 0;
		if (mod->enabled) // don't bother with disabled modules
		dolist (ln2, mod->controllist)
		{
			//Get the control
			control *thiscontrol = (control *) ln2->value;
			sub2 = make_menu("Visibility", thiscontrol);
			strcpy(broamcommand, config_getfull_control_setwindowprop_b(thiscontrol, szWPisvisible, &btrue));
			make_menuitem_cmd(sub2, "Visible", config_get_control_setagent_c(c, action, "BBInterfaceControl", broamcommand));
			strcpy(broamcommand, config_getfull_control_setwindowprop_b(thiscontrol, szWPisvisible, &bfalse));
			make_menuitem_cmd(sub2, "Invisible", config_get_control_setagent_c(c, action, "BBInterfaceControl", broamcommand));

			make_submenu_item(sub,thiscontrol->controlname,sub2);
			++n;
		}
		if (n==0) make_menuitem_nop(sub,"(None.)");
		make_submenu_item(m,mod->name,sub);
	}

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_menu_context(Menu *m, agent *a)
{
	make_menuitem_nop(m, "No options available.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_notifytype(int notifytype, void *messagedata)
{

}

/*=================================================*/
