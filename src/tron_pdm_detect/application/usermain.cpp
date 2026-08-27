#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

extern "C" {
#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include "dave_driver.h"
#include "../ra/fsp/src/r_drw/r_drw_base.h" // D1ドライバの型定義をインクルード
extern d2_device * d2_handle;
void R_BSP_SdramInit(bool init_memory); // SDRAM初期化関数のプロトタイプ宣言

// FSPの空のキャッシュ操作関数をオーバーライドして、データキャッシュの同期を実行する
d1_int_t d1_cacheflush (d1_device * handle, d1_int_t memtype)
{
    (void)handle;
    (void)memtype;
    SCB_CleanDCache();
    return 1;
}

d1_int_t d1_cacheblockflush (d1_device * handle, d1_int_t memtype, const void * ptr, d1_uint_t size)
{
    (void)handle;
    (void)memtype;
    SCB_CleanDCache_by_Addr((void *)ptr, (int32_t)size);
    return 1;
}
}

#include "hal_data.h"
#include "font5x7.h"

// Edge Impulse SDK & DSP Headers
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"

// Arm Ethos-U55 NPU Driver Headers & Configurations
#include "ethosu_driver.h"
#include "rm_ethosu.h"
#include "rm_ethosu_api.h"

extern "C" {
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
#if defined(VECTOR_NUMBER_NPU_IRQ)
    .irq             = VECTOR_NUMBER_NPU_IRQ,
#else
    .irq             = ((IRQn_Type) 20),
#endif
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

// 16kHz Audio Circular Buffer for Edge Impulse Voice Detection (1.0 second window)
#define AUDIO_BUFFER_SIZE 16000
static int16_t g_audio_buffer[AUDIO_BUFFER_SIZE] = {0};
static volatile uint32_t g_audio_buffer_write_ptr = 0;
static volatile bool g_audio_buffer_ready = false;

static char g_ai_detected_buf[64] = "DETECTED: -";
static char g_ai_timing_buf[64] = "NN: - ms";

// Audio get_data callback for Edge Impulse
static int get_audio_data(size_t offset, size_t length, float *out_ptr)
{
    for (size_t i = 0; i < length; i++)
    {
        uint32_t idx = (g_audio_buffer_write_ptr + offset + i) % AUDIO_BUFFER_SIZE;
        out_ptr[i] = (float)g_audio_buffer[idx];
    }
    return 0;
}

// Custom hardware-accelerated 5x7 font rendering using DAVE 2D
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

// PDM Audio Capture configurations
#define FRAME_SAMPLES (1600) // 50ms frame at 32kHz
static int32_t g_pcm32_buffer[FRAME_SAMPLES * 2] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");

#define WAVEFORM_POINTS (1024)
static int16_t g_waveform_history[WAVEFORM_POINTS] = {0};
static volatile uint32_t g_pdm_event_count = 0;
static volatile uint32_t g_rms_val = 0;
static volatile int32_t g_peak_to_peak = 0;
static volatile bool g_pdm_data_ready = false;
static uint8_t g_pcm32_frame_idx = 0;
static uint8_t g_pcm_bits = 20;

struct sbrk_log_entry {
    int incr;
    uint32_t offset_before;
    uint32_t ret;
};
extern "C" struct sbrk_log_entry g_sbrk_log[16] = { {0} };
extern "C" volatile int g_sbrk_log_idx = 0;

// PDM callback (C linkage)
extern "C" void pdm_callback(pdm_callback_args_t *p_args)
{
    if (p_args->event == PDM_EVENT_DATA)
    {
        uint32_t src_offset = g_pcm32_frame_idx * FRAME_SAMPLES;
        int32_t *p_read = &g_pcm32_buffer[src_offset];

        // Invalidate cache before reading DMA-updated buffer
        SCB_InvalidateDCache_by_Addr((void *)p_read, FRAME_SAMPLES * sizeof(int32_t));

        int32_t min_val = 2147483647;
        int32_t max_val = -2147483648;
        uint64_t sum_squares = 0;

        for (uint32_t i = 0; i < FRAME_SAMPLES; i++)
        {
            int32_t val = p_read[i];
            
            // 32ビットレジスタ内にパッキングされている20ビット符号付きPCMデータ(右詰め)を、
            // 符号拡張(sign-extension)して正しい32ビット符号付き整数値にします
            int32_t extended = (val << 12) >> 12;
            
            // 20ビット符号付きデータを16ビット符号付き範囲（-32768〜32767）に規格化するため、4ビット右シフト（16分の1）します
            int16_t sample16 = (int16_t)(extended >> 4);

            if (extended < min_val) min_val = extended;
            if (extended > max_val) max_val = extended;
            sum_squares += (int64_t)sample16 * sample16;

            if (i < WAVEFORM_POINTS)
            {
                g_waveform_history[i] = sample16;
            }

            // Downsample by 2: append 16-bit PCM values to the circular buffer at 16kHz
            if ((i % 2) == 0)
            {
                g_audio_buffer[g_audio_buffer_write_ptr] = sample16;
                uint32_t next_ptr = g_audio_buffer_write_ptr + 1;
                if (next_ptr >= AUDIO_BUFFER_SIZE)
                {
                    next_ptr = 0;
                    g_audio_buffer_ready = true;
                }
                g_audio_buffer_write_ptr = next_ptr;
            }
        }

        // ピーク値も16ビットスケールに変換します
        g_peak_to_peak = (max_val - min_val) >> 4;
        g_rms_val = integer_sqrt(sum_squares / FRAME_SAMPLES);

        g_pdm_event_count++;
        g_pdm_data_ready = true;
        g_pcm32_frame_idx ^= 1U;
    }
}

// I2C コールバックイベント受信用変数
static volatile i2c_master_event_t i2c_event = (i2c_master_event_t)0;

// 3つ目のフレームバッファを追加 (トリプルバッファ用。1.2MB分を外部SDRAMセクションに確保)
uint8_t fb_background_2[DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");

// C言語のリンケージを持つコールバック関数
extern "C" void g_cam_i2c_master_user_callback(i2c_master_callback_args_t * p_args)
{
    i2c_event = p_args->event;
}

// Vblank（垂直同期）検出用フラグとカウンタ
static volatile bool vblank_flag = false;
static volatile uint32_t vblank_interrupt_count = 0;

extern "C" ID tskid_1 = 0;                 // Task ID number (Accessed by C drivers)
LOCAL ID tskid_2;                          // Task ID number
LOCAL ID tskid_3;                          // Task ID number

// LCD GLCDC コールバック関数 (Vblank割り込み時に実行)
extern "C" void lcd_glcdc_callback(display_callback_args_t * p_args)
{
    if (p_args->event == DISPLAY_EVENT_LINE_DETECTION)
    {
        vblank_flag = true;
        vblank_interrupt_count++;
        // [重要] Vblank割り込みが入った瞬間に、超高速で描画タスク(task_1)を起こす
        tk_wup_tsk(tskid_1);
    }
}

// I2Cの完了イベントを待つヘルパー関数
static fsp_err_t wait_i2c_event(void)
{
    volatile uint32_t timeout = 1000000;
    while (i2c_event == (i2c_master_event_t)0 && timeout > 0)
    {
        timeout--;
    }
    
    if (timeout == 0)
    {
        return FSP_ERR_TIMEOUT;
    }
    
    if (i2c_event != I2C_MASTER_EVENT_ABORTED)
    {
        i2c_event = (i2c_master_event_t)0;
        return FSP_SUCCESS;
    }
    
    i2c_event = (i2c_master_event_t)0;
    return FSP_ERR_TRANSFER_ABORTED;
}

// 16ビットレジスタアドレスから8ビットデータを読み出す関数
static bool rdSensorReg16_8(uint16_t regID, uint8_t *regDat)
{
    fsp_err_t err;
    uint8_t data[2] = {(uint8_t)(regID >> 8), (uint8_t)regID};
    
    i2c_event = (i2c_master_event_t)0;
    err = R_IIC_MASTER_Write(&g_cam_i2c_master_ctrl, data, 2, true); // restart = true
    if (FSP_SUCCESS == err)
    {
        err = wait_i2c_event();
    }
    
    if (FSP_SUCCESS == err)
    {
        i2c_event = (i2c_master_event_t)0;
        err = R_IIC_MASTER_Read(&g_cam_i2c_master_ctrl, regDat, 1, false);
        if (FSP_SUCCESS == err)
        {
            err = wait_i2c_event();
        }
    }
    return (FSP_SUCCESS == err);
}

extern "C" void *__malloc_sbrk_start;
extern "C" void *__malloc_free_list;
LOCAL void task_1(INT stacd, void *exinf); // task execution function

LOCAL T_CTSK ctsk_1 = {
    .exinf   = NULL,               // 1. 拡張情報
    .tskatr  = TA_HLNG | TA_RNG3,  // 2. タスク属性
    .task    = (FP)task_1,         // 3. タスク関数
    .itskpri = 10,                 // 4. 優先度
    .stksz   = 32768,              // 5. スタックサイズ (32KBに拡張してスタックオーバーフローを防止)
    .bufptr  = NULL                // 6. スタックバッファポインタ
};

LOCAL void task_2(INT stacd, void *exinf); // task execution function
LOCAL void task_3(INT stacd, void *exinf); // task execution function

LOCAL T_CTSK ctsk_2 = {
    .exinf   = NULL,
    .tskatr  = TA_HLNG | TA_RNG3,
    .task    = (FP)task_2,
    .itskpri = 10,
    .stksz   = 1024,
    .bufptr  = NULL
};

LOCAL T_CTSK ctsk_3 = {
    .exinf   = NULL,
    .tskatr  = TA_HLNG | TA_RNG3,
    .task    = (FP)task_3,
    .itskpri = 11,                 // UI描画タスク(10)より低い優先度
    .stksz   = 32768,              // TFLite Micro用に32KBスタックを確保
    .bufptr  = NULL
};

extern "C" void *__malloc_sbrk_start;
extern "C" void *__malloc_free_list;
LOCAL void task_1(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    tm_printf((UB *)"[CHECK 1] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);

    // 電源ON直後のマイコンおよび液晶パネル周辺回路の電源電圧の安定化を待つため、
    // タスク起動の直後に1000msのウェイトを入れます (コールドスタート対策)
    tk_dly_tsk(1000);

    // FSPで自動生成されたピン定義(g_bsp_pin_cfg)を実行時に再適用し、
    // セキュリティ設定や液晶・周辺ピンの設定を確実に初期化・有効化します。
    R_IOPORT_PinsCfg(&g_ioport_ctrl, &g_bsp_pin_cfg);
    tm_printf((UB *)"[CHECK 2] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);

    // 外部SDRAMの初期化を実行
    R_BSP_SdramInit(true);
    tm_printf((UB *)"[CHECK 3] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);

    // ----------------------------------------------------
    // [重要] キャッシュの完全クリーン＆無効化 (コールドブート時のキャッシュ汚染対策)
    // ----------------------------------------------------
    // これまでにCPUキャッシュに入り込んだ可能性のあるダーティラインをすべて物理メモリへ書き戻し、
    // キャッシュをクリーンな状態に初期化して、DMA（D/AVE 2D, GLCDC）との不整合を防ぎます。
    SCB_CleanInvalidateDCache();

    tm_printf((UB *)"\n=== Camera I2C & LCD D2D Connection Test Start ===\n");

    // ----------------------------------------------------
    // [1] I2C カメラ接続確認テスト
    // ----------------------------------------------------
    tm_printf((UB *)"Resetting Camera (CAMERA_RESET -> P709)...\n");
    R_IOPORT_PinWrite(&g_ioport_ctrl, CAMERA_RESET, BSP_IO_LEVEL_LOW);
    tk_dly_tsk(100);
    R_IOPORT_PinWrite(&g_ioport_ctrl, CAMERA_RESET, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(10);

    tm_printf((UB *)"Starting GPT Clock for Camera XCLK (g_cam_clk)...\n");
    fsp_err_t err = R_GPT_Open(&g_cam_clk_ctrl, &g_cam_clk_cfg);
    if (FSP_SUCCESS == err)
    {
        R_GPT_Start(&g_cam_clk_ctrl);
    }

    tm_printf((UB *)"Opening I2C Master (g_cam_i2c_master)...\n");
    err = R_IIC_MASTER_Open(&g_cam_i2c_master_ctrl, &g_cam_i2c_master_cfg);
    if (FSP_SUCCESS == err)
    {
        R_IIC_MASTER_SlaveAddressSet(&g_cam_i2c_master_ctrl, 0x3C, I2C_MASTER_ADDR_MODE_7BIT);
        
        tm_printf((UB *)"Reading OV5640 Product ID registers via I2C...\n");
        uint8_t pid_h = 0;
        uint8_t pid_l = 0;
        bool read_success = rdSensorReg16_8(0x300a, &pid_h) && rdSensorReg16_8(0x300b, &pid_l);

        if (read_success)
        {
            tm_printf((UB *)"Product ID Read: H = 0x%02X, L = 0x%02X\n", pid_h, pid_l);
            if (pid_h == 0x56 && (pid_l == 0x40 || pid_l == 0x41 || pid_l == 0x4C))
            {
                tm_printf((UB *)"SUCCESS: Camera connection verified! (OV5640 detected)\n");
            }
            else
            {
                tm_printf((UB *)"ERROR: Product ID mismatch\n");
            }
        }
        else
        {
            tm_printf((UB *)"ERROR: Camera communication failed.\n");
        }
    }

    // ----------------------------------------------------
    // [1.5] SDRAM セルフテスト (物理メモリ検証)
    // ----------------------------------------------------
    tm_printf((UB *)"Testing physical SDRAM at address 0x%08X...\n", (uint32_t)&fb_background[0][0]);
    volatile uint32_t *p_sdram = (volatile uint32_t *)&fb_background[0][0];
    bool sdram_ok = true;
    for (uint32_t i = 0; i < 1024; i++)
    {
        p_sdram[i] = 0x55AA55AA ^ i;
    }
    
    // 書き込んだデータをキャッシュから物理メモリにフラッシュ & キャッシュ無効化
    SCB_CleanInvalidateDCache_by_Addr((void *)&fb_background[0][0], 1024 * sizeof(uint32_t));

    // 物理メモリから直接読み出して比較
    for (uint32_t i = 0; i < 1024; i++)
    {
        if (p_sdram[i] != (0x55AA55AA ^ i))
        {
            tm_printf((UB *)"SDRAM verification failed at index %d! Read: 0x%08X, Expected: 0x%08X\n", i, p_sdram[i], 0x55AA55AA ^ i);
            sdram_ok = false;
            break;
        }
    }
    if (sdram_ok)
    {
        tm_printf((UB *)"SDRAM verification SUCCESS!\n");
    }
    else
    {
        tm_printf((UB *)"ERROR: SDRAM physical memory write/read failed!\n");
    }
    tm_printf((UB *)"[CHECK 4] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);

    // テスト領域のキャッシュを再度クリーンにする
    SCB_CleanInvalidateDCache_by_Addr((void *)&fb_background[0][0], 1024 * sizeof(uint32_t));

    // ----------------------------------------------------
    // [重要] AXIバス調停（Arbitration）の設定 (GLCDC最優先・固定優先)
    // ----------------------------------------------------
    tm_printf((UB *)"=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===\n");
    for (int i = 0; i < 18; i++)
    {
        // ARBMET = 1 (Fixed Priority), ARBS = 3 (Group 3) -> 0x0013
        R_BUS->BUSS[i].CNT = 0x0013;
    }

    // ----------------------------------------------------
    // [重要] フレームバッファのゼロクリア (起動時の液晶同期ロスト防止対策)
    // ----------------------------------------------------
    tm_printf((UB *)"Clearing SDRAM framebuffers to black...\n");
    memset(fb_background, 0, sizeof(fb_background));
    memset(fb_background_2, 0, sizeof(fb_background_2));
    
    // クリアしたデータを確実に物理SDRAMへ書き戻す
    SCB_CleanInvalidateDCache();
    tm_printf((UB *)"[CHECK 5] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);

    // ----------------------------------------------------
    // [2] LCD ディスプレイ & D/AVE 2D (D2D) 初期化
    // ----------------------------------------------------
    tm_printf((UB *)"Initializing LCD (GLCDC)... \n");

    // MIPIインターフェーススイッチ有効化 (ボードのマルチプレクサをMIPI側に接続)
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_08, BSP_IO_LEVEL_LOW);
    
    // 液晶パネルのリセットシーケンス（500ms / 200ms / 500ms）
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_RESET, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(500);
 
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_RESET, BSP_IO_LEVEL_LOW);
    tk_dly_tsk(200);
 
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_RESET, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(500);
 
    // GLCDC オープンとスタート
    err = R_GLCDC_Open(&g_lcd_glcdc_ctrl, &g_lcd_glcdc_cfg);
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
    tm_printf((UB *)"[CHECK 6] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);
    
    // 最初のバッファとして設定
    uint8_t *const my_fb[3] = {
        (uint8_t *)&fb_background[0][0],
        (uint8_t *)&fb_background[1][0],
        (uint8_t *)&fb_background_2[0]
    };
    R_GLCDC_BufferChange(&g_lcd_glcdc_ctrl, my_fb[0], DISPLAY_FRAME_LAYER_1);
    tm_printf((UB *)"[CHECK 6A] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);
 
    // バックライトON (DISP_BLEN -> P514)
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_BLEN, BSP_IO_LEVEL_HIGH);
    tm_printf((UB *)"LCD Backlight enabled.\n");
    tm_printf((UB *)"[CHECK 6B] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);
 
    // D/AVE 2D ドライバのオープンと初期化
    tm_printf((UB *)"Initializing D/AVE 2D Graphics Engine...\n");
    
    // Print all _sbrk calls recorded so far
    for (int idx = 0; idx < g_sbrk_log_idx && idx < 16; idx++) {
        tm_printf((UB *)"[_sbrk LOG %d] incr = %d, offset_before = 0x%08X, ret = 0x%08X\n", 
                  idx, g_sbrk_log[idx].incr, (unsigned int)g_sbrk_log[idx].offset_before, (unsigned int)g_sbrk_log[idx].ret);
    }

    void *p_test = malloc(100);
    tm_printf((UB *)"TEST MALLOC: address = 0x%08X\n", (unsigned int)p_test);
    free(p_test);

    // Reset log index to capture d2_opendevice allocations
    int start_idx_d2 = g_sbrk_log_idx;

    d2_handle = d2_opendevice(0);
    tm_printf((UB *)"D2_HANDLE Address: 0x%08X\n", (unsigned int)d2_handle);

    for (int idx = start_idx_d2; idx < g_sbrk_log_idx && idx < 16; idx++) {
        tm_printf((UB *)"[_sbrk D2 LOG %d] incr = %d, offset_before = 0x%08X, ret = 0x%08X\n", 
                  idx, g_sbrk_log[idx].incr, (unsigned int)g_sbrk_log[idx].offset_before, (unsigned int)g_sbrk_log[idx].ret);
    }

    tm_printf((UB *)"[CHECK 6C] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);
    if (NULL == d2_handle)
    {
        tm_printf((UB *)"ERROR: Failed to open D/AVE 2D engine\n");
        while(1) { tk_dly_tsk(1000); }
    }
    d2_inithw(d2_handle, 0);
    tm_printf((UB *)"[CHECK 7] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);
 
    // 2D描画パラメータ設定 (バス負荷軽減のためコピーモード・アンチエイリアス無効に設定)
    d2_setblendmode(d2_handle, d2_bm_one, d2_bm_zero);
    d2_setalphamode(d2_handle, d2_am_constant);
    d2_setalpha(d2_handle, 0xff);
    d2_setantialiasing(d2_handle, 0);

    // [重要] 全フレームバッファを背景色 (0xFF101015) で一括初期化し、外枠やタイトルなどの静的描画を完了させておく
    // これにより、毎フレームの描画データ量を約半分に削減し、SDRAMバスのボトルネックによる画面のチラつきを解消します。
    tm_printf((UB *)"Pre-rendering background and static borders to all 3 framebuffers...\n");
    for (int i = 0; i < 3; i++)
    {
        d2_startframe(d2_handle);
        d2_framebuffer(d2_handle, my_fb[i], DISPLAY_HSIZE_INPUT0, DISPLAY_HSIZE_INPUT0, DISPLAY_VSIZE_INPUT0, d2_mode_rgb565);
        d2_clear(d2_handle, 0xFF101015);
        
        // 外枠のネオンシアン線 (0xFF00E5FF)
        d2_setcolor(d2_handle, 0, 0xFF00E5FF);
        d2_renderline(d2_handle, 30 << 4, 30 << 4, (DISPLAY_HSIZE_INPUT0 - 30) << 4, 30 << 4, 2 << 4, 0); // Top
        d2_renderline(d2_handle, 30 << 4, (DISPLAY_VSIZE_INPUT0 - 30) << 4, (DISPLAY_HSIZE_INPUT0 - 30) << 4, (DISPLAY_VSIZE_INPUT0 - 30) << 4, 2 << 4, 0); // Bottom
        d2_renderline(d2_handle, 30 << 4, 30 << 4, 30 << 4, (DISPLAY_VSIZE_INPUT0 - 30) << 4, 2 << 4, 0); // Left
        d2_renderline(d2_handle, (DISPLAY_HSIZE_INPUT0 - 30) << 4, 30 << 4, (DISPLAY_HSIZE_INPUT0 - 30) << 4, (DISPLAY_VSIZE_INPUT0 - 30) << 4, 2 << 4, 0); // Right
        
        // タイトル
        print_string_5x7(d2_handle, 50, 50, 3, "EK-RA8P1 PDM MICROPHONE TEST (microT-Kernel 3.0)", 0xFF00E5FF);
        
        // 波形表示エリアの外枠
        d2_setcolor(d2_handle, 0, 0xFF3A3A4A);
        d2_renderbox(d2_handle, 50 << 4, 180 << 4, 924 << 4, 300 << 4);
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, 51 << 4, 181 << 4, 922 << 4, 298 << 4);
        
        // 音量表示エリアの外枠
        d2_setcolor(d2_handle, 0, 0xFF3A3A4A);
        d2_renderbox(d2_handle, 50 << 4, 510 << 4, 924 << 4, 30 << 4);
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, 51 << 4, 511 << 4, 922 << 4, 28 << 4);
        
        d2_endframe(d2_handle);
        SCB_CleanInvalidateDCache();
        d2_flushframe(d2_handle);
        SCB_CleanInvalidateDCache();
    }
    tm_printf((UB *)"[CHECK 8] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);

    // ----------------------------------------------------
    // [PDMマイク初期化 & 録音開始]
    // ----------------------------------------------------
    tm_printf((UB *)"Opening PDM driver (g_pdm0)...\n");
    fsp_err_t pdm_err = R_PDM_Open(&g_pdm0_ctrl, &g_pdm0_cfg);
    tm_printf((UB *)"[CHECK 9] learning_blocks = 0x%08X, sbrk_offset = 0x%08X, sbrk_start = 0x%08X, free_list = 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks, (unsigned int)*(volatile uint32_t *)0x2204c000, (unsigned int)__malloc_sbrk_start, (unsigned int)__malloc_free_list);
    if (FSP_SUCCESS == pdm_err)
    {
        tm_printf((UB *)"PDM driver opened. Waiting for settling...\n");
        tk_dly_tsk(30); // フィルター安定のため60ms待機

        pdm_err = R_PDM_Start(&g_pdm0_ctrl, g_pcm32_buffer, sizeof(g_pcm32_buffer), FRAME_SAMPLES);
        if (FSP_SUCCESS == pdm_err)
        {
            tm_printf((UB *)"PDM capture started successfully.\n");
        }
        else
        {
            tm_printf((UB *)"ERROR: Failed to start PDM capture (0x%x)\n", pdm_err);
        }
    }
    else
    {
        tm_printf((UB *)"ERROR: Failed to open PDM driver (0x%x)\n", pdm_err);
    }

    tm_printf((UB *)"Starting D/AVE 2D Rendering Loop (PDM Audio Visualizer)...\n");

    uint8_t draw_buf = 0;
    uint8_t pending_buf = 1;
    uint8_t display_buf = 2;
    int loop_cnt = 0;

    while (1)
    {
        // [重要] GLCDCのVblank（垂直同期）割り込みが入るまでスリープ待機
        // これにより、CPUパワーを浪費せず、描画タイミングをディスプレイリフレッシュレートに完全に同期させます。
        vblank_flag = false;
        tk_can_wup(TSK_SELF); 
        tk_slp_tsk(100); 

        if (loop_cnt % 100 == 0)
        {
            tm_printf((UB *)"Loop %d: Vblank: %d, PDM: %d, RMS: %d, DrawBuf: %d\n", 
                      loop_cnt, (int)vblank_interrupt_count, (int)g_pdm_event_count, (int)g_rms_val, (int)draw_buf);
            // マイク生値デバッグ用（先頭4サンプル）
            tm_printf((UB *)"  Raw Samples: 0x%08X 0x%08X 0x%08X 0x%08X\n", 
                      (unsigned int)g_pcm32_buffer[0], (unsigned int)g_pcm32_buffer[1], 
                      (unsigned int)g_pcm32_buffer[2], (unsigned int)g_pcm32_buffer[3]);
        }

        // Start a new frame
        d2_startframe(d2_handle);

        // Bind framebuffer
        d2_framebuffer(d2_handle, my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, DISPLAY_HSIZE_INPUT0, DISPLAY_VSIZE_INPUT0, d2_mode_rgb565);

        // [バス負荷削減] 画面全体のクリア(d2_clear)は1.2MB分のSDRAMライトを発生させ帯域を飽和させるため、廃止します。
        // 代わりに、更新が必要な特定の描画領域のみを部分的に塗りつぶし（ボックスクリア）します。
        


        // 2. 波形表示エリア内側のクリア
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, 51 << 4, 181 << 4, 922 << 4, 298 << 4);

        // Zero-line reference
        d2_setcolor(d2_handle, 0, 0xFF2A2A35);
        d2_renderline(d2_handle, 51 << 4, 330 << 4, 973 << 4, 330 << 4, 1 << 4, 0);

        // Draw waveform
        d2_setcolor(d2_handle, 0, 0xFF00FF66); // Neon spring green
        int step = 2;
        int prev_x = 51;
        int prev_y = 330;
        
        for (int i = 0; i < 922; i += step)
        {
            int idx = i * WAVEFORM_POINTS / 922;
            if (idx >= WAVEFORM_POINTS) idx = WAVEFORM_POINTS - 1;
            
            int16_t sample16 = g_waveform_history[idx];
            // 縦軸の振幅を2倍にするため、分母を256にして拡大
            int32_t y_offset = ((int32_t)sample16 * 140) / 256;
            int y_coord = 330 - y_offset;
            int x_coord = 51 + i;
            
            // 描画ボックス(Y: 181 〜 479)の外にはみ出さないようクリッピング
            if (y_coord < 181) y_coord = 181;
            if (y_coord > 479) y_coord = 479;
            
            if (i > 0)
            {
                d2_renderline(d2_handle, prev_x << 4, prev_y << 4, x_coord << 4, y_coord << 4, 2 << 4, 0);
            }
            prev_x = x_coord;
            prev_y = y_coord;
        }

        // 3. 音量バー表示エリア内側のクリア
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, 51 << 4, 511 << 4, 922 << 4, 28 << 4);

        // Render filled volume bar based on RMS
        // 最大期待RMS値を 256（規格化前は 4096）と想定し、922px幅いっぱいに動くようにします。
        int32_t vol_width = ((int32_t)g_rms_val * 922) / 256; 
        if (vol_width > 922) vol_width = 922;
        
        if (vol_width > 0)
        {
            // Gradient color emulation: Cyan (0xFF00E5FF)
            d2_setcolor(d2_handle, 0, 0xFF00E5FF);
            d2_renderbox(d2_handle, 51 << 4, 511 << 4, vol_width << 4, 28 << 4);
        }

        // Display status & metrics using CPU rendering to bypass D/AVE 2D overhead and command queue limits
        char status_buf[64];
        if (pdm_err == FSP_SUCCESS) {
            sprintf(status_buf, "PDM STATUS: RUNNING   IRQ COUNT: %d", (int)g_pdm_event_count);
        } else {
            sprintf(status_buf, "PDM STATUS: ERROR (0x%04X)", (int)pdm_err);
        }
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 50, 95, 2, status_buf, 0xFFFF, 0x1082);

        char metrics_buf[64];
        sprintf(metrics_buf, "RMS VOLUME: %d   PEAK-TO-PEAK: %d", (int)g_rms_val, (int)g_peak_to_peak);
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 50, 125, 2, metrics_buf, 0xFFFF, 0x1082);

        // Display AI Voice Detection Result on the right-hand empty space (scale 3 for larger size)
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 520, 85, 3, g_ai_detected_buf, 0xFFFF, 0x1082);
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 520, 130, 3, g_ai_timing_buf, 0xFFFF, 0x1082);

        // End rendering command list
        d2_endframe(d2_handle);

        // [高速化] CPUで描画したテキスト領域（Y:80〜175, 高さ95ライン分）のみをピンポイントでキャッシュクリーンする。
        // これにより全体キャッシュクリーン(SCB_CleanInvalidateDCache)によるバス飽和と処理遅延、フリッカーを完全に回避します。
        SCB_CleanInvalidateDCache_by_Addr((void *)((uint16_t *)my_fb[draw_buf] + 80 * DISPLAY_HSIZE_INPUT0), DISPLAY_HSIZE_INPUT0 * 95 * sizeof(uint16_t));

        // Flush rendering and wait for GPU completion
        d2_flushframe(d2_handle);

        // GLCDCに対して、新しく描き終わったバッファへの切り替えを要求
        fsp_err_t glcdc_err = R_GLCDC_BufferChange(&g_lcd_glcdc_ctrl, my_fb[draw_buf], DISPLAY_FRAME_LAYER_1);
        if (FSP_SUCCESS != glcdc_err)
        {
            tm_printf((UB *)"GLCDC BufferChange Error: %d\n", glcdc_err);
        }

        // バッファインデックスのローテーション (トリプルバッファリング)
        // YOLOプロジェクトと同一 of 完全同期ロテーションを適用します
        uint8_t next_draw = display_buf;
        uint8_t next_pending = draw_buf;
        uint8_t next_display = pending_buf;

        draw_buf = next_draw;
        pending_buf = next_pending;
        display_buf = next_display;

        loop_cnt++;
    }
}

LOCAL void task_2(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    volatile uint32_t *p_target = (volatile uint32_t *)&impulse_943529_1.learning_blocks;
    uint32_t last_val = *p_target;

    tm_printf((UB *)"Monitoring learning_blocks at address 0x%08X. Initial: 0x%08X\n", (unsigned int)p_target, (unsigned int)last_val);

    while (1) {
        uint32_t val = *p_target;
        if (val != last_val) {
            tm_printf((UB *)"!!! VALUE CHANGED !!! Old: 0x%08X, New: 0x%08X\n", (unsigned int)last_val, (unsigned int)val);
            last_val = val;
        }
        tk_dly_tsk(1);
    }
}

LOCAL void task_3(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    // Wait for display and PDM drivers to stabilize
    tk_dly_tsk(1500);

    tm_printf((UB *)"=== Starting Arm Ethos-U55 NPU Voice Detection Task (task_3)... ===\n");

    // Relocate interrupt vector table for Vector 20 (NPU IRQ) under microT-Kernel
    #define RAM_VECTOR_TABLE_SIZE (16 + 32)
    __attribute__((aligned(256))) static fsp_vector_t g_ram_vector_table[RAM_VECTOR_TABLE_SIZE];
    
    uint32_t current_vtor = SCB->VTOR;
    memcpy(g_ram_vector_table, (void*)current_vtor, 29 * sizeof(fsp_vector_t));
    g_ram_vector_table[16 + 20] = (fsp_vector_t)rm_ethosu_isr;
    
    __disable_irq();
    SCB->VTOR = (uint32_t)g_ram_vector_table;
    __DSB();
    __ISB();
    __enable_irq();
    
    R_ICU->IELSR[20] = (uint32_t) ELC_EVENT_NPU_IRQ;

    // Open NPU driver
    fsp_err_t err = RM_ETHOSU_Open(&g_rm_ethosu0_ctrl, &g_rm_ethosu0_cfg);
    if (FSP_SUCCESS != err)
    {
        tm_printf((UB *)"ERROR: Failed to open Arm Ethos-U55 NPU (0x%x)\n", err);
        while(1) { tk_dly_tsk(1000); }
    }
    tm_printf((UB *)"SUCCESS: Arm Ethos-U55 NPU driver initialized successfully!\n");

    // Diagnostic prints for ei_impulse_t layout and pointers
    #include <stddef.h>
    tm_printf((UB *)"=== DIAGNOSTICS ===\n");
    tm_printf((UB *)"Size of ei_impulse_t: %d\n", (int)sizeof(ei_impulse_t));
    tm_printf((UB *)"Offset of dsp_blocks: %d\n", (int)offsetof(ei_impulse_t, dsp_blocks));
    tm_printf((UB *)"Offset of learning_blocks: %d\n", (int)offsetof(ei_impulse_t, learning_blocks));
    tm_printf((UB *)"Offset of postprocessing_blocks: %d\n", (int)offsetof(ei_impulse_t, postprocessing_blocks));
    tm_printf((UB *)"Offset of label_count: %d\n", (int)offsetof(ei_impulse_t, label_count));
    tm_printf((UB *)"Offset of categories: %d\n", (int)offsetof(ei_impulse_t, categories));
    tm_printf((UB *)"impulse_943529_1 address: 0x%08X\n", (unsigned int)&impulse_943529_1);
    tm_printf((UB *)"learning_blocks value in struct: 0x%08X\n", (unsigned int)impulse_943529_1.learning_blocks);
    tm_printf((UB *)"learning_blocks actual address: 0x%08X\n", (unsigned int)ei_learning_blocks_943529_1);
    tm_printf((UB *)"===================\n");

    while (1)
    {
        // Run inference every 250ms
        tk_dly_tsk(250);

        if (!g_audio_buffer_ready)
        {
            continue;
        }

        // Setup signal struct for Edge Impulse
        signal_t signal;
        signal.total_length = AUDIO_BUFFER_SIZE;
        signal.get_data = &get_audio_data;

        ei_impulse_result_t result = { 0 };

        // Run classifier (will invoke NPU via overridden compiled model)
        EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

        if (res == EI_IMPULSE_OK)
        {
            // Find class with highest probability
            int max_idx = -1;
            float max_prob = 0.0f;
            for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
            {
                if (result.classification[i].value > max_prob)
                {
                    max_prob = result.classification[i].value;
                    max_idx = (int)i;
                }
            }

            if (max_idx >= 0)
            {
                char temp_detected[64];
                sprintf(temp_detected, "DETECTED: %s (%d%%)", 
                        result.classification[max_idx].label, 
                        (int)(max_prob * 100.0f));
                // Pad to exactly 24 chars to prevent trailing character leakage on screen
                sprintf(g_ai_detected_buf, "%-24s", temp_detected);

                int dsp_ms = (int)(result.timing.dsp_us / 1000);
                int dsp_us = (int)(result.timing.dsp_us % 1000);

                int nn_ms = (int)(result.timing.classification_us / 1000);
                int nn_us = (int)(result.timing.classification_us % 1000);

                int64_t total_us = result.timing.dsp_us + result.timing.classification_us;
                int total_ms = (int)(total_us / 1000);
                int total_us_rem = (int)(total_us % 1000);

                char temp_timing[64];
                sprintf(temp_timing, "NN: %d.%03d ms", nn_ms, nn_us);
                // Pad to exactly 24 chars to prevent trailing character leakage on screen
                sprintf(g_ai_timing_buf, "%-24s", temp_timing);

                // Print detailed timings to console
                tm_printf((UB *)"[task_3] DETECTED: %s (%d%%) | DSP: %d.%03d ms | NN: %d.%03d ms | Total: %d.%03d ms\n",
                          result.classification[max_idx].label,
                          (int)(max_prob * 100.0f),
                          dsp_ms, dsp_us,
                          nn_ms, nn_us,
                          total_ms, total_us_rem);
            }
        }
        else
        {
            tm_printf((UB*)"[task_3] run_classifier failed with %d\n", res);
            sprintf(g_ai_detected_buf, "DETECTED: ERROR (%d)  ", res);
            sprintf(g_ai_timing_buf, "NN: - ms                ");
        }
    }
}

extern "C" EXPORT INT usermain(void)
{
    tm_putstring((UB *)"Start User-main program (Camera & LCD D2D Test).\n");

    /* Create & Start Tasks */
    tskid_1 = tk_cre_tsk(&ctsk_1);
    tk_sta_tsk(tskid_1, 0);

    tskid_2 = tk_cre_tsk(&ctsk_2);
    tk_sta_tsk(tskid_2, 0);

    tskid_3 = tk_cre_tsk(&ctsk_3);
    tk_sta_tsk(tskid_3, 0);

    tk_slp_tsk(TMO_FEVR);
    return 0;
}

// Edge Impulse SDK Porting APIs
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
