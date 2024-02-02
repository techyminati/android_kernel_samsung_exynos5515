/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for LPD driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef ___SAMSUNG_LPD_H__
#define ___SAMSUNG_LPD_H__

// TEST
/* for "1: DMA test", need to modify DT
ex) at exynos9110-rmem.dtsi
lpd_rmem: lpd_rmem {
	compatible = "shared-dma-pool";
	reg = <0x0 0x9D000000 0x80000>;
	no-map;
};
*/
#define TEST_DRAM_ACCESS 0 /* 0: default, 1: DMA test*/ 
#define TEST_M55_RESET_RELEASE 0
#define TEST_M55_DPU_WAIT 0 /* wait for DPU off, temporal until PM sequence update*/
#define TEST_M55_NOTI_AP_STATE 0 /* inform suspend/resume to CM55 for display */
#define LPD_SRAM_POWER_GATING 1

#include "lpd_dbg.h"
#include "lpd_dev.h"
#include "lpd_fw.h"
#include "lpd_mem.h"
#include "lpd_noti.h"

#include <linux/platform_device.h>


#define NULL_CHECK(val) if(IS_ERR_OR_NULL(val)) {\
							pr_err(lpd_fmt("%s %d invalid argument\n"),__func__,__LINE__);\
							return -EINVAL;\
						}
#define ERR_CHECK(val) if(val < 0) {\
							pr_err(lpd_fmt("%s %d invalid parameter\n"),__func__,__LINE__);\
							return -EINVAL;\
						}

#define META_VERSION_MAJOR(version)	(version >> 16 & 0xff)
#define META_VERSION_MINOR(version)	(version >> 8 & 0xff)
#define META_VERSION_REVISION(version)	(version & 0xff)


/* operation state of lpd */
enum lpd_state {
	LPD_STATE_INIT,
	LPD_STATE_PREPARE,
	LPD_STATE_PM_ACQUIRE,
	LPD_STATE_START_BEGIN,
	LPD_STATE_START_END,
	LPD_STATE_STOP_BEGIN,
	LPD_STATE_STOP_END,
	LPD_STATE_PM_RELEASE,
};

/*
 for meta update */
enum update_req_state {
	UPDATE_REQ_DONE = 0,
	UPDATE_REQ_PREPARED,
};

enum lpd_fw_desc_id {
	SENSOR_DATA_ID = 0,
	META_DATA_ID,
	DPU_DATA_ID,
};

struct lpd_config {
	struct lpd_config_panel{
		unsigned int id;
	} panel;
};

struct lpd_device {
	enum lpd_state state;

	struct device *dev;

	struct lpd_config cfg;

	struct lpd_chardev_info chardev;
	struct lpd_reserved_memory rmem;
	struct lpd_sram_memory sram;

	struct lpd_fw_img_desc img_desc;

	struct lpd_notifier nb;

	struct lpd_debug d;

	int enable;

	struct mutex status_lock;
	struct mutex op_lock;

	struct lpd_fw_logbuf *sram_logbuf;

	enum update_req_state meta_update_req;

	int dpu_state;
};

static inline bool IS_LPD_START_STATE(struct lpd_device *lpd)
{
	if (lpd == NULL)
		return 0;
	return lpd->state == LPD_STATE_START_BEGIN || lpd->state == LPD_STATE_START_END;
}

static inline bool IS_LPD_STOP_STATE(struct lpd_device *lpd)
{
	if (lpd == NULL)
		return 0;
	return lpd->state == LPD_STATE_STOP_BEGIN || lpd->state == LPD_STATE_STOP_END;
}

// LPD can be on until STOP_END
static inline bool IS_LPD_ON_STATE(struct lpd_device *lpd)
{
	if (lpd == NULL)
		return 0;
	return IS_LPD_START_STATE(lpd) || lpd->state == LPD_STATE_STOP_BEGIN;
}

static inline bool IS_LPD_SRAM_AVAILABLE(struct lpd_device *lpd)
{
	if (lpd == NULL)
		return false;

	if (!(lpd->sram.state & LPD_SRAM_LPD_ON))
		return false;

	if (!(lpd->sram.state & LPD_SRAM_CHUB_ON))
		return false;

	if (lpd->sram.state & LPD_SRAM_CHUB_PG)
		return false;

	return true;
}


/**
 * Check display is on
 * @return		true if Disp is on, false on otherwise
 */


int lpd_prepare(struct lpd_device *lpd, struct lpd_fw_img_desc *desc);
int lpd_pm_acquire(struct lpd_device *lpd);
int lpd_start(struct lpd_device *lpd, enum lpd_notifier_steps step);
int lpd_stop(struct lpd_device *lpd, enum lpd_notifier_steps step);
int lpd_pm_release(struct lpd_device *lpd);

/* lpd_mem */
int lpd_sram_acquire(struct lpd_device *lpd, struct platform_device *pdev);
int lpd_sram_release(struct lpd_device *lpd);

/* lpd_mem */
int lpd_sram_acquire(struct lpd_device *lpd, struct platform_device *pdev);
int lpd_sram_release(struct lpd_device *lpd);
int lpd_sram_read(struct lpd_device *lpd, void *buffer, size_t size, int offset);
int lpd_sram_write(struct lpd_device *lpd, void *buffer, size_t size, int offset);
int lpd_sram_verify(struct lpd_device *lpd, void *buf, int off_d, int off_s, int size);
int lpd_sram_fw_info(struct lpd_device *lpd);
int lpd_sram_fw_download(struct lpd_device *lpd);
int lpd_sram_fw_on(struct lpd_device *lpd);
int lpd_rmem_acquire(struct lpd_device *lpd, struct platform_device *pdev);
int lpd_rmem_release(struct lpd_device *lpd);
int lpd_rmem_read(struct lpd_device *lpd, char **buffer, size_t size, int offset);
int lpd_rmem_to_sram(struct lpd_device *lpd, int off_d, int off_s, int size);

int lpd_set_state(struct lpd_device *lpd, enum lpd_state state);

int lpd_update_sensor_data(struct lpd_device *lpd, struct lpd_sensor_data *sensor_data);
int lpd_update_meta_data(struct lpd_device *lpd, lpd_meta_info *metadata);
int lpd_update_dpu_data(struct lpd_device *lpd, struct lpd_dpu_info *lpd_dpu);
int lpd_init_logbuf(struct lpd_device *lpd);


void lpd_print_img_desc(struct lpd_device *lpd);


/* lpd_dev */
int lpd_dev_init(struct lpd_device *lpd);
int lpd_dev_uninit(struct lpd_device *lpd);
/* lpd_dbg */
int lpd_dbg_init(struct lpd_device *lpd);
int lpd_dbg_uninit(struct lpd_device *lpd);
int lpd_logbuf_outprint(struct lpd_device *lpd);
int lpd_read_cm55_fault_status(struct lpd_device *lpd);

#if TEST_M55_NOTI_AP_STATE
int lpd_logbuf_noti_apstate(struct lpd_device *lpd, u8 apState);
#endif
/* lpd_noti */
int lpd_notifier_init(struct lpd_device *lpd);
int lpd_notifier_uninit(struct lpd_device *lpd);
#endif
