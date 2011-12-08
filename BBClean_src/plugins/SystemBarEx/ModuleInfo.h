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

class ModuleInfo
{
public:
    enum index { 
		NAME_VERSION = 0,
		APPNAME,
		VERSION,
		AUTHOR,
		RELEASEDATE,
		WEBLINK,
		EMAIL,
		UPDATEDBY,
		COMPILEDATE
	};

    static char *Get(int field)
    {
        switch (field)
        {
            case APPNAME:       return "SystemBarEx";
            case VERSION:       return "2.3";
            case AUTHOR:        return "Slade Taylor";
            case RELEASEDATE:   return "March 7, 2004 12:00 GMT (beta)";
            case WEBLINK:       return "http://bb4win.sourceforge.net/systembarex/";
            case EMAIL:         return "bladestaylor@yahoo.com";
			case UPDATEDBY:		return "XZero450/Evolution\nxzero450@gmail.com";
			case COMPILEDATE:	return "Aug 01 2009";//__DATE__ is broken in my current minGW
            case NAME_VERSION:
            default:            return "SystemBarEx 2.3b";
        }
    };
};

//-----------------------------------------------------------------------------

static ModuleInfo ModInfo;
