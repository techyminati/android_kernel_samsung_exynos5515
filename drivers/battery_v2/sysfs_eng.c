/*
 *  sysfs_eng.c
 *  Samsung Mobile sysfs device
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
#include "include/sec_battery_eng.h"
#include "include/sec_vote_event.h"
#include "include/sb_adc.h"

struct sec_bat_eng_data {
	/* batt_charge_test */
	unsigned int charge_test_input;
	unsigned int charge_test_fcc;
	unsigned int charge_test_topoff;
	unsigned int charge_test_fv;
};

struct sec_bat_eng_data *sec_bat_init_eng_data(struct device *dev)
{
	struct sec_bat_eng_data *eng_data;

	eng_data = devm_kzalloc(dev, sizeof(struct sec_bat_eng_data), GFP_KERNEL);
	if (!eng_data)
		return ERR_PTR(-ENOMEM);

	eng_data->charge_test_input = 0;
	eng_data->charge_test_fcc = 0;
	eng_data->charge_test_topoff = 0;
	eng_data->charge_test_fv = 0;

	return eng_data;
}

bool sec_bat_set_charge_test(struct sec_battery_info *battery)
{
	struct sec_bat_eng_data *eng_data = battery->eng_data;
	bool is_changed = false;

	if (IS_ERR_OR_NULL(eng_data)) return false;

	if (eng_data->charge_test_input > 0) {
		pr_info("%s : input charge test %d\n", __func__, eng_data->charge_test_input);
		sec_vote(battery->input_vote, VOTER_TEST, true, eng_data->charge_test_input);
		is_changed = true;
	}
	if (eng_data->charge_test_fcc > 0) {
		pr_info("%s : fcc charge test %d\n", __func__, eng_data->charge_test_fcc);
		sec_vote(battery->fcc_vote, VOTER_TEST, true, eng_data->charge_test_fcc);
		is_changed = true;
	}
	if (eng_data->charge_test_topoff > 0) {
		pr_info("%s : topoff charge test %d\n", __func__, eng_data->charge_test_topoff);
		sec_vote(battery->topoff_vote, VOTER_TEST, true, eng_data->charge_test_topoff);
		is_changed = true;
	}

	return is_changed;
}

static ssize_t sysfs_eng_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t sysfs_eng_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SYSFS_ENG_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sysfs_eng_show_attrs,					\
	.store = sysfs_eng_store_attrs,					\
}

static struct device_attribute sysfs_eng_attrs[] = {
	SYSFS_ENG_ATTR(batt_temp_test),
	SYSFS_ENG_ATTR(batt_charge_test),
};

enum {
	BATT_TEMP_TEST = 0,
	BATT_CHARGE_TEST,
};

static ssize_t sysfs_eng_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	struct sec_bat_eng_data *eng_data = battery->eng_data;
	const ptrdiff_t offset = attr - sysfs_eng_attrs;
	int i = 0;

	if (IS_ERR_OR_NULL(eng_data))
		return 0;

	switch (offset) {
	case BATT_TEMP_TEST:
	{
		int batt_temp, blk_temp;

		sb_adc_get_valuef(BATT_TEMP, &batt_temp);
		sb_adc_get_valuef(BLK_TEMP, &blk_temp);
		i += scnprintf(buf + i, PAGE_SIZE - i, "b:%d, k:%d\n", batt_temp, blk_temp);
	}
		break;
	case BATT_CHARGE_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "i:%d, f:%d, t:%d, v:%d\n",
			eng_data->charge_test_input,
			eng_data->charge_test_fcc,
			eng_data->charge_test_topoff,
			eng_data->charge_test_fv);
		break;

	default:
		return -EINVAL;
	}

	return i;
}

static bool check_forced_value(int value)
{
	return (value > -300 && value < 3000);
}

static ssize_t sysfs_eng_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	struct sec_bat_eng_data *eng_data = battery->eng_data;
	const ptrdiff_t offset = attr - sysfs_eng_attrs;
	int x = 0;

	if (IS_ERR_OR_NULL(eng_data))
		return count;

	switch (offset) {
	case BATT_TEMP_TEST:
	{
		char tc;

		if (sscanf(buf, "%c %10d\n", &tc, &x) == 2) {
			pr_info("%s : temperature t: %c, temp: %d\n", __func__, tc, x);

			switch (tc) {
			case 'b':
				sb_adc_set(BATT_TEMP, check_forced_value(x), 0, x);
				break;
			case 'k':
				sb_adc_set(BLK_TEMP, check_forced_value(x), 0, x);
				break;
			default:
				break;
			}
		}
	}
		break;
	case BATT_CHARGE_TEST:
	{
		char tc;

		if (sscanf(buf, "%c %10d\n", &tc, &x) == 2) {
			pr_info("%s : charge t: %c, value: %d\n", __func__, tc, x);
			if (tc == 'i')
				eng_data->charge_test_input = x;
			else if (tc == 'f')
				eng_data->charge_test_fcc = x;
			else if (tc == 't')
				eng_data->charge_test_topoff = x;
			else if (tc == 'v')
				eng_data->charge_test_fv = x;
			sec_bat_set_charge_test(battery);
		}
	}
		break;

	default:
		return -EINVAL;
	}

	return count;
}

static struct sec_sysfs sysfs_eng_list = {
	.attr = sysfs_eng_attrs,
	.size = ARRAY_SIZE(sysfs_eng_attrs),
};

static int __init sysfs_eng_init(void)
{
	add_sec_sysfs(&sysfs_eng_list.list);
	return 0;
}
late_initcall(sysfs_eng_init);
