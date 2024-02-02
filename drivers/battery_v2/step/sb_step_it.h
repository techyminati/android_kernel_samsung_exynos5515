/*
 * sb_step_it.h
 * Samsung Mobile Battery Step Charging Interface Header
 *
 * Copyright (C) 2022 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SB_STEP_IT_H
#define __SB_STEP_IT_H __FILE__

#define SB_STEP_OF_NAME		"sb-step"
#define SB_STEP_OF_IT_NAME	"it-name"

#define step_log(str, ...) pr_info("[SB-STEP-%s]:%s: "str, SB_STEP_NAME, __func__, ##__VA_ARGS__)

struct device_node;

typedef void* (*it_create)(struct device_node *np);
typedef int (*it_init)(void *pdata);
typedef int (*it_monitor)(void *pdata, struct sb_step_data data);

struct list_head;
struct sb_step_it {
	struct list_head list;

	const char *name;
	it_create	cb_create;
	it_init		cb_init;
	it_monitor	cb_monitor;
};

int add_sb_step_it(struct sb_step_it *it);

#endif /* __SB_STEP_IT_H */

