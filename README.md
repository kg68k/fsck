# fsck.x - X68k File System Checker
X68k File System Checker Ver 1.03の改造版です。  
無保証につき各自の責任で使用して下さい。


## Build
PCやネット上での取り扱いを用意にするために、src/内のファイルはUTF-8で記述されています。
X68000上でビルドする際には、UTF-8からShift_JISへの変換が必要です。

### u8tosjを使用する方法

あらかじめ、[u8tosj](https://github.com/kg68k/u8tosj)をビルドしてインストールしておいてください。

トップディレクトリで`make`を実行してください。以下の処理が行われます。
1. build/ディレクトリの作成。
2. src/内の各ファイルをShift_JISに変換してbuild/へ保存。

次に、カレントディレクトリをbuild/に変更し、`make`を実行してください。
実行ファイルが作成されます。

### u8tosjを使用しない方法

ファイルを適当なツールで適宜Shift_JISに変換してから`make`を実行してください。
UTF-8のままでは正しくビルドできませんので注意してください。


## License

改造元のfsckの著作権は川本琢二(Ext)氏にあります。  
改造部分の著作権は改造者(TcbnErik)にあります。

配布規定については改造元のfsckに準じます。

詳しくはFSCK103.LZH内のreadme.docを参照してください。


## Author
TcbnErik / https://github.com/kg68k/fsck
