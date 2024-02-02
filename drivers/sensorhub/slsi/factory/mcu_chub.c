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
#include "../ssp_data.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define SMART_ALERT_MOTION  8
#define LPM_AUTO_ROTATION   7

ssize_t mcu_revision_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	unsigned int module_rev = 0;

	module_rev = get_module_rev(data);

	pr_info("[SSP]: %s - [%8u], [%8u]\n", __func__,
				data->uCurFirmRev, module_rev);

	return snprintf(buf, PAGE_SIZE, "SLSI%8u,SLSI%8u\n", data->uCurFirmRev,
			module_rev);
}

ssize_t mcu_model_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", data->model_name);
}

ssize_t mcu_update_kernel_bin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssp_dbg("[SSPBBD]: %s:%d: Ignored some code section.\n",
		__func__, __LINE__);
	return snprintf(buf, PAGE_SIZE, "NG\n");
}

ssize_t mcu_update_kernel_crashed_bin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssp_dbg("[SSPBBD]: %s:%d: Ignored some code section.\n",
		__func__, __LINE__);
	return snprintf(buf, PAGE_SIZE, "NG\n");
}

ssize_t mcu_reset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	bool isRun;
	isRun = is_sensorhub_working(data) && (data->is_reset_started == false);

	pr_info("[SSP]: %s, reset ssp mcu (%d)\n", __func__, isRun);
	if (isRun)
		reset_mcu(data, RESET_TYPE_KERNEL_SYSFS);

	return snprintf(buf, PAGE_SIZE, "OK\n");
}

ssize_t mcu_ready_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if ((is_sensorhub_working(data)) && (data->is_reset_started == false))
		data->bSspReady = true;
	else
		data->bSspReady = false;

	return snprintf(buf, PAGE_SIZE, "%d\n", data->bSspReady);
}

ssize_t mcu_sensor_state(struct device *dev,
    struct device_attribute *attr, char *buf) {
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%llu\n", data->uSensorState);
}

static char buffer[FACTORY_DATA_MAX];

ssize_t mcu_factorytest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	struct ssp_msg *msg;
	int buffer_length = 0;
	char *temp_buf = NULL;

	if (sysfs_streq(buf, "1")) {
		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		if (msg == NULL) {
			pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
				__func__);
			return -ENOMEM;
		}
		msg->cmd = MCU_FACTORY;
		msg->length = 5;
		msg->options = AP2HUB_READ;
		msg->buffer = kzalloc(5, GFP_KERNEL);
		msg->free_buffer = 0;

		memset(msg->buffer, 0, 5);

		iRet = ssp_send_command(data, msg->cmd, msg->options, 3000,
			msg->buffer, msg->length, &temp_buf, &buffer_length, msg->data);

		if (iRet != SUCCESS)
			pr_err("[SSP]: %s - ssp_send_comman error!!\n", __func__);
		else
			memcpy(buffer, temp_buf, 5);

		pr_info("%s : %d %d %d %d %d\n",  __func__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
		kfree(temp_buf);
		kfree(msg);
	} else {
		pr_err("[SSP]: %s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	ssp_dbg("[SSP]: MCU Factory Test Start! - %d\n", iRet);

	return size;
}

ssize_t mcu_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bMcuTestSuccessed = false;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->bSspShutdown == true) {
		ssp_dbg("[SSP]: %s - MCU Bin is crashed\n", __func__);
		return snprintf(buf, PAGE_SIZE, "NG,NG,NG\n");
	}

	pr_info("[SSP] MCU Factory Test Data : %u, %u, %u, %u, %u\n", buffer[0],
			buffer[1], buffer[2], buffer[3], buffer[4]);

	/* system clock, RTC, I2C primary, I2C secondary, externel pin */
	if ((buffer[0] == SUCCESS)
			&& (buffer[1] == SUCCESS)
			&& (buffer[2] == SUCCESS)
			&& (buffer[3] == SUCCESS)
			&& (buffer[4] == SUCCESS))
		bMcuTestSuccessed = true;

	ssp_dbg("[SSP]: MCU Factory Test Result - %s, %s, %s\n",
		data->model_name, (bMcuTestSuccessed ? "OK" : "NG"), "OK");

	return snprintf(buf, PAGE_SIZE, "%s,%s,%s\n", data->model_name,
		(bMcuTestSuccessed ? "OK" : "NG"), "OK");
}

ssize_t mcu_sleep_factorytest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
#if 0
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	struct ssp_msg *msg;
	int buffer_length = 0;
	char *temp_buf;

	if (sysfs_streq(buf, "1")) {
		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		if (msg == NULL) {
			pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
				__func__);
			return -ENOMEM;
		}
		temp_buf = kzalloc(MCU_SLEEP_FACTORY_DATA_LENGTH, GFP_KERNEL);
		msg->cmd = MCU_SLEEP_FACTORY;
		msg->length = MCU_SLEEP_FACTORY_DATA_LENGTH;
		msg->options = AP2HUB_READ;
		msg->buffer = temp_buf;
		msg->free_buffer = 0;

		iRet = ssp_send_command(data, msg->cmd, msg->options, 10000,
			msg->buffer, msg->length, &temp_buf, &buffer_length, msg->data);

		memcpy(buffer, temp_buf, MCU_SLEEP_FACTORY_DATA_LENGTH);
		kfree(msg);
	} else {
		pr_err("[SSP]: %s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
#endif
	ssp_dbg("[SSP]: MCU Sleep Factory Test Start!(NOT USED)\n");

	return size;
}

ssize_t mcu_sleep_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iDataIdx, iSensorData = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct sensor_value *fsb;
	u16 chLength = 0;
	int count = 0;

	fsb = kzalloc(sizeof(struct sensor_value)*SENSOR_MAX, GFP_KERNEL);

	memcpy(&chLength, buffer, 2);
	memset(fsb, 0, sizeof(struct sensor_value) * SENSOR_MAX);

	for (iDataIdx = 2; iDataIdx < chLength + 2;) {
		iSensorData = (int)buffer[iDataIdx++];

		if ((iSensorData < 0) ||
			(iSensorData >= (SENSOR_MAX - 1))) {
			pr_err("[SSP]: %s - Mcu data frame error %d\n",
				__func__, iSensorData);
			goto exit;
		}

		//data->get_sensor_data[iSensorData]((char *)buffer, &iDataIdx, &(fsb[iSensorData]));
		get_sensordata(data, (char *)buffer, &iDataIdx, iSensorData, &(fsb[iSensorData]));
	}

	fsb[PRESSURE_SENSOR].pressure[0] -= data->iPressureCal;

exit:
	ssp_dbg("[SSP]: %s Result\n"
		"[SSP]: accel %d,%d,%d\n"
		"[SSP]: gyro %d,%d,%d\n"
		"[SSP]: mag %d,%d,%d\n"
		"[SSP]: baro %d,%d\n", __func__,

		fsb[ACCELEROMETER_SENSOR].x, fsb[ACCELEROMETER_SENSOR].y,
		fsb[ACCELEROMETER_SENSOR].z, fsb[GYROSCOPE_SENSOR].x,
		fsb[GYROSCOPE_SENSOR].y, fsb[GYROSCOPE_SENSOR].z,
		fsb[GEOMAGNETIC_CALIB_SENSOR].calib_data[0], fsb[GEOMAGNETIC_CALIB_SENSOR].calib_data[1],
		fsb[GEOMAGNETIC_CALIB_SENSOR].calib_data[2], fsb[PRESSURE_SENSOR].pressure[0],
		fsb[PRESSURE_SENSOR].pressure[1]);

	count = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		fsb[ACCELEROMETER_SENSOR].x, fsb[ACCELEROMETER_SENSOR].y,
		fsb[ACCELEROMETER_SENSOR].z, fsb[GYROSCOPE_SENSOR].x,
		fsb[GYROSCOPE_SENSOR].y, fsb[GYROSCOPE_SENSOR].z,
		fsb[GEOMAGNETIC_CALIB_SENSOR].calib_data[0], fsb[GEOMAGNETIC_CALIB_SENSOR].calib_data[1],
		fsb[GEOMAGNETIC_CALIB_SENSOR].calib_data[2], fsb[PRESSURE_SENSOR].pressure[0],
		fsb[PRESSURE_SENSOR].pressure[1]);

	kfree(fsb);

	return count;
}

unsigned short ssp_get_discharing_adc(void)
{
#if 0
	u16 adc = -1;
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	int buffer_length = 0;

	msg->cmd = MSG2SSP_AP_BATT_DISCHG_ADC;
	msg->length = 2;
	msg->options = AP2HUB_READ;
	msg->buffer = (char *) &adc;
	msg->free_buffer = 0;
#if 0
	iRet = ssp_spi_sync(sensorhub_data, msg, 1000);
#else
	iRet = ssp_send_command(sensorhub_data, msg->cmd, msg->options, 1000,
		msg->buffer, msg->length, &(msg->buffer), &buffer_length, msg->data);
#endif
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}
	pr_info("[SSP]%s, %d\n", __func__, adc);

	kfree(msg);
	return adc;
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(ssp_get_discharing_adc);

