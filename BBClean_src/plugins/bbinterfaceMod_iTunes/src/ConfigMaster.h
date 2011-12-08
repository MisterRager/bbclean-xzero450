/*===================================================

	CONFIG MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_ConfigMaster_h
#define BBInterface_ConfigMaster_h

//Includes
#include "AgentMaster.h"
#include "ControlMaster.h"
#include "Definitions.h"
#include "ModuleMaster.h"

//Only needed because variables store themselves directly in a listnode.
#include "ListMaster.h"

//Global variables
extern char *config_path_plugin;
extern char *config_path_mainscript;
extern char *config_path_loadscript;
extern FILE * config_file_out;

//Functions
int config_startup();
int config_shutdown();


FILE *config_open(char *filename, const char *mode);
int config_save(char *filename);
int config_delete(char *filename);
int config_load(char *filename, module* caller, const char *section = NULL);
int config_write(char *string);
int config_backup(char *filename);
char* config_makepath(char *buffer, char *filename);

void config_printf (const char *fmt, ...);
void config_printf_noskip (const char *fmt, ...);
int check_mainscript_filetime(void);
bool check_mainscript_version(void);


bool config_set_long(char *string, long *valptr);
bool config_set_int(const char *string, int *valptr);
bool config_set_int_expr(char *string, int *valptr);
bool config_set_str(char *string, char **valptr);
bool config_set_double(char *string, double *valptr);
bool config_set_double_expr(char *str, double* value);
bool config_set_double_expr(char *str, double* value, double min, double max);
bool config_set_long(char *string, long *valptr, long min, long max);
bool config_set_int(const char *string, int *valptr, int min, int max);
bool config_set_double(char *string, double *valptr, double min, double max);
bool config_set_bool(char *string, bool *valptr);
bool config_isstringzero(const char *string);

char *config_get_control_create(controltype *ct);
char *config_get_control_create_named(controltype *ct, control *c);
char *config_get_control_create_child(control *c_p, controltype *ct);
char *config_get_control_create_child_named(control *c_p, controltype *ct, control *c);
char *config_get_control_saveas(control *c, const char *filename);
char *config_get_control_delete(control *c);
char *config_get_control_clone(control *c);

char *config_get_control_setagent_s(control *c, const char *action, const char *agenttype);
char *config_get_control_setagent_c(control *c, const char *action, const char *agenttype, const char *value);
char *config_get_control_setagent_b(control *c, const char *action, const char *agenttype, const bool *value);
char *config_get_control_setagent_i(control *c, const char *action, const char *agenttype, const int *value);
char *config_get_control_setagent_d(control *c, const char *action, const char *agenttype, const double *value);
char *config_get_control_removeagent(control *c, const char *action);
char *config_get_control_renamecontrol_s(control *c);
char *config_get_control_setagentprop_s(control *c, const char *action, const char *key);
char *config_get_control_setagentprop_c(control *c, const char *action, const char *key, const char *value);
char *config_get_control_setagentprop_b(control *c, const char *action, const char *key, const bool *value);
char *config_get_control_setagentprop_i(control *c, const char *action, const char *key, const int *value);
char *config_get_control_setagentprop_d(control *c, const char *action, const char *key, const double *value);
char *config_get_control_setcontrolprop_s(control *c, const char *key);
char *config_get_control_setcontrolprop_c(control *c, const char *key, const char *value);
char *config_get_control_setcontrolprop_b(control *c, const char *key, const bool *value);
char *config_get_control_setcontrolprop_i(control *c, const char *key, const int *value);
char *config_get_control_setcontrolprop_d(control *c, const char *key, const double *value);
char *config_get_control_setwindowprop_s(control *c, const char *key);
char *config_get_control_setwindowprop_c(control *c, const char *key, const char *value);
char *config_get_control_setwindowprop_b(control *c, const char *key, const bool *value);
char *config_get_control_setwindowprop_i(control *c, const char *key, const int *value);
char *config_get_control_setwindowprop_d(control *c, const char *key, const double *value);
char *config_get_agent_setagentprop_s(const char *agenttype, const char *key);
char *config_get_agent_setagentprop_c(const char *agenttype, const char *key, const char *value);
char *config_get_agent_setagentprop_b(const char *agenttype, const char *key, const bool *value);
char *config_get_agent_setagentprop_i(const char *agenttype, const char *key, const int *value);
char *config_get_agent_setagentprop_d(const char *agenttype, const char *key, const double *value);

char *config_get_control_setpluginprop_s(control *c, const char *key);
char *config_get_control_setpluginprop_b(control *c, const char *key, const bool *value);

char *config_get_plugin_setpluginprop_s(const char *key);
char *config_get_plugin_setpluginprop_c(const char *key, const char *value);
char *config_get_plugin_setpluginprop_b(const char *key, const bool *value);
char *config_get_plugin_setpluginprop_i(const char *key, const int *value);
char *config_get_plugin_setpluginprop_d(const char *key, const double *value);
char *config_get_plugin_load_dialog();
char *config_get_plugin_load(const char *file);
char *config_get_plugin_save();
char *config_get_plugin_saveas();
char *config_get_plugin_revert();
char *config_get_plugin_edit();

char *config_get_module_create();
char *config_get_module_load_dialog();
char *config_get_module_load(module *m);
char *config_get_module_toggle(module *m);
char *config_get_module_edit(module *m);
char *config_get_module_setdefault(module *m);

char *config_get_module_onload_s(module *m);
char *config_get_module_onunload_s(module *m);
char *config_get_module_setauthor_s(module *m);
char *config_get_module_setcomments_s(module *m);
char *config_get_module_rename_s(module *m);

char *config_get_control_assigntomodule(control *c, module *m);
char *config_get_control_detachfrommodule(control *c);
char *config_get_module_onload(module *m);
char *config_get_module_onunload(module *m);
char *config_get_plugin_onload();
char *config_get_plugin_onunload();

char *config_get_variable_set(listnode *ln);
char *config_get_variable_set_static(listnode *ln);

//---- using fully qualified names, here
char *config_getfull_control_create_child(control *c_p, controltype *ct);
char *config_getfull_control_create_child_named(control *c_p, controltype *ct, control *c);
char *config_getfull_control_delete(control *c);
char *config_getfull_control_saveas(control *c, const char *filename);
char *config_getfull_control_renamecontrol_s(control *c);
char *config_getfull_control_clone(control *c);

char *config_getfull_control_setagent_s(control *c, const char *action, const char *agenttype);
char *config_getfull_control_setagent_c(control *c, const char *action, const char *agenttype, const char *parameters);
char *config_getfull_control_setagent_b(control *c, const char *action, const char *agenttype, const bool *parameters);
char *config_getfull_control_setagent_i(control *c, const char *action, const char *agenttype, const int *parameters);
char *config_getfull_control_setagent_d(control *c, const char *action, const char *agenttype, const double *parameters);

char *config_getfull_control_removeagent(control *c, const char *action);

char *config_getfull_control_setagentprop_s(control *c, const char *action, const char *key);
char *config_getfull_control_setagentprop_c(control *c, const char *action, const char *key, const char *value);
char *config_getfull_control_setagentprop_b(control *c, const char *action, const char *key, const bool *value);
char *config_getfull_control_setagentprop_i(control *c, const char *action, const char *key, const int *value);
char *config_getfull_control_setagentprop_d(control *c, const char *action, const char *key, const double *value);

char *config_getfull_control_setcontrolprop_s(control *c, const char *key);
char *config_getfull_control_setcontrolprop_c(control *c, const char *key, const char *value);
char *config_getfull_control_setcontrolprop_b(control *c, const char *key, const bool *value);
char *config_getfull_control_setcontrolprop_i(control *c, const char *key, const int *value);
char *config_getfull_control_setcontrolprop_d(control *c, const char *key, const double *value);

char *config_getfull_control_setwindowprop_s(control *c, const char *key);
char *config_getfull_control_setwindowprop_c(control *c, const char *key, const char *value);
char *config_getfull_control_setwindowprop_b(control *c, const char *key, const bool *value);
char *config_getfull_control_setwindowprop_i(control *c, const char *key, const int *value);
char *config_getfull_control_setwindowprop_d(control *c, const char *key, const double *value);

char *config_getfull_control_assigntomodule(control *c, module *m);
char *config_getfull_control_detachfrommodule(control *c);
char *config_getfull_variable_set_static_s(module *m, listnode *ln);



#endif
/*=================================================*/
