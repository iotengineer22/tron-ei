[日本語版 (README.md)](README.md)

# I2C Gesture Recognition Edge AI (tron_i2c_detect)

## Program Description
This program runs on μT-Kernel 3.0 and implements real-time edge AI gesture recognition using sampled data from a 3-axis accelerometer (MPU-6050) connected via I2C.
Accelerometer data is polled from the MPU-6050 at 62.5 Hz (16 ms intervals), fed into an Edge Impulse (EI) SDK sliding window buffer, and classified every 80 ms. The probability distribution of four distinct gesture classes (circle, flick, idle, updown) and the highest probability gesture name are displayed on a graphical LCD screen in real time. It also visualizes the raw 3-axis waveforms (X, Y, Z) and a Magnitude bar representing total acceleration force.

## Hardware & Peripherals
* **MCU / Board**: Renesas RA8 Series (e.g., EK-RA8P1, etc.)
* **Sensor**: MPU-6050 3-axis Accelerometer
* **Graphics**: D/AVE 2D Engine, GLCDC (driving 1024x600 TFT LCD Panel)
* **Memory**: External SDRAM (allocated for Triple Buffer framebuffers and model tensors)
* **Probed I2C Pin Pairs (Auto-Detected)**:
  * Arduino Header J24 (P512 / P511)
  * Arduino Header J24 (P400 / P401)
  * PMOD2 J25 (P607 / P608)
  * PMOD1 J26 (P1303 / P1302)

## μT-Kernel 3.0 Task Configuration
1. **task_1** (Priority: 10, Stack size: 32KB)
   * LCD rendering and display task. Awakens on Vblank interrupts (60 Hz). Renders real-time waveforms, Magnitude bars, inference probabilities (%), and the identified gesture name (printed with 4x font scaling on the right column) using Dave2D and CPU-driven fonts.
2. **task_2** (Priority: 9, Stack size: 4KB)
   * Data collection and AI inference task. Assigned a higher priority than the rendering task to ensure sampling stability. Samples data from the MPU-6050 at 62.5 Hz (16 ms delay), updates sliding window buffers, and triggers neural network inference every 5 samples (80 ms cycle).

## Processing Flow
1. **Initialization**: Sets up SDRAM, GLCDC pin registers, Dave2D, pre-renders static UI borders/titles, and probes physical ports to initialize the MPU-6050.
2. **62.5 Hz Polling & 80 ms Inference (task_2)**:
   * Polls 3-axis G-forces from the sensor and streams raw values to the console.
   * Under dispatch-disabled lock, updates the LCD plot histories and the Edge Impulse input buffer `g_raw_accel_buffer`.
   * Runs model classification every 5 samples (80 ms), updating class probabilities and the maximum detected gesture.
3. **Vsync Display Loop (task_1)**:
   * Awakens upon GLCDC Vblank interrupt (`tk_slp_tsk`).
   * Clears the waveform region, plots the X/Y/Z vector lines, and updates the Magnitude bar.
   * Draws probability labels and overlays the detected gesture name (4x scaled) on the right side of the screen.
   * Blocks on GPU completion (`d2_flushframe`) and updates the GLCDC layer buffer.

## Key Parameters & Definitions
* **Recognized Gestures**:
  1. `circle`
  2. `flick`
  3. `idle`
  4. `updown`
* Sampling Rate: `62.5 Hz` (16 ms)
* Inference Cycle: `80 ms` (every 5 samples)
* `RAW_SAMPLE_COUNT` / `TOTAL_RAW_SAMPLES`: Window size variables for Edge Impulse inference.
* Edge Impulse SDK Porting: Maps system timers (`tk_get_tim`, CPU DWT cycle counter for microseconds), `malloc`, `free`, and standard output functions directly to μT-Kernel 3.0 APIs.

## Execution Log Example
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
