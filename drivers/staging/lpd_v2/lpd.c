/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for LPD driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "lpd.h"

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/pm_runtime.h>
#include <soc/samsung/cal-if.h>

struct lpd_device *lpd_drvdata;

static char *lpd_state_names[] = {
	"INIT",
	"PREPARE",
	"PM_ACQUIRE",
	"START_BEGIN",
	"START_END",
	"STOP_BEGIN",
	"STOP_END",
	"PM_RELEASE",
};

static int lpd_parse_dt(struct lpd_device *lpd, struct platform_device *pdev)
{
	int ret;
	struct device *dev;

	NULL_CHECK(lpd);
	dev = lpd->dev;

	NULL_CHECK(dev);

	if (!dev->of_node) {
		lpd_err("driver doesn't support non-dt\n");
		return -ENODEV;
	}

	ret = lpd_rmem_acquire(lpd, pdev);
	if (ret < 0) {
		lpd_err("driver failed to get reserved memory info\n");
		return -EINVAL;
	}

	ret = lpd_sram_acquire(lpd, pdev);
	if (ret < 0) {
		lpd_err("driver failed to get sram info\n");
		return -EINVAL;
	}
	return 0;

}

int lpd_set_state(struct lpd_device *lpd, enum lpd_state state)
{
	NULL_CHECK(lpd);
	mutex_lock(&lpd->status_lock);

	lpd_dbg("state changed %s -> %s\n", lpd_state_names[lpd->state], lpd_state_names[state]);
	lpd->state = state;

	mutex_unlock(&lpd->status_lock);
	return 0;
}

static int get_meta_data_offset(struct lpd_device *lpd, enum lpd_fw_desc_id desc_id)
{
	int offset;

	NULL_CHECK(lpd);

	offset = lpd->sram.off.desc;

	switch (desc_id) {
	case SENSOR_DATA_ID:
		offset += 0;
		break;

	case META_DATA_ID:
		offset += sizeof(struct lpd_sensor_data);
		break;

	case DPU_DATA_ID:
		offset += (sizeof(struct lpd_sensor_data) + sizeof(struct lpd_meta_info));
		break;

	default:
		lpd_err("ERR:%s undefined desc_id: %d\n", __func__, desc_id);
		return -1;
	}

	return offset;
}


static int check_resource_data(struct lpd_resource_info *rsc)
{
	size_t buf_size, rsc_size;
	char *rsc_buf_str[] = {
		"SRAM", "DRAM"
	};

	char *rsc_type_str[] = {
		"NONE",	"BG", "CANVAS",	"HANDS_HOUR", "HANDS_MIN", "HANDS_SEC",
		"HANDS_PIVOT", "COLON1", "COLON2", "ICON1", "ICON2", "FONT1",
		"FONT2", "FONT3", "FONT4","FONT5", "FONT6", "AMPM1", "AMPM2",
		"HOUR2", "MIN2"
	};

	if (rsc == NULL) {
		lpd_err("ERR:%s rsc is null\n", __func__);
		return -EINVAL;
	}

	if ((rsc->buf_type < 0) || (rsc->buf_type >= LPD_RSC_BUF_TYPE_MAX)) {
		lpd_err("ERR:%s: wrong buffer type: %d\n", __func__, rsc->buf_type);
		return -EINVAL;
	}

	if ((rsc->rsc_type < 0) || (rsc->rsc_type >= LPD_RSC_TYPE_MAX)) {
		lpd_err("ERR:%s: wrong resource type: %d\n", __func__, rsc->rsc_type);
		return -EINVAL;
	}

	buf_size = ARRAY_SIZE(rsc_buf_str);
	rsc_size = ARRAY_SIZE(rsc_type_str);
	if ((buf_size == LPD_RSC_BUF_TYPE_MAX) && (rsc_size == LPD_RSC_TYPE_MAX)) {
		lpd_info("rsc type       : %s\n", rsc_type_str[rsc->rsc_type]);
		lpd_info("rsc buffer type: %s\n", rsc_buf_str[rsc->buf_type]);
	} else {
		lpd_err("%s: Invalid array size: buf: %d(%d), rsc: %d(%d), check array size\n", __func__,
			buf_size, LPD_RSC_BUF_TYPE_MAX, rsc_size, LPD_RSC_TYPE_MAX);
	}

	lpd_info("rsc address    : 0x%x, size: 0x%x\n", rsc->addr, rsc->buf_size);

	return 0;
}


static int check_layout_data(struct lpd_layout_info *layout)
{
	size_t layout_size;
	char *layout_type_str[] = {
		"NONE", "ANALOG", "DIGITAL", "ICON1", "ICON2", "NUM1",
		"NUM2", "NUM3", "NUM4", "NUM5", "NUM6", "CLOCK",
		"D_NUM1", "D_NUM2", "D_NUM3", "D_NUM4"
	};

	if (layout == NULL) {
		lpd_err("ERR:%s: layout is null\n", __func__);
		return -EINVAL;
	}

	if ((layout->layout_type < 0) || (layout->layout_type >= LPD_LAYOUT_TYPE_MAX)) {
		lpd_err("ERR:%s: wrong layout type: %d\n", __func__, layout->layout_type);
		return -EINVAL;
	}

	layout_size = ARRAY_SIZE(layout_type_str);
	if (layout_size == LPD_LAYOUT_TYPE_MAX)
		lpd_info("layout type    : %s\n", layout_type_str[layout->layout_type]);
	else
		lpd_err("%s: Invalid layout array size: %d(%d) check array size\n",
			__func__, layout_size, LPD_LAYOUT_TYPE_MAX);

	return 0;
}



static int check_meta_data(struct lpd_device *lpd, lpd_meta_info *meta_data)
{
	int i;
	int ret = 0;
	struct lpd_resource_info *resource = NULL;
	struct lpd_layout_info *layout = NULL;

	lpd_info("meta_data->enable: %d\n", meta_data->enable);

	if (meta_data->enable == false)
		return 0;

	lpd_info("%s: meta_data->rsc: %d\n", __func__, meta_data->resource_cnt);
	lpd_info("%s: meta_data->layout_cnt: %d\n", __func__, meta_data->layout_cnt);

	if ((meta_data->resource_cnt <= 0) || (meta_data->layout_cnt <= 0)) {
		lpd_err("ERR:%s: invalid resource cnt: %d, layout cnt: %d\n", __func__,
			meta_data->resource_cnt, meta_data->layout_cnt);
		return -EINVAL;
	}


	for (i = 0; i < meta_data->resource_cnt; i++) {
		resource = &meta_data->resources[i];
		ret = check_resource_data(resource);
		if (ret) {
			lpd_err("ERR:%s: invalid resoruce data index: %d\n", __func__, i);
			return ret;
		}

	}

	for (i = 0; i < meta_data->layout_cnt; i++) {
		layout = &meta_data->layouts[i];
		ret = check_layout_data(layout);
		if (ret) {
			lpd_err("ERR:%s: invalid layout data index: %d\n", __func__, i);
			return ret;
		}
	}
	return ret;

}




int lpd_update_sensor_data(struct lpd_device *lpd, struct lpd_sensor_data *sensor_data)
{
	int i;
	int ret = 0;
	int offset;

	NULL_CHECK(lpd);
	NULL_CHECK(sensor_data);

	lpd_dbg("%s tick: %d\n", __func__, sensor_data->tick);

	if (sensor_data->tick == 0) {
		sensor_data->tick = 60;
		sensor_data->data_cnt = 0;

		for (i = 0; i < MAX_SENSOR_DATA; i++)
			sensor_data->id_list[i] = 0;
	}

	offset = get_meta_data_offset(lpd, SENSOR_DATA_ID);
	if (offset < 0) {
		lpd_err("ERR:%s: wrong offset: %d\n", __func__, offset);
		return offset;
	}
	lpd_dbg("%s: sram offset 0x%x size 0x%x\n", __func__, offset, sizeof(struct lpd_sensor_data));

	ret = lpd_sram_write(lpd, sensor_data, sizeof(struct lpd_sensor_data), offset);

	return ret;
}


int lpd_update_meta_data(struct lpd_device *lpd, lpd_meta_info *metadata)
{
	int ret = 0;
	int idx, offset_s = 0;
	int addr;
	size_t size;
	int offset = 0;

	NULL_CHECK(lpd);
	NULL_CHECK(metadata);

	ret = check_meta_data(lpd, metadata);
	if (ret) {
		lpd_err("ERR:%s: wrong meta data\n", __func__);
		return -EINVAL;
	}

	offset_s = lpd->sram.off.img;
	for (idx = 0; idx < metadata->resource_cnt; idx++) {
		if (metadata->resources[idx].buf_type == LPD_RSC_BUF_TYPE_SRAM) {
			addr = metadata->resources[idx].addr;
			metadata->resources[idx].addr = offset_s;
			size = metadata->resources[idx].buf_size;
			lpd_dbg(" %d: addr 0x%x-> 0x%x size 0x%x\n", idx, addr, offset_s, size);
			ret = lpd_rmem_to_sram(lpd, addr, offset_s, size);
			ERR_CHECK(ret);
			offset_s += size;
		}
	}
	offset = get_meta_data_offset(lpd, META_DATA_ID);
	if (offset < 0) {
		lpd_err("ERR:%s: wrong offset: %d\n", __func__, offset);
		return offset;
	}
	lpd_dbg("%s: sram offset 0x%x size 0x%x\n", __func__, offset, sizeof(struct lpd_meta_info));

	ret = lpd_sram_write(lpd, metadata, sizeof(struct lpd_meta_info), offset);
	ERR_CHECK(ret);

	lpd_sram_verify(lpd, metadata, 0, offset, sizeof(struct lpd_meta_info));

	return ret;
}


int lpd_update_dpu_data(struct lpd_device *lpd, struct lpd_dpu_info *lpd_dpu)
{
	int ret;
	int offset = 0;

	NULL_CHECK(lpd);
	NULL_CHECK(lpd_dpu);

	offset = get_meta_data_offset(lpd, DPU_DATA_ID);
	if (offset < 0) {
		lpd_err("ERR:%s: wrong offset: %d\n", __func__, offset);
		return offset;
	}
	lpd_dbg("%s: sram offset 0x%x size 0x%x\n", __func__, offset, sizeof(struct lpd_dpu_info));

	ret = lpd_sram_write(lpd, lpd_dpu, sizeof(struct lpd_dpu_info), offset);

	return ret;
}


int lpd_prepare_sram(struct lpd_device *lpd)
{
	int ret;
	//int addr;
	//size_t size;
	lpd_meta_info *meta_data;
	struct lpd_sensor_data *sensor_data;		/* sensor data */
	struct lpd_dpu_info *dpu_data;
	struct lpd_fw_img_desc *desc;

	NULL_CHECK(lpd);
	desc = &lpd->img_desc;
	NULL_CHECK(desc);

	meta_data = &desc->meta;
	NULL_CHECK(meta_data);

	sensor_data = &desc->sensor;
	NULL_CHECK(sensor_data);

	dpu_data = &desc->dpu;
	NULL_CHECK(dpu_data);

	// download LPW fw on every lpd start
	ret = lpd_sram_fw_download(lpd);
	ERR_CHECK(ret);

	ret = lpd_sram_fw_on(lpd); // Just for test purpose
	ERR_CHECK(ret);

	ret = lpd_sram_fw_info(lpd);
	ERR_CHECK(ret);

	ret = lpd_update_meta_data(lpd, meta_data);
	if (ret != sizeof(lpd_meta_info)) {
		lpd_err("ERR:%s: failed to update meta data: %d\n", __func__, ret);
		return -EINVAL;
	}

	ret = lpd_update_sensor_data(lpd, sensor_data);
	if (ret != sizeof(struct lpd_sensor_data)) {
		lpd_err("ERR:%s: failed to update sensor data: %d\n", __func__, ret);
		return -EINVAL;
	}

	ret = lpd_update_dpu_data(lpd, dpu_data);
	if (ret != sizeof(struct lpd_dpu_info)) {
		lpd_err("ERR:%s: failed to update dpu data: %d\n", __func__, ret);
		return -EINVAL;
	}

	ret = lpd_init_logbuf(lpd);
	if (ret < 0) {
		lpd_err("ERR:%s: failed to init log buf: %d\n", __func__, ret);
		return ret;
	}

	return 0;

}

void lpd_print_img_desc(struct lpd_device *lpd)
{
	int i;
	struct lpd_dpu_info *dpu;
	struct lpd_meta_info *meta;
	struct lpd_sensor_data *sensor;
	struct lpd_fw_img_desc *img_desc;

	if (lpd == NULL) {
		lpd_err("%s lpd is null\n", __func__);
		return;
	}

	img_desc = &lpd->img_desc;
	sensor = &img_desc->sensor;

	lpd_info("************* sensor info *************\n");
	lpd_info("tick       :  %ds\n", sensor->tick);
	lpd_info("data_cnt   :  %d\n", sensor->data_cnt);
	for (i = 0; i < MAX_SENSOR_DATA; i++)
		lpd_info("data_id[%d]: %d\n", i, sensor->id_list[i]);

	meta = &img_desc->meta;
	lpd_info("************* meta info *************\n");
	lpd_info("lpd enable: %d\n", meta->enable);

	dpu = &img_desc->dpu;
	lpd_info("************* dpu info *************\n");
	lpd_info("xres: %d, yres: %d\n", dpu->panel.xres, dpu->panel.yres);

}


// wait until dpu is completely off, tempral solution until pmu is ready
static void lpd_wait_for_dpuoff(void)
{
#if TEST_M55_DPU_WAIT
	u32 status;
	u32 cnt = 10000;
	void __iomem *conf_reg = ioremap(0x12861D00, 0x08);
	void __iomem *status_reg = ioremap(0x12861D04, 0x08);
	void __iomem *states_reg = ioremap(0x12861D08, 0x08);

	// Wait until the DPU  power block is completely turned off
	do {
		status = __raw_readl(status_reg);
		cnt--;
		udelay(10);
	} while (status && cnt);

	if (!cnt)
		lpd_err("%s, DPU power off timeout!, cfg:%x,status:%x, state:%x\n", __func__,
				__raw_readl(conf_reg), __raw_readl(status_reg), __raw_readl(states_reg));
	else
		lpd_dbg("%s, DPU power off success cfg:%x,status:%x, state:%x\n", __func__,
				__raw_readl(conf_reg), __raw_readl(status_reg), __raw_readl(states_reg));
#endif
}


// START_PRE (pm get sync) -> DPU Disable (pm put sync) -> START
int lpd_pm_acquire(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);

	if (lpd->enable == false) {
		lpd_err("%s: lpd disabled by user\n", __func__);
		return -EINVAL;
	}

	if (lpd->state == LPD_STATE_INIT) {
		lpd_err("%s not allowed as not ready to operate\n", __func__);
		return -EINVAL;
	}

	if (IS_LPD_START_STATE(lpd)){
		lpd_err("%s not allowed as already started\n", __func__);
		return -EINVAL;
	}

	lpd->d.pm_refcnt++;
	if (lpd->d.pm_refcnt != 1)
		lpd_err("%s invalid refcnt %d\n", __func__, lpd->d.pm_refcnt);

	pm_runtime_get_sync(lpd->dev);

	//LPD SRAM power on
	cal_lpd_sram_power_control(true);
	lpd->sram.state |= LPD_SRAM_LPD_ON;

	lpd_set_state(lpd, LPD_STATE_PM_ACQUIRE);

	return 0;
}

int lpd_start(struct lpd_device *lpd, enum lpd_notifier_steps step)
{
	int ret;

	NULL_CHECK(lpd);

	if (lpd->enable == false) {
		lpd_err("%s: lpd disabled by user\n", __func__);
		return -EINVAL;
	}

	switch(step) {
	case LPD_NOTIFIER_STEPS_BEGIN:
		if (lpd->state != LPD_STATE_PM_ACQUIRE){
			lpd_dbg("%s require to call PM Acquire first\n",__func__);
			return -1;
		}

		ret = lpd_prepare_sram(lpd);
		ERR_CHECK(ret);

		lpd_wait_for_dpuoff();
		lpd_set_state(lpd, LPD_STATE_START_BEGIN);
		break;

	case LPD_NOTIFIER_STEPS_END:
		if(lpd->state != LPD_STATE_START_BEGIN){
			lpd_err("%s require to call START first\n",__func__);
			return -1;
		}
		lpd_set_state(lpd, LPD_STATE_START_END);
		break;
	}

	return 0;
}

// STOP -> DPU Enable (pm get sync) -> STOP POST (pm put sync)
int lpd_stop(struct lpd_device *lpd, enum lpd_notifier_steps step)
{
	NULL_CHECK(lpd);

	switch(step){
	case LPD_NOTIFIER_STEPS_BEGIN:
		if (!IS_LPD_START_STATE(lpd)) {
			lpd_info("%s not allowed as not started\n",__func__);
			return -1;
		}
		lpd_set_state(lpd, LPD_STATE_STOP_BEGIN);
		break;

	case LPD_NOTIFIER_STEPS_END:
		if (lpd->state != LPD_STATE_STOP_BEGIN) {
			lpd_err("%s require to call STOP first\n",__func__);
			return -1;
		}
		lpd_logbuf_outprint(lpd); //print out all the CM55 log after stop
		lpd_read_cm55_fault_status(lpd);
		lpd_set_state(lpd, LPD_STATE_STOP_END);
		break;
	}

	return 0;
}

int lpd_pm_release(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);

	if (!IS_LPD_STOP_STATE(lpd)) {
		lpd_err("%s require to call STOP first\n", __func__);
		return -1;
	}

	// LPD SRAM power off
	cal_lpd_sram_power_control(false);
	lpd->sram.state &= ~LPD_SRAM_LPD_ON;

	lpd->d.pm_refcnt--;
	if(lpd->d.pm_refcnt != 0)
		lpd_err("%s invalid refcnt %d\n",__func__, lpd->d.pm_refcnt);

	pm_runtime_put_sync(lpd->dev);
	lpd_set_state(lpd, LPD_STATE_PM_RELEASE);
	return 0;

}

static int lpd_probe(struct platform_device *pdev)
{
	struct device *dev;
	struct lpd_device *lpd;
	int ret = 0;

	NULL_CHECK(pdev);
	dev = &pdev->dev;

	NULL_CHECK(dev);
	lpd = devm_kzalloc(dev, sizeof(struct lpd_device), GFP_KERNEL);
	NULL_CHECK(lpd);
	lpd->dev = dev;

	lpd->sram_logbuf = devm_kzalloc(dev, sizeof(struct lpd_fw_logbuf), GFP_KERNEL);
	if (lpd->sram_logbuf == NULL) {
		lpd_err("%s: failed to alloc sram logbuf\n", __func__);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, lpd);

	/* parse dt and hw init */
	ret = lpd_parse_dt(lpd, pdev);
	if (ret) {
		lpd_err("%s failed to get init hw with ret %d\n", __func__, ret);
		goto err;
	}

	lpd_drvdata = lpd;

	pm_runtime_enable(dev);

	lpd_notifier_init(lpd);

	ret = lpd_dev_init(lpd);
	if (ret) {
		lpd_err("failed to register chrdev\n");
		goto err;
	}

	lpd_dbg_init(lpd);

	mutex_init(&lpd->status_lock);
	mutex_init(&lpd->op_lock);

	lpd_set_state(lpd, LPD_STATE_INIT);

	lpd->enable = false;
	lpd->dpu_state = 0;

	lpd_info("%s is done. ret:%d\n", __func__, ret);
	return 0;

err:
	lpd_err("%s is failed with ret %d\n", __func__, ret);

	lpd_dbg_uninit(lpd);
	lpd_dev_uninit(lpd);

	lpd_notifier_uninit(lpd);

	if(lpd->rmem.reserved)
		lpd_rmem_release(lpd);
	lpd_sram_release(lpd);

	if (lpd->sram_logbuf)
		devm_kfree(dev, lpd->sram_logbuf);

	devm_kfree(dev, lpd);
	return ret;
}

static int lpd_remove(struct platform_device *pdev)
{
	struct lpd_device *lpd = platform_get_drvdata(pdev);
	NULL_CHECK(lpd);

	pm_runtime_disable(lpd->dev);
	lpd_dbg_uninit(lpd);
	lpd_dev_uninit(lpd);

	lpd_notifier_uninit(lpd);

	if(lpd->rmem.reserved)
		lpd_rmem_release(lpd);

	if (lpd->sram_logbuf)
		devm_kfree(lpd->dev, lpd->sram_logbuf);

	return 0;
}

static void lpd_shutdown(struct platform_device *pdev)
{
	struct lpd_device *lpd = platform_get_drvdata(pdev);

	if (lpd == NULL)
		return;

	pm_runtime_disable(lpd->dev);
	lpd_dbg_uninit(lpd);
	lpd_dev_uninit(lpd);

	lpd_notifier_uninit(lpd);

	if(lpd->rmem.reserved)
		lpd_rmem_release(lpd);
}

// call sequence: chub suspend -> lpd suspend -> chub suspend noirq (off by p/g)
static int lpd_suspend(struct device *dev)
{
	struct lpd_device *lpd = dev_get_drvdata(dev);

	NULL_CHECK(lpd);

#if TEST_M55_NOTI_AP_STATE
	if(IS_LPD_SRAM_AVAILABLE(lpd))
		lpd_logbuf_noti_apstate(lpd, 1); // sleep
#endif

	lpd->sram.state |= LPD_SRAM_CHUB_PG;

	return 0;
}

// call sequence: chub resume noirq (on from p/g) -> lpd resume -> chub resume
static int lpd_resume(struct device *dev)
{
	struct lpd_device *lpd = dev_get_drvdata(dev);

	NULL_CHECK(lpd);

	lpd->sram.state &= ~LPD_SRAM_CHUB_PG;

#if TEST_M55_NOTI_AP_STATE
	if(IS_LPD_SRAM_AVAILABLE(lpd))
		lpd_logbuf_noti_apstate(lpd, 0); // wake up
#endif

	return 0;
}

bool disp_is_on(void)
{
	return IS_LPD_ON_STATE(lpd_drvdata);
}
EXPORT_SYMBOL(disp_is_on);

static const struct dev_pm_ops lpd_pm_ops = {
	.suspend 	= lpd_suspend,
	.resume		= lpd_resume,
};

static const struct of_device_id lpd_of_match[] = {
	{.compatible = "samsung,exynos-lpd"},
	{},
};

static struct platform_driver lpd_driver = {
	.probe 		= lpd_probe,
	.remove 	= lpd_remove,
	.shutdown	= lpd_shutdown,
	.driver = {
		   .name = "exynos-lpd",
		   .owner = THIS_MODULE,
		   .of_match_table = lpd_of_match,
		   .pm = &lpd_pm_ops,
	},
};

int lpd_init(void)
{
	return platform_driver_register(&lpd_driver);
}

static void __exit lpd_cleanup(void)
{
	platform_driver_unregister(&lpd_driver);
}

module_init(lpd_init);
module_exit(lpd_cleanup);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Exynos Low Power Display");
MODULE_AUTHOR("Dongho Lee <dh205.lee@samsung.com>");
MODULE_AUTHOR("Yeonjun Kim <yenjin.kim@samsung.com>");
