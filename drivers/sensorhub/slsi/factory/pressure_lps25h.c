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

#define	VENDOR	"STM"
//#define	CHIP_ID	"LPS27H"
#define	CHIP_ID	"LPS28DFW"

#define CALIBRATION_FILE_PATH		"/efs/FactoryApp/baro_delta" //"/csa/sensor/baro_cal_data"
#define SW_OFFSET_FILE_PATH			"/efs/FactoryApp/baro_sw_offset"

#define	PR_ABS_MAX	8388607		/* 24 bit 2'compl */
#define	PR_ABS_MIN	-8388608

#define PRESSURE_SELFTEST_COUNT 5
#define CAL_DATA_LEN			10
#define SW_OFFSET_DATA_LEN		10
/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

static ssize_t sea_level_pressure_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet;

	struct ssp_data *data = dev_get_drvdata(dev);

	iRet = kstrtoint(buf, 10, &data->sealevelpressure);
	if (iRet < 0)
		return iRet;

	if (data->sealevelpressure == 0) {
		pr_info("%s, our->temperature = 0\n", __func__);
		data->sealevelpressure = -1;
	}

	pr_info("[SSP] %s sea_level_pressure = %d\n",
		__func__, data->sealevelpressure);
	return size;
}

static int parse_int(const char *s, unsigned int base, int *p)
{
	int res;
	int rv;
	int i;
	bool negative;

	res = 0;
	rv = 0;
	i = 0;
	negative = false;

	if (s[0] == '-') {
		i = 1;
		s++;
		negative = true;
	}

	for (i; i < CAL_DATA_LEN; i++) {
		unsigned int c = *s;
		unsigned int lc = c | 0x20; /* don't tolower() this line */
		unsigned int val;

		if ('0' <= c && c <= '9')
			val = c - '0';
		else if ('a' <= lc && lc <= 'f')
			val = lc - 'a' + 10;
		else
			break;

		if (val >= base)
			break;

		res = res * base + val;
		rv++;
		s++;
	}

	if (negative)
		res *= -1;
	*p = res;

	pr_err("[SSP]: %s - %llu\n", __func__, res);
	return rv;
}

int lps25h_pressure_open_calibration(struct ssp_data *data)
{
	char chBuf[CAL_DATA_LEN] = {0,};
	int iErr = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		iErr = PTR_ERR(cal_filp);
		if (iErr != -ENOENT)
			pr_err("[SSP]: %s - Can't open calibration file(%d)\n",
				__func__, iErr);
		set_fs(old_fs);
		return iErr;
	}

	iErr = vfs_read(cal_filp, chBuf, CAL_DATA_LEN * sizeof(char), &cal_filp->f_pos);

	if (iErr < 0) {
		pr_err("[SSP]: %s - Can't read the cal data from file (%d)\n",
			__func__, iErr);
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
		return iErr;
	}
	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	iErr = parse_int(chBuf, 10, &data->iPressureCal);

	if (iErr < 0) {
		pr_err("[SSP]: %s - kstrtoint failed. %d", __func__, iErr);
		return iErr;
	}

	ssp_dbg("[SSP]: calibration: %d\n", data->iPressureCal);

	if (data->iPressureCal < PR_ABS_MIN || data->iPressureCal > PR_ABS_MAX)
		pr_err("[SSP]: %s - wrong offset value!!!\n", __func__);

	return iErr;
}

int lps25h_set_pressure_cal(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg;
	s32 pressure_cal;
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	int sw_offset;
#endif

	if (!(data->uSensorState & (1 << PRESSURE_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!"\
			", pressure sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return iRet;
	}

	pressure_cal = data->iPressureCal;
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	sw_offset = data->sw_offset;
#endif

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
		return -ENOMEM;
	}
	msg->cmd = MSG2SSP_AP_MCU_SET_BARO_CAL;
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	msg->length = 8;
#else
	msg->length = 4;
#endif
	msg->options = AP2HUB_WRITE;
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	msg->buffer = (char *) kzalloc(8, GFP_KERNEL);
#else
	msg->buffer = (char *) kzalloc(4, GFP_KERNEL);
#endif
	msg->free_buffer = 1;
	memcpy(msg->buffer, &pressure_cal, 4);
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	memcpy(msg->buffer + 4, &sw_offset, 4);
#endif

	iRet = ssp_send_command(data, msg->cmd, msg->options, 0,
							msg->buffer, msg->length, NULL, NULL, msg->data);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	pr_info("[SSP] Set pressure cal data %d %d\n", pressure_cal, sw_offset);
#else
	pr_info("[SSP] Set pressure cal data %d\n", pressure_cal);
#endif
	kfree(msg->buffer);
	kfree(msg);
	return iRet;
}

#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
int open_pressure_sw_offset_file(struct ssp_data *data)
{
	char chBuf[SW_OFFSET_DATA_LEN] = {0,};
	int iErr = 0;
	mm_segment_t old_fs;
	struct file *off_set_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	off_set_filp = filp_open(SW_OFFSET_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(off_set_filp)) {
		iErr = PTR_ERR(off_set_filp);
		set_fs(old_fs);
		if (iErr != -ENOENT) {
			pr_err("[SSP]: %s - Can't open sw_offest file(%d)\n", __func__, iErr);
			return iErr;
		}
		pr_err("[SSP]: %s - sw offset file not exist (%d)\n", __func__, iErr);
		return 0;
	}

	iErr = vfs_read(off_set_filp, chBuf, SW_OFFSET_DATA_LEN * sizeof(char), &off_set_filp->f_pos);

	if (iErr < 0) {
		pr_err("[SSP]: %s - Can't read the sw_offset data from file (%d)\n",
			__func__, iErr);
		filp_close(off_set_filp, current->files);
		set_fs(old_fs);
		return iErr;
	}
	filp_close(off_set_filp, current->files);
	set_fs(old_fs);

	iErr = parse_int(chBuf, 10, &data->sw_offset);

	if (iErr < 0) {
		pr_err("[SSP]: %s - kstrtoint failed. %d", __func__, iErr);
		return iErr;
	}

	ssp_dbg("[SSP]: sw_offset: %d\n", data->sw_offset);

	return iErr;
}

static ssize_t pressure_sw_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->sw_offset);
}

static ssize_t pressure_sw_offset_store(struct device *dev, struct device_attribute *attr, const char *buf,
					 size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct file *sw_offset_filp = NULL;
	mm_segment_t old_fs;
	int sw_offset = 0;
	int iRet = 0;

	iRet = kstrtoint(buf, 10, &sw_offset);
	if (iRet < 0) {
		pr_err("[SSP]: %s - kstrtoint failed. %d\n", __func__, iRet);
		return iRet;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sw_offset_filp = filp_open(SW_OFFSET_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(sw_offset_filp)) {
		pr_err("[SSP]: %s - Can't open sw_offset file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(sw_offset_filp);
		return -EIO;
	}

	iRet = vfs_write(sw_offset_filp, buf, sizeof(buf), &sw_offset_filp->f_pos);

	if (iRet != sizeof(sw_offset)) {
		pr_err("[SSP]: %s - Can't write sw_offset to file\n",
			__func__);
		iRet = -EIO;
	}

	filp_close(sw_offset_filp, current->files);
	set_fs(old_fs);

	mutex_lock(&data->sysfs_op_mtx);
	data->sw_offset = sw_offset;
	set_pressure_cal(data);
	mutex_unlock(&data->sysfs_op_mtx);
	pr_info("[SSP] %s sw_offset %d", __func__, sw_offset);

	return size;
}
#endif

static ssize_t pressure_cabratioin_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iPressureCal = 0, iErr = 0;

	iErr = kstrtoint(buf, 10, &iPressureCal);
	if (iErr < 0) {
		pr_err("[SSP]: %s - kstrtoint failed.(%d)", __func__, iErr);
		return iErr;
	}

	if (iPressureCal < PR_ABS_MIN || iPressureCal > PR_ABS_MAX)
		return -EINVAL;

	mutex_lock(&data->sysfs_op_mtx);
	data->iPressureCal = (s32)iPressureCal;
	set_pressure_cal(data);
	mutex_unlock(&data->sysfs_op_mtx);

	return size;
}

static ssize_t pressure_cabratioin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->sysfs_op_mtx);
	pressure_open_calibration(data);
	mutex_unlock(&data->sysfs_op_mtx);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->iPressureCal);
}

/* sysfs for vendor & name */
static ssize_t pressure_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t pressure_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t raw_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->buf[PRESSURE_SENSOR].pressure[0]);
}

static ssize_t pressure_temperature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 temperature = 0;
	s32 float_temperature = 0;

	temperature = (s32) (data->buf[PRESSURE_SENSOR].pressure[1]);
	float_temperature = ((temperature%100) > 0 ? (temperature%100) : -(temperature%100));
	return sprintf(buf, "%d.%02d\n", (temperature/100), float_temperature);
}

static ssize_t pressure_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char selftest_ret = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, PRESSURE_SENSOR, 0, (char *)&selftest_ret, sizeof(selftest_ret), 1000, 1, __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", (int)selftest_ret);
}

static DEVICE_ATTR(raw_data, 0444, raw_data_read, NULL);
static DEVICE_ATTR(vendor, 0444, pressure_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, pressure_name_show, NULL);
static DEVICE_ATTR(calibration, 0664,
	pressure_cabratioin_show, pressure_cabratioin_store);
static DEVICE_ATTR(sea_level_pressure, 0220,
	NULL, sea_level_pressure_store);
static DEVICE_ATTR(temperature, 0444, pressure_temperature_show, NULL);
static DEVICE_ATTR(selftest, 0444, pressure_selftest_show, NULL);
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
static DEVICE_ATTR(sw_offset, 0664, pressure_sw_offset_show, pressure_sw_offset_store);
#endif

static struct device_attribute *pressure_attrs[] = {
	&dev_attr_raw_data,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_calibration,
	&dev_attr_sea_level_pressure,
	&dev_attr_temperature,
	&dev_attr_selftest,
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	&dev_attr_sw_offset,
#endif
	NULL,
};

void initialize_lps25h_pressure_factorytest(struct ssp_data *data)
{
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
	data->convert_coef = 4096/100;
#endif
	sensors_register(data->prs_device, data, pressure_attrs,
		"barometer_sensor");
}

void remove_lps25h_pressure_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->prs_device, pressure_attrs);
}
