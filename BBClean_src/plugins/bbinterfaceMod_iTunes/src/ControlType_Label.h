/*===================================================

	CONTROLTYPE_LABEL HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_ControlType_Label_h
#define BBInterface_ControlType_Label_h

//Includes
#include "ControlMaster.h"
#include "AgentMaster.h"

//Define these structures
struct controltype_label_details
{
	agent *agents[4];
	char *caption;

	int valign;
	int halign;
	UINT settings;

	bool is_frame;
	bool has_titlebar;
	bool is_locked;

	// slit support
	struct PluginInfo *plugin_info;
	struct ModuleInfo *module_info;
};

//Define these functions internally
int controltype_label_startup();
int controltype_label_shutdown();
int controltype_label_create(control *c);
int controltype_label_destroy(control *c);
LRESULT controltype_label_event(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void controltype_label_notify(control *c, int notifytype, void *messagedata);
int controltype_label_message(control *c, int tokencount, char *tokens[]);
void *controltype_label_getdata(control *c, int datatype);
bool controltype_label_getstringdata(control *c, char *buffer, char *propertyname);
void controltype_label_menu_context(Menu *m, control *c);
void controltype_label_notifytype(int notifytype, void *messagedata);

int controltype_frame_create(control *c);
void *controltype_frame_getdata(control *c, int datatype);

#endif
/*=================================================*/
