/*
 * linux/drivers/video/fbdev/exynos/panel/s6e36w5x01/s6e36w5x01_dimming.h
 *
 * Header file for S6E36W5X01 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E36W5X01_DIMMING_H__
#define __S6E36W5X01_DIMMING_H__
#include <linux/types.h>
#include <linux/kernel.h>
#include "../dimming.h"
#include "s6e36w5x01.h"

#define S6E36W5X01_NR_TP (11)

#define S6E36W5X01_NR_LUMINANCE (256)
#define S6E36W5X01_TARGET_LUMINANCE (600)

#define S6E36W5X01_NR_HBM_LUMINANCE (1)
#define S6E36W5X01_TARGET_HBM_LUMINANCE (1000)

#define S6E36W5X01_W5_NR_LUMINANCE (256)
#define S6E36W5X01_W5_TARGET_LUMINANCE (600)

#define S6E36W5X01_W5_NR_HBM_LUMINANCE (45)
#define S6E36W5X01_W5_TARGET_HBM_LUMINANCE (1000)

#ifdef CONFIG_SUPPORT_AOD_BL
#define S6E36W5X01_WISE_AOD_NR_LUMINANCE (4)
#define S6E36W5X01_WISE_AOD_TARGET_LUMINANCE (60)
#endif

#define S6E36W5X01_TOTAL_NR_LUMINANCE (S6E36W5X01_NR_LUMINANCE + S6E36W5X01_NR_HBM_LUMINANCE)
#define S6E36W5X01_W5_TOTAL_NR_LUMINANCE (S6E36W5X01_W5_NR_LUMINANCE + S6E36W5X01_W5_NR_HBM_LUMINANCE)

static struct tp s6e36w5x01_wise_tp[S6E36W5X01_NR_TP] = {
	{ .level = 0, .volt_src = V0_OUT, .name = "V0", .bits = 8 },
	{ .level = 1, .volt_src = V0_OUT, .name = "V1", .bits = 9 },
	{ .level = 7, .volt_src = V0_OUT, .name = "V7", .bits = 9 },
	{ .level = 11, .volt_src = VT_OUT, .name = "V11", .bits = 9 },
	{ .level = 23, .volt_src = VT_OUT, .name = "V23", .bits = 9 },
	{ .level = 35, .volt_src = VT_OUT, .name = "V35", .bits = 9 },
	{ .level = 51, .volt_src = VT_OUT, .name = "V51", .bits = 9 },
	{ .level = 87, .volt_src = VT_OUT, .name = "V87", .bits = 9 },
	{ .level = 151, .volt_src = VT_OUT, .name = "V151", .bits = 9 },
	{ .level = 203, .volt_src = VT_OUT, .name = "V203", .bits = 9 },
	{ .level = 255, .volt_src = VREG_OUT, .name = "V255", .bits = 9 },
};

#endif /* __S6E36W5X01_DIMMING_H__ */
