[English Version (README_en.md)](README_en.md)

# 加速度センサー描画ビジュアライザ (tron_i2c_d2_test)

## プログラム概要
このプログラムは、μT-Kernel 3.0 上で動作し、I2C 通信で接続された MPU-6050 3軸加速度センサーから高速サンプリング (104Hz) でデータを取得し、グラフィックディスプレイにリアルタイムで波形と合成加速度バーを描画するビジュアライザプログラムです。
I2C 通信は GPIO を制御するソフトウェア I2C (bit-banging) として実装されており、マイコンボード上の複数の接続ポート (Arduinoヘッダー、PMOD等) から加速度センサーが接続されたポートを自動検出 (Auto-Detection) します。Dave2D による高速なオシロスコープ風波形描画と、チラつきを防ぐ CPU フォント描画を組み合わせてスムーズな映像表示を実現しています。

## ハードウェア／周辺機能
* **マイコン / ボード**: ルネサス RA8 シリーズ (例: EK-RA8P1 等)
* **センサー**: MPU-6050 3軸加速度センサー
* **グラフィックス**: D/AVE 2D エンジン、GLCDC (1024x600 TFT 液晶パネル駆動)
* **メモリ**: 外部 SDRAM (トリプルフレームバッファ領域)
* **自動検出対象 I2C ピンペア**:
  * Arduino Header J24 (P512 / P511)
  * Arduino Header J24 (P400 / P401)
  * PMOD2 J25 (P607 / P608)
  * PMOD1 J26 (P1303 / P1302)

## μT-Kernel 3.0 タスク構成
1. **task_1** (優先度: 10, スタックサイズ: 32KB)
   * 液晶描画タスク。Vblank 割り込み (60Hz) で起床し、履歴バッファに格納された最新の 3 軸加速度波形 (X: 赤, Y: 緑, Z: 青) を D/AVE 2D を用いて描画。さらに合成加速度バー (Magnitude) をプロットし、数値情報を画面に CPU フォントを用いて出力しフリップします。
2. **task_2** (優先度: 9, スタックサイズ: 4KB)
   * 高精度データサンプリングタスク。優先度が 9 (描画タスクの 10 より高い) に設定されており、104Hz 周期を安定維持させます。MPU-6050 からの 3 軸の加速度値を読み出し、履歴バッファ `g_accel_x_history` 等をアトミックに更新し、同時にシリアルポートにサンプリングデータを出力します。

## 処理フロー
1. **静的描画のプリレンダリング**: `task_1` 起動時に 3 つのフレームバッファ全てに背景色、外枠、タイトル、グラフ用グリッド枠を事前に描画しておき、描画ループ中のバス負荷を軽減。
2. **センサーポート自動検出**: `task_2` 起動時に、候補ピンペアの WHO_AM_I レジスタ (`0x75`) を読み出し、`0x68` 等が返るポートを自動検出し、初期化 (±2g レンジに設定)。
3. **高精度サンプリング (104Hz)**: `task_2` 内で 13 サイクル周期パターンによるディレイ調整 (10ms×8回 ＆ 9ms×5回 ＝ 平均9.615ms) を行い、正確に 104Hz の周波数を維持してセンサーデータを取得。
4. **アトミックバッファ更新**: `tk_dis_dsp` / `tk_ena_dsp` を用いたディスパッチ禁止ロック下で、104Hz のセンサー履歴バッファおよび画面にプロットするための 3 軸波形バッファ (長さ 307) を更新。
5. **Vblank同期描画 (task_1)**:
   * 液晶の Vblank 割り込みを待ち `tk_slp_tsk` から復帰。
   * Dave2D で波形領域を黒クリアし、3 軸波形をプロット、下部に合成加速度 Magnitude バーを描画。
   * CPU フォント (`font5x7.h` 5x7フォント) を用いて、チラつきのない加速度数値情報を描画。
   * GPU 処理の完了待ち (`d2_flushframe`) 後、GLCDC レイヤーバッファを切り替え。

## 主要なパラメータ・定義
* `HISTORY_LEN` (307): 描画プロット用データ履歴長。
* `PLOT_Y_ZERO` (330): ゼロ加速度基準線の Y 座標。
* `PLOT_WIDTH` (922): グラフ表示の幅。
* `PLOT_STEP_X` (3): 描画ステップピクセル。
* サンプリング周波数: 104.0Hz (平均 9.615ms)

## 実行ログ例
```text
microT-Kernel Version 3.00

Start User-main program (Camera & LCD D2D Test).

=== MPU-6050 Accelerometer Test Start ===
=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===
Clearing SDRAM framebuffers to black...
Initializing LCD (GLCDC)...
LCD Backlight enabled.
Initializing D/AVE 2D Graphics Engine...
Pre-rendering background and static borders to all 3 framebuffers...
Initializing MPU-6050 with auto-detection...
Trying MPU-6050 on Arduino Header J24 (P512/P511)...
MPU6050 WHO_AM_I read: 0x70
SUCCESS: MPU-6050 detected on Arduino Header J24 (P512/P511)!
MPU-6050 initialized successfully.
Starting D/AVE 2D Rendering Loop (Accelerometer Visualizer)...
Loop 0: Vblank: 3, MPU_IRQ: 0, X: 0, Y: 0, Z: 0, MAG: 0
task_2 (104Hz Accelerometer Acquisition) started.
104Hz:-0.956,-0.004,-0.301
104Hz:-0.954,-0.007,-0.304
104Hz:-0.960,-0.006,-0.300
104Hz:-0.960,-0.009,-0.307
104Hz:-0.957,-0.008,-0.307
104Hz:-0.956,-0.005,-0.310
104Hz:-0.955,-0.010,-0.315
104Hz:-0.960,-0.008,-0.309
104Hz:-0.961,-0.010,-0.311
104Hz:-0.961,-0.009,-0.316
104Hz:-0.960,-0.005,-0.306
104Hz:-0.954,-0.006,-0.307
104Hz:-0.957,-0.005,-0.313
104Hz:-0.957,-0.006,-0.304
104Hz:-0.962,-0.006,-0.313
```
