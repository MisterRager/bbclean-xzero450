/*===================================================

	DEFINITIONS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_Definitions_h
#define BBInterface_Definitions_h

//Plugin information
extern const char szAppName       [];
extern const char szVersion       [];
extern const char szInfoVersion   [];
extern const char szInfoAuthor    [];
extern const char szInfoRelDate   [];
extern const char szInfoLink      [];
extern const char szInfoEmail     [];

//Local variables
extern const char szPluginAbout   [];
extern const char szPluginAboutLastControl    [];
extern const char szPluginAboutQuickRef   [];

//Strings used frequently
extern const int szBBroamLength;
extern const char szBBroam            [];
extern const char szBEntityControl    [];
extern const char szBEntityAgent      [];
extern const char szBEntityPlugin     [];
extern const char szBEntityVarset     [];
extern const char szBEntityWindow     [];
extern const char szBEntityModule     [];
extern const char szBActionCreate     [];
extern const char szBActionCreateChild[];
extern const char szBActionDelete     [];
extern const char szBActionSetAgent   [];
extern const char szBActionRemoveAgent[];
extern const char szBActionSetAgentProperty   [];
extern const char szBActionSetControlProperty [];
extern const char szBActionSetWindowProperty  [];
extern const char szBActionSetPluginProperty  [];
extern const char szBActionSetModuleProperty  [];


extern const char szBActionSetDefault			[];
extern const char szBActionAssignToModule		[];
extern const char szBActionDetachFromModule		[];
extern const char szBActionOnLoad				[];
extern const char szBActionOnUnload				[];

extern const char szBActionRename     [];
extern const char szBActionLoad       [];
extern const char szBActionEdit       [];
extern const char szBActionToggle     [];
extern const char szBActionSave       [];
extern const char szBActionSaveAs     [];
extern const char szBActionRevert     [];
extern const char szBActionAbout      [];

extern const char szTrue  [];
extern const char szFalse [];

extern const char szFilterProgram [];
extern const char szFilterScript  [];
extern const char szFilterAll [];

//Convenient arrays of strings
extern const char *szBoolArray[2];


//Constant data referrals
#define DATAFETCH_INT_DEFAULTHEIGHT 1101
#define DATAFETCH_INT_DEFAULTWIDTH 1102
#define DATAFETCH_INT_MIN_WIDTH 1201
#define DATAFETCH_INT_MIN_HEIGHT 1202
#define DATAFETCH_INT_MAX_WIDTH 1203
#define DATAFETCH_INT_MAX_HEIGHT 1204
#define DATAFETCH_VALUE_SCALE 2001
#define DATAFETCH_VALUE_BOOL 2002
#define DATAFETCH_VALUE_TEXT 2005

#define DATAFETCH_SUBAGENTS_COUNT 10000
#define DATAFETCH_SUBAGENTS_NAMES_ARRAY 10001
#define DATAFETCH_SUBAGENTS_POINTERS_ARRAY 10002
#define DATAFETCH_SUBAGENTS_TYPES_ARRAY 10003

#define DATAFETCH_CONTENTSIZES 1210 

//Constant control formats
enum CONTROL_FORMAT {
	CONTROL_FORMAT_NONE = 0,
	CONTROL_FORMAT_TRIGGER = (1 << 0),
	CONTROL_FORMAT_DROP = (1 << 1),
	CONTROL_FORMAT_BOOL = (1 << 2),
	CONTROL_FORMAT_INTEGER = (1 << 3),
	CONTROL_FORMAT_DOUBLE = (1 << 4),
	CONTROL_FORMAT_SCALE = (1 << 5),
	CONTROL_FORMAT_TEXT = (1 << 6),
	CONTROL_FORMAT_IMAGE = (1 << 7),
	CONTROL_FORMAT_PLUGIN = (1 << 8)
};

//Constant notify values
enum NOTIFY_TYPE {
	NOTIFY_NOTHING = 0,
	NOTIFY_CHANGE = 100,
	NOTIFY_RESIZE = 200,
	NOTIFY_MOVE = 201,
	NOTIFY_NEEDUPDATE = 300,
	NOTIFY_SAVE_CONTROL = 400,
	NOTIFY_SAVE_AGENT = 401,
	NOTIFY_SAVE_CONTROLTYPE = 402,
	NOTIFY_SAVE_AGENTTYPE = 403,
	NOTIFY_DRAW = 600,
	NOTIFY_DRAGACCEPT = 601,
	NOTIFY_TIMER = 700
};

/*=================================================*/
#endif // BBInterface_Definitions_h

