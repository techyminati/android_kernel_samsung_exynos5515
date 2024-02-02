/*
 * linux/drivers/video/fbdev/exynos/panel/s6e36w5x01/s6e36w5x01_small_a2_s0_panel.h
 *
 * Header file for S6W36W5x01 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E36W5X01_SMALL_A2_S0_PANEL_H__
#define __S6E36W5X01_SMALL_A2_S0_PANEL_H__
#include "../panel.h"
#include "../panel_drv.h"
#include "s6e36w5x01.h"
#include "s6e36w5x01_dimming.h"

#include "s6e36w5x01_a2_s0_panel_dimming.h"

#undef __pn_name__
#define __pn_name__	small_a2_s0

#undef __PN_NAME__
#define __PN_NAME__	SMALL1_A2_S0

/* ===================================================================================== */
/* ============================= [S6E36W5X01 READ INFO TABLE] ============================= */
/* ===================================================================================== */
/* <READINFO TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================= [S6E36W5X01 RESOURCE TABLE] ============================== */
/* ===================================================================================== */
/* <RESOURCE TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================== [S6E36W5X01 MAPPING TABLE] ============================== */
/* ===================================================================================== */

static u8 small_a2_s0_brt_table[S6E36W5X01_TOTAL_STEP][1] = {
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
	/* HBM 1 */
	{ 0xFF },
};

static u8 small_a2_s0_lpm_brt_table[S6E36W5X01_TOTAL_STEP][1] = {
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
	/* HBM 1 */
	{ 0x15 },
};

// maptbl table info
static u8 small_a2_s0_acl_control_table[MAX_S6E36W5_ACL_RATIO][1] = {
	[S6E36W5_ACL_RATIO_0] = { 0x00 },
	[S6E36W5_ACL_RATIO_8] = { 0x02 },
	[S6E36W5_ACL_RATIO_15] = { 0x01 },
};

static u8 small_a2_s0_hbm_transition_table[MAX_PANEL_HBM][SMOOTH_TRANS_MAX][1] = {
	/* HBM off */
	{
		/* Normal */
		{ 0x20 },
		/* Smooth */
		{ 0x28 },
	},
	/* HBM on */
	{
		/* Normal */
		{ 0xE0 },
		/* Smooth */
		{ 0xE8 },
	}
};

static u8 small_a2_s0_elvss_temp_table[][4] = {
	{ 0x06, 0x66, 0x99, 0x09 },
	{ 0x00, 0x00, 0x66, 0x06 },
};

static struct maptbl small_a2_s0_maptbl[MAX_MAPTBL] = {
	[GAMMA_MODE2_MAPTBL] = DEFINE_2D_MAPTBL(small_a2_s0_brt_table, init_gamma_mode2_brt_table, getidx_gamma_mode2_brt_table, copy_common_maptbl),
	[ACL_CONTROL_MAPTBL] = DEFINE_2D_MAPTBL(small_a2_s0_acl_control_table, init_common_table, getidx_acl_control_table, copy_common_maptbl),
	[TSET_MAPTBL] = DEFINE_0D_MAPTBL(small_a2_s0_tset_table, init_common_table, NULL, copy_tset_maptbl),
	[HBM_TRANSITION_MAPTBL] = DEFINE_3D_MAPTBL(small_a2_s0_hbm_transition_table, init_common_table, getidx_hbm_transition_table, copy_common_maptbl),
	[LPM_WRDISBV_MAPTBL] = DEFINE_2D_MAPTBL(small_a2_s0_lpm_brt_table, init_gamma_mode2_brt_table, getidx_gamma_mode2_brt_table, copy_common_maptbl),
	[ELVSS_TEMP_MAPTBL] = DEFINE_2D_MAPTBL(small_a2_s0_elvss_temp_table, init_common_table, getidx_hbm_onoff_table, copy_common_maptbl),
};

static DEFINE_PANEL_MDELAY(small_a2_s0_wait_10msec, 10);
static DEFINE_PANEL_MDELAY(small_a2_s0_wait_20msec, 20);
static DEFINE_PANEL_MDELAY(small_a2_s0_wait_1frame, 17);
static DEFINE_PANEL_MDELAY(small_a2_s0_wait_34msec, 34);
static DEFINE_PANEL_MDELAY(small_a2_s0_wait_100msec, 100);
static DEFINE_PANEL_MDELAY(small_a2_s0_wait_120msec, 120);

static u8 SMALL_A2_S0_KEY1_ENABLE[] = { 0xF1, 0x5A, 0x5A };
static u8 SMALL_A2_S0_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 SMALL_A2_S0_KEY3_ENABLE[] = { 0xFC, 0x5A, 0x5A };

static u8 SMALL_A2_S0_KEY1_DISABLE[] = { 0xF1, 0xA5, 0xA5 };
static u8 SMALL_A2_S0_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };
static u8 SMALL_A2_S0_KEY3_DISABLE[] = { 0xFC, 0xA5, 0xA5 };

static DEFINE_STATIC_PACKET(small_a2_s0_level1_key_enable, DSI_PKT_TYPE_WR, SMALL_A2_S0_KEY1_ENABLE, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_level2_key_enable, DSI_PKT_TYPE_WR, SMALL_A2_S0_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_level3_key_enable, DSI_PKT_TYPE_WR, SMALL_A2_S0_KEY3_ENABLE, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_level1_key_disable, DSI_PKT_TYPE_WR, SMALL_A2_S0_KEY1_DISABLE, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_level2_key_disable, DSI_PKT_TYPE_WR, SMALL_A2_S0_KEY2_DISABLE, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_level3_key_disable, DSI_PKT_TYPE_WR, SMALL_A2_S0_KEY3_DISABLE, 0);

static DEFINE_PANEL_KEY(small_a2_s0_level1_key_enable, CMD_LEVEL_1, KEY_ENABLE, &PKTINFO(small_a2_s0_level1_key_enable));
static DEFINE_PANEL_KEY(small_a2_s0_level2_key_enable, CMD_LEVEL_2, KEY_ENABLE, &PKTINFO(small_a2_s0_level2_key_enable));
static DEFINE_PANEL_KEY(small_a2_s0_level3_key_enable, CMD_LEVEL_3, KEY_ENABLE, &PKTINFO(small_a2_s0_level3_key_enable));
static DEFINE_PANEL_KEY(small_a2_s0_level1_key_disable, CMD_LEVEL_1, KEY_DISABLE, &PKTINFO(small_a2_s0_level1_key_disable));
static DEFINE_PANEL_KEY(small_a2_s0_level2_key_disable, CMD_LEVEL_2, KEY_DISABLE, &PKTINFO(small_a2_s0_level2_key_disable));
static DEFINE_PANEL_KEY(small_a2_s0_level3_key_disable, CMD_LEVEL_3, KEY_DISABLE, &PKTINFO(small_a2_s0_level3_key_disable));

static u8 SMALL_A2_S0_SLEEP_OUT[] = { 0x11 };
static u8 SMALL_A2_S0_SLEEP_IN[] = { 0x10 };
static u8 SMALL_A2_S0_DISPLAY_OFF[] = { 0x28 };
static u8 SMALL_A2_S0_DISPLAY_ON[] = { 0x29 };
static u8 SMALL_A2_S0_TE_ON[] = { 0x35, 0x00 };

static DEFINE_STATIC_PACKET(small_a2_s0_sleep_out, DSI_PKT_TYPE_WR, SMALL_A2_S0_SLEEP_OUT, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_sleep_in, DSI_PKT_TYPE_WR, SMALL_A2_S0_SLEEP_IN, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_display_on, DSI_PKT_TYPE_WR, SMALL_A2_S0_DISPLAY_ON, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_display_off, DSI_PKT_TYPE_WR, SMALL_A2_S0_DISPLAY_OFF, 0);
static DEFINE_STATIC_PACKET(small_a2_s0_te_on, DSI_PKT_TYPE_WR, SMALL_A2_S0_TE_ON, 0);

static u8 SMALL_A2_S0_WRDISBV[] = {
	0x51, 0x35
};
static DEFINE_PKTUI(small_a2_s0_wrdisbv, &small_a2_s0_maptbl[GAMMA_MODE2_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(small_a2_s0_wrdisbv, DSI_PKT_TYPE_WR, SMALL_A2_S0_WRDISBV, 0);

static u8 SMALL_A2_S0_ACL_SET[] = {
	0xFE,
	0x3C, 0x00
};
static DEFINE_STATIC_PACKET(small_a2_s0_acl_set, DSI_PKT_TYPE_WR, SMALL_A2_S0_ACL_SET, 0x1E);

static u8 SMALL_A2_S0_ACL_CONTROL[] = {
	0x55,
	0x01
};
static DEFINE_PKTUI(small_a2_s0_acl_control, &small_a2_s0_maptbl[ACL_CONTROL_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(small_a2_s0_acl_control, DSI_PKT_TYPE_WR, SMALL_A2_S0_ACL_CONTROL, 0);

static u8 SMALL_A2_S0_TSET[] = {
	0xB5,
	0x19
};
static DEFINE_PKTUI(small_a2_s0_tset, &small_a2_s0_maptbl[TSET_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(small_a2_s0_tset, DSI_PKT_TYPE_WR, SMALL_A2_S0_TSET, 0);

static u8 SMALL_A2_S0_HBM_TRANSITION[] = {
	0x53,
	0x28
};
static DEFINE_PKTUI(small_a2_s0_hbm_transition, &small_a2_s0_maptbl[HBM_TRANSITION_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(small_a2_s0_hbm_transition, DSI_PKT_TYPE_WR, SMALL_A2_S0_HBM_TRANSITION, 0);

static u8 SMALL_A2_S0_ELVSS_TEMP[] = {
	0xB5,
	0x06, 0x66, 0x99, 0x09
};
static DEFINE_PKTUI(small_a2_s0_elvss_temp, &small_a2_s0_maptbl[ELVSS_TEMP_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(small_a2_s0_elvss_temp, DSI_PKT_TYPE_WR, SMALL_A2_S0_ELVSS_TEMP, 0x1A);

static u8 SMALL_A2_S0_SMOOTH_DIMMING_INIT[] = {
	0xB1,
	0x08
};
static DEFINE_STATIC_PACKET(small_a2_s0_smooth_dimming_init, DSI_PKT_TYPE_WR, SMALL_A2_S0_SMOOTH_DIMMING_INIT, 0x05);

static u8 SMALL_A2_S0_SMOOTH_DIMMING[] = {
	0xB1,
	0x20
};
static DEFINE_STATIC_PACKET(small_a2_s0_smooth_dimming, DSI_PKT_TYPE_WR, SMALL_A2_S0_SMOOTH_DIMMING, 0x05);

static u8 SMALL_A2_S0_TSP_VSYNC[] = {
	0xB9,
	0x31, 0x21
};
static DEFINE_STATIC_PACKET(small_a2_s0_tsp_vsync, DSI_PKT_TYPE_WR, SMALL_A2_S0_TSP_VSYNC, 0x07);

static u8 SMALL_A2_S0_OSC_1[] = {
	0xD0,
	0x08
};
static DEFINE_STATIC_PACKET(small_a2_s0_osc_1, DSI_PKT_TYPE_WR, SMALL_A2_S0_OSC_1, 0);

static u8 SMALL_A2_S0_OSC_2[] = {
	0xD7,
	0x4C
};
static DEFINE_STATIC_PACKET(small_a2_s0_osc_2, DSI_PKT_TYPE_WR, SMALL_A2_S0_OSC_2, 0x09);

static u8 SMALL_A2_S0_OSC_3[] = {
	0xFE,
	0xB0
};
static DEFINE_STATIC_PACKET(small_a2_s0_osc_3, DSI_PKT_TYPE_WR, SMALL_A2_S0_OSC_3, 0);

static u8 SMALL_A2_S0_OSC_4[] = {
	0xFE,
	0x30
};
static DEFINE_STATIC_PACKET(small_a2_s0_osc_4, DSI_PKT_TYPE_WR, SMALL_A2_S0_OSC_4, 0);

static u8 SMALL_A2_S0_SECOND_PMIC_1[] = {
	0xB5,
	0x19, 0xAD, 0x0E
};
static DEFINE_STATIC_PACKET(small_a2_s0_second_pmic_1, DSI_PKT_TYPE_WR, SMALL_A2_S0_SECOND_PMIC_1, 0);

static u8 SMALL_A2_S0_SECOND_PMIC_2[] = {
	0xB5,
	0x30
};
static DEFINE_STATIC_PACKET(small_a2_s0_second_pmic_2, DSI_PKT_TYPE_WR, SMALL_A2_S0_SECOND_PMIC_2, 0x5F);

static u8 SMALL_A2_S0_SECOND_PMIC_3[] = {
	0xB5,
	0x70
};
static DEFINE_STATIC_PACKET(small_a2_s0_second_pmic_3, DSI_PKT_TYPE_WR, SMALL_A2_S0_SECOND_PMIC_3, 0x58);

static u8 SMALL_A2_S0_FPS[] = {
	0x60,
	0x01
};
static DEFINE_STATIC_PACKET(small_a2_s0_fps, DSI_PKT_TYPE_WR, SMALL_A2_S0_FPS, 0);

static u8 SMALL_A2_S0_LFD_ONOFF[] = {
	0xBD,
	0x61
};
static DEFINE_STATIC_PACKET(small_a2_s0_lfd_onoff, DSI_PKT_TYPE_WR, SMALL_A2_S0_LFD_ONOFF, 0);

static u8 SMALL_A2_S0_LFD_FREQ[] = {
	0xBD,
	0x02, 0x02, 0x00, 0x00, 0x01
};
static DEFINE_STATIC_PACKET(small_a2_s0_lfd_freq, DSI_PKT_TYPE_WR, SMALL_A2_S0_LFD_FREQ, 0x04);

static u8 SMALL_A2_S0_LPM_FPS[] = {
	0x60,
	0x01
};
static DEFINE_STATIC_PACKET(small_a2_s0_lpm_fps, DSI_PKT_TYPE_WR, SMALL_A2_S0_LPM_FPS, 0);

static u8 SMALL_A2_S0_LPM_LFD_ONOFF[] = {
	0xBD,
	0x61
};
static DEFINE_STATIC_PACKET(small_a2_s0_lpm_lfd_onoff, DSI_PKT_TYPE_WR, SMALL_A2_S0_LPM_LFD_ONOFF, 0);

static u8 SMALL_A2_S0_LPM_LFD_FREQ[] = {
	0xBD,
	0x00, 0x00, 0x44, 0x00, 0x08
};
static DEFINE_STATIC_PACKET(small_a2_s0_lpm_lfd_freq, DSI_PKT_TYPE_WR, SMALL_A2_S0_LPM_LFD_FREQ, 0x04);

static u8 SMALL_A2_S0_FFC[] = {
	0xC5,
	0x15, 0x10, 0x50, 0x1D, 0x28, 0x00
};
static DEFINE_STATIC_PACKET(small_a2_s0_ffc, DSI_PKT_TYPE_WR, SMALL_A2_S0_FFC, 0);

static u8 SMALL_A2_S0_LPM_WRDISBV[] = {
	0x51,
	0x15
};
static DEFINE_PKTUI(small_a2_s0_lpm_wrdisbv, &small_a2_s0_maptbl[LPM_WRDISBV_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(small_a2_s0_lpm_wrdisbv, DSI_PKT_TYPE_WR, SMALL_A2_S0_LPM_WRDISBV, 0);

static u8 SMALL_A2_S0_GAMMA_UPDATE[] = {
	0xF7,
	0x0F
};
static DEFINE_STATIC_PACKET(small_a2_s0_gamma_update, DSI_PKT_TYPE_WR, SMALL_A2_S0_GAMMA_UPDATE, 0);


#if defined(CONFIG_SEC_FACTORY)
static u8 SMALL_A2_S0_SELF_MASK_ENABLE[] = {
	0x7C,
	0x01, 0x00, 0xC7, 0x00, 0xC5, 0x00, 0xC5, 0x00,
	0x00, 0x00, 0x01
};
static DEFINE_STATIC_PACKET(small_a2_s0_self_mask_enable, DSI_PKT_TYPE_WR, SMALL_A2_S0_SELF_MASK_ENABLE, 0);
#else
static u8 SMALL_A2_S0_SELF_MASK_ENABLE[] = {
	0x7C,
	0x01, 0x00, 0xC6, 0x00, 0xC5, 0x00, 0xC5, 0x00,
	0x00, 0x00, 0x01
};
static DEFINE_STATIC_PACKET(small_a2_s0_self_mask_enable, DSI_PKT_TYPE_WR, SMALL_A2_S0_SELF_MASK_ENABLE, 0);

#endif

static u8 SMALL_A2_S0_SELF_MASK_DISABLE[] = {
	0x7C,
	0x00
};
static DEFINE_STATIC_PACKET(small_a2_s0_self_mask_disable, DSI_PKT_TYPE_WR, SMALL_A2_S0_SELF_MASK_DISABLE, 0);

static u8 SMALL_A2_S0_MCD_TEST_ON_1_LT_03[] = {
	0xCB,
	0x22, 0x25, 0x03, 0x01, 0x07, 0x12, 0x48, 0x9E,
	0x1D, 0x10, 0x10, 0xD0, 0xD0, 0xE3, 0xCF, 0x27,
	0x02, 0x06, 0x2B, 0x29, 0x1B, 0x19, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x23, 0x0F, 0xF3, 0xBA, 0x9E,
	0xAB, 0xFF, 0xAF, 0xAF, 0xFF, 0xFF, 0xFF, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0D, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02,
	0x1F, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x11, 0x00, 0x0C, 0x00, 0x00, 0x00,
	0x00, 0x50, 0x4B, 0x27, 0x40, 0x4B, 0x27, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00,
	0x00, 0xF8, 0x02, 0x65, 0x00, 0x00, 0x00, 0xD8,
	0x02, 0x65, 0x00, 0x00, 0x00, 0x50, 0x05, 0x65,
	0x40, 0x05, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x06, 0xF8, 0x02, 0x65, 0x00, 0x00,
	0x00, 0xD8, 0x02, 0x65, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x00, 0x00, 0x17, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 0x08, 0x0C, 0x10, 0x5D,
	0x14, 0x00, 0x5D, 0x14, 0x00, 0x00, 0x00, 0x00
};

static DEFINE_STATIC_PACKET(small_a2_s0_mcd_test_on_1_lt_03, DSI_PKT_TYPE_WR, SMALL_A2_S0_MCD_TEST_ON_1_LT_03, 0);

static u8 SMALL_A2_S0_MCD_TEST_ON_1[] = {
	0xCB,
	0x22, 0x25, 0x03, 0x01, 0x07, 0x12, 0x48, 0x9E,
	0x1D, 0x10, 0x10, 0xD0, 0xD0, 0xE3, 0xCF, 0x27,
	0x02, 0x06, 0x2B, 0x29, 0x1B, 0x19, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x23, 0x0F, 0xF3, 0xBA, 0x9E,
	0xAB, 0xFF, 0xAF, 0xAF, 0xFF, 0xFF, 0xFF, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0D, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02,
	0x27, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x11, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x50, 0x4B, 0x27, 0x40, 0x4B, 0x27, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00,
	0x00, 0xF8, 0x02, 0x65, 0x00, 0x00, 0x00, 0xD8,
	0x02, 0x65, 0x00, 0x00, 0x00, 0x50, 0x05, 0x65,
	0x40, 0x05, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x06, 0xF8, 0x02, 0x65, 0x00, 0x00,
	0x00, 0xD8, 0x02, 0x65, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x00, 0x00, 0x17, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 0x08, 0x0C, 0x10, 0x5D,
	0x14, 0x00, 0x5D, 0x14, 0x00, 0x00, 0x00, 0x00
};

static DEFINE_STATIC_PACKET(small_a2_s0_mcd_test_on_1, DSI_PKT_TYPE_WR, SMALL_A2_S0_MCD_TEST_ON_1, 0);

static u8 SMALL_A2_S0_MCD_TEST_ON_2[] = {
	0xF6,
	0x40
};
static DEFINE_STATIC_PACKET(small_a2_s0_mcd_test_on_2, DSI_PKT_TYPE_WR, SMALL_A2_S0_MCD_TEST_ON_2, 0x1D);

static u8 SMALL_A2_S0_MCD_TEST_OFF_1_LT_03[] = {
	0xCB,
	0x22, 0x25, 0x03, 0x01, 0x07, 0x12, 0x48, 0x9E,
	0x1D, 0x30, 0x2D, 0xF3, 0xEE, 0xE3, 0xCF, 0x27,
	0x02, 0x06, 0x2B, 0x29, 0x1B, 0x19, 0x35, 0x32,
	0x2F, 0x34, 0x31, 0x23, 0x0F, 0xF3, 0xFA, 0x9E,
	0xAB, 0xFF, 0xAF, 0xEF, 0xFF, 0xFF, 0xFF, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0D, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02,
	0x1F, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x11, 0x00, 0x0C, 0x00, 0x00, 0x00,
	0x00, 0x50, 0x4B, 0x27, 0x40, 0x4B, 0x27, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00,
	0x00, 0xF8, 0x02, 0x65, 0x00, 0x00, 0x00, 0xD8,
	0x02, 0x65, 0x00, 0x00, 0x00, 0x50, 0x05, 0x65,
	0x40, 0x05, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xF8, 0x02, 0x65, 0x00, 0x00,
	0x00, 0xD8, 0x02, 0x65, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x00, 0x00, 0x17, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 0x08, 0x0C, 0x10, 0x5D,
	0x14, 0x00, 0x5D, 0x14, 0x00, 0x00, 0x00, 0x00

};
static DEFINE_STATIC_PACKET(small_a2_s0_mcd_test_off_1_lt_03, DSI_PKT_TYPE_WR, SMALL_A2_S0_MCD_TEST_OFF_1_LT_03, 0);

static u8 SMALL_A2_S0_MCD_TEST_OFF_1[] = {
	0xCB,
	0x22, 0x25, 0x03, 0x01, 0x07, 0x12, 0x48, 0x9E,
	0x1D, 0x30, 0x2D, 0xF3, 0xEE, 0xE3, 0xCF, 0x27,
	0x02, 0x06, 0x2B, 0x29, 0x1B, 0x19, 0x35, 0x32,
	0x2F, 0x34, 0x31, 0x23, 0x0F, 0xF3, 0xFA, 0x9E,
	0xAB, 0xFF, 0xAF, 0xEF, 0xFF, 0xFF, 0xFF, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0D, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02,
	0x27, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x11, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x50, 0x4B, 0x27, 0x40, 0x4B, 0x27, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00,
	0x00, 0xF8, 0x02, 0x65, 0x00, 0x00, 0x00, 0xD8,
	0x02, 0x65, 0x00, 0x00, 0x00, 0x50, 0x05, 0x65,
	0x40, 0x05, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xF8, 0x02, 0x65, 0x00, 0x00,
	0x00, 0xD8, 0x02, 0x65, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x00, 0x00, 0x17, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 0x08, 0x0C, 0x10, 0x5D,
	0x14, 0x00, 0x5D, 0x14, 0x00, 0x00, 0x00, 0x00
};
static DEFINE_STATIC_PACKET(small_a2_s0_mcd_test_off_1, DSI_PKT_TYPE_WR, SMALL_A2_S0_MCD_TEST_OFF_1, 0);

static u8 SMALL_A2_S0_MCD_TEST_OFF_2[] = {
	0xF6,
	0x00
};
static DEFINE_STATIC_PACKET(small_a2_s0_mcd_test_off_2, DSI_PKT_TYPE_WR, SMALL_A2_S0_MCD_TEST_OFF_2, 0x1D);

static u8 SMALL_A2_S0_MCD_TEST_LTPS_UPDATE[] = {
	0xF7,
	0x07
};
static DEFINE_STATIC_PACKET(small_a2_s0_mcd_test_ltps_update, DSI_PKT_TYPE_WR, SMALL_A2_S0_MCD_TEST_LTPS_UPDATE, 0);

static DEFINE_COND(small_a2_s0_cond_is_id_lt_03, is_id_lt_03);
static DEFINE_COND(small_a2_s0_cond_is_id_ge_01, is_id_ge_01);
static DEFINE_COND(small_a2_s0_cond_is_smooth_dimming_available, is_smooth_dimming_available);
static DEFINE_COND(small_a2_s0_cond_is_second_pmic_for_fresh_small, is_second_pmic_for_fresh_small);

static struct seqinfo SEQINFO(small_a2_s0_set_bl_param_seq);

static void *small_a2_s0_sleep_out_cmdtbl[] = {
	&DLYINFO(small_a2_s0_wait_20msec),
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&KEYINFO(small_a2_s0_level3_key_enable),

	&PKTINFO(small_a2_s0_osc_1),
	&PKTINFO(small_a2_s0_osc_2),
	&PKTINFO(small_a2_s0_osc_3),
	&PKTINFO(small_a2_s0_osc_4),

	&PKTINFO(small_a2_s0_sleep_out),

	&CONDINFO_IF(small_a2_s0_cond_is_second_pmic_for_fresh_small),
		&DLYINFO(small_a2_s0_wait_10msec),
		&PKTINFO(small_a2_s0_second_pmic_1),
		&PKTINFO(small_a2_s0_second_pmic_2),
		&PKTINFO(small_a2_s0_second_pmic_3),
		&PKTINFO(small_a2_s0_gamma_update),
	&CONDINFO_FI(small_a2_s0_cond_is_second_pmic_for_fresh_small),

	&KEYINFO(small_a2_s0_level3_key_disable),
	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
	&DLYINFO(small_a2_s0_wait_20msec),
};

static void *small_a2_s0_init_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&KEYINFO(small_a2_s0_level3_key_enable),

	&PKTINFO(small_a2_s0_smooth_dimming_init),

	&PKTINFO(small_a2_s0_ffc),
	&PKTINFO(small_a2_s0_self_mask_enable),

	&SEQINFO(small_a2_s0_set_bl_param_seq),

	&PKTINFO(small_a2_s0_te_on),
	&PKTINFO(small_a2_s0_tsp_vsync),

	&KEYINFO(small_a2_s0_level3_key_disable),
	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

#ifdef CONFIG_SUPPORT_HMD
static void *small_a2_s0_hmd_off_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),

	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_hmd_on_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),

	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_hmd_bl_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),

	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};
#endif


static void *small_a2_s0_display_on_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&PKTINFO(small_a2_s0_display_on),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_display_off_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&PKTINFO(small_a2_s0_display_off),
	&DLYINFO(small_a2_s0_wait_1frame),
	&KEYINFO(small_a2_s0_level1_key_disable),
};


static void *small_a2_s0_check_condition_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level2_key_enable),
	&s6e36w5x01_dmptbl[DUMP_RDDPM],
	&KEYINFO(small_a2_s0_level2_key_disable),
};

static void *small_a2_s0_exit_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&s6e36w5x01_dmptbl[DUMP_RDDPM_SLEEP_IN],
#ifdef CONFIG_DISPLAY_USE_INFO
	&s6e36w5x01_dmptbl[DUMP_RDDSM],
	&s6e36w5x01_dmptbl[DUMP_ERR],
	&s6e36w5x01_dmptbl[DUMP_ERR_FG],
	&s6e36w5x01_dmptbl[DUMP_DSI_ERR],
	&s6e36w5x01_dmptbl[DUMP_SELF_DIAG],
#endif
	&KEYINFO(small_a2_s0_level2_key_disable),
	&PKTINFO(small_a2_s0_sleep_in),
	&DLYINFO(small_a2_s0_wait_120msec),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_set_bl_param_cmdtbl[] = {
	&CONDINFO_IF(small_a2_s0_cond_is_smooth_dimming_available),
		&PKTINFO(small_a2_s0_smooth_dimming),
	&CONDINFO_FI(small_a2_s0_cond_is_smooth_dimming_available),
	&PKTINFO(small_a2_s0_hbm_transition),
	&PKTINFO(small_a2_s0_wrdisbv),
	&PKTINFO(small_a2_s0_acl_set),
	&PKTINFO(small_a2_s0_acl_control),
	&PKTINFO(small_a2_s0_elvss_temp),
	&PKTINFO(small_a2_s0_tset),
	&CONDINFO_IF(small_a2_s0_cond_is_id_ge_01),
		&PKTINFO(small_a2_s0_fps),
		&PKTINFO(small_a2_s0_lfd_onoff),
		&PKTINFO(small_a2_s0_lfd_freq),
		&PKTINFO(small_a2_s0_gamma_update),
	&CONDINFO_FI(small_a2_s0_cond_is_id_ge_01),
};

static DEFINE_SEQINFO(small_a2_s0_set_bl_param_seq, small_a2_s0_set_bl_param_cmdtbl);

static void *small_a2_s0_set_bl_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&KEYINFO(small_a2_s0_level3_key_enable),
	&SEQINFO(small_a2_s0_set_bl_param_seq),
	&KEYINFO(small_a2_s0_level3_key_disable),
	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_dummy_cmdtbl[] = {
	NULL,
};


static void *small_a2_s0_res_init_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&KEYINFO(small_a2_s0_level3_key_enable),
	&s6e36w5x01_restbl[RES_COORDINATE],
	&s6e36w5x01_restbl[RES_CODE],
	&s6e36w5x01_restbl[RES_DATE],
	&s6e36w5x01_restbl[RES_OCTA_ID],
	&KEYINFO(small_a2_s0_level3_key_disable),
	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_alpm_enter_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&CONDINFO_IF(small_a2_s0_cond_is_id_ge_01),
		&PKTINFO(small_a2_s0_lpm_wrdisbv),
		&PKTINFO(small_a2_s0_lpm_fps),
		&PKTINFO(small_a2_s0_lpm_lfd_onoff),
		&PKTINFO(small_a2_s0_lpm_lfd_freq),
		&PKTINFO(small_a2_s0_gamma_update),
	&CONDINFO_FI(small_a2_s0_cond_is_id_ge_01),
	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_alpm_exit_cmdtbl[] = {
};

static void *small_a2_s0_mcd_on_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&CONDINFO_IF(small_a2_s0_cond_is_id_lt_03),
		&PKTINFO(small_a2_s0_mcd_test_on_1_lt_03),
	&CONDINFO_EL(small_a2_s0_cond_is_id_lt_03),
		&PKTINFO(small_a2_s0_mcd_test_on_1),
	&CONDINFO_FI(small_a2_s0_cond_is_id_lt_03),
	&PKTINFO(small_a2_s0_mcd_test_on_2),
	&PKTINFO(small_a2_s0_mcd_test_ltps_update),
	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static void *small_a2_s0_mcd_off_cmdtbl[] = {
	&KEYINFO(small_a2_s0_level1_key_enable),
	&KEYINFO(small_a2_s0_level2_key_enable),
	&CONDINFO_IF(small_a2_s0_cond_is_id_lt_03),
		&PKTINFO(small_a2_s0_mcd_test_off_1_lt_03),
	&CONDINFO_EL(small_a2_s0_cond_is_id_lt_03),
		&PKTINFO(small_a2_s0_mcd_test_off_1),
	&CONDINFO_FI(small_a2_s0_cond_is_id_lt_03),
	&PKTINFO(small_a2_s0_mcd_test_off_2),
	&PKTINFO(small_a2_s0_mcd_test_ltps_update),
	&KEYINFO(small_a2_s0_level2_key_disable),
	&KEYINFO(small_a2_s0_level1_key_disable),
};

static struct seqinfo small_a2_s0_seqtbl[MAX_PANEL_SEQ] = {
	[PANEL_SLEEP_OUT_SEQ] = SEQINFO_INIT("sleep-out-seq", small_a2_s0_sleep_out_cmdtbl),
	[PANEL_INIT_SEQ] = SEQINFO_INIT("init-seq", small_a2_s0_init_cmdtbl),
	[PANEL_SET_BL_SEQ] = SEQINFO_INIT("set-bl-seq", small_a2_s0_set_bl_cmdtbl),
	[PANEL_RES_INIT_SEQ] = SEQINFO_INIT("resource-init-seq", small_a2_s0_res_init_cmdtbl),

	[PANEL_CHECK_CONDITION_SEQ] = SEQINFO_INIT("check-condition-seq", small_a2_s0_check_condition_cmdtbl),
	[PANEL_DUMP_SEQ] = SEQINFO_INIT("dump-seq", small_a2_s0_check_condition_cmdtbl),
	[PANEL_DISPLAY_ON_SEQ] = SEQINFO_INIT("display-on-seq", small_a2_s0_display_on_cmdtbl),
	[PANEL_DISPLAY_OFF_SEQ] = SEQINFO_INIT("display-off-seq", small_a2_s0_display_off_cmdtbl),
	[PANEL_EXIT_SEQ] = SEQINFO_INIT("exit-seq", small_a2_s0_exit_cmdtbl),
	[PANEL_MCD_ON_SEQ] = SEQINFO_INIT("mcd-on-seq", small_a2_s0_mcd_on_cmdtbl),
	[PANEL_MCD_OFF_SEQ] = SEQINFO_INIT("mcd-off-seq", small_a2_s0_mcd_off_cmdtbl),
	[PANEL_ALPM_ENTER_SEQ] = SEQINFO_INIT("alpm-enter-seq", small_a2_s0_alpm_enter_cmdtbl),
	[PANEL_ALPM_EXIT_SEQ] = SEQINFO_INIT("alpm-exit-seq", small_a2_s0_alpm_exit_cmdtbl),
	[PANEL_DUMMY_SEQ] = SEQINFO_INIT("dummy-seq", small_a2_s0_dummy_cmdtbl),
};


struct common_panel_info s6e36w5x01_small_panel_info = {
	.ldi_name = "s6e36w5x01",
	.name = "s6e36w5x01_small_a2_s0",
	.model = "AMB675TG01",
	.vendor = "SDC",
	.id = 0x402000,
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
	.maptbl = small_a2_s0_maptbl,
	.nr_maptbl = ARRAY_SIZE(small_a2_s0_maptbl),
	.seqtbl = small_a2_s0_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(small_a2_s0_seqtbl),
	.rditbl = s6e36w5x01_rditbl,
	.nr_rditbl = ARRAY_SIZE(s6e36w5x01_rditbl),
	.restbl = s6e36w5x01_restbl,
	.nr_restbl = ARRAY_SIZE(s6e36w5x01_restbl),
	.dumpinfo = s6e36w5x01_dmptbl,
	.nr_dumpinfo = ARRAY_SIZE(s6e36w5x01_dmptbl),
	.panel_dim_info = {
		[PANEL_BL_SUBDEV_TYPE_DISP] = &s6e36w5x01_a2_s0_panel_dimming_info,
	},
	.test_key = {
		.len = ARRAY_SIZE(SMALL_A2_S0_KEY1_ENABLE),
		.level1_on = SMALL_A2_S0_KEY1_ENABLE,
		.level2_on = SMALL_A2_S0_KEY2_ENABLE,
		.level3_on = SMALL_A2_S0_KEY3_ENABLE,
		.level1_off = SMALL_A2_S0_KEY1_DISABLE,
		.level2_off = SMALL_A2_S0_KEY2_DISABLE,
		.level3_off = SMALL_A2_S0_KEY3_DISABLE,
	},
};

static int __init s6e36w5x01_small_a2_s0_panel_init(void)
{
	register_common_panel(&s6e36w5x01_small_panel_info);
	return 0;
}

arch_initcall(s6e36w5x01_small_a2_s0_panel_init)
#endif /* __S6E36W5X01_SMALL_A2_S0_PANEL_H__ */
