/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <mach/gpiomux.h>

#include "msm_sensor.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#include "msm_camera_i2c_mux.h"

#include <misc/app_info.h>

//#define CONFIG_MSMB_CAMERA_DEBUG


#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#define HW_MT9V113_SENSOR_NAME "hw_mt9v113"
DEFINE_MSM_MUTEX(hw_mt9v113_mut);

static struct msm_sensor_ctrl_t hw_mt9v113_s_ctrl;

static int8_t hw_mt9v113_module_id = 0;
static struct msm_sensor_power_setting hw_mt9v113_power_setting[] = {

	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 3,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 2,
	},
};
static struct msm_sensor_power_setting hw_mt9v113_power_down_setting[] = {
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 20,
	},
};
static struct msm_camera_i2c_reg_conf hw_mt9v113_start_settings[] = {
	{ 0x301A, 0x121C},
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_stop_settings[] = {
	{0x301A, 0x1218},
};

/*modify the initialization settings to avoid exceptional output*/
static struct msm_camera_i2c_reg_conf hw_mt9v113_recommend_settings_1[] =
{
	{0x001A, 0x0011}, 
	{0x001A, 0x0018}, 
	{0x0014, 0x2145}, 
	{0x0014, 0x2145}, 
	{0x0010, 0x0631}, 
	{0x0012, 0x0000}, 
	{0x0014, 0x244B}, 
};


static struct msm_camera_i2c_reg_conf hw_mt9v113_recommend_settings_2[] =
{
	{0x0014, 0x304B},
};
static struct msm_camera_i2c_reg_conf hw_mt9v113_recommend_settings_3[] =
{
	{0x0014, 0xB04A}, 
	{0x0018, 0x402C}, 
};
static struct msm_camera_i2c_reg_conf hw_mt9v113_recommend_settings_4[] =
{
    {0x3400, 0x7A38}, 
    {0x321C, 0x0003}, 
    {0x098C, 0x02F0}, // MCU_ADDRESS
    {0x0990, 0x0000}, // MCU_DATA_0
    {0x098C, 0x02F2}, // MCU_ADDRESS
    {0x0990, 0x0210}, // MCU_DATA_0
    {0x098C, 0x02F4}, // MCU_ADDRESS
    {0x0990, 0x001A}, // MCU_DATA_0
    {0x098C, 0x2145}, // MCU_ADDRESS [SEQ_ADVSEQ_CALLLIST_5]
    {0x0990, 0x02F4}, // MCU_DATA_0
    {0x098C, 0xA134}, // MCU_ADDRESS [SEQ_ADVSEQ_STACKOPTIONS]
    {0x0990, 0x0001}, // MCU_DATA_0
    {0x31E0, 0x0001}, // PIX_DEF_ID
    {0x098C, 0x2703}, //Output Width (A)
    {0x0990, 0x0280}, //=640
    {0x098C, 0x2705}, //Output Height (A)
    {0x0990, 0x01E0}, //=480
    {0x098C, 0x2707}, //Output Width (B)
    {0x0990, 0x0280}, //=640
    {0x098C, 0x2709}, //Output Height (B)
    {0x0990, 0x01E0}, //=480
    {0x098C, 0x270D}, //Row Start (A)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x270F}, //Column Start (A)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x2711}, //Row End (A)
    {0x0990, 0x01E7}, //=487
    {0x098C, 0x2713}, //Column End (A)
    {0x0990, 0x0287}, //=647
    {0x098C, 0x2715}, //Row Speed (A)
    {0x0990, 0x0001}, //=1
    {0x098C, 0x2717}, //Read Mode (A)
    {0x0990, 0x0026}, //=25 
    {0x098C, 0x2719}, //sensor_fine_correction (A)
    {0x0990, 0x001A}, //=26
    {0x098C, 0x271B}, //sensor_fine_IT_min (A)
    {0x0990, 0x006B}, //=107
    {0x098C, 0x271D}, //sensor_fine_IT_max_margin (A)
    {0x0990, 0x006B}, //=107
    {0x098C, 0x271F}, //Frame Lines (A)
    {0x0990, 0x032A}, // MCU_DATA_0
    {0x098C, 0x2721}, //Line Length (A)
    {0x0990, 0x0364}, //=868
    {0x098C, 0x2723}, //Row Start (B)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x2725}, //Column Start (B)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x2727}, //Row End (B)
    {0x0990, 0x01E7}, //=487
    {0x098C, 0x2729}, //Column End (B)
    {0x0990, 0x0287 }, //=647
    {0x098C, 0x272B}, //Row Speed (B)
    {0x0990, 0x0001}, //=1
    {0x098C, 0x272D}, //Read Mode (B)
    {0x0990, 0x0026}, //=25 
    {0x098C, 0x272F}, //sensor_fine_correction (B)
    {0x0990, 0x001A}, //=26
    {0x098C, 0x2731}, //sensor_fine_IT_min (B)
    {0x0990, 0x006B}, //=107
    {0x098C, 0x2733}, //sensor_fine_IT_max_margin (B)
    {0x0990, 0x006B}, //=107
    {0x098C, 0x2735}, //Frame Lines (B)
    {0x0990, 0x0426}, //=1062
    {0x098C, 0x2737}, //Line Length (B)
    {0x0990, 0x0363}, //=867
    {0x098C, 0x2739}, //Crop_X0 (A)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x273B}, //Crop_X1 (A)
    {0x0990, 0x027F}, //=639
    {0x098C, 0x273D}, //Crop_Y0 (A)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x273F}, //Crop_Y1 (A)
    {0x0990, 0x01DF}, //=479
    {0x098C, 0x2747}, //Crop_X0 (B)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x2749}, //Crop_X1 (B)
    {0x0990, 0x027F}, //=639
    {0x098C, 0x274B}, //Crop_Y0 (B)
    {0x0990, 0x0000}, //=0
    {0x098C, 0x274D}, //Crop_Y1 (B)
    {0x0990, 0x01DF}, //=479
    {0x098C, 0x222D}, //R9 Step
    {0x0990, 0x0088}, //=136
    {0x098C, 0xA408}, //search_f1_50
    {0x0990, 0x0020}, //=32
    {0x098C, 0xA409}, //search_f2_50
    {0x0990, 0x0023}, //=35
    {0x098C, 0xA40A}, //search_f1_60
    {0x0990, 0x0027}, //=39
    {0x098C, 0xA40B}, //search_f2_60
    {0x0990, 0x002A}, //=42
    {0x098C, 0x2411}, //R9_Step_60 (A)
    {0x0990, 0x0078}, //=120
    {0x098C, 0x2413}, //R9_Step_50 (A)
    {0x0990, 0x0090}, //=144
    {0x098C, 0x2415}, //R9_Step_60 (B)
    {0x0990, 0x0078}, //=120
    {0x098C, 0x2417}, //R9_Step_50 (B)
    {0x0990, 0x0090}, //=144
    {0x098C, 0xA404}, //FD Mode
    {0x0990, 0x0010}, //=16
    {0x098C, 0xA40D}, //Stat_min
    {0x0990, 0x0002}, //=2
    {0x098C, 0xA40E}, //Stat_max
    {0x0990, 0x0003}, //=3
    {0x098C, 0xA410}, //Min_amplitude
    {0x0990, 0x000A}, //=10

    {0x364E, 0x0150}, 
    {0x3650, 0x2F0A}, 
    {0x3652, 0x4332}, 
    {0x3654, 0xDBAE}, 
    {0x3656, 0x6093}, 
    {0x3658, 0x0150}, 
    {0x365A, 0x028A}, 
    {0x365C, 0x3E52}, 
    {0x365E, 0x8BAF}, 
    {0x3660, 0x3394}, 
    {0x3662, 0x00D0}, 
    {0x3664, 0x588B}, 
    {0x3666, 0x39F2}, 
    {0x3668, 0x87D0}, 
    {0x366A, 0x37F3}, 
    {0x366C, 0x0170}, 
    {0x366E, 0x0BE9}, 
    {0x3670, 0x4472}, 
    {0x3672, 0xF14E}, 
    {0x3674, 0x0FF4}, 
    {0x3676, 0x90AE}, 
    {0x3678, 0x878E}, 
    {0x367A, 0x0672}, 
    {0x367C, 0x93D0}, 
    {0x367E, 0x9C35}, 
    {0x3680, 0x892E}, 
    {0x3682, 0xB40E}, 
    {0x3684, 0x6631}, 
    {0x3686, 0xBA10}, 
    {0x3688, 0x82F5}, 
    {0x368A, 0xF26C}, 
    {0x368C, 0xE66E}, 
    {0x368E, 0x33B2}, 
    {0x3690, 0x2572}, 
    {0x3692, 0xB4D5}, 
    {0x3694, 0xFBED}, 
    {0x3696, 0xB04E}, 
    {0x3698, 0x21D2}, 
    {0x369A, 0x7FAE}, 
    {0x369C, 0xAFF5}, 
    {0x369E, 0x6CF2}, 
    {0x36A0, 0x0DAF}, 
    {0x36A2, 0x4676}, 
    {0x36A4, 0x2513}, 
    {0x36A6, 0x84FA}, 
    {0x36A8, 0x6132}, 
    {0x36AA, 0xF2CD}, 
    {0x36AC, 0x2D56}, 
    {0x36AE, 0x5F53}, 
    {0x36B0, 0x8BB9}, 
    {0x36B2, 0x3172}, 
    {0x36B4, 0x210E}, 
    {0x36B6, 0x4696}, 
    {0x36B8, 0x4F72}, 
    {0x36BA, 0x89BA}, 
    {0x36BC, 0x6552}, 
    {0x36BE, 0x6B2A}, 
    {0x36C0, 0x4516}, 
    {0x36C2, 0x0714}, 
    {0x36C4, 0x8AFA}, 
    {0x36C6, 0x7571}, 
    {0x36C8, 0xB392}, 
    {0x36CA, 0x9316}, 
    {0x36CC, 0x26B6}, 
    {0x36CE, 0x5E58}, 
    {0x36D0, 0x61F0}, 
    {0x36D2, 0xA4D2}, 
    {0x36D4, 0xEDB5}, 
    {0x36D6, 0x4C76}, 
    {0x36D8, 0x7AD8}, 
    {0x36DA, 0x1AD1}, 
    {0x36DC, 0x0A32}, 
    {0x36DE, 0xC696}, 
    {0x36E0, 0xA816}, 
    {0x36E2, 0x07B9}, 
    {0x36E4, 0x0192}, 
    {0x36E6, 0xCEF1}, 
    {0x36E8, 0xD116}, 
    {0x36EA, 0x7BD3}, 
    {0x36EC, 0x4659}, 
    {0x36EE, 0x1114}, 
    {0x36F0, 0x9112}, 
    {0x36F2, 0xE7FA}, 
    {0x36F4, 0x5014}, 
    {0x36F6, 0x553E}, 
    {0x36F8, 0x4554}, 
    {0x36FA, 0x1733}, 
    {0x36FC, 0x8DDA}, 
    {0x36FE, 0xE156}, 
    {0x3700, 0x14DE}, 
    {0x3702, 0x2154}, 
    {0x3704, 0xFA93}, 
    {0x3706, 0xC83A}, 
    {0x3708, 0x7E56}, 
    {0x370A, 0x363E}, 
    {0x370C, 0x1114}, 
    {0x370E, 0xD80F}, 
    {0x3710, 0xDD3A}, 
    {0x3712, 0x52B5}, 
    {0x3714, 0x501E}, 
    {0x3644, 0x0130}, 
    {0x3642, 0x00DC}, 
    {0x3210, 0x09B8}, 
};
static struct msm_camera_i2c_reg_conf hw_mt9v113_recommend_settings_5[] =
{
	{0x098C, 0xA20C}, 
	{0x0990, 0x0010}, 
	{0x098C, 0xA215}, 
	{0x0990, 0x0010}, 
	{0x098C, 0x2212}, 
	{0x0990, 0x0180}, 
	{0x098C, 0xA24F}, 
	{0x0990, 0x003E}, 

	{0x098C, 0x2306}, 
	{0x0990, 0x0315}, 
	{0x098C, 0x2308}, 
	{0x0990, 0xFDDC}, 
	{0x098C, 0x230A}, 
	{0x0990, 0x003A}, 
	{0x098C, 0x230C}, 
	{0x0990, 0xFF58}, 
	{0x098C, 0x230E}, 
	{0x0990, 0x02B7}, 
	{0x098C, 0x2310}, 
	{0x0990, 0xFF31}, 
	{0x098C, 0x2312}, 
	{0x0990, 0xFF4C}, 
	{0x098C, 0x2314}, 
	{0x0990, 0xFE4C}, 
	{0x098C, 0x2316}, 
	{0x0990, 0x039E}, 
	{0x098C, 0x2318}, 
	{0x0990, 0x001C}, 
	{0x098C, 0x231A}, 
	{0x0990, 0x0039}, 
	{0x098C, 0x231C}, 
	{0x0990, 0x007F}, 
	{0x098C, 0x231E}, 
	{0x0990, 0xFF77}, 
	{0x098C, 0x2320}, 
	{0x0990, 0x000A}, 
	{0x098C, 0x2322}, 
	{0x0990, 0x0020}, 
	{0x098C, 0x2324}, 
	{0x0990, 0x001B}, 
	{0x098C, 0x2326}, 
	{0x0990, 0xFFC6}, 
	{0x098C, 0x2328}, 
	{0x0990, 0x0086}, 
	{0x098C, 0x232A}, 
	{0x0990, 0x00B5}, 
	{0x098C, 0x232C}, 
	{0x0990, 0xFEC3}, 
	{0x098C, 0x232E}, 
	{0x0990, 0x0001}, 
	{0x098C, 0x2330}, 
	{0x0990, 0xFFEF}, 
	{0x098C, 0xA366}, 
	{0x0990, 0x0080}, 
	{0x098C, 0xA367}, 
	{0x0990, 0x0080}, 
	{0x098C, 0xA368}, 
	{0x0990, 0x008a}, 
	{0x098C, 0xA369}, 
	{0x0990, 0x0080}, 
	{0x098C, 0xA36A}, 
	{0x0990, 0x0080}, 
	{0x098C, 0xA36B}, 
	{0x0990, 0x008a}, 
	{0x098C, 0xA348}, 
	{0x0990, 0x0008}, 
	{0x098C, 0xA349}, 
	{0x0990, 0x0002}, 
	{0x098C, 0xA34A}, 
	{0x0990, 0x0090}, 
	{0x098C, 0xA34B}, 
	{0x0990, 0x00FF}, 
	{0x098C, 0xA34C}, 
	{0x0990, 0x0075}, 
	{0x098C, 0xA34D}, 
	{0x0990, 0x00EF}, 
	{0x098C, 0xA351}, 
	{0x0990, 0x0000}, 
	{0x098C, 0xA352}, 
	{0x0990, 0x007F}, 
	{0x098C, 0xA354}, 
	{0x0990, 0x0043}, 
	{0x098C, 0xA355}, 
	{0x0990, 0x0001}, 
	{0x098C, 0xA35D}, 
	{0x0990, 0x0078}, 
	{0x098C, 0xA35E}, 
	{0x0990, 0x0086}, 
	{0x098C, 0xA35F}, 
	{0x0990, 0x007E}, 
	{0x098C, 0xA360}, 
	{0x0990, 0x0082}, 
	{0x098C, 0x2361}, 
	{0x0990, 0x0040}, 
	{0x098C, 0xA363}, 
	{0x0990, 0x00D2}, 
	{0x098C, 0xA364}, 
	{0x0990, 0x00F6}, 
	{0x098C, 0xA302}, 
	{0x0990, 0x0000}, 
	{0x098C, 0xA303}, 
	{0x0990, 0x00EF}, 
	{0x098C, 0x274F}, 
	{0x0990, 0x0004}, 
	{0x098C, 0x2741}, 
	{0x0990, 0x0004}, 
	{0x098C, 0xAB1F}, 
	{0x0990, 0x00C7}, 
	{0x098C, 0xAB31}, 
	{0x0990, 0x001E}, 
	{0x098C, 0xAB20}, 
	{0x0990, 0x0058}, 
	{0x098C, 0xAB21}, 
	{0x0990, 0x0046}, 
	{0x098C, 0xAB22}, 
	{0x0990, 0x0002}, 
	{0x098C, 0xAB24}, 
	{0x0990, 0x0024}, //low light saturation
	{0x098C, 0x2B28}, 
	{0x0990, 0x150C}, 
	{0x098C, 0x2B2A}, 
	{0x0990, 0x1F80}, 
	/*outdoor issue*/
	{0x098C, 0xA202}, // MCU_ADDRESS [AE_WINDOW_POS]
	{0x0990, 0x0032}, // MCU_DATA_0
	{0x098C, 0xA203}, // MCU_ADDRESS [AE_WINDOW_SIZE]
	{0x0990, 0x00AB}, // MCU_DATA_0
	/*outdoor issue*/
	{0x098C, 0xA36D}, // MCU_ADDRESS [AWB MIN]
	{0x0990, 0x0004}, // MCU_DATA_0
	{0x0018, 0x0028},
	//{0x3012, 0x0128},
};
static struct msm_camera_i2c_reg_conf hw_mt9v113_recommend_settings_6[] =
{
	{0x098C, 0xA103}, 
	{0x0990, 0x0006}, //05
};

static struct msm_camera_i2c_conf_array hw_mt9v113_init_conf[] = {
	{&hw_mt9v113_recommend_settings_1[0],
	ARRAY_SIZE(hw_mt9v113_recommend_settings_1), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&hw_mt9v113_recommend_settings_2[0],
	ARRAY_SIZE(hw_mt9v113_recommend_settings_2), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&hw_mt9v113_recommend_settings_3[0],
	ARRAY_SIZE(hw_mt9v113_recommend_settings_3), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&hw_mt9v113_recommend_settings_4[0],
	ARRAY_SIZE(hw_mt9v113_recommend_settings_4), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&hw_mt9v113_recommend_settings_5[0],
	ARRAY_SIZE(hw_mt9v113_recommend_settings_5), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&hw_mt9v113_recommend_settings_6[0],
	ARRAY_SIZE(hw_mt9v113_recommend_settings_6), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct v4l2_subdev_info hw_mt9v113_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order  = 0,
	},
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_saturation[11][2] = {
	{
//Saturation x0.25
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0000},//MCU_DATA_0
	},
	{
//Saturation x0.5
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0015},//MCU_DATA_0
	},
	{
//Saturation x0.75
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0025},//MCU_DATA_0
	},
	{
//Saturation x0.75
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0035},//MCU_DATA_0
	},
	{
//Saturation x1 (Default)
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0045},//MCU_DATA_0
	},
	{
//Saturation x1.25
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0050},//MCU_DATA_0
	},
	{
//Saturation x1.5
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0060},//MCU_DATA_0
	},
	{
//Saturation x1.25
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0070},//MCU_DATA_0
	},
	{
//Saturation x1.5
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x007f},//MCU_DATA_0
	},
	{
//Saturation x1.75
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x008f},//MCU_DATA_0
	},
	{
//Saturation x2.0
  {0x098C,0xAB20},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x009f},//MCU_DATA_0
	},
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_contrast[11][38] = {
	{
//Contrast -5
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x006F},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0080},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0092},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x00A8},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x00B6},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x00C1},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00CA},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00D2},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00D8},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00DE},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00E3},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00E8},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00ED},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00F1},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00F5},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F8},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FC},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0		
  },
	{
//Contrast -4
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x006F},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0080},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0092},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x00A8},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x00B6},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x00C1},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00CA},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00D2},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00D8},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00DE},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00E3},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00E8},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00ED},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00F1},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00F5},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F8},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FC},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast -3
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0038},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0060},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0079},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0094},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x00A6},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x00B3},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00BD},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00C7},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00CF},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00D6},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00DC},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00E2},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00E8},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00ED},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00F2},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F7},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FB},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast -2
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x002C},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x004F},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0069},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0085},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0098},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x00A6},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00B2},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00BD},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00C6},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00CE},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00D6},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00DD},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00E4},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00EA},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00EF},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F5},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FA},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast -1
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x001B},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0035},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x004E},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x006B},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0080},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0090},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x009E},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00AB},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00B6},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00C0},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00CA},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00D3},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00DB},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00E3},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00EA},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F2},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F8},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast (Default)
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0007},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0016},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0039},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x005F},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x007A},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x008F},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00A1},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00AF},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00BB},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00C6},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00CF},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00D8},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00E0},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00E7},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00EE},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F4},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FA},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast 1
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x000F},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0027},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0048},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0062},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0078},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x008C},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x009D},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00AB},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00B8},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00C4},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00CF},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00D8},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00E1},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E9},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F1},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F8},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast 2
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0003},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x000A},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x001B},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0038},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x004F},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0064},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x0078},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x008B},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x009C},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00AB},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00B9},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00C5},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00D1},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00DB},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E5},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00EE},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F7},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast 3
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0002},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0007},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0015},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x002D},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0043},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0058},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x006D},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x0083},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x0097},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00A8},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00B7},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00C5},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00D1},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00DC},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E5},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00EF},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F7},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast 4
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0002},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0004},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x000E},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0023},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0038},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x004C},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x0061},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x0078},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x008E},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00A2},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00B3},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00C2},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00CF},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00DA},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E5},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00EE},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F7},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0
	},
	{
//Contrast 5
  {0x098C,0xAB3C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xAB3D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0002},//MCU_DATA_0
  {0x098C,0xAB3E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0004},//MCU_DATA_0
  {0x098C,0xAB3F},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x000E},//MCU_DATA_0
  {0x098C,0xAB40},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0023},//MCU_DATA_0
  {0x098C,0xAB41},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0038},//MCU_DATA_0
  {0x098C,0xAB42},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x004C},//MCU_DATA_0
  {0x098C,0xAB43},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x0061},//MCU_DATA_0
  {0x098C,0xAB44},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x0078},//MCU_DATA_0
  {0x098C,0xAB45},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x008E},//MCU_DATA_0
  {0x098C,0xAB46},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00A2},//MCU_DATA_0
  {0x098C,0xAB47},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00B3},//MCU_DATA_0
  {0x098C,0xAB48},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00C2},//MCU_DATA_0
  {0x098C,0xAB49},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00CF},//MCU_DATA_0
  {0x098C,0xAB4A},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00DA},//MCU_DATA_0
  {0x098C,0xAB4B},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E5},//MCU_DATA_0
  {0x098C,0xAB4C},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00EE},//MCU_DATA_0
  {0x098C,0xAB4D},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F7},//MCU_DATA_0
  {0x098C,0xAB4E},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF},//MCU_DATA_0		
	},
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_sharpness[7][2] = {
	{
  {0x098C,0xAB22},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0000},//MCU_DATA_0
	}, /* SHARPNESS LEVEL 0*/
	{
  {0x098C,0xAB22},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0001},//MCU_DATA_0
	}, /* SHARPNESS LEVEL 1*/
	{
  {0x098C,0xAB22},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0002},//MCU_DATA_0
	}, /* SHARPNESS LEVEL 2 Default*/
	{
  {0x098C,0xAB22},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0003},//MCU_DATA_0
	}, /* SHARPNESS LEVEL 3*/
	{
  {0x098C,0xAB22},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0004},//MCU_DATA_0
	}, /* SHARPNESS LEVEL 4*/
	{
  {0x098C,0xAB22},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0005},//MCU_DATA_0
	}, /* SHARPNESS LEVEL 5*/
	{
  {0x098C,0xAB22},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0006},//MCU_DATA_0
	}, /* SHARPNESS LEVEL 6*/
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_iso[7][10] = {
	/* auto */
	{
//ISO Auto
	{0x098C, 0xA20E},
	{0x0990, 0x0080},
	{0x098C, 0xA103},
	{0x0990, 0x0005},
	},
	/* auto hjt */
	{
	{0x098C, 0xA20E},
	{0x0990, 0x0080},
	{0x098C, 0xA103},
	{0x0990, 0x0005},
	},
	/* iso 100 */
	{
  //ISO 100
	{0x098C, 0xA20E},
	{0x0990, 0x0026},
	{0x098C, 0xA103},
	{0x0990, 0x0005},
	},
	/* iso 200 */
	{
  //ISO 200
	{0x098C, 0xA20E},
	{0x0990, 0x0046},
	{0x098C, 0xA103},
	{0x0990, 0x0005},
	},
	/* iso 400 */
	{
  //ISO 400
	{0x098C, 0xA20E},
	{0x0990, 0x0078},
	{0x098C, 0xA103},
	{0x0990, 0x0005},
	},
	/* iso 800 */
	{
  //ISO 800
	{0x098C, 0xA20E},
	{0x0990, 0x00A0},
	{0x098C, 0xA103},
	{0x0990, 0x0005},
	},
	/* iso 1600 */
	{
  //ISO 1600
	{0x098C, 0xA20E},
	{0x0990, 0x00C0},
	{0x098C, 0xA103},
	{0x0990, 0x0005},
	},
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_exposure_compensation[5][4] = {
	/* -2 */
	{
//Exposure Compensation 1.7EV
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
  {0x098C,0xA24F},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0018},//MCU_DATA_0
	},
	/* -1 */
	{
//Exposure Compensation 1.0EV
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
  {0x098C,0xA24F},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x002f},//MCU_DATA_0
	},
	/* 0 */
	{
//Exposure Compensation default
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
  {0x098C,0xA24F},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0044},//MCU_DATA_0
	},
	/* 1 */
	{
//Exposure Compensation -1.0EV
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
  {0x098C,0xA24F},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0060},//MCU_DATA_0
	},
	/* 2 */
	{
//Exposure Compensation -1.7EV
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
  {0x098C,0xA24F},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0080},//MCU_DATA_0
	},
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_antibanding[][6] = {
	/* OFF */
	{
//Auto-XCLK24MHz
  {0x098C,0xA404},//MCU_ADDRESS[FD_MODE]
  {0x0990,0x0050},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0006},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
	},
	/* 50Hz */
	{
//Band 50Hz
  {0x098C,0xA404},//MCU_ADDRESS[FD_MODE]
  {0x0990,0x00F0},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0006},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
	},
	/* 60Hz */
	{
//Band 60Hz
  {0x098C,0xA404},//MCU_ADDRESS[FD_MODE]
  {0x0990,0x00B0},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0006},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
	},
	/* AUTO */
	{
//Auto-XCLK24MHz
  {0x098C,0xA404},//MCU_ADDRESS[FD_MODE]
  {0x0990,0x0050},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0006},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
	},
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_effect_normal[] = {
	/* normal: */
//CAMERA_EFFECT_OFF           0
  {0x098C,0x2759},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6440},//MCU_DATA_0
  {0x098C,0x275B},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6440},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_effect_black_white[] = {
	/* B&W: */
//CAMERA_EFFECT_MONO          1
  {0x098C,0x2759},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6441},//MCU_DATA_0
  {0x098C,0x275B},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6441},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_effect_negative[] = {
	/* Negative: */
//CAMERA_EFFECT_NEGATIVE      2
  {0x098C,0x2759},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6443},//MCU_DATA_0
  {0x098C,0x275B},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6443},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_effect_old_movie[] = {
	/* Sepia(antique): */
//CAMERA_EFFECT_SEPIA         4
  {0x098C,0x2763},//MCU_ADDRESS[MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
  {0x0990,0xB023},//MCU_DATA_0
  {0x098C,0x2759},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6442},//MCU_DATA_0
  {0x098C,0x275B},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6442},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_effect_solarize[] = {
//CAMERA_EFFECT_SOLARIZE      3
  {0x098C,0x2759},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6444},//MCU_DATA_0
  {0x098C,0x275B},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6444},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};   

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_effect_aqua[] = {
//CAMERA_EFFECT_AQUA      4
  {0x098C,0x2763},//MCU_ADDRESS[MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
  {0x0990,0xC0C0},//MCU_DATA_0
  {0x098C,0x2759},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6442},//MCU_DATA_0
  {0x098C,0x275B},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6442},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};  

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_wb_auto[] = {
	/* Auto: */
//CAMERA_WB_AUTO                //1
  {0x098C,0xA34A},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x0090},//MCU_DATA_0
  {0x098C,0xA34B},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00FF},//MCU_DATA_0
  {0x098C,0xA34C},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x0075},//MCU_DATA_0
  {0x098C,0xA34D},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x00EF},//MCU_DATA_0
  {0x098C,0xA351},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xA352},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x007F},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_wb_sunny[] = {
	/* Sunny: */
//CAMERA_WB_DAYLIGHT          //5
  {0x098C,0xA34A},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x00E0},//MCU_DATA_0
  {0x098C,0xA34B},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00E0},//MCU_DATA_0
  {0x098C,0xA34C},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x007D},//MCU_DATA_0
  {0x098C,0xA34D},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x007D},//MCU_DATA_0
  {0x098C,0xA351},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x007F},//MCU_DATA_0
  {0x098C,0xA352},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x007F},//MCU_DATA_0
  {0x098C,0xA34E},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x00E0},//MCU_DATA_0
  {0x098C,0xA350},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x007D},//MCU_DATA_0
  {0x098C,0xA353},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x007F},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_wb_cloudy[] = {
	/* Cloudy: */
//CAMERA_WB_CLOUDY_DAYLIGHT   //6
  {0x098C,0xA34A},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x00EA},//MCU_DATA_0
  {0x098C,0xA34B},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00EA},//MCU_DATA_0
  {0x098C,0xA34C},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x0072},//MCU_DATA_0
  {0x098C,0xA34D},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x0072},//MCU_DATA_0
  {0x098C,0xA351},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x007F},//MCU_DATA_0
  {0x098C,0xA352},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x007F},//MCU_DATA_0
  {0x098C,0xA34E},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x00EA},//MCU_DATA_0
  {0x098C,0xA350},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x0072},//MCU_DATA_0
  {0x098C,0xA353},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x007F},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_wb_office[] = {
	/* Office: */
//CAMERA_WB_FLUORESCENT       //4
  {0x098C,0xA34A},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x00C3},//MCU_DATA_0
  {0x098C,0xA34B},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00C3},//MCU_DATA_0
  {0x098C,0xA34C},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x0084},//MCU_DATA_0
  {0x098C,0xA34D},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x0084},//MCU_DATA_0
  {0x098C,0xA351},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x002F},//MCU_DATA_0
  {0x098C,0xA352},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x002F},//MCU_DATA_0
  {0x098C,0xA34E},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x00C3},//MCU_DATA_0
  {0x098C,0xA350},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x0084},//MCU_DATA_0
  {0x098C,0xA353},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x002F},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static struct msm_camera_i2c_reg_conf hw_mt9v113_reg_wb_home[] = {
	/* Home: */
//CAMERA_WB_INCANDESCENT       //3
  {0x098C,0xA34A},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x0090},//MCU_DATA_0
  {0x098C,0xA34B},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x0090},//MCU_DATA_0
  {0x098C,0xA34C},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x008B},//MCU_DATA_0
  {0x098C,0xA34D},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x008B},//MCU_DATA_0
  {0x098C,0xA351},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xA352},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xA34E},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x0090},//MCU_DATA_0
  {0x098C,0xA350},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x008B},//MCU_DATA_0
  {0x098C,0xA353},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x0000},//MCU_DATA_0
  {0x098C,0xA103},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005},//MCU_DATA_0
  {0x098C,0xA244},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB},//MCU_DATA_0
};

static const struct i2c_device_id hw_mt9v113_i2c_id[] = {
	{HW_MT9V113_SENSOR_NAME, (kernel_ulong_t)&hw_mt9v113_s_ctrl},
	{ }
};

static int32_t msm_hw_mt9v113_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	CDBG("%s, E.", __func__);

	return msm_sensor_i2c_probe(client, id, &hw_mt9v113_s_ctrl);
}

static struct i2c_driver hw_mt9v113_i2c_driver = {
	.id_table = hw_mt9v113_i2c_id,
	.probe  = msm_hw_mt9v113_i2c_probe,
	.driver = {
		.name = HW_MT9V113_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client hw_mt9v113_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id hw_mt9v113_dt_match[] = {
	{.compatible = "qcom,hw_mt9v113", .data = &hw_mt9v113_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, hw_mt9v113_dt_match);

static struct platform_driver hw_mt9v113_platform_driver = {
	.driver = {
		.name = "qcom,hw_mt9v113",
		.owner = THIS_MODULE,
		.of_match_table = hw_mt9v113_dt_match,
	},
};

static int32_t hw_mt9v113_platform_probe(struct platform_device *pdev)
{
	int32_t rc;
	const struct of_device_id *match;
	CDBG("%s, E.", __func__);
	match = of_match_device(hw_mt9v113_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init hw_mt9v113_init_module(void)
{
	int32_t rc;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&hw_mt9v113_platform_driver,
		hw_mt9v113_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&hw_mt9v113_i2c_driver);
}

static void __exit hw_mt9v113_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (hw_mt9v113_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&hw_mt9v113_s_ctrl);
		platform_driver_unregister(&hw_mt9v113_platform_driver);
	} else
		i2c_del_driver(&hw_mt9v113_i2c_driver);
	return;
}


static void hw_mt9v113_i2c_write_table(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_camera_i2c_reg_conf *table,
		int num)
{
	int i = 0;
	int rc = 0;
	for (i = 0; i < num; ++i) {
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write(
			s_ctrl->sensor_i2c_client, table->reg_addr,
			table->reg_data,
			MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			msleep(100);
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write(
				s_ctrl->sensor_i2c_client, table->reg_addr,
				table->reg_data,
				MSM_CAMERA_I2C_WORD_DATA);
		}
		table++;
	}
}

static void hw_mt9v113_set_stauration(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	CDBG("%s %d", __func__, value);
	hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_saturation[value][0],
		ARRAY_SIZE(hw_mt9v113_reg_saturation[value]));
}

static void hw_mt9v113_set_contrast(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	CDBG("%s %d", __func__, value);
	hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_contrast[value][0],
		ARRAY_SIZE(hw_mt9v113_reg_contrast[value]));
}

static void hw_mt9v113_set_sharpness(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int val = value / 6;
	CDBG("%s %d : %d", __func__, value, val);
	hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_sharpness[val][0],
		ARRAY_SIZE(hw_mt9v113_reg_sharpness[val]));
}


static void hw_mt9v113_set_iso(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	CDBG("%s %d", __func__, value);
	hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_iso[value][0],
		ARRAY_SIZE(hw_mt9v113_reg_iso[value]));
}

static void hw_mt9v113_set_exposure_compensation(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	int val = (value + 12) / 6;
	CDBG("%s %d : %d", __func__, value, val);
	hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_exposure_compensation[val][0],
		ARRAY_SIZE(hw_mt9v113_reg_exposure_compensation[val]));
}

static void hw_mt9v113_set_effect(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	CDBG("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_EFFECT_MODE_OFF: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_effect_normal[0],
			ARRAY_SIZE(hw_mt9v113_reg_effect_normal));
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_MONO: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_effect_black_white[0],
			ARRAY_SIZE(hw_mt9v113_reg_effect_black_white));
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_NEGATIVE: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_effect_negative[0],
			ARRAY_SIZE(hw_mt9v113_reg_effect_negative));
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_SEPIA: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_effect_old_movie[0],
			ARRAY_SIZE(hw_mt9v113_reg_effect_old_movie));
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_SOLARIZE: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_effect_solarize[0],
			ARRAY_SIZE(hw_mt9v113_reg_effect_solarize));
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_AQUA: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_effect_aqua[0],
			ARRAY_SIZE(hw_mt9v113_reg_effect_aqua));
		break;
	}
	default:
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_effect_normal[0],
			ARRAY_SIZE(hw_mt9v113_reg_effect_normal));
	}
}

static void hw_mt9v113_set_antibanding(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	CDBG("%s %d", __func__, value);
	hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_antibanding[value][0],
		ARRAY_SIZE(hw_mt9v113_reg_antibanding[value]));
}

static void hw_mt9v113_set_white_balance_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	CDBG("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_WB_MODE_AUTO: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_wb_auto[0],
			ARRAY_SIZE(hw_mt9v113_reg_wb_auto));
		break;
	}
	case MSM_CAMERA_WB_MODE_INCANDESCENT: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_wb_home[0],
			ARRAY_SIZE(hw_mt9v113_reg_wb_home));
		break;
	}
	case MSM_CAMERA_WB_MODE_DAYLIGHT: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_wb_sunny[0],
			ARRAY_SIZE(hw_mt9v113_reg_wb_sunny));
					break;
	}
	case MSM_CAMERA_WB_MODE_FLUORESCENT: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_wb_office[0],
			ARRAY_SIZE(hw_mt9v113_reg_wb_office));
					break;
	}
	case MSM_CAMERA_WB_MODE_CLOUDY_DAYLIGHT: {
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_wb_cloudy[0],
			ARRAY_SIZE(hw_mt9v113_reg_wb_cloudy));
					break;
	}
	default:
		hw_mt9v113_i2c_write_table(s_ctrl, &hw_mt9v113_reg_wb_auto[0],
		ARRAY_SIZE(hw_mt9v113_reg_wb_auto));
	}
}

/*===========================================================================
 * FUNCTION    - wait_for_detect -
 *
 * DESCRIPTION: wait for I2C device operation
 *==========================================================================*/
void wait_for_detect(struct msm_sensor_ctrl_t *s_ctrl)
{

    long rc = 0;            //return value
    uint16_t reg_value = 0; //temp reg value
    int8_t try_times = 20;  //max while times

    //20 times 
    while(try_times > 0)
    {
        //read reg 0xA103 I2C status 
        rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
                s_ctrl->sensor_i2c_client,0x098C,0xA103, MSM_CAMERA_I2C_WORD_DATA);
        //fail    
        if (rc < 0) 
        {
            pr_err("%s:%d: i2c_write failed\n", __func__, __LINE__);
        }
        //get reg value
        rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
                s_ctrl->sensor_i2c_client,0x0990,&reg_value, MSM_CAMERA_I2C_WORD_DATA);
        //fail
        if (rc < 0) 
        {
            pr_err("%s:%d: i2c_read failed\n", __func__, __LINE__);
        }
        //status is OK
        if(reg_value == 0)
        {
            CDBG("%s:%d:try_times=%d ,status: OK\n", __func__, __LINE__,20-try_times);
            break;
        }

        try_times--;

        mdelay(10);
    }

}

/*===========================================================================
 * FUNCTION    - hw_mt9v113_wait -
 *
 * DESCRIPTION: wait for I2C device operation
 *==========================================================================*/
int32_t hw_mt9v113_wait(struct msm_sensor_ctrl_t *s_ctrl,  int time)
{
	int count = 0;
	int rc = 0;
	unsigned short r_value = 0;
	unsigned short bit15 = 0;

	/*modify delays and polls after register writing*/
	switch(time){
		case 0:
		case 2:
		case 3:
			mdelay(10);
			break;
		case 1:
			for(count = 50; count > 0; count --)
			{
				rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
					s_ctrl->sensor_i2c_client,0x0014,&bit15, MSM_CAMERA_I2C_WORD_DATA);
				if (rc < 0)
				{
					pr_err("%s:%d: i2c_read failed\n", __func__, __LINE__);
				}

				bit15 = bit15 & 0x8000;
				CDBG("count = %d, bit15 = 0x%x,\n", count,bit15);

				if(0x8000 == bit15) 
				{
				    break;
				}
				mdelay(10);
			}
			break;
		case 4:
		case 5:
			for(count = 50; count > 0; count --)
			{
				rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
							s_ctrl->sensor_i2c_client,0x098C,0xA103, MSM_CAMERA_I2C_WORD_DATA);
				//fail
				if (rc < 0)
				{
					pr_err("%s:%d: i2c_write failed\n", __func__, __LINE__);
				}
				//get reg value
				rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
				s_ctrl->sensor_i2c_client,0x0990,&r_value, MSM_CAMERA_I2C_WORD_DATA);
				if (rc < 0)
				{
					pr_err("%s:%d: i2c_read failed\n", __func__, __LINE__);
				}

				CDBG("count = %d, value = %d\n", count, r_value);

				if(0 == r_value)
				{
					if( 5 == time)
						mdelay(300);
					break;
				}
				mdelay(10);
			}
			break;
		default:
			break;
	}

	return 0;
}

int32_t hw_mt9v113_write_init_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int32_t i = 0;

	CDBG("%s is called !\n", __func__);
	for (i = 0; i < ARRAY_SIZE(hw_mt9v113_init_conf); i++)
	{
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(s_ctrl->sensor_i2c_client,hw_mt9v113_init_conf[i].conf,
			hw_mt9v113_init_conf[i].size,
			hw_mt9v113_init_conf[i].data_type);
		if (rc < 0)
			break;
		hw_mt9v113_wait(s_ctrl, i);
	}

	return rc;
}

int32_t hw_mt9v113_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	long rc = 0;
	int32_t i = 0;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_GET_SENSOR_INFO:
		memcpy(cdata->cfg.sensor_info.sensor_name,
			s_ctrl->sensordata->sensor_name,
			sizeof(cdata->cfg.sensor_info.sensor_name));
		cdata->cfg.sensor_info.session_id =
			s_ctrl->sensordata->sensor_info->session_id;
		for (i = 0; i < SUB_MODULE_MAX; i++)
			cdata->cfg.sensor_info.subdev_id[i] =
				s_ctrl->sensordata->sensor_info->subdev_id[i];
		CDBG("%s:%d sensor name %s\n", __func__, __LINE__,
			cdata->cfg.sensor_info.sensor_name);
		CDBG("%s:%d session id %d\n", __func__, __LINE__,
			cdata->cfg.sensor_info.session_id);
		for (i = 0; i < SUB_MODULE_MAX; i++)
			CDBG("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
				cdata->cfg.sensor_info.subdev_id[i]);

		break;
	case CFG_SET_INIT_SETTING:
		hw_mt9v113_write_init_settings(s_ctrl);
		break;
	case CFG_SET_RESOLUTION:
		break;
	case CFG_SET_STOP_STREAM:
		pr_err("%s, sensor stop stream!!", __func__);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(s_ctrl->sensor_i2c_client,
			hw_mt9v113_stop_settings,
			ARRAY_SIZE(hw_mt9v113_stop_settings),
			MSM_CAMERA_I2C_WORD_DATA);
		break;
	case CFG_SET_START_STREAM:
		pr_err("%s, sensor start stream!!", __func__);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(s_ctrl->sensor_i2c_client,
			hw_mt9v113_start_settings,
			ARRAY_SIZE(hw_mt9v113_start_settings),
			MSM_CAMERA_I2C_WORD_DATA);
		break;
	case CFG_GET_SENSOR_INIT_PARAMS:
		cdata->cfg.sensor_init_params =
			*s_ctrl->sensordata->sensor_init_params;
		CDBG("%s:%d init params mode %d pos %d mount %d\n", __func__,
			__LINE__,
			cdata->cfg.sensor_init_params.modes_supported,
			cdata->cfg.sensor_init_params.position,
			cdata->cfg.sensor_init_params.sensor_mount_angle);
		break;
	case CFG_SET_SLAVE_INFO: {
		struct msm_camera_sensor_slave_info sensor_slave_info;
		struct msm_sensor_power_setting_array *power_setting_array;
		int slave_index = 0;
		if (copy_from_user(&sensor_slave_info,
		    (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_sensor_slave_info))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		/* Update sensor slave address */
		if (sensor_slave_info.slave_addr) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				sensor_slave_info.slave_addr >> 1;
		}

		/* Update sensor address type */
		s_ctrl->sensor_i2c_client->addr_type =
			sensor_slave_info.addr_type;

		/* Update power up / down sequence */
		s_ctrl->power_setting_array =
			sensor_slave_info.power_setting_array;
		power_setting_array = &s_ctrl->power_setting_array;
		power_setting_array->power_setting = kzalloc(
			power_setting_array->size *
			sizeof(struct msm_sensor_power_setting), GFP_KERNEL);
		if (!power_setting_array->power_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(power_setting_array->power_setting,
		    (void *)sensor_slave_info.power_setting_array.power_setting,
		    power_setting_array->size *
		    sizeof(struct msm_sensor_power_setting))) {
			kfree(power_setting_array->power_setting);
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		s_ctrl->free_power_setting = true;
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.slave_addr);
		CDBG("%s sensor addr type %d\n", __func__,
			sensor_slave_info.addr_type);
		CDBG("%s sensor reg %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id_reg_addr);
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id);
		for (slave_index = 0; slave_index <
			power_setting_array->size; slave_index++) {
			CDBG("%s i %d power setting %d %d %ld %d\n", __func__,
				slave_index,
				power_setting_array->power_setting[slave_index].
				seq_type,
				power_setting_array->power_setting[slave_index].
				seq_val,
				power_setting_array->power_setting[slave_index].
				config_val,
				power_setting_array->power_setting[slave_index].
				delay);
		}
		kfree(power_setting_array->power_setting);
		break;
	}
	case CFG_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &conf_array);
		kfree(reg_setting);
		break;
	}
	case CFG_WRITE_I2C_SEQ_ARRAY: {
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_seq_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_seq_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}

	case CFG_POWER_UP:
		if (s_ctrl->func_tbl->sensor_power_up)
			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_POWER_DOWN:
		if (s_ctrl->func_tbl->sensor_power_down)
			rc = s_ctrl->func_tbl->sensor_power_down(
				s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_SET_STOP_STREAM_SETTING: {
		struct msm_camera_i2c_reg_setting *stop_setting =
			&s_ctrl->stop_setting;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		if (copy_from_user(stop_setting, (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = stop_setting->reg_setting;
		stop_setting->reg_setting = kzalloc(stop_setting->size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!stop_setting->reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(stop_setting->reg_setting,
		    (void *)reg_setting, stop_setting->size *
		    sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(stop_setting->reg_setting);
			stop_setting->reg_setting = NULL;
			stop_setting->size = 0;
			rc = -EFAULT;
			break;
		}
		break;
	}
	case CFG_SET_SATURATION: {
		int32_t sat_lev;
		if (copy_from_user(&sat_lev, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: Saturation Value is %d", __func__, sat_lev);
		hw_mt9v113_set_stauration(s_ctrl, sat_lev);
		break;
	}
	case CFG_SET_CONTRAST: {
		int32_t con_lev;
		if (copy_from_user(&con_lev, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: Contrast Value is %d", __func__, con_lev);
		hw_mt9v113_set_contrast(s_ctrl, con_lev);
		break;
	}
	case CFG_SET_SHARPNESS: {
		int32_t shp_lev;
		if (copy_from_user(&shp_lev, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: Sharpness Value is %d", __func__, shp_lev);
		hw_mt9v113_set_sharpness(s_ctrl, shp_lev);
		break;
	}
	case CFG_SET_ISO: {
		int32_t iso_lev;
		if (copy_from_user(&iso_lev, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: ISO Value is %d", __func__, iso_lev);
		hw_mt9v113_set_iso(s_ctrl, iso_lev);
		break;
	}
	case CFG_SET_EXPOSURE_COMPENSATION: {
		int32_t ec_lev;
		if (copy_from_user(&ec_lev, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: Exposure compensation Value is %d",
			__func__, ec_lev);
		hw_mt9v113_set_exposure_compensation(s_ctrl, ec_lev);
		break;
	}
	case CFG_SET_EFFECT: {
		int32_t effect_mode;
		if (copy_from_user(&effect_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: Effect mode is %d", __func__, effect_mode);
		hw_mt9v113_set_effect(s_ctrl, effect_mode);
		break;
	}
	case CFG_SET_ANTIBANDING: {
		int32_t antibanding_mode;
		if (copy_from_user(&antibanding_mode,
			(void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: anti-banding mode is %d", __func__,
			antibanding_mode);
		hw_mt9v113_set_antibanding(s_ctrl, antibanding_mode);
		break;
	}
	case CFG_SET_BESTSHOT_MODE: {
		int32_t bs_mode;
		if (copy_from_user(&bs_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: best shot mode is %d", __func__, bs_mode);
		//mt9v113_set_scene_mode(s_ctrl, bs_mode);
		break;
	}
	case CFG_SET_WHITE_BALANCE: {
		int32_t wb_mode;
		if (copy_from_user(&wb_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s: white balance is %d", __func__, wb_mode);
		hw_mt9v113_set_white_balance_mode(s_ctrl, wb_mode);
		break;
	}
#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
    case CFG_GET_SENSOR_PROJECT_INFO:
        memcpy(cdata->cfg.sensor_info.sensor_project_name,
               s_ctrl->sensordata->sensor_info->sensor_project_name,
               sizeof(cdata->cfg.sensor_info.sensor_project_name));

        pr_info("%s, %d: sensor project name %s\n", __func__, __LINE__,
               cdata->cfg.sensor_info.sensor_project_name);
    break;
#endif

	default:
		rc = -EFAULT;
		break;
	}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

/*make mt9v113 use its own if functions because of the msm-if changed to static*/
static int32_t hw_mt9v113_sensor_enable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_INIT, NULL);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_CFG, (void *)&i2c_conf->i2c_mux_mode);
	return 0;
}
static int32_t hw_mt9v113_sensor_disable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
				VIDIOC_MSM_I2C_MUX_RELEASE, NULL);
	return 0;
}

/*make mt9v113 use its own power up&down func*/

int32_t hw_mt9v113_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int32_t index = 0;
	struct msm_sensor_power_setting_array *power_setting_array = NULL;
	struct msm_sensor_power_setting *power_setting = NULL;
	struct msm_sensor_power_setting_array *power_down_setting_array = NULL;
	struct msm_camera_sensor_board_info *data = s_ctrl->sensordata;
	power_setting_array = &s_ctrl->power_setting_array;
	power_down_setting_array = &s_ctrl->power_down_setting_array;
	CDBG("%s:%d enter\n", __func__, __LINE__);

	if (data->gpio_conf->cam_gpiomux_conf_tbl != NULL) {
		pr_err("%s:%d mux install\n", __func__, __LINE__);
		msm_gpiomux_install(
			(struct msm_gpiomux_config *)
			data->gpio_conf->cam_gpiomux_conf_tbl,
			data->gpio_conf->cam_gpiomux_conf_tbl_size);
	}

	rc = msm_camera_request_gpio_table(
		data->gpio_conf->cam_gpio_req_tbl,
		data->gpio_conf->cam_gpio_req_tbl_size, 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		return rc;
	}
	for (index = 0; index < power_setting_array->size; index++) {
		CDBG("%s index %d\n", __func__, index);
		power_setting = &power_setting_array->power_setting[index];
		CDBG("%s type %d\n", __func__, power_setting->seq_type);
		CDBG("%s seq_val %d\n", __func__, power_setting->seq_val);
		switch (power_setting->seq_type) {
		case SENSOR_CLK:
			if (power_setting->seq_val >= s_ctrl->clk_info_size) {
				pr_err("%s clk index %d >= max %d\n", __func__,
					power_setting->seq_val,
					s_ctrl->clk_info_size);
				goto power_up_failed;
			}
			if (power_setting->config_val)
				s_ctrl->clk_info[power_setting->seq_val].
					clk_rate = power_setting->config_val;

			rc = msm_cam_clk_enable(s_ctrl->dev,
				&s_ctrl->clk_info[0],
				(struct clk **)&power_setting->data[0],
				s_ctrl->clk_info_size,
				1);
			if (rc < 0) {
				pr_err("%s: clk enable failed\n",
					__func__);
				goto power_up_failed;
			}

#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
			/*store data[0] for the use of power down*/
			if(s_ctrl->power_down_setting_array.power_setting){
				int32_t i = 0;
				int32_t j = 0;
				struct msm_sensor_power_setting *power_down_setting = NULL;
				for(i=0; i<power_down_setting_array->size; i++){
					power_down_setting = &power_down_setting_array->power_setting[i];
					if(power_setting->seq_val == power_down_setting->seq_val){
						for(j=0; j<s_ctrl->clk_info_size; j++)
						{
							power_down_setting->data[j] = power_setting->data[j];
							CDBG("%s clkptr %p \n", __func__, power_setting->data[j]);
						}
					}
				}
			}
#endif

			break;
		case SENSOR_GPIO:
			if (power_setting->seq_val >= SENSOR_GPIO_MAX ||
				!data->gpio_conf->gpio_num_info) {
				pr_err("%s gpio index %d >= max %d\n", __func__,
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				goto power_up_failed;
			}
			pr_debug("%s:%d gpio set val %d\n", __func__, __LINE__,
				data->gpio_conf->gpio_num_info->gpio_num
				[power_setting->seq_val]);
			gpio_set_value_cansleep(
				data->gpio_conf->gpio_num_info->gpio_num
				[power_setting->seq_val],
				power_setting->config_val);	
			break;
		case SENSOR_VREG:
			if (power_setting->seq_val >= CAM_VREG_MAX) {
				pr_err("%s vreg index %d >= max %d\n", __func__,
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				goto power_up_failed;
			}

			msm_camera_config_single_vreg(s_ctrl->dev,
				&data->cam_vreg[power_setting->seq_val],
				(struct regulator **)&power_setting->data[0],
				1);

#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
			/*store data[0] for the use of power down*/
			if(s_ctrl->power_down_setting_array.power_setting){
				int32_t i = 0;
				struct msm_sensor_power_setting *power_down_setting = NULL;
				for(i=0;i<power_down_setting_array->size; i++){
					power_down_setting = &power_down_setting_array->power_setting[i];
					if(power_setting->seq_val == power_down_setting->seq_val){
						power_down_setting->data[0] = power_setting->data[0];
					}
				}
			}
#endif

			break;
		case SENSOR_I2C_MUX:
			if (data->i2c_conf && data->i2c_conf->use_i2c_mux)
				hw_mt9v113_sensor_enable_i2c_mux(data->i2c_conf);
			break;
		default:
			pr_err("%s error power seq type %d\n", __func__,
				power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20) {
			msleep(power_setting->delay);
		} else if (power_setting->delay) {
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 1000);
		}
	}
	CDBG("%s power up switch over rc=%d\n", __func__, rc);
	if (s_ctrl->sensor_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_util(
			s_ctrl->sensor_i2c_client, MSM_CCI_INIT);
		if (rc < 0) {
			pr_err("%s cci_init failed\n", __func__);
			goto power_up_failed;
		}
	}

	if (s_ctrl->func_tbl->sensor_match_id)
		rc = s_ctrl->func_tbl->sensor_match_id(s_ctrl);
	else
		rc = msm_sensor_match_id(s_ctrl);
	if (rc < 0) {
		pr_err("%s:%d match id failed rc %d\n", __func__, __LINE__, rc);
		goto power_up_failed;
	}

#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
	if (s_ctrl->func_tbl->sensor_match_module)
		rc = s_ctrl->func_tbl->sensor_match_module(s_ctrl);
	if (rc < 0) {
		pr_err("%s:%d match module failed rc %d\n", __func__, __LINE__, rc);
	}
#endif
	CDBG("%s exit\n", __func__);
	return 0;
power_up_failed:
	CDBG("%s:%d failed\n", __func__, __LINE__);
	if (s_ctrl->sensor_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_util(
			s_ctrl->sensor_i2c_client, MSM_CCI_RELEASE);
	}

	if(s_ctrl->power_down_setting_array.power_setting){
		power_setting_array = &s_ctrl->power_down_setting_array;
		index = power_setting_array->size;
		CDBG("power down use the power down settings array\n");
	}
	for (index = 0; index < power_setting_array->size; index++){
		CDBG("%s index %d\n", __func__, index);
		power_setting = &power_setting_array->power_setting[index];
		CDBG("%s type %d\n", __func__, power_setting->seq_type);
		switch (power_setting->seq_type) {
		case SENSOR_CLK:
			msm_cam_clk_enable(s_ctrl->dev,
				&s_ctrl->clk_info[0],
				(struct clk **)&power_setting->data[0],
				s_ctrl->clk_info_size,
				0);
			CDBG("%s powerdown clock num %d\n", __func__, s_ctrl->clk_info_size);
			break;
		case SENSOR_GPIO:
			gpio_set_value_cansleep(
				data->gpio_conf->gpio_num_info->gpio_num
				[power_setting->seq_val], GPIOF_OUT_INIT_LOW);
			break;
		case SENSOR_VREG:
			msm_camera_config_single_vreg(s_ctrl->dev,
				&data->cam_vreg[power_setting->seq_val],
				(struct regulator **)&power_setting->data[0],
				0);
			break;
		case SENSOR_I2C_MUX:
			if (data->i2c_conf && data->i2c_conf->use_i2c_mux)
				hw_mt9v113_sensor_disable_i2c_mux(data->i2c_conf);
			break;
		default:
			pr_err("%s error power seq type %d\n", __func__,
				power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20) {
			msleep(power_setting->delay);
		} else if (power_setting->delay) {
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 1000);
		}
	}
	msm_camera_request_gpio_table(
		data->gpio_conf->cam_gpio_req_tbl,
		data->gpio_conf->cam_gpio_req_tbl_size, 0);
	return rc;
}


int32_t hw_mt9v113_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t index = 0;
	struct msm_sensor_power_setting_array *power_setting_array = NULL;
	struct msm_sensor_power_setting *power_setting = NULL;
	struct msm_camera_sensor_board_info *data = s_ctrl->sensordata;
	s_ctrl->stop_setting_valid = 0;
	power_setting_array = &s_ctrl->power_down_setting_array;
	
	CDBG("%s:%d enter\n", __func__, __LINE__);
	if (s_ctrl->sensor_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_util(
			s_ctrl->sensor_i2c_client, MSM_CCI_RELEASE);
	}

	for (index = 0; index < power_setting_array->size; index++) {
		CDBG("%s index %d\n", __func__, index);
		power_setting = &power_setting_array->power_setting[index];
		CDBG("%s type %d\n", __func__, power_setting->seq_type);
		switch (power_setting->seq_type) {
		case SENSOR_CLK:
			msm_cam_clk_enable(s_ctrl->dev,
				&s_ctrl->clk_info[0],
				(struct clk **)&power_setting->data[0],
				s_ctrl->clk_info_size,
				0);
			CDBG("%s powerdown clock num %d", __func__, s_ctrl->clk_info_size);
			break;
		case SENSOR_GPIO:
			if (power_setting->seq_val >= SENSOR_GPIO_MAX ||
				!data->gpio_conf->gpio_num_info) {
				pr_err("%s gpio index %d >= max %d\n", __func__,
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				continue;
			}

			gpio_set_value_cansleep(
				data->gpio_conf->gpio_num_info->gpio_num
				[power_setting->seq_val], power_setting->seq_val);
			break;
		case SENSOR_VREG:
			if (power_setting->seq_val >= CAM_VREG_MAX) {
				pr_err("%s vreg index %d >= max %d\n", __func__,
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				continue;
			}
			msm_camera_config_single_vreg(s_ctrl->dev,
				&data->cam_vreg[power_setting->seq_val],
				(struct regulator **)&power_setting->data[0],
				0);
			break;
		case SENSOR_I2C_MUX:
			if (data->i2c_conf && data->i2c_conf->use_i2c_mux)
				hw_mt9v113_sensor_disable_i2c_mux(data->i2c_conf);
			break;
		default:
			pr_err("%s error power seq type %d\n", __func__,
				power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20) {
			msleep(power_setting->delay);
		} else if (power_setting->delay) {
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 1000);
		}
	}
	msm_camera_request_gpio_table(
		data->gpio_conf->cam_gpio_req_tbl,
		data->gpio_conf->cam_gpio_req_tbl_size, 0);
	CDBG("%s exit\n", __func__);

	return 0;
}

static int hw_mt9v113_match_module(struct msm_sensor_ctrl_t *s_ctrl)
{
	hw_mt9v113_module_id = 1;
	
	/*add project name for the project menu*/
	s_ctrl->sensordata->sensor_name = "hw_mt9v113";
	strncpy(s_ctrl->sensordata->sensor_info->sensor_project_name, "23060153FF-MT-S", strlen("23060153FF-MT-S")+1);
	pr_info("%s %d : hw_mt9v113_match_module sensor_name=%s, sensor_project_name=%s \n",  __func__, __LINE__,
            s_ctrl->sensordata->sensor_name, s_ctrl->sensordata->sensor_info->sensor_project_name);
	pr_info("check module id from camera id PIN:OK \n");
	
	return 0;
}

static struct msm_sensor_fn_t hw_mt9v113_sensor_func_tbl = {
    .sensor_config = hw_mt9v113_sensor_config,
    .sensor_power_up = hw_mt9v113_sensor_power_up,
    .sensor_power_down = hw_mt9v113_sensor_power_down,
    #ifdef CONFIG_HUAWEI_KERNEL_CAMERA
	.sensor_match_module = hw_mt9v113_match_module,
    #endif
};

static struct msm_sensor_ctrl_t hw_mt9v113_s_ctrl = {
	.sensor_i2c_client = &hw_mt9v113_sensor_i2c_client,
	.power_setting_array.power_setting = hw_mt9v113_power_setting,
	.power_setting_array.size = ARRAY_SIZE(hw_mt9v113_power_setting),
	.msm_sensor_mutex = &hw_mt9v113_mut,
	.sensor_v4l2_subdev_info = hw_mt9v113_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(hw_mt9v113_subdev_info),
	.func_tbl = &hw_mt9v113_sensor_func_tbl,
#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
	.power_down_setting_array.power_setting = hw_mt9v113_power_down_setting,
	.power_down_setting_array.size = ARRAY_SIZE(hw_mt9v113_power_down_setting),
#endif
};

module_init(hw_mt9v113_init_module);
module_exit(hw_mt9v113_exit_module);
MODULE_DESCRIPTION("Aptina 0.3MP YUV sensor driver");
MODULE_LICENSE("GPL v2");
