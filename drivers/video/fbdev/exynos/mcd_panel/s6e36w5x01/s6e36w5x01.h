/*
 * linux/drivers/video/fbdev/exynos/panel/s6e36w5x01/s6e36w5x01.h
 *
 * Header file for S6E36W5X01 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E36W5X01_H__
#define __S6E36W5X01_H__

#include <linux/types.h>
#include <linux/kernel.h>


#define AID_INTERPOLATION
#define S6E36W5X01_GAMMA_CMD_CNT 	(35)

#define S6E36W5X01_IRC_ONOFF_OFS	(0)
#define S6E36W5X01_IRC_VALUE_OFS	(10)
#define S6E36W5X01_IRC_VALUE_LEN	(33)

#define S6E36W5X01_ADDR_OFS		(0)
#define S6E36W5X01_ADDR_LEN		(1)
#define S6E36W5X01_DATA_OFS		(S6E36W5X01_ADDR_OFS + S6E36W5X01_ADDR_LEN)

#define S6E36W5X01_MTP_REG		0xC8
#define S6E36W5X01_MTP_OFS		0
#define S6E36W5X01_MTP_LEN		34

#define S6E36W5X01_DATE_REG		0xA1
#define S6E36W5X01_DATE_OFS		4
#define S6E36W5X01_DATE_LEN		(PANEL_DATE_LEN)

#define S6E36W5X01_COORDINATE_REG	0xA1
#define S6E36W5X01_COORDINATE_OFS	0
#define S6E36W5X01_COORDINATE_LEN	(PANEL_COORD_LEN)

#define S6E36W5X01_ID_REG		0x04
#define S6E36W5X01_ID_OFS		0
#define S6E36W5X01_ID_LEN		(PANEL_ID_LEN)

#define S6E36W5X01_CODE_REG		0xD6
#define S6E36W5X01_CODE_OFS		0
#define S6E36W5X01_CODE_LEN		(PANEL_CODE_LEN)

#define S6E36W5X01_ELVSS_REG		0xB5
#define S6E36W5X01_ELVSS_OFS		0
#define S6E36W5X01_ELVSS_LEN		23


#define S6E36W5X01_OCTA_ID_REG		0xA1
#define S6E36W5X01_OCTA_ID_OFS		11
#define S6E36W5X01_OCTA_ID_LEN		(PANEL_OCTA_ID_LEN)


#define S6E36W5X01_CHIP_ID_REG		0xD6
#define S6E36W5X01_CHIP_ID_OFS		0
#define S6E36W5X01_CHIP_ID_LEN		5


#define S6E36W5X01_AOR_REG		0xB1
#define S6E36W5X01_AOR_OFS		0
#define S6E36W5X01_AOR_LEN		2


/* gamma mtp offset */
#define S6E36W5X01_GAMMA_MTP_LEN	(43)

/* for panel dump */
#define S6E36W5X01_RDDPM_REG		0x0A
#define S6E36W5X01_RDDPM_OFS		0
#define S6E36W5X01_RDDPM_LEN		(PANEL_RDDPM_LEN)

#define S6E36W5X01_RDDSM_REG		0x0E
#define S6E36W5X01_RDDSM_OFS		0
#define S6E36W5X01_RDDSM_LEN		(PANEL_RDDSM_LEN)

#define S6E36W5X01_ERR_REG		0xE5
#define S6E36W5X01_ERR_OFS		0
#define S6E36W5X01_ERR_LEN		5

#define S6E36W5X01_ERR_FG_REG		0xEE
#define S6E36W5X01_ERR_FG_OFS		0
#define S6E36W5X01_ERR_FG_LEN		1

#define S6E36W5X01_DSI_ERR_REG		0x05
#define S6E36W5X01_DSI_ERR_OFS		0
#define S6E36W5X01_DSI_ERR_LEN		1

#define S6E36W5X01_SELF_DIAG_REG	0x0F
#define S6E36W5X01_SELF_DIAG_OFS	0
#define S6E36W5X01_SELF_DIAG_LEN	1


enum {
	GAMMA_MAPTBL,
	GAMMA_MODE2_MAPTBL,
	TSET_MAPTBL,
	ACL_CONTROL_MAPTBL,
	HBM_TRANSITION_MAPTBL,
	LPM_WRDISBV_MAPTBL,
	ELVSS_TEMP_MAPTBL,
#ifdef CONFIG_SUPPORT_HMD
	/* HMD MAPTBL */
	HMD_GAMMA_MAPTBL,
	HMD_AOR_MAPTBL,
	HMD_ELVSS_MAPTBL,
#endif /* CONFIG_SUPPORT_HMD */
	MAX_MAPTBL,
};

enum {
	READ_ID,
	READ_COORDINATE,
	READ_CODE,
	READ_ELVSS,
	READ_MTP,
	READ_DATE,
	READ_OCTA_ID,
	READ_CHIP_ID,
	/* for brightness debugging */
	READ_AOR,
	READ_RDDPM,
	READ_RDDSM,
	READ_ERR,
	READ_ERR_FG,
	READ_DSI_ERR,
	READ_SELF_DIAG,
};

enum {
	RES_ID,
	RES_COORDINATE,
	RES_CODE,
	RES_ELVSS,
	RES_MTP,
	RES_DATE,
	RES_OCTA_ID,
	RES_CHIP_ID,
	/* for brightness debugging */
	RES_AOR,
	RES_RDDPM,
	RES_RDDSM,
	RES_ERR,
	RES_ERR_FG,
	RES_DSI_ERR,
	RES_SELF_DIAG,

};

static u8 S6E36W5X01_ID[S6E36W5X01_ID_LEN];
static u8 S6E36W5X01_COORDINATE[S6E36W5X01_COORDINATE_LEN];
static u8 S6E36W5X01_CODE[S6E36W5X01_CODE_LEN];
static u8 S6E36W5X01_ELVSS[S6E36W5X01_ELVSS_LEN];
static u8 S6E36W5X01_MTP[S6E36W5X01_MTP_LEN];
static u8 S6E36W5X01_DATE[S6E36W5X01_DATE_LEN];
static u8 S6E36W5X01_OCTA_ID[S6E36W5X01_OCTA_ID_LEN];
/* for brightness debugging */
static u8 S6E36W5X01_AOR[S6E36W5X01_AOR_LEN];
static u8 S6E36W5X01_CHIP_ID[S6E36W5X01_CHIP_ID_LEN];
static u8 S6E36W5X01_RDDPM[S6E36W5X01_RDDPM_LEN];
static u8 S6E36W5X01_RDDSM[S6E36W5X01_RDDSM_LEN];
static u8 S6E36W5X01_ERR[S6E36W5X01_ERR_LEN];
static u8 S6E36W5X01_ERR_FG[S6E36W5X01_ERR_FG_LEN];
static u8 S6E36W5X01_DSI_ERR[S6E36W5X01_DSI_ERR_LEN];
static u8 S6E36W5X01_SELF_DIAG[S6E36W5X01_SELF_DIAG_LEN];

static struct rdinfo s6e36w5x01_rditbl[] = {
	[READ_ID] = RDINFO_INIT(id, DSI_PKT_TYPE_RD, S6E36W5X01_ID_REG, S6E36W5X01_ID_OFS, S6E36W5X01_ID_LEN),
	[READ_COORDINATE] = RDINFO_INIT(coordinate, DSI_PKT_TYPE_RD, S6E36W5X01_COORDINATE_REG, S6E36W5X01_COORDINATE_OFS, S6E36W5X01_COORDINATE_LEN),
	[READ_CODE] = RDINFO_INIT(code, DSI_PKT_TYPE_RD, S6E36W5X01_CODE_REG, S6E36W5X01_CODE_OFS, S6E36W5X01_CODE_LEN),
	[READ_ELVSS] = RDINFO_INIT(elvss, DSI_PKT_TYPE_RD, S6E36W5X01_ELVSS_REG, S6E36W5X01_ELVSS_OFS, S6E36W5X01_ELVSS_LEN),
	[READ_MTP] = RDINFO_INIT(mtp, DSI_PKT_TYPE_RD, S6E36W5X01_MTP_REG, S6E36W5X01_MTP_OFS, S6E36W5X01_MTP_LEN),
	[READ_DATE] = RDINFO_INIT(date, DSI_PKT_TYPE_RD, S6E36W5X01_DATE_REG, S6E36W5X01_DATE_OFS, S6E36W5X01_DATE_LEN),
	[READ_OCTA_ID] = RDINFO_INIT(octa_id, DSI_PKT_TYPE_RD, S6E36W5X01_OCTA_ID_REG, S6E36W5X01_OCTA_ID_OFS, S6E36W5X01_OCTA_ID_LEN),
	/* for brightness debugging */
	[READ_AOR] = RDINFO_INIT(aor, DSI_PKT_TYPE_RD, S6E36W5X01_AOR_REG, S6E36W5X01_AOR_OFS, S6E36W5X01_AOR_LEN),
	[READ_CHIP_ID] = RDINFO_INIT(chip_id, DSI_PKT_TYPE_RD, S6E36W5X01_CHIP_ID_REG, S6E36W5X01_CHIP_ID_OFS, S6E36W5X01_CHIP_ID_LEN),
	[READ_RDDPM] = RDINFO_INIT(rddpm, DSI_PKT_TYPE_RD, S6E36W5X01_RDDPM_REG, S6E36W5X01_RDDPM_OFS, S6E36W5X01_RDDPM_LEN),
	[READ_RDDSM] = RDINFO_INIT(rddsm, DSI_PKT_TYPE_RD, S6E36W5X01_RDDSM_REG, S6E36W5X01_RDDSM_OFS, S6E36W5X01_RDDSM_LEN),
	[READ_ERR] = RDINFO_INIT(err, DSI_PKT_TYPE_RD, S6E36W5X01_ERR_REG, S6E36W5X01_ERR_OFS, S6E36W5X01_ERR_LEN),
	[READ_ERR_FG] = RDINFO_INIT(err_fg, DSI_PKT_TYPE_RD, S6E36W5X01_ERR_FG_REG, S6E36W5X01_ERR_FG_OFS, S6E36W5X01_ERR_FG_LEN),
	[READ_DSI_ERR] = RDINFO_INIT(dsi_err, DSI_PKT_TYPE_RD, S6E36W5X01_DSI_ERR_REG, S6E36W5X01_DSI_ERR_OFS, S6E36W5X01_DSI_ERR_LEN),
	[READ_SELF_DIAG] = RDINFO_INIT(self_diag, DSI_PKT_TYPE_RD, S6E36W5X01_SELF_DIAG_REG, S6E36W5X01_SELF_DIAG_OFS, S6E36W5X01_SELF_DIAG_LEN),
};

static DEFINE_RESUI(id, &s6e36w5x01_rditbl[READ_ID], 0);
static DEFINE_RESUI(coordinate, &s6e36w5x01_rditbl[READ_COORDINATE], 0);
static DEFINE_RESUI(code, &s6e36w5x01_rditbl[READ_CODE], 0);
static DEFINE_RESUI(elvss, &s6e36w5x01_rditbl[READ_ELVSS], 0);
static DEFINE_RESUI(mtp, &s6e36w5x01_rditbl[READ_MTP], 0);
static DEFINE_RESUI(date, &s6e36w5x01_rditbl[READ_DATE], 0);
static DEFINE_RESUI(octa_id, &s6e36w5x01_rditbl[READ_OCTA_ID], 0);
/* for brightness debugging */
static DEFINE_RESUI(aor, &s6e36w5x01_rditbl[READ_AOR], 0);
static DEFINE_RESUI(chip_id, &s6e36w5x01_rditbl[READ_CHIP_ID], 0);
static DEFINE_RESUI(rddpm, &s6e36w5x01_rditbl[READ_RDDPM], 0);
static DEFINE_RESUI(rddsm, &s6e36w5x01_rditbl[READ_RDDSM], 0);
static DEFINE_RESUI(err, &s6e36w5x01_rditbl[READ_ERR], 0);
static DEFINE_RESUI(err_fg, &s6e36w5x01_rditbl[READ_ERR_FG], 0);
static DEFINE_RESUI(dsi_err, &s6e36w5x01_rditbl[READ_DSI_ERR], 0);
static DEFINE_RESUI(self_diag, &s6e36w5x01_rditbl[READ_SELF_DIAG], 0);


static struct resinfo s6e36w5x01_restbl[] = {
	[RES_ID] = RESINFO_INIT(id, S6E36W5X01_ID, RESUI(id)),
	[RES_COORDINATE] = RESINFO_INIT(coordinate, S6E36W5X01_COORDINATE, RESUI(coordinate)),
	[RES_CODE] = RESINFO_INIT(code, S6E36W5X01_CODE, RESUI(code)),
	[RES_ELVSS] = RESINFO_INIT(elvss, S6E36W5X01_ELVSS, RESUI(elvss)),
	[RES_MTP] = RESINFO_INIT(mtp, S6E36W5X01_MTP, RESUI(mtp)),
	[RES_DATE] = RESINFO_INIT(date, S6E36W5X01_DATE, RESUI(date)),
	[RES_OCTA_ID] = RESINFO_INIT(octa_id, S6E36W5X01_OCTA_ID, RESUI(octa_id)),
	[RES_AOR] = RESINFO_INIT(aor, S6E36W5X01_AOR, RESUI(aor)),
	[RES_CHIP_ID] = RESINFO_INIT(chip_id, S6E36W5X01_CHIP_ID, RESUI(chip_id)),
	[RES_RDDPM] = RESINFO_INIT(rddpm, S6E36W5X01_RDDPM, RESUI(rddpm)),
	[RES_RDDSM] = RESINFO_INIT(rddsm, S6E36W5X01_RDDSM, RESUI(rddsm)),
	[RES_ERR] = RESINFO_INIT(err, S6E36W5X01_ERR, RESUI(err)),
	[RES_ERR_FG] = RESINFO_INIT(err_fg, S6E36W5X01_ERR_FG, RESUI(err_fg)),
	[RES_DSI_ERR] = RESINFO_INIT(dsi_err, S6E36W5X01_DSI_ERR, RESUI(dsi_err)),
	[RES_SELF_DIAG] = RESINFO_INIT(self_diag, S6E36W5X01_SELF_DIAG, RESUI(self_diag)),
};

enum {
	DUMP_RDDPM = 0,
	DUMP_RDDPM_SLEEP_IN,
	DUMP_RDDSM,
	DUMP_ERR,
	DUMP_ERR_FG,
	DUMP_DSI_ERR,
	DUMP_SELF_DIAG,
};

static void show_rddpm(struct dumpinfo *info);
static void show_rddpm_before_sleep_in(struct dumpinfo *info);
static void show_rddsm(struct dumpinfo *info);
static void show_err(struct dumpinfo *info);
static void show_err_fg(struct dumpinfo *info);
static void show_dsi_err(struct dumpinfo *info);
static void show_self_diag(struct dumpinfo *info);


static struct dumpinfo s6e36w5x01_dmptbl[] = {
	[DUMP_RDDPM] = DUMPINFO_INIT(rddpm, &s6e36w5x01_restbl[RES_RDDPM], show_rddpm),
	[DUMP_RDDPM_SLEEP_IN] = DUMPINFO_INIT(rddpm_sleep_in, &s6e36w5x01_restbl[RES_RDDPM], show_rddpm_before_sleep_in),
	[DUMP_RDDSM] = DUMPINFO_INIT(rddsm, &s6e36w5x01_restbl[RES_RDDSM], show_rddsm),
	[DUMP_ERR] = DUMPINFO_INIT(err, &s6e36w5x01_restbl[RES_ERR], show_err),
	[DUMP_ERR_FG] = DUMPINFO_INIT(err_fg, &s6e36w5x01_restbl[RES_ERR_FG], show_err_fg),
	[DUMP_DSI_ERR] = DUMPINFO_INIT(dsi_err, &s6e36w5x01_restbl[RES_DSI_ERR], show_dsi_err),
	[DUMP_SELF_DIAG] = DUMPINFO_INIT(self_diag, &s6e36w5x01_restbl[RES_SELF_DIAG], show_self_diag),
};

enum {
	S6E36W5_ACL_RATIO_0,
	S6E36W5_ACL_RATIO_8,
	S6E36W5_ACL_RATIO_15,
	MAX_S6E36W5_ACL_RATIO,
};

static int init_common_table(struct maptbl *);
static int init_gamma_mode2_brt_table(struct maptbl *tbl);
static int getidx_gamma_mode2_brt_table(struct maptbl *);
static void copy_common_maptbl(struct maptbl *, u8 *);

static int getidx_acl_control_table(struct maptbl *tbl);
static void copy_tset_maptbl(struct maptbl *tbl, u8 *dst);
static int getidx_hbm_transition_table(struct maptbl *tbl);
static int getidx_hbm_onoff_table(struct maptbl *tbl);

static inline bool is_id_ge_01(struct panel_device *panel);
static inline bool is_id_lt_03(struct panel_device *panel);
static inline bool is_watch4_panel(struct panel_device *panel);

static bool is_smooth_dimming_available(struct panel_device *panel);
static inline bool is_second_pmic_for_fresh_small(struct panel_device *panel);
static inline bool is_second_pmic_for_heart_small(struct panel_device *panel);

#endif /* __S6E36W5X01_H__ */
