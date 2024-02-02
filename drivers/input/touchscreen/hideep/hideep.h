/*************************************************************************
 * Copyright (C) 2012 Hideep, Inc.
 * logan.ji@hideep.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 *************************************************************************/

#ifndef _LINUX_HIDEEP_H
#define _LINUX_HIDEEP_H

#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/input.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/machine.h>
#include <linux/kthread.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/task_work.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/input/mt.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>
#include <linux/fcntl.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include <uapi/linux/sched/types.h>
#include <stdbool.h>
#include <linux/wakelock.h>

#if IS_ENABLED(CONFIG_SPU_VERIFY)
#include <linux/spu-verify.h>
#define SUPPORT_FW_SIGNED
#endif

#include	"../../sec_input/sec_input.h"

#undef CONFIG_FB
#ifdef CONFIG_FB
#include <linux/fb.h>
#include <linux/notifier.h>
#endif

#define HIDEEP_PROTOCOL_2_0	/* HIDEEP_PROTOCOL_2_0W is based on hideep protocol 2.0*/
#define HIDEEP_PROTOCOL_2_0W	/* HIDEEP_PROTOCOL_2_0W is based on hideep protocol 2.0, but not same, dedicated only for watch device */
#define HIDEEP_TYPE_B_PROTOCOL	/* HIDEEP_TYPE_B_PROTOCOL is for input_dev, if define, using TYPE_B, otherwise TYPE_A. */
//#define HIDEEP_SUPPORT_KEY	/* HIDEEP_KEYBUTTON if define, it will use key button. */
//#define HIDEEP_SUPPORT_TOUCH_WHEEL	/* HIDEEP_SUPPORT_TOUCH_WHEEL if define, it will use touch bezel. */
#ifndef CONFIG_SEC_FACTORY
#define HIDEEP_SUPPORT_PALM	/* HIDEEP_SUPPORT_WHEEL if define, it will use touch palm. */
#endif
#define TSP_PATH_EXTERNAL_FW		"/sdcard/Firmware/TSP/tsp.bin"
#define TSP_PATH_EXTERNAL_FW_SIGNED	"/sdcard/Firmware/TSP/tsp_signed.bin"
#define TSP_PATH_SPU_FW_SIGNED		"/spu/TSP/ffu_tsp.bin"

#define FRAME_HEADER_SIZE		8
#define MAX_I2C_BUFFER_SIZE		512// if size of system i2c buffer is smaller than this value, need to modify

#define HIDEEP_DEV_NAME			"hideep_dev"
#define HIDEEP_TS_NAME			"hideep"
#define HIDEEP_WHEEL_NAME		"detent_bezel"
#define HIDEEP_DEBUG_DEVICE_NAME	"hideep_debug"
#define HIDEEP_I2C_NAME			"hideep_ts"

/* ic type bit */
#define HIDEEP_CRIMSON			0x0010
#define HIDEEP_LIME			0x0020
#define HIDEEP_MAROON			0x0030
#define HIDEEP_RUBY			0x0040
#define HIDEEP_LEMON			0x0050
#define HIDEEP_ORANGE			0x0060
#define HIDEEP_GARNET			0x0070

/* Touch & key event */
#define HIDEEP_INFO_ADDR			0x0000
#define HIDEEP_VR_INFO_ADDR			0x0006
#define HIDEEP_CHIP_ID_ADDR			0x001C
#define HIDEEP_EVENT_COUNT_ADDR		0x240
#define HIDEEP_TOUCH_DATA_ADDR		0x242
#define HIDEEP_KEY_DATA_ADDR		0x2BA
#define HIDEEP_RAW_DATA_ADDR		0x1000

/* command list */
#define HIDEEP_RESET_CMD		0x9800
#define HIDEEP_SYNC_CMD			0x9802
#define HIDEEP_INTCLR_CMD		0x9802
#define HIDEEP_OPMODE_CMD		0x9804
#define HIDEEP_SWTICH_CMD		0x9805
#define HIDEEP_CONNECTION_CHECK_CMD	0x9807
#define HIDEEP_2D_TEST_COMMAND		0x9807
#define HIDEEP_1D_TEST_COMMAND		0x980A
#define HIDEEP_SLEEP_CMD		0x980D
#define HIDEEP_IDLE_MODE_CMD		0x980E
#define HIDEEP_CHARGER_MODE_CMD		0x9811
#define HIDEEP_FIX_ACTIVE_CMD		0x9815
#define HIDEEP_DEAD_ZONE_CMD		0x9816
#define HIDEEP_BEZEL_ENABLE_CMD		0x9817
#define HIDEEP_GLOVE_MODE_CMD		0x9901

/* firmware integrity check */
#define HIDEEP_INTEGRITY_CHECK_PASS		0x98AA
#define HIDEEP_INTEGRITY_CHECK_FAIL		0xFA11

/* multi-touch & key definitions.*/
#define HIDEEP_MT_MAX			2
#define HIDEEP_KEY_MAX			3

/* multi touch event bit */
#define HIDEEP_MT_ALWAYS_REPORT		0
#define HIDEEP_MT_TOUCHED		1
#define HIDEEP_MT_FIRST_CONTACT		2
#define HIDEEP_MT_DRAG_MOVE		3
#define HIDEEP_MT_RELEASED		4
#define HIDEEP_MT_WHEEL_CW		6
#define HIDEEP_MT_WHEEL_CCW		7

/* multi touch type code */
#define HIDEEP_MT_HOVERING_TOUCH	0x00
#define HIDEEP_MT_FINGER_TOUCH		0x01
#define HIDEEP_MT_PASSIVE_STYLUS_TOUCH	0x02
#define HIDEEP_MT_NAIL_TOUCH		0x03
#define HIDEEP_MT_PALM_TOUCH		0x04
#define HIDEEP_MT_CHEEK_TOUCH		0x05
#define HIDEEP_MT_WHEEL_TOUCH		0x06
#define HIDEEP_GESTURE_SINGLE_TAP		0x07

/* touch event */
#define HIDEEP_COORDINATE_ACTION_PRESS		0x06
#define HIDEEP_COORDINATE_ACTION_MOVE		0x0A
#define HIDEEP_COORDINATE_ACTION_RELEASE		0x1A

/* key event bit */
#define HIDEEP_KEY_RELEASED		0x20
#define HIDEEP_KEY_PRESSED		0x40
#define HIDEEP_KEY_FIRST_PRESSED	0x80
#define HIDEEP_KEY_PRESSED_MASK		0xC0

/* SEC_TS_DEBUG : Print event contents */
#define HIDEEP_DEBUG_PRINT_ALLEVENT		0x01
#define HIDEEP_DEBUG_PRINT_ONEEVENT		0x02
#define HIDEEP_DEBUG_PRINT_I2C_READ_CMD		0x04
#define HIDEEP_DEBUG_PRINT_I2C_WRITE_CMD	0x08
#define HIDEEP_DEBUG_SEND_UEVENT		0x80

/* bezel */
#define BZ_CW				1
#define BZ_CC				-1

/* proc */
#define HIDEEP_PROC_SIZE		1024

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

#ifdef HIDEEP_PROTOCOL_2_0
struct hideep_mt_t {
	u16 x;
	u16 y;
	u16 z;
	u8 w;
	u8 flag;
	u8 type;
	u8 index;
#ifdef HIDEEP_PROTOCOL_2_0W
	u16 angle;
#endif
};
#endif

enum firmware_update{
	TSP_BUILT_IN = 0,
	TSP_SDCARD,
	NONE,
	TSP_SPU,
	TSP_VERIFICATION,
};

/* IC State */
enum e_dev_state {
	state_init = 1,
	state_normal,
	state_sleep,
	state_updating,
	state_debugging,
};


struct vr_info_t {
	u16 gen_ver;                 // 0x0006
	u16 vr_ver;                  // 0x0008
	u16 boot_ver;                // 0x000A
	u16 core_ver;                // 0x000C
	u16 custom_ver;              // 0x000E
	u16 release_ver;             // 0x0010
	u8  model_name[6];           // 0x0012
	u8  factory_id;              // 0x0018
	u8  panel_type;              // 0x0019
	u16 product_code;            // 0x001A
	u16 ic_code;                 // 0x001C
	u8  module_id;               // 0x001E
	u8  rsv0;                    // 0x001F
	u8  num_tx;                  // 0x0020
	u8  num_rx;                  // 0x0021
	u8  num_key;                 // 0x0022
	u8  num_auluz_tx;            // 0x0023
	u8  num_auluz_rx;            // 0x0024
	u8  rsv1[3];                 // 0x0025
	u16 resolution_x;            // 0x0028
	u16 resolution_y;            // 0x002A
	u8  ic_name[4];              // 0x002C
	u8  rsv2[24];                // 0x0030
	u16 max_w;                   // 0x0048
	u16 max_z;                   // 0x004A
};

struct dwz_info_t {
	u32 code_start; // code start addr
	u8 code_crc[12]; // code crc

	u32 c_code_start; // custom code start addr
	u16 c_code_len; // custom code length
	u16 gen_ver; // generation version

	u32 vr_start; // VR code start addr
	u16 vr_len; // VR code length
	u16 rsv0;

	u32 ft_start; // FT start   addr
	u16 ft_len; // FT length
	u16 vr_version; // VR version

	u16 boot_ver; // bootloader version
	u16 core_ver; // core version
	u16 custom_ver; // custom version
	u16 release_ver; // Release version

	u8 factory_id; // Factory ID
	u8 panel_type; // Panel type

	u8 model_name[6]; // model name
	u16 product_code; // product code
	u16 module_id; // extra option

	u16 product_id; // Product ID
	u16 vendor_id; // Vendor ID
};

struct hideep_platform_data_t {
	u32 max_x;
	u32 max_y;
	u32 max_z;
	u32 max_w;

	int irq_gpio;
	int reset_gpio;

	const char *int_mode; //interrupt mode
	const char *firmware_name;

	struct regulator *vcc_vdd;
	struct regulator *vcc_vid;

	int bringup;
	int model_num;
	bool support_ldo_control;
	bool support_reset_control;
	bool support_bezel_detent;
	bool regulator_boot_on;

	int tsp_id;
	int tspid_val;

	u8 img_version_of_ic[4];
	u8 img_version_of_bin[4];
};

struct hideep_debug_dev_t {
	struct miscdevice misc;

	wait_queue_head_t i_packet;

	u32 ready;
	u8 *vr_buff;
	u8 *img_buff;
	u16 vr_size;
	u16 img_size;

	bool debug_enable;
	bool release_flag;

	struct hideep_t *ts;
};

struct hideep_function_list_t {
	//core
	int (*i2c_read)(struct hideep_t *, u16, u8 *, u16);
	int (*i2c_write)(struct hideep_t *, u16, u8 *, u16);
	void (*reset_ic)(struct hideep_t *);
	void (*power)(struct hideep_t *, int);

	//isp
	int (*config_ic_feature)(struct hideep_t *);
	int (*update_all)(struct hideep_t *, const char *);
	int (*update_part)(struct hideep_t *, u8 *, int , int, bool);
	int (*get_vr_info)(struct hideep_t *);
	void (*sp_func)(struct hideep_t *, u8 *, int, int);
};

struct flash_type {
	bool support_lock;
	bool support_ecc;
	bool support_nvm_extra_outer_work;

	u32 unlock_mask;
	u32 page_size;

	u32 HIDEEP_MERASE;
	u32 HIDEEP_SERASE;
	u32 HIDEEP_PERASE;
	u32 HIDEEP_PROG;
	u32 HIDEEP_WRONLY;
	u32 HIDEEP_INF;
	u32 HIDEEP_MCS;

	u32 HIDEEP_NVM_PAGE_SIZE;
	u32 HIDEEP_NVM_MASK;
	u32 HIDEEP_NVM_ADDR_MASK;
	u32 HIDEEP_FLASH_PIO_SIG;
	u32 HIDEEP_FLASH_ERASE_ENABLE;
	u32 HIDEEP_FLASH_WRITE_ENABLE;
};

/* 28 bytes */
struct debug_state {
	u32 crc_bin_boot;
	u32 crc_bin_exec;
	u32 crc_calc_boot;
	u32 crc_calc_exec;
	u32 mode_status : 4;
	u32 mode_forcing_state : 4;
	u32 noise_mode : 2;
	u32 normal_charger : 1;
	u32 wireless_charger : 1;
	u32 high_sensitivity_mode : 3;
	u32 wet_mode : 2;
	u32 deadzone_expansion_mode : 2;
	u32 module_test_mode : 1;
	u32 digital_wheel_mode : 2;
	u32 reserve : 10;
	u16 baseline_init_count_palm;
	u16 baseline_init_count_abnomal;
	u16 baseline_init_count_aot;
	u16 vsync_abnomal_expire_count;
} __attribute__ ((packed));

struct hideep_t {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *wheel_dev;
	struct hideep_platform_data_t *p_data;
	struct sec_cmd_data sec;

	struct mutex dev_mutex;
	struct mutex i2c_mutex;

	struct wake_lock wakelock;
	struct completion resume_done;

	struct hideep_debug_dev_t debug_dev;
	struct hideep_function_list_t *hideep_api;

	struct delayed_work work_print_info;
	struct delayed_work work_read_info;
	u32 print_info_cnt_open;
	u32 print_info_cnt_release;

	bool suspended;
	enum e_dev_state dev_state;

#ifdef CONFIG_FB
	struct notifier_block fb_notif;
#endif

	volatile bool shutdown_is_on_going;

	u32 tch_count;
	u8 fw_state;

	struct hideep_mt_t touch_evt[HIDEEP_MT_MAX];
	int touch_count;
	u16 p_x;
	u16 p_y;

	unsigned char i2c_buf[256];
	struct vr_info_t *vr_info;

	bool manually_update;
	int interrupt_state;

#if 1 //support auto detection of ic type
	u32 fw_size;
	struct flash_type nvm_ops;
	u32 chip_id_sfr;
	u8 update_mode;
	const char *ic_name;
#endif
	int enabled;
	int debug_flag;
	u8 palm_flag;
	bool aot_enable;
	u8 glove_mode;
	u8 bezel_enable;
	u8 fix_active_mode;
	u8 wireless_charger_mode;
	u8 charger_mode;
	volatile int power_status;
	bool probe_done;
	bool info_work_done;
	s16 *proc_data;
};

extern unsigned int logflags;

/* debug message */
#define HIDEEP_LOG_DEBUG		(1 << 0)
#define HIDEEP_LOG_COORD		(1 << 1)
#define HIDEEP_LOG_I2C			(1 << 2)
#define HIDEEP_LOG_UPDATE		(1 << 3)
#define HIDEEP_LOG_IC_DBG		(1 << 4)
#define HIDEEP_LOG_POWER		(1 << 5)
#define HIDEEP_LOG_SETTING		(HIDEEP_LOG_IC_DBG | HIDEEP_LOG_POWER | HIDEEP_LOG_UPDATE)

#define ts_log_err(a, arg...) \
	printk(KERN_ERR HIDEEP_DEV_NAME " %s:%d "a"\n", __FUNCTION__, __LINE__, ##arg)

#define ts_log_info(a, arg...) \
	printk(KERN_INFO HIDEEP_DEV_NAME " %s:%d "a"\n", __FUNCTION__, __LINE__, ##arg)

#define ts_log_debug(a, arg...) \
	do { \
		if (logflags & HIDEEP_LOG_DEBUG) \
		printk(KERN_DEBUG HIDEEP_DEV_NAME " %s:%d "a"\n", __FUNCTION__, __LINE__, ##arg); \
	} while (0)

#define ts_log_coord(a, arg...) \
	do { \
		if (logflags & HIDEEP_LOG_COORD) \
		printk(KERN_DEBUG "hdp:"a"\n", ##arg); \
	} while (0)

#define ts_log_i2c(a, arg...) \
	do { \
		if (logflags & HIDEEP_LOG_I2C) \
		printk(KERN_DEBUG HIDEEP_DEV_NAME "_i2c %s:%d " a "\n", __FUNCTION__, __LINE__, ##arg); \
	} while (0)

#define ts_log_update(a, arg...) \
	do { \
		if (logflags & HIDEEP_LOG_UPDATE) \
		printk(KERN_DEBUG HIDEEP_DEV_NAME "_update %s:%d " a "\n", __FUNCTION__, __LINE__, ##arg); \
	} while (0)

#define ts_log_ic_dbg(a, arg...) \
	do { \
		if (logflags & HIDEEP_LOG_IC_DBG) \
		printk(KERN_DEBUG HIDEEP_DEV_NAME "_dbg %s:%d " a "\n", __FUNCTION__, __LINE__, ##arg); \
	} while (0)

#define ts_log_power(a, arg...) \
	do { \
		if (logflags & HIDEEP_LOG_POWER) \
		printk(KERN_DEBUG HIDEEP_DEV_NAME "_pwr %s:%d " a "\n", __FUNCTION__, __LINE__, ##arg); \
	} while (0)


#define PGM_BURST_WR
#define PGM_VERIFY

#define NVM_PAGE_SIZE			512

#define HIDEEP_NVM_DOWNLOAD		0x10000000

/* register map */
#define YRAM_BASE			(0x40000000)
#define PERIPHERAL_BASE			(0x50000000)
#define ESI_BASE			(PERIPHERAL_BASE + 0x00000000)
#define FLASH_BASE			(PERIPHERAL_BASE + 0x01000000)
#define SYSCON_BASE			(PERIPHERAL_BASE + 0x02000000)

#define SYSCON_MOD_CON			(SYSCON_BASE + 0x0000)
#define SYSCON_SPC_CON			(SYSCON_BASE + 0x0004)
#define SYSCON_CLK_CON			(SYSCON_BASE + 0x0008)
#define SYSCON_CLK_ENA			(SYSCON_BASE + 0x000C)
#define SYSCON_RST_CON			(SYSCON_BASE + 0x0010)
#define SYSCON_WDT_CON			(SYSCON_BASE + 0x0014)
#define SYSCON_WDT_CNT			(SYSCON_BASE + 0x0018)
#define SYSCON_PWR_CON			(SYSCON_BASE + 0x0020)
#define SYSCON_PGM_ID			(SYSCON_BASE + 0x00F4)
#define SYSCON_CHIP_ID			(SYSCON_BASE + 0x007C)

#define FLASH_CON			(FLASH_BASE + 0x0000)
#define FLASH_STA			(FLASH_BASE + 0x0004)
#define FLASH_CFG			(FLASH_BASE + 0x0008)
#define FLASH_TIM			(FLASH_BASE + 0x000C)
#define FLASH_CACHE_CFG			(FLASH_BASE + 0x0010)
#define FLASH_ECC_CON			(FLASH_BASE + 0x0040)
#define ESI_TX_INVALID			(ESI_BASE + 0x0008)

#define NVM_MASK_OFS			0x0000000C


#define HIDEEP_BOOT_SECTION		0x00000400
#define HIDEEP_BOOT_LEN			0x00000400
#define HIDEEP_DWZ_SECTION		0x00000280
#define HIDEEP_DWZ_INFO_OFS		0x000002C0
#define HIDEEP_DWZ_LEN			(HIDEEP_BOOT_SECTION - HIDEEP_DWZ_SECTION)

#define GET_PAYLOAD_SIZE_FROM_HEADER(x) ((x[1] >> 6) ? ((x[2] * 256 + x[3]) * (x[7] + 1)) : (x[2] * x[3] * (x[7] + 1)))

typedef struct pgm_packet {
	union {
		unsigned char b[8];
		unsigned int w[2];
	} header;

	unsigned int payload[512 / sizeof(unsigned int)];
} pgm_packet_t;

/* Device Driver <==> Application */
/* lseek(), write(), read() command */
#define HIDEEP_RELEASE_FLAG		0x8000
#define HIDEEP_VR_ADDR_LEN		0xFFFF
#define HIDEEP_MAX_RAW_LEN		0x3000
#define HIDEEP_READ_WRITE_VR		0xF000
#define HIDEEP_I2C_BYPASS		0x20000000

#define MAX_VR_BUFF			2048
#define MAX_RAW_SIZE			7369 // max tx * max rx * 2 + 8 + 1, 1 is magin size
#endif

#define HIDEEP_READ_DATA_READY_CHECK	0x0002
#define HIDEEP_READ_IC_INFO		0x0070
#define HIDEEP_READ_HEADER_INFO		0x1000
#define HIDEEP_CMD_CONNECTION_RESULT_REG	0x1008
#define HIDEEP_DATA_READY_RETURN	0x01
#define HIDEEP_TSP_CONNECTION_PASS	0x300D
#define HIDEEP_TSP_CONNECTION_FAIL	0xFA11

#define HIDEEP_MODE_STATE_CHECK_REG	0x0025

enum switch_power_mode {
	TO_TOUCH_MODE			= 0,
	TO_LOWPOWER_MODE		= 1,
};

enum ic_state_mode {
	ACTIVE_NOMAL			= 0,
	ACTIVE_IDLE,
	LPM_IDLE,
};

int hideep_debug_init(struct hideep_t *ts);
void hideep_debug_uninit(void);

int hideep_sysfs_init(struct hideep_t *ts);
int hideep_sysfs_exit(struct hideep_t *ts);

void hideep_isp_init(struct hideep_t *ts);
int hideep_resume(struct device *dev);
int hideep_suspend(struct device *dev);

int sec_ts_fn_init(struct hideep_t *ts);
void sec_ts_fn_remove(struct hideep_t *ts);
int hideep_firmware_update_on_hidden_menu(struct hideep_t *ts, int update_type);

int hideep_fix_active_mode(struct hideep_t *ts, u8 enable);
void hideep_run_rawdata_read_all(struct hideep_t *ts);

int hideep_set_lowpowermode(struct hideep_t *ts, u8 mode);
int hideep_start_device(struct hideep_t *ts);
int hideep_stop_device(struct hideep_t *ts);

