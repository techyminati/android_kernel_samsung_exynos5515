/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Samsung EXYNOS Panel Information.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_PANEL_H__
#define __EXYNOS_PANEL_H__

enum decon_psr_mode {
	DECON_VIDEO_MODE = 0,
	DECON_DP_PSR_MODE = 1,
	DECON_MIPI_COMMAND_MODE = 2,
};

enum type_of_ddi {
	TYPE_OF_SM_DDI = 0,
	TYPE_OF_MAGNA_DDI,
	TYPE_OF_NORMAL_DDI,
};

#define MAX_RES_NUMBER		5
#define HDR_CAPA_NUM		4
#if IS_ENABLED(CONFIG_MCD_PANEL)
#define MAX_DDI_NAME_LEN	50
#endif

struct lcd_res_info {
	unsigned int width;
	unsigned int height;
	unsigned int dsc_en;
	unsigned int dsc_width;
	unsigned int dsc_height;
};

struct lcd_mres_info {
	u32 en;
	u32 number;
	struct lcd_res_info res_info[MAX_RES_NUMBER];
};

struct lcd_hdr_info {
	unsigned int num;
	unsigned int type[HDR_CAPA_NUM];
	unsigned int max_luma;
	unsigned int max_avg_luma;
	unsigned int min_luma;
};

/*
 * dec_sw : slice width in DDI side
 * enc_sw : slice width in AP(DECON & DSIM) side
 */
struct dsc_slice {
	unsigned int dsc_dec_sw[MAX_RES_NUMBER];
	unsigned int dsc_enc_sw[MAX_RES_NUMBER];
};

struct stdphy_pms {
	unsigned int p;
	unsigned int m;
	unsigned int s;
	unsigned int k;
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
	unsigned int mfr;
	unsigned int mrr;
	unsigned int sel_pf;
	unsigned int icp;
	unsigned int afc_enb;
	unsigned int extafc;
	unsigned int feed_en;
	unsigned int fsel;
	unsigned int fout_mask;
	unsigned int rsel;
#endif
};

struct exynos_dsc {
	u32 en;
	u32 cnt;
	u32 slice_num;
	u32 slice_h;
	u32 dec_sw;
	u32 enc_sw;
};

#if IS_ENABLED(CONFIG_MCD_PANEL)
enum {
	EXYNOS_PANEL_VRR_NS_MODE = 0,
	EXYNOS_PANEL_VRR_HS_MODE = 1,
	EXYNOS_PANEL_VRR_PASSIVE_HS_MODE = 2,
};

static inline char *EXYNOS_VRR_MODE_STR(int vrr_mode)
{
	if (vrr_mode == EXYNOS_PANEL_VRR_NS_MODE)
		return "NS";
	else if (vrr_mode == EXYNOS_PANEL_VRR_HS_MODE)
		return "HS";
	else if (vrr_mode == EXYNOS_PANEL_VRR_PASSIVE_HS_MODE)
		return "pHS";
	else
		return "NONE";
}

static inline bool IS_EXYNOS_VRR_NS_MODE(int vrr_mode)
{
	return (vrr_mode == EXYNOS_PANEL_VRR_NS_MODE);
}

static inline bool IS_EXYNOS_VRR_HS_MODE(int vrr_mode)
{
	return (vrr_mode == EXYNOS_PANEL_VRR_HS_MODE ||
			vrr_mode == EXYNOS_PANEL_VRR_PASSIVE_HS_MODE);
}

static inline bool IS_EXYNOS_VRR_PASSIVE_MODE(int vrr_mode)
{
	return (vrr_mode == EXYNOS_PANEL_VRR_PASSIVE_HS_MODE);
}
#endif

#define MAX_DISPLAY_MODE		32
/* exposed to user */
struct exynos_display_mode {
	unsigned int index;
	unsigned int width;
	unsigned int height;
	unsigned int mm_width;
	unsigned int mm_height;
	unsigned int fps;
	unsigned int group;
};

struct exynos_display_mode_info {
	struct exynos_display_mode mode;
	unsigned int cmd_lp_ref;
	bool dsc_en;
	unsigned int dsc_width;
	unsigned int dsc_height;
	unsigned int dsc_dec_sw;
	unsigned int dsc_enc_sw;
#if IS_ENABLED(CONFIG_MCD_PANEL)
	void *pdata;		/* used by common panel driver */
#endif
};
#if IS_ENABLED(CONFIG_MCD_PANEL)
struct exynos_display_modes {
	/*
	 * exynos_display_mode_info get from
	 * panel_display_modes.
	 */
	unsigned int num_mode_infos;
	unsigned int native_mode_info;
	struct exynos_display_mode_info **mode_infos;

	/*
	 * exynos_display_mode get from @mode_infos
	 */
	unsigned int num_modes;
	unsigned int native_mode;
	struct exynos_display_mode **modes;
};

#define MAX_COLOR_MODE		5

struct  panel_color_mode {
	int cnt;
	int mode[MAX_COLOR_MODE];
};
#endif

struct exynos_panel_info {
	unsigned int id; /* panel id. It is used for finding connected panel */
	enum decon_psr_mode mode;

	unsigned int vfp;
	unsigned int vbp;
	unsigned int hfp;
	unsigned int hbp;
	unsigned int vsa;
	unsigned int hsa;
#if IS_ENABLED(CONFIG_MCD_PANEL)
	unsigned int v_blank_t;
#endif
	/* resolution */
	unsigned int xres;
	unsigned int yres;
#if IS_ENABLED(CONFIG_MCD_PANEL)
	unsigned int old_xres;
	unsigned int old_yres;
#endif

	/* physical size */
	unsigned int width;
	unsigned int height;

	unsigned int hs_clk;
	struct stdphy_pms dphy_pms;
	unsigned int esc_clk;

	unsigned int fps;
#if IS_ENABLED(CONFIG_MCD_PANEL)
	unsigned int cmd_lp_ref;
	unsigned int req_vrr_fps;
	unsigned int req_vrr_mode;
	unsigned int vrr_mode;
	unsigned int panel_vrr_fps;
	unsigned int panel_vrr_mode;
	unsigned int panel_te_sw_skip_count;
	unsigned int panel_te_hw_skip_count;
#endif

	struct exynos_dsc dsc;

	enum type_of_ddi ddi_type;
	unsigned int data_lane;
	unsigned int vt_compensation;
	unsigned int mres_mode;
	struct lcd_mres_info mres;
	struct lcd_hdr_info hdr;
	unsigned int bpc;
	unsigned int continuous_underrun_max;

	int display_mode_count;
#if defined(CONFIG_EXYNOS_DECON_DQE)
	char ddi_name[MAX_DDI_NAME_LEN];
#endif
	unsigned int cur_mode_idx;
#if IS_ENABLED(CONFIG_MCD_PANEL)
	unsigned int panel_mode_idx;
#endif
	struct exynos_display_mode_info display_mode[MAX_DISPLAY_MODE];
#if IS_ENABLED(CONFIG_MCD_PANEL)
	struct panel_color_mode color_mode;
#endif
#if defined(CONFIG_PANEL_DISPLAY_MODE)
	unsigned int cur_exynos_mode;
	unsigned int req_exynos_mode;
	struct exynos_display_modes *exynos_modes;
	struct panel_display_modes *panel_modes;
#endif
};
#endif /* __EXYNOS_PANEL_H__ */
