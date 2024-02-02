/*
 * sec_battery_thermal.c
 * Samsung Mobile Battery Driver
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "include/sec_battery.h"
#include "include/sec_vote_event.h"
#include "include/sb_wrl_def.h"
#include "include/sb_adc.h"

#define THERMAL_HYSTERESIS_2	19
#define THERMAL_HYSTERESIS_3	29
#define THERMAL_HYSTERESIS_5	49

struct bat_temp_limit {
	const char *adc;
	int ept_temp;
	int high_temp;
	int recv_temp;
	unsigned int fcc;

	unsigned int cstep_cnt;
	unsigned int *cstep_table;
};

char *sec_bat_thermal_zone[] = {
	"COLD",
	"COOL3",
	"COOL2",
	"COOL1",
	"NORMAL",
	"WARM",
	"OVERHEAT",
	"OVERHEATLIMIT",
};

#if !defined(CONFIG_SEC_FACTORY)
enum {
	SB_THM_BAT_LIMIT_NONE = 0,
	SB_THM_BAT_LIMIT_CHG,
	SB_THM_BAT_LIMIT_EPT
};

static int sec_bat_check_limit(struct bat_temp_limit *btl, int chg_limit, int temp)
{
	switch (chg_limit) {
	case SB_THM_BAT_LIMIT_EPT:
		if (btl->high_temp >= temp)
			return sec_bat_check_limit(btl, SB_THM_BAT_LIMIT_CHG, temp);
		break;
	case SB_THM_BAT_LIMIT_CHG:
		if (btl->recv_temp >= temp)
			return SB_THM_BAT_LIMIT_NONE;
		if (btl->ept_temp <= temp)
			return SB_THM_BAT_LIMIT_EPT;
		break;
	case SB_THM_BAT_LIMIT_NONE:
	default:
		if (btl->ept_temp <= temp)
			return SB_THM_BAT_LIMIT_EPT;
		if (btl->high_temp <= temp)
			return SB_THM_BAT_LIMIT_CHG;
		break;
	}

	return chg_limit;
}

void sec_bat_check_bat_temp_limit(struct sec_battery_info *battery)
{
	struct bat_temp_limit *btl = NULL;
	int temp = 0;

	if (battery->pdata->temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return;

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL)
		return;

	btl = battery->pdata->charging_current[battery->cable_type].btl;
	if (btl == NULL)
		return;

	sb_adc_get_valuef(btl->adc, &temp);
	battery->chg_limit = sec_bat_check_limit(btl, battery->chg_limit, temp);
	switch (battery->chg_limit) {
	case SB_THM_BAT_LIMIT_EPT:
		sec_votef(VN_WPC_PHM, VOTER_HEAT_CONTROL, true, WPC_PHM_EPT_ON);
		break;
	case SB_THM_BAT_LIMIT_CHG:
		sec_votef(VN_WPC_PHM, VOTER_HEAT_CONTROL, false, WPC_PHM_OFF);
		sec_vote(battery->fcc_vote, VOTER_HEAT_CONTROL, true, btl->fcc);

		if ((btl->cstep_cnt > 0) &&
			(battery->cstep_applied == battery->cstep))
			battery->cstep = min(battery->cstep + 1, btl->cstep_cnt - 1);
		break;
	case SB_THM_BAT_LIMIT_NONE:
	default:
		sec_votef(VN_WPC_PHM, VOTER_HEAT_CONTROL, false, WPC_PHM_OFF);
		sec_vote(battery->fcc_vote, VOTER_HEAT_CONTROL, false, 0);
		if ((btl->cstep_cnt > 0) && (battery->cstep > 0)) {
			pr_info("%s: step_curr(%d)\n",
				__func__, btl->cstep_table[battery->cstep]);
			sec_vote(battery->fcc_vote, VOTER_CSTEP, true,
				btl->cstep_table[battery->cstep]);
			battery->cstep_applied = battery->cstep;
		}
		break;
	}

	pr_info("%s: step->step applied (%d->%d), curr(%d), chg_limit(%s)\n",
		__func__, battery->cstep, battery->cstep_applied,
		battery->charging_current, GET_BOOL_STR(battery->chg_limit));
}
#else
void sec_bat_check_bat_temp_limit(struct sec_battery_info *battery)
{
}
#endif

void sec_bat_set_threshold(struct sec_battery_info *battery)
{
	if (is_wired_type(battery->cable_type)) {
		battery->cold_cool3_thresh = battery->pdata->wire_cold_cool3_thresh;
		battery->cool3_cool2_thresh = battery->pdata->wire_cool3_cool2_thresh;
		battery->cool2_cool1_thresh = battery->pdata->wire_cool2_cool1_thresh;
		battery->cool1_normal_thresh = battery->pdata->wire_cool1_normal_thresh;
		battery->normal_warm_thresh = battery->pdata->wire_normal_warm_thresh;
		battery->warm_overheat_thresh = battery->pdata->wire_warm_overheat_thresh;
	} else {
		battery->cold_cool3_thresh = battery->pdata->wireless_cold_cool3_thresh;
		battery->cool3_cool2_thresh = battery->pdata->wireless_cool3_cool2_thresh;
		battery->cool2_cool1_thresh = battery->pdata->wireless_cool2_cool1_thresh;
		battery->cool1_normal_thresh = battery->pdata->wireless_cool1_normal_thresh;
		battery->normal_warm_thresh = battery->pdata->wireless_normal_warm_thresh;
		battery->warm_overheat_thresh = battery->pdata->wireless_warm_overheat_thresh;
	}

	switch (battery->thermal_zone) {
	case BAT_THERMAL_OVERHEAT:
		battery->warm_overheat_thresh = battery->pdata->warm_recov_thresh;
		battery->normal_warm_thresh = battery->pdata->warm_recov_thresh;
		break;
	case BAT_THERMAL_WARM:
		if (battery->cable_type == SB_CBL_D2D_WRL)
			battery->normal_warm_thresh = battery->pdata->warm_recov_thresh - 10;
		else
			battery->normal_warm_thresh = battery->pdata->warm_recov_thresh;
		break;
	case BAT_THERMAL_COOL1:
		battery->cool1_normal_thresh += THERMAL_HYSTERESIS_2;
		break;
	case BAT_THERMAL_COOL2:
		battery->cool2_cool1_thresh += THERMAL_HYSTERESIS_2;
		battery->cool1_normal_thresh += THERMAL_HYSTERESIS_2;
		break;
	case BAT_THERMAL_COOL3:
		battery->cool3_cool2_thresh += THERMAL_HYSTERESIS_2;
		battery->cool2_cool1_thresh += THERMAL_HYSTERESIS_2;
		battery->cool1_normal_thresh += THERMAL_HYSTERESIS_2;
		break;
	case BAT_THERMAL_COLD:
		battery->cold_cool3_thresh += THERMAL_HYSTERESIS_2;
		battery->cool3_cool2_thresh += THERMAL_HYSTERESIS_2;
		battery->cool2_cool1_thresh += THERMAL_HYSTERESIS_2;
		battery->cool1_normal_thresh += THERMAL_HYSTERESIS_2;
		break;
	case BAT_THERMAL_NORMAL:
	default:
		break;
	}
}

void sec_bat_thermal_check(struct sec_battery_info *battery)
{
	int pre_thermal_zone = battery->thermal_zone;
	int voter_swelling_status = SEC_BAT_CHG_MODE_CHARGING;
	int bat_thm;

	sb_adc_get_valuef(BATT_TEMP, &bat_thm);
	pr_err("%s: co_c3: %d, c3_c2: %d, c2_c1: %d, c1_no: %d, no_wa: %d, wa_ov: %d, tz(%s)\n", __func__,
			battery->cold_cool3_thresh, battery->cool3_cool2_thresh, battery->cool2_cool1_thresh,
			battery->cool1_normal_thresh, battery->normal_warm_thresh, battery->warm_overheat_thresh,
			sec_bat_thermal_zone[battery->thermal_zone]);

	if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING ||
		battery->skip_swelling) {
		battery->health_change = false;
		pr_debug("%s: DISCHARGING or 15 test mode. stop thermal check\n", __func__);
		battery->thermal_zone = BAT_THERMAL_NORMAL;
		sec_vote(battery->topoff_vote, VOTER_SWELLING, false, 0);
		sec_vote(battery->fcc_vote, VOTER_SWELLING, false, 0);
		sec_vote(battery->fv_vote, VOTER_SWELLING, false, 0);
		sec_vote(battery->chgen_vote, VOTER_SWELLING, false, 0);
		sec_votef(VN_WPC_PHM, VOTER_SWELLING, false, WPC_PHM_OFF);
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		sec_bat_set_threshold(battery);
		return;
	}

	if (battery->pdata->temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE) {
		pr_err("%s: BAT_THM, Invalid Temp Check Type\n", __func__);
		return;
	} else {
		/* COLD - COOL3 - COOL2 - COOL1 - NORMAL - WARM - OVERHEAT - OVERHEATLIMIT*/
		/* temporary setting for usb cable case*/
		if (bat_thm >= battery->normal_warm_thresh) {
			if (bat_thm >= battery->warm_overheat_thresh) {
				battery->thermal_zone = BAT_THERMAL_OVERHEAT;
			} else {
				battery->thermal_zone = BAT_THERMAL_WARM;
			}
		} else if (bat_thm <= battery->cool1_normal_thresh) {
			if (bat_thm <= battery->cold_cool3_thresh) {
				battery->thermal_zone = BAT_THERMAL_COLD;
			} else if (bat_thm <= battery->cool3_cool2_thresh) {
				battery->thermal_zone = BAT_THERMAL_COOL3;
			} else if (bat_thm <= battery->cool2_cool1_thresh) {
				battery->thermal_zone = BAT_THERMAL_COOL2;
			} else {
				battery->thermal_zone = BAT_THERMAL_COOL1;
			}
		} else {
			battery->thermal_zone = BAT_THERMAL_NORMAL;
		}
	}

	if (pre_thermal_zone != battery->thermal_zone) {
		battery->bat_thm_count++;

		if (battery->bat_thm_count < battery->pdata->temp_check_count) {
			pr_info("%s : bat_thm_count %d/%d\n", __func__,
					battery->bat_thm_count, battery->pdata->temp_check_count);
			battery->thermal_zone = pre_thermal_zone;
			return;
		}

		pr_info("%s: thermal zone update (%s -> %s), bat_thm(%d)\n", __func__,
				sec_bat_thermal_zone[pre_thermal_zone],
				sec_bat_thermal_zone[battery->thermal_zone], bat_thm);
		battery->health_change = true;
		battery->bat_thm_count = 0;

		sec_bat_set_threshold(battery);
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SWELLING_MODE);

		switch (battery->thermal_zone) {
		case BAT_THERMAL_OVERHEAT:
			sec_votef(VN_WPC_PHM, VOTER_SWELLING, true, WPC_PHM_EPT_ON);
			if (battery->voltage_now > battery->pdata->high_temp_float)
				sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_BUCK_OFF);
			else
				sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

			battery->health = POWER_SUPPLY_HEALTH_OVERHEAT;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_UNSAFETY_TEMPERATURE]++;
			battery->cisd.data[CISD_DATA_UNSAFE_TEMPERATURE_PER_DAY]++;
#endif
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			break;
		case BAT_THERMAL_WARM:
			sec_votef(VN_WPC_PHM, VOTER_SWELLING, true, WPC_PHM_ON);
			if (is_wired_type(battery->cable_type)) {
				if (battery->voltage_now > battery->pdata->high_temp_float)
					sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_BUCK_OFF);

				else if (battery->voltage_now > battery->pdata->swelling_high_rechg_voltage)
					sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				else
					sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING);
			} else
				sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

			if (is_wireless_type(battery->cable_type))
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wireless_warm_current);
			else 
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wire_warm_current);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_HIGH_TEMP_SWELLING]++;
			battery->cisd.data[CISD_DATA_HIGH_TEMP_SWELLING_PER_DAY]++;
#endif
			sec_vote(battery->fv_vote, VOTER_SWELLING, true, battery->pdata->high_temp_float);
			sec_vote(battery->topoff_vote, VOTER_SWELLING, true, battery->pdata->high_temp_topoff);

			battery->health = POWER_SUPPLY_HEALTH_GOOD;
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			break;
		case BAT_THERMAL_COOL1:
			if (is_wireless_type(battery->cable_type)) {
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wireless_cool1_current);
			} else {
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wire_cool1_current);
			}
			sec_vote(battery->fv_vote, VOTER_SWELLING, true, battery->pdata->low_temp_float);
			sec_vote(battery->topoff_vote, VOTER_SWELLING, false, 0);
			sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING);
			sec_votef(VN_WPC_PHM, VOTER_SWELLING, false, WPC_PHM_OFF);
			battery->health = POWER_SUPPLY_HEALTH_GOOD;
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			break;
		case BAT_THERMAL_COOL2:
			if (is_wireless_type(battery->cable_type)) {
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wireless_cool2_current);
			} else {
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wire_cool2_current);
			}
			sec_vote(battery->fv_vote, VOTER_SWELLING, true, battery->pdata->low_temp_float);
			sec_vote(battery->topoff_vote, VOTER_SWELLING, false, 0);
			sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING);
			sec_votef(VN_WPC_PHM, VOTER_SWELLING, false, WPC_PHM_OFF);
			battery->health = POWER_SUPPLY_HEALTH_GOOD;
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			break;
		case BAT_THERMAL_COOL3:
			if (is_wireless_type(battery->cable_type)) {
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wireless_cool3_current);
			} else {
				sec_vote(battery->fcc_vote, VOTER_SWELLING, true, battery->pdata->wire_cool3_current);
			}
			sec_vote(battery->fv_vote, VOTER_SWELLING, true, battery->pdata->low_temp_float);
			sec_vote(battery->topoff_vote, VOTER_SWELLING, false, 0);
			sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING);
			sec_votef(VN_WPC_PHM, VOTER_SWELLING, false, WPC_PHM_OFF);
			battery->health = POWER_SUPPLY_HEALTH_GOOD;
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			break;
		case BAT_THERMAL_COLD:
			sec_votef(VN_WPC_PHM, VOTER_SWELLING, true, WPC_PHM_EPT_ON);
			sec_vote(battery->chgen_vote, VOTER_SWELLING, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			battery->health = POWER_SUPPLY_HEALTH_COLD;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_UNSAFETY_TEMPERATURE]++;
			battery->cisd.data[CISD_DATA_UNSAFE_TEMPERATURE_PER_DAY]++;
#endif
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			break;
		case BAT_THERMAL_NORMAL:
		default:
			sec_vote(battery->fcc_vote, VOTER_SWELLING, false, 0);
			sec_vote(battery->fv_vote, VOTER_SWELLING, false, 0);
			sec_vote(battery->topoff_vote, VOTER_SWELLING, false, 0);
			sec_vote(battery->chgen_vote, VOTER_SWELLING, false, 0);
			sec_votef(VN_WPC_PHM, VOTER_SWELLING, false, WPC_PHM_OFF);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_SWELLING_RECOVERY_CNT]++;
			battery->cisd.data[CISD_DATA_SWELLING_RECOVERY_CNT_PER_DAY]++;
#endif
			battery->health = POWER_SUPPLY_HEALTH_GOOD;
			break;
		}
		if ((battery->thermal_zone >= BAT_THERMAL_COOL3) && (battery->thermal_zone <= BAT_THERMAL_WARM)) {
#if defined(CONFIG_ENABLE_FULL_BY_SOC)
			if ((battery->capacity >= 100) || (battery->status == POWER_SUPPLY_STATUS_FULL))
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_FULL);
			else
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
#else
			if (battery->status != POWER_SUPPLY_STATUS_FULL)
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
#endif
		}
	} else { /* pre_thermal_zone == battery->thermal_zone */
		battery->health_change = false;

		switch (battery->thermal_zone) {
		case BAT_THERMAL_WARM:
			if (battery->health == POWER_SUPPLY_HEALTH_GOOD) {
				if (get_sec_voter_status(battery->chgen_vote,
						VOTER_SWELLING, &voter_swelling_status) < 0) {
					pr_err("%s: INVALID VOTER ID\n", __func__);
					break;
				}

				pr_info("%s: voter_swelling_status: %s\n", __func__,
					CHARGE_MODE_STRING[voter_swelling_status]);
				if (voter_swelling_status == SEC_BAT_CHG_MODE_CHARGING) {
					if (sec_bat_check_full(battery, battery->pdata->full_check_type)) {
						pr_info("%s: battery thermal zone WARM. Full charged.\n",
							__func__);
						sec_vote(battery->chgen_vote, VOTER_SWELLING, true,
								SEC_BAT_CHG_MODE_CHARGING_OFF);
					}
				} else if (is_wired_type(battery->cable_type) &&
						(battery->voltage_now <=
							battery->pdata->swelling_high_rechg_voltage)) {
					pr_info("%s: thermal zone WARM. charging recovery. Vnow: %d\n",
							__func__, battery->voltage_now);
					battery->expired_time = battery->pdata->expired_time;
					battery->prev_safety_time = 0;
					sec_vote(battery->fv_vote, VOTER_SWELLING, true,
							battery->pdata->high_temp_float);
					sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);
					sec_vote(battery->chgen_vote, VOTER_SWELLING, true,
							SEC_BAT_CHG_MODE_CHARGING);
				}
			}
			break;
		default:
			break;
		}
	}

	return;
}

#if defined(CONFIG_OF)
#define sb_of_read_u32(np, pdata, value) \
({ \
	int ret = 0; \
	ret = of_property_read_u32(np, #value, (unsigned int *)&pdata->value); \
	if (!ret) \
		pr_info("%s: %s - write "#value" to %d\n", __func__, np->name, pdata->value); \
	ret;\
})

struct bat_temp_limit *sec_bat_parse_bat_temp_limit(struct device_node *np)
{
	struct bat_temp_limit *btl = NULL;
	const struct property *prop;
	int clen = 0, ret = 0;

	if (!np)
		return ERR_PTR(-EINVAL);

	if (strcmp(np->name, "bat_temp_limit"))
		return NULL;

	btl = kzalloc(sizeof(struct bat_temp_limit), GFP_KERNEL);
	if (btl == NULL)
		return ERR_PTR(-ENOMEM);

	prop = of_find_property(np, "adc", NULL);
	if (prop && prop->value)
		btl->adc = prop->value;
	else
		btl->adc = BATT_TEMP;

	ret = sb_of_read_u32(np, btl, ept_temp);
	if (ret) {
		pr_info("%s: ept_temp is empty!\n", __func__);
		btl->ept_temp = 30000;
	}
	ret = sb_of_read_u32(np, btl, high_temp);
	if (ret)
		goto failed_to_get_limit;
	ret = sb_of_read_u32(np, btl, recv_temp);
	if (ret)
		goto failed_to_get_limit;
	ret = sb_of_read_u32(np, btl, fcc);
	if (ret)
		goto failed_to_get_limit;

	clen = of_property_count_u32_elems(np, "cstep");
	if (clen > 0) {
		btl->cstep_cnt = clen;
		pr_info("%s: cstep_cnt is %d\n", __func__, btl->cstep_cnt);

		btl->cstep_table = kcalloc(btl->cstep_cnt, sizeof(unsigned int), GFP_KERNEL);
		if (btl->cstep_table == NULL) {
			ret = -ENOMEM;
			goto failed_to_alloc_cstep_table;
		}
		if (of_property_read_u32_array(np, "cstep",
				(u32 *)btl->cstep_table, btl->cstep_cnt)) {
			ret = -EINVAL;
			goto failed_to_get_cstep;
		}
	}

	return btl;

failed_to_get_cstep:
	kfree(btl->cstep_table);
failed_to_alloc_cstep_table:
failed_to_get_limit:
	kfree(btl);
	return ERR_PTR(ret);
}
#else
struct bat_temp_limit *sec_bat_parse_bat_temp_limit(struct device_node *np)
{
	return NULL;
}
#endif

