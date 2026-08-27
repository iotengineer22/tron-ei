[日本語版 (README.md)](README.md)

# Voice Keyword Detection Edge AI - CPU Version (tron_pdm_detect_cpu)

## Program Description
This program runs on μT-Kernel 3.0 and implements real-time voice keyword detection using TensorFlow Lite Micro (CPU execution) on audio inputs.
Audio data captured from a digital PDM microphone at 32 kHz via DMA is downsampled to 16 kHz in the callback and buffered into a 1.0-second sliding window. The Edge Impulse SDK then runs classification inference directly on the CPU, displaying identified keywords, inference latency (ms), and active audio waveforms on the LCD screen in real time. This version is designed for environments without NPU hardware or where NPU drivers are disabled.

## Hardware & Peripherals
* **MCU / Board**: Renesas RA8 Series (e.g., EK-RA8P1, etc.)
* **Microphone**: Digital PDM Microphone (integrated with DMA transfer)
* **Graphics**: D/AVE 2D Engine, GLCDC (driving 1024x600 TFT LCD Panel)
* **Memory**: External SDRAM (allocated for PCM double buffers, sliding audio buffers, model tensors, and Triple Buffer framebuffers)

## μT-Kernel 3.0 Task Configuration
1. **task_1** (Priority: 10, Stack size: 32KB)
   * LCD rendering and synchronization task. Awakens on Vblank interrupts (60 Hz). Renders the audio oscilloscope waveform, Magnitude bar, CPU classified keyword text (`g_ai_detected_buf`), and inference latency (`g_ai_timing_buf`) on the screen using Dave2D and CPU-driven fonts.
2. **task_2** (Priority: 10, Stack size: 4KB)
   * Audio collection and CPU inference task. Opens the PDM driver to capture DMA data. Once a full 1.0-second audio window has been filled, it calls the Edge Impulse classification engine to run inference on the CPU, updating the result buffers.

## Processing Flow
1. **Initialization**: Configures SDRAM, GLCDC, Dave2D, pre-renders static UI layouts, and enables PDM audio DMA.
2. **Microphone DMA Callback (`pdm_callback`)**:
   * Triggered at 32 kHz every 50 ms (1600 samples).
   * Sign-extends raw 20-bit signed PCM from 32-bit registers, and normalizes it to a 16-bit range.
   * **Downsamples data to 16 kHz** by discarding every second sample, and appends it to the 1.0-second sliding ring buffer `g_audio_buffer`, updating write pointers and waveform histories.
3. **Voice Keyword Inference (task_2)**:
   * When a full 1.0-second window is ready (`g_audio_buffer_ready` flag set), calls `run_classifier`.
   * Executes inference on the CPU using TensorFlow Lite Micro. Stores the identified keyword and CPU latency in global text buffers.
4. **Vsync Display Loop (task_1)**:
   * Awakens on Vblank interrupt (`tk_slp_tsk`).
   * Plots waveforms and updates Magnitude bars.
   * Overlays the classification outputs (e.g., `DETECTED: [Keyword]`, `NN: XX ms`) in large text.
   * Flushes GPU command lists and updates the GLCDC layer buffer.

## Key Parameters & Definitions
* `AUDIO_BUFFER_SIZE` (16000): Sliding audio window buffer size for 1.0 second at 16 kHz.
* `FRAME_SAMPLES` (1600): PDM capture frame size (32 kHz, 50 ms).
* `g_ai_detected_buf` / `g_ai_timing_buf`: Output text buffers updated by CPU inference.

## Execution Log Example
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
