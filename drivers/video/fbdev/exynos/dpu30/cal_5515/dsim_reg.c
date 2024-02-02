/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * SFR access functions for Samsung EXYNOS SoC MIPI-DSI Master driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../dsim.h"
#include "regs-decon.h"
#include "regs-dsim.h"

/* dsim version */
#define DSIM_VER_2P5			0x02050000

/* These definitions are need to guide from AP team */
#define DSIM_STOP_STATE_CNT		0xA
#define DSIM_BTA_TIMEOUT		0xff
#define DSIM_LP_RX_TIMEOUT		0xffff
#define DSIM_MULTI_PACKET_CNT		0xffff
#define DSIM_PLL_STABLE_TIME		0x682A
#define DSIM_FIFOCTRL_THRESHOLD		0x1 /* 1 ~ 32 */
#define DSIM_PH_FIFOCTRL_THRESHOLD	32 /* 1 ~ 32 */

#define PLL_SLEEP_CNT_MULT		450
#define PLL_SLEEP_CNT_MARGIN_RATIO	0	/* 10% ~ 20% */
#define PLL_SLEEP_CNT_MARGIN	(PLL_SLEEP_CNT_MULT * PLL_SLEEP_CNT_MARGIN_RATIO / 100)

/* If below values depend on panel. These values wil be move to panel file.
 * And these values are valid in case of video mode only.
 */
#define DSIM_CMD_ALLOW_VALUE		4
#define DSIM_STABLE_VFP_VALUE		2
#define TE_MARGIN					5 /* 5% */

#define PLL_LOCK_CNT_MULT		500
#define PLL_LOCK_CNT_MARGIN_RATIO	0	/* 10% ~ 20% */
#define PLL_LOCK_CNT_MARGIN	(PLL_LOCK_CNT_MULT * PLL_LOCK_CNT_MARGIN_RATIO / 100)

#define LPRX_BIAS_SLEEP_EN		0x800

u32 DSIM_PHY_BIAS_CON_VAL[] = {
	0x00000010,
	0x00000110,
	0x00003223,
	0x00000000,
	0x00000000,
};

u32 DSIM_PHY_PLL_CON_VAL[] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000500,
	0x0000008E,
	0x00001450,
	0x00000A28,
};

u32 DSIM_PHY_MC_GNR_CON_VAL[] = {
	0x00000000,
	0x00001450,
};
u32 DSIM_PHY_MC_ANA_CON_VAL[] = {
	/* EDGE_CON[14:12] DPHY=3'b111, CPHY=3'b001 */
	0x00007033,
	0x00000004, /* BIAS SLEEP ENABLE */
	0x00000002,
};

/* same value in all master data lane */
u32 DSIM_PHY_MD_GNR_CON_VAL[] = {
	0x00000000,
	0x00001450,
};
/* Need to check reg_cnt */
u32 DSIM_PHY_MD_ANA_CON_VAL[] = {
	0x00007033,
	0x00000004,
	0x00000000, /* BIAS SLEEP ENABLE */
	0x00000000,
};


/* DPHY timing table */
/* below table have to be changed to meet MK DPHY spec*/
const u32 dphy_timing[][10] = {
	/* bps, clk_prepare, clk_zero, clk_post, clk_trail,
	 * hs_prepare, hs_zero, hs_trail, lpx, hs_exit
	 */
	{1500, 7, 23, 6, 6, 7, 9, 6, 5, 9},
	{1490, 59, 24, 6, 69, 59, 10, 70, 44, 9},
	{1480, 59, 23, 6, 68, 58, 10, 70, 44, 9},
	{1470, 58, 23, 6, 68, 58, 10, 69, 44, 9},
	{1460, 58, 23, 6, 68, 58, 10, 69, 43, 9},
	{1450, 58, 23, 6, 67, 57, 10, 68, 43, 8},
	{1440, 57, 23, 6, 67, 57, 9, 68, 43, 8},
	{1430, 57, 22, 6, 67, 56, 9, 68, 42, 8},
	{1420, 56, 22, 6, 66, 56, 9, 67, 42, 8},
	{1410, 56, 22, 6, 66, 56, 9, 67, 42, 8},
	{1400, 56, 22, 5, 66, 55, 9, 67, 41, 8},
	{1390, 55, 22, 5, 65, 55, 9, 66, 41, 8},
	{1380, 55, 22, 5, 65, 54, 9, 66, 41, 8},
	{1370, 54, 21, 5, 64, 54, 9, 66, 41, 8},
	{1360, 54, 21, 5, 64, 54, 9, 65, 40, 8},
	{1350, 54, 21, 5, 64, 53, 9, 65, 40, 8},
	{1340, 53, 21, 5, 63, 53, 9, 65, 40, 8},
	{1330, 53, 21, 5, 63, 53, 8, 64, 39, 8},
	{1320, 52, 21, 5, 63, 52, 8, 64, 39, 8},
	{1310, 52, 20, 5, 62, 52, 8, 64, 39, 8},
	{1300, 51, 20, 5, 62, 51, 8, 63, 38, 7},
	{1290, 51, 20, 5, 62, 51, 8, 63, 38, 7},
	{1280, 51, 20, 5, 61, 51, 8, 63, 38, 7},
	{1270, 50, 20, 5, 61, 50, 8, 62, 38, 7},
	{1260, 50, 19, 5, 61, 50, 8, 62, 37, 7},
	{1250, 49, 19, 5, 60, 49, 8, 62, 37, 7},
	{1240, 49, 19, 5, 60, 49, 8, 61, 37, 7},
	{1230, 49, 19, 5, 60, 49, 8, 61, 36, 7},
	{1220, 48, 19, 5, 59, 48, 7, 61, 36, 7},
	{1210, 48, 18, 5, 59, 48, 7, 60, 36, 7},
	{1200, 47, 18, 4, 59, 48, 7, 60, 35, 7},
	{1190, 47, 18, 4, 58, 47, 7, 60, 35, 7},
	{1180, 47, 18, 4, 58, 47, 7, 59, 35, 7},
	{1170, 46, 18, 4, 58, 46, 7, 59, 35, 7},
	{1160, 46, 17, 4, 57, 46, 7, 59, 34, 6},
	{1150, 45, 17, 4, 57, 46, 7, 58, 34, 6},
	{1140, 45, 17, 4, 57, 45, 7, 58, 34, 6},
	{1130, 45, 17, 4, 56, 45, 7, 58, 33, 6},
	{1120, 44, 17, 4, 56, 44, 7, 57, 33, 6},
	{1110, 44, 17, 4, 56, 44, 6, 57, 33, 6},
	{1100, 43, 16, 4, 55, 44, 6, 57, 32, 6},
	{1090, 43, 16, 4, 55, 43, 6, 56, 32, 6},
	{1080, 43, 16, 4, 55, 43, 6, 56, 32, 6},
	{1070, 42, 16, 4, 54, 43, 6, 56, 32, 6},
	{1060, 42, 16, 4, 54, 42, 6, 55, 31, 6},
	{1050, 41, 16, 4, 54, 42, 6, 55, 31, 6},
	{1040, 41, 15, 4, 53, 41, 6, 54, 31, 6},
	{1030, 41, 15, 4, 53, 41, 6, 54, 30, 6},
	{1020, 40, 15, 4, 53, 41, 6, 54, 30, 6},
	{1010, 40, 15, 4, 52, 40, 6, 53, 30, 5},
	{1000, 39, 15, 3, 52, 40, 5, 53, 29, 5},
	{990, 39, 14, 3, 52, 39, 5, 53, 29, 5},
	{980, 39, 14, 3, 51, 39, 5, 52, 29, 5},
	{970, 38, 14, 3, 51, 39, 5, 52, 29, 5},
	{960, 38, 14, 3, 51, 38, 5, 52, 28, 5},
	{950, 37, 14, 3, 50, 38, 5, 51, 28, 5},
	{940, 37, 13, 3, 50, 38, 5, 51, 28, 5},
	{930, 37, 13, 3, 49, 37, 5, 51, 27, 5},
	{920, 36, 13, 3, 49, 37, 5, 50, 27, 5},
	{910, 36, 13, 3, 49, 36, 5, 50, 27, 5},
	{900, 35, 13, 3, 48, 36, 5, 50, 26, 5},
	{890, 35, 13, 3, 48, 36, 4, 49, 26, 5},
	{880, 35, 12, 3, 48, 35, 4, 49, 26, 5},
	{870, 34, 12, 3, 47, 35, 4, 49, 26, 4},
	{860, 34, 12, 3, 47, 35, 4, 48, 25, 4},
	{850, 33, 12, 3, 47, 34, 4, 48, 25, 4},
	{840, 33, 12, 3, 46, 34, 4, 48, 25, 4},
	{830, 33, 11, 3, 46, 33, 4, 47, 24, 4},
	{820, 32, 11, 3, 46, 33, 4, 47, 24, 4},
	{810, 32, 11, 3, 45, 33, 4, 47, 24, 4},
	{800, 31, 11, 2, 45, 32, 4, 46, 23, 4},
	{790, 31, 11, 2, 45, 32, 4, 46, 23, 4},
	{780, 30, 11, 2, 44, 31, 4, 46, 23, 4},
	{770, 30, 10, 2, 44, 31, 3, 45, 23, 4},
	{760, 30, 10, 2, 44, 31, 3, 45, 22, 4},
	{750, 29, 10, 2, 43, 30, 3, 45, 22, 4},
	{740, 29, 10, 2, 43, 30, 3, 44, 22, 4},
	{730, 28, 10, 2, 43, 30, 3, 44, 21, 4},
	{720, 28, 9, 2, 42, 29, 3, 44, 21, 3},
	{710, 28, 9, 2, 42, 29, 3, 43, 21, 3},
	{700, 27, 9, 2, 42, 28, 3, 43, 20, 3},
	{690, 27, 9, 2, 41, 28, 3, 43, 20, 3},
	{680, 26, 9, 2, 41, 28, 3, 42, 20, 3},
	{670, 26, 9, 2, 41, 27, 3, 42, 20, 3},
	{660, 26, 8, 2, 40, 27, 2, 42, 19, 3},
	{650, 25, 8, 2, 40, 26, 2, 41, 19, 3},
	{640, 25, 8, 2, 40, 26, 2, 41, 19, 3},
	{630, 24, 8, 2, 39, 26, 2, 40, 18, 3},
	{620, 24, 8, 2, 39, 25, 2, 40, 18, 3},
	{610, 24, 7, 2, 39, 25, 2, 40, 18, 3},
	{600, 23, 7, 1, 38, 25, 2, 39, 17, 3},
	{590, 23, 7, 1, 38, 24, 2, 39, 17, 3},
	{580, 22, 7, 1, 38, 24, 2, 39, 17, 2},
	{570, 22, 7, 1, 37, 23, 2, 38, 17, 2},
	{560, 22, 6, 1, 37, 23, 2, 38, 16, 2},
	{550, 21, 6, 1, 37, 23, 1, 38, 16, 2},
	{540, 21, 6, 1, 36, 22, 1, 37, 16, 2},
	{530, 20, 6, 1, 36, 22, 1, 37, 15, 2},
	{520, 20, 6, 1, 35, 21, 1, 37, 15, 2},
	{510, 20, 5, 1, 35, 21, 1, 36, 15, 2},
	{500, 19, 5, 1, 35, 21, 1, 36, 14, 2},
	{490, 19, 5, 1, 34, 20, 1, 36, 14, 2},
	{480, 18, 5, 1, 34, 20, 1, 35, 14, 2},
	{470, 18, 5, 1, 34, 20, 1, 35, 14, 2},
	{460, 18, 5, 1, 33, 19, 1, 35, 13, 2},
	{450, 17, 4, 1, 33, 19, 1, 34, 13, 2},
	{440, 17, 4, 1, 33, 18, 0, 34, 13, 2},
	{430, 16, 4, 1, 32, 18, 0, 34, 12, 1},
	{420, 16, 4, 1, 32, 18, 0, 33, 12, 1},
	{410, 16, 4, 1, 32, 17, 0, 33, 12, 1},
	{400, 15, 4, 0, 31, 17, 0, 33, 11, 1},
	{390, 15, 3, 0, 31, 16, 0, 32, 11, 1},
	{380, 14, 3, 0, 31, 16, 0, 32, 11, 1},
	{370, 14, 3, 0, 30, 16, 0, 32, 11, 1},
	{360, 14, 3, 0, 30, 15, 0, 31, 10, 1},
	{350, 13, 3, 0, 30, 15, 0, 31, 10, 1},
	{340, 13, 2, 0, 29, 15, 0, 31, 10, 1},
	{330, 12, 2, 0, 29, 14, 0, 30, 9, 1},
	{320, 12, 2, 0, 29, 14, 0, 30, 9, 1},
	{310, 12, 2, 0, 28, 13, 0, 30, 9, 1},
	{300, 11, 2, 0, 28, 13, 0, 29, 8, 1},
	{290, 11, 1, 0, 28, 13, 0, 29, 8, 0},
	{280, 10, 1, 0, 27, 12, 0, 29, 8, 0},
	{270, 10, 1, 0, 27, 12, 0, 28, 8, 0},
	{260, 9, 1, 0, 27, 11, 0, 28, 7, 0},
	{250, 9, 1, 0, 26, 11, 0, 28, 7, 0},
	{240, 9, 1, 0, 26, 11, 0, 27, 7, 0},
	{230, 8, 0, 0, 26, 10, 0, 27, 6, 0},
	{220, 8, 0, 0, 25, 10, 0, 27, 6, 0},
	{210, 7, 0, 0, 25, 10, 0, 26, 6, 0},
	{200, 7, 0, 0, 25, 9, 0, 26, 5, 0},
	{190, 7, 0, 0, 24, 9, 0, 25, 5, 0},
	{180, 6, 0, 0, 24, 8, 0, 25, 5, 0},
	{170, 6, 0, 0, 24, 8, 0, 25, 5, 0},
	{160, 5, 0, 0, 23, 8, 0, 24, 4, 0},
	{150, 5, 0, 0, 23, 7, 0, 24, 4, 0},
	{140, 5, 0, 0, 23, 7, 0, 24, 4, 0},
	{130, 4, 0, 0, 22, 6, 0, 23, 3, 0},
	{120, 4, 0, 0, 22, 6, 0, 23, 3, 0},
	{110, 3, 0, 0, 22, 6, 0, 23, 3, 0},
	{100, 3, 0, 0, 21, 5, 0, 22, 2, 0},
	{90, 3, 0, 0, 21, 5, 0, 22, 2, 0},
	{80, 2, 0, 0, 20, 5, 0, 22, 2, 0},
};

const u32 b_dphyctl[14] = {
	0x0af, 0x0c8, 0x0e1, 0x0fa,			/* esc 7 ~ 10 */
	0x113, 0x12c, 0x145, 0x15e, 0x177,	/* esc 11 ~ 15 */
	0x190, 0x1a9, 0x1c2, 0x1db, 0x1f4	/* esc 16 ~ 20 */
};

/***************************** DPHY CAL functions *******************************/
static u32 dsim_reg_get_version(u32 id)
{
	u32 version;

	version = dsim_read(id, DSIM_VERSION);

	return version;
}

#if defined(CONFIG_EXYNOS_DSIM_DITHER)
static void dsim_reg_set_dphy_dither_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON4, val, DSIM_PHY_DITHER_EN);
}
#endif

#ifdef DPHY_LOOP
void dsim_reg_set_dphy_loop_back_test(u32 id)
{
	dsim_phy_write_mask(id, 0x0370, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0470, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0570, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0670, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0770, 1, (0x3 << 0));
}

static void dsim_reg_set_dphy_loop_test(u32 id)
{
	dsim_phy_write_mask(id, 0x0374, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0474, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0574, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0674, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0774, ~0, (1 << 3));

	dsim_phy_write_mask(id, 0x0374, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0474, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0574, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0674, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0774, 0x6, (0x7 << 0));

	dsim_phy_write_mask(id, 0x037c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x047c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x057c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x067c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x077c, 0x2, (0xffff << 0));
}
#endif

static void dsim_reg_set_dphy_wclk_buf_sft(u32 id, u32 cnt)
{
	u32 val = DSIM_PHY_WCLK_BUF_SFT_CNT(cnt);

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON6, val,
			DSIM_PHY_WCLK_BUF_SFT_CNT_MASK);
}

/* DPHY setting */
static void dsim_reg_set_pll_freq(u32 id, u32 p, u32 m, u32 s, u32 k)
{
	u32 val, mask;

	/* K value */
	val = DSIM_PHY_PMS_K(k);
	dsim_phy_write(id, DSIM_PHY_PLL_CON1, val);

	/* P & S value */
	val = DSIM_PHY_PMS_P(p) | DSIM_PHY_PMS_S(s);
	mask = DSIM_PHY_PMS_P_MASK | DSIM_PHY_PMS_S_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON0, val, mask);

	/* M value */
	val = DSIM_PHY_PMS_M(m);
	mask = DSIM_PHY_PMS_M_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON2, val, mask);
}

static void dsim_reg_set_dphy_timing_values(u32 id,
			struct dphy_timing_value *t, u32 hsmode)
{
	u32 val;
	u32 hs_en, skewcal_en;
	u32 i;

	/* HS mode setting */
	if (hsmode) {
		/* under 1500Mbps : don't need SKEWCAL enable */
		hs_en = 1;
		skewcal_en = 0;
	} else {
		/* above 1500Mbps : need SKEWCAL enable */
		hs_en = 0;
		skewcal_en = 1;
	}

	/* clock lane setting */
	val = DSIM_PHY_ULPS_EXIT(t->b_dphyctl);
	dsim_phy_write(id, DSIM_PHY_MC_TIME_CON4, val);

	val = DSIM_PHY_HSTX_CLK_SEL(hs_en) | DSIM_PHY_TLPX(t->lpx);
	dsim_phy_write(id, DSIM_PHY_MC_TIME_CON0, val);

	/* skew cal implementation : disable */
	val = skewcal_en;
	dsim_phy_write(id, DSIM_PHY_MC_DESKEW_CON0, val);
	/* add 'run|init_run|wait_run time' if skewcal is enabled */

	val = DSIM_PHY_TCLK_PREPARE(t->clk_prepare) |
		DSIM_PHY_TCLK_ZERO(t->clk_zero);
	dsim_phy_write(id, DSIM_PHY_MC_TIME_CON1, val);

	val = DSIM_PHY_THS_EXIT(t->hs_exit) | DSIM_PHY_TCLK_TRAIL(t->clk_trail);
	dsim_phy_write(id, DSIM_PHY_MC_TIME_CON2, val);

	val = DSIM_PHY_TCLK_POST(t->clk_post);
	dsim_phy_write(id, DSIM_PHY_MC_TIME_CON3, val);

	/* add other clock lane setting if necessary */

	/* data lane setting : D0 ~ D1 */
	for (i = 0; i < 2; i++) {
		val = DSIM_PHY_ULPS_EXIT(t->b_dphyctl);
		dsim_phy_write(id, DSIM_PHY_MD_TIME_CON4(i), val);

		val = DSIM_PHY_HSTX_CLK_SEL(hs_en) | DSIM_PHY_TLPX(t->lpx) |
			DSIM_PHY_TLP_EXIT_SKEW(0) | DSIM_PHY_TLP_ENTRY_SKEW(0);
		dsim_phy_write(id, DSIM_PHY_MD_TIME_CON0(i), val);

		/* skew cal implementation later */
		val = DSIM_PHY_THS_PREPARE(t->hs_prepare) |
			DSIM_PHY_THS_ZERO(t->hs_zero);
		dsim_phy_write(id, DSIM_PHY_MD_TIME_CON1(i), val);

		val = DSIM_PHY_THS_EXIT(t->hs_exit) |
			DSIM_PHY_THS_TRAIL(t->hs_trail);
		dsim_phy_write(id, DSIM_PHY_MD_TIME_CON2(i), val);

		val = DSIM_PHY_TTA_GET(3) | DSIM_PHY_TTA_GO(0);
		dsim_phy_write(id, DSIM_PHY_MD_TIME_CON3(i), val);

		/* add other clock lane setting if necessary */
	}
}

#if defined(CONFIG_EXYNOS_DSIM_DITHER)
static void dsim_reg_set_dphy_param_dither(u32 id, struct stdphy_pms *dphy_pms)
{
	u32 val, mask;

	/* MFR & MRR*/
	val = DSIM_PHY_DITHER_MFR(dphy_pms->mfr)
		| DSIM_PHY_DITHER_MRR(dphy_pms->mrr);
	dsim_phy_write(id, DSIM_PHY_PLL_CON3, val);

	/* SEL_PF & ICP */
	val = DSIM_PHY_DITHER_SEL_PF(dphy_pms->sel_pf)
		| DSIM_PHY_DITHER_ICP(dphy_pms->icp);
	mask = DSIM_PHY_DITHER_SEL_PF_MASK | DSIM_PHY_DITHER_ICP_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON5, val, mask);

	/* AFC_ENB & EXTAFC & FSEL * RSEL*/
	val = (DSIM_PHY_DITHER_AFC_ENB((dphy_pms->afc_enb) ? ~0 : 0))
		| DSIM_PHY_DITHER_EXTAFC(dphy_pms->extafc)
		| DSIM_PHY_DITHER_FSEL(((dphy_pms->fsel) ? ~0 : 0))
		| DSIM_PHY_DITHER_RSEL(dphy_pms->rsel);
	mask = DSIM_PHY_DITHER_AFC_ENB_MASK | DSIM_PHY_DITHER_EXTAFC_MASK
		| DSIM_PHY_DITHER_FSEL_MASK | DSIM_PHY_DITHER_RSEL_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON4, val, mask);

	/* FEED_EN */
	val = ((dphy_pms->feed_en) ? ~0 : 0) | ((dphy_pms->fout_mask) ? ~0 : 0);
	mask = DSIM_PHY_DITHER_FEED_EN | DSIM_PHY_DITHER_FOUT_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON2, val, mask);
}
#endif

/* BIAS Block Control Register */
static void dsim_reg_set_bias_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < 5; i++)
		dsim_phy_write(id, DSIM_PHY_BIAS_CON(i), blk_ctl[i]);
}

/* PLL Control Register */
static void dsim_reg_set_pll_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(DSIM_PHY_PLL_CON_VAL); i++)
		dsim_phy_write(id, DSIM_PHY_PLL_CON(i), blk_ctl[i]);
}

/* Master Clock Lane General Control Register */
static void dsim_reg_set_mc_gnr_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < 2; i++)
		dsim_phy_write(id, DSIM_PHY_MC_GNR_CON(i), blk_ctl[i]);
}

/* Master Clock Lane Analog Block Control Register */
static void dsim_reg_set_mc_ana_con(u32 id, u32 *blk_ctl)
{
	u32 i;
	u32 reg_cnt = 2;

	for (i = 0; i < reg_cnt; i++)
		dsim_phy_write(id, DSIM_PHY_MC_ANA_CON(i), blk_ctl[i]);
}

/* Master Data Lane General Control Register */
static void dsim_reg_set_md_gnr_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < MAX_DSIM_DATALANE_CNT; i++) {
		dsim_phy_write(id, DSIM_PHY_MD_GNR_CON0(i), blk_ctl[0]);
		dsim_phy_write(id, DSIM_PHY_MD_GNR_CON1(i), blk_ctl[1]);
	}
}

/* Master Data Lane Analog Block Control Register */
static void dsim_reg_set_md_ana_con(u32 id, u32 *blk_ctl)
{
	u32 i;
	u32 blk_con2;

	for (i = 0; i < MAX_DSIM_DATALANE_CNT; i++) {
		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON0(i), blk_ctl[0]);
		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON1(i), blk_ctl[1]);

		/* Only data lane 0 has LPRX BIAS SLEEP function */
		if (i == 0)
			blk_con2 = blk_ctl[2] | LPRX_BIAS_SLEEP_EN;

		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON2(i), blk_ctl[2]);
		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON3(i), blk_ctl[3]);
	}
}

#ifdef DPDN_INV_SWAP
void dsim_reg_set_inv_dpdn(u32 id, u32 inv_clk, u32 inv_data[4])
{
	u32 i;
	u32 val, mask;

	val = inv_clk ? (DSIM_PHY_CLK_INV) : 0;
	mask = DSIM_PHY_CLK_INV;
	dsim_phy_write_mask(id, DSIM_PHY_MC_DATA_CON0, val, mask);

	for (i = 0; i < MAX_DSIM_DATALANE_CNT; i++) {
		val = inv_data[i] ? (DSIM_PHY_DATA_INV) : 0;
		mask = DSIM_PHY_DATA_INV;
		dsim_phy_write_mask(id,  DSIM_PHY_MD_DATA_CON0(i), val, mask);
	}
}

static void dsim_reg_set_dpdn_swap(u32 id, u32 clk_swap)
{
	u32 val, mask;

	val = DSIM_PHY_DPDN_SWAP(clk_swap);
	mask = DSIM_PHY_DPDN_SWAP_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_ANA_CON1, val, mask);
}
#endif

/******************* DSIM CAL functions *************************/
static void dsim_reg_sw_reset(u32 id)
{
	u32 cnt = 200;
	u32 state;

	dsim_write_mask(id, DSIM_SWRST, ~0, DSIM_SWRST_RESET);

	do {
		state = dsim_read(id, DSIM_SWRST) & DSIM_SWRST_RESET;
		cnt--;
		udelay(10);
	} while (state && cnt);

	if (!cnt)
		dsim_err("dsim%d sw reset timeout\n", id);
}

static void dsim_reg_set_config_options(u32 id, u32 lane,
		u32 eotp_en, u32 per_frame_read, u32 pix_format, u32 vc_id)
{
	u32 val, mask;

	val = DSIM_CONFIG_NUM_OF_DATA_LANE(lane) | DSIM_CONFIG_EOTP_EN(eotp_en)
		| DSIM_CONFIG_PER_FRAME_READ_EN(per_frame_read)
		| DSIM_CONFIG_PIXEL_FORMAT(pix_format)
		| DSIM_CONFIG_VC_ID(vc_id);

	mask = DSIM_CONFIG_NUM_OF_DATA_LANE_MASK | DSIM_CONFIG_EOTP_EN_MASK
		| DSIM_CONFIG_PER_FRAME_READ_EN_MASK
		| DSIM_CONFIG_PIXEL_FORMAT_MASK
		| DSIM_CONFIG_VC_ID_MASK;

	dsim_write_mask(id, DSIM_CONFIG, val, mask);
}

static void dsim_reg_enable_lane(u32 id, u32 lane, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_LANES_EN(lane));
}

static int dsim_reg_wait_phy_ready_clane(u32 id, bool en)
{
	u32 cnt = 200;
	u32 val;

	do {
		val = dsim_phy_read(id, DSIM_PHY_MC_GNR_CON0);
		if (en == DSIM_PHY_PHY_READY_GET(val))
			break;
		cnt--;
		udelay(10);
	} while (cnt);

	if (!cnt) {
		dsim_err("PHY clock lane is not ready[timeout]\n");
		return -EBUSY;
	}

	return 0;
}

static int dsim_reg_wait_phy_ready_dlane(u32 id, u32 lane_id, bool en)
{
	u32 cnt = 200;
	u32 val, reg_id;

	reg_id = DSIM_PHY_MD_GNR_CON0(lane_id);

	do {
		val = dsim_phy_read(id, reg_id);
		if (en == DSIM_PHY_PHY_READY_GET(val))
			break;
		cnt--;
		udelay(10);
	} while (cnt);

	if (!cnt) {
		dsim_err("PHY data lane%d is not ready[timeout]\n", lane_id);
		return -EBUSY;
	}

	return 0;
}

static int dsim_reg_enable_lane_phy(u32 id, u32 lane, bool en)
{
	u32 i, lane_cnt = 0;
	u32 reg_id;
	u32 ret = 0;
	u32 val = en ? ~0 : 0;

	/* check enabled data lane count */
	lane_cnt = hweight32(lane);

	/*
	 * [step1] enable phy_enable
	 */

	/* (1.1) clock lane on|off */
	reg_id = DSIM_PHY_MC_GNR_CON0;
	dsim_phy_write(id, reg_id, val);

	/* (1.2) data lane on|off */
	for (i = 0; i < lane_cnt; i++) {
		reg_id = DSIM_PHY_MD_GNR_CON0(i);
		dsim_phy_write(id, reg_id, val);
	}

	/*
	 * [step2] wait for phy_ready
	 */

	/* (2.1) check ready of clock lane */
	if (dsim_reg_wait_phy_ready_clane(id, en))
		ret++;

	/* (2.2) check ready of data lanes (index : from '1') */
	for (i = 0; i < lane_cnt; i++)
		if (dsim_reg_wait_phy_ready_dlane(id, i, en))
			ret++;

	if (ret) {
		dsim_err("Error to enable PHY lane(err=%d)\n", ret);
		return -EBUSY;
	} else
		return 0;
}

static void dsim_reg_pll_stable_time(u32 id, u32 lock_cnt)
{
	u32 val;

	val = DSIM_PHY_PLL_LOCK_CNT(lock_cnt);
	dsim_phy_write(id, DSIM_PHY_PLL_CON7, val);
}

static void dsim_reg_set_pll(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON0, val, DSIM_PHY_PLL_EN_MASK);
}

static u32 dsim_reg_is_pll_stable(u32 id)
{
	u32 val, pll_lock;

	val = dsim_phy_read(id, DSIM_PHY_PLL_STAT0);
	pll_lock = DSIM_PHY_PLL_LOCK_GET(val);
	if (pll_lock)
		return 1;

	return 0;
}

static int dsim_reg_enable_pll(u32 id, u32 en)
{
	u32 cnt = 200;

	if (en)
		dsim_reg_clear_int(id, DSIM_INTSRC_PLL_STABLE);

	dsim_reg_set_pll(id, en);

	do {
		if (en == dsim_reg_is_pll_stable(id))
			break;
		cnt--;
		udelay(10);
	} while (cnt);

	if (!cnt) {
		dsim_err("dsim%d wait phy pll stable\n", id);
		return -EBUSY;
	}

	return 0;
}

static void dsim_reg_set_esc_clk_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_ESCCLK_EN);
}

static void dsim_reg_set_esc_clk_prescaler(u32 id, u32 en, u32 p)
{
	u32 val = en ? DSIM_CLK_CTRL_ESCCLK_EN : 0;
	u32 mask = DSIM_CLK_CTRL_ESCCLK_EN | DSIM_CLK_CTRL_ESC_PRESCALER_MASK;

	val |= DSIM_CLK_CTRL_ESC_PRESCALER(p);
	dsim_write_mask(id, DSIM_CLK_CTRL, val, mask);
}

static void dsim_reg_set_esc_clk_on_lane(u32 id, u32 en, u32 lane)
{
	u32 val;

	lane = (lane >> 1) | (1 << 4);

	val = en ? DSIM_CLK_CTRL_LANE_ESCCLK_EN(lane) : 0;
	dsim_write_mask(id, DSIM_CLK_CTRL, val,
				DSIM_CLK_CTRL_LANE_ESCCLK_EN_MASK);
}

static void dsim_reg_set_stop_state_cnt(u32 id)
{
	u32 val = DSIM_ESCMODE_STOP_STATE_CNT(DSIM_STOP_STATE_CNT);

	dsim_write_mask(id, DSIM_ESCMODE, val,
				DSIM_ESCMODE_STOP_STATE_CNT_MASK);
}

static void dsim_reg_set_timeout(u32 id)
{
	u32 val = DSIM_TIMEOUT_BTA_TOUT(DSIM_BTA_TIMEOUT)
		| DSIM_TIMEOUT_LPRX_TOUT(DSIM_LP_RX_TIMEOUT);

	dsim_write(id, DSIM_TIMEOUT, val);
}

static void dsim_reg_disable_hsa(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HSA_DISABLE);
}

static void dsim_reg_disable_hbp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HBP_DISABLE);
}

static void dsim_reg_disable_hfp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HFP_DISABLE);
}

static void dsim_reg_disable_hse(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HSE_DISABLE);
}

static void dsim_reg_set_burst_mode(u32 id, u32 burst)
{
	u32 val = burst ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_BURST_MODE);
}

static void dsim_reg_set_sync_inform(u32 id, u32 inform)
{
	u32 val = inform ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_SYNC_INFORM);
}

#if defined(CONFIG_EXYNOS_PLL_SLEEP)
static void dsim_reg_set_pll_sleep_enable(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PLL_SLEEP);
}
#endif

/* 0=D-PHY, 1=C-PHY */
void dsim_reg_set_phy_selection(u32 id, u32 sel)
{
	u32 val = sel ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PHY_SELECTION);
}

static void dsim_reg_set_vporch(u32 id, u32 vbp, u32 vfp)
{
	u32 val;

	val = DSIM_VPORCH_VFP_TOTAL(vfp) | DSIM_VPORCH_VBP(vbp);
	dsim_write(id, DSIM_VPORCH, val);
}

static void dsim_reg_set_vfp_detail(u32 id, u32 cmd_allow, u32 stable_vfp)
{
	u32 val = DSIM_VPORCH_VFP_CMD_ALLOW(cmd_allow)
		| DSIM_VPORCH_STABLE_VFP(stable_vfp);
	dsim_write(id, DSIM_VFP_DETAIL, val);
}

static void dsim_reg_set_hporch(u32 id, u32 hbp, u32 hfp)
{
	u32 val;

	val = DSIM_HPORCH_HFP(hfp) | DSIM_HPORCH_HBP(hbp);
	dsim_write(id, DSIM_HPORCH, val);
}

static void dsim_reg_set_sync_area(u32 id, u32 vsa, u32 hsa)
{
	u32 val = DSIM_SYNC_VSA(vsa) | DSIM_SYNC_HSA(hsa);

	dsim_write(id, DSIM_SYNC, val);
}

static void dsim_reg_set_resol(u32 id, struct exynos_panel_info *lcd_info)
{
	u32 height, width, val;

	if (lcd_info->dsc.en)
		width = lcd_info->dsc.enc_sw * lcd_info->dsc.slice_num;
	else
		width = lcd_info->xres;

	height = lcd_info->yres;

	val = DSIM_RESOL_VRESOL(height) | DSIM_RESOL_HRESOL(width);

	dsim_write(id, DSIM_RESOL, val);
}

static void dsim_reg_set_porch(u32 id, struct exynos_panel_info *lcd)
{
	if (lcd->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_vporch(id, lcd->vbp, lcd->vfp);
		dsim_reg_set_vfp_detail(id,
				DSIM_CMD_ALLOW_VALUE,
				DSIM_STABLE_VFP_VALUE);
		dsim_reg_set_hporch(id, lcd->hbp, lcd->hfp);
		dsim_reg_set_sync_area(id, lcd->vsa, lcd->hsa);
	}
}

static void dsim_reg_set_vt_htiming0(u32 id, u32 hsa_period, u32 hact_period)
{
	u32 val = DSIM_VT_HTIMING0_HSA_PERIOD(hsa_period)
		| DSIM_VT_HTIMING0_HACT_PERIOD(hact_period);

	dsim_write(id, DSIM_VT_HTIMING0, val);
}

static void dsim_reg_set_vt_htiming1(u32 id, u32 hfp_period, u32 hbp_period)
{
	u32 val = DSIM_VT_HTIMING1_HFP_PERIOD(hfp_period)
		| DSIM_VT_HTIMING1_HBP_PERIOD(hbp_period);

	dsim_write(id, DSIM_VT_HTIMING1, val);
}

static void dsim_reg_set_hperiod(u32 id, struct exynos_panel_info *lcd)
{
	u32 vclk,  wclk;
	u32 hblank, vblank;
	u32 width, height;
	u32 hact_period, hsa_period, hbp_period, hfp_period;

	if (lcd->dsc.en)
		width = lcd->dsc.enc_sw * lcd->dsc.slice_num;
	else
		width = lcd->xres;

	height = lcd->yres;

	if (lcd->mode == DECON_VIDEO_MODE) {
		hblank = lcd->hsa + lcd->hbp + lcd->hfp;
		vblank = lcd->vsa + lcd->vbp + lcd->vfp;
		vclk = DIV_ROUND_CLOSEST((width + hblank) * (height + vblank) *
				lcd->fps, 1000000);
		wclk = DIV_ROUND_CLOSEST(lcd->hs_clk, 16);

		/* round calculation to reduce fps error */
		hact_period = DIV_ROUND_CLOSEST(width * wclk, vclk);
		hsa_period = DIV_ROUND_CLOSEST(lcd->hsa * wclk, vclk);
		hbp_period = DIV_ROUND_CLOSEST(lcd->hbp * wclk, vclk);
		hfp_period = DIV_ROUND_CLOSEST(lcd->hfp * wclk, vclk);

		dsim_reg_set_vt_htiming0(id, hsa_period, hact_period);
		dsim_reg_set_vt_htiming1(id, hfp_period, hbp_period);
	}
}

static void dsim_reg_set_video_mode(u32 id, u32 mode)
{
	u32 val = mode ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_VIDEO_MODE);
}

static int dsim_reg_wait_idle_status(u32 id, u32 is_vm)
{
	u32 cnt = 1000;
	u32 reg_id, val, status;

	if (is_vm) {
		reg_id = DSIM_LINK_STATUS0;

		do {
			val = dsim_read(id, reg_id);
			status = DSIM_LINK_STATUS0_VIDEO_MODE_STATUS_GET(val);
			if (status == DSIM_STATUS_IDLE)
				break;
			cnt--;
			udelay(10);
		} while (cnt);

		if (!cnt) {
			dsim_err("dsim%d wait timeout idle status(%u)\n", id, status);
			return -EBUSY;
		}
	}

	return 0;
}

/* 0 = command, 1 = video mode */
static u32 dsim_reg_get_display_mode(u32 id)
{
	u32 val;

	val = dsim_read(id, DSIM_CONFIG);
	return DSIM_CONFIG_DISPLAY_MODE_GET(val);
}

enum dsim_datalane_status dsim_reg_get_datalane_status(u32 id)
{
	u32 val;

	val = dsim_read(id, DSIM_LINK_STATUS2);
	return DSIM_LINK_STATUS2_DATALANE_STATUS_GET(val);
}

static void dsim_reg_enable_dsc(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_CPRS_EN);
}

static void dsim_reg_set_cprl_ctrl(u32 id, struct exynos_panel_info *lcd_info)
{
	u32 multi_slice = 1;
	u32 val;

	/* if multi-slice(2~4 slices) DSC compression is used in video mode
	 * MULTI_SLICE_PACKET configuration must be matched
	 * to DDI's configuration
	 */
	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE)
		multi_slice = 1;
	else if (lcd_info->mode == DECON_VIDEO_MODE)
		multi_slice = lcd_info->dsc.slice_num > 1 ? 1 : 0;

	/* if MULTI_SLICE_PACKET is enabled,
	 * only one packet header is transferred
	 * for multi slice
	 */
	val = DSIM_CPRS_CTRL_MULI_SLICE_PACKET(multi_slice)
		| DSIM_CPRS_CTRL_NUM_OF_SLICE(lcd_info->dsc.slice_num);

	dsim_write(id, DSIM_CPRS_CTRL, val);
}

static void dsim_reg_set_num_of_slice(u32 id, u32 num_of_slice)
{
	u32 val = DSIM_CPRS_CTRL_NUM_OF_SLICE(num_of_slice);

	dsim_write_mask(id, DSIM_CPRS_CTRL, val,
				DSIM_CPRS_CTRL_NUM_OF_SLICE_MASK);
}

static void dsim_reg_get_num_of_slice(u32 id, u32 *num_of_slice)
{
	u32 val = dsim_read(id, DSIM_CPRS_CTRL);

	*num_of_slice = DSIM_CPRS_CTRL_NUM_OF_SLICE_GET(val);
}

static void dsim_reg_set_multi_slice(u32 id, struct exynos_panel_info *lcd_info)
{
	u32 multi_slice = 1;
	u32 val;

	/* if multi-slice(2~4 slices) DSC compression is used in video mode
	 * MULTI_SLICE_PACKET configuration must be matched
	 * to DDI's configuration
	 */
	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE)
		multi_slice = 1;
	else if (lcd_info->mode == DECON_VIDEO_MODE)
		multi_slice = lcd_info->dsc.slice_num > 1 ? 1 : 0;

	/* if MULTI_SLICE_PACKET is enabled,
	 * only one packet header is transferred
	 * for multi slice
	 */
	val = multi_slice ? ~0 : 0;
	dsim_write_mask(id, DSIM_CPRS_CTRL, val,
				DSIM_CPRS_CTRL_MULI_SLICE_PACKET_MASK);
}

static void dsim_reg_set_size_of_slice(u32 id, struct exynos_panel_info *lcd_info)
{
	u32 slice_w = lcd_info->dsc.dec_sw;
	u32 val_01 = 0;
	u32 val_23 = 0;

	if (lcd_info->dsc.slice_num == 4) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE1(slice_w) |
			DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);
		val_23 = DSIM_SLICE23_SIZE_OF_SLICE3(slice_w) |
			DSIM_SLICE23_SIZE_OF_SLICE2(slice_w);

		dsim_write(id, DSIM_SLICE01, val_01);
		dsim_write(id, DSIM_SLICE23, val_23);
	} else if (lcd_info->dsc.slice_num == 2) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE1(slice_w) |
			DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);

		dsim_write(id, DSIM_SLICE01, val_01);
	} else if (lcd_info->dsc.slice_num == 1) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);

		dsim_write(id, DSIM_SLICE01, val_01);
	} else {
		dsim_err("not supported slice mode. dsc(%d), slice(%d)\n",
				lcd_info->dsc.cnt, lcd_info->dsc.slice_num);
	}
}

static void dsim_reg_print_size_of_slice(u32 id)
{
	u32 val;
	u32 slice0_w, slice1_w, slice2_w, slice3_w;

	val = dsim_read(id, DSIM_SLICE01);
	slice0_w = DSIM_SLICE01_SIZE_OF_SLICE0_GET(val);
	slice1_w = DSIM_SLICE01_SIZE_OF_SLICE1_GET(val);

	val = dsim_read(id, DSIM_SLICE23);
	slice2_w = DSIM_SLICE23_SIZE_OF_SLICE2_GET(val);
	slice3_w = DSIM_SLICE23_SIZE_OF_SLICE3_GET(val);

	dsim_dbg("dsim%d: slice0 w(%d), slice1 w(%d), slice2 w(%d), slice3(%d)\n",
			id, slice0_w, slice1_w, slice2_w, slice3_w);
}

static void dsim_reg_set_multi_packet_count(u32 id, u32 multipacketcnt)
{
	u32 val = DSIM_CMD_CONFIG_MULTI_PKT_CNT(multipacketcnt);

	dsim_write_mask(id, DSIM_CMD_CONFIG, val,
				DSIM_CMD_CONFIG_MULTI_PKT_CNT_MASK);
}

static void dsim_reg_set_cmd_te_ctrl0(u32 id, u32 stablevfp)
{
	u32 val = DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP(stablevfp);

	dsim_write(id, DSIM_CMD_TE_CTRL0, val);
}

static void dsim_reg_set_cmd_te_ctrl1(u32 id, u32 teprotecton, u32 tetout)
{
	u32 val = DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON(teprotecton)
		| DSIM_CMD_TE_CTRL1_TIME_TE_TOUT(tetout);

	dsim_write(id, DSIM_CMD_TE_CTRL1, val);
}

static void dsim_reg_get_cmd_timer(unsigned int fps, unsigned int *te_protect,
		unsigned int *te_timeout, u32 hs_clk)
{
	*te_protect = hs_clk * (100 - TE_MARGIN) * 100 / fps / 16;
	*te_timeout = hs_clk * (100 + TE_MARGIN * 2) * 100 / fps / 16;
}

void dsim_reg_set_cmd_ctrl(u32 id, struct exynos_panel_info *lcd_info,
						struct dsim_clks *clks)
{
	unsigned int time_stable_vfp;
	unsigned int time_te_protect_on;
	unsigned int time_te_tout;

	if (lcd_info->dsc.en)
		time_stable_vfp = lcd_info->xres * DSIM_STABLE_VFP_VALUE / 100;
	else
		time_stable_vfp = lcd_info->xres * DSIM_STABLE_VFP_VALUE * 3 / 100;
	dsim_reg_get_cmd_timer(lcd_info->fps, &time_te_protect_on,
			&time_te_tout, clks->hs_clk);
	dsim_reg_set_cmd_te_ctrl0(id, time_stable_vfp);
	dsim_reg_set_cmd_te_ctrl1(id, time_te_protect_on, time_te_tout);
}

static void dsim_reg_enable_noncont_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val,
				DSIM_CLK_CTRL_NONCONT_CLOCK_LANE);
}

static void dsim_reg_enable_clocklane(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val,
				DSIM_CLK_CTRL_CLKLANE_ONOFF);
}

void dsim_reg_enable_packetgo(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CMD_CONFIG, val,
				DSIM_CMD_CONFIG_PKT_GO_EN);
}

void dsim_reg_set_packetgo_ready(u32 id)
{
	dsim_write_mask(id, DSIM_CMD_CONFIG, DSIM_CMD_CONFIG_PKT_GO_RDY,
				DSIM_CMD_CONFIG_PKT_GO_RDY);
}

static void dsim_reg_enable_multi_cmd_packet(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CMD_CONFIG, val,
				DSIM_CMD_CONFIG_MULTI_CMD_PKT_EN);
}

/*
 * 0 = Disable shadow update (applied to Operating_SFR directly)
 * 1 = Enable shadow update (applied to Operating_SFR based on protocol)
 */
static void dsim_reg_enable_shadow(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_SFR_CTRL, val,
				DSIM_SFR_CTRL_SHADOW_EN);
}

static void dsim_reg_set_link_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_CLOCK_SEL);
}

static int dsim_reg_get_link_clock(u32 id)
{
	int val = 0;

	val = dsim_read_mask(id, DSIM_CLK_CTRL, DSIM_CLK_CTRL_CLOCK_SEL);

	return val;
}

static void dsim_reg_enable_hs_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_TX_REQUEST_HSCLK);
}

static void dsim_reg_enable_word_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_WORDCLK_EN);
}

u32 dsim_reg_is_hs_clk_ready(u32 id)
{
	if (dsim_read(id, DSIM_DPHY_STATUS) & DSIM_DPHY_STATUS_TX_READY_HSCLK)
		return 1;

	return 0;
}

u32 dsim_reg_is_clk_stop(u32 id)
{
	if (dsim_read(id, DSIM_DPHY_STATUS) & DSIM_DPHY_STATUS_STOPSTATE_CLK)
		return 1;

	return 0;
}

void dsim_reg_enable_per_frame_read(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val,
			DSIM_CONFIG_PER_FRAME_READ_EN_MASK);
}

int dsim_reg_wait_hs_clk_ready(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_hs_clk_ready(id);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not HS state.\n");
		return -EBUSY;
	}

	return 0;
}

int dsim_reg_wait_hs_clk_disable(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_clk_stop(id);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master clock isn't disabled.\n");
		return -EBUSY;
	}

	return 0;
}

void dsim_reg_force_dphy_stop_state(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_ESCMODE, val, DSIM_ESCMODE_FORCE_STOP_STATE);
}

void dsim_reg_enter_ulps(u32 id, u32 enter)
{
	u32 val = enter ? ~0 : 0;
	u32 mask = DSIM_ESCMODE_TX_ULPS_CLK | DSIM_ESCMODE_TX_ULPS_DATA;

	dsim_write_mask(id, DSIM_ESCMODE, val, mask);
}

void dsim_reg_exit_ulps(u32 id, u32 exit)
{
	u32 val = exit ? ~0 : 0;
	u32 mask = DSIM_ESCMODE_TX_ULPS_CLK_EXIT |
			DSIM_ESCMODE_TX_ULPS_DATA_EXIT;

	dsim_write_mask(id, DSIM_ESCMODE, val, mask);
}

void dsim_reg_set_num_of_transfer(u32 id, u32 num_of_transfer)
{
	u32 val = DSIM_NUM_OF_TRANSFER_PER_FRAME(num_of_transfer);

	dsim_write(id, DSIM_NUM_OF_TRANSFER, val);
}

static u32 dsim_reg_is_ulps_state(u32 id, u32 lanes)
{
	u32 val = dsim_read(id, DSIM_DPHY_STATUS);
	u32 data_lane = lanes >> DSIM_LANE_CLOCK;

	if (((DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(val) & data_lane) == data_lane)
			&& (val & DSIM_DPHY_STATUS_ULPS_CLK))
		return 1;

	return 0;
}

static void dsim_reg_set_deskew_hw(u32 id, u32 interval, u32 position)
{
	u32 val = DSIM_DESKEW_CTRL_HW_INTERVAL(interval)
		| DSIM_DESKEW_CTRL_HW_POSITION(position);
	u32 mask = DSIM_DESKEW_CTRL_HW_INTERVAL_MASK
		| DSIM_DESKEW_CTRL_HW_POSITION_MASK;

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val, mask);
}

static void dsim_reg_enable_deskew_hw_enable(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val, DSIM_DESKEW_CTRL_HW_EN);
}

void dsim_reg_set_cm_underrun_lp_ref(u32 id, u32 lp_ref)
{
	u32 val = DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF(lp_ref);

	dsim_write(id, DSIM_UNDERRUN_CTRL, val);
}

static void dsim_reg_set_threshold(u32 id, u32 threshold)
{
	u32 val = DSIM_THRESHOLD_LEVEL(threshold);

	dsim_write(id, DSIM_THRESHOLD, val);

}

static void dsim_reg_set_vt_compensate(u32 id, u32 compensate)
{
	u32 val = DSIM_VIDEO_TIMER_COMPENSATE(compensate);

	if (dsim_reg_get_version(id) >= DSIM_VER_2P5)
		return;

	dsim_write_mask(id, DSIM_VIDEO_TIMER, val,
			DSIM_VIDEO_TIMER_COMPENSATE_MASK);
}

static void dsim_reg_set_vstatus_int(u32 id, u32 vstatus)
{
	u32 val = DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL(vstatus);

	dsim_write_mask(id, DSIM_VIDEO_TIMER, val,
			DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL_MASK);
}

static void dsim_reg_set_bist_te_interval(u32 id, u32 interval)
{
	u32 val = DSIM_BIST_CTRL0_BIST_TE_INTERVAL(interval);

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_TE_INTERVAL_MASK);
}

static void dsim_reg_set_bist_mode(u32 id, u32 bist_mode)
{
	u32 val = DSIM_BIST_CTRL0_BIST_PTRN_MODE(bist_mode);

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_PTRN_MODE_MASK);
}

static void dsim_reg_enable_bist_pattern_move(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_PTRN_MOVE_EN);
}

static void dsim_reg_enable_bist(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_BIST_CTRL0, val, DSIM_BIST_CTRL0_BIST_EN);
}

static void dsim_set_hw_deskew(u32 id, u32 en)
{
	u32 hw_interval = 1;

	if (en) {
		/* 0 : VBP first line, 1 : VFP last line*/
		dsim_reg_set_deskew_hw(id, hw_interval, 0);
		dsim_reg_enable_deskew_hw_enable(id, en);
	} else {
		dsim_reg_enable_deskew_hw_enable(id, en);
	}
}

static int dsim_reg_wait_enter_ulps_state(u32 id, u32 lanes)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_ulps_state(id, lanes);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not ULPS state.\n");
		return -EBUSY;
	}

	dsim_dbg("DSI Master is ULPS state.\n");

	return 0;
}

static u32 dsim_reg_is_not_ulps_state(u32 id, u32 lanes)
{
	u32 val = dsim_read(id, DSIM_DPHY_STATUS);
	u32 data_lane = lanes >> DSIM_LANE_CLOCK;

	if ((!(DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(val) & data_lane))
			&& !(val & DSIM_DPHY_STATUS_ULPS_CLK))
		return 1;

	return 0;
}

static int dsim_reg_wait_exit_ulps_state(u32 id, u32 lanes)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_not_ulps_state(id, lanes);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not stop state.\n");
		return -EBUSY;
	}

	dsim_dbg("DSI Master is stop state.\n");

	return 0;
}

static int dsim_reg_get_dphy_timing(u32 hs_clk, u32 esc_clk,
		struct dphy_timing_value *t)
{
	int i;

	i = ARRAY_SIZE(dphy_timing) - 1;

	for (; i >= 0; i--) {
		if (dphy_timing[i][0] < hs_clk) {
			continue;
		} else {
			t->bps = hs_clk;
			t->clk_prepare = dphy_timing[i][1];
			t->clk_zero = dphy_timing[i][2];
			t->clk_post = dphy_timing[i][3];
			t->clk_trail = dphy_timing[i][4];
			t->hs_prepare = dphy_timing[i][5];
			t->hs_zero = dphy_timing[i][6];
			t->hs_trail = dphy_timing[i][7];
			t->lpx = dphy_timing[i][8];
			t->hs_exit = dphy_timing[i][9];
			break;
		}
	}

	if (i < 0) {
		dsim_err("%u Mhz hs clock can't find proper dphy timing values\n",
				hs_clk);
		return -EINVAL;
	}

	dsim_dbg("%s: bps(%u) clk_prepare(%u) clk_zero(%u) clk_post(%u)\n",
			__func__, t->bps, t->clk_prepare, t->clk_zero,
			t->clk_post);
	dsim_dbg("clk_trail(%u) hs_prepare(%u) hs_zero(%u) hs_trail(%u)\n",
			t->clk_trail, t->hs_prepare, t->hs_zero, t->hs_trail);
	dsim_dbg("lpx(%u) hs_exit(%u)\n", t->lpx, t->hs_exit);

	if ((esc_clk > 20) || (esc_clk < 7)) {
		dsim_err("%u Mhz cann't be used as escape clock\n", esc_clk);
		return -EINVAL;
	}

	t->b_dphyctl = b_dphyctl[esc_clk - 7];
	dsim_dbg("b_dphyctl(%u)\n", t->b_dphyctl);

	return 0;
}

static void dsim_reg_set_config(u32 id, struct exynos_panel_info *lcd_info,
		struct dsim_clks *clks)
{
	u32 threshold;
	u32 num_of_slice;
	u32 num_of_transfer;
	u32 idx;

#if !defined(BRINGUP_DSIM_BIST)
	/* shadow read disable */
	dsim_reg_enable_shadow_read(id, 1);
	/* dsim_reg_enable_shadow(id, 1); */
#endif

	if (lcd_info->mode == DECON_VIDEO_MODE)
		dsim_reg_enable_clocklane(id, 0);
	else
		dsim_reg_enable_noncont_clock(id, 1);

	dsim_set_hw_deskew(id, 0); /* second param is to control enable bit */

	/* set bta & lpdr timeout vlaues */
	dsim_reg_set_timeout(id);

	dsim_reg_set_cmd_transfer_mode(id, 0);
	dsim_reg_set_stop_state_cnt(id);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		idx = lcd_info->cur_mode_idx;
		dsim_reg_set_cm_underrun_lp_ref(id,
				lcd_info->display_mode[idx].cmd_lp_ref);
	}

	if (lcd_info->dsc.en)
		threshold = lcd_info->dsc.enc_sw * lcd_info->dsc.slice_num;
	else
		threshold = lcd_info->xres;

	dsim_reg_set_threshold(id, threshold);

	dsim_reg_set_resol(id, lcd_info);
	dsim_reg_set_porch(id, lcd_info);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		if (lcd_info->dsc.en)
			/* use 1-line transfer only */
			num_of_transfer = lcd_info->yres;
		else
			num_of_transfer = lcd_info->xres * lcd_info->yres
							/ threshold;

		dsim_reg_set_num_of_transfer(id, num_of_transfer);
	} else {
		num_of_transfer = lcd_info->xres * lcd_info->yres / threshold;
		dsim_reg_set_num_of_transfer(id, num_of_transfer);
	}

	/* set number of lanes, eotp enable, per_frame_read, pixformat, vc_id */
	dsim_reg_set_config_options(id, lcd_info->data_lane -1, 1, 0,
			DSIM_PIXEL_FORMAT_RGB24, 0);

	/* CPSR & VIDEO MODE can be set when shadow enable on */
#if !defined(BRINGUP_DSIM_BIST)
	/* shadow enable */
	dsim_reg_enable_shadow(id, 1);
#endif

	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_video_mode(id, DSIM_CONFIG_VIDEO_MODE);
		dsim_reg_set_hperiod(id, lcd_info);
		dsim_dbg("%s: video mode set\n", __func__);
	} else {
		dsim_reg_set_video_mode(id, 0);
		dsim_dbg("%s: command mode set\n", __func__);
	}

	dsim_reg_enable_dsc(id, lcd_info->dsc.en);
	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE)
		dsim_reg_enable_shadow(id, 0);

	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_disable_hsa(id, 0);
		dsim_reg_disable_hbp(id, 0);
		dsim_reg_disable_hfp(id, 1);
		dsim_reg_disable_hse(id, 0);
		dsim_reg_set_burst_mode(id, 1);
		dsim_reg_set_sync_inform(id, 0);
		dsim_reg_enable_clocklane(id, 0);
	}

	if (lcd_info->dsc.en) {
		dsim_dbg("%s: dsc configuration is set\n", __func__);
		dsim_reg_set_cprl_ctrl(id, lcd_info);
		dsim_reg_set_size_of_slice(id, lcd_info);

		dsim_reg_get_num_of_slice(id, &num_of_slice);
		dsim_dbg("dsim%d: number of DSC slice(%d)\n", id, num_of_slice);
		dsim_reg_print_size_of_slice(id);
	}

	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_multi_packet_count(id, DSIM_PH_FIFOCTRL_THRESHOLD);
		dsim_reg_enable_multi_cmd_packet(id, 0);
	}
	dsim_reg_enable_packetgo(id, 0);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		dsim_reg_set_cmd_ctrl(id, lcd_info, clks);
	} else if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_vt_compensate(id, lcd_info->vt_compensation);
		dsim_reg_set_vstatus_int(id, DSIM_VSYNC);
	}

	/* dsim_reg_enable_shadow_read(id, 1); */
	/* dsim_reg_enable_shadow(id, 1); */

}

/*
 * configure and set DPHY PLL, byte clock, escape clock and hs clock
 *	- PMS value have to be optained by using PMS Gen.
 *      tool (MSC_PLL_WIZARD2_00.exe)
 *	- PLL out is source clock of HS clock
 *	- byte clock = HS clock / 16
 *	- calculate divider of escape clock using requested escape clock
 *	  from driver
 *	- DPHY PLL, byte clock, escape clock are enabled.
 *	- HS clock will be enabled another function.
 *
 * Parameters
 *	- hs_clk : in/out parameter.
 *		in :  requested hs clock. out : calculated hs clock
 *	- esc_clk : in/out paramter.
 *		in : requested escape clock. out : calculated escape clock
 *	- word_clk : out parameter. byte clock = hs clock / 16
 */
static int dsim_reg_set_clocks(u32 id, struct dsim_clks *clks,
		struct stdphy_pms *dphy_pms, u32 en)
{
	unsigned int esc_div;
	struct dsim_pll_param pll;
	struct dphy_timing_value t;
	u32 pll_lock_cnt;
	int ret = 0;
	u32 hsmode = 0;
#ifdef DPDN_INV_SWAP
	u32 inv_data[4] = {0, };
#endif

	if (en) {
		/*
		 * Do not need to set clocks related with PLL,
		 * if DPHY_PLL is already stabled because of LCD_ON_UBOOT.
		 */
		if (dsim_reg_is_pll_stable(id)) {
			dsim_info("dsim%d DPHY PLL is already stable\n", id);
			return -EBUSY;
		}

		/*
		 * set p, m, s to DPHY PLL
		 * PMS value has to be optained by PMS calculation tool
		 * released to customer
		 */
		pll.p = dphy_pms->p;
		pll.m = dphy_pms->m;
		pll.s = dphy_pms->s;
		pll.k = dphy_pms->k;

		/* get word clock */
		/* clks ->hs_clk is from DT */
		clks->word_clk = clks->hs_clk / 16;
		dsim_dbg("word clock is %u MHz\n", clks->word_clk);

		/* requeseted escape clock */
		dsim_dbg("requested escape clock %u MHz\n", clks->esc_clk);

		/* escape clock divider */
		esc_div = clks->word_clk / clks->esc_clk;

		/* adjust escape clock */
		if ((clks->word_clk / esc_div) > clks->esc_clk)
			esc_div += 1;

		/* adjusted escape clock */
		clks->esc_clk = clks->word_clk / esc_div;
		dsim_dbg("escape clock divider is 0x%x\n", esc_div);
		dsim_dbg("escape clock is %u MHz\n", clks->esc_clk);

		/* set BIAS ctrl : default value */
		dsim_reg_set_bias_con(id, DSIM_PHY_BIAS_CON_VAL);

		/* set PLL ctrl : default value */
		dsim_reg_set_pll_con(id, DSIM_PHY_PLL_CON_VAL);

		if (clks->hs_clk < 1500)
			hsmode = 1;

		dsim_reg_set_esc_clk_prescaler(id, 1, esc_div);
		/* get DPHY timing values using hs clock and escape clock */
		dsim_reg_get_dphy_timing(clks->hs_clk, clks->esc_clk, &t);
		dsim_reg_set_dphy_timing_values(id, &t, hsmode);
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
		/* check dither sequence */
		dsim_reg_set_dphy_param_dither(id, dphy_pms);
		dsim_reg_set_dphy_dither_en(id, 1);
#endif

		/* set clock lane General Control Register control */
		dsim_reg_set_mc_gnr_con(id, DSIM_PHY_MC_GNR_CON_VAL);

		/* set clock lane Analog Block Control Register control */
		dsim_reg_set_mc_ana_con(id, DSIM_PHY_MC_ANA_CON_VAL);

#ifdef DPDN_INV_SWAP
		dsim_reg_set_dpdn_swap(id, 1);
#endif

		/* set data lane General Control Register control */
		dsim_reg_set_md_gnr_con(id, DSIM_PHY_MD_GNR_CON_VAL);

#ifdef DPDN_INV_SWAP
		inv_data[0] = 0;
		inv_data[1] = 1;
		inv_data[2] = 1;
		inv_data[3] = 0;
		dsim_reg_set_inv_dpdn(id, 0, inv_data);
#endif

		/* set data lane Analog Block Control Register control */
		dsim_reg_set_md_ana_con(id, DSIM_PHY_MD_ANA_CON_VAL);

		/* set PMSK on PHY */
		dsim_reg_set_pll_freq(id, pll.p, pll.m, pll.s, pll.k);

		/*set wclk buf sft cnt */
		dsim_reg_set_dphy_wclk_buf_sft(id, 3);

		/* set PLL's lock time (lock_cnt)
		 * PLL lock cnt setting guide
		 * PLL_LOCK_CNT_MULT = 500
		 * PLL_LOCK_CNT_MARGIN = 10 (10%)
		 * PLL lock time = PLL_LOCK_CNT_MULT * Tp
		 * Tp = 1 / (OSC clk / pll_p)
		 * PLL lock cnt = PLL lock time * OSC clk
		 */
		pll_lock_cnt =
			(PLL_LOCK_CNT_MULT + PLL_LOCK_CNT_MARGIN) * pll.p;
		dsim_reg_pll_stable_time(id, pll_lock_cnt);

#ifdef DPHY_LOOP
		dsim_reg_set_dphy_loop_test(id);
#endif
		/* enable PLL */
		ret = dsim_reg_enable_pll(id, 1);
	} else {
		/* check disable PHY timing */
		/* TBD */
		dsim_reg_set_esc_clk_prescaler(id, 0, 0xff);
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
		/* check dither sequence */
		dsim_reg_set_dphy_dither_en(id, 0);
#endif
		dsim_reg_enable_pll(id, 0);
	}

	return ret;
}

static int dsim_reg_set_lanes(u32 id, u32 lanes, u32 en)
{
	/* LINK lanes */
	dsim_reg_enable_lane(id, lanes, en);

	return 0;
}

static int dsim_reg_set_lanes_dphy(u32 id, u32 lanes, u32 en)
{
	/* PHY lanes */
	if (dsim_reg_enable_lane_phy(id, (lanes >> 1), en))
		return -EBUSY;

	return 0;
}

static u32 dsim_reg_is_noncont_clk_enabled(u32 id)
{
	int ret;

	ret = dsim_read_mask(id, DSIM_CLK_CTRL,
					DSIM_CLK_CTRL_NONCONT_CLOCK_LANE);
	return ret;
}

static int dsim_reg_set_hs_clock(u32 id, u32 en)
{
	int reg = 0;
	int is_noncont = dsim_reg_is_noncont_clk_enabled(id);

	if (en) {
		dsim_reg_enable_hs_clock(id, 1);
		if (!is_noncont)
			reg = dsim_reg_wait_hs_clk_ready(id);
	} else {
		dsim_reg_enable_hs_clock(id, 0);
		reg = dsim_reg_wait_hs_clk_disable(id);
	}
	return reg;
}

static void dsim_reg_set_int(u32 id, u32 en)
{
	u32 val = en ? 0 : ~0;
	u32 mask;

	/*
	 * TODO: underrun irq will be unmasked in the future.
	 * underrun irq(dsim_reg_set_config) is ignored in zebu emulator.
	 * it's not meaningful
	 */
	mask = DSIM_INTMSK_SW_RST_RELEASE | DSIM_INTMSK_SFR_PL_FIFO_EMPTY |
		DSIM_INTMSK_SFR_PH_FIFO_EMPTY |
		DSIM_INTMSK_FRAME_DONE | DSIM_INTMSK_INVALID_SFR_VALUE |
		DSIM_INTMSK_UNDER_RUN | DSIM_INTMSK_RX_DATA_DONE |
		DSIM_INTMSK_ERR_RX_ECC | DSIM_INTMSK_VT_STATUS;

	dsim_write_mask(id, DSIM_INTMSK, val, mask);
}

/*
 * enter or exit ulps mode
 *
 * Parameter
 *	1 : enter ULPS mode
 *	0 : exit ULPS mode
 */
static int dsim_reg_set_ulps(u32 id, u32 en, u32 lanes)
{
	int ret = 0;

	if (en) {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;

	} else {
		/* Exit ULPS clock and data lane */
		dsim_reg_exit_ulps(id, 1);

		ret = dsim_reg_wait_exit_ulps_state(id, lanes);
		if (ret)
			return ret;

		/* wait at least 1ms : Twakeup time for MARK1 state  */
		udelay(1000);

		/* Clear ULPS exit request */
		dsim_reg_exit_ulps(id, 0);

		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);
	}

	return ret;
}

/*
 * enter or exit ulps mode for LSI DDI
 *
 * Parameter
 *	1 : enter ULPS mode
 *	0 : exit ULPS mode
 * assume that disp block power is off after ulps mode enter
 */
static int dsim_reg_set_smddi_ulps(u32 id, u32 en, u32 lanes)
{
	int ret = 0;

	if (en) {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;
		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);
	} else {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;

		/* Exit ULPS clock and data lane */
		dsim_reg_exit_ulps(id, 1);

		ret = dsim_reg_wait_exit_ulps_state(id, lanes);
		if (ret)
			return ret;

		/* wait at least 1ms : Twakeup time for MARK1 state */
		udelay(1000);

		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);

		/* Clear ULPS exit request */
		dsim_reg_exit_ulps(id, 0);
	}

	return ret;
}

static int dsim_reg_set_ulps_by_ddi(u32 id, u32 ddi_type, u32 lanes, u32 en)
{
	int ret;

	switch (ddi_type) {
	case TYPE_OF_SM_DDI:
		ret = dsim_reg_set_smddi_ulps(id, en, lanes);
		break;
	case TYPE_OF_MAGNA_DDI:
		dsim_err("This ddi(%d) doesn't support ULPS\n", ddi_type);
		ret = -EINVAL;
		break;
	case TYPE_OF_NORMAL_DDI:
	default:
		ret = dsim_reg_set_ulps(id, en, lanes);
		break;
	}

	if (ret < 0)
		dsim_err("%s: failed to %s ULPS", __func__,
				en ? "enter" : "exit");

	return ret;
}

/******************** EXPORTED DSIM CAL APIs ********************/
void dpu_sysreg_select_dphy_rst_control(void __iomem *sysreg, u32 dsim_id, u32 sel)
{
	u32 phy_num = dsim_id;
	u32 old = readl(sysreg + DISP_DPU_MIPI_PHY_CON);
	u32 val = sel ? ~0 : 0;
	u32 mask = SEL_RESET_DPHY_MASK(phy_num);

	val = (val & mask) | (old & ~mask);
	writel(val, sysreg + DISP_DPU_MIPI_PHY_CON);
	dsim_dbg("%s: phy_con_sel val=0x%x", __func__, val);
}

void dpu_sysreg_dphy_reset(void __iomem *sysreg, u32 dsim_id, u32 rst)
{
	u32 old = readl(sysreg + DISP_DPU_MIPI_PHY_CON);
	u32 val = rst ? ~0 : 0;
	u32 mask = M_RESETN_M2S0_TOP_MASK;

	val = (val & mask) | (old & ~mask);
	writel(val, sysreg + DISP_DPU_MIPI_PHY_CON);
	dsim_dbg("%s: phy_con_rst val=0x%x", __func__, val);
}

static u32 dsim_reg_translate_lanecnt_to_lanes(u32 lanecnt)
{
	u32 lanes, i;

	lanes = DSIM_LANE_CLOCK;
	for (i = 1; i <= lanecnt; ++i)
		lanes |= 1 << i;

	return lanes;
}

/* This function is just for dsim basic initialization for reading panel id */
void dsim_reg_preinit(u32 id)
{
	u32 lanes;
	struct dsim_device *dsim = get_dsim_drvdata(id);
	struct dsim_clks clks;
	struct exynos_panel_info lcd_info;
	u32 idx;

	/* default configuration just for reading panel id */
	memset(&clks, 0, sizeof(struct dsim_clks));
	clks.hs_clk = 384;
	clks.esc_clk = 12;
	memset(&lcd_info, 0, sizeof(struct exynos_panel_info));
	lcd_info.vfp = 10;
	lcd_info.vbp = 6;
	lcd_info.vsa = 1;
	lcd_info.hfp = 1;
	lcd_info.hbp = 3;
	lcd_info.hsa = 1;
	lcd_info.fps = 60;
	lcd_info.hs_clk = 384;
	lcd_info.mode = DECON_MIPI_COMMAND_MODE;
	lcd_info.xres = 360;
	lcd_info.yres = 360;
	lcd_info.dphy_pms.p = 2;
	lcd_info.dphy_pms.m = 118;
	lcd_info.dphy_pms.s = 2;
	lcd_info.dphy_pms.k = 0;
	lcd_info.data_lane = 1;
	lcd_info.cur_mode_idx = 0;
	idx = lcd_info.cur_mode_idx;
	lcd_info.display_mode[idx].cmd_lp_ref = 1695;

	/* DPHY reset control from SYSREG(0) */
	dpu_sysreg_select_dphy_rst_control(dsim->res.ss_regs, dsim->id, 0);

	lanes = dsim_reg_translate_lanecnt_to_lanes(lcd_info.data_lane);

	/* choose OSC_CLK */
	dsim_reg_set_link_clock(id, 0);

	dsim_reg_sw_reset(id);
	dsim_reg_set_lanes(id, lanes, 1);
	dsim_reg_set_esc_clk_on_lane(id, 1, lanes);
	dsim_reg_enable_word_clock(id, 1);

	/* Enable DPHY reset : DPHY reset start */
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 0);

	/* panel power on and reset are mandatory for reading panel id */
	dsim_set_panel_power(dsim, 1);

	dsim_reg_set_clocks(id, &clks, &lcd_info.dphy_pms, 1);
	dsim_reg_set_lanes_dphy(id, lanes, true);
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 1); /* Release DPHY reset */

	dsim_reg_set_link_clock(id, 1);	/* Selection to word clock */

	dsim_reg_set_config(id, &lcd_info, &clks);
	dsim_reset_panel(dsim);
}

void dsim_reg_init(u32 id, struct exynos_panel_info *lcd_info, struct dsim_clks *clks,
		bool panel_ctrl)
{
	u32 lanes;
#if !defined(CONFIG_EXYNOS_LCD_ON_UBOOT)
	struct dsim_device *dsim = get_dsim_drvdata(id);
#endif

	if (dsim->state == DSIM_STATE_INIT) {
		if (dsim_reg_get_link_clock(dsim->id)) {
			dsim_info("dsim%d is already enabled in bootloader\n", dsim->id);
			/* to prevent irq storm that may occur in the OFF STATE */
#if !IS_ENABLED(CONFIG_MCD_PANEL)			
			dsim_reg_clear_int(id, 0xffffffff);
			return;
#endif

		}
	}

	/* DPHY reset control from SYSREG(0) */
	dpu_sysreg_select_dphy_rst_control(dsim->res.ss_regs, dsim->id, 0);

	lanes = dsim_reg_translate_lanecnt_to_lanes(lcd_info->data_lane);

	/* choose OSC_CLK */
	dsim_reg_set_link_clock(id, 0);

	/* Enable DPHY reset : DPHY reset start */
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 0);

	dsim_reg_sw_reset(id);
#if !IS_ENABLED(CONFIG_MCD_PANEL)
#if defined(CONFIG_EXYNOS_LCD_ON_UBOOT)
	/* TODO: This code will be implemented as uboot style */
#else
	/* Panel power on */
	if (panel_ctrl)
		dsim_set_panel_power(dsim, 1);
#endif
#endif

	dsim_reg_set_clocks(id, clks, &lcd_info->dphy_pms, 1);

	dsim_reg_set_lanes_dphy(id, lanes, true);

	dsim_reg_set_lanes(id, lanes, 1);

	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 1); /* Release DPHY reset */

	dsim_reg_set_link_clock(id, 1);	/* Selection to word clock */

	dsim_reg_set_esc_clk_on_lane(id, 1, lanes);

	dsim_reg_enable_word_clock(id, 1);

	dsim_reg_set_config(id, lcd_info, clks);

//	dsim_reg_set_pll_clk_gate_enable(id, DPHY_PLL_CLK_GATE_EN); /* PHY pll clock gate disable */
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
	dsim_reg_set_pll_sleep_enable(id, true);	/* PHY pll sleep enable */
#endif

#if IS_ENABLED(CONFIG_MCD_PANEL)
	/* Panel power on */
	if (panel_ctrl)
		dsim_set_panel_power(dsim, 1);
#else
#if defined(CONFIG_EXYNOS_LCD_ON_UBOOT)
	/* TODO: This code will be implemented as uboot style */
#else
	if (panel_ctrl)
		dsim_reset_panel(dsim);
#endif
#endif
}

/* Set clocks and lanes and HS ready */
void dsim_reg_start(u32 id)
{
	dsim_reg_set_hs_clock(id, 1);
	dsim_reg_set_int(id, 1);
}

/* Unset clocks and lanes and stop_state */
int dsim_reg_stop(u32 id, u32 lanes)
{
	int err = 0;
	u32 is_vm;

	struct dsim_device *dsim = get_dsim_drvdata(id);

	dsim_reg_clear_int(id, 0xffffffff);
	/* disable interrupts */
	dsim_reg_set_int(id, 0);

	/* first disable HS clock */
	if (dsim_reg_set_hs_clock(id, 0) < 0)
		dsim_err("The CLK lane doesn't be switched to LP mode\n");

	/* 0. wait the IDLE status */
	is_vm = dsim_reg_get_display_mode(id);
	err = dsim_reg_wait_idle_status(id, is_vm);
	if (err < 0)
		dsim_err("DSIM status is not IDLE!\n");

	/* 1. clock selection : OSC */
	dsim_reg_set_link_clock(id, 0);

	/* 2. master resetn */
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 0);

	/* 3. disable lane */
	dsim_reg_set_lanes_dphy(id, lanes, false);

	/* 4. turn off WORDCLK and ESCCLK */
	dsim_reg_set_esc_clk_on_lane(id, 0, lanes);
	dsim_reg_set_esc_clk_en(id, 0);

	/* 5. disable PLL */
	dsim_reg_set_clocks(id, NULL, NULL, 0);

	if (err == 0)
		dsim_reg_sw_reset(id);

	return err;
}

void dsim_reg_recovery_process(struct dsim_device *dsim)
{
	dsim_info("%s +\n", __func__);

	dsim_reg_clear_int(dsim->id, 0xffffffff);

	/* 0. disable HS clock */
	if (dsim_reg_set_hs_clock(dsim->id, 0) < 0)
		dsim_err("The CLK lane doesn't be switched to LP mode\n");

	/* 1. clock selection : OSC */
	dsim_reg_set_link_clock(dsim->id, 0);

	/* 2. reset & release */
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, dsim->id, 0);
	dsim_reg_function_reset(dsim->id);
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, dsim->id, 1);

	/* 3. clock selection : PLL */
	dsim_reg_set_link_clock(dsim->id, 1);

	/* 4. enable HS clock */
	dsim_reg_set_hs_clock(dsim->id, 1);

	dsim_info("%s -\n", __func__);
}

/* Exit ULPS mode and set clocks and lanes */
int dsim_reg_exit_ulps_and_start(u32 id, u32 ddi_type, u32 lanes)
{
	int ret = 0;

	#if 0
	/*
	 * Guarantee 1.2v signal level for data lane(positive) when exit ULPS.
	 * DSIM Should be set standby. If not, lane goes to 600mv sometimes.
	 */
	dsim_reg_set_hs_clock(id, 1);
	dsim_reg_set_hs_clock(id, 0);
	#endif

	/* try to exit ULPS mode. The sequence is depends on DDI type */
	ret = dsim_reg_set_ulps_by_ddi(id, ddi_type, lanes, 0);
	dsim_reg_start(id);
	return ret;
}

/* Unset clocks and lanes and enter ULPS mode */
int dsim_reg_stop_and_enter_ulps(u32 id, u32 ddi_type, u32 lanes)
{
	int ret = 0;
	u32 is_vm;
	struct dsim_device *dsim = get_dsim_drvdata(id);
	struct dsim_regs regs;

	dsim_reg_clear_int(id, 0xffffffff);
	/* disable interrupts */
	dsim_reg_set_int(id, 0);

	/* 1. turn off clk lane & wait for stopstate_clk */
	ret = dsim_reg_set_hs_clock(id, 0);
	if (ret < 0)
		dsim_err("The CLK lane doesn't be switched to LP mode\n");

	/* 2. enter to ULPS & wait for ulps state of clk and data */
	dsim_reg_set_ulps_by_ddi(id, ddi_type, lanes, 1);

	/* 3. sequence for BLK_DPU off */
	/* 3.1 wait idle */
	is_vm = dsim_reg_get_display_mode(id);
	ret = dsim_reg_wait_idle_status(id, is_vm);
	if (ret < 0)
		dsim_err("%s : DSIM_status is not IDLE!\n", __func__);
	/* 3.2 OSC clock */
	dsim_reg_set_link_clock(id, 0);
	/* 3.3 off DPHY */
	dsim_reg_set_lanes_dphy(id, lanes, false);
	dsim_reg_set_clocks(id, NULL, NULL, 0);

	if (ret < 0) {
		dsim_to_regs_param(dsim, &regs);
		__dsim_dump(id, &regs);
	}

	/* 3.4 sw reset */
	dsim_reg_sw_reset(id);

	return ret;
}

int dsim_reg_get_int_and_clear(u32 id)
{
	u32 val;

	val = dsim_read(id, DSIM_INTSRC);
	dsim_reg_clear_int(id, val);

	return val;
}

void dsim_reg_clear_int(u32 id, u32 int_src)
{
	dsim_write(id, DSIM_INTSRC, int_src);
}

void dsim_reg_wr_tx_header(u32 id, u32 d_id, unsigned long d0, u32 d1, u32 bta)
{
	u32 val = DSIM_PKTHDR_BTA_TYPE(bta) | DSIM_PKTHDR_ID(d_id) |
		DSIM_PKTHDR_DATA0(d0) | DSIM_PKTHDR_DATA1(d1);

	dsim_write_mask(id, DSIM_PKTHDR, val, DSIM_PKTHDR_DATA);
}

void dsim_reg_wr_tx_payload(u32 id, u32 payload)
{
	dsim_write(id, DSIM_PAYLOAD, payload);
}

u32 dsim_reg_header_fifo_is_empty(u32 id)
{
	return dsim_read_mask(id, DSIM_FIFOCTRL, DSIM_FIFOCTRL_EMPTY_PH_SFR);
}

u32 dsim_reg_payload_fifo_is_empty(u32 id)
{

	return dsim_read_mask(id, DSIM_FIFOCTRL, DSIM_FIFOCTRL_EMPTY_PL_SFR);
}

bool dsim_reg_is_writable_ph_fifo_state(u32 id, u32 cmd_cnt)
{
	u32 val = dsim_read(id, DSIM_FIFOCTRL);

	val = DSIM_FIFOCTRL_NUMBER_OF_PH_SFR_GET(val);
	val += cmd_cnt;

	if (val < DSIM_PH_FIFOCTRL_THRESHOLD)
		return true;
	else
		return false;
}

u32 dsim_reg_get_rx_fifo(u32 id)
{
	return dsim_read(id, DSIM_RXFIFO);
}

u32 dsim_reg_rx_fifo_is_empty(u32 id)
{
	return dsim_read_mask(id, DSIM_FIFOCTRL, DSIM_FIFOCTRL_EMPTY_RX);
}

int dsim_reg_get_linecount(u32 id, u32 mode)
{
	u32 val = 0;

	if (mode == DECON_VIDEO_MODE) {
		val = dsim_read(id, DSIM_LINK_STATUS0);
		return DSIM_LINK_STATUS0_VM_LINE_CNT_GET(val);
	} else if (mode == DECON_MIPI_COMMAND_MODE) {
		val = dsim_read(id, DSIM_LINK_STATUS1);
		return DSIM_LINK_STATUS1_CMD_TRANSF_CNT_GET(val);
	}

	return -EINVAL;
}

int dsim_reg_rx_err_handler(u32 id, u32 rx_fifo)
{
	int ret = 0;
	u32 err_bit = rx_fifo >> 8; /* Error_Range [23:8] */

	if ((err_bit & MIPI_DSI_ERR_BIT_MASK) == 0) {
		dsim_dbg("dsim%d, Non error reporting format (rx_fifo=0x%x)\n",
				id, rx_fifo);
		return ret;
	}

	/* Parse error report bit*/
	if (err_bit & MIPI_DSI_ERR_SOT)
		dsim_err("SoT error!\n");
	if (err_bit & MIPI_DSI_ERR_SOT_SYNC)
		dsim_err("SoT sync error!\n");
	if (err_bit & MIPI_DSI_ERR_EOT_SYNC)
		dsim_err("EoT error!\n");
	if (err_bit & MIPI_DSI_ERR_ESCAPE_MODE_ENTRY_CMD)
		dsim_err("Escape mode entry command error!\n");
	if (err_bit & MIPI_DSI_ERR_LOW_POWER_TRANSMIT_SYNC)
		dsim_err("Low-power transmit sync error!\n");
	if (err_bit & MIPI_DSI_ERR_HS_RECEIVE_TIMEOUT)
		dsim_err("HS receive timeout error!\n");
	if (err_bit & MIPI_DSI_ERR_FALSE_CONTROL)
		dsim_err("False control error!\n");
	if (err_bit & MIPI_DSI_ERR_ECC_SINGLE_BIT)
		dsim_err("ECC error, single-bit(detected and corrected)!\n");
	if (err_bit & MIPI_DSI_ERR_ECC_MULTI_BIT)
		dsim_err("ECC error, multi-bit(detected, not corrected)!\n");
	if (err_bit & MIPI_DSI_ERR_CHECKSUM)
		dsim_err("Checksum error(long packet only)!\n");
	if (err_bit & MIPI_DSI_ERR_DATA_TYPE_NOT_RECOGNIZED)
		dsim_err("DSI data type not recognized!\n");
	if (err_bit & MIPI_DSI_ERR_VCHANNEL_ID_INVALID)
		dsim_err("DSI VC ID invalid!\n");
	if (err_bit & MIPI_DSI_ERR_INVALID_TRANSMIT_LENGTH)
		dsim_err("Invalid transmission length!\n");

	dsim_err("dsim%d, (rx_fifo=0x%x) Check DPHY values about HS clk.\n",
			id, rx_fifo);
	return -EINVAL;
}

void dsim_reg_enable_shadow_read(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_SFR_CTRL, val,
				DSIM_SFR_CTRL_SHADOW_REG_READ_EN);
}

void dsim_reg_function_reset(u32 id)
{
	u32 cnt = 1000;
	u32 state;

	dsim_write_mask(id, DSIM_SWRST, ~0,  DSIM_SWRST_FUNCRST);

	do {
		state = dsim_read(id, DSIM_SWRST) & DSIM_SWRST_FUNCRST;
		cnt--;
		udelay(10);
	} while (state && cnt);

	if (!cnt)
		dsim_err("%s is timeout.\n", __func__);

}

/* Set porch and resolution to support Partial update */
void dsim_reg_set_partial_update(u32 id, struct exynos_panel_info *lcd_info)
{
	dsim_reg_set_resol(id, lcd_info);
	dsim_reg_set_threshold(id, lcd_info->xres);
	dsim_reg_set_porch(id, lcd_info);
	dsim_reg_set_num_of_transfer(id, lcd_info->yres);
}

void dsim_reg_set_mres(u32 id, struct exynos_panel_info *lcd_info)
{
	u32 threshold;
	u32 num_of_slice;
	u32 num_of_transfer;
	int idx;

	if (lcd_info->mode != DECON_MIPI_COMMAND_MODE) {
		dsim_info("%s: mode[%d] doesn't support multi resolution\n",
				__func__, lcd_info->mode);
		return;
	}

	idx = lcd_info->mres_mode;
	dsim_reg_set_cm_underrun_lp_ref(id,
			lcd_info->display_mode[idx].cmd_lp_ref);

	if (lcd_info->dsc.en) {
		threshold = lcd_info->dsc.enc_sw * lcd_info->dsc.slice_num;
		/* use 1-line transfer only */
		num_of_transfer = lcd_info->yres;
	} else {
		threshold = lcd_info->xres;
		num_of_transfer = lcd_info->xres * lcd_info->yres / threshold;
	}

	dsim_reg_set_threshold(id, threshold);
	dsim_reg_set_resol(id, lcd_info);
	dsim_reg_set_porch(id, lcd_info);
	dsim_reg_set_num_of_transfer(id, num_of_transfer);

	dsim_reg_enable_dsc(id, lcd_info->dsc.en);
	if (lcd_info->dsc.en) {
		dsim_dbg("%s: dsc configuration is set\n", __func__);
		dsim_reg_set_num_of_slice(id, lcd_info->dsc.slice_num);
		dsim_reg_set_multi_slice(id, lcd_info); /* multi slice */
		dsim_reg_set_size_of_slice(id, lcd_info);

		dsim_reg_get_num_of_slice(id, &num_of_slice);
		dsim_dbg("dsim%d: number of DSC slice(%d)\n", id, num_of_slice);
		dsim_reg_print_size_of_slice(id);
	}
}

void dsim_reg_set_bist(u32 id, u32 en)
{
	if (en) {
		dsim_reg_set_bist_te_interval(id, 4505);
		dsim_reg_set_bist_mode(id, DSIM_GRAY_GRADATION);
		dsim_reg_enable_bist_pattern_move(id, true);
		dsim_reg_enable_bist(id, en);
	}
}

void dsim_reg_set_cmd_transfer_mode(u32 id, u32 lp)
{
	u32 val = lp ? ~0 : 0;

	dsim_write_mask(id, DSIM_ESCMODE, val, DSIM_ESCMODE_CMD_LPDT);
}

/* Only for command mode */
void dsim_reg_set_dphy_freq_hopping(u32 id, u32 p, u32 m, u32 k, u32 en)
{
	/* This is not supported in exynos3830 */
}

#define PREFIX_LEN	40
#define ROW_LEN		32
static void dsim_print_hex_dump(void __iomem *regs, const void *buf, size_t len)
{
	char prefix_buf[PREFIX_LEN];
	unsigned long p;
	size_t i, row;

	for (i = 0; i < len; i += ROW_LEN) {
		p = buf - regs + i;

		if (len - i < ROW_LEN)
			row = len - i;
		else
			row = ROW_LEN;

		snprintf(prefix_buf, sizeof(prefix_buf), "[%08lX] ", p);
		print_hex_dump(KERN_NOTICE, prefix_buf, DUMP_PREFIX_NONE,
				32, 4, buf + i, row, false);
	}
}

void __dsim_dump(u32 id, struct dsim_regs *regs)
{
	/* change to updated register read mode (meaning: SHADOW in DECON) */
	dsim_info("=== DSIM %d LINK SFR DUMP ===\n", id);
	dsim_reg_enable_shadow_read(id, 0);
	dsim_print_hex_dump(regs->regs, regs->regs + 0x0000, 0x114);

#ifdef CONFIG_DPHY_APB_CONTROL
	dsim_dphy_apb_enable(dsim, 1);
#endif

	dsim_info("=== DSIM %d DPHY SFR DUMP ===\n", id);

	/* DPHY dump */
	dsim_info("-[BIAS_CTRL : offset + 0x0400]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x0400, 0x20);

	dsim_info("-[DTB_CTRL : offset + 0x0800]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x0800, 0x20);

	dsim_info("-[PLL_CTRL : offset + 0x0C00]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x0C00, 0x40);

	dsim_info("-[DPHY_ACTRL_MC : offset + 0x1000]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1000, 0x10);

	dsim_info("-[DPHY_DCTRL_MC : offset + 0x1080]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1080, 0x40);

	dsim_info("-[DPHY_ACTRL_MD0 : offset + 0x1400]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1400, 0x20);

	dsim_info("-[DPHY_DCTRL_MD0 : offset + 0x1480]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1480, 0x30);

	dsim_info("-[DPHY_ACTRL_MD1 : offset + 0x1800]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1800, 0x10);

	dsim_info("-[DPHY_DCTRL_MD1 : offset + 0x1880]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1880, 0x30);

	dsim_info("-[DPHY_ACTRL_MD2 : offset + 0x1C00]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1C00, 0x10);

	dsim_info("-[DPHY_DCTRL_MD2 : offset + 0x1C80]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x1C80, 0x30);

	dsim_info("-[DPHY_ACTRL_MD3 : offset + 0x2000]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x2000, 0x10);

	dsim_info("-[DPHY_DCTRL_MD3 : offset + 0x2080]-\n");
	dsim_print_hex_dump(regs->phy_regs, regs->phy_regs + 0x2080, 0x30);

#ifdef CONFIG_DPHY_APB_CONTROL
	dsim_dphy_apb_enable(dsim, 0);
#endif

	/* restore to avoid size mismatch (possible config error at DECON) */
	dsim_reg_enable_shadow_read(id, 1);
}
