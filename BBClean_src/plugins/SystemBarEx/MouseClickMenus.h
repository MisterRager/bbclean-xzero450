/*---------------------------------------------------------------------------------
 SystemBarEx (© 2004 Slade Taylor [bladestaylor@yahoo.com])
 ----------------------------------------------------------------------------------
 based on BBSystemBar 1.2 (© 2002 Chris Sutcliffe (ironhead) [ironhead@rogers.com])
 ----------------------------------------------------------------------------------
 SystemBarEx is a plugin for Blackbox for Windows.  For more information,
 please visit [http://bb4win.org] or [http://sourceforge.net/projects/bb4win].
 ----------------------------------------------------------------------------------
 SystemBarEx is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/

//-------------------------------------------
typedef bool (*pF_TV_RB)();
typedef void (*pF_TV_RV)();
//-------------------------------------------

enum {
    TE_WITH_NONE = 0,
    TE_WITH_ALT,
    TE_WITH_SHIFT,
    TE_WITH_ALT_AND_SHIFT,
    //--------------
    TE_MODIFIER_LENGTH
};

//---------------------------------------------

enum {
    NULL_TE = 0,
    BLANK_TE,
    //--------------
    BUTTON_TE,
    UP_DOWN_TE,
    MODIFIER_TE,
    EXECUTION_TE,
    //--------------
    TE_FACTOR_LENGTH = 4
};

//---------------------------------------------

enum { // should be in same order as execution_list
    TE_DO_NOTHING = 0,
    TE_ACTIVATE_BRINGFRONT,
    TE_SHOW_TASK_MENU,
    TE_MINIMIZE_TASK,
	TE_MAXIMIZE_TASK,
    TE_CLOSE_TASK,
    TE_TOGGLE_ICONIZED,
    TE_MOVE_PREVIOUS_WORKSPACE,
    TE_MOVE_NEXT_WORKSPACE,
	TE_TOGGLE_AOT,
	TE_TOGGLE_ROLL,
	TE_TOGGLE_PIN,
	//TE_SEND_TASK_TO_TRAY, <-- todo
    //--------------
    TE_FUNCTION_LENGTH,
    TE_BITS_PER_FXN = 4
};

#define TE_BUTTON_LENGTH                3
#define TE_UPDOWN_LENGTH                2

//---------------------------------------------

static unsigned TaskEventIntArray[TE_FUNCTION_LENGTH];

static BYTE getTEC(BYTE x, BYTE y, BYTE z) {
    return (TaskEventIntArray[x] & (0xF << (TE_BITS_PER_FXN * (z + y * TE_MODIFIER_LENGTH))))
        >> (TE_BITS_PER_FXN * (z + y * TE_MODIFIER_LENGTH));
};

static void clearTEC(BYTE x, BYTE y, BYTE z) {
    TaskEventIntArray[x] &= ~(0xF << (TE_BITS_PER_FXN * (z + y * TE_MODIFIER_LENGTH)));
};

static void setTEC(BYTE x, BYTE y, BYTE z, BYTE h) {
    clearTEC(x, y, z);
    TaskEventIntArray[x] |= (h << (TE_BITS_PER_FXN * (z + y * TE_MODIFIER_LENGTH)));
};

static bool testTEC(BYTE x, BYTE y, BYTE z, BYTE h) { return getTEC(x, y, z) == h; };

//---------------------------------------------

static BYTE tmeLengthArray[] = {
    TE_BUTTON_LENGTH,
    TE_UPDOWN_LENGTH,
    TE_MODIFIER_LENGTH,
    TE_FUNCTION_LENGTH
};

//---------------------------------------------

static struct TE_struct {
	char *txt;
	BYTE spot;
}

button_list[] = {
    {"Right",           BLANK_TE},
    {"Middle",          BLANK_TE},
    {"Left",            BLANK_TE},
    {" ",               NULL_TE}
},

up_down_list[] = {
    {"Down",            BLANK_TE},
    {"Up",              BLANK_TE},
    {" ",               NULL_TE}
},

modifier_list[] = {
    {"[None] +",        BLANK_TE},
    {"Alt +",           BLANK_TE},
    {"Shift +",         BLANK_TE},
    {"Alt + Shift +",   BLANK_TE},
    {" ",               NULL_TE}
},

execution_list[] = {
    {"Null",                                TE_DO_NOTHING},
    {"Activate",                            TE_ACTIVATE_BRINGFRONT},
    {"Taskbar Menu",                        TE_SHOW_TASK_MENU},
    {"Minimize",                            TE_MINIMIZE_TASK},
	{"Maximize",							TE_MAXIMIZE_TASK},
    {"Close",                               TE_CLOSE_TASK},
    {"Iconize",                             TE_TOGGLE_ICONIZED},
    {"Move Left",                           TE_MOVE_PREVIOUS_WORKSPACE},
    {"Move Right",                          TE_MOVE_NEXT_WORKSPACE},
	{"AlwaysOnTop",							TE_TOGGLE_AOT},
	{"RollUp",								TE_TOGGLE_ROLL},
	{"Pin",									TE_TOGGLE_PIN},
    {" ",                                   NULL_TE}
};

static struct big_TE_array {
	struct TE_struct *here;
	BYTE length;
}

TE_data_array[] = {
	{&button_list[0],       TE_BUTTON_LENGTH},
	{&up_down_list[0],      TE_UPDOWN_LENGTH},
	{&modifier_list[0],     TE_MODIFIER_LENGTH},
	{&execution_list[0],    TE_FUNCTION_LENGTH}
};

//---------------------------------------------

struct FunctionItem {
    unsigned click;
    pF_TV_RV p;
};

static FunctionItem FunctionItemArray[TE_BUTTON_LENGTH * TE_UPDOWN_LENGTH * TE_BITS_PER_FXN];
static FunctionItem FunctionItemArrayDBL[TE_BUTTON_LENGTH];

//---------------------------------------------------------------------------
class TaskEventItem
{
public:
    TaskEventItem();
    ~TaskEventItem();
    void SetStuff(char*, BYTE, BYTE);
    void CreateMenuArray(BYTE);

    char theText[MINI_MAX_LINE_LENGTH];
    Menu *theMenu;
    TaskEventItem *pArray;
    BYTE ID;
    BYTE theIndex;
};

static TaskEventItem    *pTaskEventSubmenuItem;

//-------------------------------------------
//-------------------------------------------


//	Duplicate Everything for Double Clicks
//-------------------------------------------
static unsigned TaskDBLEventIntArray[TE_BUTTON_LENGTH];

//---------------------------------------------

enum {
    DBL_NULL_TE = 0,
    DBL_BLANK_TE,
    //--------------
    DBL_BUTTON_TE,
    DBL_EXECUTION_TE,
    //--------------
    DBL_TE_FACTOR_LENGTH = 4
};

//---------------------------------------------

//---------------------------------------------
static void setDBLTEC(BYTE x, BYTE h) {
    TaskDBLEventIntArray[x] = h;
};

bool testDBLTEC(BYTE x, BYTE h) { return TaskDBLEventIntArray[x] == h ? true : false; };

//---------------------------------------------

static BYTE tmeDBLLengthArray[] = {
    TE_BUTTON_LENGTH,
    TE_FUNCTION_LENGTH
};

//---------------------------------------------

static struct TE_DBL_struct {
	char *txt;
	BYTE spot;
}

button_dbl_list[] = {
    {"Right",           BLANK_TE},
    {"Middle",          BLANK_TE},
    {"Left",            BLANK_TE},
    {" ",               NULL_TE}
},

execution_dbl_list[] = {
    {"Null",                                TE_DO_NOTHING},
    {"Activate",                            TE_ACTIVATE_BRINGFRONT},
    {"Taskbar Menu",                        TE_SHOW_TASK_MENU},
    {"Minimize",                            TE_MINIMIZE_TASK},
	{"Maximize",							TE_MAXIMIZE_TASK},
    {"Close",                               TE_CLOSE_TASK},
    {"Iconize",                             TE_TOGGLE_ICONIZED},
    {"Move Left",                           TE_MOVE_PREVIOUS_WORKSPACE},
    {"Move Right",                          TE_MOVE_NEXT_WORKSPACE},
	{"AlwaysOnTop",							TE_TOGGLE_AOT},
	{"RollUp",								TE_TOGGLE_ROLL},
	{"Pin",									TE_TOGGLE_PIN},
    {" ",                                   NULL_TE}
};

static struct big_TE_DBL_array {
	struct TE_DBL_struct *here;
	BYTE length;
}

TE_DBL_data_array[] = {
	{&button_dbl_list[0],       TE_BUTTON_LENGTH},
	{&execution_dbl_list[0],    TE_FUNCTION_LENGTH}
};

class TaskEventItemDBL
{
public:
    TaskEventItemDBL();
    ~TaskEventItemDBL();
    void SetStuff(char*, BYTE, BYTE);
    void CreateMenuArray(BYTE);

    char theText[MINI_MAX_LINE_LENGTH];
    Menu *theMenu;
    TaskEventItemDBL *pArray;
    BYTE ID;
    BYTE theIndex;
};

static TaskEventItemDBL    *pTaskEventSubmenuItemDBL;
