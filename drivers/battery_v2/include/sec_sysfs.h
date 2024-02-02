/*
 * sec_sysfs.h
 * Samsung Mobile sysfs Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
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

#ifndef __SEC_SYSFS_H
#define __SEC_SYSFS_H __FILE__

#include<linux/list.h>
#include <linux/device.h>

struct sec_sysfs {
	struct device_attribute* attr;
	unsigned long size;
	struct list_head list;
};

extern int sec_bat_create_attrs(struct device *dev);
extern void add_sec_sysfs(struct list_head *plist);

#endif /* __SEC_SYSFS_H */
