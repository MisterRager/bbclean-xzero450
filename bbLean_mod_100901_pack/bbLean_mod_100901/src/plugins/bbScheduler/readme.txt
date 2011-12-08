/*---------------------------------------------------------------------------------
 bbScheduler (ｩ 2010 nocd5)
 ----------------------------------------------------------------------------------
 bbScheduler is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbScheduler is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/

※プラグインについて
	指定した日時に指定されたコマンドを実行します。
	日時、コマンドは共に"Schedules.rc"で設定されます。
	コマンドはBro@mが使用可能です。

※Schedules.rcのシンタックス
	yyyy.mm.dd.HH.MM.SS <command>で記述します。
	don't careのエレメント(年月日時分)は"*"で記述します。
	尚、"20**"のような記述には対応してませんのでご了承ください。

	ex 0) ****.**.**.**.**.30 foo.exe
			毎分30秒にfoo.exeを実行します。

	ex 1) ****.**.**.12.00.00 @BBCore.ShowMenu
			毎日12時にメニューを表示します。
※注意
	Scheduler.rcのパーシングは非常に軟弱ですので
	"*"の数など注意してください。
	また、秒の部分は"**"にしないでください。
