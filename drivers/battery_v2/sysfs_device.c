/*
 *  sysfs_device.c
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
#include "include/sec_vote_event.h"
#include "include/sb_wrl_def.h"
#include "include/sb_wrl_fw.h"
#include "include/sb_adc.h"

enum {
	EXT_DEV_NONE = 0,
	EXT_DEV_GAMEPAD_CHG,
	EXT_DEV_GAMEPAD_OTG,
};

static ssize_t sysfs_device_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t sysfs_device_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SYSFS_DEVICE_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sysfs_device_show_attrs,					\
	.store = sysfs_device_store_attrs,					\
}

static struct device_attribute sysfs_device_attrs[] = {
	SYSFS_DEVICE_ATTR(batt_capacity_max),
	SYSFS_DEVICE_ATTR(fg_capacity),
	SYSFS_DEVICE_ATTR(fg_asoc),
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	SYSFS_DEVICE_ATTR(fg_cycle),
	SYSFS_DEVICE_ATTR(fg_full_voltage),
	SYSFS_DEVICE_ATTR(fg_fullcapnom),
	SYSFS_DEVICE_ATTR(battery_cycle),
#endif

	SYSFS_DEVICE_ATTR(wc_status),
	SYSFS_DEVICE_ATTR(wc_enable),
	SYSFS_DEVICE_ATTR(wc_control),
	SYSFS_DEVICE_ATTR(wc_control_cnt),
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	SYSFS_DEVICE_ATTR(batt_wireless_firmware_update),
	SYSFS_DEVICE_ATTR(otp_firmware_result),
	SYSFS_DEVICE_ATTR(otp_firmware_ver_bin),
	SYSFS_DEVICE_ATTR(otp_firmware_ver),
#endif
	SYSFS_DEVICE_ATTR(wc_vout),
	SYSFS_DEVICE_ATTR(wc_vrect),
	SYSFS_DEVICE_ATTR(wc_tx_id),
	SYSFS_DEVICE_ATTR(wc_op_freq),
	SYSFS_DEVICE_ATTR(wc_op_freq_strength),
	SYSFS_DEVICE_ATTR(wc_cmd_info),
	SYSFS_DEVICE_ATTR(wc_tx_mfc_iin_from_uno),
	SYSFS_DEVICE_ATTR(batt_inbat_wireless_cs100),
	SYSFS_DEVICE_ATTR(battery_dump),
	SYSFS_DEVICE_ATTR(wc_auth_adt_sent),
};

enum {
	BATT_CAPACITY_MAX = 0,
	FG_CAPACITY,
	FG_ASOC,
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	FG_CYCLE,
	FG_FULL_VOLTAGE,
	FG_FULLCAPNOM,
	BATTERY_CYCLE,
#endif

	WC_STATUS,
	WC_ENABLE,
	WC_CONTROL,
	WC_CONTROL_CNT,
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	BATT_WIRELESS_FIRMWARE_UPDATE,
	OTP_FIRMWARE_RESULT,
	OTP_FIRMWARE_VER_BIN,
	OTP_FIRMWARE_VER,
#endif
	WC_VOUT,
	WC_VRECT,
	WC_TX_ID,
	WC_OP_FREQ,
	WC_OP_FREQ_STRENGTH,
	WC_CMD_INFO,
	WC_TX_MFC_IIN_FROM_UNO,
	BATT_INBAT_WIRELESS_CS100,
	BATTERY_DUMP,
	WC_AUTH_ADT_SENT,
};

static ssize_t sysfs_device_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_device_attrs;
	union power_supply_propval value = {0, };
	int i = 0;

	switch (offset) {
	case BATT_CAPACITY_MAX:
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case FG_CAPACITY:
		value.intval = SEC_BATTERY_CAPACITY_DESIGNED;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ", value.intval);

		value.intval = SEC_BATTERY_CAPACITY_ABSOLUTE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ", value.intval);

		value.intval = SEC_BATTERY_CAPACITY_TEMPERARY;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ", value.intval);

		value.intval = SEC_BATTERY_CAPACITY_CURRENT;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x\n", value.intval);
		break;
	case FG_ASOC:
	{
		int ret = 0;

		ret = psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_FULL, value);
		if (ret < 0)
			value.intval = -1;

		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
	}
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case FG_CYCLE:
		value.intval = SEC_BATTERY_CAPACITY_CYCLE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		value.intval = value.intval / 100;
		dev_info(battery->dev, "fg cycle(%d)\n", value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case FG_FULL_VOLTAGE:
	{
		int recharging_voltage = battery->pdata->recharge_condition_vcell;

		if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)
			recharging_voltage = battery->pdata->swelling_high_rechg_voltage;
		else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE)
			recharging_voltage = battery->pdata->swelling_low_rechg_voltage;

		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d\n",
			value.intval, recharging_voltage);
		break;
	}
	case FG_FULLCAPNOM:
		value.intval = SEC_BATTERY_CAPACITY_AGEDCELL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATTERY_CYCLE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->batt_cycle);
		break;
#endif

	case WC_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			is_wireless_type(battery->cable_type) ? 1: 0);
		break;
	case WC_ENABLE:
	case WC_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->wc_enable);
		break;
	case WC_CONTROL_CNT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->wc_enable_cnt_value);
		break;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	case BATT_WIRELESS_FIRMWARE_UPDATE:
		value.intval = SB_WRL_OTP_FIRM_VERIFY;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		pr_info("%s RX firmware verify. result: %d\n", __func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case OTP_FIRMWARE_RESULT:
		value.intval = SB_WRL_OTP_FIRM_RESULT;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case OTP_FIRMWARE_VER_BIN:
		value.intval = SB_WRL_OTP_FIRM_VER_BIN;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case OTP_FIRMWARE_VER:
		value.intval = SB_WRL_OTP_FIRM_VER;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
#endif

	case WC_VOUT:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_VRECT:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_ENERGY_AVG, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_TX_ID:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);
		pr_info("%s TX ID (%d)",__func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_OP_FREQ:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_OP_FREQ_STRENGTH:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ_STRENGTH, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_CMD_INFO:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_CMD, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x ", value.intval);

		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VAL, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x\n", value.intval);
		break;
	case WC_TX_MFC_IIN_FROM_UNO:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATT_INBAT_WIRELESS_CS100:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_STATUS, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATTERY_DUMP:
		{
			char temp_buf[1024] = {0,};
			int size = 1024;
			int batt_temp = 0, blk_temp = 0;

			sb_adc_get_valuef(BATT_TEMP, &batt_temp);
			sb_adc_get_valuef(BLK_TEMP, &blk_temp);
			snprintf(temp_buf+strlen(temp_buf), size, "%d,%d,%d,%d,%d,%d,%d,"
				"%s,%s,%s,%s,%d,%s,%d,%d,%lu,"
#if defined(CONFIG_BATTERY_AGE_FORECAST)
				"%d,"
#endif
				"%d,",
				battery->voltage_now, battery->current_now,
				battery->current_max, battery->charging_current,
				battery->capacity, batt_temp, blk_temp,
				sb_get_bst_str(battery->status),
				sb_get_cm_str(battery->charging_mode),
				sb_get_hl_str(battery->health),
				sb_get_ct_str(battery->cable_type),
				battery->muic_cable_type,
				sb_get_tz_str(battery->thermal_zone),
				is_slate_mode(battery),
				battery->store_mode,
				(battery->expired_time / 1000)
#if defined(CONFIG_BATTERY_AGE_FORECAST)
				, battery->batt_cycle
#endif
				, lpcharge);
			size = sizeof(temp_buf) - strlen(temp_buf);

			psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN, value);
			snprintf(temp_buf+strlen(temp_buf), size, "%s", value.strval);
			size = sizeof(temp_buf) - strlen(temp_buf);

			psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN, value);
			snprintf(temp_buf+strlen(temp_buf), size, "%s", value.strval);
			
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case WC_AUTH_ADT_SENT:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", value.strval);
		break;
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t sysfs_device_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sysfs_device_attrs;
	union power_supply_propval value = {0, };
	int x = 0;

	switch (offset) {
	case BATT_CAPACITY_MAX:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_err(battery->dev,
					"%s: BATT_CAPACITY_MAX(%d), fg_reset(%d)\n", __func__, x, fg_reset);
			if (!fg_reset && !battery->store_mode) {
				value.intval = x;
				psy_do_property(battery->pdata->fuelgauge_name, set,
						POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN, value);

				/* update soc */
				value.intval = 0;
				psy_do_property(battery->pdata->fuelgauge_name, get,
						POWER_SUPPLY_PROP_CAPACITY, value);
				battery->capacity = value.intval;
			} else {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				battery->fg_reset = 1;
#endif
			}
		}
		break;
	case FG_CAPACITY:
	case FG_ASOC:
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case FG_CYCLE:
	case FG_FULL_VOLTAGE:
	case FG_FULLCAPNOM:
		break;
	case BATTERY_CYCLE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev, "%s: BATTERY_CYCLE(%d)\n", __func__, x);
			if (x >= 0) {
				int prev_battery_cycle = battery->batt_cycle;

				battery->batt_cycle = x;
#if defined(CONFIG_BATTERY_CISD)
				battery->cisd.data[CISD_DATA_CYCLE] = x;
#endif
				psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_EXT_PROP_LONG_LIFE_STEP, value);
				if ((value.intval >= 0) &&
					(value.intval < battery->pdata->num_age_step)) {
					battery->pdata->age_step = value.intval;
				}

				if (prev_battery_cycle < 0)
					sec_bat_aging_check(battery);
			}
		}
		break;
#endif

	case WC_STATUS:
		break;
	case WC_ENABLE:
	case WC_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->wc_enable = !!(x);
			battery->wc_enable_cnt = 0;
			sec_votef(VN_WPC_EN, VOTER_WPC_CONTROL,
				!battery->wc_enable, battery->wc_enable);
		}
		break;
	case WC_CONTROL_CNT:
		if (sscanf(buf, "%10d\n", &x) == 1)
			battery->wc_enable_cnt_value = x;

		break;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	case BATT_WIRELESS_FIRMWARE_UPDATE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_MANUFACTURER, value);
		}
		break;
	case OTP_FIRMWARE_RESULT:
	case OTP_FIRMWARE_VER_BIN:
	case OTP_FIRMWARE_VER:
		break;
#endif

	case WC_VOUT:
	case WC_VRECT:
	case WC_TX_ID:
	case WC_OP_FREQ:
	case WC_OP_FREQ_STRENGTH:
	case WC_CMD_INFO:
	case WC_TX_MFC_IIN_FROM_UNO:
		break;
	case BATT_INBAT_WIRELESS_CS100:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: send cs100 command\n", __func__);
			value.intval = POWER_SUPPLY_STATUS_FULL;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_STATUS, value);
		}
		break;
	case BATTERY_DUMP:
		break;
	case WC_AUTH_ADT_SENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: adt test send(%d)\n", __func__, x);
			value.intval = !!(x);
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
		}
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static struct sec_sysfs sysfs_device_list = {
	.attr = sysfs_device_attrs,
	.size = ARRAY_SIZE(sysfs_device_attrs),
};

static int __init sysfs_device_init(void)
{
	add_sec_sysfs(&sysfs_device_list.list);
	return 0;
}
late_initcall(sysfs_device_init);
