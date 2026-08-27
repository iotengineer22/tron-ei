[English Version (README_en.md)](README_en.md)

# 音声キーワード検出エッジAI CPU版 (tron_pdm_detect_cpu)

## プログラム概要
このプログラムは、μT-Kernel 3.0 上で動作し、デジタル PDM マイクから入力された音声に対し、TensorFlow Lite Micro (CPU 実行) を用いて音声キーワード検出 (Voice Keyword Detection) を実行するエッジ AI プログラムです。
PDM マイクから 32kHz で DMA キャプチャしたデータをコールバックで 16kHz にダウンサンプリングし、1 秒分のオーディオリングバッファに蓄積します。その後、Edge Impulse SDK を用いて CPU 上で分類推論を実行し、特定した音声キーワード、判定時間 (ms)、および音声波形を液晶上にリアルタイムで重ね描き表示します。NPU が搭載されていない、または NPU ドライバを使用しない環境用の CPU 実装バージョンです。

## ハードウェア／周辺機能
* **マイコン / ボード**: ルネサス RA8 シリーズ (例: EK-RA8P1 等)
* **マイク**: デジタル PDM マイク (DMA 転送連携)
* **グラフィックス**: D/AVE 2D エンジン、GLCDC (1024x600 TFT 液晶パネル駆動)
* **メモリ**: 外部 SDRAM (PCM キャプチャ用ダブルバッファ、スライディングオーディオバッファ、トリプルフレームバッファ領域、モデル領域)

## μT-Kernel 3.0 タスク構成
1. **task_1** (優先度: 10, スタックサイズ: 32KB)
   * 液晶表示タスク。Vblank 周期 (60Hz) で起床し、音声オシロスコープ波形、音量 RMS バー、CPU が分類検出したキーワード結果 (`g_ai_detected_buf`)、および推論時間 (`g_ai_timing_buf`) を液晶上に Dave2D および CPU フォントで描画・フリップします。
2. **task_2** (優先度: 10, スタックサイズ: 4KB)
   * オーディオ収集＆CPU推論タスク。初期ウェイト後に PDM マイクと DMA チャネルをオープン。オーディオデータが十分に蓄積されると (1秒分)、Edge Impulse 分類エンジンを呼び出し、CPU 上で TFLite Micro 推論を実行して判定結果を更新します。

## 処理フロー
1. **初期化**: SDRAM 初期設定、GLCDC ピン、Dave2D 設定、液晶背景枠描画、および PDM 周辺ドライバのオープン。
2. **マイク受信コールバック (`pdm_callback`)**:
   * 32kHz・50ms (1600サンプル) の DMA 完了時に起動。
   * 32 ビットレジスタから符号付き 20 ビット PCM を取り出し符号拡張し、16 ビットに正規化。
   * 2 サンプルに 1 回間引くことで **16kHz へのダウンサンプリング** を実行し、1 秒分のリングバッファ `g_audio_buffer` にデータを書き込み、書込ポインタを更新。同時描画履歴バッファも更新。
3. **音声キーワード推論 (task_2)**:
   * リングバッファに 1 秒分のオーディオデータが溜まり準備ができると (`g_audio_buffer_ready` フラグ)、Edge Impulse SDK の推論関数 `run_classifier` を実行。
   * CPU 上でモデル推論が走行。完了後、検出したキーワードと処理時間をグローバルバッファに格納。
4. **Vblank同期描画 (task_1)**:
   * Vblank 同期で起床し、波形をプロット、音量 Magnitude を表示。
   * 推論結果（例: `DETECTED: [キーワード]`, `NN: XX ms`）を画面上に大きくオーバーレイ表示。

## 主要なパラメータ・定義
* `AUDIO_BUFFER_SIZE` (16000): 16kHz サンプリング時の 1 秒分バッファサイズ。
* `FRAME_SAMPLES` (1600): PDM キャプチャフレーム (32kHz 50ms)。
* `g_ai_detected_buf` / `g_ai_timing_buf`: 画面に描画される検出テキスト。

## 実行ログ例
```
microT-Kernel Version 3.00

Start User-main program (Camera & LCD D2D Test).
[CHECK 1] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000
Monitoring learning_blocks at address 0x22000350. Initial: 0x0201990C
[CHECK 2] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000
[CHECK 3] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000

=== Camera I2C & LCD D2D Connection Test Start ===
Resetting Camera (CAMERA_RESET -> P709)...
Starting GPT Clock for Camera XCLK (g_cam_clk)...
Opening I2C Master (g_cam_i2c_master)...
Reading OV5640 Product ID registers via I2C...
Product ID Read: H = 0x56, L = 0x40
SUCCESS: Camera connection verified! (OV5640 detected)
Testing physical SDRAM at address 0x68000000...
SDRAM verification SUCCESS!
[CHECK 4] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000
=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===
Clearing SDRAM framebuffers to black...
[CHECK 5] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000
Initializing LCD (GLCDC)...
=== Starting CPU Voice Detection Task (task_3)... ===
SUCCESS: CPU Voice Detection Task initialized successfully!
[CHECK 6] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000
[CHECK 6A] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000
LCD Backlight enabled.
[CHECK 6B] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x00000000
Initializing D/AVE 2D Graphics Engine...
[_sbrk LOG 0] incr = 0, offset_before = 0x00000000, ret = 0x22000480
[_sbrk LOG 1] incr = 12, offset_before = 0x00000000, ret = 0x22000480
TEST MALLOC: address = 0x22000490
D2_HANDLE Address: 0x22000500
[_sbrk D2 LOG 3] incr = 476, offset_before = 0x00000078, ret = 0x220004F8
[_sbrk D2 LOG 4] incr = 388, offset_before = 0x00000254, ret = 0x220006D4
[_sbrk D2 LOG 5] incr = 120, offset_before = 0x000003D8, ret = 0x22000858
[CHECK 6C] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x2200048C
[CHECK 7] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x220004EC
Pre-rendering background and static borders to all 3 framebuffers...
[CHECK 8] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x220004EC
Opening PDM driver (g_pdm0)...
[CHECK 9] learning_blocks = 0x0201990C, sbrk_offset = 0x4A884227, sbrk_start = 0x22000480, free_list = 0x220004EC
PDM driver opened. Waiting for settling...
PDM capture started successfully.
Starting D/AVE 2D Rendering Loop (PDM Audio Visualizer)...
Loop 0: Vblank: 6, PDM: 0, RMS: 0, DrawBuf: 0
  Raw Samples: 0x000000A2 0x0000007B 0x0000009C 0x00000083
[task_3] DETECTED: noise (84%) | DSP: 27.720 ms | NN: 0.596 ms | Total: 28.316 ms
[task_3] DETECTED: noise (99%) | DSP: 28.199 ms | NN: 0.596 ms | Total: 28.795 ms
[task_3] DETECTED: noise (98%) | DSP: 22.318 ms | NN: 6.468 ms | Total: 28.786 ms
[task_3] DETECTED: noise (49%) | DSP: 22.319 ms | NN: 0.596 ms | Total: 22.915 ms
[task_3] DETECTED: noise (58%) | DSP: 22.066 ms | NN: 0.596 ms | Total: 22.662 ms
[task_3] DETECTED: up (63%) | DSP: 27.947 ms | NN: 0.595 ms | Total: 28.542 ms
[task_3] DETECTED: up (42%) | DSP: 27.853 ms | NN: 0.595 ms | Total: 28.448 ms
[task_3] DETECTED: noise (97%) | DSP: 27.859 ms | NN: 0.596 ms | Total: 28.455 ms
[task_3] DETECTED: noise (96%) | DSP: 22.065 ms | NN: 0.596 ms | Total: 22.661 ms
[task_3] DETECTED: noise (97%) | DSP: 22.065 ms | NN: 0.595 ms | Total: 22.660 ms
[task_3] DETECTED: noise (97%) | DSP: 27.976 ms | NN: 0.596 ms | Total: 28.572 ms
[task_3] DETECTED: up (48%) | DSP: 28.001 ms | NN: 0.595 ms | Total: 28.596 ms
Loop 100: Vblank: 106, PDM: 89, RMS: 2, DrawBuf: 2
```
