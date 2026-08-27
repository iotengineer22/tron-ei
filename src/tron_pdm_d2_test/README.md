[English Version (README_en.md)](README_en.md)

# PDMデジタルマイク音声ビジュアライザ (tron_pdm_d2_test)

## プログラム概要
このプログラムは、μT-Kernel 3.0 上で動作し、DMA 転送を用いてデジタル PDM (Pulse Density Modulation) マイクからリアルタイムに音声をキャプチャし、グラフィック液晶ディスプレイ上に音声のオシロスコープ波形と音量強度 (RMS値, Peak-to-Peak値) を表示する音声ビジュアライザプログラムです。
PDM マイクから 32kHz のサンプリング周波数で 50ms フレーム (1600サンプル) 単位で入力されるデータを符号拡張および 16 ビット正規化処理し、D/AVE 2D を用いて高速描画を行い、音量統計情報を画面上にオーバーレイ表示します。

## ハードウェア／周辺機能
* **マイコン / ボード**: ルネサス RA8 シリーズ (例: EK-RA8P1 等)
* **マイク**: デジタル PDM マイク (DMA 転送連携)
* **グラフィックス**: D/AVE 2D エンジン、GLCDC (1024x600 TFT 液晶パネル駆動)
* **メモリ**: 外部 SDRAM (ダブルバッファリング対応の PCM キャプチャバッファ、トリプルフレームバッファ領域)

## μT-Kernel 3.0 タスク構成
1. **task_1** (優先度: 10, スタックサイズ: 32KB)
   * 描画・同期制御タスク。Vblank 割り込み (60Hz) に同期して起床し、DMA 割り込みから算出した音声データ履歴バッファ `g_waveform_history` を使ってオシロスコープ波形を描画します。また、上部に RMS 音量インジケータバーをプロットし、各種数値データを CPU フォントで重ね描きしてバッファをフリップします。
2. **task_2** (優先度: 10, スタックサイズ: 4KB)
   * マイク制御タスク。タスク起動時に 3 秒待機し、液晶パネルや背景画面の初期レンダリングが完了した後に、PDM 周辺ドライバをオープンして DMA によるデータ収集を開始します。

## 処理フロー
1. **マイク DMA 割り込みコールバック (`pdm_callback`)**:
   * 32kHz、50ms ごと (1600サンプル) に PDM データの受信が完了すると起動。
   * 受信したバッファのキャッシュを無効化し、32 ビットレジスタにパッキングされた 20 ビットの符号付き PCM データを正しい符号拡張 (sign-extension) を施して取り出し、さらに 16 ビット範囲 (`-32768〜32767`) に正規化 (4ビット右シフト)。
   * サンプルデータから Peak-to-Peak 値および RMS 値 (`g_rms_val`) を高速計算し、同時に描画用波形履歴 `g_waveform_history` を更新。
2. **Vblank同期描画 (task_1)**:
   * Vblank 割り込みを待ち `tk_slp_tsk` から復帰。
   * Dave2D で波形領域を黒クリアし、音声のオシロスコープ波形（緑色の細線）を 1024 点プロットして描画。
   * RMS 値をインジケータ幅に換算し、シアン色のレベルインジケータバーを描画。
   * 音量値、ピーク値、受信フレーム数を CPU フォントでチラつきなく表示。
   * GPU 処理の完了を待ち、GLCDC 液晶バッファを切り替え。

## 主要なパラメータ・定義
* `FRAME_SAMPLES` (1600): 32kHz サンプリング時の 50ms 分のオーディオフレームサイズ。
* `WAVEFORM_POINTS` (1024): 画面プロット用のサンプル数。
* `g_pcm32_buffer`: DMA が直接書き込む PCM 一時バッファ (ダブルバッファ構成)。
* `g_pcm_bits` (20): PDM マイクの有効ビット長。
* `g_rms_val` / `g_peak_to_peak`: リアルタイムに計算される音声統計。

## 実行ログ例
```
microT-Kernel Version 3.00

Start User-main program (Camera & LCD D2D Test).
task 2 running...

=== Camera I2C & LCD D2D Connection Test Start ===
Resetting Camera (CAMERA_RESET -> P709)...
Starting GPT Clock for Camera XCLK (g_cam_clk)...
Opening I2C Master (g_cam_i2c_master)...
Reading OV5640 Product ID registers via I2C...
Product ID Read: H = 0x56, L = 0x40
SUCCESS: Camera connection verified! (OV5640 detected)
Testing physical SDRAM at address 0x68000000...
SDRAM verification SUCCESS!
=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===
Clearing SDRAM framebuffers to black...
Initializing LCD (GLCDC)...
LCD Backlight enabled.
Initializing D/AVE 2D Graphics Engine...
Pre-rendering background and static borders to all 3 framebuffers...
Opening PDM driver (g_pdm0)...
PDM driver opened. Waiting for settling...
PDM capture started successfully.
Starting D/AVE 2D Rendering Loop (PDM Audio Visualizer)...
Loop 0: Vblank: 3, PDM: 0, RMS: 0, DrawBuf: 0
  Raw Samples: 0x000FF64C 0x000FF66F 0x000FF6B6 0x000FF6B9
Loop 100: Vblank: 103, PDM: 88, RMS: 12, DrawBuf: 2
  Raw Samples: 0x00000080 0x000FFF62 0x000FFEBA 0x0000003C
task 2 running...
Loop 200: Vblank: 203, PDM: 177, RMS: 2, DrawBuf: 1
  Raw Samples: 0x000FFFED 0x00000009 0x000FFFED 0x00000000
task 2 running...
Loop 300: Vblank: 303, PDM: 265, RMS: 2, DrawBuf: 0
  Raw Samples: 0x000FFFE5 0x00000010 0x000FFFD7 0x00000012
Loop 400: Vblank: 403, PDM: 354, RMS: 2, DrawBuf: 2
  Raw Samples: 0x000FFFE8 0x00000001 0x000FFFF6 0x000FFFF2
```
