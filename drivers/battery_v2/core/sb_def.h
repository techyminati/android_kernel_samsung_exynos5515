/*
 * sb_def.h
 * Samsung Mobile Battery DEF Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __SB_DEF_H
#define __SB_DEF_H __FILE__

enum sb_dev_type {
	SB_DEV_UNKNOWN = 0,

	SB_DEV_BATTERY,
	SB_DEV_DIRECT_CHARGER,
	SB_DEV_DUAL_BATTERY,

	SB_DEV_CHARGER,
	SB_DEV_FUEL_GAUGE,
	SB_DEV_WIRELESS_CHARGER,
	SB_DEV_LIMITTER,
	SB_DEV_DIVIDER,

	SB_DEV_MODULE,
	SB_DEV_MAX,
};

typedef union _sb_data {
	unsigned long long	data;
	unsigned char		buf[8];

	void				*ptr;
} sb_data;

typedef int (*sb_func)(void *pdata, sb_data data);

#endif /* __SB_DEF_H */

