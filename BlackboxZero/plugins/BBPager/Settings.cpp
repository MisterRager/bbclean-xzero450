#include "BBPager.h"

//===========================================================================

char rcpath[MAX_PATH];
char editor[MAX_LINE_LENGTH];

//struct POSITION position;
int desktopChangeButton;
int focusButton;
int moveButton;

// Window information
int screenWidth, screenHeight, screenLeft, screenTop, screenRight, screenBottom;
int vScreenWidth, vScreenHeight, vScreenLeft, vScreenTop, vScreenRight, vScreenBottom;
double ratioX, ratioY;
int leftMargin, topMargin;
bool drawBorder;

// Transparency
bool transparency;
int transparencyAlpha;

bool useSlit;

//===========================================================================

void ReadRCSettings()
{
	char temp[MAX_LINE_LENGTH];

	if (!FileExists(rcpath)) 
	{
		InitRC();
		return;
	}
	// If a config file was found we read the plugin settings from the file...
		
	// Position of BBPager window
	int n, n1, n2;
	strcpy(temp, ReadString(rcpath, "bbpager.position:", ""));
	n = sscanf(temp, "+%d-%d", &n1, &n2);

	if (n == 2)
	{
		position.unix = true;
		xpos = position.x = position.ox = n1;
		ypos = position.y = position.oy = n2;
	}
	else 
	{
		position.unix = false;
		xpos = position.x = position.ox = ReadInt(rcpath, "bbpager.position.x:", 0);
		ypos = position.y = position.oy = ReadInt(rcpath, "bbpager.position.y:", 0);
	}

	//strcpy(position.placement, ReadString(rcpath, "bbpager.placement:", "TopLeft"));

	//if (position.x >= GetSystemMetrics(SM_CX--SCREEN) || position.x < 0) position.x = 0;
	//if (position.y >= GetSystemMetrics(SM_CY--SCREEN) || position.y < 0) position.y = 0;

	position.raised = ReadBool(rcpath, "bbpager.raised:", true);
	position.snapWindow = ReadBool(rcpath, "bbpager.snapWindow:", true);

	// BBPager metrics
	desktop.width = ReadInt(rcpath, "bbpager.desktop.width:", 40);
	desktop.height  = ReadInt(rcpath, "bbpager.desktop.height:", 30);

	UpdateMonitorInfo();

	// Make sure desktop sizes are within limits
	if (desktop.width < 0) desktop.width = 40;
	//if (desktop.width > 200) desktop.width = 200;
	if (desktop.height < 0) desktop.height = 30;
	//if (desktop.height > 200) desktop.height = 200;

	// get mouse button for desktop changing, etc., 1 = LMB, 2 = Middle, 3 = RMB
	desktopChangeButton = ReadInt(rcpath, "bbpager.desktopChangeButton:", 2);

	focusButton = ReadInt(rcpath, "bbpager.windowFocusButton:", 1);

	moveButton = ReadInt(rcpath, "bbpager.windowMoveButton:", 3);

	//get vertical or horizontal alignment setting
	strcpy(temp, ReadString(rcpath, "bbpager.alignment:", "horizontal"));
	if (!_stricmp(temp, "vertical")) 
	{
		position.horizontal = false;
		position.vertical = true;
	}
	else 
	{
		position.horizontal = true;
		position.vertical = false;
	}

	// row and column number
	frame.columns = ReadInt(rcpath, "bbpager.columns:", 1);
	frame.rows = ReadInt(rcpath, "bbpager.rows:", 1);

	if (frame.rows < 1) frame.rows = 1;
	if (frame.columns < 1) frame.columns = 1;

	//numbers on desktop enable
	desktop.numbers = ReadBool(rcpath, "bbpager.desktopNumbers:", false);

	//windows on desktop enable
	desktop.windows = ReadBool(rcpath, "bbpager.desktopWindows:", false);

	desktop.tooltips = ReadBool(rcpath, "bbpager.windowToolTips:", false);

	// Autohide enable
	position.autohide = ReadBool(rcpath, "bbpager.autoHide:", false);

	/*if (position.autohide)
	{
		GetPos(true);
		SetPos(position.side);
		position.hidden = false;
		HidePager();
	}
*/
	// default BB editor
	//GetBlackboxEditor(editor);
	strcpy(editor, ReadString(extensionsrcPath(), "blackbox.editor:", "notepad.exe"));

	topMargin = ReadInt(extensionsrcPath(), "blackbox.desktop.marginTop:", 0);
	leftMargin = ReadInt(extensionsrcPath(), "blackbox.desktop.marginLeft:", 0);
	usingAltMethod = ReadBool(extensionsrcPath(), "blackbox.workspaces.altMethod:", false);

	drawBorder = ReadBool(rcpath, "bbpager.drawBorder:", true);

	// Transparency
	transparency = ReadBool(rcpath, "bbpager.transparency:", false);
	transparencyAlpha = ReadInt(rcpath, "bbpager.transparency.alpha:", 200);

	useSlit = ReadBool(rcpath, "bbpager.useSlit:", false);

	// Read check
	//MessageBox(0, "RC Settings Read", "ReadRCSettings()", MB_OK | MB_TOPMOST | MB_SETFOREGROUND);
}

//===========================================================================

void WriteRCSettings()
{
	static char szTemp[MAX_LINE_LENGTH];
	static char temp[32];
	DWORD retLength = 0;

	// Write plugin settings to config file, using path found in ReadRCSettings()...
	HANDLE file = CreateFile(rcpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file)
	{
		// BBPager position
		//if (inSlit) { position.ox = xpos; position.oy = ypos; }
		if (position.unix) 
		{
			sprintf(szTemp, "bbpager.position: +%d-%d\r\n", position.ox, position.oy);
			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
		}
		else 
		{
			sprintf(szTemp, "bbpager.position.x: %d\r\n", position.ox);
			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
			sprintf(szTemp, "bbpager.position.y: %d\r\n", position.oy);
			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
		}

		// placement
		/*strcpy(temp, position.placement);
		sprintf(szTemp, "bbpager.placement: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);*/

		// desktop size
		sprintf(szTemp, "bbpager.desktop.width: %d\r\n", desktop.width);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
		sprintf(szTemp, "bbpager.desktop.height: %d\r\n", desktop.height);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// alignment
		(position.vertical) ? strcpy(temp, "vertical") : strcpy(temp, "horizontal");
		sprintf(szTemp, "bbpager.alignment: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Write column/row values
		sprintf(szTemp, "bbpager.columns: %d\r\n", frame.columns);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		sprintf(szTemp, "bbpager.rows: %d\r\n", frame.rows);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// desktop change mouse button, etc.
		sprintf(szTemp, "bbpager.desktopChangeButton: %d\r\n", desktopChangeButton);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		sprintf(szTemp, "bbpager.windowMoveButton: %d\r\n", moveButton);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		sprintf(szTemp, "bbpager.windowFocusButton: %d\r\n", focusButton);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Always on top
		(position.raised) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.raised: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Autohide
		(position.autohide) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.autoHide: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Snap window to edge of screen
		(position.snapWindow) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.snapWindow: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Transparency
		if (usingWin2kXP)
		{
			(transparency) ? strcpy(temp, "true") : strcpy(temp, "false");
			sprintf(szTemp, "bbpager.transparency: %s\r\n", temp);
 			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

			sprintf(szTemp, "bbpager.transparency.alpha: %d\r\n", transparencyAlpha);
			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
		}

		(drawBorder) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.drawBorder: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Numbers on Desktops
		(desktop.numbers) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.desktopNumbers: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Windows on Desktops
		(desktop.windows) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.desktopWindows: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(usingAltMethod) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.desktopAltMethod: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(desktop.tooltips) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.windowToolTips: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		// Are we in the slit?
		(useSlit) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbpager.useSlit: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
 	}
	CloseHandle(file);

	// Write check
	//MessageBox(0, "RC Settings wrote", "WriteRCSettings()", MB_OK | MB_TOPMOST | MB_SETFOREGROUND);
}

//===========================================================================

void InitRC()	
{
	char temp[MAX_LINE_LENGTH], temp2[MAX_LINE_LENGTH], path[MAX_LINE_LENGTH], defaultpath[MAX_LINE_LENGTH];
	int nLen;	

	// First we look for the config file in the same folder as the plugin...
	GetModuleFileName(hInstance, rcpath, sizeof(rcpath));
	nLen = strlen(rcpath) - 1;
	while (nLen >0 && rcpath[nLen] != '\\') nLen--;
	rcpath[nLen + 1] = 0;
	strcpy(temp, rcpath);
	strcpy(path, rcpath);
	strcpy(temp2, rcpath); // flush temp2

	strcat(temp2, "bbpager.bb"); // set bspath to "bbpager.bb"
	strcpy(bspath, temp2);

	strcat(temp, "bbpager.rc");
	strcat(path, "bbpagerrc");
	// ...checking the two possible filenames example.rc and examplerc ...
	if (FileExists(temp)) strcpy(rcpath, temp);
	else if (FileExists(path)) strcpy(rcpath, path);
	// ...if not found, we try the Blackbox directory...
	else
	{
		// ...but first we save the default path (bbpager.rc in the same
		// folder as the plugin) just in case we need it later (see below)...
		strcpy(defaultpath, temp);
		GetBlackboxPath(rcpath, sizeof(rcpath));
		strcpy(temp, rcpath);
		strcpy(path, rcpath);
		strcat(temp, "bbpager.rc");
		strcat(path, "bbpagerrc");
		if (FileExists(temp)) strcpy(rcpath, temp);
		else if (FileExists(path)) strcpy(rcpath, path);
		else // If no config file was found, we use the default path and settings, and return
		{
			strcpy(rcpath, defaultpath);
			position.x = position.y = 0;
			desktop.width = 40;
			desktop.height = 30;
			position.horizontal = true;
			position.vertical = false;
			frame.rows = 1;
			frame.columns = 1;
			desktopChangeButton = 2;
			focusButton = 1;
			moveButton = 3;
			//raiseButton = 3;
			position.raised = true;
			position.snapWindow = true;
			position.autohide = false;
			position.hidden = false;
			desktop.numbers = false;
			desktop.windows = false;
			desktop.tooltips = false;
			transparency = false;
			transparencyAlpha = 200;
			drawBorder = true;
			useSlit = false;
			usingAltMethod = false;
			//return;

			WriteRCSettings();
		}
	}
}

void UpdateMonitorInfo()
{
	// multimonitor
	vScreenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
	vScreenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);

	vScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	vScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	vScreenRight = vScreenLeft + vScreenWidth;
	vScreenBottom = vScreenTop + vScreenHeight;

	ratioX = vScreenWidth / desktop.width;
	ratioY = vScreenHeight / desktop.height;

	
	int xScreen, yScreen;
	xScreen = GetSystemMetrics(SM_CXSCREEN);
	yScreen = GetSystemMetrics(SM_CYSCREEN);

	if (vScreenWidth > xScreen || vScreenHeight > yScreen)
	{	// multimon
		// current monitor
		HMONITOR hMon = MonitorFromWindow(hwndBBPager, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mInfo;
		mInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMon, &mInfo);

		screenLeft = mInfo.rcMonitor.left;
		screenRight = mInfo.rcMonitor.right;
		screenTop = mInfo.rcMonitor.top;
		screenBottom = mInfo.rcMonitor.bottom;
		screenWidth = screenRight - screenLeft;
		screenHeight = screenBottom - screenTop;
	}
	else	// single mon (or treat as such)
	{
		vScreenTop = vScreenLeft = 0;

		screenLeft = 0;
		screenRight = screenWidth;
		screenTop = 0;
		screenBottom = screenHeight;
		screenWidth = vScreenWidth = xScreen;
		screenHeight = vScreenHeight = yScreen;

		ratioX = screenWidth / desktop.width;
		ratioY = screenHeight / desktop.height;
	}
}