/*
 * sec_battery_ttf.c
 * Samsung Mobile Battery Driver
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "include/sec_battery.h"
#include "include/sec_battery_ttf.h"
#include "include/sb_wrl_data.h"

#define is_ttf_thermal_zone(thermal_zone) ( \
	thermal_zone == BAT_THERMAL_NORMAL || \
	thermal_zone == BAT_THERMAL_COOL1 || \
	thermal_zone == BAT_THERMAL_COOL2)

static int sec_calc_ttf(struct sec_battery_info * battery, unsigned int ttf_curr)
{
	struct sec_cv_slope *cv_data = battery->ttf_d->cv_data;
	int i, cc_time = 0, cv_time = 0;
	int soc = battery->capacity;
	int charge_current = ttf_curr;
	int design_cap = battery->ttf_d->ttf_capacity;
	union power_supply_propval value = {0, };

	value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
	soc = value.intval;

	if (!cv_data || (ttf_curr <= 0)) {
		pr_info("%s: no cv_data or val: %d\n", __func__, ttf_curr);
		return -1;
	}
	for (i = 0; i < battery->ttf_d->cv_data_length; i++) {
		if (charge_current >= cv_data[i].fg_current)
			break;
	}
	i = i >= battery->ttf_d->cv_data_length ? battery->ttf_d->cv_data_length - 1 : i;
	if (cv_data[i].soc < soc) {
		for (i = 0; i < battery->ttf_d->cv_data_length; i++) {
			if (soc <= cv_data[i].soc)
				break;
		}
		cv_time =
		    ((cv_data[i - 1].time - cv_data[i].time) * (cv_data[i].soc - soc)
		     / (cv_data[i].soc - cv_data[i - 1].soc)) + cv_data[i].time;
	} else {		/* CC mode || NONE */
		cv_time = cv_data[i].time;
		cc_time =
			design_cap * (cv_data[i].soc - soc) / ttf_curr * 3600 / 1000;
		pr_debug("%s: cc_time: %d\n", __func__, cc_time);
		if (cc_time < 0)
			cc_time = 0;
	}

	pr_info("%s: cap: %d, soc: %4d, T: %6d, avg: %4d, cv soc: %4d, i: %4d, val: %d\n",
	     __func__, design_cap, soc, cv_time + cc_time,
	     battery->current_avg, cv_data[i].soc, i, ttf_curr);

	if (cv_time + cc_time >= 0)
		return cv_time + cc_time + 60;
	else
		return 60;	/* minimum 1minutes */
}

void sec_bat_calc_time_to_full(struct sec_battery_info * battery)
{
	union power_supply_propval value = {0, };

	if (!battery->ttf_d) return;

	if (delayed_work_pending(&battery->ttf_d->timetofull_work)) {
		pr_info("%s: keep time_to_full(%5d sec)\n", __func__, battery->ttf_d->timetofull);
	} else if (battery->status == POWER_SUPPLY_STATUS_CHARGING ||
		(battery->status == POWER_SUPPLY_STATUS_FULL && battery->capacity != 100)) {
		int charge = battery->pdata->charging_current[battery->cable_type].ttf_fcc;

		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_TX_PWR_BUDG, value);
		switch (value.intval) {
		case SB_WRL_TX_PWR_BUDG_2W:
			charge = battery->pdata->charging_current[SB_CBL_AUTH_WRL_2W].ttf_fcc;
			break;
		case SB_WRL_TX_PWR_BUDG_5W:
			charge = battery->pdata->charging_current[SB_CBL_AUTH_WRL_5W].ttf_fcc;
			break;
		default:
			break;
		}

		battery->ttf_d->timetofull = sec_calc_ttf(battery, charge);
		dev_info(battery->dev, "%s: T: %5d sec, passed time: %5ld, current: %d\n",
				__func__, battery->ttf_d->timetofull, battery->charging_passed_time, charge);
	} else {
		battery->ttf_d->timetofull = -1;
	}
}

static void sec_bat_time_to_full_work(struct work_struct *work)
{
	struct sec_ttf_data *dev = container_of(work,
				struct sec_ttf_data, timetofull_work.work);
	struct sec_battery_info *battery = dev->pdev;
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_MAX, value);
	battery->current_max = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg = value.intval;

	sec_bat_calc_time_to_full(battery);
	dev_info(battery->dev, "%s:\n",__func__);
	if (battery->voltage_now > 0)
		battery->voltage_now--;

	power_supply_changed(battery->psy_bat);
}

extern unsigned int lpcharge;
void ttf_work_start(struct sec_battery_info *battery)
{
	if (!battery->ttf_d) return;

	cancel_delayed_work(&battery->ttf_d->timetofull_work);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->ttf_d->timetofull_work, 0);
}

int ttf_display(struct sec_battery_info *battery)
{
	if (!battery->ttf_d) return -EPERM;

	if (battery->capacity == 100)
		return 0;

	if (((battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
		(battery->status == POWER_SUPPLY_STATUS_FULL && battery->capacity != 100)) &&
		is_ttf_thermal_zone(battery->thermal_zone))
		return battery->ttf_d->timetofull;
	else
		return 0;
}

int ttf_parse_fcc(struct device_node *np, unsigned int *fcc)
{
	if (!np)
		return -EINVAL;

	return of_property_read_u32(np, "ttf_fcc", fcc);
}

int ttf_init(struct sec_battery_info *battery)
{
	sec_battery_platform_data_t *bpdata = battery->pdata;
	struct sec_ttf_data *pdata;
	struct device_node *np;
	int ret = 0, len = 0;
	const u32 *p;

	np = of_find_node_by_name(NULL, "battery");
		if (!np) {
			pr_info("%s: np NULL\n", __func__);
			return -ENOENT;
	}

	pdata = kzalloc(sizeof(struct sec_ttf_data), GFP_KERNEL);
	if (!pdata) {
		pr_err("Failed to allocate memory\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32(np, "battery,ttf_capacity",
					   &pdata->ttf_capacity);
	if (ret < 0) {
		pr_err("%s error reading capacity_calculation_type %d\n", __func__, ret);
		pdata->ttf_capacity = bpdata->battery_full_capacity;
	}

	p = of_get_property(np, "battery,cv_data", &len);
	if (p) {
		pdata->cv_data = kzalloc(len, GFP_KERNEL);
		pdata->cv_data_length = len / sizeof(struct sec_cv_slope);
		pr_err("%s: len= %ld, length= %d, %d\n", __func__,
		       sizeof(int) * len, len, pdata->cv_data_length);
		ret = of_property_read_u32_array(np, "battery,cv_data",
				(u32 *)pdata->cv_data, len / sizeof(u32));
		if (ret) {
			pr_err("%s: failed to read battery->cv_data: %d\n",
				__func__, ret);
			kfree(pdata->cv_data);
			pdata->cv_data = NULL;
			kfree(pdata);
			return -ESRCH;
		}
	} else {
		pr_err("%s: there is not cv_data\n", __func__);
		kfree(pdata);
		return -ESRCH;
	}

	pdata->pdev = battery;
	battery->ttf_d = pdata;
	battery->ttf_d->timetofull = -1;
	INIT_DELAYED_WORK(&battery->ttf_d->timetofull_work, sec_bat_time_to_full_work);

	pr_info("%s: finished\n", __func__);
	return 0;
}
