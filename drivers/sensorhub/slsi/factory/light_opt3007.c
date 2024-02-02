/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "../ssp.h"

#define	VENDOR		"TI"
#define	CHIP_ID		"OPT3007"

#define CALIBRATION_FILE_PATH		"/efs/FactoryApp/light_cal_data"
#define CALIBRATION_DATA_AMOUNT     4

#define OPT3007_REG_EXPONENT(n)		((n) >> 12)
#define OPT3007_REG_MANTISSA(n)		((n) & 0xfff)
#define ChCoef			720 // 50.0
#define ChCoef_BOE		300 // 30.0


/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/
static s32 get_lsb_size(uint8_t e)
{
	s32 lsb_size = 0;

	switch (e) {
	case 0:
		lsb_size = 1;// 0.01f;
		break;
	case 1:
		lsb_size = 2;//0.02f;
		break;
	case 2:
		lsb_size = 4;//0.04f;
		break;
	case 3:
		lsb_size = 8;//0.08f;
		break;
	case 4:
		lsb_size = 16;//0.16f;
		break;
	case 5:
		lsb_size = 32;//0.32f;
		break;
	case 6:
		lsb_size = 64;//0.64f;
		break;
	case 7:
		lsb_size = 128;//1.28f;
		break;
	case 8:
		lsb_size = 256;//2.56f;
		break;
	case 9:
		lsb_size = 512;//5.12f;
		break;
	case 10:
		lsb_size = 1024;//10.24f;
		break;
	case 11:
		lsb_size = 2048;//20.48f;
		break;
	default:
		lsb_size = 0;
		break;
	}
	return lsb_size;
}

s32 light_get_lux(struct ssp_data *data, uint16_t m, uint8_t e)
{
	static s32 lux;
	static s32 lsb_size;

	lsb_size = (s32)get_lsb_size(e);

	if ((data->lcd_type & 0x00030000) == 0x00010000) {  // sdc : 0, boe : 1
		lux = (s32)((lsb_size * m * ChCoef_BOE) / 1000); /* lsb size 0.01, ChCoed 0.1 = 1000 */
		//pr_err("[SSP]: %s - BOE", __func__);
	}
	else {
		lux = (s32)((lsb_size * m * ChCoef) / 1000); /* lsb size 0.01, ChCoed 0.1 = 1000 */
		//pr_err("[SSP]: %s - SDC", __func__);
	}

	return lux;
}

static ssize_t light_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t light_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->lux);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	u16 result_r = (u16)(OPT3007_REG_MANTISSA(data->buf[LIGHT_SENSOR].rdata));
	u8 result_e = (u8)(OPT3007_REG_EXPONENT(data->buf[LIGHT_SENSOR].rdata));

	return snprintf(buf, PAGE_SIZE, "%u,%u\n", result_r, result_e);
}

int opt3007_light_open_calibration(struct ssp_data *data)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;
	struct light_calibration_data buffer;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			     O_RDONLY | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		memset(&data->lightcal, 0, sizeof(data->lightcal));

		pr_err("[SSP]: %s - Can't open calibration file %d\n", __func__, ret);
		return ret;
	}

	ret = vfs_read(cal_filp, (char *)&buffer, sizeof(buffer), &cal_filp->f_pos);
	if (ret != sizeof(buffer))
		ret = -EIO;

	memcpy(&data->lightcal, &buffer, sizeof(data->lightcal));

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int light_do_calibrate(struct ssp_data *data, int iEnable)
{
	s32 lux_sum = 0;
	int iRet = 0, iCount;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int send_buf[3];

	if (!iEnable) {
		data->lightcal.cal_status = 0;
		data->lightcal.cal_lux = 0;
		data->lightcal.curr_lux = 0;
	} else {
		data->lightcal.cal_status = 1;
		data->lightcal.cal_lux = 0;
		data->lightcal.curr_lux = 0;
		msleep(300);

		for (iCount = 0; iCount < CALIBRATION_DATA_AMOUNT; iCount++) {
			lux_sum += data->lux;
			mdelay(100);
		}
		data->lightcal.cal_lux = (lux_sum / CALIBRATION_DATA_AMOUNT);
		data->lightcal.curr_lux = data->lux;
	}

	ssp_dbg("[SSP]: do light calibrate %d, %d, %d\n",
		data->lightcal.cal_status, data->lightcal.cal_lux, data->lightcal.curr_lux);

	send_buf[0] = (int)data->lightcal.cal_status;
	send_buf[1] = (int)data->lightcal.cal_lux;
	send_buf[2] = (int)data->lightcal.curr_lux;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return iRet;
	}

	iRet = vfs_write(cal_filp, (char *)send_buf, 3 * sizeof(int), &cal_filp->f_pos);

	ssp_dbg("[SSP]:light calibrate write_buf %d, %d, %d %d\n",
		send_buf[0], send_buf[1], send_buf[2], iRet);

	if (iRet != 3 * sizeof(int)) {
		pr_err("[SSP]: %s - Can't write the lightcal to file\n",
			__func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return iRet;
}



static ssize_t light_cal_data_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	opt3007_light_open_calibration(data);
	pr_info("[SSP] %s: %d\n", __func__, data->lightcal.cal_status);

	data->lightcal.curr_lux = data->lux;

	iRet = sprintf(buf, "%d, %d, %d\n", data->lightcal.cal_status, data->lightcal.cal_lux, data->lightcal.curr_lux);

	return iRet;
}

static ssize_t light_cal_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet;
	int64_t dEnable;
	struct ssp_data *data = dev_get_drvdata(dev);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	mutex_lock(&data->sysfs_op_mtx);
	iRet = light_do_calibrate(data, (int)dEnable);
	mutex_unlock(&data->sysfs_op_mtx);

	if (iRet < 0)
		pr_err("[SSP]: %s - light_do_calibrate() failed\n", __func__);

	return iRet;
}


static DEVICE_ATTR(vendor, S_IRUGO, light_vendor_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, light_name_show, NULL);
static DEVICE_ATTR(lux, S_IRUGO, light_lux_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, light_data_show, NULL);
static DEVICE_ATTR(light_cal, S_IRUGO | S_IWUSR | S_IWGRP, light_cal_data_show, light_cal_data_store);

static struct device_attribute *light_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_lux,
	&dev_attr_raw_data,
	&dev_attr_light_cal,
	NULL,
};

void initialize_opt3007_light_factorytest(struct ssp_data *data)
{
	sensors_register(data->light_device, data, light_attrs, "light_sensor");
}

void remove_opt3007_light_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->light_device, light_attrs);
}
