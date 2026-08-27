[日本語版 (README.md)](README.md)

# Accelerometer Waveform Visualizer (tron_i2c_d2_test)

## Program Description
This program runs on μT-Kernel 3.0 and visualizes real-time accelerometer readings. It samples data from an MPU-6050 3-axis accelerometer at a high frequency (104 Hz) via software-driven I2C (bit-banging), displaying active waveforms and a magnitude bar on a graphical LCD display.
The software I2C driver features auto-detection of the physical connection, probing multiple candidates (Arduino headers and PMOD interfaces) on the microcontroller board. The UI combines high-speed oscilloscope-style vector line drawing using Dave2D with flicker-free text rendering using CPU-drawn fonts.

## Hardware & Peripherals
* **MCU / Board**: Renesas RA8 Series (e.g., EK-RA8P1, etc.)
* **Sensor**: MPU-6050 3-axis Accelerometer
* **Graphics**: D/AVE 2D Engine, GLCDC (driving 1024x600 TFT LCD Panel)
* **Memory**: External SDRAM (allocated for Triple Buffer framebuffers)
* **Probed I2C Pin Pairs (Auto-Detected)**:
  * Arduino Header J24 (P512 / P511)
  * Arduino Header J24 (P400 / P401)
  * PMOD2 J25 (P607 / P608)
  * PMOD1 J26 (P1303 / P1302)

## μT-Kernel 3.0 Task Configuration
1. **task_1** (Priority: 10, Stack size: 32KB)
   * LCD rendering task. Awakens on Vblank interrupts (60 Hz). Copies the sampled 3-axis histories (X: Red, Y: Green, Z: Blue) and plots vector lines via Dave2D. Draws a magnitude bar representing acceleration force and outputs numerical stats using CPU-driven text rendering.
2. **task_2** (Priority: 9, Stack size: 4KB)
   * High-accuracy data sampling task. Assigned a higher priority (9) than the rendering task (10) to stabilize the 104 Hz sampling frequency. Reads the MPU-6050 values, updates histories atomically under dispatch-disabled lock, and streams raw outputs to the console.

## Processing Flow
1. **Pre-rendering static elements**: Upon task_1 startup, draws backgrounds, borders, titles, and grid lines across all three framebuffers to conserve bus bandwidth during the rendering loop.
2. **Port Auto-Detection**: Upon task_2 startup, probes candidates by reading the WHO_AM_I register (`0x75`) to detect the connected MPU-6050 (expecting `0x68`) and configures it to ±2g range.
3. **High-Accuracy 104 Hz Sampling**: Adapts delay intervals in task_2 using a 13-cycle patterns (10ms * 8 cycles + 9ms * 5 cycles = 9.615ms average) to maintain an accurate 104 Hz sampling rate.
4. **Atomic Updates**: Updates history arrays (length 307) under lock (`tk_dis_dsp` / `tk_ena_dsp`) to prevent data corruption during rendering.
5. **Vsync Display Loop**:
   * Awakens upon GLCDC Vblank interrupt (`tk_slp_tsk` awakened by callback).
   * Clears the plot box, draws 3-axis waveforms, and updates the Magnitude bar at the bottom.
   * Renders numeric labels using a CPU font (`font5x7.h` 5x7 bitmap font) to eliminate screen flickering.
   * Blocks on GPU completion (`d2_flushframe`) and updates the GLCDC layer buffer.

## Key Parameters & Definitions
* `HISTORY_LEN` (307): Waveform plot history length.
* `PLOT_Y_ZERO` (330): Y-coordinate for 0g center line.
* `PLOT_WIDTH` (922): Width of the graph.
* `PLOT_STEP_X` (3): X-axis increment step.
* Sampling Rate: 104.0 Hz (9.615 ms average interval)

## Execution Log Example
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
