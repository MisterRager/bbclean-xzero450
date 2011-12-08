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
	switch(notifytype)
	{
		case NOTIFY_CHANGE:
		{
			//Send the Bro@m
			//This is an awkward way to handle it, but necessary,
			//since a broam can delete the original source
			char *temp = new_string(details->command);
			BBMessageBox(NULL, details->command, "test", MB_OK);
			PostMessage(message_window, BB_EXECUTE, 0, (LPARAM)temp);
			break;
		}

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
	make_menuitem_str(m, "Entry:", config_get_control_setagent_s(c, action, "Bro@m"),
		a ? ((agenttype_broam_details *) a->agentdetails)->command : "");

	if (0 == (controlformat & CONTROL_FORMAT_SCALE))
	{
		make_menuitem_cmd(m, "@BBShowPlugins", config_get_control_setagent_c(c, action, "Bro@m", "@BBShowPlugins"));
		make_menuitem_cmd(m, "@BBHidePlugins", config_get_control_setagent_c(c, action, "Bro@m", "@BBHidePlugins"));
		make_menuitem_cmd(m, "@BBInterface Plugin About", config_get_control_setagent_c(c, action, "Bro@m", "@BBInterface Plugin About"));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_bbicontrols_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_bbicontrols_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	//Declare variables
	list *controllist;
	listnode *currentnode;
	Menu *submenu;
	const bool btrue = true;
	const bool bfalse = false;
	char broamcommand[1024];


	//Get the control list
	controllist = control_getcontrollist();

	//Make the "Make Control Visible" menu
	submenu = make_menu(c, "Make Control Visible");
	dolist (currentnode, controllist)
	{
		control *thiscontrol = (control *) currentnode->value;
		strcpy(broamcommand, config_get_control_setwindowprop_b(thiscontrol, szWPisvisible, &btrue));
		make_menuitem_cmd(submenu, thiscontrol->controlname, config_get_control_setagent_c(c, action, "BBInterfaceControl", broamcommand));
	}
	make_submenu_item(m, "Make Control Visible", submenu);

	submenu = make_menu(c, "Make Control Invisible");
	dolist (currentnode, controllist)
	{
		control *thiscontrol = (control *) currentnode->value;
		strcpy(broamcommand, config_get_control_setwindowprop_b(thiscontrol, szWPisvisible, &bfalse));
		make_menuitem_cmd(submenu, thiscontrol->controlname, config_get_control_setagent_c(c, action, "BBInterfaceControl", broamcommand));
	}
	make_submenu_item(m, "Make Control Invisible", submenu);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_menu_context(Menu *m, agent *a)
{
	MakeMenuNOP(m, "No options available.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_broam_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_broam_notifytype(int notifytype, void *messagedata)
{

}


/*=================================================*/
