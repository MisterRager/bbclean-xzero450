/*
0.2: 
Bugs fixed:
    - ShowSaveMessage is not read from .rc
    - Workspace value in .rc is now 1-based
Features added:
    - Launch editor with .rc, controlled by LaunchEditor in .rc
    - Launch new windows in Maximized, Minimized, and Sticky states
    
0.1: Initial release

*/
#include "BBApi.h"
#include <stdlib.h>

// ----------------------------------
// plugin info

LPCSTR szVersion        = "0.2.1";
LPCSTR szAppName        = "WinTyler";
LPCSTR szInfoVersion    = "0.2";
LPCSTR szInfoAuthor     = "0.2 - TiCL | 0.2.1 - XZero450";
LPCSTR szInfoRelDate    = "2006-09-20";
LPCSTR szInfoLink       = "http://wiki.bb4win.org/wiki/WinTyler";
LPCSTR szInfoEmail      = "ticler@gmail.com";


// ----------------------------------
// Interface declaration

extern "C"
{
	DLL_EXPORT int beginPlugin(HINSTANCE hPluginInstance);
	DLL_EXPORT int beginSlitPlugin(HINSTANCE hPluginInstance, HWND hSlit);
	DLL_EXPORT int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit);
	DLL_EXPORT void endPlugin(HINSTANCE hPluginInstance);
	DLL_EXPORT LPCSTR pluginInfo(int field);
};

// ----------------------------------
// Global vars

HINSTANCE hInstance;
HWND BBhwnd;
HWND hSlit_present;
bool is_bblean;

// receives the path to "bbSDK.rc"
char rcpath[MAX_PATH];

// ----------------------------------
// Style info

struct style_info
{
	StyleItem Frame;
	int bevelWidth;
	int borderWidth;
	COLORREF borderColor;
} style_info;

// ----------------------------------
// Plugin window properties

struct plugin_properties
{
	// settings
	int xpos, ypos;
	int width, height;

	bool useSlit;
	bool alwaysOnTop;
	bool snapWindow;
	bool pluginToggle;
	bool alphaEnabled;
	bool drawBorder;
	int  alphaValue;

	// our plugin window
	HWND hwnd;

	// current state variables
	bool is_ontop;
	bool is_moving;
	bool is_sizing;
	bool is_hidden;

	// the Slit window, if we are in it.
	HWND hSlit;

	// GDI objects
	HBITMAP bufbmp;
	HFONT hFont;

	int currentPlacementMethod;
    int n_columns;    
    bool showSaveMessage;
    bool launchEditor;
} my;

// ----------------------------------
// some function prototypes

void GetStyleSettings();
void ReadRCSettings();
void WriteRCSettings();
void ShowMyMenu(bool popup);
void invalidate_window(void);
void set_window_modes(void);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);




//*****************************************************************************
// utilities

// case insensitive string compare, up to lenght of second string
int my_substr_icmp(const char *a, const char *b)
{
	return memicmp(a, b, strlen(b));
}

// debugging (checkout "DBGVIEW" from "http://www.sysinternals.com/")
void dbg_printf (const char *fmt, ...)
{
	char buffer[4096]; va_list arg;
	va_start(arg, fmt);
	vsprintf (buffer, fmt, arg);
	OutputDebugString(buffer);
}

//===========================================================================

enum placementMethods{
    DEFAULT_PLACEMENT,
    N_COLUMNS,
    SAVED_POS
};

RECT screenArea;
int nextAvailableColumn; //stores X axis of the left of next available column
int column_width;

typedef struct SavedPos{
    char *title;
    int xpos;
    int ypos;
    int height; //maximize = -1, minimize = -2
    int width;  //maximize = -1, minimize = -2
    int desk;   //current = -1, sticky = -2
    SavedPos *next;
};

SavedPos *savedPosList=NULL;
char windowTitleBuffer[200];
//======

// no-slit interface
int beginPlugin(HINSTANCE hPluginInstance)
{
	
	// ---------------------------------------------------
	// grab some global information

	BBhwnd          = GetBBWnd();
	hInstance       = hPluginInstance;
	//hSlit_present   = hSlit;
	is_bblean       = 0 == my_substr_icmp(GetBBVersion(), "bblean");

	// ---------------------------------------------------
	// register the window class

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof wc);

	wc.lpfnWndProc      = WndProc;      // window procedure
	wc.hInstance        = hInstance;    // hInstance of .dll
	wc.lpszClassName    = szAppName;    // window class name
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wc.style            = CS_DBLCLKS;

	if (!RegisterClass(&wc))
	{
		MessageBox(BBhwnd,
			"Error registering window class", szVersion,
				MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	// ---------------------------------------------------
	// Zero out variables, read configuration and style

	ZeroMemory(&my, sizeof my);

	ReadRCSettings();
	//~ GetStyleSettings();

	// ---------------------------------------------------
	// create the window

	my.hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,   // window ex-style
		szAppName,          // window class name
		NULL,               // window caption text
		WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // window style
		0,            // x position
		0,            // y position
		0,           // window width
		0,          // window height
		NULL,               // parent window
		NULL,               // window menu
		hInstance,          // hInstance of .dll
		NULL                // creation data
		);

	
	return 0;

}

//===========================================================================
// on unload...

void endPlugin(HINSTANCE hPluginInstance)
{
    dbg_printf("Good bhye!");
	// Destroy the window...
	DestroyWindow(my.hwnd);

	// Unregister window class...
	UnregisterClass(szAppName, hPluginInstance);
}

//===========================================================================
// pluginInfo is used by Blackbox for Windows to fetch information about
// a particular plugin.

LPCSTR pluginInfo(int field)
{
	switch (field)
	{
		case PLUGIN_NAME:           return szAppName;       // Plugin name
		case PLUGIN_VERSION:        return szInfoVersion;   // Plugin version
		case PLUGIN_AUTHOR:         return szInfoAuthor;    // Author
		case PLUGIN_RELEASE:        return szInfoRelDate;   // Release date, preferably in yyyy-mm-dd format
		case PLUGIN_LINK:           return szInfoLink;      // Link to author's website
		case PLUGIN_EMAIL:          return szInfoEmail;     // Author's email
		default:                    return szVersion;       // Fallback: Plugin name + version, e.g. "MyPlugin 1.0"
	}
}



//===========================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//List of events to monitor
    static int msgs[] = { BB_RECONFIGURE, BB_BROADCAST, BB_TASKSUPDATE, 0};
    taskinfo tlp; //tmp storage
    
	switch (message)
	{
 		case WM_CREATE:
 			// Register to reveive these message
  			SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
 
  			// Make the window appear on all workspaces
  			MakeSticky(hwnd);
            dbg_printf("Halo thar!");
  			break;
  
  		case WM_DESTROY:
  			//RemoveSticky(hwnd);
  			SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
  			break;
 

		// ----------------------------------------------------------
		// Blackbox sends a "BB_RECONFIGURE" message on style changes etc.

		case BB_RECONFIGURE:
		 	ReadRCSettings();
		/*	GetStyleSettings();
			set_window_modes(); */
			break;

		case BB_TASKSUPDATE:
            
            SavedPos *savedPos;
            //dbg_printf("HALO %l",(long)lParam);
            
            switch(lParam)
            {
                case TASKITEM_ADDED:
                    switch(my.currentPlacementMethod)
                    {
                        case DEFAULT_PLACEMENT:
                            //do nothing
                            dbg_printf("default placement");
                        
                            break;
                        case N_COLUMNS:
                            MoveWindow((HWND)wParam, nextAvailableColumn, screenArea.top, column_width, screenArea.bottom-screenArea.top, true);
                            nextAvailableColumn = (nextAvailableColumn + column_width)%(screenArea.right-screenArea.left);
                            
                            break;
                        case SAVED_POS:
                        {
                            GetWindowText((HWND)wParam,windowTitleBuffer,200);
                            dbg_printf("Title: %s (len: %d)", windowTitleBuffer,strlen(windowTitleBuffer));
                             
                            for (savedPos = savedPosList; savedPos!=NULL; savedPos=savedPos->next) {
                                if (NULL != strstr(windowTitleBuffer, savedPos->title))
                                {
                                    dbg_printf("Width: %d, Height: %d", savedPos->width, savedPos->height);
									if (savedPos->width == -1 && savedPos->height == -1) { //Maximize window
                                        dbg_printf("Maximize window");
                                        SendMessage(BBhwnd, BB_WINDOWMAXIMIZE, 0, (LPARAM)wParam);
                                        /* SendMessage(BBhwnd, BB_WINDOWGROWHEIGHT, 0, (LPARAM)wParam);
                                        SendMessage(BBhwnd, BB_WINDOWGROWWIDTH, 0, (LPARAM)wParam); */
                                    } else if (savedPos->width == -2 && savedPos->height == -2) { //minimize window
                                        dbg_printf("Minimize window");
                                        SendMessage(BBhwnd, BB_WINDOWMINIMIZE, 0, (LPARAM)wParam);
                                    } else {
                                        dbg_printf("Manual positioning");
										if ( savedPos->xpos != -1 && savedPos->ypos != -1) {
											//Not sure as to whether all of this should be in this block
											MoveWindow((HWND)wParam, 
												savedPos->xpos, 
												savedPos->ypos, 
												savedPos->width, 
												savedPos->height,
												true);
											if (savedPos->height == -1){ //max height
												SendMessage(BBhwnd, BB_WINDOWGROWHEIGHT, 0, (LPARAM)wParam);
											} else if (savedPos->width == -1) { //max width
												SendMessage(BBhwnd, BB_WINDOWGROWWIDTH, 0, (LPARAM)wParam);
											}
										} else {
											MoveWindow((HWND)wParam, 
												-1, 
												-1, 
												savedPos->width, 
												savedPos->height,
												true);
										}
                                    }
                                    
                                    
                                    switch(savedPos->desk) {
										case -4://Sticky and AoT
										case -3://AoT
                                        case -2:
                                            //make sticky
                                            MakeSticky((HWND)wParam);
                                            break;
                                        case -1:
                                            //place in current desktop
                                            break;
                                        default:
                                            //manual placement
                                            GetTaskLocation((HWND)wParam,&tlp);
                                            tlp.desk = savedPos->desk;
                                            SetTaskLocation((HWND)wParam,&tlp,BBTI_SETDESK);
                                    }
                                                                        
                                    break; //stop searching through savePosList
                                        
                                }
                                    
                            }
                            break;
                        }
                        
                    }
                    break;
                
                case TASKITEM_REMOVED:
                    switch(my.currentPlacementMethod)
                    {
                        case N_COLUMNS:
                            GetTaskLocation((HWND)wParam,&tlp);
                            nextAvailableColumn = tlp.xpos;
                            break;
                        
                    }
                default:
                    //
                    break;
            }                
            break;

		// ----------------------------------------------------------
		// Blackbox sends Broams to all windows...

		case BB_BROADCAST:
		{
			const char *msg_string = (LPCSTR)lParam;

			// Our broadcast message prefix:
			const char broam_prefix[] = "@WinTyler.";
			const int broam_prefix_len = sizeof broam_prefix - 1; // minus terminating \0

			// check broams sent from our own menu
			if (!memicmp(msg_string, broam_prefix, broam_prefix_len))
			{
				msg_string += broam_prefix_len;
				
				if (!strnicmp(msg_string, "SavePosition",12))
				{
                    dbg_printf("SavePosition");
                    
                    HWND activeTask = GetTask(GetActiveTask());
                    GetTaskLocation(activeTask, &tlp);
                    
					GetWindowText(activeTask,windowTitleBuffer,200);
                    WriteString(rcpath, "newWindowTitle:", windowTitleBuffer); //what is the function for appending?
                    RenameSetting(rcpath, "newWindowTitle:", "WindowTitle:");
                    
                    
                    /*if(tlp==NULL)
                    {
                        dbg_printf("tlp null");
                        break;
                    }*/
                        
                    WriteInt(rcpath, "newXPos:", tlp.xpos);
                    RenameSetting(rcpath, "newXPos:", "XPos:");
                    WriteInt(rcpath, "newYPos:", tlp.ypos);
                    RenameSetting(rcpath, "newYPos:", "YPos:");
                    WriteInt(rcpath, "newWidth:", tlp.width);
                    RenameSetting(rcpath, "newWidth:", "Width:");
                    WriteInt(rcpath, "newHeight:", tlp.height);
                    RenameSetting(rcpath, "newHeight:", "Height:");
                    WriteInt(rcpath, "newWorkspace:", tlp.desk + 1); //.rc stores 1-based workspace value
                    RenameSetting(rcpath, "newWorkspace:", "Workspace:");
                    
			        //How to open the conf file for editing?
                    
                    //PostMessage(BBhwnd, BB_EXECUTE, 0, (LPARAM) "notepad.exe WinTyler.rc");
                    if(my.launchEditor)
                    {
                        BBExecute(BBhwnd, NULL, "notepad.exe", rcpath, NULL, SW_SHOWNORMAL, false);
                    }
                    if(my.showSaveMessage)
                    {
                        BBMessageBox(MB_OK, "Window position saved. Please check WinTyler.rc");
                    }
                    //RECONFIGURE??
                }
				
			}
			break;
		}

		default:
			return DefWindowProc(hwnd,message,wParam,lParam);
	}
	return 0;
}


void ReadRCSettings(void)
{
	int i = 0;
	do
	{
		// First and third, we look for the config file
		// in the same folder as the plugin...
		HINSTANCE hInst = hInstance;
		// second we check the blackbox directory
		if (1 == i) hInst = NULL;

		GetModuleFileName(hInst, rcpath, sizeof(rcpath));
		char *file_name_start = strrchr(rcpath, '\\');
		if (file_name_start) ++file_name_start;
		else file_name_start = strchr(rcpath, 0);
		strcpy(file_name_start, "WinTyler.rc");

	} while (++i < 3 && false == FileExists(rcpath));

	// If a config file was found we read the plugin settings from it...
	// ...if not, the ReadXXX functions give us just the defaults.
    
    my.showSaveMessage = ReadBool(rcpath, "ShowSaveMessage:", false);
    my.launchEditor = ReadBool(rcpath, "LaunchEditor:", true);
    my.currentPlacementMethod = ReadInt(rcpath, "PlacementMethod:",N_COLUMNS);
    dbg_printf("PlacementMethod: %d",my.currentPlacementMethod);    
    GetMonitorRect(NULL, &screenArea, GETMON_WORKAREA);
    //dbg_printf("Screen working area: %d %d",screenArea.right, screenArea.bottom);    
    
    LONG rcPos=0;
    SavedPos *newPos;
    
    switch(my.currentPlacementMethod)
    {
        case N_COLUMNS:            
            my.n_columns = ReadInt(rcpath,"NumberOfColumns:",2);    
            dbg_printf("NumberOfColumns: %d",my.n_columns);
            nextAvailableColumn=screenArea.left;
            column_width = (screenArea.right - screenArea.left)/my.n_columns;
            break;
        
        case SAVED_POS:            
            savedPosList = NULL;
            
            for ( ; ; )
            {
                const char *c = ReadValue(rcpath,"WindowTitle:",&rcPos);

                if ( c == NULL ) break;              
                newPos = (SavedPos*)malloc(sizeof(SavedPos));        
                newPos->next = savedPosList;
                savedPosList = newPos;
                
                dbg_printf("c=<%s> pos=%d", c, rcPos);
                dbg_printf("Len: %d",strlen(c));
                
                savedPosList->title = (char*)malloc(sizeof(char)*strlen(c)+1);
                strcpy(savedPosList->title,c);
                savedPosList->title[strlen(c)]='\0';
                
                c = ReadValue(rcpath,"XPos:",&rcPos);
                savedPosList->xpos = atoi(c);
                
                c = ReadValue(rcpath,"YPos:",&rcPos);
                savedPosList->ypos = atoi(c);
                
                c = ReadValue(rcpath,"Width:",&rcPos);
                savedPosList->width = atoi(c);
                /*if(savedPosList->width == -1)
                {
                    savedPosList->width =  screenArea.right - screenArea.left;
                }*/
                
                c = ReadValue(rcpath,"Height:",&rcPos);
                savedPosList->height = atoi(c);
                /*if(savedPosList->height == -1)
                {
                    savedPosList->height=  screenArea.bottom - screenArea.top;
                }*/
                
                c = ReadValue(rcpath,"Workspace:",&rcPos);
                savedPosList->desk = atoi(c) - 1; //Taskinfo stores 0-based workspace value
                
                
                dbg_printf("XPos: %d, YPos: %d",savedPosList->xpos,savedPosList->ypos);
                dbg_printf("Width: %d, Height: %d, WS: %d",
                            savedPosList->width,
                            savedPosList->height,
                            savedPosList->desk);
                
            }                
            break;
    }
       
    

	
}

//*****************************************************************************
