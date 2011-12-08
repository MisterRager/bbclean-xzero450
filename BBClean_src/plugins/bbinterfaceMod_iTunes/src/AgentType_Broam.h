/*===================================================

	AGENTTYPE_BROAM HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_Broam_h
#define BBInterface_AgentType_Broam_h

//Includes
#include "AgentMaster.h"

//Define these structures
struct agenttype_broam_details
{
	char *command;
	int minval;
	int maxval;
};

//Define these functions internally

int agenttype_broam_startup();
int agenttype_broam_shutdown();

int     agenttype_broam_create(agent *a, char *parameterstring);
int     agenttype_broam_destroy(agent *a);
int     agenttype_broam_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_broam_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_broam_getdata(agent *a, int datatype);
void    agenttype_broam_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_broam_bbicontrols_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_broam_menu_context(Menu *m, agent *a);
void    agenttype_broam_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
