/*===================================================

	AGENTTYPE_SYSTEMINFO HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_SystemInfo_h
#define BBInterface_AgentType_SystemInfo_h

//Includes
#include "AgentMaster.h"

//Define these structures
struct agenttype_systeminfo_details
{
	char *internal_identifier;
	int monitor_type;
};

//Define these functions internally

int agenttype_systeminfo_startup();
int agenttype_systeminfo_shutdown();

int     agenttype_systeminfo_create(agent *a, char *parameterstring);
int     agenttype_systeminfo_destroy(agent *a);
int     agenttype_systeminfo_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_systeminfo_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_systeminfo_getdata(agent *a, int datatype);
void    agenttype_systeminfo_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_systeminfo_menu_context(Menu *m, agent *a);
void    agenttype_systeminfo_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
