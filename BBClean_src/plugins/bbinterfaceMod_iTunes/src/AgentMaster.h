/*===================================================

	AGENT MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentMaster_h
#define BBInterface_AgentMaster_h

//Pre-defined structures
struct agenttype;
struct agent;

//Includes
#include "ControlMaster.h"
#include "MenuMaster.h"

//Cirular dependency. Whoah. This surely needs some redesign.
struct module;

//Define these structures
struct agenttype
{
	char    agenttypenamefriendly[64];
	char    agenttypename[64];
	int     format;
	bool    writable;
	int     (*func_create)(agent *a, char *parameterstring);
	int     (*func_destroy)(agent *a);
	int     (*func_message)(agent *a, int tokencount, char *tokens[]);
	void    (*func_notify)(agent *a, int notifytype, void *messagedata);
	void*   (*func_getdata)(agent *a, int datatype);
	void    (*func_menu_set)(Menu *m, control *c, agent *a, char *action, int controlformat);
	void    (*func_menu_context)(Menu *m, agent *a);
	void    (*func_notifytype)(int notifytype, void *messagedata);
};

struct agent
{
	char agentname[256];
	char agentaction[256];
	int format;
	agenttype *agenttypeptr;        //Pointer to the type of agent
	agent *parentagentptr;          //Pointer to the parent agent (null if it is directly attached to a control)
	control *controlptr;            //Pointer to the control
	void *agentdetails;         //Pointer to details about the agent
	bool writable;
};

//Define these functions internally
int agent_startup();
int agent_shutdown();

int agent_create(agent **a, control *c, agent *parentagent, char *action, char *agenttypename, char *parameterstring, int dataformat);
int agent_destroy(agent **a);

void agent_save();

void agent_notify(agent *a, int notifytype, void *data);
void *agent_getdata(agent *a, int datatype);
int agent_message(int tokencount, char *tokens[], bool from_core, module* caller);
int agent_message(agent *a, int tokencount, char *tokens[]);
int agent_controlmessage(control *c, int tokencount, char *tokens[], int agentcount, agent *agents[], char *agentnames[], int agenttypes[]);

void agent_menu_set(Menu *m, control *c, agent *a, int controlformat, char *action);
void agent_menu_context(Menu *m, control *c, agent *a);

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
	);

void agent_unregistertype(agenttype *at);

#endif
/*=================================================*/
