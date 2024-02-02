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
static int set_thermistor_threshold(struct ssp_data *data)
{
	int iRet = 0;

	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
		return -ENOMEM;
	}
	msg->cmd = MSG2SSP_AP_STATUS_SIOP;
	msg->length = 12;
	msg->options = AP2HUB_WRITE;
	msg->buffer = (char *) kzalloc(12, GFP_KERNEL);
	//msg->free_buffer = 1;

	memcpy(msg->buffer, data->threshold, 12);

	iRet = ssp_send_command(data, msg->cmd, msg->options, 0,
				msg->buffer, msg->length, NULL, NULL, msg->data);

	if (iRet != SUCCESS) {
		pr_err("[SSP] %s -fail to set_thermistor_threshold(%d)\n", __func__, iRet);
		iRet = ERROR;
	}
	kfree(msg->buffer);
	kfree(msg);
	return iRet;
}

static ssize_t threshold0_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->threshold[0]);
}

static ssize_t threshold0_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	int64_t threshold;

	iRet = kstrtoll(buf, 10, &threshold);
	if (iRet < 0)
		return iRet;

	data->threshold[0] = (s32)threshold;

	iRet = set_thermistor_threshold(data);

	pr_info("[SSP] %s (ret:%d) threshold0 = %d\n", __func__, iRet, data->threshold[0]);

	return size;
}

static ssize_t threshold1_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->threshold[1]);
}

static ssize_t threshold1_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	int64_t threshold;

	iRet = kstrtoll(buf, 10, &threshold);
	if (iRet < 0)
		return iRet;

	data->threshold[1] = (s32)threshold;

	iRet = set_thermistor_threshold(data);

	pr_info("[SSP] %s (ret:%d) threshold1 = %d\n", __func__, iRet, data->threshold[1]);

	return size;
}

static ssize_t threshold2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->threshold[2]);
}

static ssize_t threshold2_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	int64_t threshold;

	iRet = kstrtoll(buf, 10, &threshold);
	if (iRet < 0)
		return iRet;

	data->threshold[2] = (s32)threshold;

	iRet = set_thermistor_threshold(data);

	pr_info("[SSP] %s (ret:%d) threshold2 = %d\n", __func__, iRet, data->threshold[2]);

	return size;
}

static DEVICE_ATTR(threshold0, 0660, threshold0_show, threshold0_store);
static DEVICE_ATTR(threshold1, 0660, threshold1_show, threshold1_store);
static DEVICE_ATTR(threshold2, 0660, threshold2_show, threshold2_store);

static struct device_attribute *thermistor_attrs[] = {
	&dev_attr_threshold0,
	&dev_attr_threshold1,
	&dev_attr_threshold2,
	NULL,
};

void initialize_thermistor_factorytest(struct ssp_data *data)
{
	struct device *ret;
	ret = sensors_register(data->thermistor_device, data, thermistor_attrs,
		"thermistor_sensor");

	pr_err("[SSP] %s [0x%x] \n", __func__, ret);
}

void remove_thremistor_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->thermistor_device, thermistor_attrs);

	pr_err("[SSP] %s\n", __func__);
}
