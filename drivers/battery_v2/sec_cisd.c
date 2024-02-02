/*
 *  sec_cisd.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "include/sec_battery.h"
#include "include/sec_cisd.h"
#include "include/sec_sysfs.h"
#include "include/sb_adc.h"

#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

const char *cisd_data_str[] = {
	"RESET_ALG",
	"ALG_INDEX",
	"FULL_CNT",
	"RECHARGING_CNT",
	"VALERT_CNT",
	"BATT_CYCLE",
	"WIRE_CNT",
	"WIRELESS_CNT",
	"HIGH_SWELLING_CNT",
	"LOW_SWELLING_CNT",
	"SWELLING_CHARGING",
	"SWELLING_FULL_CNT",
	"SWELLING_RECOVERY_CNT",
	"BATT_THM_MAX",
	"BATT_THM_MIN",
	"CHG_BATT_THM_MAX",
	"CHG_BATT_THM_MIN",
	"UNSAFETY_VOLT",
	"UNSAFETY_TEMP",
	"SAFETY_TIMER",
	"BUCK_OFF",
};
const char *cisd_data_str_d[] = {
	"FULL_CNT_D",
	"RECHARGING_CNT_D",
	"VALERT_CNT_D",
	"WIRE_CNT_D",
	"WIRELESS_CNT_D",
	"HIGH_SWELLING_CNT_D",
	"LOW_SWELLING_CNT_D",
	"SWELLING_CHARGING_D",
	"SWELLING_FULL_CNT_D",
	"SWELLING_RECOVERY_CNT_D",
	"BATT_THM_MAX_D",
	"BATT_THM_MIN_D",
	"CHG_BATT_THM_MAX_D",
	"CHG_BATT_THM_MIN_D",
	"UNSAFETY_VOLT_D",
	"UNSAFETY_TEMP_D",
	"SAFETY_TIMER_D",
	"BUCK_OFF_D",
};

const char *cisd_wc_data_str[] = {
	"INDEX",
	"SNGL_NOBLE",
	"SNGL_VEHICLE",
	"SNGL_MINI",
	"SNGL_ZERO",
	"SNGL_DREAM",
	"STAND_HERO",
	"STAND_DREAM",
	"EXT_PACK",
	"EXT_PACK_TA"
};

bool sec_bat_cisd_check(struct sec_battery_info *battery)
{
	union power_supply_propval incur_val = {0, };
	union power_supply_propval chgcur_val = {0, };
	union power_supply_propval capcurr_val = {0, };
	struct cisd *pcisd = &battery->cisd;
	bool ret = false;
	int batt_temp;

	if (battery->factory_mode || battery->is_jig_on || battery->skip_cisd) {
		dev_dbg(battery->dev, "%s: No need to check in factory mode\n",
			__func__);
		return ret;
	}

	sb_adc_get_valuef(BATT_TEMP, &batt_temp);
	if ((battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
		(battery->status == POWER_SUPPLY_STATUS_FULL)) {

#if 0
		/* check abnormal vbat */
		pcisd->ab_vbat_check_count = battery->voltage_now > pcisd->max_voltage_thr ?
				pcisd->ab_vbat_check_count + 1 : 0;

		if ((pcisd->ab_vbat_check_count >= pcisd->ab_vbat_max_count) &&
			!(pcisd->state & CISD_STATE_OVER_VOLTAGE)) {
			dev_info(battery->dev, "%s : [CISD] Battery Over Voltage Protction !! vbat(%d)mV\n",
				 __func__, battery->voltage_now);
			vbat_val.intval = true;
			psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_VBAT_OVP,
					vbat_val);
			pcisd->data[CISD_DATA_VBAT_OVP]++;
			pcisd->data[CISD_DATA_VBAT_OVP_PER_DAY]++;
			pcisd->state |= CISD_STATE_OVER_VOLTAGE;
#if defined(CONFIG_SEC_ABC)
			sec_abc_send_event("MODULE=battery@ERROR=over_voltage");
#endif
		}
#endif
		/* get actual input current */
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CURRENT_AVG, incur_val);

		/* get actual charging current */
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, chgcur_val);

		if (batt_temp > pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = batt_temp;
		if (batt_temp < pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = batt_temp;
		if (batt_temp > pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = batt_temp;
		if (batt_temp < pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = batt_temp;
		dev_info(battery->dev, "%s: [CISD] iavg: %d, incur: %d, chgcur: %d,\n"
			"cc_T: %ld, lcd_off_T: %ld, passed_T: %ld, full_T: %ld, chg_end_T: %ld, cisd: 0x%x\n", __func__,
			battery->current_avg, incur_val.intval, chgcur_val.intval,
			pcisd->cc_start_time, pcisd->lcd_off_start_time, battery->charging_passed_time,
			battery->charging_fullcharged_time, pcisd->charging_end_time, pcisd->state);
	} else  {
		capcurr_val.intval = SEC_BATTERY_CAPACITY_FULL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, capcurr_val);
		if (capcurr_val.intval == -1) {
			dev_info(battery->dev, "%s: [CISD] FG I2C fail. skip cisd check \n", __func__);
			return ret;
		}
	}

	if (batt_temp > pcisd->data[CISD_DATA_BATT_TEMP_MAX])
		pcisd->data[CISD_DATA_BATT_TEMP_MAX] = batt_temp;
	if (batt_temp < battery->cisd.data[CISD_DATA_BATT_TEMP_MIN])
		pcisd->data[CISD_DATA_BATT_TEMP_MIN] = batt_temp;
	if (batt_temp > pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = batt_temp;
	if (batt_temp < pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = batt_temp;

	return ret;
}

static void sec_battery_cisd_parse_dt(struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return;
	}

	ret = of_property_read_u32(np, "battery,battery_full_capacity",
		&pdata->battery_full_capacity);
	if (ret) {
		pr_info("%s : battery_full_capacity is Empty\n", __func__);
	} else {
		pr_info("%s : battery_full_capacity : %d\n", __func__, pdata->battery_full_capacity);
		pdata->cisd_cap_high_thr = pdata->battery_full_capacity + 1000;
		pdata->cisd_cap_low_thr = pdata->battery_full_capacity + 500;
		pdata->cisd_cap_limit = (pdata->battery_full_capacity * 11) / 10;
	}

	ret = of_property_read_u32(np, "battery,cisd_max_voltage_thr",
		&pdata->max_voltage_thr);
	if (ret) {
		pr_info("%s : cisd_max_voltage_thr is Empty\n", __func__);
		pdata->max_voltage_thr = 4400;
	}

	ret = of_property_read_u32(np, "battery,cisd_alg_index",
			&pdata->cisd_alg_index);
	if (ret) {
		pr_info("%s : cisd_alg_index is Empty. Defalut set to six\n", __func__);
		pdata->cisd_alg_index = 6;
	} else {
		pr_info("%s : set cisd_alg_index : %d\n", __func__, pdata->cisd_alg_index);
	}
}

struct cisd *gcisd;
void sec_battery_cisd_init(struct sec_battery_info *battery)
{
	union power_supply_propval capfull_val;

	sec_battery_cisd_parse_dt(battery);

	battery->cisd.state = CISD_STATE_NONE;

	battery->cisd.delay_time = 600; /* 10 min */
	battery->cisd.diff_volt_now = 40;
	battery->cisd.diff_cap_now = 5;

	capfull_val.intval = SEC_BATTERY_CAPACITY_FULL;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_ENERGY_NOW, capfull_val);
	battery->cisd.curr_cap_max = capfull_val.intval;
	battery->cisd.err_cap_high_thr = battery->pdata->cisd_cap_high_thr;
	battery->cisd.err_cap_low_thr = battery->pdata->cisd_cap_low_thr;
	battery->cisd.cc_delay_time = 3600; /* 60 min */
	battery->cisd.lcd_off_delay_time = 10200; /* 230 min */
	battery->cisd.full_delay_time = 3600; /* 60 min */
	battery->cisd.recharge_delay_time = 9000; /* 150 min */
	battery->cisd.cc_start_time = 0;
	battery->cisd.full_start_time = 0;
	battery->cisd.lcd_off_start_time = 0;
	battery->cisd.overflow_start_time = 0;
	battery->cisd.charging_end_time = 0;
	battery->cisd.charging_end_time_2 = 0;
	battery->cisd.recharge_count = 0;
	battery->cisd.recharge_count_2 = 0;
	battery->cisd.recharge_count_thres = 2;
	battery->cisd.leakage_e_time = 3600; /* 60 min */
	battery->cisd.leakage_f_time = 7200; /* 120 min */
	battery->cisd.leakage_g_time = 14400; /* 240 min */
	battery->cisd.current_max_thres = 1600;
	battery->cisd.charging_current_thres = 1000;
	battery->cisd.current_avg_thres = 1000;

	battery->cisd.data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;
	battery->cisd.data[CISD_DATA_FULL_COUNT] = 1;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.capacity_now = capfull_val.intval;
	battery->cisd.overflow_cap_thr = capfull_val.intval > battery->pdata->cisd_cap_limit ?
		capfull_val.intval : battery->pdata->cisd_cap_limit;

	battery->cisd.ab_vbat_max_count = 2; /* should be 1 */
	battery->cisd.ab_vbat_check_count = 0;
	battery->cisd.max_voltage_thr = battery->pdata->max_voltage_thr;
	battery->cisd.cisd_alg_index = 6;
	pr_info("%s: cisd.err_cap_high_thr:%d, cisd.err_cap_low_thr:%d, cisd.overflow_cap_thr:%d\n", __func__,
		battery->cisd.err_cap_high_thr, battery->cisd.err_cap_low_thr, battery->cisd.overflow_cap_thr);

	/* set cisd pointer */
	gcisd = &battery->cisd;

	/* initialize pad data */
	mutex_init(&battery->cisd.padlock);
	init_cisd_pad_data(&battery->cisd);
}


static struct pad_data* create_pad_data(unsigned int pad_id, unsigned int pad_count)
{
	struct pad_data* temp_data;

	temp_data = kzalloc(sizeof(struct pad_data), GFP_KERNEL);
	if (temp_data == NULL)
		return NULL;

	temp_data->id = pad_id;
	temp_data->count = pad_count;
	temp_data->prev = temp_data->next = NULL;

	return temp_data;
}

static struct pad_data* find_pad_data_by_id(struct cisd* cisd, unsigned int pad_id)
{
	struct pad_data* temp_data = cisd->pad_array->next;

	if (cisd->pad_count <= 0 || temp_data == NULL)
		return NULL;

	while ((temp_data->id != pad_id) &&
		((temp_data = temp_data->next) != NULL));

	return temp_data;
}

static void add_pad_data(struct cisd* cisd, unsigned int pad_id, unsigned int pad_count)
{
	struct pad_data* temp_data = cisd->pad_array->next;
	struct pad_data* pad_data;

	if (pad_id >= MAX_PAD_ID)
		return;

	pad_data = create_pad_data(pad_id, pad_count);
	if (pad_data == NULL)
		return;

	pr_info("%s: id(0x%x), count(%d)\n", __func__, pad_id, pad_count);
	while (temp_data) {
		if (temp_data->id > pad_id) {
			temp_data->prev->next = pad_data;
			pad_data->prev = temp_data->prev;
			pad_data->next = temp_data;
			temp_data->prev = pad_data;
			cisd->pad_count++;
			return;
		}
		temp_data = temp_data->next;
	}

	pr_info("%s: failed to add pad_data(%d, %d)\n",
		__func__, pad_id, pad_count);
	kfree(pad_data);
}

void init_cisd_pad_data(struct cisd* cisd)
{
	struct pad_data *temp_data = NULL;

	mutex_lock(&cisd->padlock);
	temp_data = cisd->pad_array;
	while (temp_data) {
		struct pad_data* next_data = temp_data->next;

		kfree(temp_data);
		temp_data = next_data;
	}

	/* create dummy data */
	cisd->pad_array = create_pad_data(0, 0);
	if (cisd->pad_array == NULL)
		goto err_create_dummy_data;
	temp_data = create_pad_data(MAX_PAD_ID, 0);
	if (temp_data == NULL) {
		kfree(cisd->pad_array);
		cisd->pad_array = NULL;
		goto err_create_dummy_data;
	}
	cisd->pad_count = 0;
	cisd->pad_array->next = temp_data;
	temp_data->prev = cisd->pad_array;

err_create_dummy_data:
	mutex_unlock(&cisd->padlock);
}

void count_cisd_pad_data(struct cisd* cisd, unsigned int pad_id)
{
	struct pad_data* pad_data;

	if (cisd->pad_array == NULL) {
		pr_info("%s: can't update the connected count of pad_id(0x%x) because of null\n",
			__func__, pad_id);
		return;
	}

	mutex_lock(&cisd->padlock);
	if ((pad_data = find_pad_data_by_id(cisd, pad_id)) != NULL)
		pad_data->count++;
	else
		add_pad_data(cisd, pad_id, 1);
	mutex_unlock(&cisd->padlock);
}

void set_cisd_pad_data(struct sec_battery_info *battery, const char* buf)
{
	struct cisd *pcisd = &battery->cisd;
	unsigned int pad_total_count, pad_id, pad_count;
	struct pad_data *pad_data;
	int i, x;

	pr_info("%s: %s\n", __func__, buf);
	if (pcisd->pad_count > 0)
		init_cisd_pad_data(pcisd);

	if (pcisd->pad_array == NULL) {
		pr_info("%s: can't set the pad data because of null\n", __func__);
		return;
	}

	if (sscanf(buf, "%10u %n", &pad_total_count, &x) <= 0) {
		pr_info("%s: failed to read pad index\n", __func__);
		return;
	}
	buf += (size_t)x;
	pr_info("%s: stored pad_total_count(%d)\n", __func__, pad_total_count);

	if (pad_total_count >= MAX_PAD_ID)
		return;

	if (pad_total_count <= 0)
		return;

	pr_info("%s: add pad data(count: %d)\n", __func__, pad_total_count);
	for (i = 0; i < pad_total_count; i++) {
		if (sscanf(buf, "0x%02x:%10u %n", &pad_id, &pad_count, &x) != 2) {
			pr_info("%s: failed to read pad data(0x%x, %d, %d)!!!re-init pad data\n",
				__func__, pad_id, pad_count, x);
			init_cisd_pad_data(pcisd);
			break;
		}
		buf += (size_t)x;

		mutex_lock(&pcisd->padlock);
		if ((pad_data = find_pad_data_by_id(pcisd, pad_id)) != NULL)
			pad_data->count = pad_count;
		else
			add_pad_data(pcisd, pad_id, pad_count);
		mutex_unlock(&pcisd->padlock);
	}
}

static ssize_t sysfs_cisd_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t sysfs_cisd_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SYSFS_CISD_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sysfs_cisd_show_attrs,					\
	.store = sysfs_cisd_store_attrs,					\
}

static struct device_attribute sysfs_cisd_attrs[] = {
	SYSFS_CISD_ATTR(cisd_fullcaprep_max),
	SYSFS_CISD_ATTR(cisd_data),
	SYSFS_CISD_ATTR(cisd_data_json),
	SYSFS_CISD_ATTR(cisd_data_d_json),
	SYSFS_CISD_ATTR(cisd_wire_count),
	SYSFS_CISD_ATTR(cisd_wc_data),
	SYSFS_CISD_ATTR(cisd_wc_data_json),
	SYSFS_CISD_ATTR(prev_battery_data),
	SYSFS_CISD_ATTR(prev_battery_info),
};

enum {
	CISD_FULLCAPREP_MAX = 0,
	CISD_DATA,
	CISD_DATA_JSON,
	CISD_DATA_D_JSON,
	CISD_WIRE_COUNT,
	CISD_WC_DATA,
	CISD_WC_DATA_JSON,
	PREV_BATTERY_DATA,
	PREV_BATTERY_INFO,
};

static ssize_t sysfs_cisd_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_cisd_attrs;
	union power_supply_propval value = {0, };
	int i = 0;

	switch (offset) {
	case CISD_FULLCAPREP_MAX:
		value.intval = SEC_BATTERY_CAPACITY_FULL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case CISD_DATA:
	{
		struct cisd *pcisd = &battery->cisd;
		char temp_buf[1024] = {0,};
		int j = 0;

		sprintf(temp_buf+strlen(temp_buf), "%d", pcisd->data[CISD_DATA_RESET_ALG]);
		for (j = CISD_DATA_RESET_ALG + 1; j < CISD_DATA_MAX_PER_DAY; j++)
			sprintf(temp_buf+strlen(temp_buf), " %d", pcisd->data[j]);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
	}
		break;
	case CISD_DATA_JSON:
	{
		struct cisd *pcisd = &battery->cisd;
		char temp_buf[1920] = {0,};
		int j = 0;

		sprintf(temp_buf+strlen(temp_buf), "\"%s\":\"%d\"",
				cisd_data_str[CISD_DATA_RESET_ALG], pcisd->data[CISD_DATA_RESET_ALG]);
		for (j = CISD_DATA_RESET_ALG + 1; j < CISD_DATA_MAX; j++)
			sprintf(temp_buf+strlen(temp_buf), ",\"%s\":\"%d\"", cisd_data_str[j], pcisd->data[j]);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
	}
		break;
	case CISD_DATA_D_JSON:
	{
		struct cisd *pcisd = &battery->cisd;
		char temp_buf[1920] = {0,};
		int j = 0;

		sprintf(temp_buf+strlen(temp_buf), "\"%s\":\"%d\"",
			cisd_data_str_d[CISD_DATA_FULL_COUNT_PER_DAY-CISD_DATA_MAX],
			pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY]);
		for (j = CISD_DATA_FULL_COUNT_PER_DAY + 1; j < CISD_DATA_MAX_PER_DAY; j++)
			sprintf(temp_buf+strlen(temp_buf), ",\"%s\":\"%d\"",
			cisd_data_str_d[j-CISD_DATA_MAX], pcisd->data[j]);

		/* Clear Daily Data */
		for (j = CISD_DATA_FULL_COUNT_PER_DAY; j < CISD_DATA_MAX_PER_DAY; j++)
			pcisd->data[j] = 0;

		pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
		pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
		pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
		pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
		pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
	}
		break;
	case CISD_WIRE_COUNT:
	{
		struct cisd *pcisd = &battery->cisd;

		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			pcisd->data[CISD_DATA_WIRE_COUNT]);
	}
		break;
	case CISD_WC_DATA:
	{
		struct cisd *pcisd = &battery->cisd;
		struct pad_data *pad_data = NULL;
		char temp_buf[1024] = {0,};
		int j = 0, size = 1024;

		mutex_lock(&pcisd->padlock);
		pad_data = pcisd->pad_array;
		snprintf(temp_buf, size, "%d", pcisd->pad_count);
		while ((pad_data != NULL) && ((pad_data = pad_data->next) != NULL) &&
				(pad_data->id < MAX_PAD_ID) && (j++ < pcisd->pad_count)) {
			snprintf(temp_buf+strlen(temp_buf), size, " 0x%02x:%d", pad_data->id, pad_data->count);
			size = sizeof(temp_buf) - strlen(temp_buf);
		}
		mutex_unlock(&pcisd->padlock);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
	}
		break;
	case CISD_WC_DATA_JSON:
	{
		struct cisd *pcisd = &battery->cisd;
		struct pad_data *pad_data = NULL;
		char temp_buf[1024] = {0,};
		int j = 0, size = 1024;

		mutex_lock(&pcisd->padlock);
		pad_data = pcisd->pad_array;
		snprintf(temp_buf+strlen(temp_buf), size, "\"%s\":\"%d\"",
			PAD_INDEX_STRING, pcisd->pad_count);
		while ((pad_data != NULL) && ((pad_data = pad_data->next) != NULL) &&
				(pad_data->id < MAX_PAD_ID) && (j++ < pcisd->pad_count)) {
			snprintf(temp_buf+strlen(temp_buf), size, ",\"%s%02x\":\"%d\"",
				PAD_JSON_STRING, pad_data->id, pad_data->count);
			size = sizeof(temp_buf) - strlen(temp_buf);
		}
		mutex_unlock(&pcisd->padlock);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
	}
		break;
	case PREV_BATTERY_DATA:
	{
		int batt_temp;

		sb_adc_get_valuef(BATT_TEMP, &batt_temp);
		if (battery->enable_update_data)
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d, %d, %d, %d\n",
				battery->voltage_now, batt_temp, battery->is_jig_on,
				(battery->charger_mode == SEC_BAT_CHG_MODE_CHARGING) ? 1 : 0);
	}
		break;
	case PREV_BATTERY_INFO:
	{
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d,%d,%d,%d\n",
			battery->prev_volt, battery->prev_temp,
			battery->prev_jig_on, battery->prev_chg_on);
		pr_info("%s: Read Prev Battery Info : %d, %d, %d, %d\n", __func__,
			battery->prev_volt, battery->prev_temp,
			battery->prev_jig_on, battery->prev_chg_on);
	}
		break;
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t sysfs_cisd_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_cisd_attrs;
	int x = 0, i;

	switch (offset) {
	case CISD_FULLCAPREP_MAX:
		break;
	case CISD_DATA:
	{
		struct cisd *pcisd = &battery->cisd;
		int temp_data[CISD_DATA_MAX_PER_DAY] = {0,};

		sscanf(buf, "%10d\n", &temp_data[0]);

		if (temp_data[CISD_DATA_RESET_ALG] > 0) {
			if (sscanf(buf,
				"%10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, "
				"%10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, "
				"%10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, "
				"%10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d\n",
				&temp_data[0], &temp_data[1], &temp_data[2],
				&temp_data[3], &temp_data[4], &temp_data[5],
				&temp_data[6], &temp_data[7], &temp_data[8],
				&temp_data[9], &temp_data[10], &temp_data[11],
				&temp_data[12], &temp_data[13], &temp_data[14],
				&temp_data[15], &temp_data[16], &temp_data[17],
				&temp_data[18], &temp_data[19], &temp_data[20],
				&temp_data[21], &temp_data[22], &temp_data[23],
				&temp_data[24], &temp_data[25], &temp_data[26],
				&temp_data[27], &temp_data[28], &temp_data[29],
				&temp_data[30], &temp_data[31], &temp_data[32],
				&temp_data[33], &temp_data[34], &temp_data[35],
				&temp_data[36], &temp_data[37], &temp_data[38]) <= CISD_DATA_MAX_PER_DAY) {
				for (i = 0; i < CISD_DATA_MAX_PER_DAY; i++)
					pcisd->data[i] = 0;

				pcisd->data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;

				for (i = CISD_DATA_FULL_COUNT; i < CISD_DATA_MAX_PER_DAY; i++) {
					battery->cisd.data[i] = temp_data[i];
				}
			}
		} else {
			const char *p = buf;

			pr_info("%s: %s\n", __func__, buf);
			for (i = CISD_DATA_RESET_ALG; i < CISD_DATA_MAX_PER_DAY; i++) {
				sscanf(p, "%10d%n", &pcisd->data[i], &x);
				p += (size_t)x;
			}

			pr_info("%s: %s cisd data\n", __func__,
				((temp_data[CISD_DATA_RESET_ALG] < 0 || battery->fg_reset) ? "init" : "update"));

			if (temp_data[CISD_DATA_RESET_ALG] < 0 || battery->fg_reset) {
				/* initialize data */
				for (i = CISD_DATA_RESET_ALG; i < CISD_DATA_MAX_PER_DAY; i++)
					pcisd->data[i] = 0;
				for (i = WC_DATA_INDEX; i < WC_DATA_MAX; i++)
					pcisd->wc_data[i] = 0;

				battery->fg_reset = 0;

				pcisd->data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;

				pcisd->data[CISD_DATA_FULL_COUNT] = 1;
				pcisd->data[CISD_DATA_BATT_TEMP_MAX] = -300;
				pcisd->data[CISD_DATA_BATT_TEMP_MIN] = 1000;
				pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
				pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
				pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = -300;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = 1000;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
			}
		}

		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	}
		break;
	case CISD_DATA_JSON:
	{
		char tc;
		struct cisd *pcisd = &battery->cisd;

		if (sscanf(buf, "%1c\n", &tc) == 1) {
			if (tc == 'c') {
				for (i = 0; i < CISD_DATA_MAX; i++)
					pcisd->data[i] = 0;

				pcisd->data[CISD_DATA_FULL_COUNT] = 1;
				pcisd->data[CISD_DATA_BATT_TEMP_MAX] = -300;
				pcisd->data[CISD_DATA_BATT_TEMP_MIN] = 1000;
				pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
				pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
				pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = -300;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = 1000;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
				pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
			}
		}
	}
		break;
	case CISD_DATA_D_JSON:
		break;
	case CISD_WIRE_COUNT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			struct cisd *pcisd = &battery->cisd;
			pr_info("%s: Wire Count : %d\n", __func__, x);
			pcisd->data[CISD_DATA_WIRE_COUNT] = x;
			pcisd->data[CISD_DATA_WIRE_COUNT_PER_DAY]++;
		}
		break;
	case CISD_WC_DATA:
		set_cisd_pad_data(battery, buf);
		break;
	case CISD_WC_DATA_JSON:
		break;
	case PREV_BATTERY_DATA:
		if (sscanf(buf, "%10d, %10d, %10d, %10d\n",
				&battery->prev_volt, &battery->prev_temp,
				&battery->prev_jig_on, &battery->prev_chg_on) >= 4) {
			pr_info("%s: prev voltage : %d, prev_temp : %d, prev_jig_on : %d, prev_chg_on : %d\n",
				__func__, battery->prev_volt, battery->prev_temp,
				battery->prev_jig_on, battery->prev_chg_on);

			if (battery->prev_volt >= 3700 && battery->prev_temp >= 150 &&
					!battery->prev_jig_on && battery->fg_reset)
				pr_info("%s: Battery have been Removed\n", __func__);
		}
		battery->enable_update_data = 1;
		break;
	case PREV_BATTERY_INFO:
		break;

	default:
		return -EINVAL;
	}

	return count;
}

static struct sec_sysfs sysfs_cisd_list = {
	.attr = sysfs_cisd_attrs,
	.size = ARRAY_SIZE(sysfs_cisd_attrs),
};

static int __init sysfs_cisd_init(void)
{
	add_sec_sysfs(&sysfs_cisd_list.list);
	return 0;
}
late_initcall(sysfs_cisd_init);
