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

template <class _T>
class LeanList
{
public:
    //--------------------------
    LeanList()
    {
        v = NULL;
        next = NULL;
    };
    //--------------------------
    LeanList(_T _v)
    {
        v = _v;
        next = NULL;
    };
    //--------------------------
    ~LeanList()
    {
        if (next)
            delete next;
    };
    //--------------------------
    LeanList<_T>* append(_T _v)
    {
        return next = new LeanList<_T>(_v);
    };
    //--------------------------
    LeanList<_T>* erase()
    {
        LeanList<_T> *tmp = next->next;
        next->next = NULL;
        delete next;
        return next = tmp;
    };
    //--------------------------
    unsigned size()
    {
        unsigned i = 0;
        LeanList<_T> *tmp = next;
        while (tmp) ++i, tmp = tmp->next;
        return i;
    };
    //--------------------------
    void push_back(_T _v)
    {
        LeanList<_T> *tmp = this;
        while (tmp->next) tmp = tmp->next;
        tmp->next = new LeanList<_T>(_v);
    };
    //--------------------------
    void clear()
    {
        delete next;
        next = NULL;
    };
    //--------------------------

    _T v;
    LeanList<_T> *next;
};
