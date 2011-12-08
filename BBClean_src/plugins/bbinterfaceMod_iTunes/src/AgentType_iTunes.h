/*===================================================

	AGENTTYPE_itunes HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_iTunes_h
#define BBInterface_AgentType_iTunes_h

//Includes
#include "AgentMaster.h"


//Define these structures
struct agenttype_itunes_details
{
	char *internal_identifier;
	int commandcode;
};

struct agenttype_itunes_itrackproperties
{
	char title[1024];
	char artist[1024];
	char album[1024];
	long sec;
	char time[16];
	long bitrate;
	long samplerate;
	char sbitrate[16];
	char ssamplerate[16];
	long rating;
	bool hasartwork;
};


//Define these functions internally

int agenttype_itunes_startup();
int agenttype_itunes_shutdown();

int     agenttype_itunes_create(agent *a, char *parameterstring);
int     agenttype_itunes_destroy(agent *a);
int     agenttype_itunes_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_itunes_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_itunes_getdata(agent *a, int datatype);
void    agenttype_itunes_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_itunes_menu_context(Menu *m, agent *a);
void    agenttype_itunes_notifytype(int notifytype, void *messagedata);

int     agenttype_itunespoller_create(agent *a, char *parameterstring);
void    agenttype_itunespoller_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_itunespoller_getdata(agent *a, int datatype);
void    agenttype_itunespoller_bool_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_itunespoller_scale_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_itunespoller_text_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_itunespoller_notifytype(int notifytype, void *messagedata);
void agenttype_itunesimage_notify(agent *a, int notifytype, void *messagedata);
void agenttype_itunesimage_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);



#endif 
