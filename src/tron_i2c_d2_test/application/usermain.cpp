// 修正ポイント1：C言語で書かれたOSやライブラリのヘッダファイルを extern "C" で囲む
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

// Waveform plot layout (Restored to 1024x600 parallel LCD layout)
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

// 3つ目のフレームバッファを追加 (トリプルバッファ用。1.2MB分を外部SDRAMセクションに確保)
uint8_t fb_background_2[DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");

// Vblank（垂直同期）検出用フラグとカウンタ
static volatile bool vblank_flag = false;
static volatile uint32_t vblank_interrupt_count = 0;

LOCAL ID tskid_1;                          // Task ID number
LOCAL ID tskid_2;                          // Task ID number

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

LOCAL T_CTSK ctsk_2 = {
    .exinf   = NULL,
    .tskatr  = TA_HLNG | TA_RNG3,
    .task    = (FP)task_2,
    .itskpri = 9,                  // 優先度9 (描画タスクの優先度10より高く設定して、104Hz周期を安定維持させる)
    .stksz   = 4096,               // スタックサイズを4KBに設定
    .bufptr  = NULL
};

LOCAL void task_1(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    // 電源ON直後のマイコンおよび液晶パネル周辺回路の電源電圧の安定化を待つため、
    // タスク起動の直後に1000msのウェイトを入れます (コールドスタート対策)
    tk_dly_tsk(1000);

    // FSPで自動生成されたピン定義(g_bsp_pin_cfg)を実行時に再適用し、
    // セキュリティ設定や液晶・周辺ピンの設定を確実に初期化・有効化します。
    R_IOPORT_PinsCfg(&g_ioport_ctrl, &g_bsp_pin_cfg);

    // 外部SDRAMの初期化を実行
    R_BSP_SdramInit(true);

    // [重要] キャッシュの完全クリーン＆無効化 (コールドブート時のキャッシュ汚染対策)
    SCB_CleanInvalidateDCache();

    tm_printf((UB *)"\n=== MPU-6050 Accelerometer Test Start ===\n");

    // [重要] AXIバス調停（Arbitration）の設定 (GLCDC最優先・固定優先)
    tm_printf((UB *)"=== Setting Bus Slave Arbitration to Fixed Priority (Group 3) ===\n");
    for (int i = 0; i < 18; i++)
    {
        // ARBMET = 1 (Fixed Priority), ARBS = 3 (Group 3) -> 0x0013
        R_BUS->BUSS[i].CNT = 0x0013;
    }

    // [重要] フレームバッファのゼロクリア (起動時の液晶同期ロスト防止対策)
    tm_printf((UB *)"Clearing SDRAM framebuffers to black...\n");
    memset(fb_background, 0, sizeof(fb_background));
    memset(fb_background_2, 0, sizeof(fb_background_2));
    
    // クリアしたデータを確実に物理SDRAMへ書き戻す
    SCB_CleanInvalidateDCache();

    // ----------------------------------------------------
    // [1] LCD ディスプレイ & D/AVE 2D (D2D) 初期化
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
    
    // 最初のバッファとして設定
    uint8_t *const my_fb[3] = {
        (uint8_t *)&fb_background[0][0],
        (uint8_t *)&fb_background[1][0],
        (uint8_t *)&fb_background_2[0]
    };
    R_GLCDC_BufferChange(&g_lcd_glcdc_ctrl, my_fb[0], DISPLAY_FRAME_LAYER_1);
 
    // バックライトON (DISP_BLEN -> P514)
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_BLEN, BSP_IO_LEVEL_HIGH);
    tm_printf((UB *)"LCD Backlight enabled.\n");
 
    // D/AVE 2D ドライバのオープンと初期化
    tm_printf((UB *)"Initializing D/AVE 2D Graphics Engine...\n");
    d2_handle = d2_opendevice(0);
    if (NULL == d2_handle)
    {
        tm_printf((UB *)"ERROR: Failed to open D/AVE 2D engine\n");
        while(1) { tk_dly_tsk(1000); }
    }
    d2_inithw(d2_handle, 0);
 
    // 2D描画パラメータ設定 (バス負荷軽減のためコピーモード・アンチエイリアス無効に設定)
    d2_setblendmode(d2_handle, d2_bm_one, d2_bm_zero);
    d2_setalphamode(d2_handle, d2_am_constant);
    d2_setalpha(d2_handle, 0xff);
    d2_setantialiasing(d2_handle, 0);

    // 全フレームバッファを背景色 (0xFF101015) で一括初期化し、外枠やタイトルなどの静的描画を完了させておく
    tm_printf((UB *)"Pre-rendering background and static borders to all 3 framebuffers...\n");
    for (int i = 0; i < 3; i++)
    {
        d2_startframe(d2_handle);
        d2_framebuffer(d2_handle, my_fb[i], DISPLAY_HSIZE_INPUT0, DISPLAY_HSIZE_INPUT0, DISPLAY_VSIZE_INPUT0, d2_mode_rgb565);
        d2_clear(d2_handle, 0xFF101015);
        
        // 外枠のネオンシアン線 (0xFF00E5FF)
        d2_setcolor(d2_handle, 0, 0xFF00E5FF);
        d2_renderline(d2_handle, 20 << 4, 30 << 4, (VISIBLE_WIDTH - 20) << 4, 30 << 4, 2 << 4, 0); // Top
        d2_renderline(d2_handle, 20 << 4, (VISIBLE_HEIGHT - 30) << 4, (VISIBLE_WIDTH - 20) << 4, (VISIBLE_HEIGHT - 30) << 4, 2 << 4, 0); // Bottom
        d2_renderline(d2_handle, 20 << 4, 30 << 4, 20 << 4, (VISIBLE_HEIGHT - 30) << 4, 2 << 4, 0); // Left
        d2_renderline(d2_handle, (VISIBLE_WIDTH - 20) << 4, 30 << 4, (VISIBLE_WIDTH - 20) << 4, (VISIBLE_HEIGHT - 30) << 4, 2 << 4, 0); // Right
        
        // タイトル
        print_string_5x7(d2_handle, 50, 50, 3, "EK-RA8P1 ACCELEROMETER TEST (microT-Kernel 3.0)", 0xFF00E5FF);
        
        // 波形表示エリアの外枠
        d2_setcolor(d2_handle, 0, 0xFF3A3A4A);
        d2_renderbox(d2_handle, (PLOT_X_START - 1) << 4, 180 << 4, (PLOT_WIDTH + 2) << 4, 300 << 4);
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 181 << 4, PLOT_WIDTH << 4, 298 << 4);
        
        // 合成加速度表示エリアの外枠
        d2_setcolor(d2_handle, 0, 0xFF3A3A4A);
        d2_renderbox(d2_handle, (PLOT_X_START - 1) << 4, 510 << 4, (PLOT_WIDTH + 2) << 4, 30 << 4);
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 511 << 4, PLOT_WIDTH << 4, 28 << 4);

        
        d2_endframe(d2_handle);
        SCB_CleanInvalidateDCache();
        d2_flushframe(d2_handle);
        SCB_CleanInvalidateDCache();
    }

    // ----------------------------------------------------
    // [MPU-6050 加速度センサー初期化]
    // ----------------------------------------------------
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
    int loop_cnt = 0;

    while (1)
    {
        // GLCDCのVblank（垂直同期）割り込みが入るまでスリープ待機 (60Hz同期)
        vblank_flag = false;
        tk_can_wup(TSK_SELF); 
        tk_slp_tsk(100); 

        if (loop_cnt % 100 == 0)
        {
            tm_printf((UB *)"Loop %d: Vblank: %d, MPU_IRQ: %d, X: %d, Y: %d, Z: %d, MAG: %d\n", 
                       loop_cnt, (int)vblank_interrupt_count, (int)g_i2c_event_count, 
                       (int)g_accel_x, (int)g_accel_y, (int)g_accel_z, (int)g_accel_mag);
        }

        // Start a new frame
        d2_startframe(d2_handle);

        // Bind framebuffer
        d2_framebuffer(d2_handle, my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, DISPLAY_HSIZE_INPUT0, DISPLAY_VSIZE_INPUT0, d2_mode_rgb565);

        // 波形表示エリア内側のクリア (背景塗りつぶし)
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 181 << 4, PLOT_WIDTH << 4, 298 << 4);

        // グリッド線（基準線のプロット）
        // ゼロ基準線（Y = 330）
        d2_setcolor(d2_handle, 0, 0xFF2A2A35);
        d2_renderline(d2_handle, PLOT_X_START << 4, 330 << 4, (PLOT_X_START + PLOT_WIDTH) << 4, 330 << 4, 1 << 4, 0);

        // +1g 基準線 (Y = 330 - 80 = 250)
        d2_setcolor(d2_handle, 0, 0xFF1C1C24);
        d2_renderline(d2_handle, PLOT_X_START << 4, 250 << 4, (PLOT_X_START + PLOT_WIDTH) << 4, 250 << 4, 1 << 4, 0);

        // -1g 基準線 (Y = 330 + 80 = 410)
        d2_setcolor(d2_handle, 0, 0xFF1C1C24);
        d2_renderline(d2_handle, PLOT_X_START << 4, 410 << 4, (PLOT_X_START + PLOT_WIDTH) << 4, 410 << 4, 1 << 4, 0);

        // 3軸波形のスナップショットコピー (ディスパッチ禁止でアトミックに取得)
        int16_t local_x_hist[HISTORY_LEN];
        int16_t local_y_hist[HISTORY_LEN];
        int16_t local_z_hist[HISTORY_LEN];
        
        tk_dis_dsp();
        memcpy(local_x_hist, (const void*)g_accel_x_history, sizeof(g_accel_x_history));
        memcpy(local_y_hist, (const void*)g_accel_y_history, sizeof(g_accel_y_history));
        memcpy(local_z_hist, (const void*)g_accel_z_history, sizeof(g_accel_z_history));
        tk_ena_dsp();

        // X軸 (赤)
        d2_setcolor(d2_handle, 0, 0xFFD50000);
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

        // Y軸 (緑)
        d2_setcolor(d2_handle, 0, 0xFF00FF66);
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

        // Z軸 (青/シアン)
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



        // 合成加速度バー表示エリア内側のクリア
        d2_setcolor(d2_handle, 0, 0xFF14141A);
        d2_renderbox(d2_handle, PLOT_X_START << 4, 511 << 4, PLOT_WIDTH << 4, 28 << 4);

        // 合成加速度 Magnitude バーを描画
        // 最大期待合成加速度値を 2.5g (40960 LSB) とする
        int32_t vol_width = ((int32_t)g_accel_mag * PLOT_WIDTH) / 40960; 
        if (vol_width > PLOT_WIDTH) vol_width = PLOT_WIDTH;
        
        if (vol_width > 0)
        {
            // ネオンシアン (0xFF00E5FF) で塗りつぶし
            d2_setcolor(d2_handle, 0, 0xFF00E5FF);
            d2_renderbox(d2_handle, PLOT_X_START << 4, 511 << 4, vol_width << 4, 28 << 4);
        }

        // テキスト情報の描画 (CPU描画を使用してチラつきを完全に防止)
        char line1_buf[64];
        char line2_buf[64];

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

        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 50, 95, 2, line1_buf, 0xFFFF, 0x1082);
        print_string_5x7_cpu((uint16_t *)my_fb[draw_buf], DISPLAY_HSIZE_INPUT0, 50, 125, 2, line2_buf, 0xFFFF, 0x1082);

        // End rendering command list
        d2_endframe(d2_handle);

        // テキスト領域 (Y:90〜145, 高さ55ライン) をキャッシュクリーン
        SCB_CleanInvalidateDCache_by_Addr((void *)((uint16_t *)my_fb[draw_buf] + 90 * DISPLAY_HSIZE_INPUT0), DISPLAY_HSIZE_INPUT0 * 55 * sizeof(uint16_t));


        // Flush rendering and wait for GPU completion
        d2_flushframe(d2_handle);

        // GLCDCに対してバッファ切り替え要求
        fsp_err_t glcdc_err = R_GLCDC_BufferChange(&g_lcd_glcdc_ctrl, my_fb[draw_buf], DISPLAY_FRAME_LAYER_1);
        if (FSP_SUCCESS != glcdc_err)
        {
            tm_printf((UB *)"GLCDC BufferChange Error: %d\n", glcdc_err);
        }

        // バッファインデックスのローテーション (トリプルバッファリング)
        uint8_t next_draw = display_buf;
        uint8_t next_pending = draw_buf;
        uint8_t next_display = pending_buf;

        draw_buf = next_draw;
        pending_buf = next_pending;
        display_buf = next_display;

        loop_cnt++;
    }
}

// task_2: 104Hz 高精度データ取得タスク (サンプリング処理を担当)
LOCAL void task_2(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    // task_1が初期化処理（GLCDC, SDRAM, 静的描画等）を完了させるまで十分待機する (3秒ウェイト)
    tk_dly_tsk(3000);

    tm_printf((UB *)"task_2 (104Hz Accelerometer Acquisition) started.\n");

    int delay_cnt = 0;

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
                
                // Magnitude = sqrt(ax^2 + ay^2 + az^2)
                uint64_t mag_sq = (int64_t)ax*ax + (int64_t)ay*ay + (int64_t)az*az;
                g_accel_mag = integer_sqrt(mag_sq);
                
                g_i2c_event_count++;
                
                // 104Hz センサー値のコンソールログ出力 (g単位の少数フォーマット)
                int32_t ax_mg = ((int32_t)ax * 1000) / 16384;
                int32_t ay_mg = ((int32_t)ay * 1000) / 16384;
                int32_t az_mg = ((int32_t)az * 1000) / 16384;

                int32_t ax_g_abs = ax_mg < 0 ? -ax_mg : ax_mg;
                int32_t ay_g_abs = ay_mg < 0 ? -ay_mg : ay_mg;
                int32_t az_g_abs = az_mg < 0 ? -az_mg : az_mg;

                tm_printf((UB *)"104Hz:%c%d.%03d,%c%d.%03d,%c%d.%03d\n",
                          ax_mg < 0 ? '-' : '+', (int)(ax_g_abs / 1000), (int)(ax_g_abs % 1000),
                          ay_mg < 0 ? '-' : '+', (int)(ay_g_abs / 1000), (int)(ay_g_abs % 1000),
                          az_mg < 0 ? '-' : '+', (int)(az_g_abs / 1000), (int)(az_g_abs % 1000));
                
                // 履歴バッファの更新 (ディスパッチ禁止でアトミックに実行)
                tk_dis_dsp();
                memmove((void*)&g_accel_x_history[0], (const void*)&g_accel_x_history[1], (HISTORY_LEN - 1) * sizeof(int16_t));
                memmove((void*)&g_accel_y_history[0], (const void*)&g_accel_y_history[1], (HISTORY_LEN - 1) * sizeof(int16_t));
                memmove((void*)&g_accel_z_history[0], (const void*)&g_accel_z_history[1], (HISTORY_LEN - 1) * sizeof(int16_t));
                g_accel_x_history[HISTORY_LEN - 1] = ax;
                g_accel_y_history[HISTORY_LEN - 1] = ay;
                g_accel_z_history[HISTORY_LEN - 1] = az;
                tk_ena_dsp();
            }
        }
        
        // 13サイクル周期パターンにより平均9.615ms（＝正確に104.0Hz）の周期を実現
        // (10ms * 8回 + 9ms * 5回) = 125ms / 13サイクル = 9.6153ms 平均
        int delay_ms = 10;
        if (delay_cnt == 1 || delay_cnt == 4 || delay_cnt == 6 || delay_cnt == 9 || delay_cnt == 12)
        {
            delay_ms = 9;
        }
        delay_cnt = (delay_cnt + 1) % 13;
        
        tk_dly_tsk(delay_ms);
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

    tk_slp_tsk(TMO_FEVR);
    return 0;
}
