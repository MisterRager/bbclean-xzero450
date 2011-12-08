/*===================================================

	MODULE MASTER HEADERS

===================================================*/
// Important: modules can no longer depend on an external control list - there is NO such thing anymore.

//Multiple definition prevention
#ifndef BBInterface_ModuleMaster_h
#define BBInterface_ModuleMaster_h

//Includes
#include "WindowMaster.h"
#include "AgentMaster.h"

//Definitions
#define MODULE_ACTION_ONLOAD 0
#define MODULE_ACTION_ONUNLOAD 1

#define MODULE_ACTION_COUNT 2


//Define these structures
struct module
{
	//Info fields - the first one is necessary.
	char name[64];
	char *author;
	char *comments;

	char *filepath;       //path to the module file
	bool enabled;

	list *controllist; // list of controls associated with module
	list *controllist_parentsonly; // list of controls associated with module
	list *variables; // list of variables associated with module

	char *actions[MODULE_ACTION_COUNT];
};

extern module *currentmodule;
extern module globalmodule;

//Define these functions internally
int module_startup();
int module_shutdown();

module* module_create(char *filepath);
module* module_create_new(char *filename);
int module_destroy(module *m, bool remove_from_list);
int module_toggle(module *m);
int module_toggle(char *modulename);

int module_message(int tokencount, char *tokens[], bool from_core, module* caller);
bool module_state(char *modulename);

Menu* module_menu_modulelist();
Menu* module_menu_editmodules();
Menu* module_menu_setactivemodule();

void module_save_list();
void module_save_all();
module* module_get(char* key);


extern list *modulelist;

#endif
/*=================================================*/
