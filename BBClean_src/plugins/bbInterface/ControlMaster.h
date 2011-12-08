/*===================================================

	CONTROL MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_ControlMaster_h
#define BBInterface_ControlMaster_h

//Pre-defined structures
struct controltype;
struct control;
struct controlchild;

//Includes
#include "WindowMaster.h"
#include "AgentMaster.h"
#include "ListMaster.h"
#include "ModuleMaster.h"

//Cirular dependency. Whoah. This surely needs some redesign.
struct module;

//Definitions

//Define these structures
struct controltype
{
	char    controltypename[64];
	bool    can_parentless;
	bool    can_parent;
	bool    can_child;
	char    id;
	int     (*func_create)(control *c);
	int     (*func_destroy)(control *c);
	LRESULT (*func_event)(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void    (*func_notify)(control *c, int notifytype, void *messagedata);
	int     (*func_message)(control *c, int tokencount, char *tokens[]);
	void*   (*func_getdata)(control *c, int datatype);
	bool    (*func_getstringvalue)(control *c, char *buffer, char *propertyname);
	void    (*func_menu_context)(Menu *m, control *c);
	void    (*func_notifytype)(int notifytype, void *messagedata);
};

struct control
{
	char controlname[64];       //UNIQUE name of the control

	controltype *controltypeptr;    //Pointer to the type of control
	control *parentptr;         //Pointer to the parent control
	window *windowptr;              //Pointer to the control's window
	module *moduleptr;			//Pointer to the module the control is associated to

	void *controldetails;       //Pointer to details about the control

	controlchild *firstchild;
	controlchild *lastchild;
	controlchild *mychildnode;
};

struct controlchild
{
	control *controlptr;
	controlchild *nextchildptr;
	controlchild *prevchildptr;
};

enum
{
	CONTROL_ID_FRAME = 1,
	CONTROL_ID_LABEL,
	CONTROL_ID_BUTTON,
	CONTROL_ID_SWITCHBUTTON,
	CONTROL_ID_SLIDER
};


//Define these functions internally
int control_startup();
int control_shutdown();

extern module* currentmodule;

int control_create(char *controlname, char *controltypename, char *controlparentname, bool include_parent, module* parentmodule);
int control_destroy(control *c, bool remove_from_list, bool save_last);
int control_rename(control *c, char *newname);
bool control_make_childof(control *c, const char *parentname);
bool control_make_parentless(control *c);
control* control_get(const char *name, module* caller = currentmodule);


LRESULT control_event(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int control_message(int tokencount, char *tokens[], bool from_core, module* caller);
void control_notify(control *c, int notifytype, void *messagedata);

void control_pluginsvisible(bool arevisible);
void control_invalidate(void);

void control_menu_create(Menu *m, control *c, bool createchild);
void control_menu_context(Menu *m, control *c);
void control_menu_settings(Menu *m, control *c);

bool control_getstringdata(control *c, char *buffer, char *propertyname);

void control_save();

void control_checklast();

void control_registertype(
	char    *controltypename,
	bool    can_parentless,
	bool    can_parent,
	bool    can_child,
	char    id,
	int     (*func_create)(control *c),
	int     (*func_destroy)(control *c),
	LRESULT (*func_event)(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam),
	void    (*func_notify)(control *c, int notifytype, void *messagedata),
	int     (*func_message)(control *c, int tokencount, char *tokens[]),
	void*   (*func_getdata)(control *c, int datatype),
	bool    (*func_getstringvalue)(control *c, char *buffer, char *propertyname),
	void    (*func_menu_context)(Menu *m, control *c),
	void    (*func_notifytype)(int notifytype, void *messagedata)
	);
void control_unregistertype(controltype *ct);


struct token_check { const char *key; unsigned int id; int args; };
int token_check(struct token_check *t, int *curtok, int tokencount, char *tokens[]);
int get_string_index (const char *key, const char **string_list);
char *new_string(const char *);
void free_string(char **s);
char *extract_string(char *d, const char *s, int n);
char* unquote(char *d, const char *s);

void variables_startup(void);
void variables_shutdown(void);
void variables_save(void);
const char *variables_get(const char *key, const char *deflt, module* defmodule);
void variables_set(bool is_static, const char *key, const char *val, module* defmodule = currentmodule);
void variables_set(bool is_static, const char *key, int val, module* defmodule = currentmodule);
char *get_dragged_file(char *buffer, WPARAM wParam);

void controls_clickraise(void);
list* control_getcontrollist(void);

#endif
/*=================================================*/
