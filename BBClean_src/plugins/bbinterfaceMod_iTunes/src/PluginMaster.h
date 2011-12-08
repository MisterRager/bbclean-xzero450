/*===================================================

	PLUGIN MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_PluginMaster_h
#define BBInterface_PluginMaster_h

//Includes
#include "MenuMaster.h"
#include <list>
#include <string>


//Functions
void plugin_startup();
void plugin_shutdown(bool save);
void plugin_reconfigure(bool force);
int plugin_message(int tokencount, char *tokens[], bool from_core, module* caller);
Menu *plugin_menu_settings(void);
void plugin_save();

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme,int FontType , LPARAM lParam);
bool detect_fullscreen_window();
bool search_fullscreen_app();

//Global Variables
extern HINSTANCE plugin_instance_plugin;
extern HWND plugin_hwnd_blackbox;
extern HWND plugin_hwnd_slit;
extern bool plugin_using_modern_os;
extern bool plugin_zerocontrolsallowed; 
extern bool plugin_suppresserrors;  
extern bool plugin_click_raise;
//extern bool plugin_backup_script;
//extern bool plugin_enableshadows;  
extern bool plugin_save_modules_on_unload;
extern char *plugin_desktop_drop_command;

extern bool plugin_snapwindows;
extern int plugin_snap_dist;
extern int plugin_snap_padding;

extern int plugin_icon_sat;
extern int plugin_icon_hue;

extern int BBVersion;
extern std::list<std::string> fontList;
extern _locale_t locale;
extern bool fullscreen_app_exist;
extern HMONITOR fullscreen_app_hMon;


#endif
/*=================================================*/
