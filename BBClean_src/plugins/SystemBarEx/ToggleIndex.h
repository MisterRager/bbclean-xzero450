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
typedef enum {
        TI_MIN = 0,
        //--------------
        TI_SAT_HUE_ON_ACTIVE_TASK = TI_MIN,
        TI_WINDOW_TRANS,
		TI_HIDE_MENU_ON_HOVER,
        TI_AUTOHIDE,
        TI_REVERSE_TASKS,
        TI_ALWAYS_ON_TOP,
        TI_TOGGLE_WITH_PLUGINS,
        TI_CHILD_WINDOW_,
        TI_TOOLTIPS_TRANS,
        TI_TOOLTIPS_TASKS,
        TI_TOOLTIPS_TRAY,
        TI_TOOLTIPS_CLOCK,
        TI_TOOLTIPS_ABOVE_BELOW,
        TI_TOOLTIPS_DOCKED,
        TI_TOOLTIPS_CENTER,
        TI_SNAP_TO_EDGE,
        TI_LOCK_POSITION,
        TI_RC_TASK_FONT_SIZE,
        TI_TASKS_IN_CURRENT_WORKSPACE,
        TI_FLASH_TASKS,
        TI_SET_WINDOWLABEL,
        TI_COMPRESS_ICONIZED,
        TI_ALLOW_GESTURES,
        TI_SHOW_THEN_MINIMIZE,
		TI_PLUGINS_SHADOWS,
		TI_INVERT_WORK_SHIFT,
        //--------------
        TI_NUMBER
} toggle_index;


class ToggleIndex
{
public:

    //-------------------------------------------------------------

    //-------------------------------------------------------------

    toggle_index		ti_n;
    unsigned			bitlist;

    //-------------------------------------------------------------

    bool    test(toggle_index x)           { return boolList[x]; };
    void    setTrue(toggle_index x)        { bitlist |= (1 << x); boolList[x] = true; };
    void    setFalse(toggle_index x)       { bitlist &= ~(1 << x); boolList[x] = false; };
    void    toggle(toggle_index x)         { test(x) ? setFalse(x): setTrue(x); };

	bool    test()                  { return boolList[ti_n]; };
    void    setTrue()               { setTrue(ti_n); };
    void    setFalse()              { setFalse(ti_n); };
    void    toggle()                { toggle(ti_n); };

    void    reset()                 { ti_n = TI_MIN; };
    toggle_index   operator ++ ()          { return ti_n = toggle_index(ti_n + 1); };

    char*   getBroam(toggle_index x)       { return broams[x]; };
    char*   getBroam()              { return getBroam(ti_n); };

    //-------------------------------------------------------------

    char *getText(toggle_index x) {
        switch (x) {
            //--------------------
            //case TI_SHOW_INSCRIPTION:              return "Inscription";
            //--------------------
            case TI_COMPRESS_ICONIZED:             return "Compress PR";
            //--------------------
            //case TI_SHOW_WINDOWLABEL:              return "WinLabel";
            //--------------------
            //case TI_SHOW_TASKS:
            case TI_TOOLTIPS_TASKS:                return "Tasks";
            //--------------------
            //case TI_SHOW_TRAY:
            case TI_TOOLTIPS_TRAY:                 return "Tray";
            //--------------------
            //case TI_SHOW_CLOCK:
            case TI_TOOLTIPS_CLOCK:                return "Clock";
            //--------------------
            //case TI_SHOW_WORKSPACE:                return "Workspace";
            //--------------------
            //case TI_SHOW_BBBUTTON:                 return "BB Button";
			//--------------------
			//case TI_SHOW_WORKMOVELEFT1:				return "Workspace MoveL";
			//case TI_SHOW_WORKMOVELEFT2:
			//--------------------
			//case TI_SHOW_WORKMOVERIGHT1:
			//case TI_SHOW_WORKMOVERIGHT2:			return "Workspace MoveR";
            //--------------------
            //case TI_WINDOW_TRANS:
            //case TI_TOOLTIPS_TRANS:                return "Trans";
            //--------------------
            case TI_SAT_HUE_ON_ACTIVE_TASK:        return "Sat/Hue on Active";
            case TI_HIDE_MENU_ON_HOVER:            return "Hide Menus on Hover";
            case TI_AUTOHIDE:                      return "Autohide";
            case TI_REVERSE_TASKS:                 return "Order Reversed";
            case TI_ALWAYS_ON_TOP:                 return "Always on Top";
            case TI_TOGGLE_WITH_PLUGINS:           return "Toggle with Plugins";
            case TI_SNAP_TO_EDGE:                  return "Snap to Edges";
            case TI_LOCK_POSITION:                 return "Lock Position";
            case TI_RC_TASK_FONT_SIZE:             return "RC Fontsize";
            case TI_TASKS_IN_CURRENT_WORKSPACE:    return "Current Workspace";
            case TI_FLASH_TASKS:                   return "Flashing";
            case TI_SET_WINDOWLABEL:               return "Set WinLabel";
            case TI_ALLOW_GESTURES:                return "Gestures";
            case TI_TOOLTIPS_CENTER:               return "Center";
            case TI_SHOW_THEN_MINIMIZE:            return "Show/Min";
            case TI_TOOLTIPS_DOCKED:               return "Docked";
            case TI_CHILD_WINDOW_:                 return "Child Window";
			case TI_PLUGINS_SHADOWS:			   return "Plugin Btn Shadows";
			case TI_INVERT_WORK_SHIFT:			   return "Invert Workspace Clicks";
            //--------------------
            case TI_TOOLTIPS_ABOVE_BELOW:
            default:                            return " ";
            //--------------------
        }
    };

    char *getText() { return getText(ti_n); };

    //-------------------------------------------------------------

    void makeMenuItem(Menu* menu, toggle_index x) {
        MakeMenuItem(menu, getText(x), getBroam(x), test(x));
    };

    //-------------------------------------------------------------
    void update() {
        BYTE i = 0;
        do boolList[i] = ((bitlist & (1 << i)) > 0);
        while (++i < TI_NUMBER);
    };

    //-------------------------------------------------------------

private:
    char        broams[TI_NUMBER][MINI_MAX_LINE_LENGTH];
    bool        boolList[TI_NUMBER];

    //-------------------------------------------------------------
};
