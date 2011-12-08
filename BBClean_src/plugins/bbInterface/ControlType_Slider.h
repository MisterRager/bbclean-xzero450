/*===================================================

	CONTROLTYPE_SLIDER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_ControlType_Slider_h
#define BBInterface_ControlType_Slider_h

//Includes
#include "ControlMaster.h"

//Define these structures
struct controltype_slider_details
{
	//Value
	double value;

	//Agent
	agent *agents[4];

	//State
	bool dragging;
	bool reversed;
	bool vertical;

	int appearance;

	bool track_needsupdate;
	RECT track;
	RECT track_clickable;
	RECT track_countable;
	int track_length;
	int track_clickable_length;
	int track_countable_length;

	bool knob_needsupdate;
	RECT knob;
	int knob_offset;
	int knob_length;
	int knob_maxlength;

	bool ignore_killfocus;
	bool focus_lost;

	int broadcast_value_minimum;
	int broadcast_value_maximum;
};

//Define these functions internally
int controltype_slider_startup();
int controltype_slider_shutdown();
int controltype_slider_create(control *c);
int controltype_slider_destroy(control *c);
LRESULT controltype_slider_event(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void controltype_slider_notify(control *c, int notifytype, void *messagedata);
int controltype_slider_message(control *c, int tokencount, char *tokens[]);
void *controltype_slider_getdata(control *c, int datatype);
bool controltype_slider_getstringdata(control *c, char *buffer, char *propertyname);
void controltype_slider_menu_context(Menu *m, control *c);
void controltype_slider_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
