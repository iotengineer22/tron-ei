[English Version (README_en.md)](README_en.md)

# 音声キーワード検出エッジAI NPU版 (tron_pdm_detect)

## プログラム概要
このプログラムは、μT-Kernel 3.0 上で動作し、デジタル PDM マイクから音声を入力し、内蔵の NPU アクセラレータ「Arm Ethos-U55」を用いて高速音声キーワード検出 (Voice Keyword Detection) を実行するエッジ AI プログラムです。
PDM マイクから 32kHz で DMA キャプチャした音声信号をコールバック処理で 16kHz にダウンサンプリングし、1 秒分のオーディオバッファ（スライディングウィンドウ）に蓄積します。その後、Edge Impulse SDK および Ethos-U55 NPU ドライバを介して推論を行い、特定した音声キーワード、判定時間 (ms)、および音声波形をリアルタイムで液晶ディスプレイ上に表示します。

## ハードウェア／周辺機能
* **マイコン / ボード**: ルネサス RA8 シリーズ (例: EK-RA8P1 等)
* **アクセラレータ**: Arm Ethos-U55 NPU (音声推論高速化用)
* **マイク**: デジタル PDM マイク (DMA 転送連携)
* **グラフィックス**: D/AVE 2D エンジン、GLCDC (1024x600 TFT 液晶パネル駆動)
* **メモリ**: 外部 SDRAM (PCM キャプチャ用ダブルバッファ、スライディングオーディオバッファ、トリプルフレームバッファ領域、モデル領域)

## μT-Kernel 3.0 タスク構成
1. **task_1** (優先度: 10, スタックサイズ: 32KB)
   * 液晶表示タスク。Vblank 周期 (60Hz) で起床し、音声オシロスコープ波形、音量 RMS バー、NPU が分類検出したキーワード結果 (`g_ai_detected_buf`)、および推論時間 (`g_ai_timing_buf`) を液晶上に Dave2D および CPU フォントで描画・フリップします。
2. **task_2** (優先度: 10, スタックサイズ: 4KB)
   * オーディオ収集＆NPU推論タスク。初期ウェイト後、PDM マイクと DMA チャネルをオープン。オーディオデータが十分に蓄積されると (1秒分)、Edge Impulse 分類エンジンを呼び出し、Ethos-U55 NPU を用いたハードウェアアクセラレート推論を実行し、判定結果を更新します。

## 処理フロー
1. **初期化**: SDRAM 初期設定、GLCDC ピン、Dave2D 設定、Ethos-U55 NPU 起動、液晶背景枠描画、および PDM 周辺ドライバのオープン。
2. **マイク受信コールバック (`pdm_callback`)**:
   * 32kHz・50ms (1600サンプル) の DMA 完了時に起動。
   * 32 ビットレジスタから符号付き 20 ビット PCM を取り出し符号拡張し、16 ビットに正規化。
   * 入力データを 2 サンプルに 1 回間引くことで **16kHz へのダウンサンプリング** を実行し、1 秒分のリングバッファ `g_audio_buffer` にデータを書き込み、書込ポインタを更新。同時描画履歴バッファも更新。
3. **音声キーワード推論 (task_2)**:
   * リングバッファに 1 秒分のオーディオデータが溜まり準備ができると (`g_audio_buffer_ready` フラグ)、Edge Impulse SDK の推論関数 `run_classifier` を実行。
   * NPU ハードウェアアクセラレータ上でモデル推論が走行。完了後、検出したキーワードと処理時間をグローバルバッファに格納。
4. **Vblank同期描画 (task_1)**:
   * Vblank 同期で起床し、波形をプロット、音量 Magnitude を表示。
   * NPU 推論された結果（例: `DETECTED: [キーワード]`, `NN: XX ms`）を画面上に大きくオーバーレイ描画。

## 主要なパラメータ・定義
* `AUDIO_BUFFER_SIZE` (16000): 16kHz サンプリング時の 1 秒分バッファサイズ。
* `FRAME_SAMPLES` (1600): PDM キャプチャフレーム (32kHz 50ms)。
* `g_ai_detected_buf` / `g_ai_timing_buf`: 画面に描画される検出テキスト。

## 実行ログ例
```
microT-Kernel Version 3.00

Start User-main program (Camera & LCD D2D Test).
[CHECK 1] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000
Monitoring learning_blocks at address 0x2200036C. Initial: 0x0202D81C
[CHECK 2] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000
[CHECK 3] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000

=== Camera I2C & LCD D2D Connection Test Start ===
Resetting Camera (CAMERA_RESET -> P709)...
Starting GPT Clock for Camera XCLK (g_cam_clk)...
Opening I2C Master (g_cam_i2c_master)...
Reading OV5640 Product ID registers via I2C...
Product ID Read: H = 0x56, L = 0x40
SUCCESS: Camera connection verified! (OV5640 detected)
Testing physical SDRAM at address 0x68000000...
SDRAM verification SUCCESS!
[CHECK 4] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000
=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===
Clearing SDRAM framebuffers to black...
[CHECK 5] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000
Initializing LCD (GLCDC)...
=== Starting Arm Ethos-U55 NPU Voice Detection Task (task_3)... ===
[RM_ETHOSU_Open] p_ctrl = 0x22000308
[RM_ETHOSU_Open] p_ctrl->p_ext_cfg = 0x22000320
[RM_ETHOSU_Open] p_ctrl->p_ext_cfg->p_dev = 0x2204C0A8
SUCCESS: Arm Ethos-U55 NPU driver initialized successfully!
=== DIAGNOSTICS ===
Size of ei_impulse_t: 112
Offset of dsp_blocks: 64
Offset of learning_blocks: 72
Offset of postprocessing_blocks: 80
Offset of label_count: 98
Offset of categories: 100
impulse_943529_1 address: 0x22000324
learning_blocks value in struct: 0x0202D81C
learning_blocks actual address: 0x0202D81C
===================
[CHECK 6] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000
[CHECK 6A] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000
LCD Backlight enabled.
[CHECK 6B] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x00000000
Initializing D/AVE 2D Graphics Engine...
[_sbrk LOG 0] incr = 0, offset_before = 0x00000000, ret = 0x220004A0
[_sbrk LOG 1] incr = 12, offset_before = 0x00000000, ret = 0x220004A0
[_sbrk LOG 2] incr = 12, offset_before = 0x0000000C, ret = 0x220004AC
[_sbrk LOG 3] incr = 12, offset_before = 0x00000018, ret = 0x220004B8
TEST MALLOC: address = 0x220004C8
D2_HANDLE Address: 0x22000538
[_sbrk D2 LOG 5] incr = 476, offset_before = 0x00000090, ret = 0x22000530
[_sbrk D2 LOG 6] incr = 388, offset_before = 0x0000026C, ret = 0x2200070C
[_sbrk D2 LOG 7] incr = 120, offset_before = 0x000003F0, ret = 0x22000890
[CHECK 6C] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x220004C4
[CHECK 7] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x22000524
Pre-rendering background and static borders to all 3 framebuffers...
[CHECK 8] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x22000524
Opening PDM driver (g_pdm0)...
[CHECK 9] learning_blocks = 0x0202D81C, sbrk_offset = 0x00000000, sbrk_start = 0x220004A0, free_list = 0x22000524
PDM driver opened. Waiting for settling...
PDM capture started successfully.
Starting D/AVE 2D Rendering Loop (PDM Audio Visualizer)...
Loop 0: Vblank: 6, PDM: 0, RMS: 0, DrawBuf: 0
  Raw Samples: 0x00000110 0x00000132 0x0000010E 0x00000131
[task_3] DETECTED: noise (34%) | DSP: 22.328 ms | NN: 0.235 ms | Total: 22.563 ms
[task_3] DETECTED: noise (84%) | DSP: 28.764 ms | NN: 0.235 ms | Total: 28.999 ms
[task_3] DETECTED: noise (83%) | DSP: 28.890 ms | NN: 0.235 ms | Total: 29.125 ms
[task_3] DETECTED: noise (63%) | DSP: 28.744 ms | NN: 0.235 ms | Total: 28.979 ms
[task_3] DETECTED: noise (85%) | DSP: 22.328 ms | NN: 0.235 ms | Total: 22.563 ms
[task_3] DETECTED: up (98%) | DSP: 22.329 ms | NN: 0.235 ms | Total: 22.564 ms
[task_3] DETECTED: up (99%) | DSP: 28.751 ms | NN: 0.235 ms | Total: 28.986 ms
[task_3] DETECTED: up (99%) | DSP: 28.751 ms | NN: 0.235 ms | Total: 28.986 ms
[task_3] DETECTED: up (99%) | DSP: 22.328 ms | NN: 0.235 ms | Total: 22.563 ms
[task_3] DETECTED: up (91%) | DSP: 22.328 ms | NN: 0.235 ms | Total: 22.563 ms
[task_3] DETECTED: up (98%) | DSP: 28.730 ms | NN: 0.235 ms | Total: 28.965 ms
[task_3] DETECTED: noise (69%) | DSP: 28.787 ms | NN: 0.235 ms | Total: 29.022 ms
```
