[日本語版 (README.md)](README.md)

# Edge Impulse Integration Middleware Library for μT-Kernel 3.0 (TRON-EI)

This project is a high-performance **Edge AI Integration Middleware Library (TRON-EI)** running under the **μT-Kernel 3.0** real-time OS. It ports and integrates the inference SDK of the industry-standard edge AI platform, **Edge Impulse**, enabling seamless data capture, preprocessing, and real-time inference for image, audio, and motion sensor inputs.

This is a development project for the **RTOS Middleware Category** of the TRON Programming Contest 2026.

📄 **[日本語版 (README.md)](README.md)**

---

> [!IMPORTANT]
> **Pre-built Binaries for Verification & Flashing Guide**
> Pre-built SREC binary files for flashing and verification are located in the **[/debug](debug/)** folder.
>
> A comprehensive flashing manual containing step-by-step guides (including hardware connection specifications, troubleshooting screen freeze, flash memory initialization/erase, and serial logs verification) is located in **[/debug/README.md (Flashing Manual)](debug/README.md)**. Please refer to it when testing the projects.

---

## 1. Middleware Overview & Background

When deploying edge AI models onto a real-time OS (RTOS), developers face major hurdles in abstracting hardware dependencies and mapping OS APIs (such as thread safety, memory allocation, and high-precision timers).

This middleware provides a **porting layer (OS bridge)** that maps the Edge Impulse C++ SDK to μT-Kernel 3.0 native APIs. Additionally, it integrates driver stacks for digital microphones, motion sensors, NPU hardware, and 2D GPU rendering. As a result, application developers can immediately build multi-sensor AI recognition systems without writing low-level hardware binders or OS wrapper code.

---

## 2. Architecture & Core Features

This middleware exposes the following abstracted services to the application layer:

### 1. Edge Impulse Porting Layer (OS Bridge)
Binds the following time, memory, and sync tasks required by Edge Impulse to native μT-Kernel 3.0 APIs:
* **Millisecond and Microsecond Timers**: Wraps system tick timer (`tk_get_tim`) and DWT (Data Watchpoint and Trace) cycle counters to provide microsecond-level execution timing and timestamps.
* **Dynamic Memory Management**: Standardizes dynamic memory pooling or thread-safe `malloc`/`free` mappings within μT-Kernel tasks.
* **Serial Debug Logging**: Integrates console output logging using T-Monitor `tm_printf`/`tm_putstring` APIs.

### 2. Asynchronous Sampling & Inference Pipeline (Producer-Consumer)
Provides a multi-tasking middleware framework to parallelize high-accuracy sensor data capture with computationally heavy AI inference and graphics rendering.

![Sensor Parallel Architecture](img/sensor_parallel_architecture.png)

* **Data Acquisition & Inference Task (`task_2` / Priority 9-10)**: 
  Samples devices at precise intervals (Microphone: 16kHz/32kHz, G-Sensor: 62.5Hz/104Hz), stores raw data in sliding window buffers, and kicks model inference in the background.
* **LCD Rendering Task (`task_1` / Priority 10)**: 
  Wakes up at the Vblank vertical sync edge (60 Hz) to plot raw signal waveforms using the Dave2D GPU and print classification outputs onto the screen.
* **Atomic Buffer Protection**: 
  To prevent data corruption (race conditions) between tasks, the middleware implements atomic update macros utilizing μT-Kernel dispatch lock APIs (`tk_dis_dsp` / `tk_ena_dsp`). This prevents audio or motion buffers from being modified while they are being read by the rendering task.

---

## 3. Sub-Project Directory Structure & Samples (src)

To verify the utility of the middleware, the package contains 6 sample programs covering vision, audio, and motion AI applications:

| Folder | Middleware Implementation Demo | Sensor Input & Model Inference |
| :--- | :--- | :--- |
| **[tron_edge_fomo_npu_type](src/tron_edge_fomo_npu_type)** | FOMO Object Detection Library Demo (NPU) | MIPI Camera / Object Detection via **Arm Ethos-U55 NPU** |
| **[tron_i2c_d2_test](src/tron_i2c_d2_test)** | 3-Axis Accelerometer Waveform Visualizer | MPU-6050 (Software I2C) / High-speed 104Hz Plotting |
| **[tron_i2c_detect](src/tron_i2c_detect)** | 3-Axis Accelerometer Gesture Edge AI | MPU-6050 / **Gesture Classification (4 Action Classes)** |
| **[tron_pdm_d2_test](src/tron_pdm_d2_test)** | PDM Digital Microphone Visualizer | PDM Microphone (32kHz) / RMS Volume & Oscilloscope |
| **[tron_pdm_detect](src/tron_pdm_detect)** | Voice Keyword Recognition (NPU version) | PDM Mic (32kHz to 16kHz) / **Keyword Spotting via NPU** |
| **[tron_pdm_detect_cpu](src/tron_pdm_detect_cpu)** | Voice Keyword Recognition (CPU version) | PDM Mic (32kHz to 16kHz) / **Keyword Spotting via CPU** |

---

## 4. Middleware Innovations & Technical Achievements

### 1. Jitter-Free Digital Audio Downsampling
For the voice keyword detection applications (`tron_pdm_detect`), 32 kHz audio is captured via DMA. The DMA-complete callback interrupt handler performs on-the-fly downsampling to 16 kHz.
Because model inference is offloaded to the background NPU, the CPU does not experience processing spikes during inference. This ensures that the audio callback is never delayed, achieving jitter-free streaming without a single dropped audio frame.

![CPU vs NPU AI Inference Latency Comparison](img/cpu_vs_npu_comparison.png)

### 2. Software I2C (Bit-Banging) with Auto-Port Detection
To support motion sensors even on boards where hardware I2C lines are limited, a GPIO-driven software I2C driver is implemented in the middleware layer.
It features an **auto-port detection loop** that probes 4 potential pin-pairs (Arduino header and PMOD interfaces) by checking the sensor WHO_AM_I register. It automatically binds to the active connection, ensuring robust initialization.

### 3. Jitter-Free 104 Hz Sensor Sampling via Sleep Pattern Calibration
In the accelerometer visualizer (`tron_i2c_d2_test`), using standard RTOS delay calls (`tk_dly_tsk`) is insufficient for a precise 104 Hz sampling rate (average 9.615 ms intervals) due to 1 ms quantization tick jitter.
This middleware integrates a **13-cycle delay correction algorithm** that dynamically adjusts sleep intervals, reducing timing jitter and securing highly precise data sampling.

### 4. GPU-Driven Vector Oscilloscope Line Plotter
To draw audio (1024 points) and motion (307 points) waveforms at high frame rates, the middleware wraps the Dave2D line rendering pipeline.
By rendering full-screen clearing and vector plots within 2 ms, and combining it with a flicker-free CPU-driven text font API, it delivers a smooth visual output without screen tearing.

---

## 5. Development Environment & Execution
* **IDE**: Renesas e2 studio / FSP v6.5.0
* **RTOS**: μT-Kernel 3.0
* **Evaluation Board**: Renesas EK-RA8P1
* **How to Build**: Run `build.bat` in the respective program folder, or import the projects into e2 studio.
* **Deliverables**: Middleware source code, 6 sample programs leveraging the middleware, and console output logs demonstrating successful execution.
