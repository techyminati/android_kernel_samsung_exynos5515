/*
 *  sysfs_battery.c
 *  Samsung Mobile sysfs battery
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "include/sec_sysfs.h"
#include "include/sec_battery.h"

static ssize_t sysfs_battery_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t sysfs_battery_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SYSFS_BATTERY_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sysfs_battery_show_attrs,					\
	.store = sysfs_battery_store_attrs,					\
}

static struct device_attribute sysfs_battery_attrs[] = {
	SYSFS_BATTERY_ATTR(batt_reset_soc),
	SYSFS_BATTERY_ATTR(batt_read_raw_soc),
	SYSFS_BATTERY_ATTR(batt_type),
	SYSFS_BATTERY_ATTR(batt_vfocv),
	SYSFS_BATTERY_ATTR(batt_vol_adc),
	SYSFS_BATTERY_ATTR(batt_current_ua_now),
	SYSFS_BATTERY_ATTR(batt_current_ua_avg),
	SYSFS_BATTERY_ATTR(chg_current_adc),

	SYSFS_BATTERY_ATTR(batt_charging_source),

	SYSFS_BATTERY_ATTR(batt_inbat_voltage),
	SYSFS_BATTERY_ATTR(batt_inbat_voltage_ocv),

	SYSFS_BATTERY_ATTR(batt_chip_id),
	SYSFS_BATTERY_ATTR(safety_timer_info),
	SYSFS_BATTERY_ATTR(safety_timer_set),

	SYSFS_BATTERY_ATTR(update),
	SYSFS_BATTERY_ATTR(voter_status),
};

enum {
	BATT_RESET_SOC = 0,
	BATT_READ_RAW_SOC,
	BATT_TYPE,
	BATT_VFOCV,
	BATT_VOL_ADC,
	BATT_CURRENT_UA_NOW,
	BATT_CURRENT_UA_AVG,
	CHG_CURRENT_ADC,

	BATT_CHARGING_SOURCE,

	BATT_INBAT_VOLTAGE,
	BATT_INBAT_VOLTAGE_OCV,

	BATT_CHIP_ID,
	SAFETY_TIMER_INFO,
	SAFETY_TIMER_SET,

	UPDATE,
	VOTER_STATUS,
};

static ssize_t sysfs_battery_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_battery_attrs;
	union power_supply_propval value = {0, };
	int i = 0;

	switch (offset) {
	case BATT_RESET_SOC:
		break;
	case BATT_READ_RAW_SOC:
		value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_RAW;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case BATT_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			battery->batt_type);
		break;
	case BATT_VFOCV:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->voltage_ocv);
		break;
	case BATT_VOL_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0\n");
		break;
	case BATT_CURRENT_UA_NOW:
		value.intval = SEC_BATTERY_CURRENT_UA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);
#if defined(CONFIG_SEC_FACTORY)
		pr_err("%s: batt_current_ua_now (%d)\n", __func__, value.intval);
#endif
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case BATT_CURRENT_UA_AVG:
		value.intval = SEC_BATTERY_CURRENT_UA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CURRENT_AVG, value);
		
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case CHG_CURRENT_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0\n");
		break;

	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->cable_type);
		break;

	case BATT_INBAT_VOLTAGE:
	case BATT_INBAT_VOLTAGE_OCV:
	{
		int ret_inbat = 0;

		if (battery->pdata->support_fgsrc_change == true) {
			int j, k, ocv, ocv_data[10];

			value.intval = 0;
			psy_do_property(battery->pdata->fgsrc_switch_name, set,
				POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);

			for (j = 0; j < 10; j++) {
				psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_EXT_PROP_FG_INBAT_VOLTAGE, value);
				ocv_data[j] = value.intval;
			}

			value.intval = 1;
			psy_do_property(battery->pdata->fgsrc_switch_name, set,
				POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);
			for (j = 1; j < 10; j++) {
				ocv = ocv_data[j];
				k = j;
				while (k > 0 && ocv_data[k-1] > ocv) {
					ocv_data[k] = ocv_data[k-1];
					k--;
				}
				ocv_data[k] = ocv;
			}
			ocv = 0;
			for (j = 2; j < 8; j++) {
				ocv += ocv_data[j];
			}
			ret_inbat = ocv / 6;
		}

		dev_info(battery->dev, "in-battery voltage ocv(%d)\n", ret_inbat);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", ret_inbat);
	}
		break;

	case BATT_CHIP_ID:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHIP_ID, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case SAFETY_TIMER_INFO:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%ld\n",
			battery->cal_safety_time);
		break;
	case SAFETY_TIMER_SET:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
	       battery->safety_timer_set);
		break;

	case UPDATE:
		break;
	case VOTER_STATUS:
		i = show_sec_vote_status(buf, PAGE_SIZE);
		break;
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t sysfs_battery_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_battery_attrs;
	union power_supply_propval value = {0, };
	int x = 0;

	switch (offset) {
	case BATT_RESET_SOC:
		/* Do NOT reset fuel gauge in charging mode */
		if ((battery->cable_type == SB_CBL_NONE) ||
			battery->is_jig_on) {
			sec_bat_set_misc_event(battery,
				BATT_MISC_EVENT_BATT_RESET_SOC, BATT_MISC_EVENT_BATT_RESET_SOC);

			value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_RESET;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CAPACITY, value);
			dev_info(battery->dev,"do reset SOC\n");
			/* update battery info */
			sec_bat_get_battery_info(battery);
		}
		break;
	case BATT_READ_RAW_SOC:
		break;
	case BATT_TYPE:
		strncpy(battery->batt_type, buf, sizeof(battery->batt_type) - 1);
		battery->batt_type[sizeof(battery->batt_type)-1] = '\0';
		break;
	case BATT_VFOCV:
	case BATT_VOL_ADC:
	case BATT_CURRENT_UA_NOW:
	case BATT_CURRENT_UA_AVG:
	case CHG_CURRENT_ADC:
		break;

	case BATT_CHARGING_SOURCE:
		break;

	case BATT_INBAT_VOLTAGE:
	case BATT_INBAT_VOLTAGE_OCV:
		break;

	case BATT_CHIP_ID:
	case SAFETY_TIMER_INFO:
		break;
	case SAFETY_TIMER_SET:
		if (sscanf(buf, "%10d\n", &x) == 1)
			battery->safety_timer_set = !!(x);
		break;

	case UPDATE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			/* update battery info */
			sec_bat_get_battery_info(battery);
		}
		break;
	case VOTER_STATUS:
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static struct sec_sysfs sysfs_battery_list = {
	.attr = sysfs_battery_attrs,
	.size = ARRAY_SIZE(sysfs_battery_attrs),
};

static int __init sysfs_battery_init(void)
{
	add_sec_sysfs(&sysfs_battery_list.list);
	return 0;
}
late_initcall(sysfs_battery_init);
