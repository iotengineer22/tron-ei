/* generated HAL source file - do not edit */
#include "hal_data.h"

dmac_instance_ctrl_t g_transfer1_ctrl;
transfer_info_t g_transfer1_info =
{ .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
  .transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
  .transfer_settings_word_b.irq = TRANSFER_IRQ_END,
  .transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
  .transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
  .transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE,
  .transfer_settings_word_b.mode = TRANSFER_MODE_BLOCK,
  .p_dest = (void*) NULL,
  .p_src = (void const*) NULL,
  .num_blocks = 0,
  .length = 0, };
const dmac_extended_cfg_t g_transfer1_extend =
{ .offset = 1, .src_buffer_size = 1,
#if defined(VECTOR_NUMBER_DMAC1_INT)
    .irq                 = VECTOR_NUMBER_DMAC1_INT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif
  .ipl = (12),
  .channel = 1, .p_callback = pdm_rxi_dmac_isr, .p_context = &g_pdm0_ctrl, .activation_source = ELC_EVENT_PDM_DAT2, };
const transfer_cfg_t g_transfer1_cfg =
{ .p_info = &g_transfer1_info, .p_extend = &g_transfer1_extend, };
/* Instance structure to use this module. */
const transfer_instance_t g_transfer1 =
{ .p_ctrl = &g_transfer1_ctrl, .p_cfg = &g_transfer1_cfg, .p_api = &g_transfer_on_dmac };
pdm_instance_ctrl_t g_pdm0_ctrl;

/** PDM instance configuration */
const pdm_extended_cfg_t g_pdm0_cfg_extend =
{ .clock_div = PDM_CLOCK_DIV_2,

  /** Function Settings. */
  .short_circuit_detection_enable = PDM_SHORT_CIRCUIT_ENABLED,
  .over_voltage_lower_limit_detection_enable = PDM_OVERVOLTAGE_LOWER_LIMIT_ENABLED,
  .over_voltage_upper_limit_detection_enable = PDM_OVERVOLTAGE_UPPER_LIMIT_ENABLED,
  .buffer_overwrite_detection_enable = PDM_BUFFER_OVERWRITE_DETECTION_ENABLED,

  /** Filter Settings. */
  .moving_average_mode = PDM_MOVING_AVERAGE_MODE_1_ORDER,
  .low_pass_filter_shift = PDM_LPF_RIGHT_SHIFT_0,
  .compensation_filter_shift = PDM_COMPENSATION_FILTER_RIGHT_SHIFT_0,
  .high_pass_filter_shift = PDM_HPF_RIGHT_SHIFT_0,
  .sinc_filter_mode = PDM_SINC_FILTER_MODE_4,
  .sincrng = PDM2_CALCULATED_SINCRNG_VALUE,
  .sincdec = PDM2_CALCULATED_SINCDEC_VALUE,
  .hpf_coefficient_s0 = 0x3F61,
  .hpf_coefficient_k1 = 0x3EC1,
  .hpf_coefficient_h =
  { 0x4000, 0xC000 },
  .compensation_filter_coefficient_h =
  { 0x1FE8, 0x0039, 0x003C, 0x1E56, 0x01DC, 0x06E1, 0x01DC, 0x1E56, 0x003C, 0x0039, 0x1FE8 },
  .lpf_coefficient_h0 = 0x0400,
  .lpf_coefficient_h1 =
  { 0x1FF8,
    0x000A,
    0x1FF0,
    0x0018,
    0x1FDC,
    0x0034,
    0x1FB3,
    0x0076,
    0x1F2E,
    0x0289,
    0x0289,
    0x1F2E,
    0x0076,
    0x1FB3,
    0x0034,
    0x1FDC,
    0x0018,
    0x1FF0,
    0x000A,
    0x1FF8 },

  /** Data Threshold. */
  .interrupt_threshold = PDM_INTERRUPT_THRESHOLD_16,

  /** Short-Circuit Detection. */
  .short_circuit_count_h = 0x1FFF,
  .short_circuit_count_l = 0x1FFF,

  /** Overvoltage Detection. */
  .overvoltage_detection_lower_limit = 0x85EE0,
  .overvoltage_detection_upper_limit = 0x7A120,

};

/** PDM interface configuration */
const pdm_cfg_t g_pdm0_cfg =
{ .unit = 0, .channel = 2, .pcm_width = PDM_PCM_WIDTH_20_BITS_0_18, .pcm_edge = PDM_INPUT_DATA_EDGE_RISE,

#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == g_transfer1)
                .p_transfer_rx                         = NULL,
#else
  .p_transfer_rx = &g_transfer1,
#endif
#undef RA_NOT_DEFINED
  .p_callback = pdm_callback,
  .p_context = NULL, .p_extend = &g_pdm0_cfg_extend,

#if defined(VECTOR_NUMBER_PDM_SDET)
                .sdet_irq                              = PDM_SDET_IRQn,
#else
  .sdet_irq = FSP_INVALID_VECTOR,
#endif
  .sdet_ipl = (12),

#if defined(VECTOR_NUMBER_PDM_DAT2)
                .dat_irq                               = PDM_DAT2_IRQn,
#else
  .dat_irq = FSP_INVALID_VECTOR,
#endif
  .dat_ipl = (BSP_IRQ_DISABLED),

#if defined(VECTOR_NUMBER_PDM_ERR2)
                .err_irq                               = PDM_ERR2_IRQn,
#else
  .err_irq = FSP_INVALID_VECTOR,
#endif
  .err_ipl = (12), };

/* Instance structure to use this module. */
const pdm_instance_t g_pdm0 =
{ .p_ctrl = &g_pdm0_ctrl, .p_cfg = &g_pdm0_cfg, .p_api = &g_pdm_on_pdm };
gpt_instance_ctrl_t g_cam_clk_ctrl;
#if 0
const gpt_extended_pwm_cfg_t g_cam_clk_pwm_extend =
{
    .trough_ipl             = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT12_COUNTER_UNDERFLOW)
    .trough_irq             = VECTOR_NUMBER_GPT12_COUNTER_UNDERFLOW,
#else
    .trough_irq             = FSP_INVALID_VECTOR,
#endif
    .poeg_link              = GPT_POEG_LINK_POEG0,
    .output_disable         = (gpt_output_disable_t) ( GPT_OUTPUT_DISABLE_NONE),
    .adc_trigger            = (gpt_adc_trigger_t) ( GPT_ADC_TRIGGER_NONE),
    .dead_time_count_up     = 0,
    .dead_time_count_down   = 0,
    .adc_a_compare_match    = 0,
    .adc_b_compare_match    = 0,
    .interrupt_skip_source  = GPT_INTERRUPT_SKIP_SOURCE_NONE,
    .interrupt_skip_count   = GPT_INTERRUPT_SKIP_COUNT_0,
    .interrupt_skip_adc     = GPT_INTERRUPT_SKIP_ADC_NONE,
    .gtioca_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
    .gtiocb_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
};
#endif
const gpt_extended_cfg_t g_cam_clk_extend =
        { .gtioca =
        { .output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW },
          .gtiocb =
          { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
          .start_source = (gpt_source_t) (GPT_SOURCE_NONE), .stop_source = (gpt_source_t) (GPT_SOURCE_NONE), .clear_source =
                  (gpt_source_t) (GPT_SOURCE_NONE),
          .count_up_source = (gpt_source_t) (GPT_SOURCE_NONE), .count_down_source = (gpt_source_t) (GPT_SOURCE_NONE), .capture_a_source =
                  (gpt_source_t) (GPT_SOURCE_NONE),
          .capture_b_source = (gpt_source_t) (GPT_SOURCE_NONE), .capture_a_ipl = (BSP_IRQ_DISABLED), .capture_b_ipl =
                  (BSP_IRQ_DISABLED),
          .compare_match_c_ipl = (BSP_IRQ_DISABLED), .compare_match_d_ipl = (BSP_IRQ_DISABLED), .compare_match_e_ipl =
                  (BSP_IRQ_DISABLED),
          .compare_match_f_ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT12_CAPTURE_COMPARE_A)
    .capture_a_irq         = VECTOR_NUMBER_GPT12_CAPTURE_COMPARE_A,
#else
          .capture_a_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT12_CAPTURE_COMPARE_B)
    .capture_b_irq         = VECTOR_NUMBER_GPT12_CAPTURE_COMPARE_B,
#else
          .capture_b_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT12_COMPARE_C)
    .compare_match_c_irq   = VECTOR_NUMBER_GPT12_COMPARE_C,
#else
          .compare_match_c_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT12_COMPARE_D)
    .compare_match_d_irq   = VECTOR_NUMBER_GPT12_COMPARE_D,
#else
          .compare_match_d_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT12_COMPARE_E)
    .compare_match_e_irq   = VECTOR_NUMBER_GPT12_COMPARE_E,
#else
          .compare_match_e_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT12_COMPARE_F)
    .compare_match_f_irq   = VECTOR_NUMBER_GPT12_COMPARE_F,
#else
          .compare_match_f_irq = FSP_INVALID_VECTOR,
#endif
          .compare_match_value =
          { (uint32_t) 0x0, /* CMP_A */
            (uint32_t) 0x0, /* CMP_B */
            (uint32_t) 0x0, /* CMP_C */
            (uint32_t) 0x0, /* CMP_D */
            (uint32_t) 0x0, /* CMP_E */
            (uint32_t) 0x0, /* CMP_F */},
          .compare_match_status = ((0U << 5U) | (0U << 4U) | (0U << 3U) | (0U << 2U) | (0U << 1U) | 0U), .capture_filter_gtioca =
                  GPT_CAPTURE_FILTER_NONE,
          .capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,
#if 0
    .p_pwm_cfg             = &g_cam_clk_pwm_extend,
#else
          .p_pwm_cfg = NULL,
#endif
#if 0
    .gtior_setting.gtior_b.gtioa  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.oadflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.oahld  = 0U,
    .gtior_setting.gtior_b.oae    = (uint32_t) true,
    .gtior_setting.gtior_b.oadf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfaen  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsa  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
    .gtior_setting.gtior_b.gtiob  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.obdflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.obhld  = 0U,
    .gtior_setting.gtior_b.obe    = (uint32_t) false,
    .gtior_setting.gtior_b.obdf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfben  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsb  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
#else
          .gtior_setting.gtior = 0U,
#endif

          .gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,
          .gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL, };

const timer_cfg_t g_cam_clk_cfg =
{ .mode = TIMER_MODE_PERIODIC,
/* Actual period: 8.333333333333334e-8 seconds. Actual duty: 50%. */.period_counts = (uint32_t) 0xa,
  .duty_cycle_counts = 0x5, .source_div = (timer_source_div_t) 0, .channel = 12, .p_callback = NULL,
  /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
  .p_context = (void*) &NULL,
#endif
  .p_extend = &g_cam_clk_extend,
  .cycle_end_ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT12_COUNTER_OVERFLOW)
    .cycle_end_irq       = VECTOR_NUMBER_GPT12_COUNTER_OVERFLOW,
#else
  .cycle_end_irq = FSP_INVALID_VECTOR,
#endif
        };
/* Instance structure to use this module. */
const timer_instance_t g_cam_clk =
{ .p_ctrl = &g_cam_clk_ctrl, .p_cfg = &g_cam_clk_cfg, .p_api = &g_timer_on_gpt };
iic_master_instance_ctrl_t g_cam_i2c_master_ctrl;
const iic_master_extended_cfg_t g_cam_i2c_master_extend =
{ .timeout_mode = IIC_MASTER_TIMEOUT_MODE_SHORT,
  .timeout_scl_low = IIC_MASTER_TIMEOUT_SCL_LOW_ENABLED,
  .smbus_operation = 0,
  /* Actual calculated bitrate: 393082. Actual calculated duty cycle: 50%. */.clock_settings.brl_value = 15,
  .clock_settings.brh_value = 15,
  .clock_settings.cks_value = 2,
  .clock_settings.sddl_value = 0,
  .clock_settings.dlcs_value = 0, };
const i2c_master_cfg_t g_cam_i2c_master_cfg =
{ .channel = 1, .rate = I2C_MASTER_RATE_FAST, .slave = 0x00, .addr_mode = I2C_MASTER_ADDR_MODE_7BIT,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_tx = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_rx = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
  .p_callback = g_cam_i2c_master_user_callback,
  .p_context = NULL,
#if defined(VECTOR_NUMBER_IIC1_RXI)
    .rxi_irq             = VECTOR_NUMBER_IIC1_RXI,
#else
  .rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TXI)
    .txi_irq             = VECTOR_NUMBER_IIC1_TXI,
#else
  .txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TEI)
    .tei_irq             = VECTOR_NUMBER_IIC1_TEI,
#else
  .tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_ERI)
    .eri_irq             = VECTOR_NUMBER_IIC1_ERI,
#else
  .eri_irq = FSP_INVALID_VECTOR,
#endif
  .ipl = (8),
  .p_extend = &g_cam_i2c_master_extend, };
/* Instance structure to use this module. */
const i2c_master_instance_t g_cam_i2c_master =
{ .p_ctrl = &g_cam_i2c_master_ctrl, .p_cfg = &g_cam_i2c_master_cfg, .p_api = &g_i2c_master_on_iic };
void g_hal_init(void)
{
    g_common_init ();
}
