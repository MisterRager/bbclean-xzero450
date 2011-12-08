※予期せぬ問題が発生する可能性がありますので、
　定期的に設定のバックアップをとっておいてください。

追加機能

  ディスク使用/空き領域モニタ
  
  ネットワークモニタ
      (タスクマネージャのネットワークの所で表示される順序が
       インターフェース番号の順番と対応している...多分)
       
  ネットワークモニタ専用オートスケールグラフ
  
  iTunesサポート
      再生・停止等のコントロール
      再生中のトラック情報テキストの表示
      再生中のトラックのアートワーク表示
      iTunesの状態取得(bool) 
	  など。

  時計表示

  システム情報テキスト表示
    ホスト名・ユーザ名・PC稼動時間
    OS名・OSビルド番号
    bb4win バージョン・スタイル名
      
  フォント変更のサポート
      (Window Options->Font->Font,Font Size,Font Bold)
      
  フルスクリーンアプリ検出
      フルスクリーンアプリケーションを自動検出し、
      Always On Topの挙動を
        ・フルスクリーン画面よりも常に上に配置
        ・フルスクリーン画面が検出されたら隠す
      の2種から選択できるように変更
      (Detect Fullscreen Appsオプション)
      
  ボタンを押したときのスタイル変更をサポート

  スライダーのカスタムスタイルをサポートし、いくつかオプションを追加

  bbClean & bbLean mod の Text Shadow を反映させるように変更
  個別に詳細設定することも可能に変更
    (それに伴い、enable shadowsオプションを廃止)
  
  borderColor,borderWidthを個別に設定可能に変更

  以下、リクエストがあって追加したもの
  ・スタイルに"Label"と"Clock"を追加
  ・スタイルに"Button"と"PressedButton"を追加
  ・複雑なカスタムスタイルオプションをサポート(Advancedメニュー内) 
      (ToolbarStyleだけどbevelは常にflat、とかできたりします)
  ・グラフの色変更をサポート
      (Graph - Option - Custom Chart Color & Chart Color )
  ・全てのワークスペースに表示を【しない】場合に、
  　表示するワークスペースを指定&保存可能に変更


バグとりっぽいもの
  コンフィグファイル再読み込みや短時間でのLoad/Unloadを行った場合に
  タイマーが残って動作がおかしくなるのを修正
  
サンプルのモジュール
  kazmix_samplescript\iTunesModule.rc   iTunes Control Frame
  kazmix_samplescript\sysmeterModule.rc System Monitor Frame

注意？
  微妙に安定性悪し。
  iTunesが立ち上がっているときにPluginのUnloadをしないほうが良いかも。

  RewindやFastForwardは、
    MouseDown->Rewind
    MouseUp->Resume FastForward/Rewind
  で設定すると幸せになれると思います。


改造した人
  kazmix(かずみくす)
  http://www.kazmix.com/cside.php
  kazmix@kazmix.com
  

更新履歴
10/10/09 ver 0.9.9_k10b
  Windows7でクラッシュする問題を修正？

09/01/05 ver 0.9.9_k10
  Style-Custom内にBorderメニューを追加(Color&Width)
  iTunes関連での追加
    Track Has Artwork(Switched State Valueで利用可能 Artworkの有無を判別)
	Rating(レート)の追加 (Sliderで利用時は変更も可能、Captionは確認のみ)
  StyleにButtonとPressedButtonを追加
  表示するワークスペースの保存機能を追加
  bug fix (Localeの設定ミス)

07/05/14 ver 0.9.9k9c
  AutoScaleGraphのスケーリング固定オプションが保存されない不具合を修正

07/05/08 ver 0.9.9k9b
  削除し忘れていた無駄なコードを削除
  プラグインのバージョンを変更してなかったのを修正

07/05/06 ver 0.9.9k9
  グラフの色変更機能追加
  AutoScaleGraphのスケーリングを固定にできるオプション追加
  スライダーのカスタムスタイル周りの機能実装・追加
  時計表示機能・システム情報表示機能追加
  Shadowの実装を変更(bbClean,bbLean Modのstyle引継ぎ＆アイテム個別設定)

07/03/05 ver 0.9.9k8
  影を付けるとフォント変更が反映されなかったバグを修正
  ボタンを押したときのフォントも変更されるように修正
  フォント変更≠カスタムスタイルに変更(例：ToolbarStyle+CustomFontが可能に)
  ボタンを押したときのスタイル・フォントを変更する機能を追加
  スタイルに"Label"と"Clock"を追加
  カスタムスタイルにAdvancedオプション追加
  ※以下、改造前のbbinterfaceからある不具合の修正
    apply bug fix patch from pkt-zer0
    パスに全角文字が入っているとico以外の画像が表示できないバグを修正

07/02/08 ver 0.9.9k7
  フォント指定部分の内部処理を変更
    システムモニタのテキスト部分などがバグるのを回避…できた？

07/02/07 ver 0.9.9k6
  最大化アプリ検出機能追加

07/02/06 ver 0.9.9k5b
  bug fix

07/02/04 ver 0.9.9k5
  フォント指定を選択方式に変更

07/02/02 ver 0.9.9k4
  フォント変更機能を仮実装

06/12/13 ver 0.9.9k3
  AutoScaleGraphのFilled Chartのバグを修正

06/11/22 ver 0.9.9k2
  ネットワークモニタを拡張
    -アップロード/ダウンロード/合計を選択可能に
  
  ディスク領域モニタを拡張(System Monitorから分離)
    -監視するドライブを選択可能に(UNCも指定可)
      (ex. "C:\","D:\","\\server\disk")
  
  ディスク領域モニタの値が更新されないことがあるバグを修正
  
06/11/22 (ver 0.9.9k1)
  iTunesコントロールでRewindとFastForwardから戻る術が無かったのを修正
    
06/11/21
  公開開始
