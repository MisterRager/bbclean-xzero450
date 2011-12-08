/*===================================================

	AGENTTYPE_COMPOUNDTEXT HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_CompoundText_h
#define BBInterface_AgentType_CompoundText_h

//Includes
#include "AgentMaster.h"



//Define these structures
#define AGENTTYPE_COMPOUNDTEXT_MAXAGENTS 10
struct agenttype_compoundtext_details
{
	char *text;
	char finaltext[1024];

	agent *agents[AGENTTYPE_COMPOUNDTEXT_MAXAGENTS];
};

//Define these functions internally

int agenttype_compoundtext_startup();
int agenttype_compoundtext_shutdown();

int     agenttype_compoundtext_create(agent *a, char *parameterstring);
int     agenttype_compoundtext_destroy(agent *a);
int     agenttype_compoundtext_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_compoundtext_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_compoundtext_getdata(agent *a, int datatype);
void    agenttype_compoundtext_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_compoundtext_menu_context(Menu *m, agent *a);
void    agenttype_compoundtext_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
