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

enum
{
    INT_CUSTOM_X = 0,
    INT_CUSTOM_Y,
    INT_WIDTH_PERCENT,
    INT_TOOLTIP_ALPHA,
    INT_WINDOW_ALPHA,
    INT_TASK_FONTSIZE,
    INT_TASK_ICON_SIZE,
    INT_TASK_ICON_HUE,
    INT_TASK_ICON_SAT,
	INT_TASK_MAXWIDTH,
    INT_USER_HEIGHT,
    INT_TRAY_ICON_SIZE,
    INT_TRAY_ICON_HUE,
    INT_TRAY_ICON_SAT,
    INT_TOOLTIP_DELAY,
    INT_TOOLTIP_DISTANCE,
    INT_TOOLTIP_MAXWIDTH,
    //--------------
    NUMBER_INT_BROAMS
};

class IntMenuItem
{
public:
    IntMenuItem() {};
    ~IntMenuItem() {};

    void Set(char *t, int index, int *p, int minimum, int maximum)
    {
        strcpy(m_text, t);
        sprintf(m_broam, "@sbx.IntMenu.%d ", index);
        m_value = p;
        m_min = minimum;
        m_max = maximum;
    };

    void CreateIntMenu(Menu* m) { MakeMenuItemInt(m, m_text, m_broam, *m_value, m_min, m_max); };

    int *m_value;

private:
    char m_text[50];
    char m_broam[50];
    int m_min;
    int m_max;
};
