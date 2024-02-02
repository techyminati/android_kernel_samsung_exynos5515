/*
 * sec_battery.c
 * Samsung Mobile Battery Driver
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "include/sec_battery.h"
#include "include/sec_battery_ttf.h"
#include "include/sec_sysfs.h"
#include <linux/list.h>
#include <linux/sec_ext.h>
#include <linux/sec_debug.h>

#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

#include "include/sec_battery_eng.h"
#include "include/sec_vote_event.h"
#include "include/sb_wrl_def.h"
#include "include/sb_adc.h"
#include "step/sb_step.h"

const char *sec_voter_name[] = FOREACH_VOTER(GENERATE_STRING);
bool sleep_mode = false;

static enum power_supply_property sec_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_POWER_DESIGN,
	POWER_SUPPLY_PROP_POWER_NOW,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
};

static enum power_supply_property sec_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property sec_wireless_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
};

static enum power_supply_property sec_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

char *sec_bat_charge_mode_str[] = {
	"Buck-Off",
	"Charging-Off",
	"Charging-On",
};

static int sec_bat_get_boot_mode(void)
{
	static int bootmode = -1;
	char *bootmode_str = NULL;

	if (bootmode >= 0)
		return bootmode;

	/* init default value */
	bootmode = 0;

	bootmode_str = strstr(saved_command_line, "bootmode=");
	if (bootmode_str != NULL) {
		sscanf(bootmode_str, "bootmode=%d", &bootmode);
		pr_info("%s: bootmode = %d\n", __func__, bootmode);
	}

	return bootmode;
}

const char* sb_get_ct_str(int cable_type)
{
	switch (cable_type) {
	case SB_CBL_NONE:
		return "NONE";
	case SB_CBL_TA:
		return "TA";
	case SB_CBL_USB:
		return "USB";
	case SB_CBL_UARTOFF:
		return "UARTOFF";
	case SB_CBL_WRL:
		return "WRL";
	case SB_CBL_INBOX_WRL:
		return "INBOX_WRL";
	case SB_CBL_D2D_WRL:
		return "D2D_WRL";
	case SB_CBL_INCOMP_WRL:
		return "INCOMP_WRL";
	case SB_CBL_ERR_WRL:
		return "ERR_WRL";
	case SB_CBL_FM_WRL:
		return "FM_WRL";
	case SB_CBL_AUTH_WRL_2W:
		return "AUTH_WRL_2W";
	case SB_CBL_AUTH_WRL_5W:
		return "AUTH_WRL_5W";
	case SB_CBL_ERR_AUTH_WRL:
		return "ERR_AUTH_WRL";
	default:
		return "UNKNOWN";
	}
}

const char* sb_get_cm_str(unsigned int charging_mode)
{
	switch (charging_mode) {
	case SEC_BATTERY_CHARGING_NONE:
		return "None";
	case SEC_BATTERY_CHARGING_1ST:
		return "Normal";
	case SEC_BATTERY_CHARGING_2ND:
		return "Additional";
	case SEC_BATTERY_CHARGING_RECHARGING:
		return "Re-Charging";
	default:
		return "UNKNOWN";
	}
}

const char* sb_get_bst_str(int status)
{
	switch (status) {
	case POWER_SUPPLY_STATUS_UNKNOWN:
		return "Unknown";
	case POWER_SUPPLY_STATUS_CHARGING:
		return "Chg";
	case POWER_SUPPLY_STATUS_DISCHARGING:
		return "DisChg";
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		return "NotChg";
	case POWER_SUPPLY_STATUS_FULL:
		return "Full";
	default:
		return "UNKNOWN";
	}
}

const char* sb_get_hl_str(int health)
{
	switch (health) {
	case POWER_SUPPLY_HEALTH_GOOD:
		return "Good";
	case POWER_SUPPLY_HEALTH_OVERHEAT:
		return "Overheat";
	case POWER_SUPPLY_HEALTH_WARM:
		return "Warm";
	case POWER_SUPPLY_HEALTH_DEAD:
		return "Dead";
	case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		return "OverVoltage";
	case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
		return "UnspecFailure";
	case POWER_SUPPLY_HEALTH_COLD:
		return "Cold";
	case POWER_SUPPLY_HEALTH_COOL:
		return "Cool";
	case POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE:
		return "WatchdogTimerExpire";
	case POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE:
		return "SafetyTimerExpire";
	case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
		return "UnderVoltage";
	case POWER_SUPPLY_HEALTH_OVERHEATLIMIT:
		return "OverheatLimit";
	default:
		return "UNKNOWN";
	}
}

const char* sb_get_tz_str(int thermal_zone)
{
	switch (thermal_zone) {
	case BAT_THERMAL_NORMAL:
		return "Normal";
	case BAT_THERMAL_COLD:
		return "COLD";
	case BAT_THERMAL_COOL3:
		return "COOL3";
	case BAT_THERMAL_COOL2:
		return "COOL2";
	case BAT_THERMAL_COOL1:
		return "COOL1";
	case BAT_THERMAL_WARM:
		return "WARM";
	case BAT_THERMAL_OVERHEAT:
		return "OVERHEAT";
	case BAT_THERMAL_OVERHEATLIMIT:
		return "OVERHEATLIM";
	default:
		return "UNKNOWN";
	}
}

static struct sec_charging_current *get_def_chg_cur_ptr(struct sec_battery_info *battery)
{
	if (!battery->pdata->charging_current)
		return NULL;

	return &battery->pdata->charging_current[SB_CBL_UNKNOWN];
}

static struct sec_charging_current *get_chg_cur_ptr(struct sec_battery_info *battery, int cable_type)
{
	if ((cable_type < SB_CBL_UNKNOWN) || (cable_type >= SB_CBL_MAX))
		return ERR_PTR(-EINVAL);

	return &battery->pdata->charging_current[cable_type];
}

static int get_next_value(int target, int value, int step)
{
	if (target == value)
		return target;

	if (target > value) {
		if (target - value < step)
			return target;

		return value + step;
	}

	/* if target < value */
	if (value - target < step)
		return target;

	return value - step;
}

static void fcc_step_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, fcc_work.work);
	union power_supply_propval value = {0, };
	struct sec_charging_current *chg_cur;
	char *chg_name = battery->pdata->charger_name;
	int target_fcc;

	pr_info("%s: start\n", __func__);

	chg_cur = get_chg_cur_ptr(battery, battery->cable_type);
	if (IS_ERR_OR_NULL(chg_cur)) {
		sec_vote_refresh(battery->fcc_vote);
		goto end_work;
	}

	chg_name = (chg_cur->flags.fcc_tdev_wpc) ?
		battery->pdata->wireless_charger_name :
		battery->pdata->charger_name;

	target_fcc = get_sec_vote_result(battery->fcc_vote);
	psy_do_property(chg_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);

	value.intval = get_next_value(target_fcc, value.intval, 50);
	psy_do_property(chg_name, set,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);

	if (target_fcc == value.intval)
		goto end_work;

	queue_delayed_work(battery->monitor_wqueue,
		&battery->fcc_work, msecs_to_jiffies(1000));
	return;

end_work:
	__pm_relax(battery->fcc_ws);
}

int set_charging_current(void *data, int v)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };
	struct sec_charging_current *chg_cur;
	char *chg_name = battery->pdata->charger_name;

	battery->charging_current = v;

	chg_cur = get_chg_cur_ptr(battery, battery->cable_type);
	if (IS_ERR_OR_NULL(chg_cur))
		goto set_fcc;

	if (chg_cur->flags.fcc_step_work) {
		/* start fcc work */
		__pm_stay_awake(battery->fcc_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->fcc_work, 0);

		goto end_fcc;
	}

	chg_name = (chg_cur->flags.fcc_tdev_wpc) ?
		battery->pdata->wireless_charger_name :
		battery->pdata->charger_name;

set_fcc:
	value.intval = v;
	psy_do_property(chg_name, set,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);

end_fcc:
	pr_info("%s: power(%d), input(%d), charge(%d)\n", __func__,
		battery->charge_power, battery->input_current, battery->charging_current);
	return 0;
}

int set_input_current(void *data, int v)
{
	struct sec_battery_info *battery = data;
	struct sec_charging_current *chg_cur;

	battery->input_current = v;
	battery->charge_power = battery->input_voltage * v;
	if (battery->charge_power > battery->max_charge_power)
		battery->max_charge_power = battery->charge_power;

	chg_cur = get_chg_cur_ptr(battery, battery->cable_type);
	if (!IS_ERR_OR_NULL(chg_cur) && chg_cur->flags.icl_step_work) {
		/* start icl work */
	} else {
		union power_supply_propval value = {0, };

		value.intval = v;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value);
	}

	return 0;
}

int set_float_voltage(void *data, int voltage)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };

	value.intval = voltage;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
	return 0;
}

int set_topoff_current(void *data, int v)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };
	bool do_chgen_vote = false;

	if (battery->charging_mode == SEC_BATTERY_CHARGING_2ND ||
		battery->pdata->full_check_type == SEC_BATTERY_FULLCHARGED_CHGPSY ||
		battery->pdata->full_check_type == SEC_BATTERY_FULLCHARGED_CHGINT)
		do_chgen_vote = true;

	if (do_chgen_vote)
		sec_vote(battery->chgen_vote, VOTER_TOPOFF_CHANGE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

	value.intval = v;
	psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_FULL, value);

	if (do_chgen_vote)
		sec_vote(battery->chgen_vote, VOTER_TOPOFF_CHANGE, false, 0);
	battery->topoff_condition = v;

	return 0;
}

int sec_bat_set_charge(void *data, int chg_mode)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval val = {0, };
	ktime_t current_time = {0, };
	struct timespec ts = {0, };

	battery->charger_mode = chg_mode;
	pr_info("%s set %s mode\n", __func__, sec_bat_charge_mode_str[chg_mode]);

	val.intval = battery->status;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_STATUS, val);
	current_time = ktime_get_boottime();
	ts = ktime_to_timespec(current_time);

	if (chg_mode == SEC_BAT_CHG_MODE_CHARGING) {
		/*Reset charging start time only in initial charging start */
		if (battery->charging_start_time == 0) {
			if (ts.tv_sec < 1)
				ts.tv_sec = 1;
			battery->charging_start_time = ts.tv_sec;
		}
	} else {
		battery->charging_start_time = 0;
		battery->charging_passed_time = 0;
		battery->charging_fullcharged_time = 0;
		battery->full_check_cnt = 0;
#if defined(CONFIG_BATTERY_CISD)
		battery->usb_overheat_check = false;
		battery->cisd.ab_vbat_check_count = 0;
		if (chg_mode == SEC_BAT_CHG_MODE_BUCK_OFF) {
			battery->cisd.data[CISD_DATA_BUCK_OFF]++;
			battery->cisd.data[CISD_DATA_BUCK_OFF_PER_DAY]++;
		}
#endif
	}

	val.intval = chg_mode;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, val);
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, val);
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, val);

	return 0;
}

static bool sec_bat_check_maintain_status(struct sec_battery_info *battery)
{
	switch (battery->health) {
	case POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE:
	case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
	case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
	case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
	case POWER_SUPPLY_HEALTH_VBAT_OVP:
	case POWER_SUPPLY_HEALTH_VSYS_OVP:
	case POWER_SUPPLY_HEALTH_UNKNOWN:
		return true;
	default:
		return false;
	}

}

void sec_bat_set_misc_event(struct sec_battery_info *battery,
	unsigned int misc_event_val, unsigned int misc_event_mask)
{
	unsigned int temp = battery->misc_event;

	mutex_lock(&battery->misclock);
	battery->misc_event &= ~misc_event_mask;
	battery->misc_event |= misc_event_val;
	pr_info("%s: misc event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->misc_event);
	mutex_unlock(&battery->misclock);

	if (battery->prev_misc_event != battery->misc_event) {
		cancel_delayed_work(&battery->misc_event_work);
		__pm_stay_awake(battery->misc_event_ws);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->misc_event_work, 0);
	}
}

void sec_bat_set_current_event(struct sec_battery_info *battery,
					unsigned int current_event_val, unsigned int current_event_mask)
{
	unsigned int temp = battery->current_event;

	mutex_lock(&battery->current_eventlock);

	battery->current_event &= ~current_event_mask;
	battery->current_event |= current_event_val;

	pr_info("%s: current event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->current_event);

	mutex_unlock(&battery->current_eventlock);
}

int sec_bat_set_charging_current(struct sec_battery_info *battery)
{
	mutex_lock(&battery->iolock);

	if ((battery->cable_type != SB_CBL_NONE) &&
		(battery->muic_cable_type != ATTACHED_DEV_JIG_UART_OFF_MUIC)) {
		struct sec_charging_current *chg_cur =
			get_chg_cur_ptr(battery, battery->cable_type);

		if (!IS_ERR_OR_NULL(chg_cur))
			sb_step_monitor(chg_cur->st,
				set_step_data(battery->voltage_now, battery->voltage_avg, battery->cable_type, battery->status, battery->thermal_zone));

		sec_bat_check_bat_temp_limit(battery);

		/* check topoff current */
		if ((battery->charging_mode == SEC_BATTERY_CHARGING_2ND) &&
			(battery->pdata->full_check_type_2nd == SEC_BATTERY_FULLCHARGED_CHGPSY))
			sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, true,
				battery->pdata->full_check_current_2nd);

		pr_info("%s: update cc(ic = %d , fcc = %d , eoc = %d)\n",
			__func__, battery->input_current, battery->charging_current, battery->topoff_current);
	}

	mutex_unlock(&battery->iolock);
	return 0;
}

void sec_bat_temp_misc_test(struct sec_battery_info *battery, bool enable)
{
	if (enable) {
		battery->backup_wire_thr[0] = battery->pdata->wire_warm_overheat_thresh;
		battery->backup_wire_thr[1] = battery->pdata->wire_normal_warm_thresh;
		battery->pdata->wire_warm_overheat_thresh = 500;
		battery->pdata->wire_normal_warm_thresh = 480;
		sec_bat_set_threshold(battery);
		battery->disable_temp_check = true;
	} else {
		if (battery->disable_temp_check) {
			battery->pdata->wire_warm_overheat_thresh = battery->backup_wire_thr[0];
			battery->pdata->wire_normal_warm_thresh = battery->backup_wire_thr[1];
		}
		sec_bat_set_threshold(battery);
		battery->disable_temp_check = false;
	}
	dev_info(battery->dev, "%s: set(%d)\n", __func__, enable);
}

static bool sec_bat_check_by_psy(struct sec_battery_info *battery)
{
	char *psy_name = NULL;
	union power_supply_propval value = {0, };
	bool ret = true;

	switch (battery->pdata->battery_check_type) {
	case SEC_BATTERY_CHECK_PMIC:
		psy_name = battery->pdata->pmic_name;
		break;
	case SEC_BATTERY_CHECK_FUELGAUGE:
		psy_name = battery->pdata->fuelgauge_name;
		break;
	case SEC_BATTERY_CHECK_CHARGER:
		psy_name = battery->pdata->charger_name;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid Battery Check Type\n", __func__);
		ret = false;
		goto battery_check_error;
		break;
	}

	psy_do_property(psy_name, get,
		POWER_SUPPLY_PROP_PRESENT, value);
	ret = (bool)value.intval;

battery_check_error:
	return ret;
}

static bool sec_bat_check(struct sec_battery_info *battery)
{
	bool ret = true;

	if (battery->factory_mode || battery->is_jig_on) {
		dev_dbg(battery->dev, "%s: No need to check in factory mode\n",
			__func__);
		return ret;
	}

	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return ret;
	}

	switch (battery->pdata->battery_check_type) {
	case SEC_BATTERY_CHECK_INT:
	case SEC_BATTERY_CHECK_CALLBACK:
		if (battery->cable_type == SB_CBL_NONE) {
			ret = battery->present;
		} else {
			if (battery->pdata->check_battery_callback)
				ret = battery->pdata->check_battery_callback();
		}
		break;
	case SEC_BATTERY_CHECK_PMIC:
	case SEC_BATTERY_CHECK_FUELGAUGE:
	case SEC_BATTERY_CHECK_CHARGER:
		ret = sec_bat_check_by_psy(battery);
		break;
	case SEC_BATTERY_CHECK_ADC:
	case SEC_BATTERY_CHECK_NONE:
		dev_dbg(battery->dev, "%s: No Check\n", __func__);
	default:
		break;
	}

	return ret;
}

static bool sec_bat_get_cable_type(
			struct sec_battery_info *battery,
			int cable_source_type)
{
	bool ret = false;
	int cable_type = battery->cable_type;

	if (cable_source_type & SEC_BATTERY_CABLE_SOURCE_CALLBACK) {
		if (battery->pdata->check_cable_callback)
			cable_type =
				battery->pdata->check_cable_callback();
	}

	if (battery->cable_type == cable_type) {
		dev_dbg(battery->dev,
			"%s: No need to change cable status\n", __func__);
	} else {
		if (cable_type < SB_CBL_NONE ||
			cable_type >= SB_CBL_MAX) {
			dev_err(battery->dev,
				"%s: Invalid cable type\n", __func__);
		} else {
			battery->cable_type = cable_type;
			if (battery->pdata->check_cable_result_callback)
				battery->pdata->check_cable_result_callback(
						battery->cable_type);

			ret = true;

			dev_dbg(battery->dev, "%s: Cable Changed (%d)\n",
				__func__, battery->cable_type);
		}
	}

	return ret;
}

static void sec_bat_set_cable_type(struct sec_battery_info *battery, int cable_type)
{
	if (cable_type < 0)
		dev_info(battery->dev, "%s: ignore event(%d)\n",
			__func__, cable_type);
	else
		battery->wire_status = cable_type;

	dev_info(battery->dev,
		"%s: current_cable(%d), wire_status(%d)\n",
		__func__, cable_type, battery->wire_status);

	/* cable is attached or detached
	 * if current_cable_type is minus value,
	 * check cable by sec_bat_get_cable_type()
	 * although SEC_BATTERY_CABLE_SOURCE_EXTERNAL is set
	 * (0 is SB_CBL_UNKNOWN)
	 */
	if ((cable_type >= SB_CBL_UNKNOWN) &&
		(cable_type < SB_CBL_MAX) &&
		(battery->pdata->cable_source_type &
		SEC_BATTERY_CABLE_SOURCE_EXTERNAL)) {

		if (cable_type != battery->cable_type) {
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
				&battery->cable_work, 0);
		} else {
			dev_info(battery->dev,
				"%s: Cable is Not Changed(%d)\n",
				__func__, battery->cable_type);
		}
	} else {
		if (sec_bat_get_cable_type(battery,
					battery->pdata->cable_source_type)) {
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
				&battery->cable_work, 0);
		}
	}
}

void sec_bat_set_charging_status(struct sec_battery_info *battery,
		int status)
{
	union power_supply_propval value = {0, };

	switch (status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
	case POWER_SUPPLY_STATUS_DISCHARGING:
		if ((battery->status == POWER_SUPPLY_STATUS_FULL ||
			(battery->capacity == 100 && !is_slate_mode(battery))) &&
			!battery->store_mode) {
			value.intval = 100;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CHARGE_FULL, value);
			/* To get SOC value (NOT raw SOC), need to reset value */
			value.intval = 0;
			psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_PROP_CAPACITY, value);
			battery->capacity = value.intval;
		}
		battery->expired_time = battery->pdata->expired_time;
		battery->prev_safety_time = 0;
		break;
	case POWER_SUPPLY_STATUS_FULL:
		if (is_wireless_type(battery->cable_type)) {
			bool send_cs100_cmd = true;

#ifdef CONFIG_CS100_JPNCONCEPT
			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);

			/* In case of the JPN PAD, this pad blocks the charge after give the cs100 command. */
			send_cs100_cmd = (battery->charging_mode == SEC_BATTERY_CHARGING_2ND ||	value.intval);
#endif
			if (send_cs100_cmd) {
				value.intval = POWER_SUPPLY_STATUS_FULL;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_STATUS, value);
			}
		}
		break;
	default:
		break;
	}
	battery->status = status;
}

static bool sec_bat_battery_cable_check(struct sec_battery_info *battery)
{
	if (!sec_bat_check(battery)) {
		if (battery->check_count < battery->pdata->check_count)
			battery->check_count++;
		else {
			dev_err(battery->dev,
				"%s: Battery Disconnected\n", __func__);
			battery->present = false;
			battery->health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;

			if (battery->status !=
				POWER_SUPPLY_STATUS_DISCHARGING) {
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_NOT_CHARGING);
				sec_vote(battery->chgen_vote, POWER_SUPPLY_HEALTH_UNSPEC_FAILURE,
						true, SEC_BAT_CHG_MODE_BUCK_OFF);
			}

			if (battery->pdata->check_battery_result_callback)
				battery->pdata->
					check_battery_result_callback();
			return false;
		}
	} else
		battery->check_count = 0;

	battery->present = true;

	if (battery->health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		battery->health = POWER_SUPPLY_HEALTH_GOOD;

		if (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);
			sec_vote(battery->chgen_vote, POWER_SUPPLY_HEALTH_UNSPEC_FAILURE, false, 0);
		}
	}

	dev_dbg(battery->dev, "%s: Battery Connected\n", __func__);

	if (battery->pdata->cable_check_type &
		SEC_BATTERY_CABLE_CHECK_POLLING) {
		if (sec_bat_get_cable_type(battery,
			battery->pdata->cable_source_type)) {
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
		}
	}
	return true;
}

static int sec_bat_ovp_uvlo_by_psy(struct sec_battery_info *battery)
{
	char *psy_name = NULL;
	union power_supply_propval value = {0, };

	value.intval = POWER_SUPPLY_HEALTH_GOOD;

	switch (battery->pdata->ovp_uvlo_check_type) {
	case SEC_BATTERY_OVP_UVLO_PMICPOLLING:
		psy_name = battery->pdata->pmic_name;
		break;
	case SEC_BATTERY_OVP_UVLO_CHGPOLLING:
		psy_name = battery->pdata->charger_name;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid OVP/UVLO Check Type\n", __func__);
		goto ovp_uvlo_check_error;
		break;
	}

	psy_do_property(psy_name, get,
		POWER_SUPPLY_PROP_HEALTH, value);

ovp_uvlo_check_error:
	return value.intval;
}

static bool sec_bat_ovp_uvlo_result(
		struct sec_battery_info *battery, int health)
{
	if (battery->health != health) {
		battery->health = health;
		switch (health) {
		case POWER_SUPPLY_HEALTH_GOOD:
			dev_info(battery->dev, "%s: Safe voltage\n", __func__);
			dev_info(battery->dev, "%s: is_recharging : %d\n", __func__, battery->is_recharging);
			sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_CHARGING);
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
			sec_vote(battery->chgen_vote, VOTER_VBUS_OVP, false, 0);
			battery->health_check_count = 0;
			break;
		case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
			dev_info(battery->dev,
				"%s: Unsafe voltage (%d)\n",
				__func__, health);
			sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_NOT_CHARGING);
			sec_vote(battery->chgen_vote, VOTER_VBUS_OVP, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->is_recharging = false;
			battery->health_check_count = DEFAULT_HEALTH_CHECK_COUNT;
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_UNSAFETY_VOLTAGE]++;
			battery->cisd.data[CISD_DATA_UNSAFE_VOLTAGE_PER_DAY]++;
#endif
			/*
			 * Take the wakelock during 10 seconds
			 * when over-voltage status is detected
			 */
			__pm_wakeup_event(battery->vbus_ws, jiffies_to_msecs(HZ * 10));
			break;
		}
		power_supply_changed(battery->psy_bat);
		return true;
	}

	return false;
}

static bool sec_bat_ovp_uvlo(struct sec_battery_info *battery)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;

	if (battery->wdt_kick_disable) {
		dev_dbg(battery->dev,
			"%s: No need to check in wdt test\n",
			__func__);
		return false;
	} else if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
		(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
		dev_dbg(battery->dev, "%s: No need to check in Full status", __func__);
		return false;
	}

	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_OVERVOLTAGE &&
		battery->health != POWER_SUPPLY_HEALTH_UNDERVOLTAGE) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return false;
	}

	health = battery->health;

	switch (battery->pdata->ovp_uvlo_check_type) {
	case SEC_BATTERY_OVP_UVLO_CALLBACK:
		if (battery->pdata->ovp_uvlo_callback)
			health = battery->pdata->ovp_uvlo_callback();
		break;
	case SEC_BATTERY_OVP_UVLO_PMICPOLLING:
	case SEC_BATTERY_OVP_UVLO_CHGPOLLING:
		health = sec_bat_ovp_uvlo_by_psy(battery);
		break;
	case SEC_BATTERY_OVP_UVLO_PMICINT:
	case SEC_BATTERY_OVP_UVLO_CHGINT:
		/* nothing for interrupt check */
	default:
		break;
	}

	/*
	 * Move the location for calling the get_health
	 * in case of attaching the jig
	 */
	if (battery->factory_mode || battery->is_jig_on) {
		dev_dbg(battery->dev,
			"%s: No need to check in factory mode\n",
			__func__);
		return false;
	}

	return sec_bat_ovp_uvlo_result(battery, health);
}

static bool sec_bat_check_recharge(struct sec_battery_info *battery)
{
	if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)
		return false;

	if ((battery->status == POWER_SUPPLY_STATUS_CHARGING) &&
			(battery->pdata->full_condition_type &
			SEC_BATTERY_FULL_CONDITION_NOTIMEFULL) &&
			(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
		dev_info(battery->dev,
				"%s: Re-charging by NOTIMEFULL (%d)\n",
				__func__, battery->capacity);
		goto check_recharge_check_count;
	}

	if (battery->status == POWER_SUPPLY_STATUS_FULL &&
			battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
		int recharging_voltage = battery->pdata->recharge_condition_vcell;
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE) {
			recharging_voltage = battery->pdata->swelling_low_rechg_voltage;
			dev_info(battery->dev, "%s: recharging voltage changed by low temp(%d)\n",
					__func__, recharging_voltage);
		}

		dev_info(battery->dev, "%s: recharging voltage (%d)\n",
				__func__, recharging_voltage);

		if ((battery->pdata->recharge_condition_type &
					SEC_BATTERY_RECHARGE_CONDITION_SOC) &&
				(battery->capacity <=
				battery->pdata->recharge_condition_soc)) {
			battery->expired_time = battery->pdata->recharging_expired_time;
			battery->prev_safety_time = 0;
			dev_info(battery->dev,
					"%s: Re-charging by SOC (%d)\n",
					__func__, battery->capacity);
			goto check_recharge_check_count;
		}

		if ((battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL) &&
			(battery->voltage_avg <= recharging_voltage)) {
			battery->expired_time = battery->pdata->recharging_expired_time;
			battery->prev_safety_time = 0;
			dev_info(battery->dev,
					"%s: Re-charging by average VCELL (%d)\n",
					__func__, battery->voltage_avg);
			goto check_recharge_check_count;
		}

		if ((battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_VCELL) &&
			(battery->voltage_now <= recharging_voltage)) {
			battery->expired_time = battery->pdata->recharging_expired_time;
			battery->prev_safety_time = 0;
			dev_info(battery->dev,
					"%s: Re-charging by VCELL (%d)\n",
					__func__, battery->voltage_now);
			goto check_recharge_check_count;
		}
	}

	battery->recharge_check_cnt = 0;
	return false;

check_recharge_check_count:
	if (battery->recharge_check_cnt <
		battery->pdata->recharge_check_count)
		battery->recharge_check_cnt++;
	dev_dbg(battery->dev,
		"%s: recharge count = %d\n",
		__func__, battery->recharge_check_cnt);

	if (battery->recharge_check_cnt >=
		battery->pdata->recharge_check_count)
		return true;
	else
		return false;
}

static bool sec_bat_voltage_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
		dev_dbg(battery->dev,
			"%s: Charging Disabled\n", __func__);
		return true;
	}

	/* OVP/UVLO check */
	if (sec_bat_ovp_uvlo(battery)) {
		if (battery->pdata->ovp_uvlo_result_callback)
			battery->pdata->
				ovp_uvlo_result_callback(battery->health);
		return false;
	}

	if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
			((battery->charging_mode != SEC_BATTERY_CHARGING_NONE &&
			battery->charger_mode == SEC_BAT_CHG_MODE_CHARGING) ||
			(battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING))) {
		int voltage_ref = battery->pdata->recharge_condition_vcell - 40;
		pr_info("%s: chg mode (%d), voltage_ref(%d), voltage_now(%d)\n",
			__func__, battery->charging_mode, voltage_ref, battery->voltage_now);

		value.intval = 0;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
		if (value.intval < battery->pdata->full_condition_soc &&
				battery->voltage_now < voltage_ref) {
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);
			battery->is_recharging = false;
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
			sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, false, 0);
			sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);
			sec_votef(VN_WPC_PHM, VOTER_FULL_CHARGE, false, WPC_PHM_OFF);
			dev_info(battery->dev,
				"%s: battery status full -> charging, RepSOC(%d)\n", __func__, value.intval);
			return false;
		}
	}

	/* Re-Charging check */
	if (sec_bat_check_recharge(battery)) {
		if (battery->pdata->full_check_type !=
			SEC_BATTERY_FULLCHARGED_NONE)
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
		else
			battery->charging_mode = SEC_BATTERY_CHARGING_2ND;
		battery->is_recharging = true;

#if defined(CONFIG_BATTERY_CISD)
		battery->cisd.recharge_count++;
		battery->cisd.recharge_count_2++;
		battery->cisd.data[CISD_DATA_RECHARGING_COUNT]++;
		battery->cisd.data[CISD_DATA_RECHARGING_COUNT_PER_DAY]++;
#endif
		sec_vote(battery->chgen_vote, VOTER_CABLE, true, SEC_BAT_CHG_MODE_CHARGING);
		sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, false, 0);
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);
		sec_votef(VN_WPC_PHM, VOTER_FULL_CHARGE, false, WPC_PHM_OFF);
		return false;
	}

	return true;
}

#if defined(CONFIG_BATTERY_AGE_FORECAST)
static bool sec_bat_set_aging_step(struct sec_battery_info *battery, int step)
{
	union power_supply_propval value = {0, };

	if (battery->pdata->num_age_step <= 0 || step < 0 || step >= battery->pdata->num_age_step) {
		pr_info("%s: [AGE] abnormal age step : %d/%d\n",
			__func__, step, battery->pdata->num_age_step-1);
		return false;
	}

	battery->pdata->age_step = step;

	/* float voltage */
	battery->pdata->chg_float_voltage =
		battery->pdata->age_data[battery->pdata->age_step].float_voltage;
	sec_vote(battery->fv_vote, VOTER_AGING_STEP, true, battery->pdata->chg_float_voltage);

	/* full/recharge condition */
	battery->pdata->recharge_condition_vcell =
		battery->pdata->age_data[battery->pdata->age_step].recharge_condition_vcell;
	battery->pdata->full_condition_soc =
		battery->pdata->age_data[battery->pdata->age_step].full_condition_soc;
	battery->pdata->full_condition_vcell =
		battery->pdata->age_data[battery->pdata->age_step].full_condition_vcell;

#if defined(CONFIG_UPDATE_BATT_PARAM_FG)
	value.intval = battery->pdata->age_step;
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_EXT_PROP_UPDATE_BATTERY_DATA, value);

	value.intval = battery->pdata->age_cc_cv_threshold[battery->pdata->age_step];
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_UPDATE_BATTERY_DATA, value);
#else
	value.intval = battery->pdata->full_condition_soc;
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_CAPACITY_LEVEL, value);
#endif

	dev_info(battery->dev,
		"%s: Step(%d/%d), Cycle(%d), float_v(%d), r_v(%d), f_s(%d), f_vl(%d)\n",
		__func__,
		battery->pdata->age_step, battery->pdata->num_age_step-1, battery->batt_cycle,
		battery->pdata->chg_float_voltage,
		battery->pdata->recharge_condition_vcell,
		battery->pdata->full_condition_soc,
		battery->pdata->full_condition_vcell);

	return true;
}

void sec_bat_aging_check(struct sec_battery_info *battery)
{
	int prev_step = battery->pdata->age_step;
	int calc_step = -1, temp;
	bool ret = 0;

	if (battery->pdata->num_age_step <= 0 || battery->batt_cycle < 0)
		return;

	sb_adc_get_valuef(BATT_TEMP, &temp);
	if (temp < 50) {
		pr_info("%s: [AGE] skip (temperature:%d)\n", __func__, temp);
		return;
	}

	for (calc_step = battery->pdata->num_age_step - 1; calc_step >= 0; calc_step--) {
		if (battery->pdata->age_data[calc_step].cycle <= battery->batt_cycle)
			break;
	}

	if (calc_step == prev_step) {
		if (battery->pdata->chg_float_voltage !=
			battery->pdata->age_data[calc_step].float_voltage) {
			/* float voltage */
			battery->pdata->chg_float_voltage =
				battery->pdata->age_data[calc_step].float_voltage;
			sec_vote(battery->fv_vote, VOTER_AGING_STEP, true,
				battery->pdata->chg_float_voltage);

			/* full/recharge condition */
			battery->pdata->recharge_condition_vcell =
				battery->pdata->age_data[calc_step].recharge_condition_vcell;
			battery->pdata->full_condition_soc =
				battery->pdata->age_data[calc_step].full_condition_soc;
			battery->pdata->full_condition_vcell =
				battery->pdata->age_data[calc_step].full_condition_vcell;
		}
		return;
	}

	ret = sec_bat_set_aging_step(battery, calc_step);
	dev_info(battery->dev,
		"%s: %s change step (%d->%d), Cycle(%d)\n",
		__func__, ret ? "Succeed in" : "Fail to",
		prev_step, battery->pdata->age_step, battery->batt_cycle);
}
#endif

static bool sec_bat_check_fullcharged_condition(
					struct sec_battery_info *battery)
{
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
		full_check_type = battery->pdata->full_check_type;
	else
		full_check_type = battery->pdata->full_check_type_2nd;

	switch (full_check_type) {
	case SEC_BATTERY_FULLCHARGED_ADC:
	case SEC_BATTERY_FULLCHARGED_FG_CURRENT:
	case SEC_BATTERY_FULLCHARGED_SOC:
	case SEC_BATTERY_FULLCHARGED_CHGGPIO:
	case SEC_BATTERY_FULLCHARGED_CHGPSY:
		break;

	/* If these is NOT full check type or NONE full check type,
	 * it is full-charged
	 */
	case SEC_BATTERY_FULLCHARGED_CHGINT:
	case SEC_BATTERY_FULLCHARGED_TIME:
	case SEC_BATTERY_FULLCHARGED_NONE:
	default:
		return true;
		break;
	}

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
	if (battery->capacity >= 100 &&
		!battery->is_recharging) {
		dev_info(battery->dev,
			"%s: enough SOC (%d%%), skip other full_condition_type\n",
			__func__, battery->capacity);
		return true;
	}
#endif

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_SOC) {
		if (battery->capacity <
			battery->pdata->full_condition_soc) {
			dev_dbg(battery->dev,
				"%s: Not enough SOC (%d%%)\n",
				__func__, battery->capacity);
			return false;
		}
	}

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_VCELL) {
		if (battery->voltage_now <
			battery->pdata->full_condition_vcell) {
			dev_dbg(battery->dev,
				"%s: Not enough VCELL (%dmV)\n",
				__func__, battery->voltage_now);
			return false;
		}
	}

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_AVGVCELL) {
		if (battery->voltage_avg <
			battery->pdata->full_condition_avgvcell) {
			dev_dbg(battery->dev,
				"%s: Not enough AVGVCELL (%dmV)\n",
				__func__, battery->voltage_avg);
			return false;
		}
	}

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_OCV) {
		if (battery->voltage_ocv <
			battery->pdata->full_condition_ocv) {
			dev_dbg(battery->dev,
				"%s: Not enough OCV (%dmV)\n",
				__func__, battery->voltage_ocv);
			return false;
		}
	}

	return true;
}

static void sec_bat_do_test_function(
		struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	switch (battery->test_mode) {
		case 1:
			if (battery->status == POWER_SUPPLY_STATUS_CHARGING) {
				sec_vote(battery->chgen_vote, VOTER_TEST, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_DISCHARGING);
			}
			break;
		case 2:
			if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
				sec_vote(battery->chgen_vote, VOTER_TEST, true, SEC_BAT_CHG_MODE_CHARGING);
				psy_do_property(battery->pdata->charger_name, get,
						POWER_SUPPLY_PROP_STATUS, value);
				sec_bat_set_charging_status(battery, value.intval);
			}
			battery->test_mode = 0;
			break;
		case 3: // clear temp block
			battery->health = POWER_SUPPLY_HEALTH_GOOD;
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_DISCHARGING);
			break;
		case 4:
			if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
				sec_vote(battery->chgen_vote, VOTER_TEST, true, SEC_BAT_CHG_MODE_CHARGING);
				psy_do_property(battery->pdata->charger_name, get,
						POWER_SUPPLY_PROP_STATUS, value);
				sec_bat_set_charging_status(battery, value.intval);
			}
			break;
		default:
			pr_info("%s: error test: unknown state\n", __func__);
			break;
	}
}

static bool sec_bat_time_management(
				struct sec_battery_info *battery)
{
	struct timespec ts = {0, };
	unsigned long charging_time;

	if (battery->charging_start_time == 0 || !battery->safety_timer_set) {
		dev_dbg(battery->dev,
			"%s: Charging Disabled\n", __func__);
		return true;
	}

	get_monotonic_boottime(&ts);

	if (ts.tv_sec >= battery->charging_start_time) {
		charging_time = ts.tv_sec - battery->charging_start_time;
	} else {
		charging_time = 0xFFFFFFFF - battery->charging_start_time
			+ ts.tv_sec;
	}

	battery->charging_passed_time = charging_time;

	switch (battery->status) {
	case POWER_SUPPLY_STATUS_FULL:
		if (battery->expired_time == 0) {
			dev_info(battery->dev,
				"%s: Recharging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->health = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
			battery->is_recharging = false;
			sec_votef(VN_WPC_PHM, VOTER_TIME_EXPIRED, true, WPC_PHM_ON);
			sec_vote(battery->chgen_vote, VOTER_TIME_EXPIRED, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			return false;
		}
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		if ((battery->pdata->full_condition_type &
			SEC_BATTERY_FULL_CONDITION_NOTIMEFULL) &&
			(battery->is_recharging && (battery->expired_time == 0))) {
			dev_info(battery->dev,
			"%s: Recharging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->health = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
			battery->is_recharging = false;
			sec_votef(VN_WPC_PHM, VOTER_TIME_EXPIRED, true, WPC_PHM_ON);
			sec_vote(battery->chgen_vote, VOTER_TIME_EXPIRED, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			return false;
		} else if (!battery->is_recharging &&
			(battery->expired_time == 0)) {
			dev_info(battery->dev,
				"%s: Charging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->health = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_SAFETY_TIMER]++;
			battery->cisd.data[CISD_DATA_SAFETY_TIMER_PER_DAY]++;
#endif
#if defined(CONFIG_SEC_ABC)
			sec_abc_send_event("MODULE=battery@ERROR=safety_timer");
#endif
			sec_votef(VN_WPC_PHM, VOTER_TIME_EXPIRED, true, WPC_PHM_ON);
			sec_vote(battery->chgen_vote, VOTER_TIME_EXPIRED, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			return false;
		}
		break;
	default:
		dev_err(battery->dev,
			"%s: Undefine Battery Status\n", __func__);
		return true;
	}

	return true;
}

bool sec_bat_check_full(struct sec_battery_info *battery, int full_check_type)
{
	union power_supply_propval value = {0, };
	bool ret = false;
	int err = 0;

	switch (full_check_type) {
	case SEC_BATTERY_FULLCHARGED_FG_CURRENT:
		if ((battery->current_now > 0 && battery->current_now <
			battery->pdata->full_check_current_1st) &&
			(battery->current_avg > 0 && battery->current_avg <
			(battery->charging_mode ==
			SEC_BATTERY_CHARGING_1ST ?
			battery->pdata->full_check_current_1st :
			battery->pdata->full_check_current_2nd))) {
				battery->full_check_cnt++;
				dev_dbg(battery->dev,
				"%s: Full Check Current (%d)\n",
				__func__,
				battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	case SEC_BATTERY_FULLCHARGED_TIME:
		if ((battery->charging_mode ==
			SEC_BATTERY_CHARGING_2ND ?
			(battery->charging_passed_time -
			battery->charging_fullcharged_time) :
			battery->charging_passed_time) >
			(battery->charging_mode ==
			SEC_BATTERY_CHARGING_1ST ?
			battery->pdata->full_check_current_1st :
			battery->pdata->full_check_current_2nd)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev,
				"%s: Full Check Time (%d)\n",
				__func__,
				battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	case SEC_BATTERY_FULLCHARGED_SOC:
		if (battery->capacity >=
			(battery->charging_mode ==
			SEC_BATTERY_CHARGING_1ST ?
			battery->pdata->full_check_current_1st :
			battery->pdata->full_check_current_2nd)) {
			battery->full_check_cnt++;
			dev_info(battery->dev,
				"%s: Full Check SOC (%d)\n",
				__func__,
				battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		err = gpio_request(
			battery->pdata->chg_gpio_full_check,
			"GPIO_CHG_FULL");
		if (err) {
			dev_err(battery->dev,
				"%s: Error in Request of GPIO\n", __func__);
			break;
		}
		if (!(gpio_get_value_cansleep(
			battery->pdata->chg_gpio_full_check) ^
			!battery->pdata->chg_polarity_full_check)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev,
				"%s: Full Check GPIO (%d)\n",
				__func__, battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		gpio_free(battery->pdata->chg_gpio_full_check);
		break;

	case SEC_BATTERY_FULLCHARGED_CHGINT:
	case SEC_BATTERY_FULLCHARGED_CHGPSY:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_STATUS, value);

		if (value.intval == POWER_SUPPLY_STATUS_FULL) {
			battery->full_check_cnt++;
			dev_info(battery->dev,
				"%s: Full Check Charger (%d)\n",
				__func__, battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	/* If these is NOT full check type or NONE full check type,
	 * it is full-charged
	 */
	case SEC_BATTERY_FULLCHARGED_ADC:
	case SEC_BATTERY_FULLCHARGED_NONE:
		battery->full_check_cnt = 0;
		ret = true;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid Full Check\n", __func__);
		break;
	}

	if ((battery->current_now > 0) &&
		(battery->current_now <= battery->pdata->ux_full_inow) &&
		(battery->charging_mode == SEC_BATTERY_CHARGING_1ST) &&
		(battery->thermal_zone != BAT_THERMAL_COOL3) &&
		!battery->is_recharging) {
		battery->full_check_cnt++;
		dev_info(battery->dev,
			"%s: UX Full INOW (%d, %d)\n",
			__func__, battery->current_now, battery->pdata->ux_full_inow);
	}

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
	if (battery->capacity >= 100 &&
		battery->charging_mode == SEC_BATTERY_CHARGING_1ST &&
		!battery->is_recharging) {
		battery->full_check_cnt++;
		dev_info(battery->dev,
			"%s: enough SOC to make FULL(%d%%)\n",
			__func__, battery->capacity);
	}
#endif

	if (battery->full_check_cnt >=
		battery->pdata->full_check_count) {
		battery->full_check_cnt = 0;
		ret = true;
	}

	return ret;
}

static bool sec_bat_check_fullcharged(struct sec_battery_info *battery)
{
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;

	if (!sec_bat_check_fullcharged_condition(battery))
		return false;

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
		full_check_type = battery->pdata->full_check_type;
	else
		full_check_type = battery->pdata->full_check_type_2nd;

	return sec_bat_check_full(battery, full_check_type);
}

static void sec_bat_do_fullcharged(
				struct sec_battery_info *battery, bool force_fullcharged)
{
	union power_supply_propval value = {0, };

	/* To let charger/fuel gauge know the full status,
	 * set status before calling sec_bat_set_charge()
	 */
#if defined(CONFIG_BATTERY_CISD)
	struct timespec now_ts;

	if (battery->status != POWER_SUPPLY_STATUS_FULL) {
		battery->cisd.data[CISD_DATA_FULL_COUNT]++;
		battery->cisd.data[CISD_DATA_FULL_COUNT_PER_DAY]++;
	}
#endif
	sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_FULL);

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST &&
		battery->pdata->full_check_type_2nd != SEC_BATTERY_FULLCHARGED_NONE && !force_fullcharged) {
		battery->charging_mode = SEC_BATTERY_CHARGING_2ND;
		battery->charging_fullcharged_time = battery->charging_passed_time;
		sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, true, battery->pdata->full_check_current_2nd);
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true, SEC_BAT_CHG_MODE_CHARGING);
		pr_info("%s: 1st charging is done\n", __func__);
	} else {
#if defined(CONFIG_BATTERY_CISD)
		now_ts = ktime_to_timespec(ktime_get_boottime());
		if (!battery->is_recharging) {
			battery->cisd.charging_end_time = now_ts.tv_sec;
		}

		dev_info(battery->dev, "%s: cisd - leakage EFGH start(%ld)\n",
			__func__, ((unsigned long)now_ts.tv_sec));
		battery->cisd.state &= ~(CISD_STATE_LEAK_E|CISD_STATE_LEAK_F|CISD_STATE_LEAK_G);
		battery->cisd.charging_end_time_2 = now_ts.tv_sec;
		battery->cisd.recharge_count_2 = 0;
#endif
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->is_recharging = false;
		battery->expired_time = battery->pdata->expired_time;
		battery->prev_safety_time = 0;

		if (!battery->wdt_kick_disable) {
			pr_info("%s: wdt kick enable -> Charger Off, %d\n",
				__func__, battery->wdt_kick_disable);

			if (is_wireless_type(battery->cable_type)) {
				sec_votef(VN_WPC_PHM, VOTER_FULL_CHARGE, true, WPC_PHM_ON);
				if (battery->lpcharge_boost == 0) {
					sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true,
						SEC_BAT_CHG_MODE_CHARGING_OFF);
				}
			} else
				sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true,
					SEC_BAT_CHG_MODE_CHARGING_OFF);
		} else {
			pr_info("%s: wdt kick disabled -> skip charger off, %d\n",
					__func__, battery->wdt_kick_disable);
		}

#if defined(CONFIG_BATTERY_AGE_FORECAST)
		sec_bat_aging_check(battery);
#endif

		value.intval = POWER_SUPPLY_STATUS_FULL;
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_STATUS, value);
	}

	/* platform can NOT get information of battery
	 * because wakeup time is too short to check uevent
	 * To make sure that target is wakeup if full-charged,
	 * activated wake lock in a few seconds
	 */
	if (battery->pdata->polling_type == SEC_BATTERY_MONITOR_ALARM)
		__pm_wakeup_event(battery->vbus_ws, jiffies_to_msecs(HZ * 10));
}

static bool sec_bat_fullcharged_check(
				struct sec_battery_info *battery)
{
	bool set_full_charged = false;

	if ((battery->status == POWER_SUPPLY_STATUS_DISCHARGING) ||
		(battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
		dev_dbg(battery->dev,
			"%s: No Need to Check Full-Charged\n", __func__);
		return true;
	} else if (battery->charger_mode == SEC_BAT_CHG_MODE_CHARGING_OFF) {
		if ((battery->status != POWER_SUPPLY_STATUS_FULL) ||
			(battery->capacity < 100) ||
			(battery->thermal_zone == BAT_THERMAL_NORMAL))
			return true;

		set_full_charged = true;
		dev_info(battery->dev, "%s: Fixed status to FullCharged\n", __func__);
	}

	if (sec_bat_check_fullcharged(battery) || set_full_charged) {
		union power_supply_propval value = {0, };

		if (battery->capacity < 100) {
			battery->full_check_cnt = battery->pdata->full_check_count;
		} else {
			sec_bat_do_fullcharged(battery, set_full_charged);
		}

		/* update capacity max */
		value.intval = battery->capacity;
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_CHARGE_FULL, value);
		pr_info("%s : forced full-charged sequence for the capacity(%d)\n",
				__func__, battery->capacity);
	}

	dev_info(battery->dev,
		"%s: Charging Mode : %s\n", __func__,
		battery->is_recharging ?
		sb_get_cm_str(SEC_BATTERY_CHARGING_RECHARGING) :
		sb_get_cm_str(battery->charging_mode));

	return true;
}

void sec_bat_get_battery_info(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	int batt_temp = 0, blk_temp = 0;

	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
	battery->voltage_now = value.intval;

	value.intval = SEC_BATTERY_VOLTAGE_AVERAGE;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
	battery->voltage_avg = value.intval;

	/* Do not call it to reduce time after cable_work, this function call FG full log*/
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL)) {
		value.intval = SEC_BATTERY_VOLTAGE_OCV;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
		battery->voltage_ocv = value.intval;
	}

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg = value.intval;

	/* input current limit in charger */
	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_MAX, value);
	battery->current_max = value.intval;

	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CHARGE_COUNTER, value);
	battery->charge_counter = value.intval;

	/* check abnormal status for wireless charging */
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL) &&
		is_wireless_type(battery->cable_type)) {
		value.intval = (battery->status == POWER_SUPPLY_STATUS_FULL) ?
			100 : battery->capacity;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
	}

	sb_adc_all_update();
	sb_adc_get_valuef(BATT_TEMP, &batt_temp);
	sb_adc_get_valuef(BLK_TEMP, &blk_temp);

	/* To get SOC value (NOT raw SOC), need to reset value */
	value.intval = 0;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
	/* if the battery status was full, and SOC wasn't 100% yet,
		then ignore FG SOC, and report (previous SOC +1)% */
	battery->capacity = value.intval;

	dev_info(battery->dev,
		"%s:Vnow(%dmV),Inow(%dmA),Imax(%dmA),Ichg(%dmA),SOC(%d%%),Tbat(%d), Tblk(%d)\n",
		__func__,
		battery->voltage_now, battery->current_now,
		battery->current_max, battery->charging_current,
		battery->capacity, batt_temp, blk_temp);
	dev_dbg(battery->dev,
		"%s,Vavg(%dmV),Vocv(%dmV),Iavg(%dmA)\n",
		battery->present ? "Connected" : "Disconnected",
		battery->voltage_avg, battery->voltage_ocv, battery->current_avg);

#if defined(CONFIG_SEC_DEBUG_EXTRA_INFO)
	secdbg_exin_set_batt(battery->capacity, battery->voltage_now, batt_temp, battery->current_now);
#endif
}

static void sec_bat_polling_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(
		work, struct sec_battery_info, polling_work.work);

	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	dev_dbg(battery->dev, "%s: Activated\n", __func__);
}

static void sec_bat_program_alarm(
				struct sec_battery_info *battery, int seconds)
{
	pr_info("%s seconds = %d\n", __func__, seconds);
	alarm_start(&battery->polling_alarm,
			ktime_add(battery->last_poll_time, ktime_set(seconds, 0)));
}

static unsigned int sec_bat_get_polling_time(
	struct sec_battery_info *battery)
{
	if (battery->status ==
		POWER_SUPPLY_STATUS_FULL)
		battery->polling_time =
			battery->pdata->polling_time[
			POWER_SUPPLY_STATUS_CHARGING];
	else
		battery->polling_time =
			battery->pdata->polling_time[
			battery->status];

	battery->polling_short = true;

	switch (battery->status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		if (battery->polling_in_sleep)
			battery->polling_short = false;
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		if (battery->polling_in_sleep)
			battery->polling_time =
				battery->pdata->polling_time[
					SEC_BATTERY_POLLING_TIME_SLEEP];
		else
			battery->polling_time =
				battery->pdata->polling_time[
				battery->status];
		if (!battery->wc_enable) {
			battery->polling_time = battery->pdata->polling_time[
					SEC_BATTERY_POLLING_TIME_CHARGING];
			pr_info("%s: wc_enable is false, polling time is 30sec\n", __func__);
		}
		battery->polling_short = false;
		break;
	case POWER_SUPPLY_STATUS_FULL:
		if (battery->polling_in_sleep) {
			if (!(battery->pdata->full_condition_type &
				SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL) &&
				battery->charging_mode ==
				SEC_BATTERY_CHARGING_NONE) {
				battery->polling_time =
					battery->pdata->polling_time[
						SEC_BATTERY_POLLING_TIME_SLEEP];
			}
			battery->polling_short = false;
		} else {
			if (battery->charging_mode ==
				SEC_BATTERY_CHARGING_NONE)
				battery->polling_short = false;
		}
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE ||
			(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) &&
			(battery->health_check_count > 0)) {
			battery->health_check_count--;
			battery->polling_time = 1;
			battery->polling_short = false;
		} else if (battery->health == POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE) {
			battery->polling_short = false;
		} else {
			if (battery->polling_in_sleep == true)
				battery->polling_short = false;
		}
		break;
	}

	if (battery->polling_short)
		return battery->pdata->polling_time[
			SEC_BATTERY_POLLING_TIME_BASIC];
	/* set polling time to 46s to reduce current noise on wc */
	else if (is_wireless_type(battery->cable_type) &&
			battery->status == POWER_SUPPLY_STATUS_CHARGING)
		battery->polling_time = 46;

	if (is_wired_type(battery->cable_type))
		battery->polling_time = battery->pdata->polling_time[
			SEC_BATTERY_POLLING_TIME_BASIC];

	return battery->polling_time;
}

static bool sec_bat_is_short_polling(
	struct sec_battery_info *battery)
{
	/* Change the full and short monitoring sequence
	 * Originally, full monitoring was the last time of polling_count
	 * But change full monitoring to first time
	 * because temperature check is too late
	 */
	if (!battery->polling_short || battery->polling_count == 1)
		return false;
	else
		return true;
}

static void sec_bat_update_polling_count(
	struct sec_battery_info *battery)
{
	/* do NOT change polling count in sleep
	 * even though it is short polling
	 * to keep polling count along sleep/wakeup
	 */
	if (battery->polling_short && battery->polling_in_sleep)
		return;

	if (battery->polling_short &&
		((battery->polling_time /
		battery->pdata->polling_time[
		SEC_BATTERY_POLLING_TIME_BASIC])
		> battery->polling_count))
		battery->polling_count++;
	else
		battery->polling_count = 1;	/* initial value = 1 */
}

static void sec_bat_set_polling(
	struct sec_battery_info *battery)
{
	unsigned int polling_time_temp = 0;

	dev_dbg(battery->dev, "%s: Start\n", __func__);

	polling_time_temp = sec_bat_get_polling_time(battery);

	dev_info(battery->dev,
		"%s: Status:%s, Sleep:%s, Charging:%s, Short Poll:%s\n",
		__func__, sb_get_bst_str(battery->status),
		battery->polling_in_sleep ? "Yes" : "No",
		(battery->charging_mode ==
		SEC_BATTERY_CHARGING_NONE) ? "No" : "Yes",
		battery->polling_short ? "Yes" : "No");
	dev_info(battery->dev,
		"%s: Polling time %d/%d sec.\n", __func__,
		battery->polling_short ?
		(polling_time_temp * battery->polling_count) :
		polling_time_temp, battery->polling_time);

	/* To sync with log above,
	 * change polling count after log is displayed
	 * Do NOT update polling count in initial monitor
	 */
	if (!battery->pdata->monitor_initial_count)
		sec_bat_update_polling_count(battery);
	else
		dev_dbg(battery->dev,
			"%s: Initial monitor %d times left.\n", __func__,
			battery->pdata->monitor_initial_count);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		if (battery->pdata->monitor_initial_count) {
			battery->pdata->monitor_initial_count--;
			schedule_delayed_work(&battery->polling_work, HZ);
		} else
			schedule_delayed_work(&battery->polling_work,
				polling_time_temp * HZ);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		battery->last_poll_time = ktime_get_boottime();

		if (battery->pdata->monitor_initial_count) {
			battery->pdata->monitor_initial_count--;
			sec_bat_program_alarm(battery, 1);
		} else {
			sec_bat_program_alarm(battery, polling_time_temp);
		}
		break;
	case SEC_BATTERY_MONITOR_TIMER:
		break;
	default:
		break;
	}
	dev_dbg(battery->dev, "%s: End\n", __func__);
}

#if defined(CONFIG_UPDATE_BATTERY_DATA)
static int sec_bat_parse_dt(struct device *dev, struct sec_battery_info *battery);
static void sec_bat_update_data_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, batt_data_work.work);

	sec_battery_update_data(battery->data_path);
	__pm_relax(battery->batt_data_ws);
}
#endif

static void sec_bat_misc_event_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, misc_event_work.work);

	pr_info("%s: change misc event(0x%x --> 0x%x)\n",
		__func__, battery->prev_misc_event, battery->misc_event);
	battery->prev_misc_event = battery->misc_event;
	__pm_relax(battery->misc_event_ws);

	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
}

static bool sec_bat_get_phm_status(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (is_wireless_type(battery->cable_type))
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_PHM, value);

	return value.intval;
}

static void sec_bat_calculate_safety_time(struct sec_battery_info *battery)
{
	unsigned long long expired_time = battery->expired_time;
	struct timespec ts = {0, };
	int curr = 0;
	/* use charging current in single path models */
	int input_power =
		(battery->pdata->single_charger_path ? battery->charging_current : battery->current_max) *
		battery->input_voltage * 1000;
	int charging_power = battery->charging_current * battery->pdata->chg_float_voltage;

	if (sec_bat_get_phm_status(battery) ||
		(battery->lcd_status && battery->stop_timer)) {
		battery->prev_safety_time = 0;
		return;
	}

	get_monotonic_boottime(&ts);

	if (battery->prev_safety_time == 0) {
		battery->prev_safety_time = ts.tv_sec;
	}

	if (input_power > charging_power) {
		curr = battery->charging_current;
	} else {
		curr = input_power / battery->pdata->chg_float_voltage;
		curr = (curr * 9) / 10;
	}

	if (battery->lcd_status && !battery->stop_timer) {
		battery->stop_timer = true;
	} else if (!battery->lcd_status && battery->stop_timer) {
		battery->stop_timer = false;
	}

	pr_debug("%s : EXPIRED_TIME(%llu), IP(%d), CP(%d), CURR(%d), STANDARD(%d)\n",
		__func__, expired_time, input_power, charging_power, curr, battery->pdata->standard_curr);

	if (curr == 0)
		return;
	else if (curr > battery->pdata->standard_curr)
		curr = battery->pdata->standard_curr;

	expired_time *= battery->pdata->standard_curr;
	do_div(expired_time, curr);

	pr_debug("%s : CAL_EXPIRED_TIME(%llu) TIME NOW(%ld) TIME PREV(%ld)\n",
		__func__, expired_time, ts.tv_sec, battery->prev_safety_time);

	if (expired_time <= ((ts.tv_sec - battery->prev_safety_time) * 1000))
		expired_time = 0;
	else
		expired_time -= ((ts.tv_sec - battery->prev_safety_time) * 1000);

	battery->cal_safety_time = expired_time;
	expired_time *= curr;
	do_div(expired_time, battery->pdata->standard_curr);

	battery->expired_time = expired_time;
	battery->prev_safety_time = ts.tv_sec;
	pr_debug("%s : REMAIN_TIME(%ld) CAL_REMAIN_TIME(%ld)\n",
		__func__, battery->expired_time, battery->cal_safety_time);
}

static void sec_bat_check_store_mode(struct sec_battery_info *battery)
{
	unsigned int chg_max = battery->pdata->store_mode_chg_max,
		chg_min = battery->pdata->store_mode_chg_min;
	union power_supply_propval val = {0, };

	if (is_nocharge_type(battery->cable_type) ||
		is_slate_mode(battery) ||
		!battery->store_mode)
		return;

	dev_info(battery->dev,
		"%s capacity(%d %d %d) status(%d) store_mode(%d)\n",
		__func__, battery->capacity, chg_max, chg_min,
		battery->status, battery->store_mode);

	val.intval = battery->store_mode;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_STORE_MODE, val);

	sec_vote(battery->fcc_vote, VOTER_STORE_MODE, true,
		battery->pdata->store_wpc_current);

	if (battery->capacity >= chg_max) {
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
		sec_votef(VN_WPC_APM, VOTER_STORE_MODE, true, WPC_APM_STORE);
		sec_votef(VN_WPC_PHM, VOTER_STORE_MODE, true, WPC_PHM_ON);
		sec_vote(battery->chgen_vote, VOTER_STORE_MODE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
		battery->health = POWER_SUPPLY_HEALTH_GOOD;
	}

	if ((battery->capacity <= chg_min) &&
		(battery->status == POWER_SUPPLY_STATUS_DISCHARGING)) {
		sec_vote(battery->chgen_vote, VOTER_STORE_MODE, false, 0);
		sec_votef(VN_WPC_PHM, VOTER_STORE_MODE, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_APM, VOTER_STORE_MODE, false, WPC_APM_NONE);
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
	}
}

static void sec_bat_monitor_work(
				struct work_struct *work)
{
	struct sec_battery_info *battery =
		container_of(work, struct sec_battery_info,
		monitor_work.work);
	static struct timespec old_ts = {0, };
	struct timespec c_ts = {0, };
	union power_supply_propval val = {0, };

	dev_info(battery->dev, "%s: Start\n", __func__);

	c_ts = ktime_to_timespec(ktime_get_boottime());
	if (!battery->wc_enable) {
		pr_info("%s: wc_enable(%d), cnt(%d)\n",
			__func__, battery->wc_enable, battery->wc_enable_cnt);
		if ((battery->wc_enable_cnt++) > battery->wc_enable_cnt_value) {
			battery->wc_enable = true;
			battery->wc_enable_cnt = 0;
			sec_votef(VN_WPC_EN, VOTER_WPC_CONTROL,
				!battery->wc_enable, battery->wc_enable);
		}
	}

	/* monitor once after wakeup */
	if (battery->polling_in_sleep) {
		battery->polling_in_sleep = false;
		if ((battery->status == POWER_SUPPLY_STATUS_DISCHARGING) &&
			(!battery->store_mode)) {
			if ((unsigned long)(c_ts.tv_sec - old_ts.tv_sec) < 10 * 60) {
				union power_supply_propval value = {0, };
				int batt_temp = 0, blk_temp = 0;

				psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
				battery->voltage_now = value.intval;

				value.intval = 0;
				psy_do_property(battery->pdata->fuelgauge_name, get,
						POWER_SUPPLY_PROP_CAPACITY, value);
				battery->capacity = value.intval;

				sb_adc_all_update();
				sb_adc_get_valuef(BATT_TEMP, &batt_temp);
				sb_adc_get_valuef(BLK_TEMP, &blk_temp);
#if defined(CONFIG_BATTERY_CISD)
				sec_bat_cisd_check(battery);
#endif
				power_supply_changed(battery->psy_bat);
				pr_info("Skip monitor work(%ld, Vnow:%d(mV), SoC:%d(%%), Tbat:%d(0.1'C), Tblk:%d(0.1'C))\n",
						c_ts.tv_sec - old_ts.tv_sec, battery->voltage_now, battery->capacity,
						batt_temp, blk_temp);

				goto skip_monitor;
			}
		}
	}
	/* update last monitor time */
	old_ts = c_ts;

	sec_bat_get_battery_info(battery);
#if defined(CONFIG_BATTERY_CISD)
	sec_bat_cisd_check(battery);
#endif

	/* time to full check */
	sec_bat_calc_time_to_full(battery);

	/* 0. test mode */
	if (battery->test_mode) {
		dev_err(battery->dev, "%s: Test Mode\n", __func__);
		sec_bat_do_test_function(battery);
		if (battery->test_mode != 0)
			goto continue_monitor;
	}

	/* 1. battery check */
	if (!sec_bat_battery_cable_check(battery))
		goto continue_monitor;

	/* 2. voltage check */
	if (!sec_bat_voltage_check(battery))
		goto continue_monitor;

	/* monitor short routine in initial monitor */
	if (battery->pdata->monitor_initial_count || sec_bat_is_short_polling(battery))
		goto skip_current_monitor;

	/* 3. time management */
	if (!sec_bat_time_management(battery))
		goto continue_monitor;

	/* 4. bat thm check */
	if (!sec_bat_check_maintain_status(battery))
		sec_bat_thermal_check(battery);

	/* 5. full charging check */
	sec_bat_fullcharged_check(battery);

	/* 6. additional check */
	if (battery->pdata->monitor_additional_check)
		battery->pdata->monitor_additional_check();

continue_monitor:
	/* clear HEATING_CONTROL*/
	sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);

	/* calculate safety time */
	if (battery->charger_mode == SEC_BAT_CHG_MODE_CHARGING)
		sec_bat_calculate_safety_time(battery);

	/* set charging current */
	sec_bat_set_charging_current(battery);

skip_current_monitor:
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);

	if (is_wireless_type(battery->cable_type)) {
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WDT_KICK, val);

		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_CHARGE_MONITOR, val);
	}

	pr_info("%s: Sts(%s), mode(%s), Health(%s), Cable(%s %d), tz(%s), lcd(%d), slate(%d), store(%d), st(%lu)"
#if defined(CONFIG_BATTERY_AGE_FORECAST)
		", cyc(%d)"
#endif
		", lpm(%d)\n", __func__,
		sb_get_bst_str(battery->status),
		sb_get_cm_str(battery->charging_mode),
		sb_get_hl_str(battery->health),
		sb_get_ct_str(battery->cable_type),
		battery->muic_cable_type,
		sb_get_tz_str(battery->thermal_zone),
		battery->lcd_status,
		is_slate_mode(battery),
		battery->store_mode,
		(battery->expired_time / 1000)
#if defined(CONFIG_BATTERY_AGE_FORECAST)
		, battery->batt_cycle
#endif
		, lpcharge
		);
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	dev_info(battery->dev,
			"%s: battery->stability_test(%d), battery->eng_not_full_status(%d)\n",
			__func__, battery->stability_test, battery->eng_not_full_status);
#endif

	sec_bat_check_store_mode(battery);

	power_supply_changed(battery->psy_bat);

skip_monitor:
	val.intval = battery->cable_type;
	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);

	sec_bat_set_polling(battery);

	if (battery->capacity <= 0 || battery->health_change)
		__pm_wakeup_event(battery->monitor_ws, jiffies_to_msecs(HZ * 5));
	else
		__pm_relax(battery->monitor_ws);

	dev_info(battery->dev, "%s: End\n", __func__);

	return;
}

static enum alarmtimer_restart sec_bat_alarm(
	struct alarm *alarm, ktime_t now)
{
	struct sec_battery_info *battery = container_of(alarm,
				struct sec_battery_info, polling_alarm);

	dev_dbg(battery->dev,
			"%s\n", __func__);

	/* In wake up, monitor work will be queued in complete function
	 * To avoid duplicated queuing of monitor work,
	 * do NOT queue monitor work in wake up by polling alarm
	 */
	if (!battery->polling_in_sleep) {
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_dbg(battery->dev, "%s: Activated\n", __func__);
	}

	return ALARMTIMER_NORESTART;
}

static bool sec_bat_skip_clear_voter(int event)
{
	return (event == VOTER_SLATE ||	event == VOTER_TEST ||
		event == VOTER_AGING_STEP || event == VOTER_BOOT_MODE);
}

static void sec_bat_cable_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, cable_work.work);
	union power_supply_propval val = {0, };
	struct sec_charging_current *chg_cur;
	int current_cable_type = SB_CBL_NONE;
	int prev_cable_type;
	int i;

	dev_info(battery->dev, "%s: Start\n", __func__);
	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL,
				SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);

	current_cable_type = battery->wire_status;
	prev_cable_type = battery->cable_type;
	battery->cable_type = current_cable_type;

	sec_bat_set_threshold(battery);

	/* set online(cable type) */
	val.intval = battery->cable_type;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_ONLINE, val);
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_ONLINE, val);
	sec_vote_refresh(battery->chgen_vote);

	if (battery->pdata->check_cable_result_callback)
		battery->pdata->check_cable_result_callback(battery->cable_type);
	/* platform can NOT get information of cable connection
	 * because wakeup time is too short to check uevent
	 * To make sure that target is wakeup
	 * if cable is connected and disconnected,
	 * activated wake lock in a few seconds
	 */
	__pm_wakeup_event(battery->vbus_ws, jiffies_to_msecs(HZ * 10));

	if (battery->cable_type == SB_CBL_NONE ||
		((battery->pdata->cable_check_type &
		SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE) &&
		battery->cable_type == SB_CBL_UNKNOWN)) {
		/* initialize all status */
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->is_recharging = false;
#if defined(CONFIG_BATTERY_CISD)
		battery->cisd.charging_end_time = 0;
		battery->cisd.recharge_count = 0;
		battery->cisd.charging_end_time_2 = 0;
		battery->cisd.recharge_count_2 = 0;
		battery->cisd.ab_vbat_check_count = 0;
		battery->cisd.state &= ~CISD_STATE_OVER_VOLTAGE;
#endif
		battery->input_voltage = 0;
		battery->charge_power = 0;
		battery->max_charge_power = 0;
		battery->chg_limit = false;
		battery->cstep = 0;
		battery->cstep_applied = 0;
		sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_DISCHARGING);
		battery->health = POWER_SUPPLY_HEALTH_GOOD;
		/* usb default current is 100mA before configured*/
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA,
					(SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE |
					SEC_BAT_CURRENT_EVENT_AFC |
					SEC_BAT_CURRENT_EVENT_USB_SUPER |
					SEC_BAT_CURRENT_EVENT_USB_100MA |
					SEC_BAT_CURRENT_EVENT_VBAT_OVP |
					SEC_BAT_CURRENT_EVENT_VSYS_OVP |
					SEC_BAT_CURRENT_EVENT_CHG_LIMIT |
					SEC_BAT_CURRENT_EVENT_AICL |
					SEC_BAT_CURRENT_EVENT_SELECT_PDO));
		sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_WIRELESS_MISALIGN);

		sec_votef(VN_WPC_PHM, VOTER_SWELLING, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_PHM, VOTER_FULL_CHARGE, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_PHM, VOTER_TIME_EXPIRED, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_PHM, VOTER_STORE_MODE, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_APM, VOTER_STORE_MODE, false, WPC_APM_NONE);
		for (i = 0; i < VOTER_MAX; i++) {
			if (sec_bat_skip_clear_voter(i))
				continue;
			sec_vote(battery->topoff_vote, i, false, 0);
			sec_vote(battery->chgen_vote, i, false, 0);
			sec_vote(battery->input_vote, i, false, 0);
			sec_vote(battery->fcc_vote, i, false, 0);
			sec_vote(battery->fv_vote, i, false, 0);
		}
	} else if (is_slate_mode(battery)) {
		dev_info(battery->dev,
			"%s:slate mode on\n", __func__);
		/* Some charger ic's buck is enabled after vbus off, So disable buck again*/
		sec_vote_refresh(battery->chgen_vote);
		battery->is_recharging = false;
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->health = POWER_SUPPLY_HEALTH_GOOD;
		sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_DISCHARGING);

		sec_votef(VN_WPC_PHM, VOTER_SWELLING, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_PHM, VOTER_FULL_CHARGE, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_PHM, VOTER_TIME_EXPIRED, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_PHM, VOTER_STORE_MODE, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_APM, VOTER_STORE_MODE, false, WPC_APM_NONE);
		for (i = 0; i < VOTER_MAX; i++) {
			if (sec_bat_skip_clear_voter(i))
				continue;
			sec_vote(battery->topoff_vote, i, false, 0);
			sec_vote(battery->chgen_vote, i, false, 0);
			sec_vote(battery->input_vote, i, false, 0);
			sec_vote(battery->fcc_vote, i, false, 0);
			sec_vote(battery->fv_vote, i, false, 0);
		}
	} else if (is_incomp_wrl_type(battery->cable_type)) {
		dev_info(battery->dev,
			"%s: incompatible type\n", __func__);
		battery->is_recharging = false;
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->health = POWER_SUPPLY_HEALTH_UNKNOWN;
		sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_NOT_CHARGING);
		sec_bat_set_misc_event(battery,
			BATT_MISC_EVENT_WIRELESS_INCOMPATIBLE,
			BATT_MISC_EVENT_WIRELESS_INCOMPATIBLE);
		sec_vote(battery->chgen_vote, VOTER_CABLE, true, SEC_BAT_CHG_MODE_BUCK_OFF);
	} else if (is_nocharge_type(prev_cable_type) ||
			is_incomp_wrl_type(prev_cable_type)) {
		battery->input_voltage = SEC_INPUT_VOLTAGE_5V;
		battery->health = POWER_SUPPLY_HEALTH_GOOD;

		dev_info(battery->dev,
			"%s: c: %d, cm: %d, tz: %d\n", __func__,
			battery->cable_type, battery->charger_mode, battery->thermal_zone);

		if (battery->pdata->full_check_type !=
			SEC_BATTERY_FULLCHARGED_NONE)
			battery->charging_mode =
				SEC_BATTERY_CHARGING_1ST;
		else
			battery->charging_mode =
				SEC_BATTERY_CHARGING_2ND;

		if (battery->status == POWER_SUPPLY_STATUS_FULL)
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_FULL);
		else
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
		if (battery->capacity >= 100) {
			sec_bat_do_fullcharged(battery, true);
			dev_info(battery->dev,
				"%s: charging start at full, do not turn on charging\n", __func__);
		} else
#endif
			sec_vote(battery->chgen_vote, VOTER_CABLE, true, SEC_BAT_CHG_MODE_CHARGING);

		/* start ttf work */
		ttf_work_start(battery);
	}

	/* set charging current */
	battery->aicl_current = 0;
	sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AICL);
	sec_vote(battery->fcc_vote, VOTER_CABLE, SEC_VOTE_FORCE_SET,
		battery->pdata->charging_current[current_cable_type].fast_charging_current);

	/* init sb-step about prev cable type */
	chg_cur = get_chg_cur_ptr(battery, prev_cable_type);
	if (!IS_ERR_OR_NULL(chg_cur))
		sb_step_init(chg_cur->st);

	/* init sb-step about new cable type */
	chg_cur = get_chg_cur_ptr(battery, current_cable_type);
	if (!IS_ERR_OR_NULL(chg_cur))
		sb_step_init(chg_cur->st);

	/* polling time should be reset when cable is changed
	 * polling_in_sleep should be reset also
	 * before polling time is re-calculated
	 * to prevent from counting 1 for events
	 * right after cable is connected
	 */
	battery->polling_in_sleep = false;
	sec_bat_get_polling_time(battery);

	dev_dbg(battery->dev,
		"%s: Status:%s, Sleep:%s, Charging:%s, Short Poll:%s\n",
		__func__, sb_get_bst_str(battery->status),
		battery->polling_in_sleep ? "Yes" : "No",
		(battery->charging_mode ==
		SEC_BATTERY_CHARGING_NONE) ? "No" : "Yes",
		battery->polling_short ? "Yes" : "No");
	dev_dbg(battery->dev,
		"%s: Polling time is reset to %d sec.\n", __func__,
		battery->polling_time);

	battery->polling_count = 1;	/* initial value = 1 */

	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	__pm_relax(battery->cable_ws);
	dev_dbg(battery->dev, "%s: End\n", __func__);
}

static int sec_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	dev_dbg(battery->dev,
		"%s: (%d,%d)\n", __func__, psp, val->intval);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
			full_check_type = battery->pdata->full_check_type;
		else
			full_check_type = battery->pdata->full_check_type_2nd;
		if ((full_check_type == SEC_BATTERY_FULLCHARGED_CHGINT) &&
			(val->intval == POWER_SUPPLY_STATUS_FULL))
			sec_bat_do_fullcharged(battery, false);
		sec_bat_set_charging_status(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		sec_bat_ovp_uvlo_result(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		sec_bat_set_cable_type(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		battery->capacity = val->intval;
		power_supply_changed(battery->psy_bat);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		battery->present = val->intval;

		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue,
					&battery->monitor_work, 0);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
	case POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW:
		break;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case POWER_SUPPLY_PROP_POWER_DESIGN:
		sec_bat_parse_dt(battery->dev, battery);
		break;
#endif
#if defined(CONFIG_BATTERY_CISD)
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		pr_info("%s: Valert was occured! run monitor work for updating cisd data!\n", __func__);
		battery->cisd.data[CISD_DATA_VALERT_COUNT]++;
		battery->cisd.data[CISD_DATA_VALERT_COUNT_PER_DAY]++;
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work_on(0, battery->monitor_wqueue,
			&battery->monitor_work, 0);
		break;
#endif
	case POWER_SUPPLY_PROP_WPC_FREQ_STRENGTH:
		pr_info("%s : wpc vout strength(%d)\n", __func__, val->intval);
		sec_bat_set_misc_event(battery,
			((val->intval <= 0) ? BATT_MISC_EVENT_WIRELESS_MISALIGN : 0),
			BATT_MISC_EVENT_WIRELESS_MISALIGN);
		break;

	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_AICL_CURRENT:
			battery->aicl_current = val->intval;
			battery->max_charge_power = battery->charge_power = battery->input_voltage * val->intval;
			pr_info("%s: aicl : %dmA, %dmW)\n", __func__,
				battery->aicl_current, battery->charge_power);

			if (is_wired_type(battery->cable_type))
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_AICL,
					SEC_BAT_CURRENT_EVENT_AICL);

#if defined(CONFIG_BATTERY_CISD)
#if defined(CISD_NOT_SUPPORT)
			battery->cisd.data[CISD_DATA_AICL_COUNT]++;
			battery->cisd.data[CISD_DATA_AICL_COUNT_PER_DAY]++;
#endif
#endif
#if defined(CONFIG_SEC_ABC)
			if (is_wired_type(battery->cable_type))
				sec_abc_send_event("MODULE=battery@ERROR=aicl");
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_SYSOVLO:
			if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: Vsys is ovlo !!\n", __func__);
				battery->is_recharging = false;
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				battery->health = POWER_SUPPLY_HEALTH_VSYS_OVP;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_VSYS_OVP, SEC_BAT_CURRENT_EVENT_VSYS_OVP);
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
#if defined(CONFIG_BATTERY_CISD)
#if defined(CISD_NOT_SUPPORT)
				battery->cisd.data[CISD_DATA_VSYS_OVP]++;
				battery->cisd.data[CISD_DATA_VSYS_OVP_PER_DAY]++;
#endif
#endif
#if defined(CONFIG_SEC_ABC)
				sec_abc_send_event("MODULE=battery@ERROR=vsys_ovp");
#endif
				sec_vote(battery->chgen_vote, VOTER_SYSOVLO, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
						&battery->monitor_work, 0);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_VBAT_OVP:
			if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: Vbat is ovlo !!\n", __func__);
				battery->is_recharging = false;
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				battery->health = POWER_SUPPLY_HEALTH_VBAT_OVP;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_VBAT_OVP, SEC_BAT_CURRENT_EVENT_VBAT_OVP);
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);

				sec_vote(battery->chgen_vote, VOTER_VBAT_OVP, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
						&battery->monitor_work, 0);
			}
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value = {0, };
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
			(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) {
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		} else {
			if ((battery->pdata->cable_check_type &
				SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE) &&
				!lpcharge) {
				switch (battery->cable_type) {
				case SB_CBL_USB:
					val->intval =
						POWER_SUPPLY_STATUS_DISCHARGING;
					return 0;
				}
			}
#if defined(CONFIG_STORE_MODE)
			if (battery->store_mode && !lpcharge &&
				battery->cable_type != SB_CBL_NONE &&
				battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			} else
#endif
				val->intval = battery->status;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (battery->cable_type == SB_CBL_NONE) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		} else {
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_CHARGE_TYPE, value);

			if (value.intval == POWER_SUPPLY_CHARGE_TYPE_UNKNOWN)
				/* if error in CHARGE_TYPE of charger
				 * set CHARGE_TYPE as NONE
				 */
				val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			else
				val->intval = value.intval;
		}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (battery->health >= POWER_SUPPLY_HEALTH_MAX)
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		else
			val->intval = battery->health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = battery->present;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = battery->cable_type;
		pr_info("%s cable type = %d sleep_mode = %d\n", __func__, val->intval, sleep_mode);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = battery->pdata->technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = battery->pdata->chg_float_voltage;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
#ifdef CONFIG_SEC_FACTORY
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		battery->voltage_now = value.intval;
		dev_err(battery->dev,
			"%s: voltage now(%d)\n", __func__, battery->voltage_now);
#endif
		/* voltage value should be in uV */
		val->intval = battery->voltage_now * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
#ifdef CONFIG_SEC_FACTORY
		value.intval = SEC_BATTERY_VOLTAGE_AVERAGE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
		battery->voltage_avg = value.intval;
		dev_err(battery->dev,
			"%s: voltage avg(%d)\n", __func__, battery->voltage_avg);
#endif
		/* voltage value should be in uV */
		val->intval = battery->voltage_avg * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = battery->current_now;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = battery->current_avg;
		break;
	case POWER_SUPPLY_PROP_POWER_DESIGN:
		if (is_nocharge_type(battery->cable_type)) {
			val->intval = 0;
		} else {
			struct sec_charging_current *chg_cur =
				get_chg_cur_ptr(battery, battery->cable_type);

			val->intval = battery->pdata->chg_float_voltage *
				chg_cur->fast_charging_current;
		}
		break;
	case POWER_SUPPLY_PROP_POWER_NOW:
		if (is_nocharge_type(battery->cable_type))
			val->intval = 0;
		else
			val->intval = get_sec_vote_result(battery->fv_vote) *
				get_sec_vote_result(battery->fcc_vote);
		break;
	/* charging mode (differ from power supply) */
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = battery->charging_mode;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (battery->pdata->fake_capacity) {
			val->intval = 50;
			pr_info("%s : capacity(%d)\n", __func__, val->intval);
		} else {
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
			if (battery->status == POWER_SUPPLY_STATUS_FULL) {
				if (battery->eng_not_full_status)
					val->intval = battery->capacity;
				else
					val->intval = 100;
			} else {
				val->intval = battery->capacity;
			}
#else
			if (battery->status == POWER_SUPPLY_STATUS_FULL)
				val->intval = 100;
			else
				val->intval = battery->capacity;
#endif
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		sb_adc_get_valuef(BATT_TEMP, &val->intval);
		break;
#if defined(CONFIG_FUELGAUGE_MAX77705)
	case POWER_SUPPLY_PROP_POWER_NOW:
		value.intval = SEC_BATTERY_ISYS_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_POWER_AVG:
		value.intval = SEC_BATTERY_ISYS_AVG_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
		val->intval = value.intval;
		break;
#endif
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = ttf_display(battery);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_SWELLING_MODE)
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW:
		val->intval = battery->wire_status;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = battery->charge_counter;
		break;
	case POWER_SUPPLY_PROP_WPC_FREQ_STRENGTH:
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_CHARGE_POWER:
			val->intval = battery->charge_power;
			break;
		case POWER_SUPPLY_EXT_PROP_SLATE_MODE:
			val->intval = is_slate_mode(battery);
			break;
		case POWER_SUPPLY_EXT_PROP_STORE_MODE:
			val->intval = (int)battery->store_mode;
			break;
		case POWER_SUPPLY_EXT_PROP_BOOT_BOOST_CHARGE:
			val->intval = battery->lpcharge_boost;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_usb_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
		(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) {
		val->intval = 0;
		return 0;
	}

	val->intval = (is_slate_mode(battery)) ?
		0 : is_usb_type(battery->cable_type);

	return 0;
}

static int sec_ac_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
		(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) {
		val->intval = 0;
		return 0;
	}

	val->intval = (is_slate_mode(battery)) ?
		0 : is_ta_type(battery->cable_type);

	return 0;
}

static int sec_wireless_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = is_wireless_type(battery->cable_type);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = (battery->pdata->wireless_charger_name) ?
			1 : 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_wireless_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		break;
	case POWER_SUPPLY_PROP_AUTHENTIC:
#if defined(CONFIG_BATTERY_CISD)
		pr_info("%s : tx_type(0x%x)\n", __func__, val->intval);
		count_cisd_pad_data(&battery->cisd, val->intval);
#endif
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sb_noti_set_misc_event(struct sec_battery_info *battery, struct sbn_bit_event *misc)
{
	if ((battery == NULL) || (misc == NULL))
		return -EINVAL;

	switch (misc->mask) {
	case BATT_MISC_EVENT_WIRELESS_FW_UPDATE:
	{
		struct sec_charging_current *def_chg_cur =
			get_def_chg_cur_ptr(battery);

		sec_vote(battery->fcc_vote, VOTER_WPC_FW,
			(misc->value), def_chg_cur->fast_charging_current);
	}
		break;
	default:
		break;
	}

	sec_bat_set_misc_event(battery, misc->value, misc->mask);
	return 0;
}

static int sb_noti_handler(struct notifier_block *nb, unsigned long action, void *data)
{
	struct sec_battery_info *battery =
		container_of(nb, struct sec_battery_info, sb_nb);
	int ret = 0;

	switch (action) {
	case SB_NOTIFY_EVENT_MISC:
		ret = sb_noti_set_misc_event(battery, data);
		break;
	}

	return ret;

}

#if defined(CONFIG_MUIC_NOTIFIER)
static int sec_bat_cable_check(struct sec_battery_info *battery,
		bool is_attach, muic_attached_dev_t muic_dev)
{
	int current_cable_type = -1;

	pr_info("[%s] ATTACH(%s), MUIC_DEV(%d)\n", __func__, GET_BOOL_STR(is_attach), muic_dev);
	switch (muic_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		battery->is_jig_on = is_attach;
#if defined(CONFIG_BATTERY_CISD)
		battery->skip_cisd = is_attach;
#endif
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC:
		current_cable_type = SB_CBL_NONE;
		break;

	/* WIRED TYPE */
	case ATTACHED_DEV_TA_MUIC:
		current_cable_type = (is_attach) ? SB_CBL_TA : SB_CBL_NONE;
		break;
	case ATTACHED_DEV_USB_MUIC:
		current_cable_type = (is_attach) ? SB_CBL_USB : SB_CBL_NONE;
		break;
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
		current_cable_type = (is_attach) ? SB_CBL_UARTOFF : SB_CBL_NONE;
		break;

	default:
		pr_err("%s: invalid type for charger:%d\n",
			__func__, muic_dev);
		break;
	}

#ifdef CONFIG_SEC_FACTORY
	if (battery->pdata->support_fgsrc_change) {
		union power_supply_propval val = {0, };

		psy_do_property(battery->pdata->fgsrc_switch_name, get,
			POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, val);

		if (is_attach) {
			if (((muic_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC) ||
				(muic_dev == ATTACHED_DEV_JIG_USB_OFF_MUIC) ||
				(muic_dev == ATTACHED_DEV_JIG_USB_ON_MUIC)) &&
				(!val.intval)) {
				val.intval = 1;
				psy_do_property(battery->pdata->fgsrc_switch_name, set,
					POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, val);
			}
		} else if (val.intval) {
			val.intval = 0;
			psy_do_property(battery->pdata->fgsrc_switch_name, set,
				POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, val);
		}
	}
#endif

	return current_cable_type;
}

static int batt_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	const char *cmd;
	int cable_type = SB_CBL_NONE;
	struct sec_battery_info *battery =
		container_of(nb, struct sec_battery_info, batt_nb);
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;

	mutex_lock(&battery->batt_handlelock);
	switch (action) {
	case MUIC_NOTIFY_CMD_DETACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
		cmd = "DETACH";
		cable_type = sec_bat_cable_check(battery, false, attached_dev);
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		break;
	case MUIC_NOTIFY_CMD_ATTACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
		cmd = "ATTACH";
		cable_type = sec_bat_cable_check(battery, true, attached_dev);
		battery->muic_cable_type = attached_dev;
		break;
	default:
		cmd = "ERROR";
		cable_type = -1;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		break;
	}

	if ((cable_type == SB_CBL_UNKNOWN) &&
		(battery->status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		battery->cable_type = cable_type;
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_info(battery->dev,
			"%s: UNKNOWN cable plugin\n", __func__);
		mutex_unlock(&battery->batt_handlelock);
		return 0;
	}

	sec_bat_set_cable_type(battery, cable_type);
	mutex_unlock(&battery->batt_handlelock);

	if (!strcmp(cmd, "DETACH")) {
		if (is_wireless_type(battery->cable_type) &&
			!is_wireless_type(cable_type)) {
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_WIRELESS_COUNT]++;
			battery->cisd.data[CISD_DATA_WIRELESS_COUNT_PER_DAY]++;
#endif
		}
	}

	pr_info("%s: CMD=%s, attached_dev=%d cable_type(%d %d)\n",
		__func__, cmd, attached_dev, battery->cable_type, cable_type);

	return 0;
}
#endif /* CONFIG_MUIC_NOTIFIER */

#ifdef CONFIG_OF
static int sec_bat_parse_chg_current_dt(struct device_node *np,
		sec_charging_current_t *chg_cur, sec_charging_current_t *def_chg_cur)
{
	struct bat_temp_limit *btl = NULL;
	struct sb_step *st = NULL;
	struct device_node *child;
	unsigned int i, icl, fcc, temp, ttf_fcc, flags;
	int len = 0;

	if (np == NULL || chg_cur == NULL || def_chg_cur == NULL)
		return -EINVAL;

	len = of_property_count_u32_elems(np, "cable_number");
	if (len <= 0)
		return -ENODEV;

	if (of_property_read_u32(np, "input_current", &icl))
		icl = def_chg_cur->input_current_limit;

	if (of_property_read_u32(np, "charging_current", &fcc))
		fcc = def_chg_cur->fast_charging_current;

	if (ttf_parse_fcc(np, &ttf_fcc))
		ttf_fcc = def_chg_cur->ttf_fcc;

	if (of_property_read_u32(np, "flags", &flags))
		flags = def_chg_cur->flag;

	for_each_child_of_node(np, child) {
		if (IS_ERR_OR_NULL(btl))
			btl = sec_bat_parse_bat_temp_limit(child);

		if (IS_ERR_OR_NULL(st))
			st = sb_step_create(child);
	}

	if (IS_ERR_OR_NULL(btl)) {
		pr_info("%s: failed to parse bat_temp_limit(%ld)\n",
			__func__, PTR_ERR(btl));
		btl = NULL;
	}

	if (IS_ERR_OR_NULL(st)) {
		pr_info("%s: failed to parse sb_step(%ld)\n",
			__func__, PTR_ERR(st));
		st = NULL;
	}

	for (i = 0; i <= len; i++) {
		if (of_property_read_u32_index(np, "cable_number", i, &temp))
			continue;

		chg_cur[temp].input_current_limit = icl;
		chg_cur[temp].fast_charging_current = fcc;
		chg_cur[temp].ttf_fcc = ttf_fcc;
		chg_cur[temp].flag = flags;
		chg_cur[temp].btl = btl;
		chg_cur[temp].st = st;
		pr_info("%s: [%d] set flags(0x%x), ext chg(bat_temp_limit = %s, sb_step = %s)\n",
			__func__, temp, flags, GET_BOOL_STR(btl != NULL), GET_BOOL_STR(st != NULL));
	}

	return 0;
}

static int sec_bat_parse_dt(struct device *dev,
		struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0, len = 0;
	unsigned int i = 0;
	const u32 *p;
	u32 temp = 0;

	np = of_find_node_by_name(NULL, "cable-info");
	if (!np) {
		pr_err ("%s : np NULL\n", __func__);
	} else {
		struct device_node *child;
		u32 input_current = 0, charging_current = 0, ttf_fcc = 0, flags = 0;

		pdata->charging_current =
			kcalloc(SB_CBL_MAX, sizeof(sec_charging_current_t), GFP_KERNEL);
		if (!pdata->charging_current) {
			pr_info("%s: failed to alloc charging_current table!!!\n", __func__);
			return -ENOMEM;
		}

		ret = of_property_read_u32(np, "default_input_current", &input_current);
		ret = of_property_read_u32(np, "default_charging_current", &charging_current);
		ret = ttf_parse_fcc(np, &ttf_fcc);
		ret = of_property_read_u32(np, "flags", &flags);

		ret = of_property_read_u32(np, "full_check_current_1st", &pdata->full_check_current_1st);
		if (ret) {
			pr_info("full_check_current_1st is empty\n");
			pdata->full_check_current_1st = 60;
		}

		ret = of_property_read_u32(np, "full_check_current_2nd", &pdata->full_check_current_2nd);
		if (ret) {
			pr_info("full_check_current_2nd is empty\n");
			pdata->full_check_current_2nd = 30;
		}

		for (i = 0; i < SB_CBL_MAX; i++) {
			pdata->charging_current[i].input_current_limit = input_current;
			pdata->charging_current[i].fast_charging_current = charging_current;
			pdata->charging_current[i].ttf_fcc = ttf_fcc;
			pdata->charging_current[i].flag = flags;
		}

		for_each_child_of_node(np, child) {
			ret = sec_bat_parse_chg_current_dt(child,
				pdata->charging_current, get_def_chg_cur_ptr(battery));
			if (ret != 0)
				pr_err("%s: failed to parse %s, ret = %d\n",
					__func__, child->name, ret);
		}
	}
	for (i = 0; i < SB_CBL_MAX; i++) {
		pr_info("%s : CABLE_NUM(%d) INPUT(%d) CHARGING(%d)\n",
			__func__, i,
			pdata->charging_current[i].input_current_limit,
			pdata->charging_current[i].fast_charging_current);
	}

	pr_info("%s : TOPOFF_1ST(%d), TOPOFF_2ND(%d)\n",
		__func__, pdata->full_check_current_1st, pdata->full_check_current_2nd);

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return 1;
	}

	ret = of_property_read_u32(np,
			"battery,expired_time", &temp);
	if (ret) {
		pr_info("expired time is empty\n");
		pdata->expired_time = 3 * 60 * 60;
	} else {
		pdata->expired_time = (unsigned int) temp;
	}
	pdata->expired_time *= 1000;
	battery->expired_time = pdata->expired_time;

	ret = of_property_read_u32(np,
			"battery,recharging_expired_time", &temp);
	if (ret) {
		pr_info("expired time is empty\n");
		pdata->recharging_expired_time = 90 * 60;
	} else {
		pdata->recharging_expired_time = (unsigned int) temp;
	}
	pdata->recharging_expired_time *= 1000;

	ret = of_property_read_u32(np,
			"battery,standard_curr", &pdata->standard_curr);
	if (ret) {
		pr_info("standard_curr is empty\n");
		pdata->standard_curr = 2150;
	}

	ret = of_property_read_string(np,
		"battery,vendor", (char const **)&pdata->vendor);
	if (ret)
		pr_info("%s: Vendor is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,charger_name", (char const **)&pdata->charger_name);
	if (ret)
		pr_info("%s: Charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,fuelgauge_name", (char const **)&pdata->fuelgauge_name);
	if (ret)
		pr_info("%s: Fuelgauge name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
	if (ret)
		pr_info("%s: Wireless charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,fgsrc_switch_name", (char const **)&pdata->fgsrc_switch_name);
	pdata->support_fgsrc_change = !(ret);
	pr_info("%s: fgsrc_switch_name is %s\n", __func__,
		pdata->support_fgsrc_change ? "True" : "False");

	ret = of_property_read_string(np,
		"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
	if (ret)
		pr_info("%s: Wireless charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,chip_vendor", (char const **)&pdata->chip_vendor);
	if (ret)
		pr_info("%s: Chip vendor is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,technology",
		&pdata->technology);
	if (ret)
		pr_info("%s : technology is Empty\n", __func__);

	pdata->fake_capacity = of_property_read_bool(np, "battery,fake_capacity");

	pdata->single_charger_path = of_property_read_bool(np,
							"battery,single_charger_path");

	p = of_get_property(np, "battery,polling_time", &len);
	if (!p)
		return 1;

	len = len / sizeof(u32);
	pdata->polling_time = kzalloc(sizeof(*pdata->polling_time) * len, GFP_KERNEL);
	ret = of_property_read_u32_array(np, "battery,polling_time",
					pdata->polling_time, len);
	if (ret)
		pr_info("%s : battery,polling_time is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,cable_check_type",
		&pdata->cable_check_type);
	if (ret)
		pr_info("%s : Cable check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,cable_source_type",
		&pdata->cable_source_type);
	if (ret)
		pr_info("%s: Cable_source_type is Empty\n", __func__);
	ret = of_property_read_u32(np, "battery,polling_type",
		&pdata->polling_type);
	if (ret)
		pr_info("%s : Polling type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,monitor_initial_count",
		&pdata->monitor_initial_count);
	if (ret)
		pr_info("%s : Monitor initial count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,battery_check_type",
		&pdata->battery_check_type);
	if (ret)
		pr_info("%s : Battery check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,check_count",
		&pdata->check_count);
	if (ret)
		pr_info("%s : Check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,ovp_uvlo_check_type",
		&pdata->ovp_uvlo_check_type);
	if (ret)
		pr_info("%s : Ovp Uvlo check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_check_type",
		&pdata->temp_check_type);
	if (ret)
		pr_info("%s : Temp check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_check_count",
		&pdata->temp_check_count);
	if (ret)
		pr_info("%s : Temp check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_check_type",
		&pdata->full_check_type);
	if (ret)
		pr_info("%s : Full check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_check_type_2nd",
		&pdata->full_check_type_2nd);
	if (ret)
		pr_info("%s : Full check type 2nd is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_check_count",
		&pdata->full_check_count);
	if (ret)
		pr_info("%s : Full check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,chg_gpio_full_check",
		&pdata->chg_gpio_full_check);
	if (ret)
		pr_info("%s : Chg gpio full check is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,chg_polarity_full_check",
		&pdata->chg_polarity_full_check);
	if (ret)
		pr_info("%s : Chg polarity full check is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_type",
		&pdata->full_condition_type);
	if (ret)
		pr_info("%s : Full condition type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_soc",
		&pdata->full_condition_soc);
	if (ret)
		pr_info("%s : Full condition soc is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_vcell",
		&pdata->full_condition_vcell);
	if (ret)
		pr_info("%s : Full condition vcell is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_check_count",
		&pdata->recharge_check_count);
	if (ret)
		pr_info("%s : Recharge check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_type",
		&pdata->recharge_condition_type);
	if (ret)
		pr_info("%s : Recharge condition type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_soc",
		&pdata->recharge_condition_soc);
	if (ret)
		pr_info("%s : Recharge condition soc is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_vcell",
		&pdata->recharge_condition_vcell);
	if (ret)
		pr_info("%s : Recharge condition vcell is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,ux_full_inow",
		&pdata->ux_full_inow);
	if (ret)
		pr_info("%s : ux_full_inow is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,chg_float_voltage",
		(unsigned int *)&pdata->chg_float_voltage);
	if (ret) {
		pr_info("%s: chg_float_voltage is Empty\n", __func__);
		pdata->chg_float_voltage = 4350;
	}

	ret = of_property_read_u32(np, "battery,boot_charge_current",
			&pdata->boot_charge_current);
	if (ret) {
		pr_info("%s: boot_charge_current is Empty\n", __func__);
		pdata->boot_charge_current = 250;
	}

	ret = of_property_read_u32(np, "battery,boot_mode_time",
			&pdata->boot_mode_time);
	if (ret) {
		pr_info("%s: boot_mode_time is Empty\n", __func__);
		pdata->boot_mode_time = 55000; /* default 1min (margin = 5sec) */
	}

	ret = of_property_read_u32(np, "battery,tct_low_current",
			&pdata->tct_low_current);
	if (ret) {
		pr_info("%s: tct_low_current is Empty\n", __func__);
		pdata->tct_low_current = 60;
	}

	ret = of_property_read_u32(np, "battery,wire_warm_overheat_thresh", &temp);
	pdata->wire_warm_overheat_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_warm_overheat_thresh is Empty\n", __func__);
		pdata->wire_warm_overheat_thresh = 450;
	}

	ret = of_property_read_u32(np, "battery,wire_normal_warm_thresh", &temp);
	pdata->wire_normal_warm_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_normal_warm_thresh is Empty\n", __func__);
		pdata->wire_normal_warm_thresh = 420;
	}

	ret = of_property_read_u32(np, "battery,wire_cool1_normal_thresh", &temp);
	pdata->wire_cool1_normal_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool1_normal_thresh is Empty\n", __func__);
		pdata->wire_cool1_normal_thresh = 150;
	}

	ret = of_property_read_u32(np, "battery,wire_cool2_cool1_thresh", &temp);
	pdata->wire_cool2_cool1_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool2_cool1_thresh is Empty\n", __func__);
		pdata->wire_cool2_cool1_thresh = 150;
	}

	ret = of_property_read_u32(np, "battery,wire_cool3_cool2_thresh", &temp);
	pdata->wire_cool3_cool2_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool3_cool2_thresh is Empty\n", __func__);
		pdata->wire_cool3_cool2_thresh = 50;
	}

	ret = of_property_read_u32(np, "battery,wire_cold_cool3_thresh", &temp);
	pdata->wire_cold_cool3_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cold_cool3_thresh is Empty\n", __func__);
		pdata->wire_cold_cool3_thresh = 0;
	}

	ret = of_property_read_u32(np, "battery,wireless_warm_overheat_thresh", &temp);
	pdata->wireless_warm_overheat_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_warm_overheat_thresh is Empty\n", __func__);
		pdata->wireless_warm_overheat_thresh = 450;
	}

	ret = of_property_read_u32(np, "battery,wireless_normal_warm_thresh", &temp);
	pdata->wireless_normal_warm_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_normal_warm_thresh is Empty\n", __func__);
		pdata->wireless_normal_warm_thresh = 420;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool1_normal_thresh", &temp);
	pdata->wireless_cool1_normal_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool1_normal_thresh is Empty\n", __func__);
		pdata->wireless_cool1_normal_thresh = 150;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool2_cool1_thresh", &temp);
	pdata->wireless_cool2_cool1_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool2_cool1_thresh is Empty\n", __func__);
		pdata->wireless_cool2_cool1_thresh = 150;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool3_cool2_thresh", &temp);
	pdata->wireless_cool3_cool2_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool3_cool2_thresh is Empty\n", __func__);
		pdata->wireless_cool3_cool2_thresh = 50;
	}

	ret = of_property_read_u32(np, "battery,wireless_cold_cool3_thresh", &temp);
	pdata->wireless_cold_cool3_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cold_cool3_thresh is Empty\n", __func__);
		pdata->wireless_cold_cool3_thresh = 0;
	}

	ret = of_property_read_u32(np, "battery,warm_recov_thresh", &temp);
	pdata->warm_recov_thresh = (int)temp;
	if (ret) {
		pr_info("%s : warm_recov_thresh is Empty\n", __func__);
		pdata->warm_recov_thresh = 401;
	}

	ret = of_property_read_u32(np, "battery,swelling_high_rechg_voltage",
		(unsigned int *)&pdata->swelling_high_rechg_voltage);
	if (ret) {
		pr_info("%s: swelling_high_rechg_voltage is Empty\n", __func__);
		pdata->swelling_high_rechg_voltage = 4120;
	}

	ret = of_property_read_u32(np, "battery,swelling_low_rechg_voltage",
		(unsigned int *)&pdata->swelling_low_rechg_voltage);
	if (ret) {
		pr_info("%s: swelling_low_rechg_voltage is Empty\n", __func__);
				pdata->swelling_low_rechg_voltage = 4000;
	}

	ret = of_property_read_u32(np, "battery,wire_warm_current", &temp);
	pdata->wire_warm_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_warm_current is Empty\n", __func__);
		pdata->wire_warm_current = 169;
	}

	ret = of_property_read_u32(np, "battery,wire_cool1_current", &temp);
	pdata->wire_cool1_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool1_current is Empty\n", __func__);
		pdata->wire_cool1_current = 240;
	}

	ret = of_property_read_u32(np, "battery,wire_cool2_current", &temp);
	pdata->wire_cool2_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool2_current is Empty\n", __func__);
		pdata->wire_cool2_current = 56;
	}

	ret = of_property_read_u32(np, "battery,wire_cool3_current", &temp);
	pdata->wire_cool3_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool3_current is Empty\n", __func__);
		pdata->wire_cool3_current = 40;
	}

	ret = of_property_read_u32(np, "battery,wireless_warm_current", &temp);
	pdata->wireless_warm_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_warm_current is Empty\n", __func__);
		pdata->wireless_warm_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool1_current", &temp);
	pdata->wireless_cool1_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool1_current is Empty\n", __func__);
		pdata->wireless_cool1_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool2_current", &temp);
	pdata->wireless_cool2_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool2_current is Empty\n", __func__);
		pdata->wireless_cool2_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool3_current", &temp);
	pdata->wireless_cool3_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool3_current is Empty\n", __func__);
		pdata->wireless_cool3_current = 500;
	}

	ret = of_property_read_u32(np, "battery,high_temp_topoff", &temp);
	pdata->high_temp_topoff = (int)temp;
	if (ret) {
		pr_info("%s : high_temp_topoff is Empty\n", __func__);
		pdata->high_temp_topoff = 25;
	}

	ret = of_property_read_u32(np, "battery,low_temp_topoff", &temp);
	pdata->low_temp_topoff = (int)temp;
	if (ret) {
		pr_info("%s : low_temp_topoff is Empty\n", __func__);
		pdata->low_temp_topoff = 25;
	}

	ret = of_property_read_u32(np, "battery,high_temp_float", &temp);
	pdata->high_temp_float = (int)temp;
	if (ret) {
		pr_info("%s : high_temp_float is Empty\n", __func__);
		pdata->high_temp_float = 4270;
	}

	ret = of_property_read_u32(np, "battery,low_temp_float", &temp);
	pdata->low_temp_float = (int)temp;
	if (ret) {
		pr_info("%s : low_temp_float is Empty\n", __func__);
		pdata->low_temp_float = 4420;
	}

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	p = of_get_property(np, "battery,age_data", &len);
	if (p) {
		battery->pdata->num_age_step = len / sizeof(sec_age_data_t);
		battery->pdata->age_data = kzalloc(len, GFP_KERNEL);

		if (battery->pdata->age_data) {
			ret = of_property_read_u32_array(np, "battery,age_data",
					(u32 *)battery->pdata->age_data, len/sizeof(u32));
			if (ret) {
				pr_err("%s failed to read battery->pdata->age_data: %d\n",
						__func__, ret);
				kfree(battery->pdata->age_data);
				battery->pdata->age_data = NULL;
				battery->pdata->num_age_step = 0;
			}
			pr_err("%s num_age_step : %d\n", __func__, battery->pdata->num_age_step);
			for (len = 0; len < battery->pdata->num_age_step; ++len) {
				pr_info("[%d/%d]cycle:%d, float:%d, full_v:%d, recharge_v:%d, soc:%d\n",
					len, battery->pdata->num_age_step-1,
					battery->pdata->age_data[len].cycle,
					battery->pdata->age_data[len].float_voltage,
					battery->pdata->age_data[len].full_condition_vcell,
					battery->pdata->age_data[len].recharge_condition_vcell,
					battery->pdata->age_data[len].full_condition_soc);
			}
		}
	} else {
		battery->pdata->num_age_step = 0;
		pr_err("%s there is not age_data\n", __func__);
	}

	p = of_get_property(np, "battery,age_data_cc_cv_threshold", &len);
	if (p) {
		battery->pdata->num_age_cc_cv_threshold = len / sizeof(int);
		battery->pdata->age_cc_cv_threshold = kzalloc(len, GFP_KERNEL);

		if (battery->pdata->age_cc_cv_threshold) {
			ret = of_property_read_u32_array(np, "battery,age_data_cc_cv_threshold",
				(u32 *)battery->pdata->age_cc_cv_threshold, len/sizeof(u32));

			if (ret) {
				pr_err("%s failed to read battery->pdata->age_data: %d\n",
						__func__, ret);
				kfree(battery->pdata->age_cc_cv_threshold);
				battery->pdata->age_cc_cv_threshold = NULL;
				battery->pdata->num_age_cc_cv_threshold = 0;
			} else {
				for (len = 0; len < battery->pdata->num_age_step; ++len) {
					pr_info("[%d/%d] age_cc_cv_threshold = %d\n",
						len, battery->pdata->num_age_step-1,
						battery->pdata->age_cc_cv_threshold[len]);
				}
			}
		}
	} else {
		battery->pdata->num_age_cc_cv_threshold = 0;
		pr_err("%s there is not num_age_cc_cv_threshold\n", __func__);
	}
#endif

	pr_info("%s: vendor : %s, technology : %d, cable_check_type : %d\n"
		"cable_source_type : %d, event_waiting_time : %d\n"
		"polling_type : %d, initial_count : %d, check_count : %d, ovp_uvlo_check_type : %d\n"
		"temp_check_type : %d\n",
		__func__,
		pdata->vendor, pdata->technology,pdata->cable_check_type,
		pdata->cable_source_type, pdata->event_waiting_time,
		pdata->polling_type, pdata->monitor_initial_count,
		pdata->check_count, pdata->ovp_uvlo_check_type,
		pdata->temp_check_type);
	return 0;
}

static void sec_bat_parse_lpm_dt(struct sec_battery_info *battery)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	struct device_node *np;
	int ret = 0;

	if (!lpcharge)
		return;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return;
	}

	ret = of_property_read_u32(np, "battery,lpm_boot_mode_time",
			&pdata->boot_mode_time);
	if (ret)
		pr_info("%s: lpm_boot_mode_time is Empty\n", __func__);

	pr_info("%s: boot_mode_time : %d\n",
		__func__, pdata->boot_mode_time);
}

static void sec_bat_parse_mode_dt(struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0, len = 0;
	bool is_recovery_mode = false;
	const u32 *p;

	is_recovery_mode = (sec_bat_get_boot_mode() == 2);
	battery->store_mode |= is_recovery_mode;
	if (!battery->store_mode) {
		sec_vote(battery->fcc_vote, VOTER_STORE_MODE, false, 0);
		sec_vote(battery->chgen_vote, VOTER_STORE_MODE, false, 0);
		sec_votef(VN_WPC_PHM, VOTER_STORE_MODE, false, WPC_PHM_OFF);
		sec_votef(VN_WPC_APM, VOTER_STORE_MODE, false, WPC_APM_NONE);
		return;
	}

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		return;
	}

	if (is_recovery_mode) {
		pdata->store_wpc_current = 150;
		pdata->store_mode_chg_max = 15;
		pdata->store_mode_chg_min = 10;
	} else {
		ret = of_property_read_u32(np, "battery,store_wpc_current",
			&pdata->store_wpc_current);
		if (ret) {
			pr_info("%s: store_wpc_current is Empty\n", __func__);
			pdata->store_wpc_current = 75;
		}

		pdata->store_mode_chg_max = STORE_MODE_CHARGING_MAX;
		pdata->store_mode_chg_min = STORE_MODE_CHARGING_MIN;
	}

	pr_info("%s: fcc(%d), limit_cap(%d, %d), warm(%d, %d)\n",
		__func__,
		pdata->store_wpc_current,
		pdata->store_mode_chg_max, pdata->store_mode_chg_min,
		pdata->wireless_normal_warm_thresh, pdata->warm_recov_thresh);

	sec_vote(battery->fcc_vote, VOTER_STORE_MODE, true, pdata->store_wpc_current);

	p = of_get_property(np, "battery,store_mode_polling_time", &len);
	if (!p)
		return;

	len = len / sizeof(u32);
	pdata->polling_time = kzalloc(sizeof(*pdata->polling_time) * len, GFP_KERNEL);
	ret = of_property_read_u32_array(np, "battery,store_mode_polling_time",
					pdata->polling_time, len);
	if (ret)
		pr_info("%s: battery,polling_time is Empty\n", __func__);
}

static void sec_bat_parse_mode_dt_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, parse_mode_dt_work.work);

	sec_bat_parse_mode_dt(battery);

	sec_bat_set_charging_current(battery);

	__pm_relax(battery->parse_mode_dt_ws);
}

#if defined(CONFIG_SEC_FACTORY)
static int sec_bat_parse_dt_for_factory(struct sec_battery_info *battery)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	sec_charging_current_t *def_chg_cur;
	struct device_node *np;
	unsigned int i, icl = 0, fcc = 0;
	int ret = 0, temp = 0;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		return -ENODEV;
	}

	def_chg_cur = get_def_chg_cur_ptr(battery);
	ret = of_property_read_u32(np, "battery,factory_icl", &icl);
	if (ret) {
		icl = def_chg_cur->input_current_limit;
		pr_info("%s: factory_icl is Empty, set default current(%d)\n",
			__func__, icl);
	}

	ret = of_property_read_u32(np, "battery,factory_fcc", &fcc);
	if (ret) {
		fcc = def_chg_cur->fast_charging_current;
		pr_info("%s: factory_fcc is Empty, set default current(%d)\n",
			__func__, fcc);
	}

	for (i = 0; i < SB_CBL_MAX; i++) {
		pdata->charging_current[i].input_current_limit = icl;
		pdata->charging_current[i].fast_charging_current = fcc;
	}

	ret = of_property_read_u32(np, "battery,factory_wire_normal_warm_thresh",
		(unsigned int *)&temp);
	if (ret) {
		temp = pdata->wire_warm_overheat_thresh - 10;
		pr_info("%s: factory_wire_normal_warm_thresh is Empty, set default value(%d)\n",
			__func__, temp);
	}
	pdata->wire_normal_warm_thresh = temp;
	pdata->wireless_normal_warm_thresh = temp;

	ret = of_property_read_u32(np, "battery,factory_boot_mode_time",
			&pdata->boot_mode_time);
	if (ret) {
		pr_info("%s: factory_boot_mode_time is Empty\n", __func__);
		pdata->boot_mode_time = 55000; /* default 1min (margin = 5sec) */
	}

	return 0;
}

static int sec_bat_parse_dt_for_prevent_overheat(struct sec_battery_info *battery)
{
	return 0;
}
#else
static int sec_bat_parse_dt_for_factory(struct sec_battery_info *battery)
{
	return 0;
}

static int sec_bat_parse_dt_for_prevent_overheat(struct sec_battery_info *battery)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	struct device_node *np;
	int ret = 0, temp = 0;
	bool is_activated = false;

	if (lpcharge) {
		pr_info("%s: keep state during lpm\n", __func__);
		return 0;
	}

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "battery,wireless_prevent_overheat_thresh",
		(unsigned int *)&temp);
	if (!ret) {
		pr_info("%s: update wireless_warm_overheat_thresh(%d --> %d)\n",
			__func__, pdata->wireless_warm_overheat_thresh, temp);
		is_activated = true;
		pdata->wireless_warm_overheat_thresh = temp;
	}

	if (is_activated) {
		__pm_stay_awake(battery->prevent_overheat_ws);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->prevent_overheat_work, msecs_to_jiffies(180000)); /* 3mins */
	}

	return 0;
}
#endif
#endif

static void sec_bat_lpcharge_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, lpcharge_work.work);

	pr_info("%s\n", __func__);
	battery->lpcharge_boost = 0;
	sec_vote(battery->fcc_vote, VOTER_BOOT_MODE, false, 0);
	sec_bat_set_threshold(battery);

	sec_bat_set_charging_current(battery);

	if (is_wireless_type(battery->cable_type)) {
		int vout, vrect, i;
		int reset_wireless_chg = 0;
		union power_supply_propval value = {0, };

		if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)) {
			/* WA code. Boot and put it on the charging pad 2 seconds later
			 * Vout can be abnormal vaule
			 */
			for (i = 0; i < 3; i++) {
				psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_PROP_ENERGY_NOW, value);
				vout = value.intval;

				psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_PROP_ENERGY_AVG, value);
				vrect = value.intval;

				if (vout >= 2000) {
					reset_wireless_chg = 0;
					break;
				} else {
					if (vrect >= 3500)
						reset_wireless_chg = 1;
					else
						reset_wireless_chg = 0;
				}
				pr_info("%s vout(%d) vrect(%d) reset_wireless_chg(%d)\n",
					__func__, vout, vrect, reset_wireless_chg);
				msleep(100);
			}

			/* if vout is abnormal, reset charging using PHM */
			if (reset_wireless_chg) {
				pr_info("%s vout off!!\n", __func__);
				sec_votef(VN_WPC_PHM, VOTER_BOOT_MODE, true, WPC_PHM_ON);

				pr_info("%s vout on!!\n", __func__);
				sec_votef(VN_WPC_PHM, VOTER_BOOT_MODE, true, WPC_PHM_OFF);

				sec_vote_refresh(battery->chgen_vote);

				sec_vote(battery->fv_vote, VOTER_AGING_STEP, true,
					battery->pdata->chg_float_voltage);

				battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
				battery->expired_time = battery->pdata->expired_time;
				battery->prev_safety_time = 0;
			}
		}
	}
	sec_votef(VN_WPC_PHM, VOTER_BOOT_MODE, false, WPC_PHM_OFF);
	sec_votef(VN_WPC_APM, VOTER_BOOT_MODE, false, WPC_APM_NONE);
	sb_notify_call_data(SB_NOTIFY_BAT_BOOT_MODE, false);

	__pm_relax(battery->lpcharge_ws);
}

static void sec_bat_prevent_overheat_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, prevent_overheat_work.work);
	sec_battery_platform_data_t *pdata = battery->pdata;
	struct device_node *np;
	int ret = 0, temp = 0;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		__pm_relax(battery->prevent_overheat_ws);
		return;
	}

	ret = of_property_read_u32(np, "battery,wireless_warm_overheat_thresh",
		(unsigned int *)&temp);
	if (!ret) {
		pr_info("%s: recover wireless_warm_overheat_thresh(%d --> %d)\n",
			__func__, pdata->wireless_warm_overheat_thresh, temp);
		pdata->wireless_warm_overheat_thresh = temp;
	} else {
		pdata->wireless_warm_overheat_thresh = 450;
	}
	sec_bat_set_threshold(battery);

	__pm_relax(battery->prevent_overheat_ws);
}

#ifdef CONFIG_OF
extern sec_battery_platform_data_t sec_battery_pdata;
#endif

static const struct power_supply_desc battery_power_supply_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = sec_battery_props,
	.num_properties = ARRAY_SIZE(sec_battery_props),
	.get_property = sec_bat_get_property,
	.set_property = sec_bat_set_property,
};

static const struct power_supply_desc usb_power_supply_desc = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = sec_power_props,
	.num_properties = ARRAY_SIZE(sec_power_props),
	.get_property = sec_usb_get_property,
};

static const struct power_supply_desc ac_power_supply_desc = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = sec_ac_props,
	.num_properties = ARRAY_SIZE(sec_ac_props),
	.get_property = sec_ac_get_property,
};

static const struct power_supply_desc wireless_power_supply_desc = {
	.name = "wireless",
	.type = POWER_SUPPLY_TYPE_WIRELESS,
	.properties = sec_wireless_props,
	.num_properties = ARRAY_SIZE(sec_wireless_props),
	.get_property = sec_wireless_get_property,
	.set_property = sec_wireless_set_property,
};

static int sec_battery_probe(struct platform_device *pdev)
{
	sec_battery_platform_data_t *pdata = NULL;
	struct sec_battery_info *battery;
	struct power_supply_config battery_cfg = {};

	int ret = 0;
	int i = 0;

	union power_supply_propval value = {0, };
	union power_supply_propval chg_value = {0, };
	sec_charging_current_t *def_chg_cur;

	dev_info(&pdev->dev,
		"%s: SEC Battery Driver Loading\n", __func__);

	battery = kzalloc(sizeof(*battery), GFP_KERNEL);
	if (!battery)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				sizeof(sec_battery_platform_data_t),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_bat_free;
		}

		battery->pdata = pdata;

		if (sec_bat_parse_dt(&pdev->dev, battery)) {
			dev_err(&pdev->dev,
				"%s: Failed to get battery dt\n", __func__);
			ret = -EINVAL;
			goto err_bat_free;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		battery->pdata = pdata;
	}

	platform_set_drvdata(pdev, battery);

	battery->dev = &pdev->dev;

	mutex_init(&battery->iolock);
	mutex_init(&battery->misclock);
	mutex_init(&battery->batt_handlelock);
	mutex_init(&battery->current_eventlock);

	battery->eng_data = sec_bat_init_eng_data(battery->dev);

	battery->monitor_ws = wakeup_source_register(battery->dev, "sec-battery-monitor");
	battery->cable_ws = wakeup_source_register(battery->dev, "sec-battery-cable");
	battery->vbus_ws = wakeup_source_register(battery->dev, "sec-battery-vbus");
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	battery->batt_data_ws = wakeup_source_register(battery->dev, "sec-battery-update_data");
#endif
	battery->misc_event_ws = wakeup_source_register(battery->dev, "sec-battery-misc_event");
#ifdef CONFIG_OF
	battery->parse_mode_dt_ws = wakeup_source_register(battery->dev, "sec-battery-parse_mode_dt");
#endif
	battery->lpcharge_ws = wakeup_source_register(battery->dev, "sec-battery-lpcharge");
	battery->prevent_overheat_ws = wakeup_source_register(battery->dev, "sb-prevent-overheat");
	battery->fcc_ws = wakeup_source_register(battery->dev, "sb-fcc");

	battery->fcc_vote = sec_vote_init(VN_FCC, SEC_VOTE_MIN, VOTER_MAX,
			130, sec_voter_name, set_charging_current, battery);
	battery->input_vote = sec_vote_init(VN_ICL, SEC_VOTE_MIN, VOTER_MAX,
			130, sec_voter_name, set_input_current, battery);
	battery->fv_vote = sec_vote_init(VN_FV, SEC_VOTE_MIN, VOTER_MAX,
			battery->pdata->chg_float_voltage, sec_voter_name, set_float_voltage, battery);
	battery->chgen_vote = sec_vote_init(VN_CHG_EN, SEC_VOTE_MIN, VOTER_MAX,
			SEC_BAT_CHG_MODE_CHARGING_OFF, sec_voter_name, sec_bat_set_charge, battery);
	battery->topoff_vote = sec_vote_init(VN_TOPOFF, SEC_VOTE_MIN, VOTER_MAX,
			battery->pdata->full_check_current_1st, sec_voter_name, set_topoff_current, battery);

	/* set vote priority */
	change_sec_voter_pri(battery->fcc_vote, VOTER_TEST, VOTE_PRI_10);
	change_sec_voter_pri(battery->input_vote, VOTER_TEST, VOTE_PRI_10);
	change_sec_voter_pri(battery->topoff_vote, VOTER_TEST, VOTE_PRI_10);
	change_sec_voter_pri(battery->fcc_vote, VOTER_BOOT_MODE, VOTE_PRI_9);

	/* initialization of battery info */
	sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_DISCHARGING);
	battery->health = POWER_SUPPLY_HEALTH_GOOD;
	battery->present = true;
	battery->is_jig_on = false;
	battery->wdt_kick_disable = 0;

	battery->polling_count = 1;	/* initial value = 1 */
	battery->polling_time = pdata->polling_time[
		SEC_BATTERY_POLLING_TIME_DISCHARGING];
	battery->polling_in_sleep = false;
	battery->polling_short = false;

	battery->check_count = 0;

	battery->input_current = 0;
	battery->charging_current = 0;
	battery->topoff_current = 0;

	battery->charging_start_time = 0;
	battery->charging_passed_time = 0;
	battery->charging_fullcharged_time = 0;
	battery->wc_enable = true;
	battery->wc_enable_cnt = 0;
	battery->wc_enable_cnt_value = 3;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	battery->stability_test = 0;
	battery->eng_not_full_status = 0;
#endif
	battery->wire_status = SB_CBL_NONE;

	battery->chg_limit = false;

	battery->skip_swelling = false;

	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA, SEC_BAT_CURRENT_EVENT_USB_100MA);

	battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
	battery->is_recharging = false;
	battery->cable_type = SB_CBL_NONE;
	battery->test_mode = 0;
	battery->factory_mode = false;
	battery->store_mode = false;

	battery->safety_timer_set = true;
	battery->stop_timer = false;
	battery->prev_safety_time = 0;
	battery->lcd_status = false;

	battery->thermal_zone = BAT_THERMAL_NORMAL;
	sec_bat_set_threshold(battery);

#if defined(CONFIG_BATTERY_CISD)
	battery->usb_overheat_check = false;
	battery->skip_cisd = false;
#endif

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	battery->batt_cycle = -1;
	battery->pdata->age_step = 0;
#endif

	battery->health_change = false;

	/* Check High Voltage charging option for wireless charging */
	/* '1' means disabling High Voltage charging */
	if (charging_night_mode == '1')
		sleep_mode = true;
	else
		sleep_mode = false;

	/* set temp check cond keystring */
	battery->disable_temp_check = false;
	if (temp_misc_test == '1')
		sec_bat_temp_misc_test(battery, true);

	/* Check High Voltage charging option for wired charging */
	if (get_afc_mode() == CH_MODE_AFC_DISABLE_VAL) {
		pr_info("HV wired charging mode is disabled\n");
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);
	}

	if (battery->pdata->charger_name == NULL)
		battery->pdata->charger_name = "sec-charger";
	if (battery->pdata->fuelgauge_name == NULL)
		battery->pdata->fuelgauge_name = "sec-fuelgauge";

	/* create work queue */
	battery->monitor_wqueue =
		create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!battery->monitor_wqueue) {
		dev_err(battery->dev,
			"%s: Fail to Create Workqueue\n", __func__);
		goto err_irq;
	}

	/* init time to full */
	ttf_init(battery);

	INIT_DELAYED_WORK(&battery->monitor_work, sec_bat_monitor_work);
	INIT_DELAYED_WORK(&battery->cable_work, sec_bat_cable_work);
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	INIT_DELAYED_WORK(&battery->batt_data_work, sec_bat_update_data_work);
#endif
	INIT_DELAYED_WORK(&battery->misc_event_work, sec_bat_misc_event_work);
#ifdef CONFIG_OF
	INIT_DELAYED_WORK(&battery->parse_mode_dt_work, sec_bat_parse_mode_dt_work);
#endif
	INIT_DELAYED_WORK(&battery->lpcharge_work, sec_bat_lpcharge_work);
	INIT_DELAYED_WORK(&battery->prevent_overheat_work, sec_bat_prevent_overheat_work);
	INIT_DELAYED_WORK(&battery->fcc_work, fcc_step_work);

#if defined(CONFIG_STORE_MODE) && !defined(CONFIG_SEC_FACTORY)
	battery->store_mode = true;
#endif
	sec_bat_parse_lpm_dt(battery);
	sec_bat_parse_mode_dt(battery);
	sec_bat_parse_dt_for_factory(battery);
	sec_bat_parse_dt_for_prevent_overheat(battery);

	/* set default fcc vote */
	def_chg_cur = get_def_chg_cur_ptr(battery);
	if (def_chg_cur != NULL)
		sec_vote(battery->fcc_vote, VOTER_CABLE, true, def_chg_cur->fast_charging_current);
	else
		sec_vote(battery->fcc_vote, VOTER_CABLE, true, 130);

	__pm_stay_awake(battery->lpcharge_ws);
	battery->lpcharge_boost = 1;
	sec_vote(battery->fcc_vote, VOTER_BOOT_MODE, true, battery->pdata->boot_charge_current);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->lpcharge_work, msecs_to_jiffies(battery->pdata->boot_mode_time));
	sec_votef(VN_WPC_APM, VOTER_BOOT_MODE, true, WPC_APM_BOOT);
	sec_votef(VN_WPC_PHM, VOTER_BOOT_MODE, true, WPC_PHM_OFF);

	switch (pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		INIT_DELAYED_WORK(&battery->polling_work,
			sec_bat_polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		battery->last_poll_time = ktime_get_boottime();
		alarm_init(&battery->polling_alarm, ALARM_BOOTTIME,
			sec_bat_alarm);
		break;
	default:
		break;
	}

#if defined(CONFIG_BATTERY_CISD)
	sec_battery_cisd_init(battery);
#endif

	battery_cfg.drv_data = battery;

	/* init power supplier framework */
	battery->psy_wireless = power_supply_register(&pdev->dev, &wireless_power_supply_desc, &battery_cfg);
	if (!battery->psy_wireless) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_wireless\n", __func__);
		goto err_workqueue;
	}
	battery->psy_wireless->supplied_to = supply_list;
	battery->psy_wireless->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_usb = power_supply_register(&pdev->dev, &usb_power_supply_desc, &battery_cfg);
	if (!battery->psy_usb) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_usb\n", __func__);
		goto err_supply_unreg_wireless;
	}
	battery->psy_usb->supplied_to = supply_list;
	battery->psy_usb->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_ac = power_supply_register(&pdev->dev, &ac_power_supply_desc, &battery_cfg);
	if (!battery->psy_ac) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_ac\n", __func__);
		goto err_supply_unreg_usb;
	}
	battery->psy_ac->supplied_to = supply_list;
	battery->psy_ac->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_bat = power_supply_register(&pdev->dev, &battery_power_supply_desc, &battery_cfg);
	if (!battery->psy_bat) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_bat\n", __func__);
		goto err_supply_unreg_ac;
	}

	ret = sec_bat_create_attrs(&battery->psy_bat->dev);
	if (ret) {
		dev_err(battery->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_req_irq;
	}

	/* initialize battery level*/
	value.intval = 0;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
	battery->capacity = value.intval;

	sb_notify_register(&battery->sb_nb,
		sb_noti_handler, "battery", SB_DEV_BATTERY);
	sb_notify_call_data(SB_NOTIFY_BAT_BOOT_MODE, true);

#if defined(CONFIG_MUIC_NOTIFIER)
	muic_notifier_register(&battery->batt_nb,
	batt_handle_notification, MUIC_NOTIFY_DEV_CHARGER);
#endif

	value.intval = true;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX, value);

	/* to sync muic cable check */
	for (i = 0; i < 30; i++) {
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_ONLINE, value);
		if (value.intval != SB_CBL_NONE) {
			sec_bat_set_cable_type(battery, value.intval);
			break;
		}
		psy_do_property(pdata->charger_name, get,
			POWER_SUPPLY_PROP_STATUS, chg_value);
		if ((battery->cable_type != SB_CBL_NONE) ||
			(chg_value.intval != POWER_SUPPLY_STATUS_UNKNOWN) ||
			(chg_value.intval != POWER_SUPPLY_STATUS_DISCHARGING))
			break;
		msleep(100);
	}

	if (battery->cable_type == SB_CBL_NONE) {
		dev_info(&pdev->dev,
				"%s: SEC Battery Driver Monitorwork\n", __func__);
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	}

	if (battery->pdata->check_battery_callback)
		battery->present = battery->pdata->check_battery_callback();

	sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, true, battery->pdata->full_check_current_1st);
	sec_vote(battery->fv_vote, VOTER_FULL_CHARGE, true, battery->pdata->chg_float_voltage);

	dev_info(battery->dev,
		"%s: SEC Battery Driver Loaded\n", __func__);
	return 0;

err_req_irq:
	if (battery->pdata->bat_irq)
		free_irq(battery->pdata->bat_irq, battery);
	power_supply_unregister(battery->psy_bat);
err_supply_unreg_ac:
	power_supply_unregister(battery->psy_ac);
err_supply_unreg_usb:
	power_supply_unregister(battery->psy_usb);
err_supply_unreg_wireless:
	power_supply_unregister(battery->psy_wireless);
err_workqueue:
	destroy_workqueue(battery->monitor_wqueue);
err_irq:
	wakeup_source_unregister(battery->monitor_ws);
	wakeup_source_unregister(battery->cable_ws);
	wakeup_source_unregister(battery->vbus_ws);
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wakeup_source_unregister(battery->batt_data_ws);
#endif
	wakeup_source_unregister(battery->misc_event_ws);
#ifdef CONFIG_OF
	wakeup_source_unregister(battery->parse_mode_dt_ws);
#endif
	wakeup_source_unregister(battery->lpcharge_ws);
	wakeup_source_unregister(battery->prevent_overheat_ws);
	wakeup_source_unregister(battery->fcc_ws);
	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
	mutex_destroy(&battery->batt_handlelock);
	mutex_destroy(&battery->current_eventlock);
	kfree(pdata);
err_bat_free:
	kfree(battery);

	return ret;
}

static int sec_battery_remove(struct platform_device *pdev)
{
	struct sec_battery_info *battery = platform_get_drvdata(pdev);
#ifndef CONFIG_OF
	int i;
#endif

	pr_info("%s: ++\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	flush_workqueue(battery->monitor_wqueue);
	destroy_workqueue(battery->monitor_wqueue);
	wakeup_source_unregister(battery->monitor_ws);
	wakeup_source_unregister(battery->cable_ws);
	wakeup_source_unregister(battery->vbus_ws);
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wakeup_source_unregister(battery->batt_data_ws);
#endif
	wakeup_source_unregister(battery->misc_event_ws);
#ifdef CONFIG_OF
	wakeup_source_unregister(battery->parse_mode_dt_ws);
#endif
	wakeup_source_unregister(battery->lpcharge_ws);
	wakeup_source_unregister(battery->prevent_overheat_ws);
	wakeup_source_unregister(battery->fcc_ws);

	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
	mutex_destroy(&battery->batt_handlelock);
	mutex_destroy(&battery->current_eventlock);

	sb_notify_unregister(&battery->sb_nb);

	power_supply_unregister(battery->psy_wireless);
	power_supply_unregister(battery->psy_ac);
	power_supply_unregister(battery->psy_usb);
	power_supply_unregister(battery->psy_bat);

	kfree(battery);

	pr_info("%s: --\n", __func__);

	return 0;
}

static int sec_battery_prepare(struct device *dev)
{
	struct sec_battery_info *battery
		= dev_get_drvdata(dev);

	dev_info(battery->dev, "%s: Start\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	/* monitor_ws should be unlocked before cancle monitor_work */
	__pm_relax(battery->monitor_ws);
	cancel_delayed_work_sync(&battery->monitor_work);

	battery->polling_in_sleep = true;
	sec_bat_set_polling(battery);

	/* cancel work for polling
	 * that is set in sec_bat_set_polling()
	 * no need for polling in sleep
	 */
	if (battery->pdata->polling_type ==
		SEC_BATTERY_MONITOR_WORKQUEUE)
		cancel_delayed_work(&battery->polling_work);

	dev_info(battery->dev, "%s: End\n", __func__);

	return 0;
}

static int sec_battery_suspend(struct device *dev)
{
	return 0;
}

static int sec_battery_resume(struct device *dev)
{
	return 0;
}

static void sec_battery_complete(struct device *dev)
{
	struct sec_battery_info *battery
		= dev_get_drvdata(dev);

	dev_dbg(battery->dev, "%s: Start\n", __func__);

	/* cancel current alarm and reset after monitor work */
	if (battery->pdata->polling_type == SEC_BATTERY_MONITOR_ALARM)
		alarm_cancel(&battery->polling_alarm);

	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->monitor_work, 0);

	dev_dbg(battery->dev, "%s: End\n", __func__);

	return;
}

static void sec_battery_shutdown(struct platform_device *pdev)
{
	struct sec_battery_info *battery
		= platform_get_drvdata(pdev);

	pr_info("%s: ++\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	pr_info("%s: --\n", __func__);
}

#ifdef CONFIG_OF
static struct of_device_id sec_battery_dt_ids[] = {
	{ .compatible = "samsung,sec-battery" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_battery_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_battery_pm_ops = {
	.prepare = sec_battery_prepare,
	.suspend = sec_battery_suspend,
	.resume = sec_battery_resume,
	.complete = sec_battery_complete,
};

static struct platform_driver sec_battery_driver = {
	.driver = {
		.name = "sec-battery",
		.owner = THIS_MODULE,
		.pm = &sec_battery_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = sec_battery_dt_ids,
#endif
	},
	.probe = sec_battery_probe,
	.remove = sec_battery_remove,
	.shutdown = sec_battery_shutdown,
};

static int __init sec_battery_init(void)
{
	return platform_driver_register(&sec_battery_driver);
}

static void __exit sec_battery_exit(void)
{
	platform_driver_unregister(&sec_battery_driver);
}

late_initcall(sec_battery_init);
module_exit(sec_battery_exit);

MODULE_DESCRIPTION("Samsung Battery Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
