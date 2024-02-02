/*
 * sec_battery.h
 * Samsung Mobile Battery Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 *
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

#ifndef __SEC_BATTERY_H
#define __SEC_BATTERY_H __FILE__

#include "sec_charging_common.h"
#include <linux/of_gpio.h>
#include <linux/alarmtimer.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/sec_batt.h>
#include "sec_battery_vote.h"
#include "../core/sb_notify.h"

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif
#if defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif
#if defined(CONFIG_BATTERY_CISD)
#include "sec_cisd.h"
#endif

#define SEC_BAT_CURRENT_EVENT_NONE					0x000000
#define SEC_BAT_CURRENT_EVENT_AFC					0x000001
#define SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE		0x000002
#define SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL	0x000004
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1	0x000008
#define SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING		0x000020
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2	0x000080
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3	0x000010
#define SEC_BAT_CURRENT_EVENT_SWELLING_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1 | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2 | SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3)
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1 | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2 | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3)
#define SEC_BAT_CURRENT_EVENT_CHG_LIMIT			0x000200
#define SEC_BAT_CURRENT_EVENT_CALL				0x000400
#define SEC_BAT_CURRENT_EVENT_SLATE				0x000800
#define SEC_BAT_CURRENT_EVENT_VBAT_OVP			0x001000
#define SEC_BAT_CURRENT_EVENT_VSYS_OVP			0x002000
#define SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK		0x004000
#define SEC_BAT_CURRENT_EVENT_AICL				0x008000
#define SEC_BAT_CURRENT_EVENT_HV_DISABLE		0x010000
#define SEC_BAT_CURRENT_EVENT_SELECT_PDO		0x020000
/* #define SEC_BAT_CURRENT_EVENT_FG_RESET			0x040000 */
#define SEC_BAT_CURRENT_EVENT_LOW_CURRENT_TCT	0x040000

#define SEC_BAT_CURRENT_EVENT_DC_ERR			0x400000
#define SEC_BAT_CURRENT_EVENT_SIOP_LIMIT		0x800000
#define SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST	0x1000000
#define SEC_BAT_CURRENT_EVENT_25W_OCP			0x2000000
#define SEC_BAT_CURRENT_EVENT_AFC_DISABLE		0x4000000
#define SEC_BAT_CURRENT_EVENT_SEND_UVDM			0x8000000

#define SEC_BAT_CURRENT_EVENT_USB_SUSPENDED		0x10000000
#define SEC_BAT_CURRENT_EVENT_USB_SUPER		0x20000000
#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
#define SEC_BAT_CURRENT_EVENT_USB_100MA		0x40000000
#else
#define SEC_BAT_CURRENT_EVENT_USB_100MA		0x00000000
#endif
#if defined(CONFIG_DISABLE_MFC_IC)
#define SEC_BAT_CURRENT_EVENT_WPC_EN		0x80000000
#else
#define SEC_BAT_CURRENT_EVENT_WPC_EN		0x00000000
#endif

#define SIOP_EVENT_NONE		0x0000
#define SIOP_EVENT_WPC_CALL	0x0001

#if defined(CONFIG_SEC_FACTORY)	// SEC_FACTORY
#define STORE_MODE_CHARGING_MAX 80
#define STORE_MODE_CHARGING_MIN 70
#else	// !SEC_FACTORY, STORE MODE
#define STORE_MODE_CHARGING_MAX 50
#define STORE_MODE_CHARGING_MIN 40
#define STORE_MODE_CHARGING_MAX_VZW 35
#define STORE_MODE_CHARGING_MIN_VZW 30
#endif //(CONFIG_SEC_FACTORY)

#define DEFAULT_HEALTH_CHECK_COUNT	5
#define TEMP_HIGHLIMIT_DEFAULT	2000

#define BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE	0x00000001
#define BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE	0x00000002
#define BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE		0x00000004
#define BATT_MISC_EVENT_BATT_RESET_SOC			0x00000008
#define BATT_MISC_EVENT_WATER_HICCUP_TYPE		0x00000020
#define BATT_MISC_EVENT_WIRELESS_DET_LEVEL		0x00000040
#define BATT_MISC_EVENT_WIRELESS_FW_UPDATE		0x00000080
#define BATT_MISC_EVENT_WIRELESS_FOD			0x00000100
/*
#define BATT_MISC_EVENT_WIRELESS_AUTH_START		0x00000200
#define BATT_MISC_EVENT_WIRELESS_AUTH_RECVED	0x00000400
#define BATT_MISC_EVENT_WIRELESS_AUTH_FAIL		0x00000800
#define BATT_MISC_EVENT_WIRELESS_AUTH_PASS		0x00001000
*/
/* Move auth event to sb_wrl_data.h */
#define BATT_MISC_EVENT_WIRELESS_AUTH_SHIFT		9
#define BATT_MISC_EVENT_WIRELESS_AUTH_MASK		(0xF << BATT_MISC_EVENT_WIRELESS_AUTH_SHIFT)

#define BATT_MISC_EVENT_TEMP_HICCUP_TYPE		0x00002000
#define BATT_MISC_EVENT_BATTERY_HEALTH			0x000F0000
#define BATT_MISC_EVENT_HEALTH_OVERHEATLIMIT	0x00100000
#define BATT_MISC_EVENT_ABNORMAL_PAD			0x00200000
#define BATT_MISC_EVENT_WIRELESS_MISALIGN		0x00400000
#define BATT_MISC_EVENT_WIRELESS_INCOMPATIBLE	0x00800000

#define BATTERY_HEALTH_SHIFT	16
enum misc_battery_health {
	BATTERY_HEALTH_UNKNOWN = 0,
	BATTERY_HEALTH_GOOD,
	BATTERY_HEALTH_NORMAL,
	BATTERY_HEALTH_AGED,
	BATTERY_HEALTH_MAX = BATTERY_HEALTH_AGED,

	/* For event */
	BATTERY_HEALTH_BAD = 0xF,
};
#define BATT_MISC_EVENT_MUIC_ABNORMAL	(BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |\
					BATT_MISC_EVENT_WATER_HICCUP_TYPE |\
					BATT_MISC_EVENT_TEMP_HICCUP_TYPE)

#define BATT_MISC_EVENT_RESET_AFTER_READ_MASK	\
	(BATT_MISC_EVENT_WIRELESS_INCOMPATIBLE)

#define SEC_INPUT_VOLTAGE_0V	0
#define SEC_INPUT_VOLTAGE_5V	5
#define SEC_INPUT_VOLTAGE_9V	9
#define SEC_INPUT_VOLTAGE_10V	10
#define SEC_INPUT_VOLTAGE_12V	12

enum {
	BAT_THERMAL_COLD = 0,
	BAT_THERMAL_COOL3,
	BAT_THERMAL_COOL2,
	BAT_THERMAL_COOL1,
	BAT_THERMAL_NORMAL,
	BAT_THERMAL_WARM,
	BAT_THERMAL_OVERHEAT,
	BAT_THERMAL_OVERHEATLIMIT,
};

struct sec_ttf_data;

struct sec_battery_info {
	struct device *dev;
	sec_battery_platform_data_t *pdata;
	struct sec_ttf_data *ttf_d;

	/* power supply used in Android */
	struct power_supply *psy_bat;
	struct power_supply *psy_usb;
	struct power_supply *psy_ac;
	struct power_supply *psy_wireless;
	unsigned int irq;

	struct notifier_block sb_nb;

	struct sec_bat_eng_data *eng_data;

	int pd_usb_attached;
#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block batt_nb;
#endif
	bool safety_timer_set;
	bool lcd_status;
	bool skip_swelling;

	int status;
	int health;
	bool present;
	unsigned int charger_mode;

	int voltage_now;		/* cell voltage (mV) */
	int voltage_avg;		/* average voltage (mV) */
	int voltage_ocv;		/* open circuit voltage (mV) */
	int current_now;		/* current (mA) */
	int inbat_adc;			/* inbat adc */
	int current_avg;		/* average current (mA) */
	int current_max;		/* input current limit (mA) */
	int charge_counter;		/* remaining capacity (uAh) */
	int current_adc;

	unsigned int capacity;			/* SOC (%) */
	unsigned int input_voltage;		/* CHGIN/WCIN input voltage (V) */
	unsigned int charge_power;		/* charge power (mW) */
	unsigned int max_charge_power;		/* max charge power (mW) */
	unsigned int aicl_current;

	/* keep awake until monitor is done */
	struct wakeup_source* monitor_ws;
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;

	unsigned int polling_count;
	unsigned int polling_time;
	bool polling_in_sleep;
	bool polling_short;

	struct delayed_work polling_work;
	struct alarm polling_alarm;
	ktime_t last_poll_time;

#if defined(CONFIG_BATTERY_CISD)
	struct cisd cisd;
	bool skip_cisd;
	bool usb_overheat_check;
	int prev_volt;
	int prev_temp;
	int prev_jig_on;
	int enable_update_data;
	int prev_chg_on;
#endif
	/* battery check */
	unsigned int check_count;

	/* health change check*/
	bool health_change;
	/* ovp-uvlo health check */
	int health_check_count;

	/* time check */
	unsigned long charging_start_time;
	unsigned long charging_passed_time;
	unsigned long charging_fullcharged_time;

	/* chg temperature check */
	unsigned int chg_limit;

	int cold_cool3_thresh;
	int cool3_cool2_thresh;
	int cool2_cool1_thresh;
	int cool1_normal_thresh;
	int normal_warm_thresh;
	int warm_overheat_thresh;
	int thermal_zone;
	int bat_thm_count;

	/* charging */
	unsigned int charging_mode;
	bool is_recharging;
	int wdt_kick_disable;
	int topoff_condition;

	bool is_jig_on;
	int cable_type;
	int muic_cable_type;

	struct wakeup_source* cable_ws;
	struct delayed_work cable_work;
	struct wakeup_source* vbus_ws;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	struct delayed_work batt_data_work;
	struct wakeup_source* batt_data_ws;
	char *data_path;
#endif
#ifdef CONFIG_OF
	struct delayed_work parse_mode_dt_work;
	struct wakeup_source* parse_mode_dt_ws;
#endif
	struct delayed_work lpcharge_work;
	struct wakeup_source* lpcharge_ws;
	int lpcharge_boost;

	struct delayed_work prevent_overheat_work;
	struct wakeup_source *prevent_overheat_ws;

	struct delayed_work fcc_work;
	struct wakeup_source *fcc_ws;

	char batt_type[48];
	unsigned int full_check_cnt;
	unsigned int recharge_check_cnt;

	struct mutex iolock;
	unsigned int input_current;
	unsigned int charging_current;
	unsigned int topoff_current;
	unsigned int current_event;

	unsigned int cstep;
	unsigned int cstep_applied;
	/* wireless charging enable */
	int wc_enable;
	int wc_enable_cnt;
	int wc_enable_cnt_value;
	int wire_status;

	/* test mode */
	int test_mode;
	bool factory_mode;
	bool store_mode;
	bool low_current_tct;

	int stability_test;
	int eng_not_full_status;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int batt_cycle;
#endif
	struct mutex misclock;
	unsigned int misc_event;
	unsigned int prev_misc_event;
	struct delayed_work misc_event_work;
	struct wakeup_source* misc_event_ws;
	struct mutex batt_handlelock;
	struct mutex current_eventlock;

	bool stop_timer;
	unsigned long prev_safety_time;
	unsigned long expired_time;
	unsigned long cal_safety_time;
	int fg_reset;

	struct sec_vote *fcc_vote;
	struct sec_vote *input_vote;
	struct sec_vote *fv_vote;
	struct sec_vote *chgen_vote;
	struct sec_vote *topoff_vote;

	bool disable_temp_check;
	int backup_wire_thr[2];
};

const char* sb_get_ct_str(int cable_type);
const char* sb_get_cm_str(unsigned int charging_mode);
const char* sb_get_bst_str(int status);
const char* sb_get_hl_str(int health);
const char* sb_get_tz_str(int thrmal_zone);

void sec_bat_temp_misc_test(struct sec_battery_info *battery, bool enable);

extern unsigned int lpcharge;
extern bool sleep_mode;
extern int fg_reset;
/* thermal */
extern void sec_bat_check_bat_temp_limit(struct sec_battery_info *);
extern struct bat_temp_limit *sec_bat_parse_bat_temp_limit(struct device_node *);
extern void sec_bat_thermal_check(struct sec_battery_info *battery);

extern void sec_bat_get_battery_info(struct sec_battery_info *battery);
extern void sec_bat_set_misc_event(struct sec_battery_info *battery,
	unsigned int misc_event_val, unsigned int misc_event_mask);
extern void sec_bat_set_current_event(struct sec_battery_info *battery,
	unsigned int current_event_val, unsigned int current_event_mask);
extern int sec_bat_set_charging_current(struct sec_battery_info *battery);
extern void sec_bat_aging_check(struct sec_battery_info *battery);
extern void sec_bat_fw_update_work(struct sec_battery_info *battery, int mode);
extern void sec_bat_set_threshold(struct sec_battery_info *battery);
extern void sec_bat_set_charging_status(struct sec_battery_info *battery, int status);
extern bool sec_bat_check_full(struct sec_battery_info *battery, int full_check_type);

#if defined(CONFIG_UPDATE_BATTERY_DATA)
extern int sec_battery_update_data(const char* file_path);
#endif
#if defined(CONFIG_BATTERY_CISD)
extern bool sec_bat_cisd_check(struct sec_battery_info *battery);
extern void sec_battery_cisd_init(struct sec_battery_info *battery);
extern void set_cisd_pad_data(struct sec_battery_info *battery, const char* buf);
#endif
#endif /* __SEC_BATTERY_H */
