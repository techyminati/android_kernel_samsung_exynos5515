/*
 * sb_misc.h
 * Samsung Mobile Battery MISC Header
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

#ifndef __SB_MISC_H
#define __SB_MISC_H __FILE__

#include <linux/fs.h>

struct device;
struct sb_misc;

typedef int (*misc_func)(struct device *dev, unsigned int cmd, unsigned long arg);

#define SB_MISC_DISABLE	(-3662)
#if IS_ENABLED(CONFIG_SB_NOTIFY)
struct sb_misc *sb_misc_init(struct device *dev, const char *path, misc_func cb_ioctl);
int sb_misc_exit(struct sb_misc *misc);
#else
static inline struct sb_misc *sb_misc_init(struct device *dev, const char *path, misc_func cb_ioctl)
{ return ERR_PTR(SB_MISC_DISABLE); }
static inline int sb_misc_exit(struct sb_misc *misc)
{ return SB_MISC_DISABLE; }
#endif

#endif /* __SB_MISC_H */

