[日本語版 (README.md)](README.md)

# Voice Keyword Detection Edge AI - NPU Version (tron_pdm_detect)

## Program Description
This program runs on μT-Kernel 3.0 and implements real-time voice keyword detection using the built-in "Arm Ethos-U55" NPU accelerator on Renesas RA8 microcontrollers.
Audio data captured from a digital PDM microphone at 32 kHz via DMA is downsampled to 16 kHz in the callback and buffered into a 1.0-second sliding window. The Edge Impulse SDK uses the Ethos-U55 NPU driver to run high-speed hardware-accelerated classification inference, displaying identified keywords, inference latency (ms), and active audio waveforms on the LCD screen in real time.

## Hardware & Peripherals
* **MCU / Board**: Renesas RA8 Series (e.g., EK-RA8P1, etc.)
* **AI Accelerator**: Arm Ethos-U55 NPU (Neural Processing Unit for hardware acceleration)
* **Microphone**: Digital PDM Microphone (integrated with DMA transfer)
* **Graphics**: D/AVE 2D Engine, GLCDC (driving 1024x600 TFT LCD Panel)
* **Memory**: External SDRAM (allocated for PCM double buffers, sliding audio buffers, model tensors, and Triple Buffer framebuffers)

## μT-Kernel 3.0 Task Configuration
1. **task_1** (Priority: 10, Stack size: 32KB)
   * LCD rendering and synchronization task. Awakens on Vblank interrupts (60 Hz). Renders the audio oscilloscope waveform, Magnitude bar, NPU classified keyword text (`g_ai_detected_buf`), and inference latency (`g_ai_timing_buf`) on the screen using Dave2D and CPU-driven fonts.
2. **task_2** (Priority: 10, Stack size: 4KB)
   * Audio collection and NPU inference task. Opens the PDM driver to capture DMA data. Once a full 1.0-second audio window has been filled, it calls the Edge Impulse classification engine to run inference on the Ethos-U55 NPU hardware, updating the result buffers.

## Processing Flow
1. **Initialization**: Configures SDRAM, GLCDC, Dave2D, Ethos-U55 NPU hardware, pre-renders static UI layouts, and enables PDM audio DMA.
2. **Microphone DMA Callback (`pdm_callback`)**:
   * Triggered at 32 kHz every 50 ms (1600 samples).
   * Sign-extends raw 20-bit signed PCM from 32-bit registers, and normalizes it to a 16-bit range.
   * **Downsamples data to 16 kHz** by discarding every second sample, and appends it to the 1.0-second sliding ring buffer `g_audio_buffer`, updating write pointers and waveform histories.
3. **Voice Keyword Inference (task_2)**:
   * When a full 1.0-second window is ready (`g_audio_buffer_ready` flag set), calls `run_classifier`.
   * Executes inference on the Ethos-U55 NPU accelerator. Stores the identified keyword and NPU latency in global text buffers.
4. **Vsync Display Loop (task_1)**:
   * Awakens on Vblank interrupt (`tk_slp_tsk`).
   * Plots waveforms and updates Magnitude bars.
   * Overlays the classification outputs (e.g., `DETECTED: [Keyword]`, `NN: XX ms`) in large text.
   * Flushes GPU command lists and updates the GLCDC layer buffer.

## Key Parameters & Definitions
* `AUDIO_BUFFER_SIZE` (16000): Sliding audio window buffer size for 1.0 second at 16 kHz.
* `FRAME_SAMPLES` (1600): PDM capture frame size (32 kHz, 50 ms).
* `g_ai_detected_buf` / `g_ai_timing_buf`: Output text buffers updated by NPU inference.

## Execution Log Example
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
