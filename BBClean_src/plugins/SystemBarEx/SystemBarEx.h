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

#pragma warning(disable: 4786)

//-------------------------------------------
#include "bbTooltip.h"
#include "IconItem.h"
#include "LeanList.h"
#include "TaskbarMenuIndex.h"
#include "ModuleInfo.h"
#include "ToggleIndex.h"
#include "EnableIndex.h"
#include "IntMenuItem.h"
#include "MouseClickMenus.h"
#include "bbAbout.h"
static bbAbout  *p_bbAbout;
//-------------------------------------------
static IntMenuItem          IM_Array[NUMBER_INT_BROAMS];
static bbTooltipInfo        TipSettings;
static bbTooltip            bbTip;
extern TaskbarMenuIndex     TBMenuInfo;
static ToggleIndex          ToggleInfo;
static EnableIndex			EnableInfo;
static IconItem             TaskIconItem,
                            TrayIconItem;

//-------------------------------------------

//---------------------------------------------------------------------------

extern "C"
{
    __declspec(dllexport) int       beginPlugin(HINSTANCE);
    __declspec(dllexport) int       beginSlitPlugin(HINSTANCE, HWND);
    __declspec(dllexport) void      endPlugin(HINSTANCE);
    __declspec(dllexport) LPCSTR    pluginInfo(int x) { return ModuleInfo::Get(x); };
};

//---------------------------------------------------------------------------

#define DEFAULT_POSITION_X              0
#define DEFAULT_POSITION_Y              0
#define DEFAULT_SHADOW_XY				0

#define DEFAULT_SHADOW_COLOR			0x00000000

#define DEFAULT_WIDTH_PERCENT           60
#define DEFAULT_PLACEMENT               PLACEMENT_BOTTOM_CENTER
#define DEFAULT_HEIGHT_TYPE             HEIGHT_AUTO

#define DEFAULT_BBBUTTON_TEXT           "bb"
#define	DEFAULT_BBBUTTON_RCOMMAND		"ShowSBXMenu"
#define	DEFAULT_BBBUTTON_LCOMMAND		"ShowRootMenu"
#define	DEFAULT_BBBUTTON_MCOMMAND		"ShowRootMenu"
#define	DEFAULT_WINDOWLABEL_COMMAND		"default"
#define DEFAULT_USER_HEIGHT             16

#define DEFAULT_TASK_ICONS              ICONIZED_ONLY

#define DEFAULT_TASK_ICON_SIZE          8
#define DEFAULT_TASK_ICON_SATURATION    0
#define DEFAULT_TASK_ICON_HUE           70
#define DEFAULT_TASK_FONT_SIZE          12
#define DEFAULT_TASK_MAXWIDTH			0

#define DEFAULT_TRAY_ICON_SIZE          8
#define DEFAULT_TRAY_ICON_SATURATION    0
#define DEFAULT_TRAY_ICON_HUE           70

#define DEFAULT_CLOCK_FORMAT            "%#I:%M %p"
#define DEFAULT_CLOCK_TOOLTIP_FORMAT    "%a %#d %b"

#define DEFAULT_TOOLTIP_ALPHA           255
#define DEFAULT_WINDOW_ALPHA            255

#define DEFAULT_OBJECT_CONFIG           826913
#define DEFAULT_ENBALE_CONFIG			660541565

#define DEFAULT_SHOW_MENU_ITEMS         63448576
#define DEFAULT_PLACEMENT_CONFIG_STRING "0143256"
//#define DEFAULT_PLACEMENT_CONFIG        2046728//123892752//9992
#define DEFAULT_STYLE_CONFIG            2146212482
#define STYLE_CONFIG_ALL_PR             224694

#define DEFAULT_TE_RIGHT                67239937
#define DEFAULT_TE_MIDDLE               108265472
#define DEFAULT_TE_LEFT                 5242881  // with up null and down activate
#define DEFAULT_TE_DBL_RIGHT			0
#define DEFAULT_TE_DBL_MIDDLE			0
#define DEFAULT_TE_DBL_LEFT				4

#define DEFAULT_BBTOOLTIP_POPUP_DELAY   0
#define DEFAULT_BBTOOLTIP_DISTANCE      5
#define DEFAULT_BBTOOLTIP_MAXWIDTH      180

#define	DEFAULT_SHADOW_SETTING			0

#define WORKSPACE_MOVE_BTN_WIDTH		8

//XZero's
#define TRAYICON_REFRESH 3

#define PLACEMENT_LINK_TO_TOOLBAR 6
#define PLACEMENT_CUSTOM 7
#define NUMBER_PLACEMENT 8


#define BB_DRAGOVER (WM_USER+100)
#define TASK_RISE_TIMER 4


#define _OffsetRect(lprc, dx, dy) (*lprc).left += (dx), (*lprc).right += (dx), (*lprc).top += (dy), (*lprc).bottom += (dy)

//---------------------------------------------------------------------------

enum {
    CLOCK_UPDATE_TIMER = 1,
    WINDOWLABEL_TIMER,
    CHECK_RECT_TIMER,
    CHILD_TIMER,
};

//---------------------------------------------------------------------------

#define OUTER_SPACING                   1
#define INNER_SPACING                   2

//---------------------------------------------------------------------------

#define ELEM_ALIGN_LEFT                 false
#define ELEM_ALIGN_RIGHT                true

//---------------------------------------------------------------------------

#define PT_LEAVE                        true
#define PT_HOVER                        false

//---------------------------------------------------------------------------

#define HORIZONTAL                      true
#define VERTICAL                        false

//---------------------------------------------------------------------------

#define READ                            true
#define WRITE                           false

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

static int bb_messages[] = {
    BB_DESKTOPINFO,
    BB_LISTDESKTOPS,
    BB_RECONFIGURE,
    BB_BROADCAST,
    BB_TASKSUPDATE,
    BB_TRAYUPDATE,
    BB_TOOLBARUPDATE,
    BB_REDRAWGUI,
    BB_SETTOOLBARLABEL,
    0
};

//---------------------------------------------------------------------------

static char *HeightSettingNames[] = {
    "Auto",
    "Custom",
    "Toolbar"
};

enum {
    HEIGHT_AUTO = 0,
    HEIGHT_CUSTOM,
    HEIGHT_TOOLBAR,
    //--------------
    NUMBER_HEIGHT
};

//---------------------------------------------------------------------------

static char *PlacementNames[] = {
    "Top Left",
    "Top Right",
    "Top Center",
    "Bottom Left",
    "Bottom Right",
    "Bottom Center",
    "Link to Toolbar",
    "Custom"
};

//---------------------------------------------------------------------------

static char *TaskDisplayNames[] = {
    "Text",
    "Icon/Text",
	"Icon", //"Iconized Only",  // changed menu
};

enum {
    TEXT_ONLY = 0,
    ICON_AND_TEXT,
    ICONIZED_ONLY,
    //--------------
    NUMBER_TASK_TYPE
};

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

static BYTE StyleIDList[] = {
    SN_TOOLBARCLOCK,
    SN_TOOLBARLABEL,
    SN_TOOLBARBUTTON,
    SN_TOOLBARBUTTONP,
    SN_TOOLBAR,
    SN_TOOLBARWINDOWLABEL,
    0,
    0
};

static char *StyleDisplayNames[] = {
    "Clock",
    "Label",
    "Button",
    "ButtonP",
    "Toolbar",
    "Window Label",
    "Parent Relative",
    "Auto"
};

enum STYLE_INDEX {
    STYLE_TOOLBARCLOCK = 0,
    STYLE_TOOLBARLABEL,
    STYLE_TOOLBARBUTTON,
    STYLE_TOOLBARBUTTON_P,
    STYLE_TOOLBAR,
    STYLE_TOOLBARWINDOWLABEL,
    //--------------
    STYLE_PARENTRELATIVE,
    STYLE_DEFAULT,
    //--------------
    NUMBER_STYLE_NAMES,
    NUMBER_STYLE_ITEMS = NUMBER_STYLE_NAMES - 2,
	STYLE_BITS_PER_FXN = 3
};

StyleItem			StyleItemArray[NUMBER_STYLE_ITEMS];

static unsigned                     StyleConfig;
// .......for these, input "STYLE_MENU_" defines to return a "STYLE_INDEX", above
static BYTE getSC(BYTE x)           { return (StyleConfig & (0x7 << (x * STYLE_BITS_PER_FXN))) >> (x * STYLE_BITS_PER_FXN); };
static void clearSC(BYTE x)         { StyleConfig &= ~(0x7 << (x * STYLE_BITS_PER_FXN)); };
static void setSC(BYTE x, BYTE y)   { clearSC(x), StyleConfig |= ((y) << ((x) * STYLE_BITS_PER_FXN)); };
static bool testSC(BYTE x, BYTE y)  { return getSC(x) == y; };


//---------------------------------------------------------------------------

#define bufDC                           hdcArray[0]
#define bufIconTask                     hdcArray[1]
#define bufIconTray                     hdcArray[2]
#define bufBackground                   hdcArray[3]
#define bufActiveTask                   hdcArray[4]
#define bufInactiveTask                 hdcArray[5]
#define bufActiveTaskExtra              hdcArray[6]
#define bufInactiveTaskExtra            hdcArray[7]
#define bufIconizedActiveTask           hdcArray[8]
#define bufIconizedInactiveTask         hdcArray[9]
#define bufWorkspacePressed             hdcArray[10]
#define bufWorkspaceNotPressed          hdcArray[11]
#define bufBBButtonNotPressed           hdcArray[12]
#define bufBBButtonPressed              hdcArray[13]
#define bufClockNotPressed              hdcArray[14]
#define bufClockPressed                 hdcArray[15]
#define bufTray                         hdcArray[16]
#define bufWindowLabelPressed           hdcArray[17]
#define bufWindowLabelNotPressed        hdcArray[18]
#define bufWSLButtonNotPressed          hdcArray[19]
#define bufWSLButtonPressed             hdcArray[20]
#define bufWSRButtonNotPressed          hdcArray[21]
#define bufWSRButtonPressed             hdcArray[22]

#define NUMBER_HDC                      23

static HDC  MainDisplayDC,
            hdcArray[NUMBER_HDC];

static HBITMAP hBitmapNullArray[NUMBER_HDC];
static HFONT hFontNull;

//---------------------------------------------------------------------------

static char *StyleMenuNames[] = {
    "Tray",
    "Clock",
    "Tasks",
    "WinLabel",
    "BB Button",
    "Workspace",
    "Pressed",
    "Tooltips",
	"Workspace Move",
	"Toolbar"
};

enum STYLE_MENU_INDEX {
    STYLE_MENU_SYSTRAY,
    STYLE_MENU_CLOCK,
    STYLE_MENU_TASKS,
    STYLE_MENU_WINDOWLABEL,
    STYLE_MENU_BBBUTTON,
    STYLE_MENU_WORKSPACE,
    STYLE_MENU_PRESSED,
    STYLE_MENU_TOOLTIPS,
	STYLE_MENU_WORKSPACEBUTTONS,
	STYLE_MENU_TOOLBAR,
    //--------------
    NUMBER_STYLE_MENUS
};

//---------------------------------------------------------------------------

static char *PluginMenuText[] = {
    "Tooltips",
    "Window",
    "Style",
    "Misc."
};

enum {
    MENU_ID_TOOLTIPS = 0,                               // matches PluginMenuText[]
    MENU_ID_WINDOW,
    MENU_ID_STYLE,
    MENU_ID_MISC,
	//MENU_ID_BBBUTTON,
    //--------------
    NUMBER_PLUGIN_MENU_LIST,
    //--------------
    MENU_ID_PLACEMENT = NUMBER_PLUGIN_MENU_LIST,
    MENU_ID_HEIGHT,
    MENU_ID_ORDER,
    MENU_ID_TASKS,
    MENU_ID_TRAY,
    MENU_ID_TASKMENUITEMS,
    MENU_ID_TOOLTIP_PLACEMENT,
	//MENU_ID_BBUTTON_EDIT,
    MENU_ID_ENABLED,
    MENU_ID_EDIT_STRING,
    //--------------
    MENU_ID_PLUGIN_MENU,
    MENU_ID_TASKBAR_MENU,
    //--------------
    NUMBER_BEGIN_STYLE_MENUS,
    //--------------
    MENU_ID_STYLE_TRAY = NUMBER_BEGIN_STYLE_MENUS,      // matches StyleMenuNames[]
    MENU_ID_STYLE_CLOCK,
    MENU_ID_STYLE_TASKS,
    MENU_ID_STYLE_WINDOWLABEL,
    MENU_ID_STYLE_START,
    MENU_ID_STYLE_WORKSPACE,
    MENU_ID_STYLE_PRESSED,
    MENU_ID_STYLE_TOOLTIPS,
	MENU_ID_STYLE_WORKSPACEBUTTONS,
	MENU_ID_STYLE_TOOLBAR,
    //--------------
    NUMBER_MENU_OBJECTS
};

static Menu *MenuArray[NUMBER_MENU_OBJECTS];

//---------------------------------------------------------------------------

static char *RestoreAllChar = "<All Icons>";

//---------------------------------------------------------------------------

class SystrayItem {
public:
    SystrayItem(unsigned intpointer, systemTray *psystemtray)
    {
        pseudopointer = intpointer;
        pSystrayStructure = psystemtray;
    };
    ~SystrayItem() {};

    unsigned    pseudopointer;
    systemTray *pSystrayStructure;
    char        tip[256];
};

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

class TaskbarItem {
public:
    TaskbarItem(tasklist*);
    ~TaskbarItem();

    inline void Paint();
    bool IsActive(HDC, HDC);
    HICON GetIconHandle(int);

    unsigned    ViewPosition;
    RECT        TaskRect;
    SIZE        TextSize;
    COLORREF    TaskTextColor;
    HDC         BufBacking;
    tasklist    *pTaskList;
    HDC         bufIcon;
    HBITMAP     bmpIcon;
    HICON       hIcon_old;
    bool        bPressed;
    int         task_width_old;
    bool        bBufferA;
    bool        bBufferI;
    bool        IsIconized,
                bFlashed,
                TaskExists,
                bButtonPressed;
    BYTE        *m_pBit;
    HICON       hIcon_local_s;
    HICON       hIcon_local_b;
};

static TaskbarItem  *pTaskbarItemStored,
                    *pTaskbarItemStored_Menu;//,
                    //*pTaskbarItemFlash;

//---------------------------------------------------------------------------

static char *ElementNames[] = {
    "BB Button",
    "Workspace",
    "Clock",
    "Tray",
	"Workspace MoveL",
	"Workspace MoveR",
    "Tasks",
    "Window Label"
};

enum {
    BG_BB_BUTTON = 0,
    BG_WORKSPACE,
    BG_CLOCK,
    BG_TRAY,
	BG_WORKSPACEMOVEL,
	BG_WORKSPACEMOVER,
    //------------------
    NUMBER_BACKGROUNDITEMS,             // only the number of BackgroundItem objects
    NUMBER_ELEMENTS,                    // the BackgroundItems plus the tasks
    BG_TASKS = NUMBER_BACKGROUNDITEMS,
    BG_WINDOWLABEL,
    PC_BITS_PER_ELEMENT = 3,
};


//static unsigned                     PlacementConfig;
//static unsigned getPC(BYTE x)		{ return (PlacementConfig & (0x7 << (x * PC_BITS_PER_ELEMENT))) >> (x * PC_BITS_PER_ELEMENT); };
//static void clearPC(BYTE x)			{ PlacementConfig &= ~(0x7 << (x * PC_BITS_PER_ELEMENT)); };
//static void setPC(BYTE x, BYTE y)	{ PlacementConfig |= (y << (x * PC_BITS_PER_ELEMENT)); };
//static bool testPC(BYTE x, BYTE y)	{ return getPC(x) == y; };

//---------------------------------------------------------------------------

class BackgroundItem {
public:
    BackgroundItem(
        BYTE,
        enable_index,
        toggle_index,
        HDC,
        HDC,
        char*,
        unsigned*,
        char*,
        bool (BackgroundItem::*)(),
        void (*)(unsigned));

    ~BackgroundItem() {};

    void PaintBG(unsigned);
    bool PaintTray();
    bool PaintWindowLabel();
    bool GenericBG();
	bool PaintMoveLeft();
	bool PaintMoveRight();

    void                (*pMouseEvent)(unsigned);
    unsigned            width,
                        *pTextWidth;
    BYTE                styleIndex;
    enable_index  ocIndex;
    toggle_index  ocTip;
    RECT                placementRect,
                        mouseRect;
    HDC                 hdcP,
                        hdcNP;
    char                *pText,
                        *pTip;
    bool                (BackgroundItem::*pPaint)();
    BYTE                overflow;
    bool                bgPainted,
                        align,
                        bPressed;
};

static BackgroundItem   *BGItemArray[NUMBER_BACKGROUNDITEMS],
                        *BGItemOrderArray[NUMBER_BACKGROUNDITEMS],
                        *pWindowLabelItem;

//---------------------------------------------------------------------------

#define TB_MAX      200
#define TB_OVERFLOW 100

class TaskMenuLine {
public:
    TaskMenuLine() { m_enabled = false; };
    ~TaskMenuLine() {};

    char    m_buf[TB_MAX];
    HMENU   m_hsubmenu;
    int     m_id;
    int     m_state;
    bool    m_enabled;
};

static char tm_buf_data[TB_MAX + 100];//tm_buf[TB_MAX], 

//---------------------------------------------------------------------------

bool APIENTRY               DllMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved) { return true; };
static LRESULT CALLBACK     SystemBarExWndProc(HWND, UINT, WPARAM, LPARAM);
static inline bool          HandleCapture(UINT, POINT);
static inline bool          TasksMouseEvent(unsigned);
static Menu*                XMakeMenu(LPSTR);

static bool                 taskmenu_scaffold[MI_NUMBER];

static Menu*                MakeSystemSubmenu(HWND, char*, HMENU);
static void                 CleanMenuItem(char*, UINT);
static void                 PickTaskMenuItem(HWND, Menu*, HMENU, int, int, int, char*, bool*);
static inline int           GetTaskMenuInfo(HWND, HMENU*, TaskMenuLine**, int*, bool*);
static inline char*         BuildSingleTaskBroam(HWND, char*, int);

static int                  GetItemLength(char*, SIZE*);

static bool                 CheckButton(UINT, POINT, RECT*, bool*, void (*)(UINT), bool, bool),
                            TestAsyncKeyState(unsigned),
                            retW_TIn(),
                            retW_TI(),
                            retW(),
                            retT();

static inline void  TrayMouseEvent(unsigned),
                    UpdateTaskbarLists(),
                    SetBarHeightAndCoord(),
                    PaintTasks(),
                    ShowTaskMenu(HWND),
                    ActivateTask(HWND),
                    PaintInscription(),
                    BBButtonMouseEvent(unsigned),
					WorkspaceMoveLMouseEvent(unsigned),
					WorkspaceMoveRMouseEvent(unsigned),
                    WorkspaceMouseEvent(unsigned),
                    ClockMouseEvent(unsigned),
                    tmeCycle(TaskEventItem**, unsigned, char*),
					tmeDBLCycle(TaskEventItemDBL**, unsigned, char*);

static void ShowPluginMenu(bool),
            RefreshAll(),
            NotifyToolbar(),
            UpdatePlacementOrder(),
            //UpdateTransparency(toggle_index),
			UpdateToolTipsTransparency(),
			UpdateBarTransparency(),
            GetStyleSettings(bool),
            UpdateClock(bool),
            UpdateFunctionItemArray(),
            UpdateClientRect(),
            Reconfig(bool),
            windowAll(unsigned),
            Autohide(unsigned, unsigned),
            HideBar(bool),
            SetTransTrue(HWND, BYTE),
            WindowLabelMouseEvent(unsigned),
            PaintBackgrounds(HDC, HDC, BYTE, unsigned, unsigned),
            RCSettings(bool),
			//ReadExclusions(),
            deleteSystrayList(),
            deleteTaskbarList(LeanList<TaskbarItem*>*),
            ShowBar(bool),
            SetFullNormal(bool),
            SetScreenMargin(bool),
            DropStyle(HDROP),
            ProcessStringBroam(char*, char*),
            ReleaseButton(RECT*),
            Single_TaskMouseEvent(unsigned),
            ShowRootMenu(),
            CreatePath(char*, char*/*, bool*/),
            BitBltRect(HDC, HDC, RECT*, int),
            winAll_1(HWND, unsigned),
            winAll_2(HWND, unsigned),
            fullNorm(HWND, RECT*, unsigned, unsigned),
            FocusActiveTask(),
            X_BBTokenize(char*, BYTE);

static void taskEventFxn0(),
            taskEventFxn1(),
            taskEventFxn2(),
            taskEventFxn3(),
			taskEventFxn4(),
            taskEventFxn5(),
            taskEventFxn6(),
            taskEventFxn7(),
            taskEventFxn8(),
			taskEventFxn9(),
            taskEventFxn10(),
            taskEventFxn11();

static pF_TV_RV pFxnArray[] = {
    taskEventFxn0,
    taskEventFxn1,
    taskEventFxn2,
    taskEventFxn3,
	taskEventFxn4,
    taskEventFxn5,
    taskEventFxn6,
    taskEventFxn7,
    taskEventFxn8,
	taskEventFxn9,
    taskEventFxn10,
    taskEventFxn11
};

//---------------------------------------------------------------------------

static bool (_stdcall *pSetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);

//---------------------------

static tasklist         *p_TaskList;
static DesktopInfo      *DeskInfo;
static ToolbarInfo      *pTbInfo;

//---------------------------

static PAINTSTRUCT  paintStructure;
static HINSTANCE    hSystemBarExInstance;
static POINT        MouseEventPoint;
static SIZE         TasksMouseSize;

//---------------------------

static HWND         hBlackboxWnd,
                    hSystemBarExWnd,
                    hDesktopWnd,
                    ActiveTaskHwnd,
                    buttonHwnd;

//---------------------------

static BYTE ActiveStyleIndex,
            Placement_Old;

//---------------------------

static COLORREF ActiveTaskTextColor,
                InactiveTaskTextColor,
                styleBorderColor,
				sbxTransColor;

//---------------------------

static RECT MainRect,
            TaskbarRect,
            TooltipActivationRect;

//---------------------------

static LeanList<SystrayItem*>   *SystrayList,
                                *SystrayListNew,
                                *pSysTmp,
                                *pSysTmpNew;

static LeanList<TaskbarItem*>   *TaskbarList,
                                *TaskbarListNew,
                                *TaskbarListIconized,
                                *pTaskTmp,
                                *pTaskTmpI,
                                *pTaskTmpNew;

//---------------------------

static char editor_path[MAX_LINE_LENGTH],
            docpath[MAX_LINE_LENGTH],
            rcpath[MAX_LINE_LENGTH],
			//exclusionpath[MAX_LINE_LENGTH],
			//exclusionpath2[MAX_LINE_LENGTH],
            *broam_temp;

//---------------------------

static unsigned TaskCount,
                NumberIconized,
                NumberNotIconized,
                SystrayIconsPainted,
                HeightSizing,
                SystemBarExPlacement,
                taskIcons,
                ScreenWidth,
                ScreenHeight,
                SystemBarExHeight,
                SystemBarExWidth,
                styleBorderWidth,
                //oldTrayCount,
                //oldTaskCount,
                IconizedShift,
                TheBorder,
                TaskWidth,
				MaxTaskWidth,
                IconizedTaskWidth,
                ExtraTaskWidth,
                TaskRegionWidth,
                TextHeight,
                TrayIconCount,
                CurrentWorkspace,
                hover_count,
                menu_id;

//---------------------------

static int  SystemBarExX,
            SystemBarExY,
            SystemBarExWidthPercent,
            WindowAlpha,
            TaskFontSize,
            CustomHeightSizing,
			TaskMaxWidth,
			BShadowX,
			BShadowY,
			BPShadowX,
			BPShadowY;

COLORREF	ShadowColors[2];
//COLORREF	TransColor = DEFAULT_SHADOW_COLOR;
//---------------------------

static bool bBackgroundExists,
            bTaskBackgroundsExist,
            bIconizedTaskBackgroundsExist,
            bTextArrayExists,
            bSystemBarExHidden,
            //buttonParentRelative,
            ToolbarBevelRaised,
            bTaskbarListsUpdated,
            bTrayListsUpdated,
            bTaskTooltips,
            isLean,
            popup;

//---------------------------

static RECT *capture_rect;
static bool *pressed_state;
static void (*MouseButtonEvent)(unsigned message);

//---------------------------

static int WindowLabelWidth;
static char WindowLabelText[MINI_MAX_LINE_LENGTH];
//static char *pWindowLabelText;
static unsigned WindowLabelX;

//---------------------------

static SIZE WorkspaceTextSize;
static char WorkspaceText[MINI_MAX_LINE_LENGTH];

//---------------------------

static SIZE BBButtonTextSize;
static char BBButtonText[MINI_MAX_LINE_LENGTH];
static char BBButtonRCommand[MINI_MAX_LINE_LENGTH];
static char BBButtonLCommand[MINI_MAX_LINE_LENGTH];
static char BBButtonMCommand[MINI_MAX_LINE_LENGTH];
//
static char WindowLabelRCommand[MINI_MAX_LINE_LENGTH];
static char WindowLabelLCommand[MINI_MAX_LINE_LENGTH];
static char WindowLabelMCommand[MINI_MAX_LINE_LENGTH];

//---------------------------

#define MAX_TOKENS 4
static unsigned BBTokenizeIndexArray[MAX_TOKENS];

//---------------------------

static unsigned InscriptionLeft;
static char     InscriptionText[MINI_MAX_LINE_LENGTH];
static SIZE     InscriptionTextSize;

//---------------------------

static time_t       SystemTime;
static SYSTEMTIME   systemTimeNow;
static struct tm    *LocalTime;
static SIZE         ClockTextSize;
static unsigned     ClockTextWidthOld;
static char         ClockTooltipFormat[MINI_MAX_LINE_LENGTH],
                    ClockTimeTooltip[MINI_MAX_LINE_LENGTH],
                    ClockFormat[MINI_MAX_LINE_LENGTH],
                    CurrentTime[MINI_MAX_LINE_LENGTH],
                    ClockTime[MINI_MAX_LINE_LENGTH],
					PlacementConfigString[MINI_MAX_LINE_LENGTH];

//char	sbx2[MINI_MAX_LINE_LENGTH];
//char	sbx[MINI_MAX_LINE_LENGTH];

BYTE				PlacementStruct[NUMBER_ELEMENTS];

//---------------------------------------------------------------------------

static bool         bIconBuffersUpdated;
//static bool         bTaskActivated;
static unsigned     IconizedRight;
static bool         bTaskPR;

class GestureItem {
public:
    GestureItem()  {};
    ~GestureItem() {};

    void Set(UINT, HWND);
    bool Process(UINT);

    int     m_x;
    int     m_threshold;
    HWND    m_hwnd;
};

static GestureItem          GestureObject;

static UINT                 TaskMessage;

static StyleItem            TooltipPRStyle;

static BITMAPINFOHEADER     bmpInfo;

static bool                 bInSlit = false;
static HWND                 hSlitWnd = NULL;
static bool                 bStart = false;

static HWND                 ActiveTaskHwndOld;

static bool                 bCaptureInv;

static int                  Global_Just;
static int                  TB_Just;

static MENUITEMINFO         menuInfo;

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

//The NULL's cause our blak background bug
HFONT                hMenuFrameFont;// = NULL;  // fonts for system menus
HFONT                hMenuTitleFont;// = NULL;

//int		ttlExclusions = 0;

//typedef struct {
//	char	excluded[MINI_MAX_LINE_LENGTH];
//} exclusionsList_t;

//---------------------------------------------------------------------
