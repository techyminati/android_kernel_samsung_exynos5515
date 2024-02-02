/*
 *  Copyright (C) 2015, Samsung Electronics Co. Ltd. All Rights Reserved.
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
#include <linux/slab.h>
#include <linux/delay.h>
#include "../ssp.h"
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#include "../ssp_data.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"AKM"
#define CHIP_ID		"AK09918C"
#define CALIBRATION_FILE_PATH	"/efs/FactoryApp/mag_cal_data"


#define GM_AKM_DATA_SPEC_MIN    -16666
#define GM_AKM_DATA_SPEC_MAX    16666

#define GM_DATA_SUM_SPEC        26666

#define GM_SELFTEST_X_SPEC_MIN  -200
#define GM_SELFTEST_X_SPEC_MAX  200
#define GM_SELFTEST_Y_SPEC_MIN  -200
#define GM_SELFTEST_Y_SPEC_MAX  200
#define GM_SELFTEST_Z_SPEC_MIN  -1000
#define GM_SELFTEST_Z_SPEC_MAX  -150

#define GM_SELFTEST_DATA_LENGTH	28
#define GM_STATUS_CHECK_LENGTH	1
#define GM_PDC_LENGTH			PDC_SIZE

#define BM_CAL_DATA_LENGTH		864 //4byte*72*3axis (float type * 1000 -> int)
#define BM_CAL_STATUS_LENGTH	2 //4byte*72*3axis (float type * 1000 -> int)
#define BM_CAL_OFFSET_LENGTH	100

#define BM_TYPE_CAL_STATUS		0x05
#define BM_TYPE_CAL_COMPLETED	0x07

#define BM_CAL_START_SLOT		2
#define BM_CAL_COMPLETED_SLOT	72

enum geomagnetic_cmd_data {
	GM_CMD_SELFTEST = 0,
	GM_CMD_STATUS_CHECK = 4,
	GM_CMD_SET_PDC = 7,
	GM_CMD_GET_PDC = 8,
};

static ssize_t magnetic_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t magnetic_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

int check_adc_data_spec(struct ssp_data *data, s16 *adc_data)
{
	int data_spec_max = 0;
	int data_spec_min = 0;

	data_spec_max = GM_AKM_DATA_SPEC_MAX;
	data_spec_min = GM_AKM_DATA_SPEC_MIN;

	if ((adc_data[0] == 0) &&
	    (adc_data[1] == 0) &&
	    (adc_data[2] == 0)) {
		return FAIL;
	} else if ((adc_data[0] >= data_spec_max)
		|| (adc_data[0] <= data_spec_min)
		|| (adc_data[1] >= data_spec_max)
		|| (adc_data[1] <= data_spec_min)
		|| (adc_data[2] >= data_spec_max)
		|| (adc_data[2] <= data_spec_min)) {
		return FAIL;
	} else if ((int)abs(adc_data[0])
		 + (int)abs(adc_data[1])
		 + (int)abs(adc_data[2]) >= GM_DATA_SUM_SPEC) {
		return FAIL;
	} else {
		return SUCCESS;
	}
}

static ssize_t magnetic_adc_data_read(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	bool bSuccess = false;
	u8 chTempbuf[4] = { 0 };
	s16 iSensorBuf[3] = {0, };
	int iRetries = 10;
	s32 dMsDelay = 20;
	memcpy(&chTempbuf[0], &dMsDelay, 4);

	data->buf[GEOMAGNETIC_CALIB_SENSOR].calib_data[0] = 0;
	data->buf[GEOMAGNETIC_CALIB_SENSOR].calib_data[1] = 0;
	data->buf[GEOMAGNETIC_CALIB_SENSOR].calib_data[2] = 0;

	if (!(atomic64_read(&data->aSensorEnable) & (1ULL << GEOMAGNETIC_CALIB_SENSOR))) {
		pr_info("[SSP]%s, enable sensor\n", __func__);
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_CALIB_SENSOR,
			chTempbuf, 4);
	}

	do {
		msleep(60);

		iSensorBuf[0] = data->buf[GEOMAGNETIC_CALIB_SENSOR].calib_data[0];
		iSensorBuf[1] = data->buf[GEOMAGNETIC_CALIB_SENSOR].calib_data[1];
		iSensorBuf[2] = data->buf[GEOMAGNETIC_CALIB_SENSOR].calib_data[2];

		if (check_adc_data_spec(data, iSensorBuf) == SUCCESS)
			break;
	} while (--iRetries);

	if (iRetries > 0)
		bSuccess = true;

	if (!(atomic64_read(&data->aSensorEnable) & (1ULL << GEOMAGNETIC_CALIB_SENSOR))) {
		pr_info("[SSP]%s, disable sensor\n", __func__);
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_CALIB_SENSOR,
			chTempbuf, 4);
	}


	pr_info("[SSP] %s - x = %d, y = %d, z = %d\n", __func__,
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);

	return sprintf(buf, "%s,%d,%d,%d\n", (bSuccess ? "OK" : "NG"),
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);
}

static ssize_t magnetic_check_dac(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	bool bSuccess = false;
	//char *buffer = NULL;
	int ret;
	//s32 imp[5] = {0,};
	char *tmpbuf = NULL;
	int return_length = 0;

	if (!data->geomag_cntl_regdata) {
		bSuccess = true;
	} else {
		pr_info("[SSP] %s - check cntl register before selftest",
			__func__);
		//ret = ssp_send_command(data, CMD_GETVALUE, SENSOR_TYPE_GEOMAGNETIC_FIELD,
		//                       SENSOR_FACTORY, 1000, NULL, 0, &buffer, &buffer_length);
		//ret = ssp_send_read_factory_cmd(data, GEOMAGNETIC_CALIB_SENSOR, 0, 1000,
		//	(char *)imp, sizeof(imp), &tmpbuf, &return_length, 4/*BIA_CMD_SIMP*/);

		if (ret != SUCCESS) {
			pr_err("[SSP]: %s - transfer fail %d\n", __func__, ret);
			return ret;
		}

		if (tmpbuf == NULL) {
			ssp_errf("buffer is null");
			return ret;
		}
		//memcpy(imp, tmpbuf, return_length);
		//ssp_errf(" (%d) %x %x %x %x %x\n", return_length, imp[0], imp[1], imp[2], imp[3], imp[4]);

		if (return_length < 22) {
			ssp_errf("buffer length error %d", return_length);
			if (tmpbuf != NULL)
				kfree(tmpbuf);
			return ret;
		}
		data->geomag_cntl_regdata = tmpbuf[21];
		bSuccess = !data->geomag_cntl_regdata;
	}

	pr_info("[SSP] %s - CTRL : 0x%x\n", __func__, data->geomag_cntl_regdata);

	data->geomag_cntl_regdata = 1;      /* reset the value */

	ret = sprintf(buf, "%s,%d,%d,%d\n", (bSuccess ? "OK" : "NG"), (bSuccess ? 1 : 0), 0, 0);

	if (tmpbuf != NULL)
		kfree(tmpbuf);

	return ret;
}

static ssize_t magnetic_raw_data_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - %d,%d,%d\n", __func__,
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[0],
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[1],
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[2]);

	if (data->bGeomagneticRawEnabled == false) {
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[0] = -1;
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[1] = -1;
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[2] = -1;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[0],
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[1],
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[2]);
}

static ssize_t magnetic_raw_data_store(struct device *dev,
				       struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	char chTempbuf[8] = { 0, };
	int iRet;
	int64_t dEnable;
	s32 dMsDelay = 20;
	int iRetries = 50;
	s16 iSensorBuf[3] = {0, };

	memcpy(&chTempbuf[0], &dMsDelay, 4);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	mutex_lock(&data->sysfs_op_mtx);
	if (dEnable) {
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[0] = 0;
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[1] = 0;
		data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[2] = 0;

		//set_delay_legacy_sensor(data, GEOMAGNETIC_CALIB_SENSOR, 20, 0);
		//enable_legacy_sensor(data, GEOMAGNETIC_CALIB_SENSOR);
		pr_info("[SSP]%s, enable sensor\n", __func__);
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_POWER_SENSOR, chTempbuf, 4);

		do {
			msleep(20);

			iSensorBuf[0] = data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[0];
			iSensorBuf[1] = data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[1];
			iSensorBuf[2] = data->buf[GEOMAGNETIC_POWER_SENSOR].power_data[2];

			if (check_adc_data_spec(data, iSensorBuf) == SUCCESS)
				break;
		} while (--iRetries);

		if (iRetries > 0) {
			pr_info("[SSP] %s - success, %d\n", __func__, iRetries);
			data->bGeomagneticRawEnabled = true;
		} else {
			pr_err("[SSP] %s - wait timeout, %d\n", __func__,
			       iRetries);
			data->bGeomagneticRawEnabled = false;
		}
	} else {
		//disable_legacy_sensor(data, GEOMAGNETIC_POWER_SENSOR);
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_POWER_SENSOR, chTempbuf, 4);
		data->bGeomagneticRawEnabled = false;
	}
	mutex_unlock(&data->sysfs_op_mtx);

	return size;
}

static ssize_t magnetic_get_status(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	bool bSuccess;

	ssp_send_read_cmd(data, GEOMAGNETIC_FACTORY, GM_CMD_STATUS_CHECK, (char *)&bSuccess, sizeof(bSuccess), 1000, GM_STATUS_CHECK_LENGTH, __func__);

	return sprintf(buf, "%s,%u\n", (bSuccess ? "OK" : "NG"), bSuccess);
}

static ssize_t magnetic_logging_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	char *buffer = NULL;
	int buffer_length = 0;
	int ret = 0;
	int logging_data[8] = {0, };

	s32 imp[5] = {0,};
	char *tmpbuf = NULL;
	int return_length = 0;

	//ret = ssp_send_command(data, CMD_GETVALUE, SENSOR_TYPE_GEOMAGNETIC_FIELD, SENSOR_FACTORY, 1000, NULL, 0, &buffer, &buffer_length);
	//read_data_mag(data, 4/*BIA_CMD_SIMP*/, buffer, &buffer_length, 1000, 20/*BIA_IMP_R_LENGTH*/, __func__);
	//ret = ssp_send_command(data, GEOMAGNETIC_FACTORY, AP2HUB_READ, 1000,
	//			(char *)imp, sizeof(imp), &tmpbuf, &return_length, 4/*BIA_CMD_SIMP*/);

	memcpy(imp, tmpbuf, return_length);
	ssp_errf("(%d) %x %x %x %x %x\n", return_length, imp[0], imp[1], imp[2], imp[3], imp[4]);

	if (ret != SUCCESS) {
		ssp_errf("ssp_send_command Fail %d", ret);
		ret = snprintf(buf, PAGE_SIZE, "-1,0,0,0,0,0,0,0,0,0,0\n");

		if (buffer != NULL)
			kfree(buffer);

		return ret;
	}

	if (buffer == NULL) {
		ssp_errf("buffer is null");
		return -EINVAL;
	}

	if (buffer_length != 14) {
		ssp_errf("buffer length error %d", buffer_length);
		ret = snprintf(buf, PAGE_SIZE, "-1,0,0,0,0,0,0,0,0,0,0\n");
		if (buffer != NULL)
			kfree(buffer);
		return -EINVAL;
	}

	logging_data[0] = buffer[0];    /* ST1 Reg */
	logging_data[1] = (short)((buffer[3] << 8) + buffer[2]);
	logging_data[2] = (short)((buffer[5] << 8) + buffer[4]);
	logging_data[3] = (short)((buffer[7] << 8) + buffer[6]);
	logging_data[4] = buffer[1];    /* ST2 Reg */
	logging_data[5] = (short)((buffer[9] << 8) + buffer[8]);
	logging_data[6] = (short)((buffer[11] << 8) + buffer[10]);
	logging_data[7] = (short)((buffer[13] << 8) + buffer[12]);

	ret = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			logging_data[0], logging_data[1],
			logging_data[2], logging_data[3],
			logging_data[4], logging_data[5],
			logging_data[6], logging_data[7],
			data->uFuseRomData[0], data->uFuseRomData[1],
			data->uFuseRomData[2]);

	kfree(buffer);

	return ret;
}

int set_pdc_matrix(struct ssp_data *data)
{
	int ret = 0;
	u8 sbuf[PDC_SIZE] = {0, };

	memcpy(sbuf, data->pdc_matrix, PDC_SIZE);
	ret = ssp_send_write_cmd(data, GEOMAGNETIC_FACTORY, GM_CMD_SET_PDC, (char *)sbuf, GM_PDC_LENGTH, __func__);

	pr_info("[SSP] %s: finished\n", __func__);

	return ret;
}

static ssize_t magnetic_matrix_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 val[PDC_SIZE] = {0, };

	ssp_send_read_cmd(data, GEOMAGNETIC_FACTORY, GM_CMD_GET_PDC, (char *)val, sizeof(val), 1000, GM_PDC_LENGTH, __func__);
	memcpy(data->pdc_matrix, val, PDC_SIZE);

	return sprintf(buf,
		       "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
		       data->pdc_matrix[0], data->pdc_matrix[1], data->pdc_matrix[2],
		       data->pdc_matrix[3], data->pdc_matrix[4],
		       data->pdc_matrix[5], data->pdc_matrix[6], data->pdc_matrix[7],
		       data->pdc_matrix[8], data->pdc_matrix[9],
		       data->pdc_matrix[10], data->pdc_matrix[11], data->pdc_matrix[12],
		       data->pdc_matrix[13], data->pdc_matrix[14],
		       data->pdc_matrix[15], data->pdc_matrix[16], data->pdc_matrix[17],
		       data->pdc_matrix[18], data->pdc_matrix[19],
		       data->pdc_matrix[20], data->pdc_matrix[21], data->pdc_matrix[22],
		       data->pdc_matrix[23], data->pdc_matrix[24],
		       data->pdc_matrix[25], data->pdc_matrix[26]);
}

static ssize_t magnetic_matrix_store(struct device *dev,
				     struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 val[PDC_SIZE] = {0, };
	int ret;
	int i;
	char *token;
	char *str;

	str = (char *)buf;

	for (i = 0; i < PDC_SIZE; i++) {
		token = strsep(&str, " \n");
		if (token == NULL) {
			pr_err("[SSP] %s : too few arguments (%d needed)", __func__, PDC_SIZE);
			return -EINVAL;
		}

		ret = kstrtou8(token, 10, &val[i]);
		if (ret < 0) {
			pr_err("[SSP] %s : kstros16 error %d", __func__, ret);
			return ret;
		}
	}

	for (i = 0; i < PDC_SIZE; i++)
		data->pdc_matrix[i] = val[i];

	pr_info("[SSP] %s : %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
		__func__, data->pdc_matrix[0], data->pdc_matrix[1], data->pdc_matrix[2],
		data->pdc_matrix[3], data->pdc_matrix[4],
		data->pdc_matrix[5], data->pdc_matrix[6], data->pdc_matrix[7],
		data->pdc_matrix[8], data->pdc_matrix[9],
		data->pdc_matrix[10], data->pdc_matrix[11], data->pdc_matrix[12],
		data->pdc_matrix[13], data->pdc_matrix[14],
		data->pdc_matrix[15], data->pdc_matrix[16], data->pdc_matrix[17],
		data->pdc_matrix[18], data->pdc_matrix[19],
		data->pdc_matrix[20], data->pdc_matrix[21], data->pdc_matrix[22],
		data->pdc_matrix[23], data->pdc_matrix[24],
		data->pdc_matrix[25], data->pdc_matrix[26]);

	set_pdc_matrix(data);

	return size;
}

static ssize_t magnetic_get_selftest(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	s8 result[4] = {-1, -1, -1, -1};
	char buf_selftest[GM_SELFTEST_DATA_LENGTH] = {0,};
	s16 iSF_X = 0, iSF_Y = 0, iSF_Z = 0;
	s16 iADC_X = 0, iADC_Y = 0, iADC_Z = 0;
	int iRet = 0;
	int spec_out_retries = 0;
	u8 chTempbuf[4] = { 0 };
	s32 dMsDelay = 20;
	s16 iSensorBuf[3] = {0, };
	int ret = -1;
	unsigned int prevRstCnt = data->uResetCnt;

	memcpy(&chTempbuf[0], &dMsDelay, 4);

	pr_info("[SSP] %s in\n", __func__);

	/* STATUS AK09916C doesn't need FuseRomdata more*/
	result[0] = 0;

Retry_selftest:
	ret = ssp_send_read_factory_cmd(data, GEOMAGNETIC_UNCALIB_SENSOR, GM_CMD_SELFTEST, (char *)buf_selftest, sizeof(buf_selftest), 1000, GM_SELFTEST_DATA_LENGTH, __func__);
	if (ret != SUCCESS && prevRstCnt != data->uResetCnt) {
		pr_info("[SSP] %s: stop retry\n", __func__);
		goto out;
	}

	iSF_X = (s16)((buf_selftest[13] << 8) + buf_selftest[14]);
	iSF_Y = (s16)((buf_selftest[15] << 8) + buf_selftest[16]);
	iSF_Z = (s16)((buf_selftest[17] << 8) + buf_selftest[18]);

	/* DAC (store Cntl Register value to check power down) */
	result[2] = buf_selftest[21];

	pr_info("[SSP] %s: self test x = %d, y = %d, z = %d\n",
		__func__, iSF_X, iSF_Y, iSF_Z);

	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
	    && (iSF_X <= GM_SELFTEST_X_SPEC_MAX)) {
		pr_info("[SSP] x passed self test, expect -200<=x<=200\n");
	} else {
		pr_info("[SSP] x failed self test, expect -200<=x<=200\n");
	}
	if ((iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
	    && (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX)) {
		pr_info("[SSP] y passed self test, expect -200<=y<=200\n");
	} else {
		pr_info("[SSP] y failed self test, expect -200<=y<=200\n");
	}
	if ((iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
	    && (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX)) {
		pr_info("[SSP] z passed self test, expect -1000<=z<=-150\n");
	} else {
		pr_info("[SSP] z failed self test, expect -1000<=z<=-150\n");
	}

	/* SELFTEST */
	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
	    && (iSF_X <= GM_SELFTEST_X_SPEC_MAX)
	    && (iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
	    && (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX)
	    && (iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
	    && (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX)) {
		result[1] = 0;
	} else {
		result[1] = -1;
	}

	iSensorBuf[0] = (s16)((buf_selftest[5] << 8) + buf_selftest[6]);
	iSensorBuf[1] = (s16)((buf_selftest[7] << 8) + buf_selftest[8]);
	iSensorBuf[2] = (s16)((buf_selftest[9] << 8) + buf_selftest[10]);

	if (check_adc_data_spec(data, iSensorBuf) == SUCCESS) {
		result[3] = 0;
	} else {
		result[3] = -1;
	}
	iADC_X = iSensorBuf[0];
	iADC_Y = iSensorBuf[1];
	iADC_Z = iSensorBuf[2];

	if ((result[1] == -1 || result[3] == -1) && (spec_out_retries < 5)) {
		spec_out_retries++;
		pr_err("[SSP] %s, selftest spec out. Retry = %d", __func__, spec_out_retries);
		goto Retry_selftest;
	}

out:
	pr_info("[SSP] %s -adc, x = %d, y = %d, z = %d, retry = %d\n",
		__func__, iADC_X, iADC_Y, iADC_Z, spec_out_retries);

	pr_info("[SSP] %s out. Result = %d %d %d %d\n",
		__func__, result[0], result[1], result[2], result[3]);

	iRet = sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		result[0], result[1], iSF_X, iSF_Y, iSF_Z,
		result[2], result[3], iADC_X, iADC_Y, iADC_Z);

	return iRet;
}

int ak09918c_mag_open_calibration(struct ssp_data *data)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;
	struct magnetic_calibration_data buffer;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			     O_RDONLY | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		memset(&data->magcal, 0, sizeof(data->magcal));

		pr_err("[SSP]: %s - Can't open calibration file %d\n", __func__, ret);
		return ret;
	}

	ret = vfs_read(cal_filp, (char *)&buffer, sizeof(buffer), &cal_filp->f_pos);
	if (ret != sizeof(buffer)) {
		ret = -EIO;
	}

	memcpy(&data->magcal, &buffer, sizeof(data->magcal));

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	ssp_infof("cal_status(%d)\n", data->magcal.cal_status);
	return ret;
}

int ak09918c_set_mag_cal(struct ssp_data *data)
{
	int ret = -1;
	int idx = 0;
	int send_len = 0;
	char buffer[BM_CAL_DATA_LENGTH];
	char s_buf[101];

	if (data->magcal.cal_status != BM_CAL_COMPLETED_SLOT) {
		ssp_errf("mag calibration is not completed(%d)", data->magcal.cal_status);
		return ret;
	}

	memcpy(buffer, data->magcal.cal_data, sizeof(buffer));
	for (idx = 0; idx < BM_CAL_DATA_LENGTH; ) {
		if (idx + BM_CAL_OFFSET_LENGTH <= BM_CAL_DATA_LENGTH) {
			send_len = BM_CAL_OFFSET_LENGTH;
		} else {
			send_len = BM_CAL_DATA_LENGTH - idx;
		}
		s_buf[0] = send_len;
		memcpy(&s_buf[1], &buffer[idx], send_len);
		ret = ssp_send_write_cmd(data, BEZELMAG_SET_CAL_DATA, 0, (char *)s_buf, send_len + 1, __func__);

		if (ret != SUCCESS) {
			ssp_errf("ssp_send_command Fail %d", ret);
			break;
		}
		idx += send_len;
		ssp_infof("%d: 0x%x 0x%x 0x%x 0x%x ... (%d)\n", __func__, idx, s_buf[1], s_buf[2], s_buf[3], s_buf[4], send_len);
	}
	return ret;
}

int ak09918c_save_mag_cal_data(struct ssp_data *data)
{
	int ret = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			     O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW |
			     O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return -EIO;
	}
	pr_err("[SSP]: %s - Open calibration file!!\n", __func__);
	ret = vfs_write(cal_filp, (char *)&data->magcal, sizeof(data->magcal), &cal_filp->f_pos);
	if (ret != sizeof(data->magcal)) {
		pr_err("[SSP]: %s - Can't write mag cal to file\n", __func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static void save_mag_cal_duration(struct ssp_data *data)
{
	if (!data->mag_status.start_time) {
		pr_err("[SSP]: %s - start time is NULL", __func__);
		return;
	}
	data->mag_status.end_time = get_current_timestamp();
	data->magcal.duration = (data->mag_status.end_time - data->mag_status.start_time) / 1000000ULL;
	pr_info("[SSP] %s: s:%llu e:%llu d:%llu\n", __func__, data->mag_status.start_time, data->mag_status.end_time, data->magcal.duration);
	data->mag_status.start_time = 0;
	data->mag_status.end_time = 0;
}

static ssize_t set_magnetic_calibration(struct device *dev,
				     struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 flag;
	int ret;
	int iRetries = 10;

	ret = kstrtou8(buf, 10, &flag);
	if (ret < 0) {
		pr_err("[SSP] %s : flag error %d", __func__, ret);
		return ret;
	}

	data->mag_status.calibration = flag;
	ssp_send_write_cmd(data, BEZELMAG_CALIBRATION, 0, (char *)&flag, 1, __func__);
	pr_info("[SSP] %s: flag(%u)\n", __func__, flag);
	if (flag) { // cal start
		memset(&data->magcal, 0x00, sizeof(data->magcal));
		ak09918c_save_mag_cal_data(data);
	} else {
		save_mag_cal_duration(data);
		do {
			msleep(10);
			if (data->mag_status.lookup_flag) {
				data->mag_status.lookup_flag = 0;
				break;
			}
		} while (--iRetries);
		pr_info("[SSP] %s: iRetries : %d\n", __func__, iRetries);
		ak09918c_save_mag_cal_data(data);
	}

	return size;
}

static ssize_t magnetic_cal_ststus_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s: cal_status(%d)\n", __func__, data->magcal.cal_status);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->magcal.cal_status);
}

static ssize_t magnetic_cal_data_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	int i;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ak09918c_mag_open_calibration(data);
	pr_info("[SSP] %s: %d\n", __func__, data->magcal.cal_status);

	iRet = sprintf(buf, "%d", data->magcal.cal_status);
	iRet += sprintf(buf + iRet, " %llu", data->magcal.duration);
	for (i = 0; i < MAGNETIC_CAL_DATA_SIZE; i++) {
		iRet += sprintf(buf + iRet, " %d", data->magcal.cal_data[i]);
	}
	iRet += sprintf(buf + iRet, "\n");

	return iRet;
}

static ssize_t magnetic_send_cal_data_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	data->magcal.cal_status = BM_CAL_COMPLETED_SLOT;
	iRet = ak09918c_set_mag_cal(data);

	return  snprintf(buf, PAGE_SIZE, "%d\n", iRet);
}

int parse_mag_cal_data(struct ssp_data *data, u8 *data_buf, u8 type)
{
	if (type == BM_TYPE_CAL_STATUS) {
		memcpy(&data->magcal.cal_status, data_buf, BM_CAL_STATUS_LENGTH);
		if (data->mag_status.calibration) {
			if ((data->mag_status.start_time == 0) && (data->magcal.cal_status >= BM_CAL_START_SLOT)) {
				data->mag_status.start_time = get_current_timestamp();
				pr_info("[SSP] %s: start[%d, %llu]\n", __func__, data->magcal.cal_status, data->mag_status.start_time);
			}
			if (data->magcal.cal_status == BM_CAL_COMPLETED_SLOT) {
				save_mag_cal_duration(data);
			}
			pr_info("[SSP] %s: cal_status[%d]\n", __func__, data->magcal.cal_status);
		} else {
			pr_info("[SSP] %s: Not calibration status(%d)\n", __func__, data->magcal.cal_status);
		}
	} else if (type == BM_TYPE_CAL_COMPLETED) {
		memcpy(data->magcal.cal_data, data_buf, BM_CAL_DATA_LENGTH);
		data->mag_status.lookup_flag = 1;
		pr_info("[SSP] %s: lookup data[%d %d %d...]\n", __func__,
			data->magcal.cal_data[0], data->magcal.cal_data[1], data->magcal.cal_data[2]);
	} else {
		return 0;
	}

	return type;
}


static DEVICE_ATTR(name, S_IRUGO, magnetic_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, magnetic_vendor_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP, magnetic_raw_data_show, magnetic_raw_data_store);
static DEVICE_ATTR(adc, S_IRUGO, magnetic_adc_data_read, NULL);
static DEVICE_ATTR(dac, S_IRUGO, magnetic_check_dac, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, magnetic_get_selftest, NULL);
static DEVICE_ATTR(status, S_IRUGO,  magnetic_get_status, NULL);
static DEVICE_ATTR(logging_data, S_IRUGO, magnetic_logging_show, NULL);
static DEVICE_ATTR(matrix, S_IRUGO | S_IWUSR | S_IWGRP, magnetic_matrix_show, magnetic_matrix_store);
static DEVICE_ATTR(calibration, S_IWUSR | S_IWGRP, NULL, set_magnetic_calibration);
static DEVICE_ATTR(cal_status, S_IRUGO, magnetic_cal_ststus_show, NULL);
static DEVICE_ATTR(cal_data, S_IRUGO, magnetic_cal_data_show, NULL);
static DEVICE_ATTR(send_cal_data, S_IRUGO, magnetic_send_cal_data_show, NULL);


static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_adc,
	&dev_attr_dac,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_status,
	&dev_attr_logging_data,
	&dev_attr_matrix,
	&dev_attr_calibration,
	&dev_attr_cal_status,
	&dev_attr_cal_data,
	&dev_attr_send_cal_data,
	NULL,
};

static void initialize_mag_variable(struct ssp_data *data)
{
	data->mag_status.calibration = 0;
	data->mag_status.lookup_flag = 0;
	data->mag_status.start_time = 0;
	data->mag_status.end_time = 0;
}

void initialize_ak09918c_magnetic_factorytest(struct ssp_data *data)
{
	struct device *ret; 	//int ret;

	initialize_mag_variable(data);
	ret = sensors_register(data->mag_device, data, mag_attrs, "magnetic_sensor");
}

void remove_ak09918c_magnetic_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mag_device, mag_attrs);
}

