/*
 *  sb_step.c
 *  Samsung Mobile Battery Step Charging
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

#include "sb_step.h"
#include "sb_step_it.h"

static LIST_HEAD(it_list);

struct sb_step {
	struct sb_step_it *it;

	void *pdata;
};

struct sb_step *sb_step_create(struct device_node *np)
{
	struct sb_step *st;
	struct sb_step_it *it;
	int ret = 0;

	if ((!np) || strcmp(np->name, SB_STEP_OF_NAME))
		return ERR_PTR(-EINVAL);

	list_for_each_entry(it, &it_list, list) {
		if ((of_property_match_string(np, SB_STEP_OF_IT_NAME, it->name) >= 0) &&
			(it->cb_create != NULL) &&
			(it->cb_init != NULL) &&
			(it->cb_monitor != NULL))
			goto find_it;
	}

	return ERR_PTR(-ENODEV);

find_it:
	st = kzalloc(sizeof(struct sb_step), GFP_KERNEL);
	if (!st)
		return ERR_PTR(-ENOMEM);

	st->it = it;
	st->pdata = it->cb_create(np);
	if (IS_ERR_OR_NULL(st->pdata)) {
		ret = PTR_ERR(st->pdata);
		kfree(st);
		return ERR_PTR(ret);
	}

	return st;
}

int sb_step_init(struct sb_step *st)
{
	if (!st)
		return -EINVAL;

	return st->it->cb_init(st->pdata);
}

int sb_step_monitor(struct sb_step *st, struct sb_step_data data)
{
	if (!st)
		return -EINVAL;

	return st->it->cb_monitor(st->pdata, data);
}

int add_sb_step_it(struct sb_step_it *it)
{
	if (!it)
		return -EINVAL;

	list_add(&it->list, &it_list);
	return 0;
}

