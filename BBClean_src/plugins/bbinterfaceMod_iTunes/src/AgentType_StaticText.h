/*===================================================

	AGENTTYPE_STATICTEXT HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_StaticText_h
#define BBInterface_AgentType_StaticText_h

//Includes
#include "AgentMaster.h"

//Define these structures
struct agenttype_statictext_details
{
	char *text;
};

//Define these functions internally

int agenttype_statictext_startup();
int agenttype_statictext_shutdown();

int     agenttype_statictext_create(agent *a, char *parameterstring);
int     agenttype_statictext_destroy(agent *a);
int     agenttype_statictext_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_statictext_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_statictext_getdata(agent *a, int datatype);
void    agenttype_statictext_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_statictext_menu_context(Menu *m, agent *a);
void    agenttype_statictext_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
