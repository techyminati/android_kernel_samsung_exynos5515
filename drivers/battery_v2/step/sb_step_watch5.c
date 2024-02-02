/*
 *  sb_step_watch5.c
 *  Samsung Mobile Battery Step Charging for Watch5
 *
 *  Copyright (C) 2022 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>

#include "../include/sec_charging_common.h"
#include "../include/sec_battery.h"
#include "../include/sec_battery_vote.h"
#include "../include/sec_vote_event.h"
#include "../include/sb_wrl_def.h"
#include "../core/sb_notify.h"

#include "sb_step.h"
#include "sb_step_it.h"

#define SB_STEP_NAME		"watch5"

#define spt_status(status) (status == POWER_SUPPLY_STATUS_CHARGING)
#define spt_thermal(thermal) ((thermal != BAT_THERMAL_COOL1) && \
			(thermal != BAT_THERMAL_COOL2) && \
			(thermal != BAT_THERMAL_COOL3) && \
			(thermal != BAT_THERMAL_COLD))

#define DEFAULT_STEP		(-1)
#define DEFAULT_VOLT_GAP	25

enum {
	STEP_MODE_CC = 0,
	STEP_MODE_CC_ADJ,
	STEP_MODE_CV_CHECK,
	STEP_MODE_CV_PRESET,
	STEP_MODE_CV,
	STEP_MODE_CV_ADJ,
	STEP_MODE_CHANGE
};

static const char *get_step_mode_str(int mode)
{
	switch (mode) {
	case STEP_MODE_CC:
		return "CC";
	case STEP_MODE_CC_ADJ:
		return "CC_ADJ";
	case STEP_MODE_CV_CHECK:
		return "CV_CHECK";
	case STEP_MODE_CV_PRESET:
		return "CV_PRESET";
	case STEP_MODE_CV:
		return "CV";
	case STEP_MODE_CV_ADJ:
		return "CV_ADJ";
	case STEP_MODE_CHANGE:
		return "Change";
	}

	return "Unknown";
}

struct step_chg {
	/* Set Value */
	int fv;
	int fcc;
	union sb_wrl_vm cc;
	union sb_wrl_vm cv;

	/* Check Value */
	int cc_cur;
	int cv_cur;
};

struct sb_step_watch5 {
	struct notifier_block	nb;

	int step;
	int mode;

	/* dt data */
	char *wrl_name;

	struct step_chg *st_table;
	unsigned int st_cnt;
};

static void set_vote_by_vm(struct sb_step_watch5 *st, union sb_wrl_vm *vm, bool en)
{
	if ((vm == NULL) || (vm->base.type >= SB_WRL_VM_MAX))
		return;

	switch (vm->base.type) {
	case SB_WRL_VM_FX:
		sec_votef(VN_WPC_LOW_VOUT, VOTER_STEP, false, 0);
		sec_votef(VN_WPC_VOUT, VOTER_STEP, en, vm->fx.vout);
		sec_votef(VN_WPC_FX_HDR, VOTER_STEP, en, vm->fx.hdr);
		break;
	case SB_WRL_VM_DC:
		sec_votef(VN_WPC_LOW_VOUT, VOTER_STEP, false, 0);
		sec_votef(VN_WPC_DC_HDR, VOTER_STEP, en, vm->dc.hdr);
		sec_votef(VN_WPC_ICHG, VOTER_STEP, en, vm->dc.ichg);
		sec_votef(VN_WPC_DC_HIGH_VOUT, VOTER_STEP, en, vm->dc.high_vout);
		break;
	case SB_WRL_VM_BT:
		sec_votef(VN_WPC_LOW_VOUT, VOTER_STEP, en, vm->bt.low_vout);
		sec_votef(VN_WPC_BT_VBAT_HDR, VOTER_STEP, en, vm->bt.vbat_hdr);
		sec_votef(VN_WPC_BT_HDR, VOTER_STEP, en, vm->bt.hdr);
		break;
	}

	sec_votef(VN_WPC_VM, VOTER_STEP, en, vm->base.type);
}

static int cb_watch5_init(void *pdata)
{
	struct sb_step_watch5 *st = pdata;

	if (!st)
		return -EINVAL;

	step_log("%s - step = %d\n",
		(st->step > DEFAULT_STEP) ? "refresh" : "init", st->step);

	st->step = DEFAULT_STEP;
	st->mode = STEP_MODE_CC;
	sec_votef(VN_FCC, VOTER_STEP, false, 0);
	sec_votef(VN_FV, VOTER_STEP, false, 0);

	/* Vout Mode vote */
	sec_votef(VN_WPC_VM, VOTER_STEP, false, 0);
	/* DC vote */
	sec_votef(VN_WPC_DC_HDR, VOTER_STEP, false, 0);
	sec_votef(VN_WPC_ICHG, VOTER_STEP, false, 0);
	sec_votef(VN_WPC_DC_HIGH_VOUT, VOTER_STEP, false, 0);
	/* BT vote */
	sec_votef(VN_WPC_LOW_VOUT, VOTER_STEP, false, 0);
	sec_votef(VN_WPC_BT_VBAT_HDR, VOTER_STEP, false, 0);
	sec_votef(VN_WPC_BT_HDR, VOTER_STEP, false, 0);

	return 0;
}

static int parse(struct device_node *np, struct sb_step_watch5 *st)
{
#if defined(CONFIG_OF)
	int ret, cnt;

	ret = sb_of_parse_str(np, st, wrl_name);
	if (ret)
		return -EINVAL;

	cnt = of_property_count_u32_elems(np, "st_table");
	if (cnt > 0) {
		int temp_cnt =
			(cnt * sizeof(unsigned int)) / sizeof(struct step_chg);

		st->st_table = kcalloc(temp_cnt, sizeof(struct step_chg), GFP_KERNEL);
		if (!st->st_table)
			return -ENOMEM;

		ret = of_property_read_u32_array(np, "st_table",
			(u32 *)st->st_table, cnt);
		if (ret) {
			kfree(st->st_table);
			return -EINVAL;
		}

		st->st_cnt = temp_cnt;
		step_log("st_table - count (%d)\n", temp_cnt);
	}
#endif
	return 0;
}

static int sb_noti_handler(struct notifier_block *nb, unsigned long action, void *data)
{
/*
	struct sb_step_watch5 *st = container_of(nb, struct sb_step_watch5, nb);
	sb_data *sbd = data;
*/
	return 0;
}

static void *cb_watch5_create(struct device_node *np)
{
	struct sb_step_watch5 *st;
	int ret = 0;

	st = kzalloc(sizeof(struct sb_step_watch5), GFP_KERNEL);
	if (!st) {
		ret = -ENOMEM;
		goto failed_alloc;
	}

	ret = parse(np, st);
	if (ret)
		goto failed_parse;

	sb_notify_register(&st->nb,
		sb_noti_handler, "sb-step-watch5", SB_DEV_MODULE);

	cb_watch5_init(st);
	step_log("finished\n");
	return st;

failed_parse:
	kfree(st);
failed_alloc:
	return ERR_PTR(ret);
}

static int check_step_volt(struct step_chg *table, unsigned int size, unsigned int volt)
{
	int idx = 0;

	while ((idx < size - 1) &&
		(table[idx].fv < volt))
		idx++;

	return idx;
}

static void set_step(struct sb_step_watch5 *st, int step)
{
	struct step_chg *stc = NULL;

	if ((step < 0) || (st->st_cnt <= step))
		return;

	stc = &st->st_table[step];

	/* set vm vote */
	set_vote_by_vm(st, &stc->cc, true);

	/* set chg vote */
	sec_votef(VN_FV, VOTER_STEP, true, stc->fv);
	sec_votef(VN_FCC, VOTER_STEP, true, stc->fcc);

	st->step = step;
	st->mode = STEP_MODE_CC;
}

static int cb_watch5_monitor(void *pdata, struct sb_step_data data)
{
	union power_supply_propval value = { 0, };
	struct sb_step_watch5 *st = pdata;
	struct step_chg *stc = NULL;
	int old_mode = STEP_MODE_CC;

	if (!st)
		return -EINVAL;

	if (!spt_status(data.status) ||
		!spt_thermal(data.thermal)) {
		cb_watch5_init(st);
		return 0;
	}

	old_mode = st->mode;
	/* check PHM */
	psy_do_property(st->wrl_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_PHM, value);
	if (value.intval) {
		step_log("keep step(%d) because of phm\n", st->step);
		goto step_end;
	}

	if (st->step == DEFAULT_STEP) {
		/* set new CC mode */
		set_step(st, check_step_volt(st->st_table, st->st_cnt, data.volt_avg));
		goto step_end;
	}

	stc = &st->st_table[st->step];
	switch (st->mode) {
	case STEP_MODE_CC:
	case STEP_MODE_CC_ADJ:
		if (stc->fv - DEFAULT_VOLT_GAP > data.volt_now) {
			st->mode = STEP_MODE_CC;
			goto step_end;
		}

		psy_do_property(st->wrl_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		st->mode = (stc->cc_cur > value.intval) ?
			(st->mode + 1) : STEP_MODE_CC;
		break;
	case STEP_MODE_CV_CHECK:
		/* clear old vm vote */
		set_vote_by_vm(st, &stc->cc, false);

		/* check iout */
		psy_do_property(st->wrl_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		if (stc->cv_cur > value.intval) {
			/* set vm vote to next cc */
			set_step(st, st->step + 1);
			break;
		}
	case STEP_MODE_CV_PRESET:
		/* set vm vote to cv */
		set_vote_by_vm(st, &stc->cv, true);
		st->mode = STEP_MODE_CV;
		break;
	case STEP_MODE_CV:
	case STEP_MODE_CV_ADJ:
		psy_do_property(st->wrl_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		st->mode = (stc->cv_cur > value.intval) ?
			(st->mode + 1) : STEP_MODE_CV;
		break;
	case STEP_MODE_CHANGE:
		/* clear old vm vote */
		set_vote_by_vm(st, &stc->cv, false);
		/* set vm vote to next cc */
		set_step(st, st->step + 1);
		break;
	default:
		break;
	}

step_end:
	step_log("Step = %d, Mode = %s --> %s\n",
		st->step, get_step_mode_str(old_mode), get_step_mode_str(st->mode));
	return 0;
}

static struct sb_step_it watch5_it = {
	.name = SB_STEP_NAME,
	.cb_create = cb_watch5_create,
	.cb_init = cb_watch5_init,
	.cb_monitor = cb_watch5_monitor
};

static int __init st_watch5_init(void)
{
	return add_sb_step_it(&watch5_it);
}
early_initcall(st_watch5_init);
