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
#include "../include/sec_battery_vote.h"
#include "../include/sec_vote_event.h"

#include "sb_step.h"
#include "sb_step_it.h"

#define SB_STEP_NAME		"base"

struct sb_step_base {
	unsigned int cc2_voltage;
	unsigned int cc2_current;
};

static int cb_base_init(void *pdata)
{
	struct sb_step_base *st = pdata;

	if (!st)
		return -EINVAL;

	sec_votef(VN_FCC, VOTER_STEP, false, 0);
	return 0;
}

static int parse(struct device_node *np, struct sb_step_base *st)
{
#if defined(CONFIG_OF)
	sb_of_parse_u32(np, st, cc2_voltage, 4120);
	sb_of_parse_u32(np, st, cc2_current, 169);
#endif
	return 0;
}

static void *cb_base_create(struct device_node *np)
{
	struct sb_step_base *st;
	int ret = 0;

	st = kzalloc(sizeof(struct sb_step_base), GFP_KERNEL);
	if (!st) {
		ret = -ENOMEM;
		goto failed_alloc;
	}

	ret = parse(np, st);
	if (ret)
		goto failed_parse;

	cb_base_init(st);
	step_log("finished\n");
	return st;

failed_parse:
	kfree(st);
failed_alloc:
	return ERR_PTR(ret);
}

static int cb_base_monitor(void *pdata, struct sb_step_data data)
{
	struct sb_step_base *st = pdata;

	if (!st)
		return -EINVAL;

	if (data.status == POWER_SUPPLY_STATUS_DISCHARGING) {
		cb_base_init(st);
		return 0;
	}

	if (data.volt_avg > st->cc2_voltage)
		sec_votef(VN_FCC, VOTER_STEP, true, st->cc2_current);

	step_log("volt = %d, ct = %d, status = %d\n",
		data.volt_avg, data.cable_type, data.status);
	return 0;
}

static struct sb_step_it base_it = {
	.name = SB_STEP_NAME,
	.cb_create = cb_base_create,
	.cb_init = cb_base_init,
	.cb_monitor = cb_base_monitor
};

static int __init st_base_init(void)
{
	return add_sb_step_it(&base_it);
}
early_initcall(st_base_init);
