/*===================================================

	AGENT MASTER CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>

//Parent Include
#include "AgentMaster.h"

//Includes
#include "Definitions.h"
#include "PluginMaster.h"
#include "StyleMaster.h"
#include "WindowMaster.h"
#include "ListMaster.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"

//Local variables
list *agenttypelist = NULL;
list *agentlist = NULL;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agent_startup()
{
	agentlist = list_create();
	agenttypelist = list_create();

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agent_shutdown()
{
	//All agents should be destroyed at this point
	list_destroy(agentlist);

	//Destroy each agent type
	listnode *ln;
	dolist (ln, agenttypelist)
	{
		agent_unregistertype((agenttype *) ln->value);
	}

	//Destroy the agent type node
	list_destroy(agenttypelist);

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agent_create(agent **a, control *c, agent *parentagent, char *action, char *agenttypename, char *parameterstring, int dataformat)
{
	//Create a new control
	agent *agentpointer;
	agentpointer = new agent;

	//Set default pointers
	agentpointer->parentagentptr = parentagent;
	agentpointer->controlptr = c;
	agentpointer->format = dataformat;
	agentpointer->agentdetails = NULL;
	agentpointer->agenttypeptr = NULL;	

	//Identify & set the agenttype
	agentpointer->agenttypeptr = (agenttype *) list_lookup(agenttypelist, agenttypename);
	if (agentpointer->agenttypeptr == NULL || !(agentpointer->agenttypeptr->format & dataformat))
	{
		if (!plugin_suppresserrors)
			BBMessageBox(NULL, "Error:  Agent type invalid!", szAppName, MB_OK|MB_SYSTEMMODAL);
		agent_destroy(&agentpointer);
		return 1;
	}

	//Set the writable flag
	agentpointer->writable = agentpointer->agenttypeptr->writable;

	//Type-specific creation details
	int result = (agentpointer->agenttypeptr->func_create)(agentpointer, parameterstring);
	if (result)
	{
		agent_destroy(&agentpointer);
		return result;
	}

	//Add to the list of agents
	char temp[1000];
	if (parentagent == NULL)
		sprintf(temp, "%s:%s.%s", c->moduleptr->name, c->controlname, action);
	else	
		sprintf(temp, "%s.%s", parentagent->agentname, action);

	if (list_add(agentlist, temp, (void *) agentpointer, NULL))
	{
		if (!plugin_suppresserrors)
			BBMessageBox(NULL, "Error:  Could not add agent to list!", temp, MB_OK|MB_SYSTEMMODAL);
		agent_destroy(&agentpointer);
		return 1;
	}

	//If the agent name is too long
	if (strlen(temp) > 255)
	{
		if (!plugin_suppresserrors)
			BBMessageBox(NULL, "Error:  Agent name is too long!\n\n(You can only embed agents so many levels deep.)", szAppName, MB_OK|MB_SYSTEMMODAL);
		agent_destroy(&agentpointer);
		return 1;
	}
	
	//Copy the agent name
	strcpy(agentpointer->agentname, temp);

	//Create the agent action
	sprintf(temp, "%s%s%s",
		(parentagent == NULL ? "" : parentagent->agentaction), 
		(parentagent == NULL ? "" : "."), 
		action);

	//Copy the agent action
	strcpy(agentpointer->agentaction, temp);	

	*a = agentpointer;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agent_destroy(agent **pa)
{
	agent *a = *pa;

	//If no agent, forget it
	if (NULL == a) return 0;

	// clear out variable
	*pa = NULL;

	//Remove it from the list
	list_remove(agentlist, a->agentname);

	//Type specific destroy
	if (a->agenttypeptr) (a->agenttypeptr->func_destroy)(a);

	//Delete the agent itself
	delete a;

	//New errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_save
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agent_save()
{
	//Save all agent type specific data
	agenttype *at;

	listnode *ln;
	dolist (ln, agenttypelist)
	{
		at = (agenttype *) ln->value;
		(at->func_notifytype)(NOTIFY_SAVE_AGENTTYPE, NULL);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agent_notify(agent *a, int notifytype, void *data)
{
	//If no agent exists, forget about it
	if (a == NULL) return;

	//Notify the agent itself
	(a->agenttypeptr->func_notify)(a, notifytype, data);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agent_message(int tokencount, char *tokens[], bool from_core, module* caller)
{
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agent_message(agent *a, int tokencount, char *tokens[])
{
	//If no agent exists, forget about it
	if (a == NULL) return 1;

	//Notify the agent itself
	return (a->agenttypeptr->func_message)(a, tokencount, tokens);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_controlmessage
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agent_controlmessage(control *c, int tokencount, char *tokens[], int agentcount, agent *agents[], char *agentnames[], int agenttypes[])
{
	//Figure out what it is
	int action = 0;
	if (tokencount == 5 && !strcmp(tokens[2], szBActionRemoveAgent)) action = 1;
	else if (tokencount == 7 && !strcmp(tokens[2], szBActionSetAgent)) action = 2;
	else if (tokencount == 7 && !strcmp(tokens[2], szBActionSetAgentProperty)) action = 3;
	else return 1;

	//Get the agent name we are trying to set
	char *fullactionstring = tokens[4];

	//Find the index of the first "." in the message
	char *dotptr = strstr(fullactionstring, ".");

	//If there is a dot, set the character to null, so we can find the first one
	if (dotptr != NULL)
	{
		dotptr[0] = '\0';
	}

	//The agent we are trying to find
	agent **targetagentptr = NULL;
	int targetagenttype = 0;
	char *targetactionstring = NULL;
	agent *parentagent = NULL;

	//Figure out which agent
	int agentindex = -1;
	for (int i = 0; i < agentcount; i++)
	{
		if (!strcmp(fullactionstring, agentnames[i])) {agentindex = i; i = agentcount;}
	}

	//If we couldn't find it, return failure
	if (agentindex == -1) return 1;

	//We found the target agent and type (at least for the control's agent)
	targetagentptr = &agents[agentindex];
	targetagenttype = agenttypes[agentindex];
	targetactionstring = fullactionstring;

	//If we had a dot, we need to keep going
	while (dotptr != NULL) 
	{
		//Set the parent agent to the last agent we found
		parentagent = *targetagentptr;

		//Get the rest of the string
		char *restofstring = dotptr+1;

		//Is there a dot here?  If so, chop it off in the same manner as before
		dotptr = strstr(restofstring, ".");
		if (dotptr != NULL)
		{
			dotptr[0] = '\0';
		}

		//Find the agent that this string points to by searching the agent's sub-agents
		int *agentcountptr = (int *) agent_getdata(*targetagentptr, DATAFETCH_SUBAGENTS_COUNT);
		if (agentcountptr == NULL || *agentcountptr <= 0) return 1;
		int subagentcount = *agentcountptr;

		//Get the other pointers
		char **agentnames = (char **) agent_getdata(*targetagentptr, DATAFETCH_SUBAGENTS_NAMES_ARRAY);
		int *agenttypes = (int *) agent_getdata(*targetagentptr, DATAFETCH_SUBAGENTS_TYPES_ARRAY);
		agent **agents = (agent **) agent_getdata(*targetagentptr, DATAFETCH_SUBAGENTS_POINTERS_ARRAY);
		if (agentnames == NULL || agenttypes == NULL || agents == NULL) return 1;

		//Figure out which agent
		agentindex = -1;
		for (int i = 0; i < *agentcountptr; i++)
		{
			//If it matches the subagent name
			if (!strcmp(restofstring, agentnames[i])) {agentindex = i; i = *agentcountptr;}
		}
		if (agentindex == -1) return 1;

		//If we found it
		targetagentptr = &agents[agentindex];
		targetagenttype = agenttypes[agentindex];
		targetactionstring = restofstring;
	}

	//If set agent property, pass on
	if (action == 3)
	{
		return agent_message(*targetagentptr, tokencount, tokens);
	}
	
	//In delete or create, delete old agent
	agent_destroy(targetagentptr);

	//Variable to store the result of the create action
	int result = 0;

	//If we're creating, create it
	if (action == 2)        
	{           
		result = agent_create(targetagentptr, c, parentagent, targetactionstring, tokens[5], tokens[6], targetagenttype);
	}

	//Notify the control to update
	control_notify(c, NOTIFY_DRAGACCEPT, NULL);
	control_notify(c, NOTIFY_NEEDUPDATE, NULL);

	//No errors
	return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agent_getdata(agent *a, int datatype)
{
	//If no agent exists, forget about it
	if (a == NULL) return NULL;

	//Get the data from the agent
	return (a->agenttypeptr->func_getdata)(a, datatype);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agent_menu_set(Menu *m, control *c, agent *a, int controlformat, char *action)
{
	bool has_nothing = true;

	//Go through all agenttypes
	listnode *ln;
	dolist (ln, agenttypelist)
	{
		agenttype *at = (agenttype *) ln->value;
		if (at->format & controlformat)
		{
			Menu *submenu = make_menu(at->agenttypenamefriendly, c);
			agent *a_set = a && at == a->agenttypeptr ? a : NULL;
			if (a_set) has_nothing = false;
			(at->func_menu_set)(submenu, c, a_set, action, controlformat);
			make_submenu_item(m, at->agenttypenamefriendly, submenu);
		}
	}

	//Add the "Nothing" option
	make_menuitem_bol(m, "Nothing", config_getfull_control_removeagent(c, action), has_nothing);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agent_menu_context(Menu *m, control *c, agent *a)
{
	if (a) (a->agenttypeptr->func_menu_context)(m, a);
	else make_menuitem_nop(m, "Not set.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_registertype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agent_registertype(
	char    *agenttypenamefriendly,
	char    *agenttypename,
	int     format,
	bool    writable,
	int     (*func_create)(agent *a, char *parameterstring),
	int     (*func_destroy)(agent *a),
	int     (*func_message)(agent *a, int tokencount, char *tokens[]),
	void    (*func_notify)(agent *a, int notifytype, void *messagedata),    
	void*   (*func_getdata)(agent *a, int datatype),
	void    (*func_menu_set)(Menu *m, control *c, agent *a, char *action, int controlformat),
	void    (*func_menu_context)(Menu *m, agent *a),
	void    (*func_notifytype)(int notifytype, void *messagedata)
	)
{
	//Error conditions
	if (strlen(agenttypenamefriendly) >= 64) return;
	if (strlen(agenttypename) >= 64) return;

	//Copy the data
	agenttype *agenttypepointer;
	agenttypepointer = new agenttype;

	strcpy(agenttypepointer->agenttypenamefriendly, agenttypenamefriendly);
	strcpy(agenttypepointer->agenttypename, agenttypename);
	
	agenttypepointer->format = format;
	agenttypepointer->writable = writable;
	
	agenttypepointer->func_create = func_create;
	agenttypepointer->func_destroy = func_destroy;
	agenttypepointer->func_message = func_message;
	agenttypepointer->func_notify = func_notify;
	agenttypepointer->func_getdata = func_getdata;
	agenttypepointer->func_menu_set = func_menu_set;
	agenttypepointer->func_menu_context = func_menu_context;
	agenttypepointer->func_notifytype = func_notifytype;

	list_add(agenttypelist, agenttypename, (void *) agenttypepointer, NULL);

	return;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agent_unregistertype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agent_unregistertype(agenttype *at)
{
	//Delete the agenttype
	delete at;
}
