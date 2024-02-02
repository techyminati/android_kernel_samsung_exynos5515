/*
 *  Copyright (C) 2011, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __SSP_PRJ_H__
#define __SSP_PRJ_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/iio/iio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
//#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/rtc.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
//#include <linux/battery/sec_battery.h>  // serene_need_fix
#include <linux/fs.h>
#ifndef CONFIG_OF
#include <linux/ssp_platformdata.h>
#endif

#ifdef CONFIG_SLEEP_MONITOR
#include <linux/power/sleep_monitor.h>
#endif
#ifdef CONFIG_PANEL_NOTIFY
#include <linux/panel_notify.h>
#endif
#include "factory/ssp_factory.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#undef CONFIG_HAS_EARLYSUSPEND
#endif

#ifdef CONFIG_SENSORS_SSP_CHUB
#include "ssp_platform.h"
#endif
#include "../../staging/nanohub/chub_log.h"

/* For communication with sensorhub */
#include "ssp_protocol.h"

#define SSP_DBG		1

#define SUCCESS		1
#define FAIL		0
#define ERROR		-1

#if SSP_DBG
#define SSP_FUNC_DBG 1
#define SSP_DATA_DBG 0

/* ssp mcu device ID */
#define DEVICE_ID		0x55

#define ssp_dbg(format, ...) do { \
		printk(KERN_INFO format, ##__VA_ARGS__); \
		} while (0)
#else
#define ssp_dbg(format, ...)
#endif

#define ssp_info(fmt, ...) do { \
		pr_info("[SSP] " fmt "\n", ##__VA_ARGS__); \
		} while (0)

#define ssp_err(fmt, ...) do { \
		pr_err("[SSP] " fmt "\n", ##__VA_ARGS__); \
		} while (0)

#define ssp_dbgf(fmt, ...) do { \
		pr_debug("[SSP] %20s(%4d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
		} while (0)

#define ssp_infof(fmt, ...) do { \
		pr_info("[SSP] %20s(%4d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
		} while (0)

#define ssp_errf(fmt, ...) do { \
		pr_err("[SSP] %20s(%4d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
		} while (0)

#if SSP_FUNC_DBG
#define func_dbg() do { \
	printk(KERN_INFO "[SSP]: %s\n", __func__); \
	} while (0)
#else
#define func_dbg()
#endif

#if SSP_DATA_DBG
#define data_dbg(format, ...) do { \
	printk(KERN_INFO format, ##__VA_ARGS__); \
	} while (0)
#else
#define data_dbg(format, ...)
#endif

#define SSP_SW_RESET_TIME		3000
#define DEFAULT_POLLING_DELAY	(200 * NSEC_PER_MSEC)
#define PROX_AVG_READ_NUM		80
#define DEFAULT_RETRIES			3
#define DATA_PACKET_SIZE		960

#define DEFAULT_MODEL_NAME	"S.LSI_CHUB"

#define SSP_STATUS_MONITOR	0
#if SSP_STATUS_MONITOR
#define SSP_MONITOR_TIME	4
#endif


/* SSP Binary Type */
enum {
	KERNEL_BINARY = 0,
	KERNEL_CRASHED_BINARY,
	UMS_BINARY,
};

/*
 * SENSOR_DELAY_SET_STATE
 * Check delay set to avoid sending ADD instruction twice
 */
enum {
	INITIALIZATION_STATE = 0,
	NO_SENSOR_STATE,
	ADD_SENSOR_STATE,
	RUNNING_SENSOR_STATE,
};

/* Firmware download STATE */
enum {
	FW_DL_STATE_FAIL = -1,
	FW_DL_STATE_NONE = 0,
	FW_DL_STATE_NEED_TO_SCHEDULE,
	FW_DL_STATE_SCHEDULED,
	FW_DL_STATE_DOWNLOADING,
	FW_DL_STATE_SYNC,
	FW_DL_STATE_DONE,
};

/* for MSG2SSP_AP_GET_THERM */
enum {
	ADC_BATT = 0,
	ADC_CHG,
};

enum {
	SENSORS_BATCH_DRY_RUN               = 0x00000001,
	SENSORS_BATCH_WAKE_UPON_FIFO_FULL   = 0x00000002
};

enum {
	META_DATA_FLUSH_COMPLETE = 1,
};

enum {
	RESET_TYPE_KERNEL_NO_EVENT = 0,
	RESET_TYPE_KERNEL_COM_FAIL,
	RESET_TYPE_KERNEL_SYSFS,
	RESET_TYPE_CHUB_NO_EVENT,
	RESET_TYPE_CHUB_CRASHED,
	RESET_TYPE_CHUB_SILENT_RESET,
	RESET_TYPE_MAX,
};

#if defined(CONFIG_PANEL_NOTIFY)
enum {
	SSP_UB_CON_CONNECTED = PANEL_EVENT_UB_CON_CONNECTED,
	SSP_UB_CON_DISCONNECTED = PANEL_EVENT_UB_CON_DISCONNECTED,
	SSP_UB_CON_UNKNOWN = SSP_UB_CON_DISCONNECTED+1,
};
#endif


#define SSP_INVALID_REVISION		99999
#define SSP_INVALID_REVISION2		0xFFFFFF

/* Gyroscope DPS */
#define GYROSCOPE_DPS250		250
#define GYROSCOPE_DPS500		500
#define GYROSCOPE_DPS1000		1000
#define GYROSCOPE_DPS2000		2000

/* Gesture Sensor Current */
#define DEFUALT_IR_CURRENT		100

/* Magnetic */
#define PDC_SIZE			27

/* kernel -> ssp manager cmd*/
#define SSP_LIBRARY_SLEEP_CMD		(1 << 5)
#define SSP_LIBRARY_LARGE_DATA_CMD	(1 << 6)
#define SSP_LIBRARY_WAKEUP_CMD		(1 << 7)

/* voice data */
#define TYPE_WAKE_UP_VOICE_SERVICE				0x01
#define TYPE_WAKE_UP_VOICE_SOUND_SOURCE_AM		0x01
#define TYPE_WAKE_UP_VOICE_SOUND_SOURCE_GRAMMER	0x02

/* wom data allocated 0x5 ~ 0xA */
#define TYPE_WOM_IMAGE_TYPE_BACKGROUND	0x5
#define TYPE_WOM_IMAGE_TYPE_MAX			0xA

/* Sensors's reporting mode */
#define REPORT_MODE_CONTINUOUS 0
#define REPORT_MODE_ON_CHANGE  1
#define REPORT_MODE_SPECIAL    2
#define REPORT_MODE_UNKNOWN    3

/* ap status */
#define SSP_AP_STATUS_RESUME		0xE1
#define SSP_AP_STATUS_SUSPEND		0xE2

/* Factory data length */
#define FACTORY_DATA_MAX	100
#define MCU_SLEEP_FACTORY_DATA_LENGTH	FACTORY_DATA_MAX
#define HRM_LIB_VERSION_INFO_LENGTH		10
#define HRM_IR_LEVEL_THRESHOLD_LENGTH	4
#define HRM_DQA_LENGTH					22

#define HRM_EOL_DATA_SIZE		24
#define HRM_XTALK2_DATA_SIZE	24
#define HRM_ECG_FAC_DATA_SIZE	10
#define HRM_ECG_LIB_DATA_SIZE	32

#define BIA_CAL_DATA_SIZE		32
#define HRM_BIA_RAW_DATA_SIZE	32

#define TEMPERATURE_DATA_SIZE	32

#define DIGITAL_HALL_CAL_DATA_SIZE	19
#define MAGNETIC_CAL_DATA_SIZE 		216 //72*3

#define SCONTEXT_DATA_SIZE		(128) //(4096) // 64

// Sensorhub dump length
#define SENSORHUB_REG_DUMP_SIZE		(256-9) //DATA_PACKET_SIZE//1365//300

/* SENSOR_TYPE */
enum {
	ACCELEROMETER_SENSOR = 0,       /* [0]=1 */
	GYROSCOPE_SENSOR,               /* [1]=2 */
	ACCELEROMETER_SENSOR_32G,       /* [2]=4 */
	DIGITAL_HALL_RAW_SENSOR,       	/* [3]=8 */
	DIGITAL_HALL_EVENT_SENSOR,		/* [4]=16 */
	PRESSURE_SENSOR,                /* [5]=32 */
	GESTURE_SENSOR,                 /* [6]=64 */
	DIGITAL_HALL_SUB_SENSOR,		/* [7]=128 */
	SENSOR8,						/* [8]=256 */
	LIGHT_SENSOR,                   /* [9]=512 */
	AUTO_BRIGHTNESS_SENSOR,         /* [10]=1024 */
	ORIENTATION_SENSOR,             /* [11]=2048 */
	ROTATION_VECTOR_SENSOR,         /* [12]=4096 */
	GAME_ROTATION_VECTOR_SENSOR,    /* [13]=8192 */
	GYRO_UNCALIB_SENSOR,            /* [14]=16384 */
	LINEAR_ACCEL_SENSOR,            /* [15]=32768 */
	SENSOR16,                       /* [16]=65536 */
	HRM_ECG_LIB_SENSOR,             /* [17]=131072 */
	HRM_RAW_SENSOR,                 /* [18]=262144 */
	HRM_RAW_FAC_SENSOR,             /* [19]=524288 */
	HRM_LIB_SENSOR,                 /* [20]=1048576 */
	HRM_RAW_FAC2_SENSOR,            /* [21]=2097152 */
	HRM_BIA_RAW_SENSOR,             /* [22]=4194304 */
	TEMPERATURE_SENSOR,                    /* [23]=8388608 */
	GEOMAGNETIC_UNCALIB_SENSOR,     /* [24]=16777216 */
	GEOMAGNETIC_CALIB_SENSOR,       /* [25]=33554432 */
	GEOMAGNETIC_POWER_SENSOR,       /* [26]=67108864 */
	SENSOR27,                       /* [27]=134217728 */
	THERMISTER_SENSOR,          	/* [28]=268435456 */
	SENSOR29,                       /* [29]=536870912 */
	GRIP_SENSOR,                    /* [30]=1073741824 */
	GRIP_RAW_SENSOR,                /* [31]=2147483648 */
	SENSOR_TYPE_SCONTEXT_T,         /* [32] Not real sensor  */
	SENSOR_MAX,
};

#define SENSOR_TYPE_MAX                 SENSOR_MAX
#define SENSOR_NAME_MAX_LEN             35

struct meta_data_event {
	s32 what;
	s32 sensor;
} __attribute__((__packed__));

struct sensor_value {
	union {
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
		struct {		/*calibrated mag, gyro*/
			s16 cal_x;
			s16 cal_y;
			s16 cal_z;
			u8 accuracy;
		};
		struct {		/*uncalibrated mag, gyro*/
			s16 uncal_x;
			s16 uncal_y;
			s16 uncal_z;
			s16 offset_x;
			s16 offset_y;
			s16 offset_z;
		};
		struct {		/*uncalibrated mag 32bit*/
			s32 uncal_mag32_x;
			s32 uncal_mag32_y;
			s32 uncal_mag32_z;
			s32 offset_mag32_x;
			s32 offset_mag32_y;
			s32 offset_mag32_z;
		};
		struct {		/* rotation vector */
			s32 quat_a;
			s32 quat_b;
			s32 quat_c;
			s32 quat_d;
			u8 acc_rot;
		};
#ifdef CONFIG_SENSORS_SSP_LIGHT_SENSOR
#if defined(CONFIG_SENSORS_SSP_OPT3007)
		struct {		/* light sensor */
			u16 rdata;
			u32 gain;
		};
#elif defined(CONFIG_SENSORS_SSP_TSL2584)
		struct {		/* light sensor */
			u8 ch0_lower;
			u8 ch0_upper;
			u8 ch1_lower;
			u8 ch1_upper;
			u32 gain;
		};
#elif defined(CONFIG_SENSORS_SSP_CM3323)
		struct {
			u16 r;
			u16 g;
			u16 b;
			u16 w;
		};
#endif
		struct {
			s32 lux;
			u8 flag;
		};
#endif
		struct {
			u32 hrm_raw_value1;
			u32 hrm_raw_value2;
			u32 hrm_raw_value3;
			u32 hrm_raw_value4;
			u32 hrm_raw_value5;
			u32 hrm_raw_value6;
			u32 hrm_raw_value7;
			u32 hrm_raw_value8;
			u32 hrm_raw_value9;
			u32 hrm_raw_value10;
			u32 hrm_raw_value11;
			u32 hrm_raw_value12;
			u32 hrm_raw_value13;
			u32 hrm_raw_value14;
			u32 hrm_raw_value15;
			u32 hrm_raw_value16;
		};
		struct {
			s32 ecg_lead_off;
			s32 ecg_ext_noise;
			s32 ecg_amp;
			s32 ecg_freq;
			s32 ecg_trim;
			s32 reserved1;
			s32 reserved2;
			s32 reserved3;
			s32 reserved4;
			s32 reserved5;
		};
		struct {
			s32 ecg_data_value[HRM_ECG_LIB_DATA_SIZE];
		};
		struct {
			s32 bia_data_value[HRM_BIA_RAW_DATA_SIZE];
		};
		struct {
			s32 temperature_data_value[TEMPERATURE_DATA_SIZE];
		};
		struct {
			s16 hr;
			s16 rri;
			s32 snr;
		};
/* hrm_eol_data
 *	ADI						TI
 *	frequency				ir_low_dc
 *	green_dc_level			ir_high_dc
 *	green_high_dc_level		ir_ac
 *	green_mid_dc_level		ir_acdc_ratio
 *	red_dc_levet			ir_noise
 *	ir_dc_level				ir_frequency
 *	noise_level				green_low_dc
 *	adc_offset[0]			green_high_dc
 *	adc_offset[1]			green_ac
 *	adc_offset[2]			green_acdc_ratio
 *	adc_offset[3]			green_noise
 *	adc_offset[4]			placeholder
 *	adc_offset[5]			placeholder
 *	adc_offset[6]			placeholder
 *	adc_offset[7]			placeholder
 *	oscRegValue				placeholder
 */
		u32 hrm_eol_data[HRM_EOL_DATA_SIZE];
		u32 hrm_xtalk2_data[HRM_XTALK2_DATA_SIZE];
		s32 hrm_ecg_fac_data[HRM_ECG_FAC_DATA_SIZE];
		s32 hrm_ecg_lib_data[HRM_ECG_LIB_DATA_SIZE];
		s32 hrm_bia_raw_data[HRM_BIA_RAW_DATA_SIZE];
		u8 front_hrm_raw[6];
		u8 ecg_data[6];
		u8 temperature_raw_data[64];
#ifdef CONFIG_SENSORS_SSP_SKIN_TEMP
		struct {
			s16 skin_temp;
			s16 skin_humid;
			s16 env_temp;
			s16 env_humid;
		};
#else
		struct { /* humi/temp sensor */
			u16 temp;
			u16 humi;
			u8 time;
		};
#endif
		struct {
			u32 uv_raw;
			u32 hrm_temp;
		};
		u8 data[20];
		s32 pressure[3];
		struct {
			u32 ohm;
			u16 adc;
			u8 inj_c;
		};
		struct {
			u32 data1;
			u16 data2;
			u16 data3;
			u8	data4;
		};
		struct { /*geomagnetic_uncal*/
			s16 uncal_data[3];
			s16 uncal_offset[3];
			u8 uncal_status;
		};
		struct { /*geomagnetic_uncal 32bit*/
			s32 uncal_mag32_data[3];
			s32 uncal_mag32_offset[3];
			u8 uncal_mag32_status;
		};
		struct { /*geomagnetic_cal*/
			s16 calib_data[3];
			s8 calib_accuracy;
			u8 calib_status;
		};
		struct { /*geomagnetic_power*/
			s16 power_data[3];
		};
#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR
		struct {
			s8 direction;
			u8 interrupt;
			s16 angle_evt;
		};
		struct {
			s16 angle;
		};
#endif
		struct meta_data_event meta_data;
	};
	u64 timestamp;
} __attribute__((__packed__));

extern struct class *sensors_event_class;

struct sensor_delay {
	int sampling_period;	/* delay (ms)*/
	int max_report_latency;	 /* batch_max_latency*/
};

struct sensor_info_ssp {
	char name[SENSOR_NAME_MAX_LEN];
	bool enable;
	int report_mode;
	int get_data_len;
	int report_data_len;
};

struct calibraion_data {
	s16 x;
	s16 y;
	s16 z;
};

struct hw_offset_data {
	char x;
	char y;
	char z;
};

struct light_calibration_data {
	s32 cal_status;
	s32 cal_lux;
	s32 curr_lux;
} __attribute__((__packed__));

struct magnetic_calibration_data {
	s16 cal_status;
	u64 duration;
	s32 cal_data[MAGNETIC_CAL_DATA_SIZE];
} __attribute__((__packed__));

struct magnetic_status {
	u8 calibration;
	u8 lookup_flag;
	u64 start_time;
	u64 end_time;
} __attribute__((__packed__));

struct digital_hall_caldata {
	u32 flg_update;
	s32 x[DIGITAL_HALL_CAL_DATA_SIZE];
	s32 y[DIGITAL_HALL_CAL_DATA_SIZE];
	s32 z[DIGITAL_HALL_CAL_DATA_SIZE];
};

/* ssp_msg options bit*/
#define SSP_RETURN		2	/* write and read option */
#define SSP_GYRO_DPS	3	/* gyro dps mask */
#define SSP_INDEX		3	/* data index mask */

#define SSP_SPI_MASK		(3 << SSP_SPI)	/* read write mask */
#define SSP_GYRO_DPS_MASK	(3 << SSP_GYRO_DPS)
/* dump index mask. Index is up to 8191 */
#define SSP_INDEX_MASK		(8191 << SSP_INDEX)

struct ssp_msg {
	u8 cmd;
	u16 length;
	u16 options;
	u32 data;

	struct list_head list;
	struct completion *done;
	char *buffer;
	u8 free_buffer;
	bool *dead_hook;
	bool dead;
	bool clean_pending_list_flag;
} __attribute__((__packed__));

enum {
	AP2HUB_READ = 0,
	AP2HUB_WRITE,
	HUB2AP_WRITE,
	AP2HUB_READY,
	AP2HUB_RETURN
};

enum {
	BIG_TYPE_DUMP = 0,
	BIG_TYPE_READ_LIB,
	/*+snamy.jeong 0706 for voice model download & pcm dump*/
	BIG_TYPE_VOICE_NET,
	BIG_TYPE_VOICE_GRAM,
	BIG_TYPE_VOICE_PCM,
	/*-snamy.jeong 0706 for voice model download & pcm dump*/
	BIG_TYPE_TEMP,
	/* Watch Only Mode */
	BIG_TYPE_WOM = 10,
	BIG_TYPE_MAX,
};

enum {
	BATCH_MODE_NONE = 0,
	BATCH_MODE_RUN,
};

struct ssp_time_diff {
	u16 batch_count;
	u16 batch_mode;
	u64 time_diff;
	u64 irq_diff;
	u16 batch_count_fixed;
};

struct ssp_big_event {
	char *library_data;
	int library_length;
};

struct ssp_data {
	struct wakeup_source *ssp_wake_lock;
	struct wakeup_source *ssp_comm_wake_lock;
	struct timer_list debug_timer;
	struct timer_list ts_sync_timer;
	struct workqueue_struct *debug_wq;
	struct workqueue_struct *ts_sync_wq;
	struct workqueue_struct *dump_wq;
	struct workqueue_struct *lpm_motion_wq;
	struct workqueue_struct *ts_diff_sync_wq;
	struct work_struct work_debug;
	struct work_struct work_ts_sync;
	struct work_struct work_lpm_motion;
	struct work_struct work_reset_noti;
	struct work_struct work_ts_diff_sync;

#ifdef CONFIG_SENSORS_SSP_VIB_NOTIFY
	struct workqueue_struct *ssp_motor_wq;
	struct work_struct work_ssp_motor;
	u16 motor_duration;
	int motor_flag;
#endif

	bool is_probe_done;
	struct delayed_work work_power_on;
	void *platform_data;
	bool is_reset_started;
	int reset_type;
	int chub_err_type;
	/* sensor hub dump */
	int dump_index;
	int log_index;
	unsigned int uLogWIdx;
	bool is_log_full;

#if SSP_STATUS_MONITOR
	struct delayed_work polling_work;
#endif
	struct calibraion_data accelcal;
	struct calibraion_data gyrocal;
	struct hw_offset_data magoffset;
	struct digital_hall_caldata hallcal;
	struct light_calibration_data lightcal;
	u32 hrmcal[HRM_EOL_DATA_SIZE];
	u32 hrmcal2[HRM_XTALK2_DATA_SIZE];
	s32 hrm_ecg_fac[HRM_ECG_FAC_DATA_SIZE];
	s32 hrm_ecg_lib[HRM_ECG_LIB_DATA_SIZE];
	s32 biacal[BIA_CAL_DATA_SIZE];
	s32 hrm_bia_raw[HRM_BIA_RAW_DATA_SIZE];   // bia_raw_show
	struct sensor_value buf[SENSOR_MAX];
	struct sensor_delay delay[SENSOR_TYPE_MAX];
	struct sensor_info_ssp info[SENSOR_TYPE_MAX];
	struct iio_dev *indio_devs[SENSOR_TYPE_MAX];
	struct iio_chan_spec indio_channels[SENSOR_TYPE_MAX];
	struct miscdevice scontext_device;

	struct device *sen_dev;
	struct device *mcu_device;
	struct device *acc_device;
	struct device *acc32g_device;
	struct device *gyro_device;
	struct device *mag_device;
	struct device *prs_device;
	struct device *light_device;
	struct device *temphumidity_device;
	struct device *hrm_device;
	struct device *hrm_ecg_device;
	struct device *front_hrm_device;
	struct device *uv_device;
	struct device *gsr_device;
	struct device *ecg_device;
	struct device *grip_device;
	struct device *bia_device;
	struct device *dhallic_device;
	struct device *thermistor_device;
	struct device *temperature_device;

	struct delayed_work work_firmware;
	struct delayed_work work_refresh;
	struct miscdevice shtc1_device;
	struct miscdevice batch_io_device;
#ifdef CONFIG_SENSORS_SSP_INPUT_DEV
	struct input_dev *hallic_input_dev;
	struct input_dev *motion_input_dev;
#endif

/*snamy.jeong@samsung.com temporary code for voice data sending to mcu*/
	struct device *voice_device;

	bool bSspShutdown;
	bool bSspReady;
	bool bAccelAlert;
	bool bAccelAlert1;
	bool bGeomagneticRawEnabled;
	bool bMcuDumpMode;
	bool bProbeIsDone;
	bool bDumping;
	bool bTimeSyncing;
	bool bLpModeEnabled;
	bool reportedData[SENSOR_MAX];
#if SSP_STATUS_MONITOR
	bool bRefreshing;
#endif
	bool bWOMode;

	int aiCheckStatus[SENSOR_MAX];

	unsigned int uComFailCnt;
	unsigned int uResetCnt;
	unsigned int uTimeOutCnt;
	unsigned int uIrqCnt;
#if SSP_STATUS_MONITOR
	unsigned int uSubIrqCnt;
#endif
	unsigned int uDumpCnt;

	u8 uCmdCtl;

	int sealevelpressure;
	unsigned int uGyroDps;
	u64 uSensorState;
	u64 uLibraryState[2];
	unsigned int uFeatureList;
	char uSsrStuckInfo[4];

	unsigned int uCurFirmRev;
	unsigned int uWOMFirmRev;

	char uLastResumeState;
	char uLastAPState;
	s32 iPressureCal;

	atomic64_t aSensorEnable;
	int64_t adDelayBuf[SENSOR_MAX];
	s32 batchLatencyBuf[SENSOR_MAX];
	s8 batchOptBuf[SENSOR_MAX];

	int (*set_mcu_reset)(int);
	int (*check_lpmode)(void);
	atomic_t eol_enable;
	atomic_t xtalk2_enable;
	int (*hrm_sensor_power)(int);

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int ap_rev;
	int accel_position;
	int mag_position;
	int fw_dl_state;
	u16 model_number;
	u8 mag_matrix_size;
	u8 *mag_matrix;
#ifdef CONFIG_SENSORS_SSP_MAGNETIC_SENSOR
	unsigned char pdc_matrix[PDC_SIZE];
	unsigned char uFuseRomData[3];
	unsigned char uMagCntlRegData;
	unsigned char geomag_cntl_regdata;
	struct magnetic_calibration_data magcal;
	struct magnetic_status mag_status;
#endif
	const char *fw_name;
	struct mutex comm_mutex;
	struct mutex pending_mutex;
	unsigned int cnt_timeout;
	unsigned int cnt_com_fail;

	/* for factory sysfs nodes reliability */
	struct mutex sysfs_op_mtx;

	struct mutex file_mutex;

#ifdef CONFIG_SENSORS_SSP_OPT3007
	s32 lux;
#else
	u32 lux;
#endif
	//int mcu_int1;
	int mcu_int2;
	int ap_int;
	int rst;
	int chg;
	struct regulator *reg_hub;
	struct regulator *reg_sns;	/* regulator ctnl for each sensor */
	/* for VDD_AUX_IN of BCM477x, optional */
	struct regulator *reg_aux;
	struct regulator *reg_hrm1p8v;	/* regulator ctnl for hrm sensor */
	struct regulator *reg_hrm3p3v;
	struct regulator *reg_hrm5v;

	struct list_head pending_list;
	void (*ssp_big_task[BIG_TYPE_MAX])(struct work_struct *);
	u64 timestamp;
	struct file *realtime_dump_file;
	int total_dump_size;
#ifdef CONFIG_SLEEP_MONITOR
	long long service_mask;
#endif
	u8 lpm_int_mode;
	s8 lpm_rotation_info;

	const char *model_name;
	int fw_type;
	int ap_hw_rev;
	int chub_fw_rev;
	int irq_gpio;
	int irq;
	int irq_state;
	bool irq_init_done;
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*gpio_config;
	struct pinctrl_state	*hrm_pu;
	struct pinctrl_state	*hrm_pd;
	struct pinctrl_state	*hrm_spi;
	struct pinctrl_state	*afe_sel_np;
	int irq_cnt;

#if defined(CONFIG_PANEL_NOTIFY)
	struct workqueue_struct *ssp_panel_wq;
	struct delayed_work work_ssp_panel;
	int ub_con_state;
#endif
#ifdef CONFIG_SENSORS_SSP_THERMISTOR_SENSOR
	s32 threshold[3];
#endif
	bool wheel_test;
	bool wheel_moved;
	unsigned int uWheelCnt;
	u64 debug_enable;
	struct ssp_big_event big_events;

	int lcd_type;
	int hrm_int;
	int afe_sel;
	int hrm_5v_onoff;
	bool hrm_int_init_done;

	u8 ts_diff[9];
	u8 reg_dump[SENSORHUB_REG_DUMP_SIZE];
	u8 afe_sel_reading;
	u8 hrm_vender;
	u8 hrm_reading1;
	u8 hrm_reading2;
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	u16 sw_offset_matrix[2];
	int sw_offset;
	int convert_coef;
#endif
	u8 bypass_logcnt;
};

struct ssp_big {
	struct ssp_data *data;
	struct work_struct work;
	u32 length;
	u32 addr;
	u8 bigType;
};

struct ssp_dump {
	struct ssp_data *data;
	struct work_struct work;
	char *dump_data;
	u32 length;
};

extern struct ssp_data *sensorhub_data;


static inline struct wakeup_source *ssp_wake_lock_init(struct device *dev, const char *name)
{
	struct wakeup_source *ws = NULL;

	ws = wakeup_source_register(dev, name);
	if (ws == NULL) {
		ssp_err("%s: wakelock register fail\n", name);
		return NULL;
	}

	return ws;
}

static inline void ssp_wake_lock_destroy(struct wakeup_source *ws)
{
	if (ws == NULL) {
		ssp_err("wakelock unregister fail\n");
		return;
	}

	wakeup_source_unregister(ws);
}

static inline void ssp_wake_lock(struct wakeup_source *ws)
{
	if (ws == NULL) {
		ssp_err("wakelock fail\n");
		return;
	}

	__pm_stay_awake(ws);
}

static inline void ssp_wake_lock_timeout(struct wakeup_source *ws, long timeout)
{
	if (ws == NULL) {
		ssp_err("wakelock timeout fail\n");
		return;
	}

	__pm_wakeup_event(ws, jiffies_to_msecs(timeout));
}

static inline void ssp_wake_unlock(struct wakeup_source *ws)
{
	if (ws == NULL) {
		ssp_err("wake unlock fail\n");
		return;
	}

	__pm_relax(ws);
}

static inline int ssp_wake_lock_active(struct wakeup_source *ws)
{
	if (ws == NULL) {
		ssp_err("wake unlock fail\n");
		return 0;
	}

	return ws->active;
}


void ssp_enable(struct ssp_data *, bool);
void ssp_enable_irq(struct ssp_data *data, bool enable);
int ssp_suspend(struct ssp_data *data, struct device *dev);
int ssp_resume(struct ssp_data *data, struct device *dev);
int ssp_send_command(struct ssp_data *data, u8 cmd, u16 options, int timeout,
						char *send_buf, int send_buf_len, char **receive_buf, int *receive_buf_len, u32 send_data);
int ssp_send_write_cmd(struct ssp_data *data, u32 cmd, u32 subcmd,
						char *sendbuf, u16 length, const char *func_name);
int ssp_send_read_cmd(struct ssp_data *data, u32 cmd, u32 subcmd,
						char *buf, u16 length, int timeout, int expectlen, const char *func_name);
int ssp_send_write_factory_cmd(struct ssp_data *data, u16 sensor_type, u32 subcmd,
						char *sendbuf, u16 length, const char *func_name);
int ssp_send_read_factory_cmd(struct ssp_data *data, u16 sensor_type, u32 subcmd,
						char *buf, u16 length, int timeout, int expectlen, const char *func_name);

void clean_pending_list(struct ssp_data *);
void toggle_mcu_reset(struct ssp_data *);
int initialize_mcu(struct ssp_data *);
int initialize_sysfs(struct ssp_data *);
void initialize_function_pointer(struct ssp_data *);
int get_sensor_state(struct ssp_data *data, u16 sensor_type, const char *func_name);

void report_meta_data(struct ssp_data *, struct sensor_value *);
#ifdef CONFIG_SENSORS_SSP_ACCELEROMETER_SENSOR
int accel_open_calibration(struct ssp_data *);
int set_accel_cal(struct ssp_data *);
#endif

#ifdef CONFIG_SENSORS_SSP_GYRO_SENSOR
int gyro_open_calibration(struct ssp_data *);
int set_gyro_cal(struct ssp_data *);
int save_gyro_caldata(struct ssp_data *, s16 *);
#endif

#ifdef CONFIG_SENSORS_SSP_PRESSURE_SENSOR
int pressure_open_calibration(struct ssp_data *);
int set_pressure_cal(struct ssp_data *);
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
int open_pressure_sw_offset_file(struct ssp_data *);
#endif
#endif

#ifdef CONFIG_SENSORS_SSP_HRM_SENSOR
int hrm_open_calibration(struct ssp_data *);
int set_hrm_calibration(struct ssp_data *);
#endif

#ifdef CONFIG_SENSORS_SSP_BIA_SENSOR
int bia_open_calibration(struct ssp_data *);
int set_bia_calibration(struct ssp_data *);
#endif

void initialize_factorytest(struct ssp_data *);
void remove_factorytest(struct ssp_data *);
void sensors_remove_symlink(struct input_dev *);
void destroy_sensor_class(void);
int sensors_create_symlink(struct input_dev *);
void remove_sysfs(struct ssp_data *);
int ssp_send_cmd(struct ssp_data *, char, int);
int send_instruction(struct ssp_data *, u8, u8, u8 *, u16);
int send_instruction_sync(struct ssp_data *, u8, u8, u8 *, u16);
int get_batch_count(struct ssp_data *, u8);
int select_irq_msg(struct ssp_data *);
int get_chipid(struct ssp_data *);
int set_sensor_position(struct ssp_data *);
void sync_sensor_state(struct ssp_data *);
int get_msdelay(int64_t);
u64 get_sensor_scanning_info(struct ssp_data *);
unsigned int get_firmware_rev(struct ssp_data *);
unsigned int get_feature_list(struct ssp_data *);
unsigned int get_ssr_stuck_info(struct ssp_data *data);
int set_model_number(struct ssp_data *data);
int set_lcd_panel_type(struct ssp_data *data);
#ifdef CONFIG_SSP_RTC
unsigned int get_rtc_diff(struct ssp_data *data);
#endif
int parse_dataframe(struct ssp_data *, char *, int);
void enable_debug_timer(struct ssp_data *);
void disable_debug_timer(struct ssp_data *);
int initialize_debug_timer(struct ssp_data *);
void ssp_timestamp_sync_cmd(struct ssp_data *);
void enable_timestamp_sync_timer(struct ssp_data *);
void disable_timestamp_sync_timer(struct ssp_data *);
int initialize_timestamp_sync_timer(struct ssp_data *);
int initialize_timestamp_diff_sync_work(struct ssp_data *data);
int handle_dump_data(struct ssp_data *, char *, int);
#if SSP_STATUS_MONITOR
int initialize_polling_work(struct ssp_data *);
#endif
int print_mcu_debug(char *, int *, int, struct ssp_data *data);
void print_report_debug(struct ssp_data *data, struct sensor_value *event, int type);

unsigned int get_module_rev(struct ssp_data *data);
void reset_noti_task(struct work_struct *work);
void reset_mcu(struct ssp_data *, int);
void reset_mcu_recovery(struct ssp_data *, int);
int download_sensorhub_firmware(struct ssp_data *data, struct device *dev, void *addr);
int queue_refresh_task(struct ssp_data *data, int delay);
void convert_acc_data(s16 *);
void *sensors_register(struct device *, void *,
	struct device_attribute*[], char *);

void sensors_unregister(struct device *,
	struct device_attribute*[]);

ssize_t mcu_sensor_state(struct device *, struct device_attribute *, char *);
ssize_t mcu_reset_show(struct device *, struct device_attribute *, char *);
ssize_t mcu_ready_show(struct device *, struct device_attribute *, char *);
ssize_t mcu_revision_show(struct device *, struct device_attribute *, char *);
ssize_t mcu_update_kernel_bin_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_update_kernel_crashed_bin_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_factorytest_store(struct device *, struct device_attribute *,
	const char *, size_t);
ssize_t mcu_factorytest_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_model_name_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_sleep_factorytest_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_sleep_factorytest_store(struct device *,
	struct device_attribute *, const char *, size_t);
ssize_t mcu_fota_rotate_status_show(struct device *,
	struct device_attribute *, char *);

unsigned int ssp_check_sec_dump_mode(void);

void recovery_mcu(struct ssp_data *data);
void ssp_read_big_library_task(struct work_struct *work);
void ssp_send_big_library_task(struct work_struct *work);


int set_time(struct ssp_data *);
int get_time(struct ssp_data *);
void handle_packet(struct ssp_data *data, char *packet, int packet_size);

struct ssp_data *ssp_probe(struct device *);

void ssp_dump_all_status(const char *buf, int st_alarm_cnt);
int sensorhub_dump_read(struct ssp_data *data, u8 type);

void refresh_task(struct work_struct *work);

#ifdef CONFIG_SENSORS_SSP_INPUT_DEV
int initialize_input_dev(struct ssp_data *);
void remove_input_dev(struct ssp_data *);
int initialize_event_symlink(struct ssp_data *);
void remove_event_symlink(struct ssp_data *);
void report_hallic_data(struct ssp_data *data, struct sensor_value *hidata);

int parse_mag_cal_data(struct ssp_data *, u8 *, u8);
void digital_hall_send_uevent(struct ssp_data *);

#endif

void print_gpm8(struct ssp_data *data);
void init_gpm8_sfrs(void);
#ifdef CONFIG_SENSORS_SSP_AP_WAKEUP
int ssp_enable_ap_wakeup(struct ssp_data *data, struct device *dev);
#endif
int check_hrm_vendor(struct ssp_data *data, struct device *dev);

#endif

