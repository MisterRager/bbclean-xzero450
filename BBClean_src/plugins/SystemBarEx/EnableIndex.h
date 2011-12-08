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
	EI_MIN,
	EI_SHOW_TASKS = EI_MIN,
	EI_SHOW_INSCRIPTION,
    EI_SHOW_WINDOWLABEL,
    EI_SHOW_WORKSPACE,
    EI_SHOW_TRAY,
    EI_SHOW_CLOCK,
    EI_SHOW_BBBUTTON,
	EI_SHOW_WORKMOVELEFT1,
	EI_SHOW_WORKMOVERIGHT1,
//	EI_SHOW_WORKMOVELEFT2,
//	EI_SHOW_WORKMOVERIGHT2,
	EI_NUMBER
} enable_index;

class EnableIndex
{
public:

    //-------------------------------------------------------------

    //-------------------------------------------------------------
	enable_index		ei_n;
	unsigned			enabled;

    //-------------------------------------------------------------
	bool    test(enable_index x)           { return enableList[x]; };
    void    setTrue(enable_index x)        { enabled |= (1 << x); enableList[x] = true; };
    void    setFalse(enable_index x)       { enabled &= ~(1 << x); enableList[x] = false; };
    void    toggle(enable_index x)         { test(x) ? setFalse(x): setTrue(x); };

	bool    test()                  { return enableList[ei_n]; };
    void    setTrue()               { setTrue(ei_n); };
    void    setFalse()              { setFalse(ei_n); };
    void    toggle()                { toggle(ei_n); };

	void    reset()                 { ei_n = EI_MIN; };
    enable_index   operator ++ ()          { return ei_n = enable_index(ei_n + 1); };

	char*   getBroam(enable_index x)       { return broams[x]; };
    char*   getBroam()              { return getBroam(ei_n); };

    //------------------------------------------------------------

	//-------------------------------------------------------------
	char *getText(enable_index x) {
        switch (x) {
            //--------------------
            case EI_SHOW_INSCRIPTION:			return "Inscription";
            //--------------------
            case EI_SHOW_WINDOWLABEL:			return "WinLabel";
            //--------------------
            case EI_SHOW_TASKS:					return "Tasks";
            //--------------------
            case EI_SHOW_TRAY:					return "Tray";               
            //--------------------
            case EI_SHOW_CLOCK:					return "Clock";
            //--------------------
            case EI_SHOW_WORKSPACE:				return "Workspace";
            //--------------------
            case EI_SHOW_BBBUTTON:				return "BB Button";
			//--------------------
			case EI_SHOW_WORKMOVELEFT1:			return "Workspace MoveL";
			//case EI_SHOW_WORKMOVELEFT2:
			//--------------------
			case EI_SHOW_WORKMOVERIGHT1:		return "Workspace MoveR";
			//case EI_SHOW_WORKMOVERIGHT2:
			//--------------------
            default:                            return " ";
            //--------------------
        }
    };
	char *getText() { return getText(ei_n); };

    //-------------------------------------------------------------

	void makeMenuItem(Menu* menu, enable_index x) {
        MakeMenuItem(menu, getText(x), getBroam(x), test(x));
    };

    //-------------------------------------------------------------

	void update() {
        BYTE i = 0;
        do enableList[i] = ((enabled & (1 << i)) > 0);
        while (++i < EI_NUMBER);
    };

    //-------------------------------------------------------------

private:
	char        broams[EI_NUMBER][MINI_MAX_LINE_LENGTH];
	bool        enableList[EI_NUMBER];

    //-------------------------------------------------------------
};
