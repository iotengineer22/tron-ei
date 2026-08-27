/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_dmac.h"
#include "r_transfer_api.h"
#include "r_pdm_api.h"
#include "r_pdm.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
FSP_HEADER
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer1;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_transfer1_ctrl;
extern const transfer_cfg_t g_transfer1_cfg;

#ifndef pdm_rxi_dmac_isr
void pdm_rxi_dmac_isr(transfer_callback_args_t *p_args);
#endif
/* Sinc Decimation ratio has been rounded to the nearest integer.
 * Target Sampling Frequency: 32000 Hz
 * Actual Sampling Frequency: 32258 Hz */
#define PDM2_CALCULATED_SINCRNG_VALUE (9)
#define PDM2_CALCULATED_SINCDEC_VALUE (62)
#define PDM2_FILTER_SETTLING_TIME_US  (841)

/** PDM Instance. */
extern const pdm_instance_t g_pdm0;

/** Access the PDM instance using these structures when calling API functions directly (::p_api is not used). */
extern pdm_instance_ctrl_t g_pdm0_ctrl;
extern const pdm_cfg_t g_pdm0_cfg;

#ifndef pdm_callback
void pdm_callback(pdm_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_cam_clk;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_cam_clk_ctrl;
extern const timer_cfg_t g_cam_clk_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_cam_i2c_master;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_cam_i2c_master_ctrl;
extern const i2c_master_cfg_t g_cam_i2c_master_cfg;

#ifndef g_cam_i2c_master_user_callback
void g_cam_i2c_master_user_callback(i2c_master_callback_args_t *p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
