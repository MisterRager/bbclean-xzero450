/*===================================================

	AGENTTYPE_SWITCHEDSTATE HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_SwitchedState_h
#define BBInterface_AgentType_SwitchedState_h

//Includes
#include "AgentMaster.h"

//Define these structures
struct agenttype_switchedstate_details
{
	int datatype;
	agent *agents[3];
};

//Define these functions internally

int agenttype_switchedstate_startup();
int agenttype_switchedstate_shutdown();

int     agenttype_switchedstate_create(agent *a, char *parameterstring);
int     agenttype_switchedstate_destroy(agent *a);
int     agenttype_switchedstate_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_switchedstate_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_switchedstate_getdata(agent *a, int datatype);
void    agenttype_switchedstate_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_switchedstate_menu_context(Menu *m, agent *a);
void    agenttype_switchedstate_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
