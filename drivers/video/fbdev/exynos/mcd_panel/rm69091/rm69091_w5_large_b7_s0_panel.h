/*
 * linux/drivers/video/fbdev/exynos/panel/rm69091/rm69091_w5_large_b7_s0_panel.h
 *
 * Header file for S6W36W5x01 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RM69091_W5_LARGE_B7_S0_PANEL_H__
#define __RM69091_W5_LARGE_B7_S0_PANEL_H__
#include "../panel.h"
#include "../panel_drv.h"
#include "rm69091.h"
#include "rm69091_dimming.h"

#include "rm69091_b7_s0_panel_dimming.h"

#undef __pn_name__
#define __pn_name__	w5_large_b7_s0

#undef __PN_NAME__
#define __PN_NAME__	W5_LARGE_B7_S0

/* ===================================================================================== */
/* ============================= [RM69091 READ INFO TABLE] ============================= */
/* ===================================================================================== */
/* <READINFO TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================= [RM69091 RESOURCE TABLE] ============================== */
/* ===================================================================================== */
/* <RESOURCE TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================== [RM69091 MAPPING TABLE] ============================== */
/* ===================================================================================== */

static u8 w5_large_b7_s0_brt_table[RM69091_W5_TOTAL_STEP][1] = {
	/* Normal 5x51+1 */
	{ 0x02 }, { 0x02 }, { 0x02 }, { 0x03 }, { 0x04 },
	{ 0x05 }, { 0x06 }, { 0x07 }, { 0x08 }, { 0x09 },
	{ 0x0A }, { 0x0B }, { 0x0C }, { 0x0D }, { 0x0E },
	{ 0x0F }, { 0x10 }, { 0x11 }, { 0x12 }, { 0x13 },
	{ 0x14 }, { 0x15 }, { 0x16 }, { 0x17 }, { 0x18 },
	{ 0x19 }, { 0x1A }, { 0x1B }, { 0x1C }, { 0x1D },
	{ 0x1E }, { 0x1F }, { 0x20 }, { 0x21 }, { 0x22 },
	{ 0x23 }, { 0x24 }, { 0x25 }, { 0x26 }, { 0x27 },
	{ 0x28 }, { 0x29 }, { 0x2A }, { 0x2B }, { 0x2C },
	{ 0x2D }, { 0x2E }, { 0x2F }, { 0x30 }, { 0x31 },
	{ 0x32 }, { 0x33 }, { 0x34 }, { 0x35 }, { 0x36 },
	{ 0x37 }, { 0x38 }, { 0x39 }, { 0x3A }, { 0x3B },
	{ 0x3C }, { 0x3D }, { 0x3E }, { 0x3F }, { 0x40 },
	{ 0x41 }, { 0x42 }, { 0x43 }, { 0x44 }, { 0x45 },
	{ 0x46 }, { 0x47 }, { 0x48 }, { 0x49 }, { 0x4A },
	{ 0x4B }, { 0x4C }, { 0x4D }, { 0x4E }, { 0x4F },
	{ 0x50 }, { 0x51 }, { 0x52 }, { 0x53 }, { 0x54 },
	{ 0x55 }, { 0x56 }, { 0x57 }, { 0x58 }, { 0x59 },
	{ 0x5A }, { 0x5B }, { 0x5C }, { 0x5D }, { 0x5E },
	{ 0x5F }, { 0x60 }, { 0x61 }, { 0x62 }, { 0x63 },
	{ 0x64 }, { 0x65 }, { 0x66 }, { 0x67 }, { 0x68 },
	{ 0x69 }, { 0x6A }, { 0x6B }, { 0x6C }, { 0x6D },
	{ 0x6E }, { 0x6F }, { 0x70 }, { 0x71 }, { 0x72 },
	{ 0x73 }, { 0x74 }, { 0x75 }, { 0x76 }, { 0x77 },
	{ 0x78 }, { 0x79 }, { 0x7A }, { 0x7B }, { 0x7C },
	{ 0x7D }, { 0x7E }, { 0x7F }, { 0x80 }, { 0x81 },
	{ 0x82 }, { 0x83 }, { 0x84 }, { 0x85 }, { 0x86 },
	{ 0x87 }, { 0x88 }, { 0x89 }, { 0x8A }, { 0x8B },
	{ 0x8C }, { 0x8D }, { 0x8E }, { 0x8F }, { 0x90 },
	{ 0x91 }, { 0x92 }, { 0x93 }, { 0x94 }, { 0x95 },
	{ 0x96 }, { 0x97 }, { 0x98 }, { 0x99 }, { 0x9A },
	{ 0x9B }, { 0x9C }, { 0x9D }, { 0x9E }, { 0x9F },
	{ 0xA0 }, { 0xA1 }, { 0xA2 }, { 0xA3 }, { 0xA4 },
	{ 0xA5 }, { 0xA6 }, { 0xA7 }, { 0xA8 }, { 0xA9 },
	{ 0xAA }, { 0xAB }, { 0xAC }, { 0xAD }, { 0xAE },
	{ 0xAF }, { 0xB0 }, { 0xB1 }, { 0xB2 }, { 0xB3 },
	{ 0xB4 }, { 0xB5 }, { 0xB6 }, { 0xB7 }, { 0xB8 },
	{ 0xB9 }, { 0xBA }, { 0xBB }, { 0xBC }, { 0xBD },
	{ 0xBE }, { 0xBF }, { 0xC0 }, { 0xC1 }, { 0xC2 },
	{ 0xC3 }, { 0xC4 }, { 0xC5 }, { 0xC6 }, { 0xC7 },
	{ 0xC8 }, { 0xC9 }, { 0xCA }, { 0xCB }, { 0xCC },
	{ 0xCD }, { 0xCE }, { 0xCF }, { 0xD0 }, { 0xD1 },
	{ 0xD2 }, { 0xD3 }, { 0xD4 }, { 0xD5 }, { 0xD6 },
	{ 0xD7 }, { 0xD8 }, { 0xD9 }, { 0xDA }, { 0xDB },
	{ 0xDC }, { 0xDD }, { 0xDE }, { 0xDF }, { 0xE0 },
	{ 0xE1 }, { 0xE2 }, { 0xE3 }, { 0xE4 }, { 0xE5 },
	{ 0xE6 }, { 0xE7 }, { 0xE8 }, { 0xE9 }, { 0xEA },
	{ 0xEB }, { 0xEC }, { 0xED }, { 0xEE }, { 0xEF },
	{ 0xF0 }, { 0xF1 }, { 0xF2 }, { 0xF3 }, { 0xF4 },
	{ 0xF5 }, { 0xF6 }, { 0xF7 }, { 0xF8 }, { 0xF9 },
	{ 0xFA }, { 0xFB }, { 0xFC }, { 0xFD }, { 0xFE },
	{ 0xFF },
	/* HBM 45 */
	{ 0xD3 }, { 0xD4 }, { 0xD5 }, { 0xD6 }, { 0xD7 },
	{ 0xD8 }, { 0xD9 }, { 0xDA }, { 0xDB }, { 0xDC },
	{ 0xDD }, { 0xDE }, { 0xDF }, { 0xE0 }, { 0xE1 },
	{ 0xE2 }, { 0xE3 }, { 0xE4 }, { 0xE5 }, { 0xE6 },
	{ 0xE7 }, { 0xE8 }, { 0xE9 }, { 0xEA }, { 0xEB },
	{ 0xEC }, { 0xED }, { 0xEE }, { 0xEF }, { 0xF0 },
	{ 0xF1 }, { 0xF2 }, { 0xF3 }, { 0xF4 }, { 0xF5 },
	{ 0xF6 }, { 0xF7 }, { 0xF8 }, { 0xF9 }, { 0xFA },
	{ 0xFB }, { 0xFC }, { 0xFD }, { 0xFE }, { 0xFF },
};

static u8 w5_large_b7_s0_lpm_brt_table[RM69091_W5_TOTAL_STEP][1] = {
	/* low 5x4+1 */
	{ 0x02 }, { 0x02 }, { 0x02 }, { 0x02 }, { 0x02 },
	{ 0x02 }, { 0x02 }, { 0x02 }, { 0x02 }, { 0x02 },
	{ 0x02 }, { 0x02 }, { 0x02 }, { 0x02 }, { 0x02 },
	{ 0x02 }, { 0x02 }, { 0x02 }, { 0x02 }, { 0x02 },
	{ 0x02 },
	/* high 4+5x46+1 */
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	/* HBM 45 */
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },
};

// maptbl table info
static u8 w5_large_b7_s0_hbm_onoff_table[][1] = {
	{ 0x00 },
	{ 0x02 },
};

static u8 w5_large_b7_s0_acl_control_table[MAX_RM69091_ACL_RATIO][1] = {
	[RM69091_ACL_RATIO_0] = { 0x12 },
	[RM69091_ACL_RATIO_8] = { 0x12 },
	[RM69091_ACL_RATIO_15] = { 0x24 },
};

static u8 w5_large_b7_s0_hbm_acl_control_table[MAX_RM69091_ACL_RATIO][1] = {
	[RM69091_ACL_RATIO_0] = { 0x17 },
	[RM69091_ACL_RATIO_8] = { 0x17 },
	[RM69091_ACL_RATIO_15] = { 0x2D },
};

static u8 w5_large_b7_s0_normal_acl_onoff_table[][1] = {
	{ 0x00 },
	{ 0x01 },
};

static u8 w5_large_b7_s0_hbm_acl_onoff_table[][1] = {
	{ 0x01 },
	{ 0x09 },
};

static u8 w5_large_b7_s0_hbm_transition_table[SMOOTH_TRANS_MAX][1] = {
	/* Normal */
	{ 0x20 },
	/* Smooth */
	{ 0x28 },
};

static u8 w5_large_b7_s0_elvss_temp_table[][1] = {
	{ 0x44 },
	{ 0x3C },
	{ 0x3A },
};

static u8 w5_large_b7_s0_vinit_temp_table[][1] = {
	{ 0x19 },
	{ 0x21 },
	{ 0x23 },
};

static u8 w5_large_b7_s0_vcl_temp_table[][1] = {
	{ 0x81 },
	{ 0x91 },
	{ 0x92 },
};

static struct maptbl w5_large_b7_s0_maptbl[MAX_MAPTBL] = {
	[GAMMA_MODE2_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_brt_table, init_gamma_mode2_brt_table, getidx_gamma_mode2_brt_table, copy_common_maptbl),
	[ACL_CONTROL_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_acl_control_table, init_common_table, getidx_acl_control_table, copy_common_maptbl),
	[HBM_ACL_CONTROL_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_hbm_acl_control_table, init_common_table, getidx_acl_control_table, copy_common_maptbl),
	[HBM_ONOFF_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_hbm_onoff_table, init_common_table, getidx_hbm_onoff_table, copy_common_maptbl),
	[NORMAL_ACL_ONOFF_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_normal_acl_onoff_table, init_common_table, getidx_acl_onoff_table, copy_common_maptbl),
	[HBM_ACL_ONOFF_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_hbm_acl_onoff_table, init_common_table, getidx_acl_onoff_table, copy_common_maptbl),
	[HBM_TRANSITION_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_hbm_transition_table, init_common_table, getidx_hbm_transition_table, copy_common_maptbl),
	[LPM_WRDISBV_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_lpm_brt_table, init_gamma_mode2_brt_table, getidx_gamma_mode2_brt_table, copy_common_maptbl),
	[ELVSS_TEMP_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_elvss_temp_table, init_common_table, getidx_elvss_temp_table, copy_common_maptbl),
	[VINIT_TEMP_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_vinit_temp_table, init_common_table, getidx_elvss_temp_table, copy_common_maptbl),
	[VCL_TEMP_MAPTBL] = DEFINE_2D_MAPTBL(w5_large_b7_s0_vcl_temp_table, init_common_table, getidx_elvss_temp_table, copy_common_maptbl),
};

static DEFINE_PANEL_MDELAY(w5_large_b7_s0_wait_1msec, 1);
static DEFINE_PANEL_MDELAY(w5_large_b7_s0_wait_1frame, 17);
static DEFINE_PANEL_MDELAY(w5_large_b7_s0_wait_sleepout, 60);
static DEFINE_PANEL_MDELAY(w5_large_b7_s0_wait_34msec, 34);
static DEFINE_PANEL_MDELAY(w5_large_b7_s0_wait_100msec, 100);
static DEFINE_PANEL_MDELAY(w5_large_b7_s0_wait_20msec, 20);
static DEFINE_PANEL_MDELAY(w5_large_b7_s0_wait_5frames, 90);

static u8 W5_LARGE_B7_S0_SLEEP_OUT[] = { 0x11 };
static u8 W5_LARGE_B7_S0_SLEEP_IN[] = { 0x10 };
static u8 W5_LARGE_B7_S0_DISPLAY_OFF[] = { 0x28 };
static u8 W5_LARGE_B7_S0_DISPLAY_ON[] = { 0x29 };
static u8 W5_LARGE_B7_S0_TE_ON[] = { 0x35, 0x02, 0x00 };
static u8 W5_LARGE_B7_S0_ENTER_IDLE[] = { 0x39 };
static u8 W5_LARGE_B7_S0_EXIT_IDLE[] = { 0x38 };


static DEFINE_STATIC_PACKET(w5_large_b7_s0_sleep_out, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SLEEP_OUT, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_sleep_in, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SLEEP_IN, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_display_on, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_DISPLAY_ON, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_display_off, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_DISPLAY_OFF, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_te_on, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_TE_ON, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_enter_idle, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_ENTER_IDLE, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_exit_idle, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_EXIT_IDLE, 0);

static u8 W5_LARGE_B7_S0_SWITCH_TO_USER_CMD[] = { 0xFE, 0x00 };
static u8 W5_LARGE_B7_S0_SWITCH_TO_MANUFACTURE_CMD[] = { 0xFE, 0x01 };
static u8 W5_LARGE_B7_S0_SWITCH_TO_DBC_CMD[] = { 0xFE, 0x06 };
static u8 W5_LARGE_B7_S0_SWITCH_TO_VSR_CMD[] = { 0xFE, 0x04 };
static u8 W5_LARGE_B7_S0_SWITCH_TO_EXT_CMD[] = { 0xFE, 0x0C };
static u8 W5_LARGE_B7_S0_SWITCH_TO_SMOOTH_CMD[] = { 0xFE, 0x05 };
static u8 W5_LARGE_B7_S0_SWITCH_TO_READ_PRODUCT_CMD[] = { 0xFE, 0x0B };


static DEFINE_STATIC_PACKET(w5_large_b7_s0_switch_to_user_cmd, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SWITCH_TO_USER_CMD, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_switch_to_manufacture_cmd, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SWITCH_TO_MANUFACTURE_CMD, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_switch_to_dbc_cmd, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SWITCH_TO_DBC_CMD, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_switch_to_vsr_cmd, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SWITCH_TO_VSR_CMD, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_switch_to_ext_cmd, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SWITCH_TO_EXT_CMD, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_switch_to_smooth_cmd, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SWITCH_TO_SMOOTH_CMD, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_switch_to_read_product_cmd, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SWITCH_TO_READ_PRODUCT_CMD, 0);

static u8 W5_LARGE_B7_S0_IDLE_MODE_EXTERNAL_POWER[] = { 0x66, 0x40 };
static DEFINE_STATIC_PACKET(w5_large_b7_s0_idle_mode_external_power, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_IDLE_MODE_EXTERNAL_POWER, 0);

static u8 W5_LARGE_B7_S0_CASET[] = {
	0x2A,
	0x00, 0x10, 0x01, 0xD1
};
static u8 W5_LARGE_B7_S0_PASET[] = {
	0x2B,
	0x00, 0x00, 0x01, 0xC1,
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_caset, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_CASET, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_paset, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_PASET, 0);

static u8 W5_LARGE_B7_S0_TOUCH_VSYNC_ON[] = {
	0xF7,
	0x04
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_touch_vsync_on, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_TOUCH_VSYNC_ON, 0);

static u8 W5_LARGE_B7_S0_HBM_ONOFF[] = {
	0x66,
	0x00
};
static DEFINE_PKTUI(w5_large_b7_s0_hbm_onoff, &w5_large_b7_s0_maptbl[HBM_ONOFF_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_hbm_onoff, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_HBM_ONOFF, 0);

static u8 W5_LARGE_B7_S0_HBM_BL[] = {
	0x63,
	0xFF
};
static DEFINE_PKTUI(w5_large_b7_s0_hbm_bl, &w5_large_b7_s0_maptbl[GAMMA_MODE2_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_hbm_bl, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_HBM_BL, 0);

static u8 W5_LARGE_B7_S0_HBM_BL_OFF[] = {
	0x63,
	0x00
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_hbm_bl_off, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_HBM_BL_OFF, 0);


static u8 W5_LARGE_B7_S0_SMOOTH_INIT[] = {
	0x9F,
	0x00
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_smooth_init, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SMOOTH_INIT, 0);

static u8 W5_LARGE_B7_S0_SMOOTH_8FRAME[] = {
	0xA0,
	0x02
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_smooth_8frame, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SMOOTH_8FRAME, 0);

static u8 W5_LARGE_B7_S0_SMOOTH_32FRAME[] = {
	0xA0,
	0x04
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_smooth_32frame, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_SMOOTH_32FRAME, 0);

static u8 W5_LARGE_B7_S0_WRDISBV[] = {
	0x51, 0x35
};
static DEFINE_PKTUI(w5_large_b7_s0_wrdisbv, &w5_large_b7_s0_maptbl[GAMMA_MODE2_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_wrdisbv, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_WRDISBV, 0);

static u8 W5_LARGE_B7_S0_NORMAL_ACL_CONTROL[] = {
	0xB3,
	0x24
};
static DEFINE_PKTUI(w5_large_b7_s0_normal_acl_control, &w5_large_b7_s0_maptbl[ACL_CONTROL_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_normal_acl_control, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_NORMAL_ACL_CONTROL, 0);

static u8 W5_LARGE_B7_S0_HBM_ACL_CONTROL[] = {
	0xB4,
	0x17
};
static DEFINE_PKTUI(w5_large_b7_s0_hbm_acl_control, &w5_large_b7_s0_maptbl[HBM_ACL_CONTROL_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_hbm_acl_control, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_HBM_ACL_CONTROL, 0);

static u8 W5_LARGE_B7_S0_NORMAL_ACL_ONOFF[] = {
	0x55,
	0x00
};
static DEFINE_PKTUI(w5_large_b7_s0_normal_acl_onoff, &w5_large_b7_s0_maptbl[NORMAL_ACL_ONOFF_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_normal_acl_onoff, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_NORMAL_ACL_ONOFF, 0);

static u8 W5_LARGE_B7_S0_HBM_ACL_ONOFF[] = {
	0xBD,
	0x01
};
static DEFINE_PKTUI(w5_large_b7_s0_hbm_acl_onoff, &w5_large_b7_s0_maptbl[HBM_ACL_ONOFF_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_hbm_acl_onoff, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_HBM_ACL_ONOFF, 0);

static u8 W5_LARGE_B7_S0_NORMAL_ACL_SET1[] = {
	0xB1,
	0x00
};
static u8 W5_LARGE_B7_S0_NORMAL_ACL_SET2[] = {
	0xA3,
	0x80
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_normal_acl_set1, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_NORMAL_ACL_SET1, 0);
static DEFINE_STATIC_PACKET(w5_large_b7_s0_normal_acl_set2, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_NORMAL_ACL_SET2, 0);

static u8 W5_LARGE_B7_S0_HBM_ACL_SET1[] = {
	0xA4,
	0x99
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_hbm_acl_set1, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_HBM_ACL_SET1, 0);

static u8 W5_LARGE_B7_S0_HBM_TRANSITION[] = {
	0x53,
	0x28
};
static DEFINE_PKTUI(w5_large_b7_s0_hbm_transition, &w5_large_b7_s0_maptbl[HBM_TRANSITION_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_hbm_transition, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_HBM_TRANSITION, 0);

static u8 W5_LARGE_B7_S0_ELVSS_TEMP[] = {
	0x6A,
	0x44
};
static DEFINE_PKTUI(w5_large_b7_s0_elvss_temp, &w5_large_b7_s0_maptbl[ELVSS_TEMP_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_elvss_temp, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_ELVSS_TEMP, 0x00);

static u8 W5_LARGE_B7_S0_VINIT_TEMP[] = {
	0x62,
	0x19
};
static DEFINE_PKTUI(w5_large_b7_s0_vinit_temp, &w5_large_b7_s0_maptbl[VINIT_TEMP_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_vinit_temp, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_VINIT_TEMP, 0x00);

static u8 W5_LARGE_B7_S0_VCL_TEMP[] = {
	0x11,
	0x81
};
static DEFINE_PKTUI(w5_large_b7_s0_vcl_temp, &w5_large_b7_s0_maptbl[VCL_TEMP_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_vcl_temp, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_VCL_TEMP, 0x00);

static u8 W5_LARGE_B7_S0_ACL_INIT[] = {
	0x89,
	0xC5
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_acl_init, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_ACL_INIT, 0x00);

static u8 W5_LARGE_B7_S0_ACL_V[] = {
	0x8A,
	0x8F
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_acl_v, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_ACL_V, 0x00);

static u8 W5_LARGE_B7_S0_ACL_H[] = {
	0x8B,
	0x8F
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_acl_h, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_ACL_H, 0x00);

static u8 W5_LARGE_B7_S0_LPM_WRDISBV[] = {
	0x51,
	0x15
};
static DEFINE_PKTUI(w5_large_b7_s0_lpm_wrdisbv, &w5_large_b7_s0_maptbl[LPM_WRDISBV_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(w5_large_b7_s0_lpm_wrdisbv, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_LPM_WRDISBV, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_ON_1[] = {
	0x78,
	0xFA
};

static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_on_1, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_ON_1, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_ON_2[] = {
	0x07,
	0xFF
};

static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_on_2, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_ON_2, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_ON_3[] = {
	0x08,
	0xFF
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_on_3, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_ON_3, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_ON_4[] = {
	0x09,
	0xFF
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_on_4, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_ON_4, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_ON_5[] = {
	0x0A,
	0xFF
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_on_5, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_ON_5, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_ON_6[] = {
	0x0B,
	0xFF
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_on_6, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_ON_6, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_ON_7[] = {
	0x0C,
	0xFF
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_on_7, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_ON_7, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_OFF_1[] = {
	0x78,
	0xFF
};

static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_off_1, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_OFF_1, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_OFF_2[] = {
	0x07,
	0xF1
};

static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_off_2, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_OFF_2, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_OFF_3[] = {
	0x08,
	0xF2
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_off_3, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_OFF_3, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_OFF_4[] = {
	0x09,
	0xF3
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_off_4, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_OFF_4, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_OFF_5[] = {
	0x0A,
	0xF4
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_off_5, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_OFF_5, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_OFF_6[] = {
	0x0B,
	0xF5
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_off_6, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_OFF_6, 0);

static u8 W5_LARGE_B7_S0_MCD_TEST_OFF_7[] = {
	0x0C,
	0xF6
};
static DEFINE_STATIC_PACKET(w5_large_b7_s0_mcd_test_off_7, DSI_PKT_TYPE_WR, W5_LARGE_B7_S0_MCD_TEST_OFF_7, 0);

static DEFINE_COND(w5_large_b7_s0_cond_is_smooth_dimming_available, is_smooth_dimming_available);
static DEFINE_COND(w5_large_b7_s0_cond_is_hbm_on, is_hbm_on);
static DEFINE_COND(w5_large_b7_s0_cond_is_prev_normal_brt, is_prev_normal_brt);

static struct seqinfo SEQINFO(w5_large_b7_s0_set_bl_param_seq);

static void *w5_large_b7_s0_init_cmdtbl[] = {
	&DLYINFO(w5_large_b7_s0_wait_1msec),
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_te_on),

	&PKTINFO(w5_large_b7_s0_switch_to_manufacture_cmd),
	&PKTINFO(w5_large_b7_s0_idle_mode_external_power),
	&PKTINFO(w5_large_b7_s0_touch_vsync_on),
	&PKTINFO(w5_large_b7_s0_acl_init),
	&PKTINFO(w5_large_b7_s0_acl_v),
	&PKTINFO(w5_large_b7_s0_acl_h),
	&PKTINFO(w5_large_b7_s0_switch_to_smooth_cmd),
	&PKTINFO(w5_large_b7_s0_smooth_init),
	&PKTINFO(w5_large_b7_s0_smooth_8frame),

	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_caset),
	&PKTINFO(w5_large_b7_s0_paset),
	&PKTINFO(w5_large_b7_s0_sleep_out),
	&DLYINFO(w5_large_b7_s0_wait_sleepout),
	&SEQINFO(w5_large_b7_s0_set_bl_param_seq),
	&PKTINFO(w5_large_b7_s0_display_on),
};

static void *w5_large_b7_s0_display_on_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_display_on),
};

static void *w5_large_b7_s0_display_off_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_display_off),
};

static void *w5_large_b7_s0_check_condition_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&rm69091_dmptbl[DUMP_RDDPM],
	&rm69091_dmptbl[DUMP_RDDSM],
};

static void *w5_large_b7_s0_exit_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&rm69091_dmptbl[DUMP_RDDPM_SLEEP_IN],
#ifdef CONFIG_DISPLAY_USE_INFO
	&rm69091_dmptbl[DUMP_RDDSM],
	&rm69091_dmptbl[DUMP_DSI_ERR],
	&rm69091_dmptbl[DUMP_SELF_DIAG],
#endif
	&PKTINFO(w5_large_b7_s0_sleep_in),
	&DLYINFO(w5_large_b7_s0_wait_5frames),
};

static void *w5_large_b7_s0_set_bl_param_cmdtbl[] = {
	&CONDINFO_IF(w5_large_b7_s0_cond_is_smooth_dimming_available),
		&PKTINFO(w5_large_b7_s0_switch_to_smooth_cmd),
		&PKTINFO(w5_large_b7_s0_smooth_32frame),
	&CONDINFO_FI(w5_large_b7_s0_cond_is_smooth_dimming_available),
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_hbm_transition),
	&PKTINFO(w5_large_b7_s0_switch_to_manufacture_cmd),
	&PKTINFO(w5_large_b7_s0_vcl_temp),
	&PKTINFO(w5_large_b7_s0_elvss_temp),
	&PKTINFO(w5_large_b7_s0_vinit_temp),
	&CONDINFO_IF(w5_large_b7_s0_cond_is_hbm_on),			// hbm
		&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
		&PKTINFO(w5_large_b7_s0_hbm_bl),
		&CONDINFO_IF(w5_large_b7_s0_cond_is_prev_normal_brt),			// prev normal(normal->hbm)
			&DLYINFO(w5_large_b7_s0_wait_20msec),
		&CONDINFO_FI(w5_large_b7_s0_cond_is_prev_normal_brt),
		&PKTINFO(w5_large_b7_s0_hbm_onoff),
		&PKTINFO(w5_large_b7_s0_switch_to_dbc_cmd),
		&PKTINFO(w5_large_b7_s0_hbm_acl_control),
		&PKTINFO(w5_large_b7_s0_hbm_acl_set1),
	&CONDINFO_EL(w5_large_b7_s0_cond_is_hbm_on),			// normal
		&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
		&PKTINFO(w5_large_b7_s0_wrdisbv),
		&PKTINFO(w5_large_b7_s0_hbm_onoff),
		&PKTINFO(w5_large_b7_s0_switch_to_dbc_cmd),
		&PKTINFO(w5_large_b7_s0_normal_acl_control),
		&PKTINFO(w5_large_b7_s0_normal_acl_set1),
		&PKTINFO(w5_large_b7_s0_normal_acl_set2),
	&CONDINFO_FI(w5_large_b7_s0_cond_is_hbm_on),
	&PKTINFO(w5_large_b7_s0_hbm_acl_onoff),
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_normal_acl_onoff),
};
static DEFINE_SEQINFO(w5_large_b7_s0_set_bl_param_seq, w5_large_b7_s0_set_bl_param_cmdtbl);

static void *w5_large_b7_s0_set_bl_cmdtbl[] = {
	&SEQINFO(w5_large_b7_s0_set_bl_param_seq),
};

static void *w5_large_b7_s0_dummy_cmdtbl[] = {
	NULL,
};

static void *w5_large_b7_s0_res_init_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_read_product_cmd),
	&rm69091_restbl[RES_PRODUCT_INFO1],
	&rm69091_restbl[RES_PRODUCT_INFO2],
	&rm69091_restbl[RES_PRODUCT_INFO3],
	&rm69091_restbl[RES_PRODUCT_INFO4],
	&rm69091_restbl[RES_PRODUCT_INFO5],
	&rm69091_restbl[RES_PRODUCT_INFO6],
	&rm69091_restbl[RES_PRODUCT_INFO7],
	&rm69091_restbl[RES_PRODUCT_INFO8],
	&rm69091_restbl[RES_PRODUCT_INFO9],
	&rm69091_restbl[RES_PRODUCT_INFO10],
	&rm69091_restbl[RES_PRODUCT_INFO11],
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
};

static void *w5_large_b7_s0_alpm_enter_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_lpm_wrdisbv),
	&PKTINFO(w5_large_b7_s0_enter_idle),
};

static void *w5_large_b7_s0_alpm_exit_cmdtbl[] = {
	&SEQINFO(w5_large_b7_s0_set_bl_param_seq),
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
	&PKTINFO(w5_large_b7_s0_exit_idle),
};

static void *w5_large_b7_s0_mcd_on_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_vsr_cmd),
	&PKTINFO(w5_large_b7_s0_mcd_test_on_1),
	&PKTINFO(w5_large_b7_s0_switch_to_ext_cmd),
	&PKTINFO(w5_large_b7_s0_mcd_test_on_2),
	&PKTINFO(w5_large_b7_s0_mcd_test_on_3),
	&PKTINFO(w5_large_b7_s0_mcd_test_on_4),
	&PKTINFO(w5_large_b7_s0_mcd_test_on_5),
	&PKTINFO(w5_large_b7_s0_mcd_test_on_6),
	&PKTINFO(w5_large_b7_s0_mcd_test_on_7),
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
};

static void *w5_large_b7_s0_mcd_off_cmdtbl[] = {
	&PKTINFO(w5_large_b7_s0_switch_to_vsr_cmd),
	&PKTINFO(w5_large_b7_s0_mcd_test_off_1),
	&PKTINFO(w5_large_b7_s0_switch_to_ext_cmd),
	&PKTINFO(w5_large_b7_s0_mcd_test_off_2),
	&PKTINFO(w5_large_b7_s0_mcd_test_off_3),
	&PKTINFO(w5_large_b7_s0_mcd_test_off_4),
	&PKTINFO(w5_large_b7_s0_mcd_test_off_5),
	&PKTINFO(w5_large_b7_s0_mcd_test_off_6),
	&PKTINFO(w5_large_b7_s0_mcd_test_off_7),
	&PKTINFO(w5_large_b7_s0_switch_to_user_cmd),
};

static struct seqinfo w5_large_b7_s0_seqtbl[MAX_PANEL_SEQ] = {
	[PANEL_INIT_SEQ] = SEQINFO_INIT("init-seq", w5_large_b7_s0_init_cmdtbl),
	[PANEL_SET_BL_SEQ] = SEQINFO_INIT("set-bl-seq", w5_large_b7_s0_set_bl_cmdtbl),
	[PANEL_RES_INIT_SEQ] = SEQINFO_INIT("resource-init-seq", w5_large_b7_s0_res_init_cmdtbl),

	[PANEL_CHECK_CONDITION_SEQ] = SEQINFO_INIT("check-condition-seq", w5_large_b7_s0_check_condition_cmdtbl),
	[PANEL_DUMP_SEQ] = SEQINFO_INIT("dump-seq", w5_large_b7_s0_check_condition_cmdtbl),

	[PANEL_DISPLAY_ON_SEQ] = SEQINFO_INIT("display-on-seq", w5_large_b7_s0_display_on_cmdtbl),
	[PANEL_DISPLAY_OFF_SEQ] = SEQINFO_INIT("display-off-seq", w5_large_b7_s0_display_off_cmdtbl),
	[PANEL_EXIT_SEQ] = SEQINFO_INIT("exit-seq", w5_large_b7_s0_exit_cmdtbl),
	[PANEL_MCD_ON_SEQ] = SEQINFO_INIT("mcd-on-seq", w5_large_b7_s0_mcd_on_cmdtbl),
	[PANEL_MCD_OFF_SEQ] = SEQINFO_INIT("mcd-off-seq", w5_large_b7_s0_mcd_off_cmdtbl),
	[PANEL_ALPM_ENTER_SEQ] = SEQINFO_INIT("alpm-enter-seq", w5_large_b7_s0_alpm_enter_cmdtbl),
	[PANEL_ALPM_EXIT_SEQ] = SEQINFO_INIT("alpm-exit-seq", w5_large_b7_s0_alpm_exit_cmdtbl),

	[PANEL_DUMMY_SEQ] = SEQINFO_INIT("dummy-seq", w5_large_b7_s0_dummy_cmdtbl),
};

struct common_panel_info rm69091_w5_large_panel_info = {
	.ldi_name = "rm69091",
	.name = "rm69091_w5_large_b7_s0",
	.model = "AMB675TG01",
	.vendor = "BOE",
	.id = 0x412000,
	.rev = 0,
	.ddi_props = {
		.gpara = (DDI_SUPPORT_WRITE_GPARA | DDI_SUPPORT_POINT_GPARA),
		.isr_action = {
			[PANEL_ISR_CONN_DET] = PANEL_ISR_ACTION_RECOVERY | PANEL_ISR_ACTION_UBCONN_ABC |
				PANEL_ISR_ACTION_UBCONN_NOTIFY | PANEL_ISR_ACTION_UBCONN_UEVENT,
		},
		.err_fg_recovery = true,
		.support_vrr = true,
	},
	.maptbl = w5_large_b7_s0_maptbl,
	.nr_maptbl = ARRAY_SIZE(w5_large_b7_s0_maptbl),
	.seqtbl = w5_large_b7_s0_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(w5_large_b7_s0_seqtbl),
	.rditbl = rm69091_rditbl,
	.nr_rditbl = ARRAY_SIZE(rm69091_rditbl),
	.restbl = rm69091_restbl,
	.nr_restbl = ARRAY_SIZE(rm69091_restbl),
	.dumpinfo = rm69091_dmptbl,
	.nr_dumpinfo = ARRAY_SIZE(rm69091_dmptbl),
	.panel_dim_info = {
		[PANEL_BL_SUBDEV_TYPE_DISP] = &rm69091_w5_b7_s0_panel_dimming_info,
	},
};


static int __init rm69091_w5_large_b7_s0_panel_init(void)
{
	register_common_panel(&rm69091_w5_large_panel_info);
	return 0;
}

arch_initcall(rm69091_w5_large_b7_s0_panel_init)
#endif /* __RM69091_W5_LARGE_B7_S0_PANEL_H__ */
