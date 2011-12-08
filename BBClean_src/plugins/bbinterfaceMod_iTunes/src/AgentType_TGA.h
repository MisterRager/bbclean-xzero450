/*===================================================

	AGENTTYPE_TGA HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_TGA_h
#define BBInterface_AgentType_TGA_h

//Includes
#include "AgentMaster.h"

//Define these structures
struct agenttype_tga_details
{
	int width;
	int height;
	char *filename;
	HANDLE image;
};

//Define these functions internally

int agenttype_tga_startup();
int agenttype_tga_shutdown();

int     agenttype_tga_create(agent *a, char *parameterstring);
int     agenttype_tga_destroy(agent *a);
int     agenttype_tga_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_tga_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_tga_getdata(agent *a, int datatype);
void    agenttype_tga_menu_set(Menu *m, control *c, agent *a, char *action, int controlformat);
void    agenttype_tga_menu_context(Menu *m, agent *a);
void    agenttype_tga_notifytype(int notifytype, void *messagedata);

void    agenttype_icon_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
int     agenttype_icon_create(agent *a, char *parameterstring);

#endif
/*=================================================*/
