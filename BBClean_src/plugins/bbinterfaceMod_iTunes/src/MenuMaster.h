/*===================================================

	MENU MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_MenuMaster_h
#define BBInterface_MenuMaster_h

//Includes
#include "ControlMaster.h"
#include "AgentMaster.h"

//Define these functions internally
int menu_startup();
int menu_shutdown();

void menu_control(control *c, bool pop);
void menu_update_global(void);
void menu_update_modules(void);
void menu_update_editmodules(void);
void menu_update_setactivemodule(void);
void menu_controloptions(Menu *m, control *c, int count, agent *agents[], char *actionprefix, char *actions[], const int types[]);
Menu* menu_control_submenu(control *c);

struct module;
Menu *make_menu(const char *title, control *c = NULL);
Menu *make_menu(const char *title, module *m);
// Use a single argument for global menus, provide an addition control/module pointer, if it makes sense
void make_menuitem_str(Menu *m, const char *title, const char* cmd, const char * init_string);
void make_menuitem_int(Menu *m, const char *title, const char* cmd, int val, int minval, int maxval);
void make_menuitem_bol(Menu *m, const char *title, const char* cmd, bool checked);
void make_menuitem_cmd(Menu *m, const char *title, const char* cmd);
void make_menuitem_nop(Menu *m, const char *title);
void make_submenu_item(Menu *m, const char *title, Menu *sub);
void show_menu(control *c, Menu *m, bool pop);

#endif
/*=================================================*/
