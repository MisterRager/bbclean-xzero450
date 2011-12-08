/*===================================================

	AGENTTYPE_WINAMP HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_Winamp_h
#define BBInterface_AgentType_Winamp_h

//Includes
#include "AgentMaster.h"

//Define these structures
struct agenttype_winamp_details
{
	char *internal_identifier;
	int commandcode;
};

//Define these functions internally

int agenttype_winamp_startup();
int agenttype_winamp_shutdown();

int     agenttype_winamp_create(agent *a, char *parameterstring);
int     agenttype_winamp_destroy(agent *a);
int     agenttype_winamp_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_winamp_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_winamp_getdata(agent *a, int datatype);
void    agenttype_winamp_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_winamp_menu_context(Menu *m, agent *a);
void    agenttype_winamp_notifytype(int notifytype, void *messagedata);

int     agenttype_winamppoller_create(agent *a, char *parameterstring);
void    agenttype_winamppoller_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_winamppoller_getdata(agent *a, int datatype);
void    agenttype_winamppoller_bool_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_winamppoller_scale_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_winamppoller_text_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_winamppoller_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
