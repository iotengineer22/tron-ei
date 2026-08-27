// 修正ポイント1：C言語で書かれたOSやライブラリのヘッダファイルを extern "C" で囲む
extern "C" {
#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include "dave_driver.h"
#include "../ra/fsp/src/r_drw/r_drw_base.h" // D1ドライバの型定義をインクルード
extern d2_device * d2_handle;
void R_BSP_SdramInit(bool init_memory); // SDRAM初期化関数のプロトタイプ宣言
extern UW knl_exctbl[];
}

#include "hal_data.h"
#include "font5x7.h"

// D/AVE 2D text render function
static void draw_char_5x7(d2_device *handle, int x, int y, int scaling, int c, d2_color color)
{
    if (c < 32 || c > 126) return;
    const unsigned char *glyph = font5x7[c - 32];
    
    // Draw character background box first (Dark charcoal: 0xFF101015)
    d2_setcolor(handle, 0, 0xFF101015);
    d2_renderbox(handle,
                 (d2_point)(x << 4),
                 (d2_point)(y << 4),
                 (d2_width)((6 * scaling) << 4),
                 (d2_width)((8 * scaling) << 4)); // 7 pixels high + 1 line spacing
    
    // Draw character foreground pixels
    d2_setcolor(handle, 0, color);
    
    for (int col = 0; col < 5; col++)
    {
        unsigned char line = glyph[col];
        for (int row = 0; row < 7; row++)
        {
            if (line & (1 << row))
            {
                // Draw a scaled pixel as a box using D2D
                d2_renderbox(handle, 
                             (d2_point)((x + col * scaling) << 4), 
                             (d2_point)((y + row * scaling) << 4), 
                             (d2_width)(scaling << 4), 
                             (d2_width)(scaling << 4));
            }
        }
    }
}

static void print_string_5x7(d2_device *handle, int x, int y, int scaling, const char *str, d2_color color)
{
    int cur_x = x;
    while (*str)
    {
        draw_char_5x7(handle, cur_x, y, scaling, *str, color);
        cur_x += 6 * scaling; // 5 pixels width + 1 pixel space
        str++;
    }
}

static void draw_char_5x7_cpu_scale2(uint16_t *fb, int pitch, int x, int y, int c, uint16_t color_565, uint16_t bg_565)
{
    if (c < 32 || c > 126) return;
    const unsigned char *glyph = font5x7[c - 32];

    for (int col = 0; col < 6; col++)
    {
        unsigned char line = (col < 5) ? glyph[col] : 0;
        int px0 = x + col * 2;
        int px1 = px0 + 1;
        
        if (px0 < 0 || px0 >= DISPLAY_HSIZE_INPUT0) continue;

        for (int row = 0; row < 8; row++)
        {
            bool pixel_on = (row < 7) && (line & (1 << row));
            uint16_t val = pixel_on ? color_565 : bg_565;
            
            int py0 = y + row * 2;
            int py1 = py0 + 1;
            
            if (py0 >= 0 && py0 < DISPLAY_VSIZE_INPUT0)
            {
                fb[py0 * pitch + px0] = val;
                if (px1 < DISPLAY_HSIZE_INPUT0) fb[py0 * pitch + px1] = val;
            }
            if (py1 >= 0 && py1 < DISPLAY_VSIZE_INPUT0)
            {
                fb[py1 * pitch + px0] = val;
                if (px1 < DISPLAY_HSIZE_INPUT0) fb[py1 * pitch + px1] = val;
            }
        }
    }
}

static void print_string_5x7_cpu(uint16_t *fb, int pitch, int x, int y, int scaling, const char *str, uint16_t color_565, uint16_t bg_565)
{
    int cur_x = x;
    while (*str)
    {
        if (scaling == 2)
        {
            draw_char_5x7_cpu_scale2(fb, pitch, cur_x, y, *str, color_565, bg_565);
        }
        else
        {
            for (int col = 0; col < 6; col++)
            {
                unsigned char line = (col < 5) ? font5x7[*str - 32][col] : 0;
                for (int col_s = 0; col_s < scaling; col_s++)
                {
                    int px = cur_x + col * scaling + col_s;
                    if (px < 0 || px >= DISPLAY_HSIZE_INPUT0) continue;
                    for (int row = 0; row < 8; row++)
                    {
                        bool pixel_on = (row < 7) && (line & (1 << row));
                        uint16_t val = pixel_on ? color_565 : bg_565;
                        for (int row_s = 0; row_s < scaling; row_s++)
                        {
                            int py = y + row * scaling + row_s;
                            if (py >= 0 && py < DISPLAY_VSIZE_INPUT0)
                            {
                                fb[py * pitch + px] = val;
                            }
                        }
                    }
                }
            }
        }
        cur_x += 6 * scaling;
        str++;
    }
}

#include <math.h>
#include <cstdio>
#include <cstring>

// Integer square root helper
static uint32_t integer_sqrt(uint32_t val)
{
    uint32_t temp, g = 0;
    uint32_t b = 0x80000000;
    while (b > 0)
    {
        temp = g + b;
        if (val >= temp)
        {
            val -= temp;
            g = temp + b;
        }
        g >>= 1;
        b >>= 2;
    }
    return g;
}

static bsp_io_port_pin_t g_scl_pin = BSP_IO_PORT_05_PIN_12;
static bsp_io_port_pin_t g_sda_pin = BSP_IO_PORT_05_PIN_11;
#define SCL_PIN g_scl_pin
#define SDA_PIN g_sda_pin

#define HISTORY_LEN (307)

// Waveform plot layout
#define VISIBLE_WIDTH (1024)
#define VISIBLE_HEIGHT (600)
#define PLOT_X_START (51)
#define PLOT_WIDTH (922)
#define PLOT_Y_ZERO (330)
#define PLOT_STEP_X (3)

static int16_t g_accel_x_history[HISTORY_LEN] = {0};
static int16_t g_accel_y_history[HISTORY_LEN] = {0};
static int16_t g_accel_z_history[HISTORY_LEN] = {0};

static volatile int16_t g_accel_x = 0;
static volatile int16_t g_accel_y = 0;
static volatile int16_t g_accel_z = 0;
static volatile uint32_t g_accel_mag = 0;
static volatile uint32_t g_i2c_event_count = 0;
static volatile bool g_mpu_detected = false;

static void i2c_delay(void)
{
    R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MICROSECONDS);
}

static void sda_high(void)
{
    R_IOPORT_PinCfg(&g_ioport_ctrl, SDA_PIN, IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);
}

static void sda_low(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, SDA_PIN, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinCfg(&g_ioport_ctrl, SDA_PIN, IOPORT_CFG_PORT_DIRECTION_OUTPUT);
}

static void scl_high(void)
{
    R_IOPORT_PinCfg(&g_ioport_ctrl, SCL_PIN, IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);
}

static void scl_low(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, SCL_PIN, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinCfg(&g_ioport_ctrl, SCL_PIN, IOPORT_CFG_PORT_DIRECTION_OUTPUT);
}

static bool read_sda(void)
{
    bsp_io_level_t level = BSP_IO_LEVEL_LOW;
    R_IOPORT_PinRead(&g_ioport_ctrl, SDA_PIN, &level);
    return (level == BSP_IO_LEVEL_HIGH);
}

static void i2c_start(void)
{
    sda_high();
    scl_high();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

static void i2c_stop(void)
{
    sda_low();
    scl_high();
    i2c_delay();
    sda_high();
    i2c_delay();
}

static bool i2c_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        if (byte & 0x80) sda_high();
        else sda_low();
        i2c_delay();
        scl_high();
        i2c_delay();
        scl_low();
        byte <<= 1;
    }
    
    // Read ACK
    sda_high();
    i2c_delay();
    scl_high();
    i2c_delay();
    bool ack = !read_sda();
    scl_low();
    i2c_delay();
    return ack;
}

static uint8_t i2c_read_byte(bool ack)
{
    uint8_t byte = 0;
    sda_high();
    for (int i = 0; i < 8; i++)
    {
        byte <<= 1;
        scl_high();
        i2c_delay();
        if (read_sda()) byte |= 1;
        scl_low();
        i2c_delay();
    }
    
    // Write ACK/NACK
    if (ack) sda_low();
    else sda_high();
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    i2c_delay();
    
    return byte;
}

static bool mpu6050_write_reg(uint8_t reg, uint8_t val)
{
    i2c_start();
    if (!i2c_write_byte(0x68 << 1)) { i2c_stop(); return false; }
    if (!i2c_write_byte(reg)) { i2c_stop(); return false; }
    if (!i2c_write_byte(val)) { i2c_stop(); return false; }
    i2c_stop();
    return true;
}

static bool mpu6050_read_regs(uint8_t reg, uint8_t *buf, uint32_t len)
{
    i2c_start();
    if (!i2c_write_byte(0x68 << 1)) { i2c_stop(); return false; }
    if (!i2c_write_byte(reg)) { i2c_stop(); return false; }
    
    i2c_start(); // Repeated start
    if (!i2c_write_byte((0x68 << 1) | 1)) { i2c_stop(); return false; }
    
    for (uint32_t i = 0; i < len; i++)
    {
        buf[i] = i2c_read_byte(i < (len - 1));
    }
    i2c_stop();
    return true;
}

struct pin_pair {
    bsp_io_port_pin_t scl;
    bsp_io_port_pin_t sda;
    const char* name;
};

static const pin_pair g_candidate_pins[] = {
    { BSP_IO_PORT_05_PIN_12, BSP_IO_PORT_05_PIN_11, "Arduino Header J24 (P512/P511)" },
    { BSP_IO_PORT_04_PIN_00, BSP_IO_PORT_04_PIN_01, "Arduino Header J24 (P400/P401)" },
    { BSP_IO_PORT_06_PIN_07, BSP_IO_PORT_06_PIN_08, "PMOD2 J25 (P607/P608)" },
    { BSP_IO_PORT_13_PIN_03, BSP_IO_PORT_13_PIN_02, "PMOD1 J26 (P1303/P1302)" }
};

static bool mpu6050_init(void)
{
    for (size_t i = 0; i < sizeof(g_candidate_pins) / sizeof(g_candidate_pins[0]); i++)
    {
        g_scl_pin = g_candidate_pins[i].scl;
        g_sda_pin = g_candidate_pins[i].sda;
        
        tm_printf((UB *)"Trying MPU-6050 on %s...\n", g_candidate_pins[i].name);
        
        // Configure SCL and SDA pins as input with pull-up enabled
        R_IOPORT_PinCfg(&g_ioport_ctrl, SCL_PIN, IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);
        R_IOPORT_PinCfg(&g_ioport_ctrl, SDA_PIN, IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);
        tk_dly_tsk(10);
        
        // Read WHO_AM_I register (0x75)
        uint8_t who_am_i = 0;
        if (mpu6050_read_regs(0x75, &who_am_i, 1))
        {
            tm_printf((UB *)"MPU6050 WHO_AM_I read: 0x%02X\n", who_am_i);
            if (who_am_i == 0x68 || who_am_i == 0x70 || who_am_i == 0x71 || who_am_i == 0x73)
            {
                tm_printf((UB *)"SUCCESS: MPU-6050 detected on %s!\n", g_candidate_pins[i].name);
                
                // Wake up MPU-6050 (write 0x00 to PWR_MGMT_1 register 0x6B)
                if (mpu6050_write_reg(0x6B, 0x00))
                {
                    tk_dly_tsk(10);
                    // Set accelerometer range to +/- 2g (write 0x00 to ACCEL_CONFIG register 0x1C)
                    if (mpu6050_write_reg(0x1C, 0x00))
                    {
                        return true;
                    }
                }
            }
        }
        else
        {
            tm_printf((UB *)"MPU6050 read failed on this port.\n");
        }
    }
    return false;
}

static bool mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    if (!mpu6050_read_regs(0x3B, buf, 6)) return false;
    
    *ax = (int16_t)((buf[0] << 8) | buf[1]);
    *ay = (int16_t)((buf[2] << 8) | buf[3]);
    *az = (int16_t)((buf[4] << 8) | buf[5]);
    return true;
}

// Third SDRAM framebuffer
uint8_t fb_background_2[DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");


// Vblank flag and counter
static volatile bool vblank_flag = false;
static volatile uint32_t vblank_interrupt_count = 0;

LOCAL ID tskid_1;                          // Task ID number
LOCAL ID tskid_2;                          // Task ID number

// LCD GLCDC callback
extern "C" void lcd_glcdc_callback(display_callback_args_t * p_args)
{
    if (p_args->event == DISPLAY_EVENT_LINE_DETECTION)
    {
        vblank_flag = true;
        vblank_interrupt_count++;
        tk_wup_tsk(tskid_1);
    }
}

// Edge Impulse SDK & MERA model inclusion
#include "edge-impulse-sdk/classifier/ei_run_dsp.h"
#include <stdarg.h>
#include <cstdlib>

extern "C" {
#include "npu_model/model.h"
#include "npu_model/model_io_data.h"
}


// Define spectral analysis configuration directly to avoid including model_variables.h (which depends on EON/TFLM)
ei_dsp_config_spectral_analysis_t ei_dsp_config_781279_47 = {
    47, // uint32_t blockId
    4, // int implementationVersion
    3, // int length of axes
    1.0f, // float scale-axes
    1, // int input-decimation-ratio
    (char*)"none", // select filter-type
    3.0f, // float filter-cutoff
    6, // int filter-order
    (char*)"FFT", // select analysis-type
    16, // int fft-length
    3, // int spectral-peaks-count
    0.1f, // float spectral-peaks-threshold
    (char*)"0.1, 0.5, 1.0, 2.0, 5.0", // string spectral-power-edges
    true, // boolean do-log
    true, // boolean do-fft-overlap
    1, // int wavelet-level
    (char*)"db4", // select wavelet
    false // boolean extra-low-freq
};


// NPU configuration
extern "C" {
#include "ethosu_driver.h"
#include "rm_ethosu.h"
#include "rm_ethosu_api.h"

struct ethosu_driver g_ethosu0;
rm_ethosu_extended_cfg_t g_rm_ethosu0_ext_cfg = {
    .p_dev = &g_ethosu0,
};
rm_ethosu_instance_ctrl_t g_rm_ethosu0_ctrl = {
    .p_ext_cfg = &g_rm_ethosu0_ext_cfg,
};
const rm_ethosu_cfg_t g_rm_ethosu0_cfg = {
    .p_callback      = NULL,
    .p_context       = NULL,
    .ipl             = (12),
    .irq             = ((IRQn_Type) 20), // Vector 20
    .secure_enable   = 1,
    .privilege_enable = 1,
};
const rm_ethosu_instance_t g_rm_ethosu0 = {
    .p_ctrl = &g_rm_ethosu0_ctrl,
    .p_cfg = &g_rm_ethosu0_cfg,
    .p_api = &g_rm_ethosu_on_npu,
};
void rm_ethosu_isr(void);
}


// sliding window variables
#define RAW_SAMPLE_COUNT 125
#define RAW_SAMPLES_PER_FRAME 3
#define TOTAL_RAW_SAMPLES (RAW_SAMPLE_COUNT * RAW_SAMPLES_PER_FRAME)

static float g_raw_accel_buffer[TOTAL_RAW_SAMPLES] = {0};
static int g_raw_sample_count = 0;

static int get_sensor_data(size_t offset, size_t length, float *out_ptr)
{
    memcpy(out_ptr, &g_raw_accel_buffer[offset], length * sizeof(float));
    return 0;
}

static char g_detected_gesture[32] = "idle";
static float g_gesture_probs[4] = {0};

static void run_npu_inference(void)
{
    signal_t signal;
    signal.total_length = TOTAL_RAW_SAMPLES;
    signal.get_data = &get_sensor_data;

    float features_out[39];
    matrix_t features_matrix(1, 39, features_out);

    // Call EI DSP Spectral Analysis
    int dsp_res = extract_spectral_analysis_features(
        &signal,
        &features_matrix,
        &ei_dsp_config_781279_47,
        62.5f // frequency
    );

    if (dsp_res != 0)
    {
        return;
    }

    // Quantize inputs to model: scale=0.65004575, zero_point=-110
    int8_t *input_ptr = GetModelInputPtr_serving_default_x_0();
    for (int i = 0; i < 39; i++)
    {
        float val = features_out[i];
        int32_t val_quant = (int32_t)roundf(val / 0.65004575f) - 110;
        if (val_quant < -128) val_quant = -128;
        if (val_quant > 127)  val_quant = 127;
        input_ptr[i] = (int8_t)val_quant;
    }

    // Run standalone NPU model compiled by MERA
    RunModel(false);

    // Dequantize outputs: scale=0.00390625, zero_point=-128
    int8_t *output_ptr = GetModelOutputPtr_StatefulPartitionedCall_0_70009();
    float max_prob = -1.0f;
    int max_idx = 2; // Default to "idle"

    // Classes sorted alphabetically
    const char *class_names[4] = { "circle", "flick", "idle", "updown" };

    for (int i = 0; i < 4; i++)
    {
        float prob = (float)(output_ptr[i] + 128) / 256.0f;
        g_gesture_probs[i] = prob;
        if (prob > max_prob)
        {
            max_prob = prob;
            max_idx = i;
        }
    }

    static int hold_frames = 0;
    if (hold_frames > 0)
    {
        hold_frames--;
        // If the model continues to predict the same non-idle gesture, refresh the hold timer
        if (max_idx != 2 && strcmp(g_detected_gesture, class_names[max_idx]) == 0)
        {
            hold_frames = 20;
        }
        // If the model predicts a DIFFERENT non-idle gesture with high confidence, switch immediately
        else if (max_idx != 2 && max_prob >= 0.80f)
        {
            strcpy(g_detected_gesture, class_names[max_idx]);
            hold_frames = 20;
        }
    }
    else
    {
        // Transition to a non-idle gesture only if confidence is high (>= 70%)
        if (max_idx != 2 && max_prob >= 0.70f)
        {
            strcpy(g_detected_gesture, class_names[max_idx]);
            hold_frames = 20;
        }
        else if (max_idx == 2)
        {
            strcpy(g_detected_gesture, "idle");
        }
    }
}

LOCAL void task_1(INT stacd, void *exinf); // task execution function

LOCAL T_CTSK ctsk_1 = {
    .exinf   = NULL,               // 1. 拡張情報
    .tskatr  = TA_HLNG | TA_RNG3,  // 2. タスク属性
    .task    = (FP)task_1,         // 3. タスク関数
    .itskpri = 10,                 // 4. 優先度
    .stksz   = 32768,              // 5. スタックサイズ
    .bufptr  = NULL                // 6. スタックバッファポインタ
};

LOCAL void task_2(INT stacd, void *exinf); // task execution function

LOCAL T_CTSK ctsk_2 = {
    .exinf   = NULL,
    .tskatr  = TA_HLNG | TA_RNG3,
    .task    = (FP)task_2,
    .itskpri = 9,                  // 優先度9
    .stksz   = 32768,              // スタックサイズを32KBに拡張してNPU & DSPスタック安全性を確保
    .bufptr  = NULL
};

LOCAL void task_1(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    tk_dly_tsk(1000);

    R_IOPORT_PinsCfg(&g_ioport_ctrl, &g_bsp_pin_cfg);

    R_BSP_SdramInit(true);

    SCB_CleanInvalidateDCache();

    tm_printf((UB *)"\n=== MPU-6050 Accelerometer Test Start ===\n");

    tm_printf((UB *)"=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===\n");
    for (int i = 0; i < 18; i++)
    {
        R_BUS->BUSS[i].CNT = 0x0013;
    }

    tm_printf((UB *)"Clearing SDRAM framebuffers to black...\n");
    memset(fb_background, 0, sizeof(fb_background));
    memset(fb_background_2, 0, sizeof(fb_background_2));
    
    SCB_CleanInvalidateDCache();

    tm_printf((UB *)"Initializing LCD (GLCDC)... \n");

    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_08, BSP_IO_LEVEL_LOW);
    
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_RESET, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(500);
 
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_RESET, BSP_IO_LEVEL_LOW);
    tk_dly_tsk(200);
 
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_RESET, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(500);
 
    fsp_err_t err = R_GLCDC_Open(&g_lcd_glcdc_ctrl, &g_lcd_glcdc_cfg);
    if (FSP_SUCCESS != err)
    {
        tm_printf((UB *)"ERROR: Failed to open GLCDC (0x%x)\n", err);
        while(1) { tk_dly_tsk(1000); }
    }
    
    err = R_GLCDC_Start(&g_lcd_glcdc_ctrl);
    if (FSP_SUCCESS != err)
    {
        tm_printf((UB *)"ERROR: Failed to start GLCDC (0x%x)\n", err);
        while(1) { tk_dly_tsk(1000); }
    }
    
    uint8_t *const my_fb[3] = {
        (uint8_t *)&fb_background[0][0],
        (uint8_t *)&fb_background[1][0],
        (uint8_t *)&fb_background_2[0]
    };
    R_GLCDC_BufferChange(&g_lcd_glcdc_ctrl, my_fb[0], DISPLAY_FRAME_LAYER_1);
 
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_BLEN, BSP_IO_LEVEL_HIGH);
    tm_printf((UB *)"LCD Backlight enabled.\n");
 
    tm_printf((UB *)"Initializing D/AVE 2D Graphics Engine...\n");
    d2_handle = d2_opendevice(0);
    if (NULL == d2_handle)
    {
        tm_printf((UB *)"ERROR: Failed to open D/AVE 2D engine\n");
        while(1) { tk_dly_tsk(1000); }
    }
    d2_inithw(d2_handle, 0);
 
    d2_setblendmode(d2_handle, d2_bm_one, d2_bm_zero);
    d2_setalphamode(d2_handle, d2_am_constant);
    d2_setalpha(d2_handle, 0xff);
    d2_setantialiasing(d2_handle, 0);

    tm_printf((UB *)"Pre-rendering background and static borders to all 3 framebuffers...\n");
    for (int i = 0; i < 3; i++)
    {
        d2_startframe(d2_handle);
        d2_framebuffer(d2_handle, my_fb[i], DISPLAY_HSIZE_INPUT0, DISPLAY_HSIZE_INPUT0, DISPLAY_VSIZE_INPUT0, d2_mode_rgb565);
        d2_clear(d2_handle, 0xFF101015);
        
        // Neon cyan outer borders
        d2_setcolor(d2_handle, 0, 0xFF00E5FF);
        d2_renderline(d2_handle, 20 << 4, 30 << 4, (VISIBLE_WIDTH - 20) << 4, 30 << 4, 2 << 4, 0); // Top
        d2_renderline(d2_handle, 20 << 4, (VISIBLE_HEIGHT - 30) << 4, (VISIBLE_WIDTH - 20) << 4, (VISIBLE_HEIGHT - 30) << 4, 2 << 4, 0); // Bottom
        d2_renderline(d2_handle, 20 << 4, 30 << 4, 20 << 4, (VISIBLE_HEIGHT - 30) << 4, 2 << 4, 0); // Left
        d2_renderline(d2_handle, (VISIBLE_WIDTH - 20) << 4, 30 << 4, (VISIBLE_WIDTH - 20) << 4, (VISIBLE_HEIGHT - 30) << 4, 2 << 4, 0); // Right
        
        // Title
        print_string_5x7(d2_handle, 50, 50, 3, "EK-RA8P1 NPU ACCELEROMETER GESTURES", 0xFF00E5FF);
        
        // Waveform plot borders
        d2_setcolor(d2_handle, 0, 0xFF3A3A4A);
        d2_renderbox(d2_handle, (PLOT_X_START - 1) << 4, 180 << 4, (PLOT_WIDTH + 2) << 4, 300 << 4);
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 181 << 4, PLOT_WIDTH << 4, 298 << 4);
        
        // Magnitude bar borders
        d2_setcolor(d2_handle, 0, 0xFF3A3A4A);
        d2_renderbox(d2_handle, (PLOT_X_START - 1) << 4, 510 << 4, (PLOT_WIDTH + 2) << 4, 30 << 4);
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 511 << 4, PLOT_WIDTH << 4, 28 << 4);
        
        d2_endframe(d2_handle);
        SCB_CleanInvalidateDCache();
        d2_flushframe(d2_handle);
        SCB_CleanInvalidateDCache();
    }

    // Register the NPU ISR to microT-Kernel's relocated RAM vector table directly
    knl_exctbl[16 + 20] = (UW)rm_ethosu_isr;
    
    R_ICU->IELSR[20] = (uint32_t) ELC_EVENT_NPU_IRQ;

    fsp_err_t npu_err = RM_ETHOSU_Open(&g_rm_ethosu0_ctrl, &g_rm_ethosu0_cfg);
    if (FSP_SUCCESS != npu_err)
    {
        tm_printf((UB *)"ERROR: Failed to open Ethos-U55 NPU (0x%x)\n", npu_err);
    }
    else
    {
        tm_printf((UB *)"Ethos-U55 NPU initialized successfully.\n");
    }


    tm_printf((UB *)"Initializing MPU-6050 with auto-detection...\n");
    g_mpu_detected = mpu6050_init();
    if (g_mpu_detected)
    {
        tm_printf((UB *)"MPU-6050 initialized successfully.\n");
    }
    else
    {
        tm_printf((UB *)"ERROR: Failed to initialize MPU-6050\n");
    }

    tm_printf((UB *)"Starting D/AVE 2D Rendering Loop (Accelerometer Visualizer)...\n");

    uint8_t draw_buf = 0;
    uint8_t pending_buf = 1;
    uint8_t display_buf = 2;
    uint32_t loop_cnt = 0;

    int16_t local_x_hist[HISTORY_LEN] = {0};
    int16_t local_y_hist[HISTORY_LEN] = {0};
    int16_t local_z_hist[HISTORY_LEN] = {0};

    while (1)
    {
        // Wait for Vblank (LINE DETECTION) interrupt
        while (!vblank_flag)
        {
            tk_slp_tsk(100);
        }
        vblank_flag = false;

        d2_startframe(d2_handle);
        d2_framebuffer(d2_handle, my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, DISPLAY_HSIZE_INPUT0, DISPLAY_VSIZE_INPUT0, d2_mode_rgb565);

        // Clear waveform area
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 181 << 4, PLOT_WIDTH << 4, 298 << 4);

        // Copy history arrays safely under scheduler lock
        tk_dis_dsp();
        memcpy(local_x_hist, g_accel_x_history, sizeof(g_accel_x_history));
        memcpy(local_y_hist, g_accel_y_history, sizeof(g_accel_y_history));
        memcpy(local_z_hist, g_accel_z_history, sizeof(g_accel_z_history));
        tk_ena_dsp();

        // Draw X axis (Red: 0xFFFF0000)
        d2_setcolor(d2_handle, 0, 0xFFFF0000);
        int32_t init_offset_x = ((int32_t)local_x_hist[0] * 80) / 16384;
        int prev_x_x = PLOT_X_START;
        int prev_y_x = PLOT_Y_ZERO - init_offset_x;
        if (prev_y_x < 181) prev_y_x = 181;
        if (prev_y_x > 479) prev_y_x = 479;
        
        for (int i = 0; i < HISTORY_LEN; i++)
        {
            int32_t y_offset = ((int32_t)local_x_hist[i] * 80) / 16384;
            int y_coord = PLOT_Y_ZERO - y_offset;
            int x_coord = PLOT_X_START + i * PLOT_STEP_X;
            if (y_coord < 181) y_coord = 181;
            if (y_coord > 479) y_coord = 479;
            if (i > 0)
            {
                d2_renderline(d2_handle, prev_x_x << 4, prev_y_x << 4, x_coord << 4, y_coord << 4, 2 << 4, 0);
            }
            prev_x_x = x_coord;
            prev_y_x = y_coord;
        }

        // Draw Y axis (Green: 0xFF00FF00)
        d2_setcolor(d2_handle, 0, 0xFF00FF00);
        int32_t init_offset_y = ((int32_t)local_y_hist[0] * 80) / 16384;
        int prev_x_y = PLOT_X_START;
        int prev_y_y = PLOT_Y_ZERO - init_offset_y;
        if (prev_y_y < 181) prev_y_y = 181;
        if (prev_y_y > 479) prev_y_y = 479;
        
        for (int i = 0; i < HISTORY_LEN; i++)
        {
            int32_t y_offset = ((int32_t)local_y_hist[i] * 80) / 16384;
            int y_coord = PLOT_Y_ZERO - y_offset;
            int x_coord = PLOT_X_START + i * PLOT_STEP_X;
            if (y_coord < 181) y_coord = 181;
            if (y_coord > 479) y_coord = 479;
            if (i > 0)
            {
                d2_renderline(d2_handle, prev_x_y << 4, prev_y_y << 4, x_coord << 4, y_coord << 4, 2 << 4, 0);
            }
            prev_x_y = x_coord;
            prev_y_y = y_coord;
        }

        // Draw Z axis (Cyan: 0xFF00FFFF)
        d2_setcolor(d2_handle, 0, 0xFF00FFFF);
        int32_t init_offset_z = ((int32_t)local_z_hist[0] * 80) / 16384;
        int prev_x_z = PLOT_X_START;
        int prev_y_z = PLOT_Y_ZERO - init_offset_z;
        if (prev_y_z < 181) prev_y_z = 181;
        if (prev_y_z > 479) prev_y_z = 479;
        
        for (int i = 0; i < HISTORY_LEN; i++)
        {
            int32_t y_offset = ((int32_t)local_z_hist[i] * 80) / 16384;
            int y_coord = PLOT_Y_ZERO - y_offset;
            int x_coord = PLOT_X_START + i * PLOT_STEP_X;
            if (y_coord < 181) y_coord = 181;
            if (y_coord > 479) y_coord = 479;
            if (i > 0)
            {
                d2_renderline(d2_handle, prev_x_z << 4, prev_y_z << 4, x_coord << 4, y_coord << 4, 2 << 4, 0);
            }
            prev_x_z = x_coord;
            prev_y_z = y_coord;
        }

        // Clear magnitude area
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 511 << 4, PLOT_WIDTH << 4, 28 << 4);

        // Draw magnitude bar
        int32_t vol_width = ((int32_t)g_accel_mag * PLOT_WIDTH) / 40960; 
        if (vol_width > PLOT_WIDTH) vol_width = PLOT_WIDTH;
        if (vol_width > 0)
        {
            d2_setcolor(d2_handle, 0, 0xFF00E5FF);
            d2_renderbox(d2_handle, PLOT_X_START << 4, 511 << 4, vol_width << 4, 28 << 4);
        }

        // Text formatting and CPU draw to prevent flickering
        char line1_buf[64];
        char line2_buf[64];
        char line3_buf[64];
        char line4_buf[128];

        int32_t ax_mg = ((int32_t)g_accel_x * 1000) / 16384;
        int32_t ay_mg = ((int32_t)g_accel_y * 1000) / 16384;
        int32_t az_mg = ((int32_t)g_accel_z * 1000) / 16384;
        int32_t mag_mg = ((int32_t)g_accel_mag * 1000) / 16384;

        int32_t ax_g_abs = ax_mg < 0 ? -ax_mg : ax_mg;
        int32_t ay_g_abs = ay_mg < 0 ? -ay_mg : ay_mg;
        int32_t az_g_abs = az_mg < 0 ? -az_mg : az_mg;
        int32_t mag_g_abs = mag_mg < 0 ? -mag_mg : mag_mg;

        sprintf(line1_buf, "X:%c%d.%03dg Y:%c%d.%03dg Z:%c%d.%03dg",
                ax_mg < 0 ? '-' : ' ', (int)(ax_g_abs / 1000), (int)(ax_g_abs % 1000),
                ay_mg < 0 ? '-' : ' ', (int)(ay_g_abs / 1000), (int)(ay_g_abs % 1000),
                az_mg < 0 ? '-' : ' ', (int)(az_g_abs / 1000), (int)(az_g_abs % 1000));
        
        sprintf(line2_buf, "MAG:%d.%03dg  SMP:%d",
                (int)(mag_g_abs / 1000), (int)(mag_g_abs % 1000),
                (int)g_i2c_event_count);

        // Pad gesture to 8 characters using %-8s
        sprintf(line3_buf, "GESTURE: %-8s", g_detected_gesture);
        
        // Show probabilities as percentage (e.g. circle: 0%, flick: 0%, ...)
        int32_t c_pct = (int32_t)(g_gesture_probs[0] * 100.0f);
        int32_t f_pct = (int32_t)(g_gesture_probs[1] * 100.0f);
        int32_t i_pct = (int32_t)(g_gesture_probs[2] * 100.0f);
        int32_t u_pct = (int32_t)(g_gesture_probs[3] * 100.0f);
        sprintf(line4_buf, "circle:%d%% flick:%d%% idle:%d%% updown:%d%%",
                (int)c_pct, (int)f_pct, (int)i_pct, (int)u_pct);

        // Draw CPU text lines
        // Left column: accel stats and percentages with increased vertical spacing (25px)
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 50, 90, 2, line1_buf, 0xFFFF, 0x1082);
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 50, 115, 2, line2_buf, 0xFFFF, 0x1082);
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 50, 140, 2, line4_buf, 0x07FF, 0x1082); // Cyan for probs
        
        // Right column: display the detected gesture much larger (scaling=4) at x=600, y=105
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 600, 105, 4, line3_buf, 0xFFE0, 0x1082); // Yellow for gesture

        d2_endframe(d2_handle);

        // Cache clean the text region: Y=85 to Y=170 (height 85 lines)
        SCB_CleanInvalidateDCache_by_Addr((void *)((uint16_t *)my_fb[draw_buf] + 85 * DISPLAY_HSIZE_INPUT0), DISPLAY_HSIZE_INPUT0 * 85 * sizeof(uint16_t));

        d2_flushframe(d2_handle);

        R_GLCDC_BufferChange(&g_lcd_glcdc_ctrl, my_fb[draw_buf], DISPLAY_FRAME_LAYER_1);

        uint8_t next_draw = display_buf;
        uint8_t next_pending = draw_buf;
        uint8_t next_display = pending_buf;

        draw_buf = next_draw;
        pending_buf = next_pending;
        display_buf = next_display;

        loop_cnt++;
    }
}

// task_2: Accel data collection (62.5Hz) & NPU Inference
LOCAL void task_2(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    tk_dly_tsk(3000);

    tm_printf((UB *)"task_2 (62.5Hz Accelerometer Acquisition & Gesture Inference) started.\n");

    int classification_stride = 0;

    while (1)
    {
        int16_t ax = 0, ay = 0, az = 0;
        if (g_mpu_detected)
        {
            if (mpu6050_read_accel(&ax, &ay, &az))
            {
                g_accel_x = ax;
                g_accel_y = ay;
                g_accel_z = az;
                
                uint64_t mag_sq = (int64_t)ax*ax + (int64_t)ay*ay + (int64_t)az*az;
                g_accel_mag = integer_sqrt(mag_sq);
                
                g_i2c_event_count++;
                
                // Convert to Gs
                float ax_g = (float)ax / 16384.0f;
                float ay_g = (float)ay / 16384.0f;
                float az_g = (float)az / 16384.0f;

                // Log raw data at 62.5Hz (convert to mg, e.g. 1.0G -> 1000mg)
                int32_t ax_mg = (int32_t)(ax_g * 1000.0f);
                int32_t ay_mg = (int32_t)(ay_g * 1000.0f);
                int32_t az_mg = (int32_t)(az_g * 1000.0f);
                tm_printf((UB *)"62.5Hz:%d,%d,%d\n", ax_mg, ay_mg, az_mg);

                // Update sliding window buffer and waveform histories under lock
                tk_dis_dsp();
                
                // EI DSP buffer update (shift left by 3 elements)
                memmove(&g_raw_accel_buffer[0], &g_raw_accel_buffer[3], (TOTAL_RAW_SAMPLES - 3) * sizeof(float));
                g_raw_accel_buffer[TOTAL_RAW_SAMPLES - 3] = ax_g;
                g_raw_accel_buffer[TOTAL_RAW_SAMPLES - 2] = ay_g;
                g_raw_accel_buffer[TOTAL_RAW_SAMPLES - 1] = az_g;

                if (g_raw_sample_count < RAW_SAMPLE_COUNT)
                {
                    g_raw_sample_count++;
                }

                // Plot waveform history update (shift left by 1 element)
                memmove((void*)&g_accel_x_history[0], (const void*)&g_accel_x_history[1], (HISTORY_LEN - 1) * sizeof(int16_t));
                memmove((void*)&g_accel_y_history[0], (const void*)&g_accel_y_history[1], (HISTORY_LEN - 1) * sizeof(int16_t));
                memmove((void*)&g_accel_z_history[0], (const void*)&g_accel_z_history[1], (HISTORY_LEN - 1) * sizeof(int16_t));
                g_accel_x_history[HISTORY_LEN - 1] = ax;
                g_accel_y_history[HISTORY_LEN - 1] = ay;
                g_accel_z_history[HISTORY_LEN - 1] = az;
                
                tk_ena_dsp();

                // Run classification stride count
                if (g_raw_sample_count >= RAW_SAMPLE_COUNT)
                {
                    classification_stride++;
                    if (classification_stride >= 5) // run classification every 5 samples (80ms)
                    {
                        classification_stride = 0;
                        run_npu_inference();
                        
                        int32_t p0_pct = (int32_t)(g_gesture_probs[0] * 100.0f);
                        int32_t p1_pct = (int32_t)(g_gesture_probs[1] * 100.0f);
                        int32_t p2_pct = (int32_t)(g_gesture_probs[2] * 100.0f);
                        int32_t p3_pct = (int32_t)(g_gesture_probs[3] * 100.0f);
                        tm_printf((UB *)"Gesture: %s (circle: %d%%, flick: %d%%, idle: %d%%, updown: %d%%)\n",
                                  g_detected_gesture, p0_pct, p1_pct, p2_pct, p3_pct);
                    }
                }
            }
        }
        
        // Exact 16 ms delay (62.5 Hz sampling)
        tk_dly_tsk(16);
    }
}

extern "C" EXPORT INT usermain(void)
{
    tm_putstring((UB *)"Start User-main program (Camera & LCD D2D Test).\n");

    /* Create & Start Tasks with validation */
    tskid_1 = tk_cre_tsk(&ctsk_1);
    if (tskid_1 < 0) {
        tm_printf((UB *)"ERROR: Failed to create task_1 (%d)\n", tskid_1);
    } else {
        tm_printf((UB *)"SUCCESS: Created task_1 (ID: %d)\n", tskid_1);
        tk_sta_tsk(tskid_1, 0);
    }

    tskid_2 = tk_cre_tsk(&ctsk_2);
    if (tskid_2 < 0) {
        tm_printf((UB *)"ERROR: Failed to create task_2 (%d)\n", tskid_2);
    } else {
        tm_printf((UB *)"SUCCESS: Created task_2 (ID: %d)\n", tskid_2);
        tk_sta_tsk(tskid_2, 0);
    }

    tk_slp_tsk(TMO_FEVR);
    return 0;
}

// Edge Impulse SDK Porting functions implemented directly for microT-Kernel
__attribute__((weak)) EI_IMPULSE_ERROR ei_run_impulse_check_canceled() {
    return EI_IMPULSE_OK;
}
__attribute__((weak)) EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {
    tk_dly_tsk(time_ms);
    return EI_IMPULSE_OK;
}
__attribute__((weak)) uint64_t ei_read_timer_ms() {
    SYSTIM systim;
    tk_get_tim(&systim);
    return (uint64_t)systim.lo;
}
__attribute__((weak)) uint64_t ei_read_timer_us() {
    static uint32_t last_cyccnt = 0;
    static uint64_t accumulated_us = 0;

    // Enable DWT Cycle Counter if not enabled
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        last_cyccnt = DWT->CYCCNT;
    }

    uint32_t current_cyccnt = DWT->CYCCNT;
    uint32_t diff = current_cyccnt - last_cyccnt;
    last_cyccnt = current_cyccnt;

    uint32_t clock_mhz = SystemCoreClock ? (SystemCoreClock / 1000000UL) : 480UL;
    accumulated_us += (uint64_t)diff / clock_mhz;

    return accumulated_us;
}
__attribute__((weak)) void ei_putchar(char c) {
    (void)c;
}
__attribute__((weak)) char ei_getchar(void) {
    return 0;
}
__attribute__((weak)) void ei_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
__attribute__((weak)) void ei_printf_float(float f) {
    printf("%f", f);
}
__attribute__((weak)) void *ei_malloc(size_t size) {
    return malloc(size);
}
__attribute__((weak)) void *ei_calloc(size_t nitems, size_t size) {
    return calloc(nitems, size);
}
__attribute__((weak)) void ei_free(void *ptr) {
    free(ptr);
}

