/*---------------------------------------------------------------------------------
 bbWheelHook (ｩ 2006-2008 nocd5)
 ----------------------------------------------------------------------------------
 bbWheelHook is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbWheelHook is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/

※プラグインについて
	本来マウスホイールのメッセージはアクティブなウィンドウに送られますが、
	コレを横取りしてマウス直下のウィンドウに送るプラグインです。


※おまけ機能
	(注意) 機能しないアプリケーションも数多くありますがご了承ください

	・Wheel + Ctrl
		一行スクロールします。

	・Wheel + Alt
		横スクロールします。

	・LeftDown + LWIN + Ctrl
		ウィンドウのどこでも(メニューは除く)ドラッグでウィンドウを移動します。

	・LeftDown + LWIN + Alt
		ウィンドウのどこでも(メニューは除く)ドラッグでウィンドウのサイズを変更します。

	上記の機能は "+ Esc" で無効にすることができます。


※除外リスト : (bbWheelHookのディレクトリ)/exclusions.rc
	 ▲注意
	 exclusions.rcの読み込みはbbWheelHook.dllロード時のみです。
	 blackboxのReconfigureやRestartでは再読み込みは行われません。

	(パス):(クラス名)
	と記述します。
	-- [例] -----------------------------------------------
	C:\Program Files\Paint.NET\PaintDotNet.exe
	E:\Tools\Jane2ch\Jane2ch.exe:TTabControl
	E:\Tools\MediaPlayerClassic\mpc.exe:msctls_trackbar32
	-------------------------------------------------------

	パスのみの場合はアプリケーション全体、クラス名まで書いてある場合は
	そのウィンドウ(コントロール)だけが除外対象となります。	
	除外対象上での一行スクロールと横スクロールは常に無効になります。

	また、そのアプリケーションがアクティブウィンドウの場合、
	除外対象上でのホイール動作はbbWheelHookを経由せず、
	メッセージは直接アプリケーションへ送られます。
	アクティブでない場合は、除外対象上でのホイール動作は
	トップレベルのウィンドウへ送られます。
	除外対象でない場合はアクティブかどうかに関わらず
	カーソル直下のウィンドウ(コントロール)へメッセージが送られます。


※SearchClassName.exeについて
	SearchClassName.exeはフリーウェアです。
	いかなる損害が発生してもnocd5は一切責任を負いませんので
	自己責任でお使いください。

	SearchClassNameはとってもチープなクラス名調べツールです。
	エディットボックス隣の"D&D"というところを
	調べたいウィンドウへD&Dして下さい。
	レジストリ等は一切触りません。

	+------------------+
	|                  | D&D ← ココ
	+------------------+

	フルパス:クラス名
	がエディットボックスに表示されますので、
	そのままexclusions.rcにコピペして下さい。

