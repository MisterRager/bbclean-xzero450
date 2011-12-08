/*===================================================

	AGENTTYPE_NETWORKMONITOR HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_NetworkMonitor_h
#define BBInterface_AgentType_NetworkMonitor_h

//Includes
#include "AgentMaster.h"

//Define these structures

struct agenttype_networkmonitor_details
{
	char *internal_identifier;
	int monitor_types;
	int monitor_interface_number;
};



//Define these functions internally

int agenttype_networkmonitor_startup();
int agenttype_networkmonitor_shutdown();

int     agenttype_networkmonitor_create(agent *a, char *parameterstring);
int     agenttype_networkmonitor_destroy(agent *a);
int     agenttype_networkmonitor_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_networkmonitor_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_networkmonitor_getdata(agent *a, int datatype);
void    agenttype_networkmonitor_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_networkmonitor_menu_context(Menu *m, agent *a);
void    agenttype_networkmonitor_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
