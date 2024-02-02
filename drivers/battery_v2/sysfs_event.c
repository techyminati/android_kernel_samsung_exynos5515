/*
 *  sysfs_event.c
 *  Samsung Mobile sysfs event
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
#include "include/sec_battery_vote.h"
#include "include/sec_vote_event.h"
#include "include/sb_wrl_def.h"
#include <linux/sec_ext.h>

/* event check */
#define EVENT_NONE			(0)
#define EVENT_2G_CALL		(0x1 << 0)
#define EVENT_3G_CALL		(0x1 << 1)
#define EVENT_MUSIC			(0x1 << 2)
#define EVENT_VIDEO			(0x1 << 3)
#define EVENT_BROWSER		(0x1 << 4)
#define EVENT_HOTSPOT		(0x1 << 5)
#define EVENT_CAMERA		(0x1 << 6)
#define EVENT_CAMCORDER		(0x1 << 7)
#define EVENT_DATA_CALL		(0x1 << 8)
#define EVENT_WIFI			(0x1 << 9)
#define EVENT_WIBRO			(0x1 << 10)
#define EVENT_LTE			(0x1 << 11)
#define EVENT_LCD			(0x1 << 12)
#define EVENT_GPS			(0x1 << 13)

static ssize_t sysfs_event_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t sysfs_event_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SYSFS_EVENT_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sysfs_event_show_attrs,					\
	.store = sysfs_event_store_attrs,					\
}

static struct device_attribute sysfs_device_attrs[] = {
	SYSFS_EVENT_ATTR(batt_lp_charging),
	SYSFS_EVENT_ATTR(batt_slate_mode),
	SYSFS_EVENT_ATTR(factory_mode),
	SYSFS_EVENT_ATTR(store_mode),
	SYSFS_EVENT_ATTR(tct_low_current_mode),

	SYSFS_EVENT_ATTR(test_mode),
	SYSFS_EVENT_ATTR(set_stability_test),
	SYSFS_EVENT_ATTR(factory_mode_bypass),
	SYSFS_EVENT_ATTR(batt_swelling_control),
	SYSFS_EVENT_ATTR(batt_wdt_control),

	SYSFS_EVENT_ATTR(event),
	SYSFS_EVENT_ATTR(call),
	SYSFS_EVENT_ATTR(2g_call),
	SYSFS_EVENT_ATTR(talk_gsm),
	SYSFS_EVENT_ATTR(3g_call),
	SYSFS_EVENT_ATTR(talk_wcdma),
	SYSFS_EVENT_ATTR(music),
	SYSFS_EVENT_ATTR(video),
	SYSFS_EVENT_ATTR(browser),
	SYSFS_EVENT_ATTR(hotspot),
	SYSFS_EVENT_ATTR(camera),
	SYSFS_EVENT_ATTR(camcorder),
	SYSFS_EVENT_ATTR(data_call),
	SYSFS_EVENT_ATTR(wifi),
	SYSFS_EVENT_ATTR(wibro),
	SYSFS_EVENT_ATTR(lte),
	SYSFS_EVENT_ATTR(gps),
	SYSFS_EVENT_ATTR(lcd),

	SYSFS_EVENT_ATTR(batt_misc_event),
	SYSFS_EVENT_ATTR(batt_current_event),
	SYSFS_EVENT_ATTR(batt_misc_test),
};

enum {
	BATT_LP_CHARGING = 0,
	BATT_SLATE_MODE,
	FACTORY_MODE,
	STORE_MODE,
	TCT_LOW_CURRENT_MODE,

	TEST_MODE,
	SET_STABILITY_TEST,
	FACTORY_MODE_BYPASS,
	BATT_SWELLING_CONTROL,
	BATT_WDT_CONTROL,

	BATT_EVENT,
	BATT_EVENT_CALL,
	BATT_EVENT_2G_CALL,
	BATT_EVENT_TALK_GSM,
	BATT_EVENT_3G_CALL,
	BATT_EVENT_TALK_WCDMA,
	BATT_EVENT_MUSIC,
	BATT_EVENT_VIDEO,
	BATT_EVENT_BROWSER,
	BATT_EVENT_HOTSPOT,
	BATT_EVENT_CAMERA,
	BATT_EVENT_CAMCORDER,
	BATT_EVENT_DATA_CALL,
	BATT_EVENT_WIFI,
	BATT_EVENT_WIBRO,
	BATT_EVENT_LTE,
	BATT_EVENT_GPS,
	BATT_EVENT_LCD,

	BATT_MISC_EVENT,
	BATT_CURRENT_EVENT,
	BATT_MISC_TEST,
};

enum battery_misc_test {
	MISC_TEST_UNKNOWN = 0,
	MISC_TEST_TEMPERATURE,
	MISC_TEST_MAX,
};

static ssize_t sysfs_event_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_device_attrs;
	int i = 0;

	switch (offset) {
	case BATT_LP_CHARGING:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", lpcharge ? 1 : 0);
		break;
	case BATT_SLATE_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", is_slate_mode(battery));
		break;
	case FACTORY_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->factory_mode);
		break;
	case STORE_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->store_mode);
		break;
	case TCT_LOW_CURRENT_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->low_current_tct);
		break;
	case TEST_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->test_mode);
		break;
	case SET_STABILITY_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->stability_test);
		break;
	case FACTORY_MODE_BYPASS:
		break;
	case BATT_SWELLING_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->skip_swelling);
		break;
	case BATT_WDT_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->wdt_kick_disable);
		break;
	case BATT_EVENT:
	case BATT_EVENT_CALL:
	case BATT_EVENT_2G_CALL:
	case BATT_EVENT_TALK_GSM:
	case BATT_EVENT_3G_CALL:
	case BATT_EVENT_TALK_WCDMA:
	case BATT_EVENT_MUSIC:
	case BATT_EVENT_VIDEO:
	case BATT_EVENT_BROWSER:
	case BATT_EVENT_HOTSPOT:
	case BATT_EVENT_CAMERA:
	case BATT_EVENT_CAMCORDER:
	case BATT_EVENT_DATA_CALL:
	case BATT_EVENT_WIFI:
	case BATT_EVENT_WIBRO:
	case BATT_EVENT_LTE:
	case BATT_EVENT_GPS:
	case BATT_EVENT_LCD:
		break;
	case BATT_MISC_EVENT:
	{
		int old_value = battery->misc_event;
	
		sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_RESET_AFTER_READ_MASK);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", old_value);
	}
		break;
	case BATT_CURRENT_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->current_event);
		break;
	case BATT_MISC_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->disable_temp_check);
		break;
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t sysfs_event_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_device_attrs;
	union power_supply_propval value = {0, };
	int x = 0;

	switch (offset) {
	case BATT_LP_CHARGING:
		break;
	case BATT_SLATE_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			int slate_mode = x;

			if (x == is_slate_mode(battery)) {
				dev_info(battery->dev,
					 "%s : skip same slate mode : %d\n",
					__func__, x);
				return count;
			} else if (slate_mode == 1) {
				/* slate_mode 1 means discharge start */
				sec_bat_set_current_event(battery,
					SEC_BAT_CURRENT_EVENT_SLATE,
					SEC_BAT_CURRENT_EVENT_SLATE);
				sec_votef(VN_WPC_PHM, VOTER_SLATE, true, WPC_PHM_ON);
				sec_vote(battery->chgen_vote, VOTER_SLATE,
						true,
						SEC_BAT_CHG_MODE_BUCK_OFF);
				dev_info(battery->dev, "%s: enable slate mode : %d\n", __func__, slate_mode);
			} else if (slate_mode == 0) {
				/* slate_mode 0 means charge start */
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SLATE);
				sec_vote(battery->chgen_vote, VOTER_SLATE,
					false, 0);
				sec_votef(VN_WPC_PHM, VOTER_SLATE, false, WPC_PHM_OFF);
				dev_info(battery->dev,
					"%s: disable slate mode : %d\n", __func__, slate_mode);
				battery->cable_type = SB_CBL_NONE;
			} else {
				dev_info(battery->dev, "%s: SLATE MODE unknown command\n", __func__);
				return -EINVAL;
			}

			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
		}
		break;
	case FACTORY_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1)
			battery->factory_mode = x ? true : false;
		break;
	case STORE_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !defined(CONFIG_SEC_FACTORY)
			if (x) {
				battery->store_mode = true;
				__pm_stay_awake(battery->parse_mode_dt_ws);
				queue_delayed_work(battery->monitor_wqueue, &battery->parse_mode_dt_work, 0);
			}
#endif
		}
		break;
	case TCT_LOW_CURRENT_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev, "%s: low_current_tct: %d\n", __func__, x);
			battery->low_current_tct = x;
			if (battery->low_current_tct) {
				sec_bat_set_current_event(battery,
					SEC_BAT_CURRENT_EVENT_LOW_CURRENT_TCT,
					SEC_BAT_CURRENT_EVENT_LOW_CURRENT_TCT);
			} else {
				sec_bat_set_current_event(battery,
					0,
					SEC_BAT_CURRENT_EVENT_LOW_CURRENT_TCT);
			}
			sec_bat_set_charging_current(battery);
		}
		break;
	case TEST_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->test_mode = x;
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
		break;
	case SET_STABILITY_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_err(battery->dev,
				"%s: BATT_STABILITY_TEST(%d)\n", __func__, x);
			if (x) {
				battery->stability_test = true;
				battery->eng_not_full_status = true;
			}
			else {
				battery->stability_test = false;
				battery->eng_not_full_status = false;
			}
		}
		break;
	case FACTORY_MODE_BYPASS:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->pdata->fake_capacity = false;
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_AUTHENTIC, value);
		}
		break;
	case BATT_SWELLING_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x) {
				pr_info("%s : 15TEST START!! SWELLING MODE DISABLE\n", __func__);
				battery->skip_swelling = true;
			} else {
				pr_info("%s : 15TEST END!! SWELLING MODE END\n", __func__);
				battery->skip_swelling = false;
			}
		}
		break;
	case BATT_WDT_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: Charger WDT Set : %d\n", __func__, x);
			battery->wdt_kick_disable = x;
		}
		break;
	case BATT_EVENT:
	case BATT_EVENT_CALL:
	case BATT_EVENT_2G_CALL:
	case BATT_EVENT_TALK_GSM:
	case BATT_EVENT_3G_CALL:
	case BATT_EVENT_TALK_WCDMA:
	case BATT_EVENT_MUSIC:
	case BATT_EVENT_VIDEO:
	case BATT_EVENT_BROWSER:
	case BATT_EVENT_HOTSPOT:
	case BATT_EVENT_CAMERA:
	case BATT_EVENT_CAMCORDER:
	case BATT_EVENT_DATA_CALL:
	case BATT_EVENT_WIFI:
	case BATT_EVENT_WIBRO:
	case BATT_EVENT_LTE:
	case BATT_EVENT_GPS:
		break;
	case BATT_EVENT_LCD:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->lcd_status = x ? true : false;
			pr_info("%s : lcd_status (%d)\n", __func__, battery->lcd_status);
		}
		break;
	case BATT_MISC_EVENT:
		if (sscanf(buf, "%10d\n", &x) == 1)
			pr_info("%s: MISC EVENT = %d ", __func__, x);

		break;
	case BATT_CURRENT_EVENT:
		break;
	case BATT_MISC_TEST:
	{
		int y;
#if defined(CM_OFFSET)
		int ret;
#endif

		if (sscanf(buf, "%10d %10d\n", &x, &y) == 2) {
			pr_info("%s batt_misc_test %d, %d\n", __func__, x, y);
			switch (x) {
			case MISC_TEST_UNKNOWN:
				pr_info("%s Unknown MISC_TEST command\n", __func__);
				break;
			case MISC_TEST_TEMPERATURE:
#if defined(CM_OFFSET)
				if (y) {
					sec_bat_temp_misc_test(battery, true);
					ret = sec_set_param(CM_OFFSET + 3, '1');
				} else {
					sec_bat_temp_misc_test(battery, false);
					ret = sec_set_param(CM_OFFSET + 3, '0');
				}
				if (ret < 0)
					pr_err("%s:sec_set_param failed\n", __func__);
				else
					pr_info("%s:batt_temp_control_test param is set\n", __func__);
#endif
				break;
			case MISC_TEST_MAX:
			default:
				pr_info("%s Wrong MISC_TEST command\n", __func__);
				break;
			}
		}
	}
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static struct sec_sysfs sysfs_event_list = {
	.attr = sysfs_device_attrs,
	.size = ARRAY_SIZE(sysfs_device_attrs),
};

static int __init sysfs_event_init(void)
{
	add_sec_sysfs(&sysfs_event_list.list);
	return 0;
}
late_initcall(sysfs_event_init);
