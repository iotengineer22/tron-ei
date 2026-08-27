[English Version (README_en.md)](README_en.md)

# I2Cジェスチャ認識エッジAI (tron_i2c_detect)

## プログラム概要
このプログラムは、μT-Kernel 3.0 上で動作し、I2C 接続された 3 軸加速度センサー (MPU-6050) からのサンプリングデータを用いて、エッジ AI によるリアルタイムジェスチャ認識を行うプログラムです。
MPU-6050 から 62.5Hz (16ms周期) で取得した加速度データを Edge Impulse (EI) SDK のスライディングバッファに入力し、80ms 周期で推論を行います。認識対象である 4 種類のジェスチャ (circle, flick, idle, updown) の確率分布と、最も可能性の高いジェスチャ名を液晶ディスプレイ上にリアルタイムで表示し、同時に 3 軸加速度波形 (X, Y, Z) および合成加速度 Magnitude バーも視覚化します。

## ハードウェア／周辺機能
* **マイコン / ボード**: ルネサス RA8 シリーズ (例: EK-RA8P1 等)
* **センサー**: MPU-6050 3軸加速度センサー
* **グラフィックス**: D/AVE 2D エンジン、GLCDC (1024x600 TFT 液晶パネル駆動)
* **メモリ**: 外部 SDRAM (トリプルフレームバッファ領域、推論用一時バッファ領域)
* **自動検出対象 I2C ピンペア**:
  * Arduino Header J24 (P512 / P511)
  * Arduino Header J24 (P400 / P401)
  * PMOD2 J25 (P607 / P608)
  * PMOD1 J26 (P1303 / P1302)

## μT-Kernel 3.0 タスク構成
1. **task_1** (優先度: 10, スタックサイズ: 32KB)
   * 液晶描画・表示制御タスク。Vblank 周期 (60Hz) で起床し、リアルタイム波形、合成加速度バー、推論確率 (%)、および最も可能性の高いジェスチャ名 (大きく拡大表示) を液晶画面に Dave2D と CPU フォントを用いて描画します。
2. **task_2** (優先度: 9, スタックサイズ: 4KB)
   * サンプリングおよび AI 推論タスク。サンプリングの安定性のため描画より優先度を高く設定。MPU-6050 から 62.5Hz (16msディレイ) で加速度データをサンプリングし、Edge Impulse バッファを更新、5サンプルごと (80ms周期) に NPU / CPU 加速推論を実行し認識結果を更新します。

## 処理フロー
1. **初期化シーケンス**: SDRAM 初期化、GLCDC ピン割り当て、Dave2D 起動、液晶外枠のプリレンダリング、および I2C ポート自動検出による MPU-6050 加速度センサーの初期化。
2. **62.5Hz サンプリングと 80ms 推論ループ (task_2)**:
   * MPU-6050 から 3 軸加速度 (g単位) を読み込み、シリアルコンソールに出力。
   * ディスパッチ禁止ロック下で、液晶描画用の履歴バッファおよび Edge Impulse 入力スライディングバッファ `g_raw_accel_buffer` を更新。
   * 5サンプル取得毎に推論関数を起動し、ジェスチャ分類推論を実行。結果 (circle, flick, idle, updown 各クラスの確率値と最大判定結果) を更新。
3. **Vblank同期描画 (task_1)**:
   * Vblank 割り込みを待ち `tk_slp_tsk` から復帰。
   * 波形と合成加速度バーをプロット。
   * 推論結果である「確率グラフ」および特定された「ジェスチャ名」(画面右側に 4 倍拡大フォントで大きくプロット) を表示。
   * GPU 完了待ち (`d2_flushframe`) 後、GLCDC バッファを切り替え。

## 主要なパラメータ・定義
* **認識ジェスチャ**:
  1. `circle` (円を描く動作)
  2. `flick` (素早く振る動作)
  3. `idle` (静止状態)
  4. `updown` (上下に振る動作)
* サンプリングレート: `62.5Hz` (16ms)
* 推論周期: `80ms` (5サンプルごと)
* `RAW_SAMPLE_COUNT` / `TOTAL_RAW_SAMPLES`: Edge Impulse 推論用サンプルサイズ。
* Edge Impulse SDK 移植関数: μT-Kernel 3.0 のシステムタイマー (`tk_get_tim`, DWTサイクルカウンタによるマイクロ秒取得)、`malloc`、`free` などの OS API をマッピングして定義。

## 実行ログ例
```text
microT-Kernel Version 3.00

Start User-main program (Camera & LCD D2D Test).
SUCCESS: Created task_1 (ID: 2)
SUCCESS: Created task_2 (ID: 3)

=== MPU-6050 Accelerometer Test Start ===
=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===
Clearing SDRAM framebuffers to black...
Initializing LCD (GLCDC)...
LCD Backlight enabled.
Initializing D/AVE 2D Graphics Engine...
Pre-rendering background and static borders to all 3 framebuffers...
Ethos-U55 NPU initialized successfully.
Initializing MPU-6050 with auto-detection...
Trying MPU-6050 on Arduino Header J24 (P512/P511)...
MPU6050 WHO_AM_I read: 0x70
SUCCESS: MPU-6050 detected on Arduino Header J24 (P512/P511)!
MPU-6050 initialized successfully.
Starting D/AVE 2D Rendering Loop (Accelerometer Visualizer)...
task_2 (62.5Hz Accelerometer Acquisition & Gesture Inference) started.
62.5Hz:-1002,25,-39
62.5Hz:-998,24,-31
62.5Hz:-1001,24,-40
62.5Hz:-1001,27,-37
62.5Hz:-1004,29,-31
Gesture: idle (circle: 0%, flick: 0%, idle: 99%, updown: 0%)
62.5Hz:-1000,23,-44
62.5Hz:-998,27,-37
62.5Hz:-1001,27,-34
62.5Hz:-1000,24,-35
62.5Hz:-999,24,-39
Gesture: idle (circle: 0%, flick: 0%, idle: 99%, updown: 0%)
62.5Hz:-1000,28,-39
62.5Hz:-995,26,-39
62.5Hz:-999,25,-39
62.5Hz:-999,24,-35
62.5Hz:-1002,27,-37
Gesture: idle (circle: 0%, flick: 0%, idle: 99%, updown: 0%)
62.5Hz:-996,25,-37
62.5Hz:-997,26,-43
62.5Hz:-1000,25,-38
62.5Hz:-1002,25,-34
62.5Hz:-1001,20,-34
```
