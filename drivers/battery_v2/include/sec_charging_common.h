/*
 * sec_charging_common.h
 * Samsung Mobile Charging Common Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
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
 */

#ifndef __SEC_CHARGING_COMMON_H
#define __SEC_CHARGING_COMMON_H __FILE__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>

#define MFC_LDO_ON		1
#define MFC_LDO_OFF		0

#define FOREACH_CHARGE_MODE(GEN_CHG_MODE) \
	GEN_CHG_MODE(SEC_BAT_CHG_MODE_BUCK_OFF) \
	GEN_CHG_MODE(SEC_BAT_CHG_MODE_CHARGING_OFF) \
	GEN_CHG_MODE(SEC_BAT_CHG_MODE_CHARGING) \
	GEN_CHG_MODE(SEC_BAT_CHG_MODE_MAX) \

#define GENERATE_CHG_MODE_ENUM(ENUM) ENUM,
#define GENERATE_CHG_MODE_STRING(STRING) #STRING,

enum CHARGE_MODE_ENUM {
	FOREACH_CHARGE_MODE(GENERATE_CHG_MODE_ENUM)
};

static const char *CHARGE_MODE_STRING[] = {
	FOREACH_CHARGE_MODE(GENERATE_CHG_MODE_STRING)
};

enum power_supply_ext_property {
	POWER_SUPPLY_EXT_PROP_CHECK_SLAVE_I2C = POWER_SUPPLY_PROP_MAX,
	POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE,
	POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_CMD,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VAL,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN,
	POWER_SUPPLY_EXT_PROP_WIRELESS_PHM,
	POWER_SUPPLY_EXT_PROP_AICL_CURRENT,
	POWER_SUPPLY_EXT_PROP_CHECK_MULTI_CHARGE,
	POWER_SUPPLY_EXT_PROP_CHIP_ID,
	POWER_SUPPLY_EXT_PROP_SYSOVLO,
	POWER_SUPPLY_EXT_PROP_VBAT_OVP,
	POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING,
	POWER_SUPPLY_EXT_PROP_FG_INBAT_VOLTAGE,

	POWER_SUPPLY_EXT_PROP_WATER_DETECT,
	POWER_SUPPLY_EXT_PROP_SURGE,

	POWER_SUPPLY_EXT_PROP_CHARGE_POWER,
	POWER_SUPPLY_EXT_PROP_MEASURE_SYS,
	POWER_SUPPLY_EXT_PROP_MEASURE_INPUT,

	POWER_SUPPLY_EXT_PROP_WC_CONTROL,
	POWER_SUPPLY_EXT_PROP_CHGINSEL,

	POWER_SUPPLY_EXT_PROP_MONITOR_WORK,
	POWER_SUPPLY_EXT_PROP_WDT_KICK,
	POWER_SUPPLY_EXT_PROP_WIRELESS_CHARGE_MONITOR,

	POWER_SUPPLY_EXT_PROP_UPDATE_BATTERY_DATA,
	POWER_SUPPLY_EXT_PROP_SLATE_MODE,
	POWER_SUPPLY_EXT_PROP_CHG_IN_OK,
	POWER_SUPPLY_EXT_PROP_FORCED_JIG_MODE,
	POWER_SUPPLY_EXT_PROP_STORE_MODE,
	POWER_SUPPLY_EXT_PROP_LONG_LIFE_STEP,
	POWER_SUPPLY_EXT_PROP_BOOT_BOOST_CHARGE,
	POWER_SUPPLY_EXT_PROP_POWER_SHARING_CHARGE,
	POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ_STRENGTH,
	POWER_SUPPLY_EXT_PROP_CHECK_DROP_CV,
	POWER_SUPPLY_EXT_PROP_FACTORY_MODE,
	POWER_SUPPLY_EXT_PROP_MUIC_MSG,
	POWER_SUPPLY_EXT_PROP_TX_PWR_BUDG,
	POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS,
};

enum power_supply_muic_msg {
	POWER_SUPPLY_MUIC_UNKNOWN = 0,
	POWER_SUPPLY_MUIC_ACOK_FALLING,
	POWER_SUPPLY_MUIC_ACOK_RISING,
};

enum power_supply_ext_health {
	POWER_SUPPLY_HEALTH_VSYS_OVP = POWER_SUPPLY_HEALTH_MAX,
	POWER_SUPPLY_HEALTH_VBAT_OVP,
};

enum sec_battery_cable {
	SB_CBL_UNKNOWN = 0,	/* default */
	SB_CBL_NONE,
	SB_CBL_TA,
	SB_CBL_USB,
	SB_CBL_UARTOFF,
	/* 5 ~ 9 : RESERVED */
	SB_CBL_WRL	= 10,
	SB_CBL_INBOX_WRL,
	SB_CBL_D2D_WRL,
	SB_CBL_INCOMP_WRL,
	SB_CBL_ERR_WRL,
	SB_CBL_FM_WRL,
	SB_CBL_AUTH_WRL_2W,
	SB_CBL_AUTH_WRL_5W,
	SB_CBL_ERR_AUTH_WRL,

	SB_CBL_MAX
};

enum sec_battery_voltage_mode {
	/* average voltage */
	SEC_BATTERY_VOLTAGE_AVERAGE = 0,
	/* open circuit voltage */
	SEC_BATTERY_VOLTAGE_OCV,
};

enum sec_battery_current_mode {
	/* uA */
	SEC_BATTERY_CURRENT_UA = 0,
	/* mA */
	SEC_BATTERY_CURRENT_MA,
};

enum sec_battery_capacity_mode {
	/* designed capacity */
	SEC_BATTERY_CAPACITY_DESIGNED = 0,
	/* absolute capacity by fuel gauge */
	SEC_BATTERY_CAPACITY_ABSOLUTE,
	/* temperary capacity in the time */
	SEC_BATTERY_CAPACITY_TEMPERARY,
	/* current capacity now */
	SEC_BATTERY_CAPACITY_CURRENT,
	/* cell aging information */
	SEC_BATTERY_CAPACITY_AGEDCELL,
	/* charge count */
	SEC_BATTERY_CAPACITY_CYCLE,
	/* full capacity rep */
	SEC_BATTERY_CAPACITY_FULL,
	/* QH capacity */
	SEC_BATTERY_CAPACITY_QH,
	/* vfsoc */
	SEC_BATTERY_CAPACITY_VFSOC,
};

enum sec_siop_event_mode {
	SIOP_EVENT_IDLE = 0,
	SIOP_EVENT_WPC_CALL_START,		/* 5V wireless charging + Call */
	SIOP_EVENT_WPC_CALL_END,		/* 5V wireless charging + Call */
	SIOP_EVENT_MAX,					/* end */
};

enum sec_battery_temp_control_source {
	TEMP_CONTROL_SOURCE_NONE = 0,
	TEMP_CONTROL_SOURCE_BAT_THM,
	TEMP_CONTROL_SOURCE_CHG_THM,
	TEMP_CONTROL_SOURCE_WPC_THM,
	TEMP_CONTROL_SOURCE_USB_THM,
};

/* ADC type */
enum sec_battery_adc_type {
	/* NOT using this ADC channel */
	SEC_BATTERY_ADC_TYPE_NONE = 0,
	/* ADC in AP */
	SEC_BATTERY_ADC_TYPE_AP,
	/* ADC by additional IC */
	SEC_BATTERY_ADC_TYPE_IC,
	SEC_BATTERY_ADC_TYPE_NUM
};

enum sec_battery_adc_channel {
	SEC_BAT_ADC_CHANNEL_CABLE_CHECK = 0,
	SEC_BAT_ADC_CHANNEL_BAT_CHECK,
	SEC_BAT_ADC_CHANNEL_TEMP,
	SEC_BAT_ADC_CHANNEL_TEMP_AMBIENT,
	SEC_BAT_ADC_CHANNEL_FULL_CHECK,
	SEC_BAT_ADC_CHANNEL_VOLTAGE_NOW,
	SEC_BAT_ADC_CHANNEL_CHG_TEMP,
	SEC_BAT_ADC_CHANNEL_INBAT_VOLTAGE,
	SEC_BAT_ADC_CHANNEL_DISCHARGING_CHECK,
	SEC_BAT_ADC_CHANNEL_DISCHARGING_NTC,
	SEC_BAT_ADC_CHANNEL_WPC_TEMP,
	SEC_BAT_ADC_CHANNEL_SLAVE_CHG_TEMP,
	SEC_BAT_ADC_CHANNEL_USB_TEMP,
	SEC_BAT_ADC_CHANNEL_NUM,
};

/* charging mode */
enum sec_battery_charging_mode {
	/* no charging */
	SEC_BATTERY_CHARGING_NONE = 0,
	/* 1st charging */
	SEC_BATTERY_CHARGING_1ST,
	/* 2nd charging */
	SEC_BATTERY_CHARGING_2ND,
	/* recharging */
	SEC_BATTERY_CHARGING_RECHARGING,
};

/* POWER_SUPPLY_EXT_PROP_MEASURE_SYS */
enum sec_battery_measure_sys {
	SEC_BATTERY_ISYS_MA = 0,
	SEC_BATTERY_ISYS_UA,
	SEC_BATTERY_ISYS_AVG_MA,
	SEC_BATTERY_ISYS_AVG_UA,
	SEC_BATTERY_VSYS,
};

/* POWER_SUPPLY_EXT_PROP_MEASURE_INPUT */
enum sec_battery_measure_input {
	SEC_BATTERY_IIN_MA = 0,
	SEC_BATTERY_IIN_UA,
	SEC_BATTERY_VBYP,
};

enum {
	SEC_VOUT_OFF_ENTER = 1,
	SEC_VOUT_OFF_EXIT,
};

/* monitor activation */
enum sec_battery_polling_time_type {
	/* same order with power supply status */
	SEC_BATTERY_POLLING_TIME_BASIC = 0,
	SEC_BATTERY_POLLING_TIME_CHARGING,
	SEC_BATTERY_POLLING_TIME_DISCHARGING,
	SEC_BATTERY_POLLING_TIME_NOT_CHARGING,
	SEC_BATTERY_POLLING_TIME_SLEEP,
};

enum sec_battery_monitor_polling {
	/* polling work queue */
	SEC_BATTERY_MONITOR_WORKQUEUE,
	/* alarm polling */
	SEC_BATTERY_MONITOR_ALARM,
	/* timer polling (NOT USE) */
	SEC_BATTERY_MONITOR_TIMER,
};
#define sec_battery_monitor_polling_t \
	enum sec_battery_monitor_polling

/* full charged check : POWER_SUPPLY_PROP_STATUS */
enum sec_battery_full_charged {
	SEC_BATTERY_FULLCHARGED_NONE = 0,
	/* current check by ADC */
	SEC_BATTERY_FULLCHARGED_ADC,
	/* fuel gauge current check */
	SEC_BATTERY_FULLCHARGED_FG_CURRENT,
	/* time check */
	SEC_BATTERY_FULLCHARGED_TIME,
	/* SOC check */
	SEC_BATTERY_FULLCHARGED_SOC,
	/* charger GPIO, NO additional full condition */
	SEC_BATTERY_FULLCHARGED_CHGGPIO,
	/* charger interrupt, NO additional full condition */
	SEC_BATTERY_FULLCHARGED_CHGINT,
	/* charger power supply property, NO additional full condition */
	SEC_BATTERY_FULLCHARGED_CHGPSY,
};

#define sec_battery_full_charged_t \
	enum sec_battery_full_charged

/* full check condition type (can be used overlapped) */
#define sec_battery_full_condition_t unsigned int
/* SEC_BATTERY_FULL_CONDITION_NOTIMEFULL
 * full-charged by absolute-timer only in high voltage
 */
#define SEC_BATTERY_FULL_CONDITION_NOTIMEFULL	1
/* SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL
 * do not set polling time as sleep polling time in full-charged
 */
#define SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL	2
/* SEC_BATTERY_FULL_CONDITION_SOC
 * use capacity for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_SOC		4
/* SEC_BATTERY_FULL_CONDITION_VCELL
 * use VCELL for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_VCELL	8
/* SEC_BATTERY_FULL_CONDITION_AVGVCELL
 * use average VCELL for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_AVGVCELL	16
/* SEC_BATTERY_FULL_CONDITION_OCV
 * use OCV for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_OCV		32

/* recharge check condition type (can be used overlapped) */
#define sec_battery_recharge_condition_t unsigned int
/* SEC_BATTERY_RECHARGE_CONDITION_SOC
 * use capacity for recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_SOC		1
/* SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL
 * use average VCELL for recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL		2
/* SEC_BATTERY_RECHARGE_CONDITION_VCELL
 * use VCELL for recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_VCELL		4

/* battery check : POWER_SUPPLY_PROP_PRESENT */
enum sec_battery_check {
	/* No Check for internal battery */
	SEC_BATTERY_CHECK_NONE,
	/* by ADC */
	SEC_BATTERY_CHECK_ADC,
	/* by callback function (battery certification by 1 wired)*/
	SEC_BATTERY_CHECK_CALLBACK,
	/* by PMIC */
	SEC_BATTERY_CHECK_PMIC,
	/* by fuel gauge */
	SEC_BATTERY_CHECK_FUELGAUGE,
	/* by charger */
	SEC_BATTERY_CHECK_CHARGER,
	/* by interrupt (use check_battery_callback() to check battery) */
	SEC_BATTERY_CHECK_INT,
};
#define sec_battery_check_t \
	enum sec_battery_check

/* OVP, UVLO check : POWER_SUPPLY_PROP_HEALTH */
enum sec_battery_ovp_uvlo {
	/* by callback function */
	SEC_BATTERY_OVP_UVLO_CALLBACK,
	/* by PMIC polling */
	SEC_BATTERY_OVP_UVLO_PMICPOLLING,
	/* by PMIC interrupt */
	SEC_BATTERY_OVP_UVLO_PMICINT,
	/* by charger polling */
	SEC_BATTERY_OVP_UVLO_CHGPOLLING,
	/* by charger interrupt */
	SEC_BATTERY_OVP_UVLO_CHGINT,
};
#define sec_battery_ovp_uvlo_t \
	enum sec_battery_ovp_uvlo

/* temperature check type */
enum sec_battery_temp_check {
	SEC_BATTERY_TEMP_CHECK_NONE = 0,	/* no temperature check */
	SEC_BATTERY_TEMP_CHECK_ADC,	/* by ADC value */
	SEC_BATTERY_TEMP_CHECK_TEMP,	/* by temperature */
};
#define sec_battery_temp_check_t \
	enum sec_battery_temp_check

/* cable check (can be used overlapped) */
#define sec_battery_cable_check_t unsigned int
/* SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE
 * for USB cable in tablet model,
 * status is stuck into discharging,
 * but internal charging logic is working
 */
#define SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE		1
/* SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE
 * for incompatible charger
 * (Not compliant to USB specification,
 * cable type is SEC_BATTERY_CABLE_UNKNOWN),
 * do NOT charge and show message to user
 * (only for VZW)
 */
#define SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE	2
/* SEC_BATTERY_CABLE_CHECK_PSY
 * check cable by power supply set_property
 */
#define SEC_BATTERY_CABLE_CHECK_PSY			4
/* SEC_BATTERY_CABLE_CHECK_INT
 * check cable by interrupt
 */
#define SEC_BATTERY_CABLE_CHECK_INT			8
/* SEC_BATTERY_CABLE_CHECK_CHGINT
 * check cable by charger interrupt
 */
#define SEC_BATTERY_CABLE_CHECK_CHGINT			16
/* SEC_BATTERY_CABLE_CHECK_POLLING
 * check cable by GPIO polling
 */
#define SEC_BATTERY_CABLE_CHECK_POLLING			32

/* check cable source (can be used overlapped) */
#define sec_battery_cable_source_t unsigned int
/* SEC_BATTERY_CABLE_SOURCE_EXTERNAL
 * already given by external argument
 */
#define	SEC_BATTERY_CABLE_SOURCE_EXTERNAL	1
/* SEC_BATTERY_CABLE_SOURCE_CALLBACK
 * by callback (MUIC, USB switch)
 */
#define	SEC_BATTERY_CABLE_SOURCE_CALLBACK	2
/* SEC_BATTERY_CABLE_SOURCE_ADC
 * by ADC
 */
#define	SEC_BATTERY_CABLE_SOURCE_ADC		4

/* capacity calculation type (can be used overlapped) */
#define sec_fuelgauge_capacity_type_t int
/* SEC_FUELGAUGE_CAPACITY_TYPE_RESET
 * use capacity information to reset fuel gauge
 * (only for driver algorithm, can NOT be set by user)
 */
#define SEC_FUELGAUGE_CAPACITY_TYPE_RESET	(-1)
/* SEC_FUELGAUGE_CAPACITY_TYPE_RAW
 * use capacity information from fuel gauge directly
 */
#define SEC_FUELGAUGE_CAPACITY_TYPE_RAW		1
/* SEC_FUELGAUGE_CAPACITY_TYPE_SCALE
 * rescale capacity by scaling, need min and max value for scaling
 */
#define SEC_FUELGAUGE_CAPACITY_TYPE_SCALE	2
/* SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE
 * change only maximum capacity dynamically
 * to keep time for every SOC unit
 */
#define SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE	4
/* SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC
 * change capacity value by only -1 or +1
 * no sudden change of capacity
 */
#define SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC	8
/* SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL
 * skip current capacity value
 * if it is abnormal value
 */
#define SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL	16

/* charger function settings (can be used overlapped) */
#define sec_charger_functions_t unsigned int
/* SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT
 * disable gradual charging current setting
 * SUMMIT:AICL, MAXIM:regulation loop
 */
#define SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT		1

/* SEC_CHARGER_MINIMUM_SIOP_CHARGING_CURRENT
 * charging current should be over than USB charging current
 */
#define SEC_CHARGER_MINIMUM_SIOP_CHARGING_CURRENT	2

enum {
	WPC_MODE_NORMAL = 0,
	WPC_MODE_BOOT,
	WPC_MODE_ATTACH,
	WPC_MODE_IDT,
	WPC_MODE_VOUT_OFF,
};

#define ACOK_NO_INPUT			0
#define ACOK_INPUT				1

struct sb_step;
struct bat_temp_limit;
struct sec_charging_current {
	unsigned int input_current_limit;
	unsigned int fast_charging_current;

	/* time to full fast charging current */
	unsigned int ttf_fcc;

	/* flags */
	union {
		unsigned int flag;

		struct {
			/* step work */
			unsigned icl_step_work : 1,
				fcc_step_work : 1;

			/* set target device flag */
			unsigned icl_tdev_wpc : 1,
				fcc_tdev_wpc : 1;

			unsigned revs : 28;
		} flags;
	};

	/* ext current */
	struct bat_temp_limit *btl;
	struct sb_step *st;
};

#define sec_charging_current_t \
	struct sec_charging_current

#if defined(CONFIG_BATTERY_AGE_FORECAST)
struct sec_age_data {
	unsigned int cycle;
	unsigned int float_voltage;
	unsigned int recharge_condition_vcell;
	unsigned int full_condition_vcell;
	unsigned int full_condition_soc;
};

#define sec_age_data_t \
	struct sec_age_data
#endif

struct sec_battery_platform_data {
	/* NO NEED TO BE CHANGED */
	/* callback functions */
	void (*initial_check)(void);
	void (*monitor_additional_check)(void);
	bool (*bat_gpio_init)(void);
	bool (*fg_gpio_init)(void);
	bool (*is_lpm)(void);
	bool (*check_jig_status)(void);
	bool (*is_interrupt_cable_check_possible)(int);
	int (*check_cable_callback)(void);
	int (*get_cable_from_extended_cable_type)(int);
	bool (*cable_switch_check)(void);
	bool (*cable_switch_normal)(void);
	bool (*check_cable_result_callback)(int);
	bool (*check_battery_callback)(void);
	bool (*check_battery_result_callback)(void);
	int (*ovp_uvlo_callback)(void);
	bool (*ovp_uvlo_result_callback)(int);
	bool (*fuelalert_process)(bool);
	bool (*get_temperature_callback)(
			enum power_supply_property,
			union power_supply_propval*);

	/* charging current for type (0: not use, default) */
	sec_charging_current_t *charging_current;
	unsigned int *polling_time;
	char *chip_vendor;
	char *pmic_name;

	/* battery */
	char *vendor;
	int technology;
	int battery_type;
	void *battery_data;

	int bat_gpio_ta_nconnected;
	/* 1 : active high, 0 : active low */
	int bat_polarity_ta_nconnected;
	int bat_irq;
	int bat_irq_gpio; /* BATT_INT(BAT_ID detecting) */
	unsigned long bat_irq_attr;
	int jig_irq;
	unsigned long jig_irq_attr;
	sec_battery_cable_check_t cable_check_type;
	sec_battery_cable_source_t cable_source_type;

	unsigned int swelling_high_rechg_voltage;
	unsigned int swelling_low_rechg_voltage;

	bool use_LED;				/* use charging LED */

	/* sustaining event after deactivated (second) */
	unsigned int event_waiting_time;

	/* Monitor setting */
	sec_battery_monitor_polling_t polling_type;
	/* for initial check */
	unsigned int monitor_initial_count;

	/* Battery check */
	sec_battery_check_t battery_check_type;
	/* how many times do we need to check battery */
	unsigned int check_count;

	/* OVP/UVLO check */
	sec_battery_ovp_uvlo_t ovp_uvlo_check_type;
	sec_battery_temp_check_t temp_check_type;
	unsigned int temp_check_count;

	/*
	 * limit can be ADC value or Temperature
	 * depending on temp_check_type
	 * temperature should be temp x 10 (0.1 degree)
	 */
	int wireless_cold_cool3_thresh;
	int wireless_cool3_cool2_thresh;
	int wireless_cool2_cool1_thresh;
	int wireless_cool1_normal_thresh;
	int wireless_normal_warm_thresh;
	int wireless_warm_overheat_thresh;
	int warm_recov_thresh;

	int wire_cold_cool3_thresh;
	int wire_cool3_cool2_thresh;
	int wire_cool2_cool1_thresh;
	int wire_cool1_normal_thresh;
	int wire_normal_warm_thresh;
	int wire_warm_overheat_thresh;

	int wire_warm_current;
	int wire_cool1_current;
	int wire_cool2_current;
	int wire_cool3_current;
	int wireless_warm_current;
	int wireless_cool1_current;
	int wireless_cool2_current;
	int wireless_cool3_current;
	int high_temp_topoff;
	int low_temp_topoff;
	int high_temp_float;
	int low_temp_float;

	unsigned int max_charging_current;

	/* If these is NOT full check type or NONE full check type,
	 * it is skipped
	 */
	/* 1st full check */
	sec_battery_full_charged_t full_check_type;
	/* 2nd full check */
	sec_battery_full_charged_t full_check_type_2nd;
	unsigned int full_check_count;
	unsigned int swell_full_check_count;
	int chg_gpio_full_check;
	/* 1 : active high, 0 : active low */
	int chg_polarity_full_check;
	sec_battery_full_condition_t full_condition_type;
	unsigned int full_condition_soc;
	unsigned int full_condition_vcell;
	unsigned int full_condition_avgvcell;
	unsigned int full_condition_ocv;

	unsigned int recharge_check_count;
	sec_battery_recharge_condition_t recharge_condition_type;
	unsigned int recharge_condition_soc;
	unsigned int recharge_condition_avgvcell;
	unsigned int recharge_condition_vcell;

	unsigned int ux_full_inow;

	/* fuel gauge */
	char *fuelgauge_name;
	int fg_irq;
	unsigned long fg_irq_attr;
	/* fuel alert SOC (-1: not use) */
	int fuel_alert_soc;
	/* fuel alert can be repeated */
	bool repeated_fuelalert;
	sec_fuelgauge_capacity_type_t capacity_calculation_type;
	/* soc should be soc x 10 (0.1% degree)
	 * only for scaling
	 */
	int capacity_max;
	int capacity_max_hv;

	int capacity_max_margin;
	int capacity_min;

	/* charger */
	char *charger_name;
	char *fgsrc_switch_name;
	bool support_fgsrc_change;

	/* wireless charger */
	char *wireless_charger_name;
	int set_cv_vout_in_low_capacity;

	int chg_gpio_en;
	int chg_irq;
	unsigned long chg_irq_attr;
	/* float voltage (mV) */
	unsigned int chg_float_voltage;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int num_age_step;
	int age_step;
	int age_data_length;
	sec_age_data_t* age_data;

	int num_age_cc_cv_threshold;
	int* age_cc_cv_threshold;
#endif
	sec_charger_functions_t chg_functions_setting;

	bool fake_capacity;

#if defined(CONFIG_BATTERY_CISD)
	unsigned int battery_full_capacity;
	unsigned int cisd_cap_high_thr;
	unsigned int cisd_cap_low_thr;
	unsigned int cisd_cap_limit;
	unsigned int max_voltage_thr;
	unsigned int cisd_alg_index;
#endif

	/* ADC setting */
	unsigned int adc_check_count;

	unsigned int full_check_current_1st;
	unsigned int full_check_current_2nd;

	unsigned int expired_time;
	unsigned int recharging_expired_time;
	bool single_charger_path;
	int standard_curr;

	unsigned int store_wpc_current;
	unsigned int store_mode_chg_max;
	unsigned int store_mode_chg_min;

	unsigned int boot_mode_time;
	unsigned int boot_charge_current;
	unsigned int tct_low_current;

	/* ADC type for each channel */
	unsigned int adc_type[];
};

struct sec_charger_platform_data {
	bool (*chg_gpio_init)(void);

	/* charging current for type (0: not use) */
	sec_charging_current_t *charging_current;

	/* wirelss charger */
	char *wireless_charger_name;

	int vbus_ctrl_gpio;
	int chg_gpio_en;
	/* float voltage (mV) */
	int chg_float_voltage;
	int irq_gpio;
	int chg_irq;
	unsigned long chg_irq_attr;

	/* otg_en setting */
	int otg_en;

	/* OVP/UVLO check */
	sec_battery_ovp_uvlo_t ovp_uvlo_check_type;
	/* 1st full check */
	sec_battery_full_charged_t full_check_type;
	/* 2nd full check */
	sec_battery_full_charged_t full_check_type_2nd;

	sec_charger_functions_t chg_functions_setting;
};

struct sec_fuelgauge_platform_data {
	bool (*fg_gpio_init)(void);
	bool (*check_jig_status)(void);
	int (*check_cable_callback)(void);
	bool (*fuelalert_process)(bool);

	/* charging current for type (0: not use) */
	unsigned int full_check_current_1st;
	unsigned int full_check_current_2nd;

	int jig_irq;
	int jig_gpio;
	bool jig_low_active;
	unsigned long jig_irq_attr;

	int fg_irq;
	unsigned long fg_irq_attr;
	/* fuel alert SOC (-1: not use) */
	int fuel_alert_soc;
	int fuel_alert_vol;
	/* fuel alert can be repeated */
	bool repeated_fuelalert;
	sec_fuelgauge_capacity_type_t capacity_calculation_type;
	/* soc should be soc x 10 (0.1% degree)
	 * only for scaling
	 */
	int capacity_max;
	int capacity_max_hv;
	int capacity_max_margin;
	int capacity_min;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	unsigned int full_condition_soc;
#endif
};

#define sec_battery_platform_data_t \
	struct sec_battery_platform_data

#define sec_charger_platform_data_t \
	struct sec_charger_platform_data

#define sec_fuelgauge_platform_data_t \
	struct sec_fuelgauge_platform_data

#define GET_BOOL_STR(state) ((state) ? "TRUE" : "FALSE")

static inline struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

#define psy_do_property(name, function, property, value) \
({	\
	struct power_supply *psy;	\
	int ret = 0;	\
	psy = get_power_supply_by_name((name));	\
	if (!psy) {	\
		pr_err("%s: Fail to "#function" psy (%s)\n",	\
			__func__, (name));	\
		value.intval = 0;	\
		ret = -ENOENT;	\
	} else {	\
		if (psy->desc->function##_property != NULL) { \
			ret = psy->desc->function##_property(psy, \
				(enum power_supply_property) (property), &(value)); \
			if (ret < 0) {	\
				pr_err("%s: Fail to %s "#function" "#property" (%d)\n", \
						__func__, name, ret);	\
				value.intval = 0;	\
			}	\
		} else {	\
			ret = -ENOSYS;	\
		}	\
		power_supply_put(psy);		\
	}					\
	ret;	\
})

#if defined(CONFIG_OF)
#define sb_of_parse_u32(np, pdata, value, deft) \
	({ \
		int ret = 0; \
		ret = of_property_read_u32(np, #value, (unsigned int *)&pdata->value); \
		if (!ret) \
			pr_info("%s: %s - write "#value" to %d\n", __func__, np->name, pdata->value); \
		else \
			pdata->value = deft; \
		ret;\
	})

#define sb_of_parse_str(np, pdata, value) \
	({ \
		int ret = 0; \
		ret = of_property_read_string(np, #value, (const char **)&pdata->value); \
		if (!ret) \
			pr_info("%s: %s - write "#value" to %s\n", __func__, np->name, pdata->value); \
		ret;\
	})

#define sb_of_parse_bool(np, pdata, value) \
	({ \
		pdata->value = of_property_read_bool(np, #value); \
		pr_info("%s: %s - write "#value" to %d\n", __func__, np->name, pdata->value); \
	})
#endif

#define get_battery_data(driver)	\
	(((struct battery_data_t *)(driver)->pdata->battery_data)	\
	[(driver)->pdata->battery_type])

#define is_auth_wrl_type(cable_type) ( \
	cable_type == SB_CBL_AUTH_WRL_2W || \
	cable_type == SB_CBL_AUTH_WRL_5W)

#define is_inbox_wrl_type(cable_type) ( \
	cable_type == SB_CBL_INBOX_WRL)

#define is_d2d_wrl_type(cable_type) ( \
	cable_type == SB_CBL_D2D_WRL)

#define is_incomp_wrl_type(cable_type) ( \
	cable_type == SB_CBL_INCOMP_WRL || \
	cable_type == SB_CBL_ERR_WRL)

#define is_wireless_type(cable_type) ( \
	cable_type == SB_CBL_WRL || \
	cable_type == SB_CBL_FM_WRL || \
	cable_type == SB_CBL_ERR_AUTH_WRL || \
	is_inbox_wrl_type(cable_type) || \
	is_d2d_wrl_type(cable_type) || \
	is_incomp_wrl_type(cable_type) || \
	is_auth_wrl_type(cable_type))

#define is_nocharge_type(cable_type) ( \
	cable_type == SB_CBL_NONE)

#define is_slate_mode(battery) ((battery->current_event & SEC_BAT_CURRENT_EVENT_SLATE) \
		== SEC_BAT_CURRENT_EVENT_SLATE)

#define is_ta_type(cable_type) ( \
	cable_type == SB_CBL_TA || \
	cable_type == SB_CBL_UARTOFF)

#define is_usb_type(cable_type) ( \
	cable_type == SB_CBL_USB)

#define is_wired_type(cable_type) ( \
	is_ta_type(cable_type) || is_usb_type(cable_type))

#endif /* __SEC_CHARGING_COMMON_H */
