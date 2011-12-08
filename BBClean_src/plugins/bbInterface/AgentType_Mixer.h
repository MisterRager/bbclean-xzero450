/*===================================================

	AGENTTYPE_MIXER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_Mixer_h
#define BBInterface_AgentType_Mixer_h

// Global Include
#include <mmsystem.h>

//Includes
#include "AgentMaster.h"
#include "ControlMaster.h"

//Define these structures
struct agenttype_mixer_details
{
	long device;
	long line;
	long control;

	HWND hwnd_reciever;

	HMIXER mixer_handle;
	MIXERCONTROLDETAILS mixer_controldetails;

	double value_double;
	bool value_bool;
};

//Define these functions internally

int agenttype_mixer_startup();
int agenttype_mixer_shutdown();

int     agenttype_mixer_create(agent *a, char *parameterstring);
int     agenttype_mixer_destroy(agent *a);
int     agenttype_mixer_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_mixer_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_mixer_getdata(agent *a, int datatype);
void    agenttype_mixerscale_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_mixerbool_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_mixer_menu_context(Menu *m, agent *a);
void    agenttype_mixer_notifytype(int notifytype, void *messagedata);


#endif
/*=================================================*/
