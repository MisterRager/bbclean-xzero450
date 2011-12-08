/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "../BB.h"
#include "../Settings.h"
#include "../Pidl.h"
#include "../Desk.h"
#include "../Workspaces.h"
#include "../Toolbar.h"
#include "MenuMaker.h"
#include "Menu.h"

#include <ctype.h>
#include <shlobj.h>
#include <shellapi.h>
#ifdef __GNUC__
#include <shlwapi.h>
#endif

//===========================================================================
struct MenuInfo MenuInfo;

static UINT bb_notify;

void MenuMaker_Init(void)
{
    register_menuclass();
    // well, that's stupid, but the first change registration
    // messes with the menu focus, so here the bbhwnd is
    // registered as a default one.
    _ITEMIDLIST *pidl = get_folder_pidl("BLACKBOX");
    bb_notify = add_change_notify_entry(BBhwnd, pidl);
    m_free(pidl);
}

//====================

void MenuMaker_Clear(void)
{
    if (MenuInfo.hTitleFont) DeleteObject(MenuInfo.hTitleFont);
    if (MenuInfo.hFrameFont) DeleteObject(MenuInfo.hFrameFont);
    MenuInfo.hTitleFont = MenuInfo.hFrameFont = NULL;
    if (FolderItem::m_byBulletBmp){
        for (int i = 0; i < FolderItem::m_nBulletBmpSize*2; i++){
            m_free(FolderItem::m_byBulletBmp[i]);
        }
        m_free(FolderItem::m_byBulletBmp);
    }
}

void MenuMaker_Exit(void)
{
    MenuMaker_Clear();
    un_register_menuclass();
    remove_change_notify_entry(bb_notify);
}

void Menu_All_Update(int special)
{
    switch (special)
    {
        case MENU_IS_CONFIG:
            if (MenuExists("IDRoot_configuration"))
                ShowMenu(MakeConfigMenu(false));
            break;

        case MENU_IS_TASKS:
            if (MenuExists("IDRoot_workspace"))
                ShowMenu(MakeDesktopMenu(false));
            else
            if (MenuExists("IDRoot_icons"))
                ShowMenu(MakeIconsMenu(false));
            else
            if (MenuExists("IDRoot_processes"))
                ShowMenu(MakeProcessesMenu(false));
            break;
    }
}

//===========================================================================
int get_bulletstyle (const char *tmp)
{
    static const char *bullet_strings[] = {
        "diamond"  ,
        "triangle" ,
        "square"   ,
        "circle"   ,
        "bitmap"   ,
        "empty"    ,
        NULL
        };

    static const char bullet_styles[] = {
        BS_DIAMOND   ,
        BS_TRIANGLE  ,
        BS_SQUARE    ,
        BS_CIRCLE    ,
        BS_BMP       ,
        BS_EMPTY     ,
        };

    int i = get_string_index(tmp, bullet_strings);
    if (-1 != i) return bullet_styles[i];
    return BS_TRIANGLE;
}

//===========================================================================
int LoadBulletBmp(BYTE** *byBulletBmp){
    char path[MAX_PATH];
    HBITMAP hBmp = (HBITMAP)LoadImage(
            NULL, make_bb_path(path, "bullet.bmp"),
            IMAGE_BITMAP,
            0,
            0,
            LR_LOADFROMFILE
    );
    // get bitmap size
    BITMAP bm;
    GetObject(hBmp, sizeof(BITMAP), &bm);
    int nSize = imax(bm.bmWidth - 2, (bm.bmHeight - 3)/2);
    if (hBmp == NULL) return 0;
    HDC hDC = CreateCompatibleDC(NULL);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);
    *byBulletBmp = (BYTE**)m_alloc(sizeof(BYTE*)*nSize*2);
    for (int i = 0, y = 1; i < nSize*2; i++, y++){
        if (i == nSize) y++;
        (*byBulletBmp)[i] = (BYTE*)m_alloc(sizeof(BYTE)*nSize);
        for (int j = 0; j < nSize; j++){
            (*byBulletBmp)[i][j] = 0xff - greyvalue(GetPixel(hDC, j+1, y));
        }
    }
    SelectObject(hDC, hOldBmp);
    DeleteDC(hDC);
    DeleteObject(hBmp);
    return nSize;
}

//===========================================================================
void MenuMaker_Configure()
{
    // clear fonts
    MenuMaker_Clear();

    MenuInfo.move_cursor = Settings_usedefCursor
        ? LoadCursor(NULL, IDC_SIZEALL)
        : LoadCursor(hMainInstance, (char*)IDC_MOVEMENU);

    StyleItem *MTitle = &mStyle.MenuTitle;
    StyleItem *MFrame = &mStyle.MenuFrame;
    StyleItem *MHilite = &mStyle.MenuHilite;

    if (false == (MTitle->validated & VALID_FONT))
    {
        strcpy(MTitle->Font, MFrame->Font);
        if (0 == (MTitle->validated & VALID_FONTHEIGHT))
            MTitle->FontHeight = MFrame->FontHeight;
        if (0 == (MTitle->validated & VALID_FONTWEIGHT))
            MTitle->FontWeight = MFrame->FontWeight;
    }

    // create fonts
    MenuInfo.hTitleFont = CreateStyleFont(MTitle);
    MenuInfo.hFrameFont = CreateStyleFont(MFrame);

    // set bullet pos & style
    const char *tmp = Settings_menuBulletPosition_cfg;
    if (0 == stricmp(tmp,"default")) tmp = mStyle.menuBulletPosition;

    FolderItem::m_nBulletStyle = get_bulletstyle(mStyle.menuBullet);
    FolderItem::m_nBulletPosition = stristr(tmp, "right") ? DT_RIGHT : DT_LEFT;
    // load bitmap
    FolderItem::m_nBulletBmpSize = LoadBulletBmp(&FolderItem::m_byBulletBmp);

    // --------------------------------------------------------------
    // pre-calulate metrics:

    if (false == (MTitle->validated & VALID_BORDER))
        MTitle->borderWidth = MFrame->borderWidth;

    if (false == (MTitle->validated & VALID_BORDERCOLOR))
        MTitle->borderColor = MFrame->borderColor;

    MenuInfo.nFrameMargin = MFrame->borderWidth;
    if (MFrame->validated & VALID_MARGIN)
        MenuInfo.nFrameMargin += MFrame->marginWidth;
    else
    if (BEVEL_SUNKEN == MFrame->bevelstyle || BEVEL2 == MFrame->bevelposition)
        MenuInfo.nFrameMargin += MFrame->bevelposition;
    else
    if (MHilite->borderWidth)
        MenuInfo.nFrameMargin += 1;

    // --------------------------------------
    // calculate item height, indent

    int ffh = get_fontheight(MenuInfo.hFrameFont);

    int itemHeight;
    if (MHilite->validated & VALID_MARGIN)
        itemHeight = ffh + MHilite->marginWidth;
    else
    if (MFrame->validated & VALID_MARGIN)
        itemHeight = ffh + 1 + 2*(MHilite->borderWidth + (MFrame->marginWidth ? 1 : 0));
    else
    if (Settings_newMetrics)
        itemHeight = imax(10, ffh) + 2 + (mStyle.bevelWidth+1)/2;
    else
        itemHeight = MFrame->FontHeight + (mStyle.bevelWidth+1)/2;

    MenuInfo.nItemHeight = itemHeight;
    MenuInfo.nItemIndent = itemHeight;

    int iconMargin;
    if (MHilite->validated & VALID_MARGIN)
        iconMargin = MHilite->marginWidth;
    else
        iconMargin = 0;

    // --------------------------------------
    // calculate item with icon height, indent

    MenuInfo.nIconItemHeight  = imax(Settings_menuIconSize + iconMargin, itemHeight);
    MenuInfo.nIconItemIndent  = imax(Settings_menuIconSize + iconMargin, itemHeight);

    // --------------------------------------
    // calculate title height, indent, margin

    int tfh = get_fontheight(MenuInfo.hTitleFont);

    int titleHeight;
    if (MTitle->validated & VALID_MARGIN)
        titleHeight = tfh + 2*MTitle->marginWidth;
    else
    if (Settings_newMetrics)
        titleHeight = imax(10, tfh) + 2 + 2*mStyle.bevelWidth;
    else
        titleHeight = MTitle->FontHeight + 2*mStyle.bevelWidth;

    int titleMargin = (titleHeight - tfh + 1) / 2;

    MenuInfo.nTitleHeight = titleHeight + MTitle->borderWidth;
    MenuInfo.nTitleIndent = imax(titleMargin, 3 + MTitle->bevelposition);
    MenuInfo.nTitlePrAdjust = 0;

    if (MTitle->parentRelative)
    {
        MenuInfo.nTitlePrAdjust = (titleMargin / 4 + iminmax(titleMargin, 1, 2));
        MenuInfo.nTitleHeight -= titleMargin - MenuInfo.nTitlePrAdjust;
    }

    // --------------------------------------
    // setup a StyleItem for the scroll rectangle

    MenuInfo.Scroller = MTitle->parentRelative ? *MHilite : *MTitle;
    if (MenuInfo.Scroller.parentRelative && 0 == MenuInfo.Scroller.borderWidth)
    {
        if (MFrame->borderWidth)
            MenuInfo.Scroller.borderColor = MFrame->borderColor;
        else
            MenuInfo.Scroller.borderColor = MFrame->TextColor;

        MenuInfo.Scroller.borderWidth = 1;
    }

    if (false != (MenuInfo.Scroller.bordered = 0 != MenuInfo.Scroller.borderWidth))
    {
        if (MFrame->borderWidth)
            MenuInfo.Scroller.borderWidth = MFrame->borderWidth;
        else
            MenuInfo.Scroller.borderWidth = 1;
    }

    MenuInfo.nScrollerSideOffset = imax(0, MFrame->borderWidth - MenuInfo.Scroller.borderWidth);
    MenuInfo.nScrollerSize = imax(10, MenuInfo.nItemIndent+MFrame->borderWidth);

    MenuInfo.nScrollerTopOffset =
        MTitle->parentRelative
        ? 0
        : MenuInfo.nFrameMargin - imax(0, MenuInfo.Scroller.borderWidth - MTitle->borderWidth);

    // --------------------------------------
    // Transparency
    Settings_menuAlpha = Settings_menuAlphaEnabled ? Settings_menuAlphaValue : 255;

    // from where on does it need a scroll button:
    MenuInfo.MaxMenuHeight = GetSystemMetrics(SM_CYSCREEN) * Settings_menuMaxHeightRatio / 100;

    MenuInfo.MaxMenuWidth = Settings_menusBroamMode
        ? iminmax(Settings_menuMaxWidth*2, 320, 640)
        : Settings_menuMaxWidth;

    MenuInfo.nAvgFontWidth = 0;
}

//===========================================================================
bool get_string_within (char *dest, char **p_src, const char *delim, bool right)
{
    char *a, *b; *dest = 0;
    if (NULL == (a = strchr(*p_src, delim[0])))
        return false;
    if (NULL == (b = right ? strrchr(++a, delim[1]) : strchr(++a, delim[1])))
        return false;
    extract_string(dest, a, b-a);
    *p_src = b+1;
    return true;
}

//===========================================================================
// separate the command from the path spec. in a string like:
//  "c:\bblean\backgrounds >> @BBCore.rootCommand bsetroot -full "%1""

char * get_special_command(char *out, char *in)
{
    char *a, *b;
    a = strstr(in, ">>");
    if (NULL == a) return NULL;
    b = a + 1;
    while (a > in && ' ' == a[-1]) --a;
    *a = 0;
    while (' ' == *++b);
    b = strstr(strcpy(out, b), "%1");
    if (b) b[1] = 's';
    return out;
}

//===========================================================================

//===========================================================================
static const char default_root_menu[] =
    "[begin]"
"\0"  "[path](Programs){PROGRAMS}"
"\0"  "[path](Desktop){DESKTOP}"
"\0"  "[submenu](Blackbox)"
"\0"    "[config](Configuration)"
"\0"    "[stylesmenu](Styles){styles}"
"\0"    "[submenu](Edit)"
"\0"      "[editstyle](style)"
"\0"      "[editmenu](menu.rc)"
"\0"      "[editplugins](plugins.rc)"
"\0"      "[editblackbox](blackbox.rc)"
"\0"      "[editextensions](extensions.rc)"
"\0"      "[end]"
"\0"    "[about](About)"
"\0"    "[reconfig](Reconfigure)"
"\0"    "[restart](Restart)"
"\0"    "[exit](Quit)"
"\0"    "[end]"
"\0"  "[submenu](Goodbye)"
"\0"    "[logoff](Log Off)"
"\0"    "[reboot](Reboot)"
"\0"    "[shutdown](Shutdown)"
"\0"    "[end]"
"\0""[end]"
"\0"
;

//===========================================================================
static const char *menu_cmds[] =
{
    "insertpath"        ,
    "stylesdir"         ,
    "include"           ,
    // ------------------
    "begin"             ,
    "end"               ,
    "submenu"           ,
    // ------------------
    "nop"               ,
    "separator"         ,
    "path"              ,
    "stylesmenu"        ,
    "volume"            ,
    // ------------------
    "workspaces"        ,
    "tasks"             ,
    "config"            ,
    // ------------------
    "exec"              ,
    NULL
};

enum
{
    e_insertpath        ,
    e_stylesdir         ,
    e_include           ,
    // ------------------
    e_begin             ,
    e_end               ,
    e_submenu           ,
    // ------------------
    e_nop               ,
    e_sep               ,
    e_path              ,
    e_stylesmenu        ,
    e_volume            ,
    // ------------------
    e_workspaces        ,
    e_tasks             ,
    e_config            ,
    // ------------------
    e_exec              ,
    // ------------------
    e_no_end            ,
    e_other
};

#define MENU_INCLUDE_MAXLEVEL 16

//===========================================================================

Menu* ParseMenu(FILE **fp, int *fc, const char *path, const char *title, const char *IDString, bool popup)
{
    char buffer[4096], menucommand[80];
    char label[MAX_PATH], data[2*MAX_PATH], icon[2*MAX_PATH];

    Menu *pMenu = NULL;
    MenuItem *Inserted_Items = NULL;
    int inserted = 0, f, cmd_id; char *s, *cp;

    for(;;)
    {
        // read the line
        if (NULL == path)
        {
            *(const char**)fp += 1 + strlen(strcpy(buffer, *(const char**)fp));
        }
        else
        if (false == ReadNextCommand(fp[*fc], buffer, sizeof(buffer)))
        {
            if (*fc) // continue from included file
            {
                FileClose(fp[(*fc)--]);
                continue;
            }
            cmd_id = e_no_end;
            goto skip;
        }

        // replace %USER% etc.
        if (strchr(buffer, '%')) ReplaceEnvVars(buffer);

        //dbg_printf("Menu %08x line:%s", pMenu, buffer);
        cp = buffer;

        // get the command
        if (false == get_string_within(menucommand, &cp, "[]", false))
            continue;

        // search the command
        cmd_id = get_string_index(menucommand, menu_cmds);
        if (-1 == cmd_id) cmd_id = e_other;

        get_string_within(label, &cp, "()", false);
        get_string_within(data,  &cp, "{}", true);
        get_string_within(icon,  &cp, "<>", false);

skip:
        if (NULL == pMenu && cmd_id != e_begin) // first item must be begin
        {
            pMenu = MakeNamedMenu(
                title ? title : NLS0("missing [begin]"), IDString, popup);
        }

        // insert collected items from [insertpath]/[stylesdir] now?
        if (inserted && cmd_id >= e_begin)
        {
            pMenu->add_folder_contents(Inserted_Items, inserted > 1);
            Inserted_Items = NULL;
            inserted = 0;
        }

        switch (cmd_id)
        {
            // If the line contains [begin] we create a title item...
            // If no menu title has been defined, display Blackbox version...
            case e_begin:
                if (0==label[0]) strcpy(label, GetBBVersion());

                if (NULL == pMenu)
                {
                    pMenu = MakeNamedMenu(label, IDString, popup);
                    continue;
                }
                // fall through, [begin] is like [submenu] when within the menu
            //====================
            case e_submenu:
                {
                    sprintf(buffer, "%s_%s", IDString, label);
                    Menu *mSub = ParseMenu(fp, fc, path, data[0]?data:label, buffer, popup);
                    if (mSub) MakeSubmenu(pMenu, mSub, label, icon);
                    else MakeMenuNOP(pMenu, label);
                }
                continue;

            //====================
            case e_no_end:
                MakeMenuNOP(pMenu, NLS0("missing [end]"));
            case e_end:
                pMenu->m_bIsDropTarg = true;
                return pMenu;

            //====================
            case e_include:
                s = unquote(buffer, label[0] ? label : data);
                if (is_relative_path(s) && path)
                    s = strcat(add_slash(data, get_directory(data, path)), s);

                if (++*fc < MENU_INCLUDE_MAXLEVEL && NULL != (fp[*fc] = FileOpen(s)))
                    continue;
                --*fc;
                MakeMenuNOP(pMenu, NLS0("[include] failed"));
                continue;

            //====================
            case e_nop:
                MakeMenuNOP(pMenu, label, icon);
                continue;

            case e_volume:
                MakeMenuVOL(pMenu, label, unquote(buffer, data), icon);
                continue;

            //====================
            case e_sep:
                MakeMenuSEP(pMenu);
                continue;

            //====================
            // a [stylemenu] item is pointing to a dynamic style folder...
            case e_stylesmenu: s = "@BBCore.style %s"; goto s_folder;

            // a [path] item is pointing to a dynamic folder...
            case e_path: s = get_special_command(buffer, data);
            s_folder:
                MakePathMenu(pMenu, label, data, s, icon);
                continue;

            //====================
            // a [styledir] item will insert styles from a folder...
            case e_stylesdir: s = "@BBCore.style %s"; goto s_insert;

            //====================
            // a [insertpath] item will insert items from a folder...
            case e_insertpath: s = get_special_command(buffer, data);
            s_insert:
            {
                struct pidl_node *p, *plist = get_folder_pidl_list(label[0] ? label : data);
                dolist (p, plist) ++inserted, MenuMaker_LoadFolder(&Inserted_Items, p->v, s);
                delete_pidl_list(&plist);
                continue;
            }

            //====================
            // special items...
            case e_workspaces:
                MakeSubmenu(pMenu, MakeDesktopMenu(popup), label[0]?label:NLS0("Workspaces"), icon);
                continue;

            case e_tasks:
                MakeSubmenu(pMenu, MakeIconsMenu(popup), label[0]?label:NLS0("Icons"), icon);
                continue;

            case e_config:
                MakeSubmenu(pMenu, MakeConfigMenu(popup), label[0]?label:NLS0("Configuration"), icon);
                continue;

            //====================
            case e_exec:
                if ('@' != data[0]) goto core_broam;
                // data specifies a broadcast message
                MakeMenuItem(pMenu, label, data, false, icon);
                continue;

            //====================
            case e_other:
                f = get_workspace_number(menucommand);
                if (-1 != f) // is this [workspace#]
                {
                    s = label; DesktopInfo DI;
                    if (0 == *s) s = DI.name, get_desktop_info(&DI, f);
                    MakeSubmenu(pMenu, GetTaskFolder(f, popup), s);
                    continue;
                }
                goto core_broam;

            //====================
            // everything else is converted to a '@BBCore....' broam
            core_broam:
            {
                int x = sprintf(buffer, "@BBCore.%s", strlwr(menucommand));
                if (data[0]) buffer[x]=' ', strcpy(buffer+x+1, data);
                MakeMenuItem(pMenu, label, buffer, false, icon);
                continue;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
static char *IDRoot_String(char *buffer, const char *menu_id)
{
    strlwr(strcpy(buffer + 7, menu_id));
    return (char*)memcpy(buffer, "IDRoot_", 7);
}

Menu * MakeRootMenu(const char *menu_id, const char *path, const char *default_menu)
{
    FILE *fp[MENU_INCLUDE_MAXLEVEL]; int fc = 0;
    if (NULL == (fp[0] = FileOpen(path)))
    {
        path = NULL;
        if (NULL == (fp[0] = (FILE*)default_menu))
            return NULL;
    }
    char IDString[MAX_PATH];
    Menu *m = ParseMenu(fp, &fc, path, NULL, IDRoot_String(IDString, menu_id), true);
    if (path) while (fc>=0) FileClose(fp[fc--]);
    return m;
}

//////////////////////////////////////////////////////////////////////
// Function:    MenuMaker_LoadFolder
// Purpose:     enumerate items in a folder, sort them, and insert them
//              into a menu
//////////////////////////////////////////////////////////////////////

int MenuMaker_LoadFolder(MenuItem **ppItems, LPCITEMIDLIST pIDFolder, const char  *optional_command)
{
    LPMALLOC pMalloc = NULL;
    IShellFolder* pThisFolder;
    LPENUMIDLIST pEnumIDList = NULL;
    LPITEMIDLIST pID; ULONG nReturned;
    int r = 0;

    // nothing to do on NULL pidl's
    if (NULL==pIDFolder) return r;

    // get a COM interface of the folder
    pThisFolder = sh_get_folder_interface(pIDFolder);
    if (NULL == pThisFolder) return r;

    // get the folders "EnumObjects" interface
    pThisFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
    SHGetMalloc(&pMalloc);

    // ---------------------------------------------
    if (pEnumIDList && pMalloc)
    {
        r = 1;
        while (S_FALSE!=pEnumIDList->Next(1, &pID, &nReturned) && 1==nReturned)
        {
            char szDispName[MAX_PATH];
            char szFullName[MAX_PATH];
            ULONG uAttr = SFGAO_FOLDER|SFGAO_LINK;
            szDispName[0] = szFullName[0] = 0;
            pThisFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&pID, &uAttr);
            sh_get_displayname(pThisFolder, pID, SHGDN_NORMAL, szDispName);
            sh_get_displayname(pThisFolder, pID, SHGDN_FORPARSING, szFullName);

            MenuItem* pItem;

            if (uAttr & SFGAO_FOLDER)
            {
                r |= 4;
                pItem = new SpecialFolderItem(
                    szDispName,
                    NULL,
                    (struct pidl_node*)new_node(joinIDlists(pIDFolder, pID)),
                    optional_command,
                    NULL
                    );
            }
            else
            {
                r |= 2;
                if (optional_command)
                {
                    // we need a full path here for comparison with the current style
                    char buffer[2*MAX_PATH];
                    sprintf(buffer, optional_command, szFullName);

                    // cut off .style file-extension
                    char *p = (char*)get_ext(szDispName);
                    if (*p && 0 == stricmp(p, ".style")) *p = 0;

                    pItem = new CommandItem(
                        buffer,
                        szDispName,
                        false,
                        NULL
                        );

                    pItem->m_nSortPriority = M_SORT_NAME;
                    pItem->m_ItemID |= MENUITEM_ID_STYLE;
                }
                else
                {
                    pItem = new CommandItem(
                        NULL,
                        szDispName,
                        false,
                        NULL
                        );

                    if ( char *p = (char*)get_ext(szFullName)){
                        struct ext_list *el;
                        dolist(el, Settings_menuExtStyle){
                            if (0 == stricmp(p, el->ext)){
                                pItem->m_pSI = (StyleItem*)c_alloc(sizeof(StyleItem));
                                memcpy(pItem->m_pSI, &el->si, sizeof(StyleItem));
                                if (!(pItem->m_pSI->validated & VALID_SHADOWCOLOR)){
                                    pItem->m_pSI->validated |= (&mStyle.MenuFrame)->validated & VALID_SHADOWCOLOR;
                                    pItem->m_pSI->ShadowColor = (&mStyle.MenuFrame)->ShadowColor;
                                }
                            }
                        }
                    }

                    if (uAttr & SFGAO_LINK) pItem->m_nSortPriority = M_SORT_NAME;
                }
                pItem->m_pidl = joinIDlists(pIDFolder, pID);

            }
            // free the relative pID
            pMalloc->Free(pID);
            // add item to the list
            pItem->next = *ppItems, *ppItems = pItem;
        }
    }
    if (pMalloc) pMalloc->Release();
    if (pEnumIDList) pEnumIDList->Release();
    pThisFolder->Release();
    return r;
}

//===========================================================================
void init_check_optional_command(const char *cmd, char *current_optional_command)
{
    const char *s;
    if (0 == memicmp(cmd, s = "@BBCore.style %s", 13))
        sprintf(current_optional_command, s, stylePath());
    else
    if (0 == memicmp(cmd, s = "@BBCore.rootCommand %s", 20))
        sprintf(current_optional_command, s, Desk_extended_rootCommand(NULL));
}

//===========================================================================

//===========================================================================
Menu *SingleFolderMenu(const char* path)
{
    char buffer[MAX_PATH];
    char command[MAX_PATH];
    char *s = get_special_command(command, strcpy(buffer, path));

    struct pidl_node* pidl_list = get_folder_pidl_list(buffer);

    char disp_name[MAX_PATH];
    if (pidl_list) sh_getdisplayname(pidl_list->v, disp_name);
    else strcpy(disp_name, get_file(unquote(disp_name, buffer)));

    Menu *m = new SpecialFolder(disp_name, pidl_list, s);
    delete_pidl_list(&pidl_list);

    char IDString[MAX_PATH];
    m->m_IDString = new_str(IDRoot_String(IDString, path));
    return m;
}

//===========================================================================
// for invocation from the keyboard:
// if the menu is present and has focus: hide it and switch to application
// if the menu is present, but has not focus: set focus to the menu
// if the menu is not present: return false -> show the menu.

bool check_menu_toggle(const char *menu_id, bool kbd_invoked)
{
    Menu *m; char IDString[MAX_PATH];

    if (menu_id[0])
        m = Menu::FindNamedMenu(IDRoot_String(IDString, menu_id));
    else
        m = Menu::last_active_menu_root();

    if (NULL == m || NULL == m->m_hwnd)
        return false;

    if (m->has_focus_in_chain()
     && (kbd_invoked || window_under_mouse() == m->m_hwnd))
    {
        focus_top_window();
        Menu_All_Hide();
        return true;
    }

    if (kbd_invoked && m->IsPinned())
    {
        m->set_keyboard_active_item();
        ForceForegroundWindow(m->m_hwnd);
        m->set_focus();
        return true;
    }

    return false;
}

//===========================================================================

bool MenuMaker_ShowMenu(int id, LPARAM param)
{
    char path[MAX_PATH];
    Menu *m = NULL;
    bool from_kbd = false;
    const char *menu_id = NULL;
    int n = 0;

    static const char * menu_string_ids[] =
    {
        "root",
        "workspaces",
        "icons",
        "configuration",
        "processes",
        "",
        NULL
    };

    enum
    {
        e_root,
        e_workspaces,
        e_icons,
        e_configuration,
        e_processes,
        e_lastmenu,
        e_other = -1
    };

    switch (id)
    {
        // -------------------------------
        case BB_MENU_ROOT: // Main menu
            menu_id = menu_string_ids[n = e_root];
            break;

        case BB_MENU_TASKS: // Workspaces menu
            menu_id = menu_string_ids[n = e_workspaces];
            break;

        case BB_MENU_ICONS: // Iconized tasks menu
            menu_id = menu_string_ids[n = e_icons];
            break;

        // -------------------------------
        case BB_MENU_TOOLBAR: // toolbar menu
            Toolbar_ShowMenu(true);
            return true;

        case BB_MENU_CONTEXT:
            m = GetContextMenu((LPCITEMIDLIST)param);
            break;

        case BB_MENU_PLUGIN:
            m = (Menu*)param;
            break;

        // -------------------------------
        case BB_MENU_BYBROAM_KBD: // "BBCore.KeyMenu [param]"
            from_kbd = true;

        case BB_MENU_BYBROAM: // "BBCore.ShowMenu [param]"
            menu_id = (const char *)param;
            n = get_string_index(menu_id, menu_string_ids);
            break;
    }

    if (menu_id)
    {
        if (check_menu_toggle(menu_id, from_kbd))
            return false;

        switch (n)
        {
            case e_root:
            case e_lastmenu:
                m = MakeRootMenu(menu_id, menuPath(), default_root_menu);
                break;

            case e_workspaces:
                m = MakeDesktopMenu(true);
                break;

            case e_icons:
                m = MakeIconsMenu(true);
                break;

            case e_configuration:
                m = MakeConfigMenu(true);
                break;

            case e_processes:
                m = MakeProcessesMenu(true);
                break;

            case e_other:
                if (-1 != (n = get_workspace_number(menu_id)))
                    m = GetTaskFolder(n, true);
                else
                if (FindConfigFile(path, menu_id, NULL))
                    m = MakeRootMenu(menu_id, path, NULL);
                else
                    m = SingleFolderMenu(menu_id);
                break;
        }
    }

    if (NULL == m) return false;
    Menu_ShowFirst(m, from_kbd);
    return true;
}

//===========================================================================
