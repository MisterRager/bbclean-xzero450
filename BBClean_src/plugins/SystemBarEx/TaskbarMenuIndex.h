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
        //--------------
        MI_MIN = 0,
        //--------------
        MI_ALL_TRAY_ICONS = MI_MIN,
        MI_DEICONIZE_ALL,
        MI_CASCADE,
        MI_TILE_HORIZONTAL,
        MI_TILE_VERTICAL,
        MI_FULL_NORMAL_ALL,
        MI_CLOSE_ALL,
        MI_RESTORE_DOWN_ALL,
        MI_MAXIMIZE_ALL,
        MI_RESTORE_ALL,
        MI_MINIMIZE_ALL,
        //--------------
        MI_BEGIN_SINGLE_TASK,
        //--------------
        MI_MOVE_LEFT = MI_BEGIN_SINGLE_TASK,
        MI_MOVE_RIGHT,
        MI_ICONIZE,
        MI_FULL_NORMAL,
        MI_RESTORE_DOWN,
        MI_MINIMIZE,
        MI_MAXIMIZE,
        MI_CLOSE,
        //--------------
        MI_BEGIN_SHELL_ITEMS,
        //--------------
        MI_SHELL_RESTORE = MI_BEGIN_SHELL_ITEMS,
        MI_SHELL_MOVE,
        MI_SHELL_SIZE,
        MI_SHELL_MINIMIZE,
        MI_SHELL_MAXIMIZE,
        MI_SHELL_CLOSE,
        MI_SHELL_SPECIAL,
        //--------------
        MI_NUMBER
        //--------------
 } menu_index;

class TaskbarMenuIndex
{
public:
    

    menu_index           n;
    unsigned        bitlist;

    bool    test(menu_index x)           { return boolList[x]; };
    void    setTrue(menu_index x)        { bitlist |= (1 << x); boolList[x] = true; };
    void    setFalse(menu_index x)       { bitlist &= ~(1 << x); boolList[x] = false; };
    bool    test()                  { return boolList[n]; };
    void    setTrue()               { setTrue(n); };
    void    setFalse()              { setFalse(n); };

    void    reset()                 { n = MI_MIN; };
    char*   getBroam(menu_index x)       { return broams[x]; };
    char*   getBroam()              { return getBroam(n); };
    menu_index   operator ++ ()          { return n = menu_index(n + 1); };

    char *getText(menu_index x)
    {
        //switch (n)
        switch (x)
        {
            case MI_ALL_TRAY_ICONS:        return "All Tray Icons";
            case MI_DEICONIZE_ALL:         return "DeIconize All";
            case MI_CASCADE:               return "Cascade";
            case MI_TILE_HORIZONTAL:       return "Tile Hor.";
            case MI_TILE_VERTICAL:         return "Tile Vert.";
            case MI_FULL_NORMAL_ALL:       return "Full Normal All";
            case MI_CLOSE_ALL:             return "Close All";
            case MI_RESTORE_DOWN_ALL:      return "Restore Down All";
            case MI_MAXIMIZE_ALL:          return "Maximize All";
            case MI_RESTORE_ALL:           return "Restore All";
            case MI_MINIMIZE_ALL:          return "Minimize All";
            case MI_MOVE_LEFT:             return "Move Left";
            case MI_MOVE_RIGHT:            return "Move Right";
            case MI_ICONIZE:               return "Iconize";
            case MI_FULL_NORMAL:           return "Full Normal";
            case MI_RESTORE_DOWN:          return "Restore Down";
            case MI_MINIMIZE:              return "Minimize";
            case MI_MAXIMIZE:              return "Maximize";
            case MI_CLOSE:                 return "Close";
            case MI_SHELL_RESTORE:         return "System Restore";
            case MI_SHELL_MOVE:            return "System Move";
            case MI_SHELL_SIZE:            return "System Size";
            case MI_SHELL_MINIMIZE:        return "System Minimize";
            case MI_SHELL_MAXIMIZE:        return "System Maximize";
            case MI_SHELL_CLOSE:           return "System Close";
            case MI_SHELL_SPECIAL:         return "System Special";
            default:                    return " ";
        }
    };

    char *getText() { return getText(n); };

    void update()
    {
        BYTE i = 0;
        do boolList[i] = ((bitlist & (1 << i)) > 0);
        while (++i < MI_NUMBER);
    };

private:
    char        broams[MI_NUMBER][MINI_MAX_LINE_LENGTH];
    bool        boolList[MI_NUMBER];
};
