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
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/ctype.h>

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"PARTRON"
#define CHIP_ID		"MLX90632"
//#define MODULE_NAME	"AFE4920"

#define TEMPERATURE_DATA_FILE_PATH "/efs/FactoryApp/temperature_data"  // TEMPERATURE Parasitic Component Calibration

#define TEMPERATURE_NAME_LENGTH             64
#define TEMPERATURE_VENDOR_LENGTH           64
#define TEMPERATURE_MODE_LENGTH             1
#define TEMPERATURE_REG_READ_LENGTH         2
#define TEMPERATURE_REG_WRITE_LENGTH        5
#define TEMPERATURE_RAW_DATA_LENGTH         64
#define TEMPERATURE_PRODUCT_ID_LENGTH       64
#define TEMPERATURE_TRIM_LENGTH             64
#define TEMPERATURE_JIG_LENGTH              64
#define TEMPERATURE_REFERENCE_LENGTH        64
#define TEMPERATURE_RATE_LENGTH             6
#define TEMPERATURE_COUNT_LENGTH            64
#define TEMPERATURE_ON_TIME_COUNT_LENGTH    64
#define TEMPERATURE_AMBIENT_LENGTH          64
#define TEMPERATURE_OBJECT_LENGTH           64
#define TEMPERATURE_ACCURACY_LENGTH         64

#define TEMPERATURE_CMD_NAME              (0)
#define TEMPERATURE_CMD_VENDOR            (1)
#define TEMPERATURE_CMD_MODE              (2)
#define TEMPERATURE_CMD_REG               (3)
#define TEMPERATURE_CMD_RAW_DATA          (4)
#define TEMPERATURE_CMD_PRODUCT_ID        (5)
#define TEMPERATURE_CMD_TRIM              (6)
#define TEMPERATURE_CMD_JIG               (7)
#define TEMPERATURE_CMD_REFERENCE         (8)
#define TEMPERATURE_CMD_RATE              (9)
#define TEMPERATURE_CMD_COUNT             (10)
#define TEMPERATURE_CMD_ON_TIME_COUNT     (11)
#define TEMPERATURE_CMD_AMBIENT           (12)
#define TEMPERATURE_CMD_OBJECT            (13)
#define TEMPERATURE_CMD_ACCURACY          (14)

#define TEMPERATURE_READ_CMD	1
#define TEMPERATURE_WRITE_CMD	2

#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))
#endif


struct temperature_reg{
	uint8_t mode;
	uint16_t address;
	uint16_t value;
} __attribute__ ((packed));

static ssize_t temperature_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char name[TEMPERATURE_NAME_LENGTH] = {0,};

	ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR, (TEMPERATURE_CMD_NAME << 8) | TEMPERATURE_READ_CMD, (char *)name, sizeof(name), 1000, TEMPERATURE_NAME_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", name);
}

static ssize_t temperature_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char vendor[TEMPERATURE_VENDOR_LENGTH] = {0,};

	ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR, (TEMPERATURE_CMD_VENDOR << 8) | TEMPERATURE_READ_CMD, (char *)vendor, sizeof(vendor), 1000, TEMPERATURE_VENDOR_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", vendor);
}

static ssize_t temperature_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	uint8_t mode = 0;

	ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR, (TEMPERATURE_CMD_MODE << 8) | TEMPERATURE_READ_CMD, (char *)&mode, sizeof(mode), 1000, TEMPERATURE_MODE_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", (int)mode);
}

static ssize_t temperature_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char buffer[TEMPERATURE_MODE_LENGTH] = {0,};
	int64_t mode;
	int iRet = 0;

	iRet = kstrtoll(buf, 10, &mode);
	if (iRet < 0)
		return iRet;

	buffer[0] = (char)mode;

	ssp_send_write_factory_cmd(data, TEMPERATURE_SENSOR, (TEMPERATURE_CMD_MODE << 8) | TEMPERATURE_WRITE_CMD, buffer, TEMPERATURE_MODE_LENGTH, __func__);

	return size;
}

static ssize_t temperature_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t val = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR, (TEMPERATURE_CMD_REG << 8) | TEMPERATURE_READ_CMD, (char *)&val, sizeof(val), 1000, TEMPERATURE_REG_READ_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u\n", val);
}

static ssize_t temperature_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct temperature_reg reg;
	uint8_t tmp[25] = {0,};
	char *strptr, *tkn;
	ssize_t len;

	struct ssp_data *data = dev_get_drvdata(dev);

	len = min(size, sizeof(tmp) - 1);

	memcpy(tmp, buf, len);
	pr_info("[SSP] %s - tmp[%d] %s\n", __func__, (int)size, tmp);

	tmp[len] = '\0';
	strptr = tmp;

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - mode NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou8(tkn, 0, &reg.mode)) {
		pr_err("[SSP] %s - mode INVAL!\n", __func__);
		return -EINVAL;
	}

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - address NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou16(tkn, 0, &reg.address)) {
		pr_err("[SSP] %s - address INVAL!\n", __func__);
		return -EINVAL;
	}

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - value NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou16(tkn, 0, &reg.value)) {
		pr_err("[SSP] %s - value INVAL!\n", __func__);
		return -EINVAL;
	}

	pr_info("[SSP] %s - mode: %d, addr: %d, value: %d", __func__, reg.mode, reg.address, reg.value);

	ssp_send_write_factory_cmd(data, TEMPERATURE_SENSOR, (TEMPERATURE_CMD_REG << 8) | TEMPERATURE_WRITE_CMD, (char *)&reg, sizeof(reg), __func__);

	return size;
}

static ssize_t temperature_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	ssize_t count;
	uint8_t temperature_raw[TEMPERATURE_RAW_DATA_LENGTH] = {0,};

	ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR, (TEMPERATURE_CMD_RAW_DATA << 8) | TEMPERATURE_READ_CMD, (char *)&temperature_raw, sizeof(temperature_raw), 1000, TEMPERATURE_RAW_DATA_LENGTH, __func__);

	count = snprintf(buf, PAGE_SIZE, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d " \
									 "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		temperature_raw[0], temperature_raw[1], temperature_raw[2], temperature_raw[3], temperature_raw[4], temperature_raw[5], temperature_raw[6], temperature_raw[7],
		temperature_raw[8], temperature_raw[9], temperature_raw[10], temperature_raw[11], temperature_raw[12], temperature_raw[13], temperature_raw[14], temperature_raw[15],
		temperature_raw[16], temperature_raw[17], temperature_raw[18], temperature_raw[19], temperature_raw[20], temperature_raw[21], temperature_raw[22], temperature_raw[23],
		temperature_raw[24], temperature_raw[25], temperature_raw[26], temperature_raw[27], temperature_raw[28], temperature_raw[29], temperature_raw[30], temperature_raw[31],
		temperature_raw[32], temperature_raw[33], temperature_raw[34], temperature_raw[35], temperature_raw[36], temperature_raw[37], temperature_raw[38], temperature_raw[39],
		temperature_raw[40], temperature_raw[41], temperature_raw[42], temperature_raw[43], temperature_raw[44], temperature_raw[45], temperature_raw[46], temperature_raw[47],
		temperature_raw[48], temperature_raw[49], temperature_raw[50], temperature_raw[51], temperature_raw[52], temperature_raw[53], temperature_raw[54], temperature_raw[55],
		temperature_raw[56], temperature_raw[57], temperature_raw[58], temperature_raw[59], temperature_raw[60], temperature_raw[61], temperature_raw[62], temperature_raw[63]);

	return count;
}

static ssize_t temperature_product_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char product_id[TEMPERATURE_PRODUCT_ID_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		product_id[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_PRODUCT_ID << 8) | TEMPERATURE_READ_CMD,
			(char *)&product_id, sizeof(product_id), 1000, TEMPERATURE_PRODUCT_ID_LENGTH, __func__);

		if (ret != SUCCESS)
			product_id[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", product_id);
}

static ssize_t temperature_trim_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char trim[TEMPERATURE_TRIM_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		trim[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_TRIM << 8) | TEMPERATURE_READ_CMD,
			(char *)&trim, sizeof(trim), 1000, TEMPERATURE_TRIM_LENGTH, __func__);

		if (ret != SUCCESS)
			trim[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", trim);
}

static ssize_t temperature_jig_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char jig[TEMPERATURE_JIG_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		jig[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_JIG << 8) | TEMPERATURE_READ_CMD,
			(char *)&jig, sizeof(jig), 1000, TEMPERATURE_JIG_LENGTH, __func__);

		if (ret != SUCCESS)
			jig[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", jig);
}

static ssize_t temperature_jig_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char buffer[4] = {0,};
	uint16_t jig;
	int iRet = 0;

	iRet = kstrtou16(buf, 0, &jig);
	if (iRet < 0)
		return iRet;
	memcpy(&buffer[0], &jig, sizeof(jig));

	ssp_send_write_factory_cmd(data, TEMPERATURE_SENSOR,
		(TEMPERATURE_CMD_JIG << 8) | TEMPERATURE_WRITE_CMD,
		buffer, sizeof(jig), __func__);

	return size;
}


static ssize_t temperature_reference_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char reference[TEMPERATURE_REFERENCE_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		reference[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_REFERENCE << 8) | TEMPERATURE_READ_CMD,
			(char *)&reference, sizeof(reference), 1000, TEMPERATURE_REFERENCE_LENGTH, __func__);

		if (ret != SUCCESS)
			reference[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", reference);
}

static ssize_t temperature_rate_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char rate[TEMPERATURE_RATE_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		rate[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_RATE << 8) | TEMPERATURE_READ_CMD,
			(char *)&rate, sizeof(rate), 1000, TEMPERATURE_RATE_LENGTH, __func__);

		if (ret != SUCCESS)
			rate[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", rate);
}

static ssize_t temperature_count_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char count[TEMPERATURE_COUNT_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		count[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_COUNT << 8) | TEMPERATURE_READ_CMD,
			(char *)&count, sizeof(count), 1000, TEMPERATURE_COUNT_LENGTH, __func__);

		if (ret != SUCCESS)
			count[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", count);
}

static ssize_t temperature_on_time_count_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char on_time_count[TEMPERATURE_ON_TIME_COUNT_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		on_time_count[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_ON_TIME_COUNT << 8) | TEMPERATURE_READ_CMD,
			(char *)&on_time_count, sizeof(on_time_count), 1000,
			TEMPERATURE_ON_TIME_COUNT_LENGTH, __func__);

		if (ret != SUCCESS)
			on_time_count[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", on_time_count);
}

static ssize_t temperature_ambient_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char ambient[TEMPERATURE_AMBIENT_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		ambient[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_AMBIENT << 8) | TEMPERATURE_READ_CMD,
			(char *)&ambient, sizeof(ambient), 1000, TEMPERATURE_AMBIENT_LENGTH, __func__);

		if (ret != SUCCESS)
			ambient[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", ambient);
}

static ssize_t temperature_object_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char object[TEMPERATURE_OBJECT_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		object[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_OBJECT << 8) | TEMPERATURE_READ_CMD,
			(char *)&object, sizeof(object), 1000, TEMPERATURE_OBJECT_LENGTH, __func__);

		if (ret != SUCCESS)
			object[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", object);
}

static ssize_t temperature_accuracy_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;
	char accuracy[TEMPERATURE_ACCURACY_LENGTH];

	if (!get_sensor_state(data, TEMPERATURE_SENSOR, __func__)) {
		accuracy[0] = '\0';
	} else {
		ret = ssp_send_read_factory_cmd(data, TEMPERATURE_SENSOR,
			(TEMPERATURE_CMD_ACCURACY << 8) | TEMPERATURE_READ_CMD,
			(char *)&accuracy, sizeof(accuracy), 1000, TEMPERATURE_ACCURACY_LENGTH, __func__);

		if (ret != SUCCESS)
			accuracy[0] = '\0';
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", accuracy);
}

static DEVICE_ATTR(name, S_IRUGO, temperature_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, temperature_vendor_show, NULL);
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR | S_IWGRP, temperature_mode_show, temperature_mode_store);
static DEVICE_ATTR(reg, S_IRUGO | S_IWUSR | S_IWGRP, temperature_reg_show, temperature_reg_store);
static DEVICE_ATTR(raw_data, S_IRUGO, temperature_raw_data_show, NULL);
static DEVICE_ATTR(product_id, S_IRUGO, temperature_product_id_show, NULL);
static DEVICE_ATTR(trim, S_IRUGO, temperature_trim_show, NULL);
static DEVICE_ATTR(jig, S_IRUGO | S_IWUSR | S_IWGRP, temperature_jig_show, temperature_jig_store);
static DEVICE_ATTR(reference, S_IRUGO, temperature_reference_show, NULL);
static DEVICE_ATTR(rate, S_IRUGO, temperature_rate_show, NULL);
static DEVICE_ATTR(count, S_IRUGO, temperature_count_show, NULL);
static DEVICE_ATTR(on_time_count, S_IRUGO, temperature_on_time_count_show, NULL);
static DEVICE_ATTR(ambient, S_IRUGO, temperature_ambient_show, NULL);
static DEVICE_ATTR(object, S_IRUGO, temperature_object_show, NULL);
static DEVICE_ATTR(accuracy, S_IRUGO, temperature_accuracy_show, NULL);

static struct device_attribute *temperature_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_mode,
	&dev_attr_reg,
	&dev_attr_raw_data,
	&dev_attr_product_id,
	&dev_attr_trim,
	&dev_attr_jig,
	&dev_attr_reference,
	&dev_attr_rate,
	&dev_attr_count,
	&dev_attr_on_time_count,
	&dev_attr_ambient,
	&dev_attr_object,
	&dev_attr_accuracy,
	NULL,
};

void initialize_mlx90632_temperature_factorytest(struct ssp_data *data)
{
	struct device *ret; 	//int ret;

	ret = sensors_register(data->temperature_device, data, temperature_attrs,
		"temperature_sensor");

	pr_err("[SSP_TEMPERATURE] %s [0x%x] \n", __func__, ret);
}

void remove_mlx90632_temperature_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->temperature_device, temperature_attrs);

	pr_err("[SSP_TEMPERATURE] %s\n", __func__);
}
