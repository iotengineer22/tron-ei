# EK-RA8P1 Flashing Manual — TRON-EI (Middleware Category)

This folder aggregates and organizes the pre-built binary files (`.srec`) required to flash μT-Kernel 3.0 and the Edge Impulse integrated middleware library onto the Renesas EK-RA8P1 evaluation board.

> [!NOTE]
> **Important Notice on Target Board & Wiring**
> * For verification work, you can use the **"EK-RA8P1 board with I2C accelerometer (MPU-6050) and wiring connected"** exactly as submitted to the contest office.
> * When running **MPU-6050 accelerometer verification programs**, please ensure the accelerometer is correctly wired to Port 1 (Arduino Connector SCL: P100 / SDA: P101) as shown below.
> * When running **Digital Microphone (PDM) verification programs**, no external microphone or wiring is required since the board uses the **on-board MEMS digital microphone (SPH0641LM4H-1)**.
> * When running **FOMO camera-based object detection**, connect the supplied MIPI-CSI2 camera module (OV5640) to the camera port.
>
> ![Board with sensor connected](../../tron-npu/img/tron_debug7.jpg)

---

## 1. Directory Structure

The pre-built SREC files are organized as follows:

* **Directly under `debug/` (Main Programs)**:
  These are the 3 main evaluation applications forming the core of the middleware library.
  * `tron_pdm_detect.srec` (Voice Keyword Spotting on NPU)
    * Listens to the on-board PDM microphone and uses the Ethos-U55 NPU to classify voice commands ("up", "down", "left", "right") with ultra-low latency.
  * `tron_i2c_detect.srec` (3-axis Accelerometer Gesture Recognition)
    * Uses 3-axis accelerometer data from the MPU-6050 to classify board motions into 4 dynamic gesture states ("wave", "snake", "updown", "idle").
  * `tron_pdm_detect_cpu.srec` (Voice Keyword Spotting on CPU)
    * Runs keyword spotting strictly on the Cortex-M85 CPU without NPU acceleration (for comparison/validation).

* **Under `base_firmware/` (Verification & Reference)**:
  These 3 programs evaluate standalone peripheral features or provide a vision model porting reference. **Please flash and verify these components as needed.**
  * `tron_pdm_d2_test.srec` (PDM Mic Waveform Plotter)
    * Streams raw digital audio and plots real-time wave graphs and RMS volume onto the LCD using the Dave2D GPU engine.
  * `tron_i2c_d2_test.srec` (I2C Sensor Reading & 2D Graphics)
    * Captures accelerometer data and plots real-time X, Y, Z axes G-force wave graphs onto the LCD screen.
  * `tron_edge_fomo_npu_type.srec` (FOMO Component Detection on NPU)
    * Identifies and counts tiny electronic components (Pico, Xiao) on PCBs in real-time camera streams (provided as a porting reference).

---

## 2. Hardware Connections & Pin Specifications

The middleware automatically detects sensor ports and drives peripherals as follows:

### ① MPU-6050 Accelerometer (I2C) Port Allocation
The middleware features an **auto-detect scan logic** checking the WHO_AM_I register on startup. It automatically initializes the sensor on the first active port discovered:

| Active Port (Scan Order) | Physical Connector Location | SCL Pin | SDA Pin |
| :--- | :--- | :---: | :---: |
| **PORT 1 (Recommended / Submitted)** | **Arduino Connector SCL/SDA** | **`P100`** | **`P101`** |
| **PORT 2** | Internal GPIO Header | `P206` | `P205` |
| **PORT 3** | **PMOD2** Connector | `P112` | `P113` |
| **PORT 4** | **PMOD1** Connector | `P102` | `P103` |

* **Power Wiring**:
  * Sensor **`VCC`** ➡ Board **`3.3V`** Pin
  * Sensor **`GND`** ➡ Board **`GND`** Pin

### ② PDM Digital Microphone
* Captures digital audio using the standard MEMS mic (SPH0641LM4H-1) on the EK-RA8P1 board.
* The middleware handles 32 kHz PDM acquisition and down-samples the audio stream to 16 kHz inside the DMA interrupt context for AI inference (no external wiring needed).

### ③ MIPI-CSI2 Camera (OV5640)
* Connect the supplied camera module to the MIPI-CSI2 camera connector on the back of the board before running camera-based AI applications.

---

## 3. Flashing Tools

Flashing requires the official free utility **Renesas Flash Programmer (Programming GUI)**.

* **Download Page**:
  [Renesas Flash Programmer (Programming GUI) Official Site](https://www.renesas.com/us/en/software-tool/renesas-flash-programmer-programming-gui#downloads)
  * Download and install the latest Windows installer or matching OS pack on your PC.

---

## 4. Flashing Procedure

Follow these steps to configure your project and flash target binary files.

### ① Hardware Setup
1. Connect your PC to the J-Link Debug port **`DEBUG1`** (micro-USB or USB-C) on the EK-RA8P1 board using a USB cable.

> [!WARNING]
> **Dealing with White-Screen LCD Freeze**
> Sometimes, plugging in the USB cable causes the LCD display to freeze in a pure white state. If this occurs, **simply unplug the USB cable, wait 10 to 20 seconds, and plug it back in.**
>
> ![White screen error state](../../tron-npu/img/tron_debug6.jpg)

2. Verify that the board power LED lights up.

### ② Creating RFP Project

1. Run **Renesas Flash Programmer (RFP)**.
2. Select **[File] ➡ [New Project...]** and configure as follows:

| Field | Value |
| :--- | :--- |
| **Microcontroller** | Select **`RA`** |
| **Project Name** | Any name (e.g., `RA8P1_Demo`) |
| **Project Folder** | Any directory (default is fine) |
| **Tool** | Select **`J-Link`** |
| **Interface** | Select **`SWD`** |

![New Project Screen](../../tron-npu/img/tron_debug2.png)

3. Click **[Create]** to establish a J-Link connection to the target microcontroller.

### ③ Selecting Binary File (SREC)

1. Click **[Browse...]** or double-click the empty file row in the center panel.
2. Load the target **`.srec` file** (e.g., `tron_pdm_detect.srec` or `base_firmware/tron_pdm_d2_test.srec`) from this `debug/` folder.

![File Loaded Screen](../../tron-npu/img/tron_debug3.png)

### ④ [Important] Erasing/Initializing Device before Flashing

When flashing large neural network binaries, write operations may fail with an address collision:
> `Error(E1000008): A device address error occurred. (Command: 13, Response: D2)`

To prevent this collision, **always initialize/erase the device configuration before starting the flash write**:

1. Click **`Target Device(D)`** in the top menu bar.
2. Select **`Initialize Device(I)...`** to trigger a physical sector reset.

![Device Initialization Selection](../../tron-npu/img/tron_debug4.png)

3. Wait until a green message "Device initialization succeeded" appears on the right log panel.

### ⑤ Executing Write Operations

1. Click the large **[Start]** button in the center.
2. The utility automatically executes Sector Erase ➡ Flash Write ➡ Verify operations.
3. Upon success, a green message **"Operation completed successfully"** appears.

![Flashing Success Screen](../../tron-npu/img/tron_debug5.png)

4. Press the physical black **`RESET`** button on the board (near the red slide switch) or cycle the USB power to restart the board with your new firmware.

---

## 5. Serial Console Logs Verification

You can monitor boot messages, kernel states, and AI inference latency in real-time via a USB serial terminal.

1. Launch your preferred terminal utility (**Tera Term**, **MobaXterm**, etc.).
2. Connect to the board's virtual COM port (J-Link CDC UART Port) and set the speed:

| Field | Value |
| :--- | :--- |
| **Speed (Baud rate)** | **`115200`** |
| **Data bits** | 8 bit |
| **Parity** | None |
| **Stop bits** | 1 bit |
| **Flow control** | None |

![Serial Port Settings](../../tron-npu/img/tron_debug8.png)

3. Press the physical black **`RESET`** button on the board.
4. You will see μT-Kernel 3.0 start up, followed by task registration logs and model execution benchmarks.

![Console Output Screen](../../tron-npu/img/tron_debug9.png)

---

## 6. Project Details & Verification Guides

Details and verification guidelines for each flashed SREC program:

### ① Voice Keyword Spotting on NPU (`tron_pdm_detect.srec`)
* **Operation & Demo**:
  * Real-time audio waveform graphs print to the LCD display.
  * Speak English keywords **`"up"`, `"down"`, `"left"`, `"right"`** clearly toward the board.
  * Upon detection, the recognized word and confidence rating (%) show up at the center of the display (ambient noises filter out as `"noise"`).
* **NPU Acceleration Details**:
  * MFCC audio feature extraction and classifier inference execute on the Ethos-U55 NPU in **~3 ms to 5 ms**.
  * By decoupling the DMA audio stream collection from the neural network task using μT-Kernel's task priority mechanisms, the audio stream remains smooth without packet dropouts.
* **CPU Comparison (`tron_pdm_detect_cpu.srec`)**:
  * Flashing the CPU-only version increases inference latencies significantly (to several dozen/hundred milliseconds), showing the drastic power efficiency benefits of offloading to the hardware NPU.

### ② Accelerometer Gesture Recognition AI (`tron_i2c_detect.srec`)
* **Operation & Demo**:
  * Hold the EK-RA8P1 board (with MPU-6050 wired) and repeat the following dynamic gestures in the air:
    * **`"wave"`**: Shake the board left-and-right rapidly (like waving goodbye).
    * **`"updown"`**: Shake the board up-and-down rapidly.
    * **`"snake"`**: Move the board in a twisting, wavy snake-like trajectory.
    * **`"idle"`**: Place the board flat on the table and let it rest.
  * The recognized motion class (e.g., `wave`) displays on the LCD screen along with real-time confidence scores.
* **Highlights**:
  * Uses a bit-banged software I2C driver to establish communications on any GPIO ports.
  * Embeds a timing compensation algorithm to maintain a clean 104 Hz sampling rate under RTOS task execution.

### ③ PDM Mic Waveform Plotter (`base_firmware/tron_pdm_d2_test.srec`)
* **Operation & Demo**:
  * Speak or whistle near the microphone to watch the audio amplitude waveform (1024 data points) and RMS volume plot onto the LCD screen.
* **Highlights**:
  * Decouples DMA completion events to feed UI rendering loops, guaranteeing tear-free display updates.

### ④ I2C Sensor Reading & 2D Graphics (`base_firmware/tron_i2c_d2_test.srec`)
* **Operation & Demo**:
  * Tilt and shake the accelerometer to watch live G-force wave plots (307 data points) for X (yellow), Y (red), and Z (blue) axes scroll across the LCD.
* **Highlights**:
  * Blends a strict 104 Hz sampling task with optimized D/AVE 2D vector drawing commands.

### ⑤ FOMO Component Detection on NPU (`base_firmware/tron_edge_fomo_npu_type.srec`)
* **Operation & Demo**:
  * Point the camera at PCBs containing tiny components (Pico/Xiao boards).
  * The application identifies components and overlays colored label boxes and item counts onto the video stream.
* **Demo Video**:
  * [YouTube Link (https://youtu.be/_uKRamoLaNA)](https://youtu.be/_uKRamoLaNA)
