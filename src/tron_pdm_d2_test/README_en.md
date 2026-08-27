[日本語版 (README.md)](README.md)

# PDM Digital Microphone Audio Visualizer (tron_pdm_d2_test)

## Program Description
This program runs on μT-Kernel 3.0 and captures real-time audio from a digital PDM (Pulse Density Modulation) microphone using DMA transfers. It visualizes the captured audio on a graphical LCD display as an oscilloscope-style waveform and maps volume intensity (RMS and Peak-to-Peak values).
Audio is sampled at 32 kHz in 50 ms frames (1600 samples), which undergo sign-extension and 16-bit normalization. The visualizer plots vector lines at high speed via Dave2D and overlays audio statistics on the screen.

## Hardware & Peripherals
* **MCU / Board**: Renesas RA8 Series (e.g., EK-RA8P1, etc.)
* **Microphone**: Digital PDM Microphone (integrated with DMA transfer)
* **Graphics**: D/AVE 2D Engine, GLCDC (driving 1024x600 TFT LCD Panel)
* **Memory**: External SDRAM (allocated for double-buffered PCM capture arrays and Triple Buffer framebuffers)

## μT-Kernel 3.0 Task Configuration
1. **task_1** (Priority: 10, Stack size: 32KB)
   * UI rendering and display synchronization task. Awakens on Vblank interrupts (60 Hz). Copies the calculated audio history `g_waveform_history` and plots vector lines (green oscilloscope waves). Draws a cyan-colored level Magnitude bar representing RMS volume, and overlays numeric stats using CPU-driven text rendering.
2. **task_2** (Priority: 10, Stack size: 4KB)
   * Microphone controller task. Delays itself for 3 seconds on startup to let the LCD initialize and pre-render, then opens the PDM driver to begin DMA audio acquisition.

## Processing Flow
1. **Microphone DMA Callback (`pdm_callback`)**:
   * Triggered at 32 kHz every 50 ms when a 1600-sample block is received.
   * Invalidates the cache for the double-buffered array, extracts 20-bit signed PCM data packed in 32-bit registers, and sign-extends and normalizes the sample to a 16-bit range (`-32768 to 32767` via 4-bit right-shift).
   * Calculates Peak-to-Peak and RMS (`g_rms_val`) values, updating the plot history buffer `g_waveform_history`.
2. **Vsync Display Loop (task_1)**:
   * Awakens upon GLCDC Vblank interrupt (`tk_slp_tsk`).
   * Clears the plot box, draws the 1024-point audio waveform vector line.
   * Updates the cyan Magnitude bar representing the RMS volume level.
   * Prints statistics (RMS, peak values, frame counter) using CPU-driven font rendering to prevent flickering.
   * Blocks on GPU completion (`d2_flushframe`) and updates the GLCDC layer buffer.

## Key Parameters & Definitions
* `FRAME_SAMPLES` (1600): Audio frame size corresponding to 50 ms at 32 kHz sampling.
* `WAVEFORM_POINTS` (1024): Number of points plotted on the oscilloscope screen.
* `g_pcm32_buffer`: Double-buffered raw DMA capture array.
* `g_pcm_bits` (20): Active bit depth of the PDM microphone.
* `g_rms_val` / `g_peak_to_peak`: Volume statistics computed dynamically.

## Execution Log Example
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
