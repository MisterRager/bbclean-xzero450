/*===================================================

	WINDOW MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_WindowMaster_h
#define BBInterface_WindowMaster_h

//Pre-defined structures
struct window;

//Includes
#include "ControlMaster.h"
#include "MenuMaster.h"

//Cirular dependency. Whoah. This surely needs some redesign.
struct module;

//Define these structures
struct window
{
	HWND hwnd;
	HBITMAP bitmap;

	control *controlptr;

	int x;
	int y;
	int width;
	int height;

	int style;
	StyleItem *styleptr;
	bool has_custom_style;
	
	bool use_custom_font;
	HFONT font;
	char Fontname[128];
	int FontHeight;
	int FontWeight;

	bool is_bordered;

	bool is_transparent;
	bool is_visible;
	bool is_ontop;
	bool is_slitted;
	bool is_toggledwithplugins;
	bool is_onallworkspaces;
	bool is_detectfullscreen;

	bool is_moving;
	bool is_sizing;
	bool is_autohidden;
	
	bool autohide;
	bool useslit;
	int transparency;
	int makeinvisible;
	int nosavevalue;
	int workspacenumber;

	bool is_button;
	struct ButtonStyleInfo *bstyleptr;
	bool is_slider;
	struct SliderStyleInfo *sstyleptr;
};

struct ButtonStyleInfo
{
	int style;
	StyleItem *styleptr;
	bool has_custom_style;
	bool use_custom_font;
	HFONT font;
	char Fontname[128];
	int FontHeight;
	int FontWeight;
	int nosavevalue;
};

struct SliderStyleInfo
{
	//bar style
	int style;
	StyleItem *styleptr; // Color,ColorTo,bevel,bevelpos
	bool has_custom_style;

	//inner style
	bool draw_inner;
	int in_style;
	StyleItem *in_styleptr; // Color,ColorTo,bevel,bevelpos
	bool in_has_custom_style;	
};

//Define these functions internally
int window_startup();
int window_shutdown();

int window_create(control *c);
int window_destroy(window **pw);

void window_menu_context(Menu *m, control *c);

void window_save_control(control *c);
void window_save();

LRESULT CALLBACK window_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int window_message(int tokencount, char *tokens[], bool from_core, module* caller);
int window_message_setproperty(control *c, int tokencount, char *tokens[]);
void style_set_customvalue(StyleItem *styleptr, int style, int nosavevalue);
void reconfigure_customvalue(control *c);

void window_pluginsvisible(bool isvisible);
void window_update(window *w, bool position, bool transparency, bool visibility, bool sticky);

//int window_helper_register(const char *classname, LRESULT CALLBACK (*callbackfunc)(HWND, UINT, WPARAM, LPARAM));
int window_helper_register(const char *classname, WNDPROC callbackfunc);
int window_helper_unregister(const char *classname);
HWND window_helper_create(const char *classname);
int window_helper_destroy(HWND hwnd);
void window_make_child(window *w, window *pw);

//Global variables
extern char szWPx   [];
extern char szWPy   [];
extern char szWPwidth   [];
extern char szWPheight  [];
extern char szWPtransparency    [];
extern char szWPisvisible   [];
extern char szWPisontop [];
extern char szWPissnappy    [];
extern char szWPisslitted   [];
extern char szWPistransparent   [];
extern char szWPistoggledwithplugins    [];
extern char szWPisonallworkspaces   [];

#endif
/*=================================================*/
