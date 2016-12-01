# stdlog

[日本語](#ja)
[English](#en)
[License](#license)

<a name="ja">
stdlogはJavaの標準出力、標準エラー出力をすべてファイルにリダイレクトするJVMTIエージェントです。
世代管理機能も有しています。

## 対応環境
* JDK 5以降
* Windows 7以降、またはLinuxカーネル2.6以降

## 使用方法

### stdlogのアタッチ
Java起動オプションに `-agentpath:/path/to/libstdlog.so=<option>` (Linux)、 `-agentpath:\path\to\stdlog.dll=<option>` を追加します。

#### stdlogオプション
オプションは `key1=value1,key2=value2,...` とカンマで結合して渡します。

* `file`
 * 出力するログファイル名
 * 必須設定項目
* `port`
 * コマンド受信用ポート番号
 * デフォルトは `0` (利用可能なクライアントポート番号を割り当て)
* `count`
 * 管理世代数
 * デフォルトは `5`

### ログローテーション
ログローテーションを行う場合は `stdlog-client.jar` でstdlogに対してコマンドを送ります。

```
$ java -jar stdlog-client.jar <port> <command>
```

#### コマンドの種類

* `rotate`
 * ログローテーションの実行
* `reopen`
 * ログファイルのリオープン

## Windowsで利用時の注意点
* stdlogはハードリンクを生成します。そのため、ログ出力先はNTFSにしてください。
* Windows版stdlogはカレントログを `file` で指定したファイルと `<file>.tmp{A,B}` というファイルに出力します。 `file` と `<file>.tmp{A,B}` はハードリンクによって同一ファイルに結び付けられています。 `java` プロセス終了時に `<file>.tmp{A,B}` が残りますが、これは `file` と同じものを指しているため、削除して問題ありません。

## ビルド方法
 * stdlog
  * Windows
   * Visual Studio 2015
   * ビルドユーザーで `JAVA_HOME` 環境変数を設定しておいてください
  * Linux
   * GCC
   * `JAVA_HOME` 環境変数を設定するか、 `make` オプションに `JAVA_HOME=/path/to/JDK` を設定してください
 * stdlog-client
  * `client` ディレクトリで `ant` を実行してください
 
===================
 
<a name="en">
stdlog is a JVMTI agent to redirect stdout and stderr to the file.
It also has a feature of log management.

## Environment
* JDK 5 or later
* Windows 7 or later, Linux kernel 2.6 or later

## How to use

### Attach stdlog
Add `-agentpath:/path/to/libstdlog.so=<option>` (Linux) / `-agentpath:\path\to\stdlog.dll=<option>` to `java` argument.

#### stdlog option
You can pass options with comma as `key1=value1,key2=value2,...` .

* `file`
 * Output logfile name
 * Mandatory option
* `port`
 * Port number to receive the command.
 * `0` is by default (Assign number from client port range)
* `count`
 * Number of log generation
 * `5` is by default

### Log rotation
You need to send the command via `stdlog-client.jar` if you want to rotate the logs.

```
$ java -jar stdlog-client.jar <port> <command>
```

#### Commands

* `rotate`
 * Invoke log rotation
* `reopen`
 * Reopen the log file.

## Notes for Windows
* stdlog generates hard link. So you have to locate the log on NTFS.
* stdlog for Windows outs current log to `file` and `<file>.tmp{A,B}` . They point the same object on file system through hard link. `<file>.tmp{A,B}` will be remained when `java` is terminated. This file is same object of `file` so you can remove it.

## How to build
 * stdlog
  * Windows
   * Visual Studio 2015
   * You have to set `JAVA_HOME` environment variable on build user
  * Linux
   * GCC
   * You have to set `JAVA_HOME` environmet variable, or you have to pass `JAVA_HOME=/path/to/JDK` to `make` argument.
 * stdlog-client
  * run `ant` on `client` directory

===================

<a name="license">
## License
GNU Lesser General Public License version 3
