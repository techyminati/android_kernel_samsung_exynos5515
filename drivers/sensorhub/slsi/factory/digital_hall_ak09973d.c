/*
 * Copyright (c)2021, Samsung Electronics Co., Ltd.
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
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/ctype.h>

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"AKM"
#define CHIP_ID		"AK09973D"

#define HALL_AUTO_CAL_X_PATH "/efs/FactoryApp/digital_hall_auto_cal_x"
#define HALL_AUTO_CAL_Y_PATH "/efs/FactoryApp/digital_hall_auto_cal_y"
#define HALL_AUTO_CAL_Z_PATH "/efs/FactoryApp/digital_hall_auto_cal_z"

#define HALL_AUTO_CAL_DATA_NUM		19
#define HALL_AUTO_CAL_FILE_BUF_LEN	140

#define HALL_SELFTEST_LENGTH	19
#define HALL_CAL_DATA_LENGTH	232

#define HALL_CMD_SELFTEST	0
#define HALL_CMD_READ_CAL	2
#define HALL_CMD_WRITE_CAL	3

#define HALL_ANGLE_SPEC 15

#define HALL_RESULT_PASS 0
#define HALL_RESULT_FAIL -1

#define HALL_UEVENT_STR_LEN 15

struct hall_selftest_data{
	u8 result;
	s16 measure[6];
	s16 selftest[3];
} __attribute__ ((packed));

void digital_hall_send_uevent(struct ssp_data *data)
{
	char *uevent_val[3] = {0, };
	char sw_int[HALL_UEVENT_STR_LEN] = {0, };
	char hw_int[HALL_UEVENT_STR_LEN] = {0, };

	if (data->buf[DIGITAL_HALL_EVENT_SENSOR].direction == 0) {
		return;
	}

	sprintf(sw_int, "SW_INT=%d", data->buf[DIGITAL_HALL_EVENT_SENSOR].direction);
	sprintf(hw_int, "HW_INT=%d", data->buf[DIGITAL_HALL_EVENT_SENSOR].interrupt);

	uevent_val[0] = sw_int;
	uevent_val[1] = hw_int;
	uevent_val[2] = NULL;

	if (!data->dhallic_device)
		return;

	kobject_uevent_env(&data->dhallic_device->kobj, KOBJ_CHANGE, uevent_val);
	pr_info("[SSP] %s: %s, %s\n", __func__, uevent_val[0], uevent_val[1]);
}


static ssize_t digital_hall_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t digital_hall_raw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s: ang1(%d), ang2(%d)\n", __func__, data->buf[DIGITAL_HALL_RAW_SENSOR].angle, data->buf[DIGITAL_HALL_SUB_SENSOR].angle);

	return snprintf(buf, PAGE_SIZE, "%d %d\n",
		data->buf[DIGITAL_HALL_RAW_SENSOR].angle, data->buf[DIGITAL_HALL_SUB_SENSOR].angle);
}

static ssize_t digital_hall_selftest1_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = -1;
	struct hall_selftest_data selftest;
	struct ssp_data *data = dev_get_drvdata(dev);

	ret = ssp_send_read_factory_cmd(data, DIGITAL_HALL_RAW_SENSOR, HALL_CMD_SELFTEST, (char *)&selftest, sizeof(selftest), 1000, HALL_SELFTEST_LENGTH, __func__);
	if (ret != SUCCESS) {
		selftest.result = 1; // 	DIGITAL_HALL_SELFTEST_NG_I2C
	}

	pr_info("[SSP] %s: %d %d %d %d %d %d %d %d %d %d\n",
		__func__, selftest.result,
		selftest.measure[0],	selftest.measure[1], selftest.measure[2],
		selftest.measure[3],	selftest.measure[4], selftest.measure[5],
		selftest.selftest[0],	selftest.selftest[1], selftest.selftest[2]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		selftest.result,
		selftest.measure[0],	selftest.measure[1], selftest.measure[2],
		selftest.measure[3],	selftest.measure[4], selftest.measure[5],
		selftest.selftest[0],	selftest.selftest[1], selftest.selftest[2]);
}

static ssize_t digital_hall_selftest2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = -1;
	struct hall_selftest_data selftest;
	struct ssp_data *data = dev_get_drvdata(dev);

	ret = ssp_send_read_factory_cmd(data, DIGITAL_HALL_SUB_SENSOR, HALL_CMD_SELFTEST, (char *)&selftest, sizeof(selftest), 1000, HALL_SELFTEST_LENGTH, __func__);
	if (ret != SUCCESS) {
		selftest.result = 1; // 	DIGITAL_HALL_SELFTEST_NG_I2C
	}

	pr_info("[SSP] %s: %d %d %d %d %d %d %d %d %d %d\n",
		__func__, selftest.result,
		selftest.measure[0],	selftest.measure[1], selftest.measure[2],
		selftest.measure[3],	selftest.measure[4], selftest.measure[5],
		selftest.selftest[0],	selftest.selftest[1], selftest.selftest[2]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		selftest.result,
		selftest.measure[0],	selftest.measure[1], selftest.measure[2],
		selftest.measure[3],	selftest.measure[4], selftest.measure[5],
		selftest.selftest[0],	selftest.selftest[1], selftest.selftest[2]);
}

static ssize_t digital_hall_read_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s: dir(%d), int(%u)\n", __func__, data->buf[DIGITAL_HALL_EVENT_SENSOR].direction, data->buf[DIGITAL_HALL_EVENT_SENSOR].interrupt);

	return snprintf(buf, PAGE_SIZE, "%d %u\n",
		data->buf[DIGITAL_HALL_EVENT_SENSOR].direction, data->buf[DIGITAL_HALL_EVENT_SENSOR].interrupt);
}

int ak09973d_save_digital_hall_cal_data(struct ssp_data *data)
{
	struct file *auto_cal_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char *write_buf = kzalloc(HALL_AUTO_CAL_FILE_BUF_LEN, GFP_KERNEL);


	/* auto_cal X */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(HALL_AUTO_CAL_X_PATH, O_CREAT | O_TRUNC | O_WRONLY, 0666);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[SSP] %s: open fail auto_cal_x_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, HALL_AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		data->hallcal.flg_update, data->hallcal.x[0],
		data->hallcal.x[1], data->hallcal.x[2],
		data->hallcal.x[3], data->hallcal.x[4],
		data->hallcal.x[5], data->hallcal.x[6],
		data->hallcal.x[7], data->hallcal.x[8],
		data->hallcal.x[9], data->hallcal.x[10],
		data->hallcal.x[11], data->hallcal.x[12],
		data->hallcal.x[13], data->hallcal.x[14],
		data->hallcal.x[15], data->hallcal.x[16],
		data->hallcal.x[17], data->hallcal.x[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		HALL_AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[SSP] %s: auto_cal_x fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);
	memset(write_buf, 0, sizeof(char) * HALL_AUTO_CAL_FILE_BUF_LEN);

	/* auto_cal Y */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(HALL_AUTO_CAL_Y_PATH, O_CREAT | O_TRUNC | O_WRONLY, 0666);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[SSP] %s: open fail auto_cal_y_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, HALL_AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		data->hallcal.y[0],
		data->hallcal.y[1], data->hallcal.y[2],
		data->hallcal.y[3], data->hallcal.y[4],
		data->hallcal.y[5], data->hallcal.y[6],
		data->hallcal.y[7], data->hallcal.y[8],
		data->hallcal.y[9], data->hallcal.y[10],
		data->hallcal.y[11], data->hallcal.y[12],
		data->hallcal.y[13], data->hallcal.y[14],
		data->hallcal.y[15], data->hallcal.y[16],
		data->hallcal.y[17], data->hallcal.y[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		HALL_AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[SSP] %s: auto_cal_y fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);
	memset(write_buf, 0, sizeof(char) * HALL_AUTO_CAL_FILE_BUF_LEN);

	/* auto_cal Z */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	auto_cal_filp = filp_open(HALL_AUTO_CAL_Z_PATH, O_CREAT | O_TRUNC | O_WRONLY, 0666);

	if (IS_ERR(auto_cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[SSP] %s: open fail auto_cal_z_filp:%d\n",
			__func__, ret);
		kfree(write_buf);
		return ret;
	}

	snprintf(write_buf, HALL_AUTO_CAL_FILE_BUF_LEN,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		data->hallcal.z[0],
		data->hallcal.z[1], data->hallcal.z[2],
		data->hallcal.z[3], data->hallcal.z[4],
		data->hallcal.z[5], data->hallcal.z[6],
		data->hallcal.z[7], data->hallcal.z[8],
		data->hallcal.z[9], data->hallcal.z[10],
		data->hallcal.z[11], data->hallcal.z[12],
		data->hallcal.z[13], data->hallcal.z[14],
		data->hallcal.z[15], data->hallcal.z[16],
		data->hallcal.z[17], data->hallcal.z[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		HALL_AUTO_CAL_FILE_BUF_LEN * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[SSP] %s: auto_cal_z fd write:%d\n",
			__func__, ret);

	filp_close(auto_cal_filp, current->files);
	set_fs(old_fs);

	kfree(write_buf);

	pr_info("[SSP] %s: saved", __func__);
	return ret;
}

static ssize_t digital_hall_check_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct digital_hall_caldata cal_data;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, DIGITAL_HALL_RAW_SENSOR, HALL_CMD_READ_CAL, (char *)&cal_data, sizeof(cal_data), 1000, HALL_CAL_DATA_LENGTH, __func__);

	if (cal_data.flg_update) {
		memcpy(&data->hallcal, &cal_data, sizeof(cal_data));
		ak09973d_save_digital_hall_cal_data(data);
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", cal_data.flg_update);
}

static ssize_t digital_hall_backup_restore_auto_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int new_value;
	int32_t auto_cal_buf[58] = { 0, };

	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		return size;

	pr_info("[SSP] %s: new_value %d\n", __func__, new_value);

	if (new_value) {
		ssp_send_read_factory_cmd(data, DIGITAL_HALL_RAW_SENSOR, HALL_CMD_READ_CAL, (char *)auto_cal_buf, sizeof(auto_cal_buf), 1000, HALL_CAL_DATA_LENGTH, __func__);

		pr_info("[SSP] %s: flg_update=%d\n", __func__, auto_cal_buf[0]);

		if (!auto_cal_buf[0])
			return size;

		memcpy(&data->hallcal, auto_cal_buf, sizeof(auto_cal_buf));

		pr_info("[SSP] %s: backup auto_cal\n", __func__);
		pr_info("[SSP] %s: %d/%d/%d/%d\n", __func__,
			data->hallcal.flg_update, data->hallcal.x[18],
			data->hallcal.y[18], data->hallcal.z[18]);
		ak09973d_save_digital_hall_cal_data(data);
	} else {
		if (data->hallcal.flg_update == 0) {
			pr_info("[SSP] %s: flg_update is zero\n", __func__);
			return size;
		}

		memcpy(auto_cal_buf, &data->hallcal, sizeof(data->hallcal));
		pr_info("[SSP] %s: sizeof(data->hallcal): %d\n", __func__, sizeof(data->hallcal));
		pr_info("[SSP] %s: restore auto_cal\n", __func__);
		pr_info("[SSP] %s: %d/%d/%d/%d\n", __func__,
			auto_cal_buf[0], auto_cal_buf[1],
			auto_cal_buf[20], auto_cal_buf[39]);

		ssp_send_write_factory_cmd(data, DIGITAL_HALL_RAW_SENSOR, HALL_CMD_WRITE_CAL, (char *)auto_cal_buf, HALL_CAL_DATA_LENGTH, __func__);
	}

	return size;
}

static ssize_t digital_hall_test_start_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 flag = 0;
	int ret;

	ret = kstrtou8(buf, 10, &flag);
	if (ret < 0) {
		pr_err("[SSP] %s : flag error %d", __func__, ret);
		return ret;
	}

	pr_info("[SSP] %s: wheel_test(%u)\n", __func__, flag);
	if (flag) { // test start
		data->wheel_test = true;
	} else {
		data->wheel_test = false;
	}
	return size;
}

static ssize_t digital_hall_angle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s: ang(%d)\n", __func__, data->buf[DIGITAL_HALL_EVENT_SENSOR].angle_evt);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->buf[DIGITAL_HALL_EVENT_SENSOR].angle_evt);
}

static DEVICE_ATTR(name, S_IRUGO, digital_hall_name_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, digital_hall_raw_show, NULL);
static DEVICE_ATTR(selftest1, S_IRUGO, digital_hall_selftest1_show, NULL);
static DEVICE_ATTR(selftest2, S_IRUGO, digital_hall_selftest2_show, NULL);
static DEVICE_ATTR(read_data, S_IRUGO, digital_hall_read_data_show, NULL);
static DEVICE_ATTR(check_auto_cal, S_IRUGO, digital_hall_check_auto_cal_show, NULL);
static DEVICE_ATTR(backup_restore_auto_cal, S_IWUSR | S_IWGRP, NULL, digital_hall_backup_restore_auto_cal_store);
static DEVICE_ATTR(test_start, S_IWUSR | S_IWGRP, NULL, digital_hall_test_start_set);
static DEVICE_ATTR(angle, 0444, digital_hall_angle_show, NULL);

static struct device_attribute *dhi_attrs[] = {
	&dev_attr_name,
	&dev_attr_raw_data,
	&dev_attr_selftest1,
	&dev_attr_selftest2,
	&dev_attr_read_data,
	&dev_attr_check_auto_cal,
	&dev_attr_backup_restore_auto_cal,
	&dev_attr_test_start,
	&dev_attr_angle,
	NULL,
};

void initialize_ak09973d_digital_hall_factorytest(struct ssp_data *data)
{
	struct device *ret;
	ret = sensors_register(data->dhallic_device, data, dhi_attrs,
		"digital_hall");

    data->dhallic_device = ret;
	pr_err("[SSP] %s [0x%x] \n", __func__, ret);
}

void remove_ak09973d_digital_hall_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->dhallic_device, dhi_attrs);

	pr_err("[SSP] %s\n", __func__);
}
