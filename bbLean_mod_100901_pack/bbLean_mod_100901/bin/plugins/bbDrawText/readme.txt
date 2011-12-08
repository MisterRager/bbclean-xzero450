/* はじめに */
デスクトップにテキストを表示するだけのプラグインです。
正確に言うと、デスクトップと同じ背景のウィンドウに文字を表示しています。

/* 特徴 */
ClearTypeフォントを使っててもキレイに文字を表示できます。

/* Bro@m */
引数のタイプ/Bro@m/動作
void    @bbDrawText.Reconfigure  設定ファイル再読み込み
bool    @bbDrawText.snapWindow   ウィンドウをスナップさせる
void    @bbDrawText.editRC       設定ファイルを編集
int     @bbDrawText.fontHeight   フォントサイズ
string  @bbDrawText.Font         フォント
int     @bbDrawText.TextColor    フォントの色
int     @bbDrawText.ShadowColor  フォントシャドウの色
int     @bbDrawText.OutlineColor フォントアウトラインの色
string  @bbDrawText.filePath     表示するテキストファイルのパス
void    @bbDrawText.openPath     ファイル選択ダイアログを表示
void    @bbDrawText.editFile     表示しているファイルを編集する

/* マウス動作 */
左ダブルクリック @bbDrawText.editFileと同じ
Ctrl+右クリック  メニュー
Ctrl+左ドラッグ  移動
Alt+左ドラッグ   サイズ変更
右クリック       bbDrawText.rc内で設定可能

/* その他 */
･フォントシャドウ、フォントアウトラインを無効にする時は
 "-1"を指定して下さい。
･ファイルパスを指定するときはダブルクォーテーションで"囲まない"
･editFileでファイルを編集/変更しても自動では更新されない
･blackboxのスタイルで指定する画像ではなく、
 Windowsの方で指定する壁紙を表示してしまうようです。
 僕の知識不足で現状では改善できていません。
 申し訳ありませんが、ご了承ください。とりあえず、
 "Smart Wallpaper"のオプションをFalseにすれば大丈夫だと思います。

/* さいごに */
bbDrawTextはフリーウェアです。
コレによって引き起こされるいかなる損害に対しても、
私、nocd5は一切の責任を負いかねますので、
使用に際しては自己責任でお願いします。
