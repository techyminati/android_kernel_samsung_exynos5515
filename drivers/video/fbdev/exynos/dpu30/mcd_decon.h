/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __MCD_DECON_H__
#define __MCD_DECON_H__

#include <linux/console.h>
#include <media/v4l2-device.h>
#include "decon.h"
#include "dpp.h"
#include "mcd_helper.h"
#include "../mcd_panel/dynamic_mipi/band_info.h"

static char *dpp_state_str[] = {
	"WIN_DISABLE",
	"WIN_COLOR_",
	"WIN_BUFFER",
	"WIN_UPDATE",
	"WIN_CURSOR",
};

static char *dpp_rot_str[] = {
	"NORMAL",
	"XFLIP",
	"YFLIP",
	"ROT180",
	"ROT90",
	"ROT90_XFLIP",
	"ROT90_YFLIP",
	"ROT270",
};

static char *csc_type_str[] = {
	"BT_601",
	"BT_709",
	"BT_2020",
	"DCI_P3",
	"BT_601_625",
	"BT_601_625_UN",
	"BT_601_525",
	"BT_601_525_UN",
	"BT_2020_CON",
	"BT_470M",
	"FILM",
	"ADOBE_RGB",
	"BT709_C",
	"BT2020_C",
	"DCI_P3_C",
};

static char *csc_range_str[] = {
	"RANGE_LIMITED",
	"RAMGE_FULL",
	"RANGE_EXTENDED",
};

static char *hdr_str[] = {
	"HDR_OFF",
	"HDR_ST2084",
	"HDR_HLG",
	"LINEAR",
	"SRGB",
	"SMPTE_170M",
	"GAMMA2_2",
	"GAMMA2_6",
	"GAMMA2_8",
};


#ifdef CONFIG_DYNAMIC_MIPI
void mcd_set_freq_hop(struct decon_device *decon, struct decon_reg_data *regs, bool en);
#else
static inline void mcd_set_freq_hop(struct decon_device *decon, struct decon_reg_data *regs, bool en) { return; }
#endif

#ifdef CONFIG_MCDHDR
int mcd_prepare_hdr_config(struct decon_device *decon, struct decon_win_config_data *win_data, struct decon_reg_data *regs);
void mcd_clear_hdr_info(struct decon_device *decon, struct decon_reg_data *regs);
void mcd_free_hdr_info(struct decon_device *decon, struct decon_reg_data *regs);
inline void mcd_fill_hdr_info(int win_idx, struct dpp_config *config, struct decon_reg_data *regs);
void mcd_unmap_all_hdr_info(struct decon_device *decon);
#else
static inline int mcd_prepare_hdr_config(struct decon_device *decon, struct decon_win_config_data *win_data, struct decon_reg_data *regs) { return 0; }
static inline void mcd_free_hdr_info(struct decon_device *decon, struct decon_reg_data *regs) { return; }
static inline void mcd_fill_hdr_info(int win_idx, struct dpp_config *config, struct decon_reg_data *regs) { return; };
static inline void mcd_unmap_all_hdr_info(struct decon_device *decon) {};
#endif

bool is_hmd_not_running(struct decon_device *decon);
int mcd_decon_create_debug_sysfs(struct decon_device *decon);

void mcd_decon_panel_dump(struct decon_device *decon);
void mcd_decon_flush_image(struct decon_device *decon);
void print_win_config_info(struct decon_device *decon, struct decon_reg_data *reg_data);

#if IS_ENABLED(CONFIG_RTC_LIB)
int get_str_cur_rtc(char *buf, size_t size);
#else
static inline int get_str_cur_rtc(char *buf, size_t size) { return -EINVAL; }
#endif

#endif //__MCD_DECON_H__
