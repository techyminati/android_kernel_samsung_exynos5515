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
#include "ssp.h"
#include "ssp_dump.h"
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>

#define SSP_DEBUG_TIMER_SEC	(10 * HZ)

#define LIMIT_RESET_CNT		20
#define LIMIT_TIMEOUT_CNT	3

#define DUMP_FILE_PATH "/opt/usr/log/MCU_DUMP"
#define DEBUG_DUMP_FILE_PATH	"/opt/usr/log/SensorHubDump"
#define DEBUG_DUMP_DATA_COMPLETE 0xDD


/*************************************************************************/
/* SSP Debug timer function                                              */
/*************************************************************************/
int print_mcu_debug(char *pchRcvDataFrame, int *pDataIdx,
		int iRcvDataFrameLength, struct ssp_data *data)
{
	int iLength = pchRcvDataFrame[(*pDataIdx)++];
	int cur = *pDataIdx;

	if (iLength > iRcvDataFrameLength - *pDataIdx || iLength <= 0) {
		ssp_dbg("[SSP]: MSG From MCU - invalid debug length(%u/%d/%d)\n",
			iLength, iRcvDataFrameLength, cur);
		return iLength ? iLength : ERROR;
	}

	/*First 16 byte of debug message is libstate*/
	data->uLibraryState[0] = *(u64 *)(&pchRcvDataFrame[cur]);
	data->uLibraryState[1] = *(u64 *)(&pchRcvDataFrame[cur+8]);
	if (data->uLibraryState[1] == 0) {
		ssp_dbg("[SSP]: %s : iLength: %d, LibraryState: %llx\n", __func__, iLength, data->uLibraryState[0]);
	} else {
		ssp_dbg("[SSP]: %s : iLength: %d, LibraryState: %llx%016llx\n", __func__, iLength, data->uLibraryState[1], data->uLibraryState[0]);
	}
	ssp_dbg("[SSP]: MSG From MCU - %s\n", &pchRcvDataFrame[(*pDataIdx)+16]);

	*pDataIdx += iLength;
	return 0;
}

void reset_mcu(struct ssp_data *data, int reason)
{
	int ret = 0;
#if SSP_STATUS_MONITOR
	data->bRefreshing = true;
#endif

	nanohub_info("[SSP] %s reason(%u)", __func__, reason);
#ifdef CONFIG_SENSORS_SSP_AP_WAKEUP
	ssp_enable_irq(data, 0);
#endif
	data->is_reset_started = true;
	data->reset_type = reason;
	clean_pending_list(data);
	disable_timestamp_sync_timer(data);
	ret = sensorhub_reset(data);

	if (ret) {
		nanohub_err("[SSP] %s, Chub reset fail", __func__);
	}
}

void reset_mcu_recovery(struct ssp_data *data, int reason)
{
	data->uResetCnt++;
	data->is_reset_started = true;
	data->reset_type = reason;
	clean_pending_list(data);

	nanohub_info("[SSP] %s, Chub silent reset\n", __func__);

	queue_refresh_task(data, 0);
}

void sync_sensor_state(struct ssp_data *data)
{
	unsigned char uBuf[9] = {0,};
	unsigned int uSensorCnt;
	int iRet = 0;
#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR
	u64 uSensorEnable_tmp = 0;
#endif

	pr_info("[SSP]%s\n", __func__);

#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR
	if (data->bLpModeEnabled == false) {
		uSensorEnable_tmp = atomic64_read(&data->aSensorEnable);
		uSensorEnable_tmp |= (1ULL << DIGITAL_HALL_EVENT_SENSOR);
		atomic64_set(&data->aSensorEnable, uSensorEnable_tmp);
	}
#endif

	for (uSensorCnt = 0; uSensorCnt < SENSOR_TYPE_SCONTEXT_T; uSensorCnt++) {
		if (atomic64_read(&data->aSensorEnable) & (1ULL << uSensorCnt)) {
			s32 dMsDelay =
				get_msdelay(data->adDelayBuf[uSensorCnt]);

			memcpy(&uBuf[0], &dMsDelay, 4);
			memcpy(&uBuf[4], &data->batchLatencyBuf[uSensorCnt], 4);
			uBuf[8] = data->batchOptBuf[uSensorCnt];
			pr_info("[SSP]%s, enable sensor(%d)\n", __func__,
				uSensorCnt);
			iRet = send_instruction(data, ADD_SENSOR, uSensorCnt, uBuf, 9);
			udelay(10);

#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR
			if (uSensorCnt == DIGITAL_HALL_EVENT_SENSOR && iRet > 0) {
				data->aiCheckStatus[DIGITAL_HALL_EVENT_SENSOR] = RUNNING_SENSOR_STATE;
			}
#endif
		}
	}
}

static void print_sensordata(struct ssp_data *data, unsigned int uSensor)
{
	switch (uSensor) {
	case ACCELEROMETER_SENSOR:
	case GYROSCOPE_SENSOR:
	case ACCELEROMETER_SENSOR_32G:
	case LINEAR_ACCEL_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums, %dms)\n", uSensor,
			data->buf[uSensor].x, data->buf[uSensor].y,
			data->buf[uSensor].z,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
		break;
	case PRESSURE_SENSOR:
#if !defined(CONFIG_SENSORS_SSP_WISE) && !defined(CONFIG_SENSORS_SSP_FRESH)
		ssp_dbg("[SSP] %u : %d, %d, %d, %d (%ums, %dms)\n", uSensor,
			data->buf[uSensor].pressure[0],
			data->buf[uSensor].pressure[1],
			data->iPressureCal,
			data->sw_offset,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
#else
		ssp_dbg("[SSP] %u : %d, %d, (%ums, %dms)\n", uSensor,
			data->buf[uSensor].pressure[0],
			data->buf[uSensor].pressure[1],
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
#endif
		break;
#ifdef CONFIG_SENSORS_SSP_LIGHT_SENSOR
	case LIGHT_SENSOR:
#if defined(CONFIG_SENSORS_SSP_OPT3007)
		ssp_dbg("[SSP] %u : %u, %u, (%ums)\n", uSensor,
			data->buf[uSensor].rdata, data->buf[uSensor].gain,
			get_msdelay(data->adDelayBuf[uSensor]));
#elif defined(CONFIG_SENSORS_SSP_TSL2584)
		ssp_dbg("[SSP] %u : %u, %u, %u, %u, %u(%ums)\n", uSensor,
			data->buf[uSensor].ch0_lower, data->buf[uSensor].ch0_upper,
			data->buf[uSensor].ch1_lower, data->buf[uSensor].ch1_upper,
			data->buf[uSensor].gain, get_msdelay(data->adDelayBuf[uSensor]));
#elif defined(CONFIG_SENSORS_SSP_CM3323)
		ssp_dbg("[SSP] %u : %u, %u, %u, %u(%ums)\n", uSensor,
			data->buf[uSensor].r, data->buf[uSensor].g,
			data->buf[uSensor].b, data->buf[uSensor].w,
			get_msdelay(data->adDelayBuf[uSensor]));
#endif
		break;
	case AUTO_BRIGHTNESS_SENSOR:
		ssp_dbg("[SSP] %u : %u, %u(%ums)\n", uSensor,
			data->buf[uSensor].lux, data->buf[uSensor].flag,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
#endif
#ifdef CONFIG_SENSORS_SSP_TEMP_HUMID_SENSOR
	case TEMPERATURE_HUMIDITY_SENSOR:
#ifdef CONFIG_SENSORS_SSP_SKIN_TEMP
		ssp_dbg("%u : %d, %d, %d, %d(%ums)", uSensor,
			data->buf[uSensor].skin_temp, data->buf[uSensor].skin_humid,
			data->buf[uSensor].env_temp, data->buf[uSensor].env_humid,
			get_msdelay(data->adDelayBuf[uSensor]));
#else
		ssp_dbg("%u : %d, %d, %d (%ums)", uSensor,
			data->buf[uSensor].temp, data->buf[uSensor].humi,
			data->buf[uSensor].time,
			get_msdelay(data->adDelayBuf[uSensor]));
#endif
		break;
#endif
#ifdef CONFIG_SENSORS_SSP_ROT_VECTOR_SENSOR
	case ROTATION_VECTOR_SENSOR:
	case GAME_ROTATION_VECTOR_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d (%ums, %dms)\n", uSensor,
			data->buf[uSensor].quat_a, data->buf[uSensor].quat_b,
			data->buf[uSensor].quat_c, data->buf[uSensor].quat_d,
			data->buf[uSensor].acc_rot,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
		break;
#endif
	case GYRO_UNCALIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].uncal_x, data->buf[uSensor].uncal_y,
			data->buf[uSensor].uncal_z, data->buf[uSensor].offset_x,
			data->buf[uSensor].offset_y,
			data->buf[uSensor].offset_z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case HRM_RAW_SENSOR:
#ifdef CONFIG_SENSORS_SSP_FRONT_HRM_SENSOR
	case FRONT_HRM_RAW_SENSOR:
#endif
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d(%ums)\n",
			uSensor,	data->buf[uSensor].hrm_raw_value1, data->buf[uSensor].hrm_raw_value2,
			data->buf[uSensor].hrm_raw_value3, data->buf[uSensor].hrm_raw_value4,
			data->buf[uSensor].hrm_raw_value5, data->buf[uSensor].hrm_raw_value6,
			data->buf[uSensor].hrm_raw_value7, data->buf[uSensor].hrm_raw_value8,
			data->buf[uSensor].hrm_raw_value9, data->buf[uSensor].hrm_raw_value10,
			data->buf[uSensor].hrm_raw_value11, data->buf[uSensor].hrm_raw_value12,
			data->buf[uSensor].hrm_raw_value13, data->buf[uSensor].hrm_raw_value14,
			data->buf[uSensor].hrm_raw_value15, data->buf[uSensor].hrm_raw_value16,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case HRM_RAW_FAC_SENSOR:
#ifdef CONFIG_SENSORS_SSP_FRONT_HRM_SENSOR
	case FRONT_HRM_RAW_FAC_SENSOR:
#endif
		ssp_dbg("[SSP] %u : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d(%ums)\n",
			uSensor, data->buf[uSensor].hrm_eol_data[0], data->buf[uSensor].hrm_eol_data[1],
			data->buf[uSensor].hrm_eol_data[2], data->buf[uSensor].hrm_eol_data[3],
			data->buf[uSensor].hrm_eol_data[4], data->buf[uSensor].hrm_eol_data[5],
			data->buf[uSensor].hrm_eol_data[6], data->buf[uSensor].hrm_eol_data[7],
			data->buf[uSensor].hrm_eol_data[8], data->buf[uSensor].hrm_eol_data[9],
			data->buf[uSensor].hrm_eol_data[10], data->buf[uSensor].hrm_eol_data[11],
			data->buf[uSensor].hrm_eol_data[12], data->buf[uSensor].hrm_eol_data[13],
			data->buf[uSensor].hrm_eol_data[14], data->buf[uSensor].hrm_eol_data[15],
			data->buf[uSensor].hrm_eol_data[16], data->buf[uSensor].hrm_eol_data[17],
			data->buf[uSensor].hrm_eol_data[18], data->buf[uSensor].hrm_eol_data[19],
			data->buf[uSensor].hrm_eol_data[20], data->buf[uSensor].hrm_eol_data[21],
			data->buf[uSensor].hrm_eol_data[22], data->buf[uSensor].hrm_eol_data[23],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case HRM_RAW_FAC2_SENSOR:
			ssp_dbg("[SSP] %u : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d(%ums)\n",
			uSensor, data->buf[uSensor].hrm_xtalk2_data[0], data->buf[uSensor].hrm_xtalk2_data[1],
			data->buf[uSensor].hrm_xtalk2_data[2], data->buf[uSensor].hrm_xtalk2_data[3],
			data->buf[uSensor].hrm_xtalk2_data[4], data->buf[uSensor].hrm_xtalk2_data[5],
			data->buf[uSensor].hrm_xtalk2_data[6], data->buf[uSensor].hrm_xtalk2_data[7],
			data->buf[uSensor].hrm_xtalk2_data[8], data->buf[uSensor].hrm_xtalk2_data[9],
			data->buf[uSensor].hrm_xtalk2_data[10], data->buf[uSensor].hrm_xtalk2_data[11],
			data->buf[uSensor].hrm_xtalk2_data[12], data->buf[uSensor].hrm_xtalk2_data[13],
			data->buf[uSensor].hrm_xtalk2_data[14], data->buf[uSensor].hrm_xtalk2_data[15],
			data->buf[uSensor].hrm_xtalk2_data[16], data->buf[uSensor].hrm_xtalk2_data[17],
			data->buf[uSensor].hrm_xtalk2_data[18], data->buf[uSensor].hrm_xtalk2_data[19],
			data->buf[uSensor].hrm_xtalk2_data[20], data->buf[uSensor].hrm_xtalk2_data[21],
			data->buf[uSensor].hrm_xtalk2_data[22], data->buf[uSensor].hrm_xtalk2_data[23],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case HRM_LIB_SENSOR:
#ifdef CONFIG_SENSORS_SSP_FRONT_HRM_SENSOR
	case FRONT_HRM_LIB_SENSOR:
#endif
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].hr, data->buf[uSensor].rri,
			data->buf[uSensor].snr,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
#ifdef CONFIG_SENSORS_SSP_UV_SENSOR
	case UV_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d(%ums)\n", uSensor,
			data->buf[uSensor].uv_raw, data->buf[uSensor].hrm_temp,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
#endif
#ifdef CONFIG_SENSORS_SSP_GSR_SENSOR
	case GSR_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].ohm, data->buf[uSensor].adc, data->buf[uSensor].inj_c,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
#endif
#ifdef CONFIG_SENSORS_SSP_ECG_SENSOR
	case ECG_SENSOR:
#endif
	case HRM_ECG_LIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d(%ums)\n", uSensor,
			data->buf[uSensor].ecg_data[0], data->buf[uSensor].ecg_data[1],
			data->buf[uSensor].ecg_data[2], data->buf[uSensor].ecg_data[3],
			data->buf[uSensor].ecg_data[4], data->buf[uSensor].ecg_data[5],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
#ifdef CONFIG_SENSORS_SSP_GRIP_SENSOR
	case GRIP_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d %d(%ums)\n", uSensor,
			data->buf[uSensor].data1, data->buf[uSensor].data2,
			data->buf[uSensor].data3, data->buf[uSensor].data4,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
#endif
	case SENSOR_TYPE_SCONTEXT_T:
		ssp_dbg("[SSP] %u : %d, %d, %d %d(%ums)\n", uSensor,
			data->buf[uSensor].data1, data->buf[uSensor].data2,
			data->buf[uSensor].data3, data->buf[uSensor].data4,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case HRM_BIA_RAW_SENSOR:
		ssp_dbg("[SSP] %u : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d(%ums)\n",
			uSensor,
			data->buf[uSensor].bia_data_value[0], data->buf[uSensor].bia_data_value[1],
			data->buf[uSensor].bia_data_value[2], data->buf[uSensor].bia_data_value[3],
			data->buf[uSensor].bia_data_value[4], data->buf[uSensor].bia_data_value[5],
			data->buf[uSensor].bia_data_value[6], data->buf[uSensor].bia_data_value[7],
			data->buf[uSensor].bia_data_value[8], data->buf[uSensor].bia_data_value[9],
			data->buf[uSensor].bia_data_value[10], data->buf[uSensor].bia_data_value[11],
			data->buf[uSensor].bia_data_value[12], data->buf[uSensor].bia_data_value[13],
			data->buf[uSensor].bia_data_value[14], data->buf[uSensor].bia_data_value[15],
			data->buf[uSensor].bia_data_value[16], data->buf[uSensor].bia_data_value[17],
			data->buf[uSensor].bia_data_value[18], data->buf[uSensor].bia_data_value[19],
			data->buf[uSensor].bia_data_value[20], data->buf[uSensor].bia_data_value[21],
			data->buf[uSensor].bia_data_value[22], data->buf[uSensor].bia_data_value[23],
			data->buf[uSensor].bia_data_value[24], data->buf[uSensor].bia_data_value[25],
			data->buf[uSensor].bia_data_value[26], data->buf[uSensor].bia_data_value[27],
			data->buf[uSensor].bia_data_value[28], data->buf[uSensor].bia_data_value[29],
			data->buf[uSensor].bia_data_value[30], data->buf[uSensor].bia_data_value[31],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GEOMAGNETIC_UNCALIB_SENSOR:
#ifdef CONFIG_SENSORS_SSP_MAGNETIC_32BIT
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].uncal_mag32_data[0], data->buf[uSensor].uncal_mag32_data[1], data->buf[uSensor].uncal_mag32_data[2],
			data->buf[uSensor].uncal_mag32_offset[0], data->buf[uSensor].uncal_mag32_offset[1], data->buf[uSensor].uncal_mag32_offset[2],
			data->buf[uSensor].uncal_mag32_status,
			get_msdelay(data->adDelayBuf[uSensor]));
#else
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].uncal_data[0], data->buf[uSensor].uncal_data[1], data->buf[uSensor].uncal_data[2],
			data->buf[uSensor].uncal_offset[0], data->buf[uSensor].uncal_offset[1], data->buf[uSensor].uncal_offset[2],
			data->buf[uSensor].uncal_status,
			get_msdelay(data->adDelayBuf[uSensor]));
#endif
		break;
	case GEOMAGNETIC_POWER_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].power_data[0], data->buf[uSensor].power_data[1], data->buf[uSensor].power_data[2],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GEOMAGNETIC_CALIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].calib_data[0], data->buf[uSensor].calib_data[1], data->buf[uSensor].calib_data[2],
			data->buf[uSensor].calib_accuracy, data->buf[uSensor].calib_status,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR
	case DIGITAL_HALL_EVENT_SENSOR:
		if (data->wheel_moved) {
			ssp_dbg("[SSP] %u : %d %u (%ums)\n", uSensor,
				data->buf[uSensor].direction, data->buf[uSensor].interrupt, get_msdelay(data->adDelayBuf[uSensor]));
			data->wheel_moved = false;
		}
		break;
	case DIGITAL_HALL_RAW_SENSOR:
	case DIGITAL_HALL_SUB_SENSOR:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].angle, get_msdelay(data->adDelayBuf[uSensor]));
		break;
#endif
	case TEMPERATURE_SENSOR:
		ssp_dbg("[SSP] %u : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d(%ums)\n",
			uSensor,
			data->buf[uSensor].temperature_data_value[0], data->buf[uSensor].temperature_data_value[1],
			data->buf[uSensor].temperature_data_value[2], data->buf[uSensor].temperature_data_value[3],
			data->buf[uSensor].temperature_data_value[4], data->buf[uSensor].temperature_data_value[5],
			data->buf[uSensor].temperature_data_value[6], data->buf[uSensor].temperature_data_value[7],
			data->buf[uSensor].temperature_data_value[8], data->buf[uSensor].temperature_data_value[9],
			data->buf[uSensor].temperature_data_value[10], data->buf[uSensor].temperature_data_value[11],
			data->buf[uSensor].temperature_data_value[12], data->buf[uSensor].temperature_data_value[13],
			data->buf[uSensor].temperature_data_value[14], data->buf[uSensor].temperature_data_value[15],
			data->buf[uSensor].temperature_data_value[16], data->buf[uSensor].temperature_data_value[17],
			data->buf[uSensor].temperature_data_value[18], data->buf[uSensor].temperature_data_value[19],
			data->buf[uSensor].temperature_data_value[20], data->buf[uSensor].temperature_data_value[21],
			data->buf[uSensor].temperature_data_value[22], data->buf[uSensor].temperature_data_value[23],
			data->buf[uSensor].temperature_data_value[24], data->buf[uSensor].temperature_data_value[25],
			data->buf[uSensor].temperature_data_value[26], data->buf[uSensor].temperature_data_value[27],
			data->buf[uSensor].temperature_data_value[28], data->buf[uSensor].temperature_data_value[29],
			data->buf[uSensor].temperature_data_value[30], data->buf[uSensor].temperature_data_value[31],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	default:
		ssp_dbg("[SSP] Wrong sensorCnt: %u\n", uSensor);
		break;
	}
}

void recovery_mcu(struct ssp_data *data)
{
	if (data->uComFailCnt < LIMIT_RESET_CNT) {
		pr_info("[SSP] : %s - uTimeOutCnt(%u), pending(%u)\n", __func__,
			data->uTimeOutCnt, !list_empty(&data->pending_list));
		data->uComFailCnt++;
#if SSP_STATUS_MONITOR
		if (data->bRefreshing) {
			pr_err("[SSP] : %s MCU is refreshing another route....."
				"Wait 2sec\n", __func__);
			msleep(2000);
		} else
#endif
		reset_mcu(data, RESET_TYPE_KERNEL_NO_EVENT);
	} else {
		ssp_enable(data, false);
	}
	data->is_reset_started = true;

	data->uTimeOutCnt = 0;
}

#ifdef CONFIG_SENSORS_SSP_AP_WAKEUP_DEBUG
typedef struct s {
	const char *name;
	phys_addr_t addr;
	void *va;
} SSP_SFRs;

SSP_SFRs sfrs_gpm8[] = {
		{"GPM8_CON", 0x12b30100, 0},
		{"GPM8_DAT", 0x12b30104, 0},
		{"GPM8_PUD", 0x12b30108, 0},
		{"GPM8_DRV", 0x12b3010c, 0},
		{"GPM8_WEINT_CON", 0x12b30720, 0},
		{"GPM8_FLTCON0", 0x12b30820, 0},
		{"GPM8_MASK", 0x12b30920, 0},
		{"GPM8_PEND", 0x12b30A20, 0},
};

SSP_SFRs sfrs_gpa02[] = {
		{"GPA0-2_CON", 0x12850000, 0},
		{"GPA0-2_DAT", 0x12850004, 0},
		{"GPA0-2_PUD", 0x12850008, 0},
		{"GPA0-2_DRV", 0x1285000C, 0},
		{"GPA0-2_WEINT_CON", 0x12850700, 0},
		{"GPA0-2_FLTCON0", 0x12850800, 0},
		{"GGPA0-2_MASK", 0x12850900, 0},
		{"GPA0-2_PEND", 0x12850A00, 0},

};

void init_gpm8_sfrs(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sfrs_gpm8) ; i++) {
		sfrs_gpm8[i].va = ioremap(sfrs_gpm8[i].addr, 0x4);
	}
	for (i = 0; i < ARRAY_SIZE(sfrs_gpa02) ; i++) {
		sfrs_gpa02[i].va = ioremap(sfrs_gpa02[i].addr, 0x4);
	}
}

void print_gpm8(struct ssp_data *data)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sfrs_gpm8) ; i++) {
		if (sfrs_gpm8[i].va)
			nanohub_info("%s: %s 0x%x\n", __func__,
					sfrs_gpm8[i].name, readl_relaxed(sfrs_gpm8[i].va));
		else
			nanohub_err("%s: %s errir\n", __func__, sfrs_gpm8[i].name);

	for (i = 0; i < ARRAY_SIZE(sfrs_gpa02) ; i++) {
		if (sfrs_gpa02[i].va)
			nanohub_info("%s: %s 0x%x\n", __func__,
					sfrs_gpa02[i].name, readl_relaxed(sfrs_gpa02[i].va));
		else
			nanohub_err("%s: %s errir\n", __func__, sfrs_gpa02[i].name);
	}
	}
}
#else
void init_gpm8_sfrs(void){}
void print_gpm8(struct ssp_data *data) {}
#endif

static void print_current_time(void)
{
	static int cnt = 0;
	struct timespec ts;
	struct rtc_time t;

	cnt += 1;
	if (cnt >= 6) {
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &t);

		nanohub_err("[SSP] %d-%02d-%02d %02d:%02d:%02d.%03lu UTC\n",
			t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour,
			t.tm_min, t.tm_sec, (ts.tv_nsec)/1000000);

		cnt = 0;
	}
}

static void debug_work_func(struct work_struct *work)
{
	unsigned int uSensorCnt;
	struct ssp_data *data = container_of(work, struct ssp_data, work_debug);
	unsigned int widx;

	widx = ssp_nanohub_log_buf_check(data);

	nanohub_err("[SSP]: %s(%u) - Sensor state: 0x%llx, En: 0x%llx, RC: %u, CC: %u, DC: %u, TC: %u (%u) (widx = %u)\n",
		__func__, data->uIrqCnt, data->uSensorState, atomic64_read(&data->aSensorEnable), data->uResetCnt,
			data->uComFailCnt, data->uDumpCnt, data->uTimeOutCnt, data->uWheelCnt, widx);

	for (uSensorCnt = 0; uSensorCnt < SENSOR_MAX; uSensorCnt++)
		if ((atomic64_read(&data->aSensorEnable) & (1ULL << uSensorCnt))
			|| data->batchLatencyBuf[uSensorCnt])
			print_sensordata(data, uSensorCnt);

	if (((atomic64_read(&data->aSensorEnable) & (1ULL << ACCELEROMETER_SENSOR))
		&& (data->batchLatencyBuf[ACCELEROMETER_SENSOR] == 0)
		&& (data->uIrqCnt == 0) && (data->uTimeOutCnt > 0))
		|| (data->uTimeOutCnt > LIMIT_TIMEOUT_CNT))
		recovery_mcu(data);

	print_current_time();
	print_gpm8(data);

	data->uIrqCnt = 0;
	data->uWheelCnt = 0;
}

static void debug_timer_func(struct timer_list *t)
{
	//struct ssp_data *data = from_timer(struct ssp_data, t, debug_timer);
	struct ssp_data *data = container_of(t, struct ssp_data, debug_timer);

	queue_work(data->debug_wq, &data->work_debug);
	mod_timer(&data->debug_timer, round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void enable_debug_timer(struct ssp_data *data)
{
	mod_timer(&data->debug_timer,
		round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void disable_debug_timer(struct ssp_data *data)
{
	del_timer_sync(&data->debug_timer);
	cancel_work_sync(&data->work_debug);
}

int initialize_debug_timer(struct ssp_data *data)
{
	timer_setup(&data->debug_timer, (void *)debug_timer_func, 0);

	data->debug_wq = create_singlethread_workqueue("ssp_debug_wq");
	if (!data->debug_wq)
		return ERROR;

	INIT_WORK(&data->work_debug, debug_work_func);
	return SUCCESS;
}

#if SSP_STATUS_MONITOR
static int check_abnormal_status(struct ssp_data *data, unsigned int uSensor)
{
	static s16 pre_buff[3] = {0, };
	int ret = 0;

	if (pre_buff[0] == data->buf[uSensor].x)
		if (pre_buff[1] == data->buf[uSensor].y)
			if (pre_buff[2] == data->buf[uSensor].z) {
				pr_err("[SSP] %s-Sensor[%d] data not changed!!\n",
					__func__, uSensor);
				if (data->batchLatencyBuf[ACCELEROMETER_SENSOR]
					>= (SSP_MONITOR_TIME * 1000))
					pr_err("[SSP]:- batchLatencyBuf[%d]ms pass...!!!\n",
					data->batchLatencyBuf[ACCELEROMETER_SENSOR]);
				else {
					ret = -1;
					pre_buff[0] = 0;
					pre_buff[1] = 0;
					pre_buff[2] = 0;
				}
				return ret;
			}

	pre_buff[0] = data->buf[uSensor].x;
	pre_buff[1] = data->buf[uSensor].y;
	pre_buff[2] = data->buf[uSensor].z;

	return ret;
}

static void debug_polling_func(struct work_struct *work)
{
	struct ssp_data *data = container_of((struct delayed_work *)work,
		struct ssp_data, polling_work);

	if (data->bSspShutdown) {
		pr_err("[SSP] : %s MCU is disabled...\n", __func__);
		goto out;
	}
	if (data->bRefreshing) {
		pr_err("[SSP] : %s MCU is refreshing another route.....\n",
			__func__);
		goto out;
	}

#if 0 /* Check raised IRQ count number */
	if (atomic64_read(&data->aSensorEnable) & (!data->uSubIrqCnt)) {
		pr_err("[SSP] : %s(%u) aSensorEnable:0x%llx. No irp happened. MCU reset now!\n",
			__func__, data->uSubIrqCnt,
			(unsigned int)atomic64_read(&data->aSensorEnable));
		if (data->bSspShutdown == false) {
			/* reset_mcu(data); */
			goto out;
		} else
			pr_err("[SSP] : %s MCU is shutdowned. Could not reset.\n",
			__func__);
		goto out;
	}
#endif
#if 1 /* Check if Acc data is the same as previous or not. */
	/* Check if acc sensor keep working */
	if (atomic64_read(&data->aSensorEnable) & (1ULL << ACCELEROMETER_SENSOR))
		if (check_abnormal_status(data, ACCELEROMETER_SENSOR)) {
			pr_err("[SSP] : Acc not working. MCU reset now...\n");
			reset_mcu(data, RESET_TYPE_CHUB_NO_EVENT);
		}
#endif
	data->uSubIrqCnt = 0;

out:
	schedule_delayed_work(&data->polling_work,
		msecs_to_jiffies(SSP_MONITOR_TIME * 1000));
}

int initialize_polling_work(struct ssp_data *data)
{
	INIT_DELAYED_WORK(&data->polling_work, debug_polling_func);
	pr_info("[SSP] : %s finished\n", __func__);

	return SUCCESS;
}
#endif

void print_report_debug(struct ssp_data *data, struct sensor_value *event, int type)
{
	static int log_count[3] = {0,};

	if(type == ACCELEROMETER_SENSOR) {
		if(data->bypass_logcnt < 3) {
			data->bypass_logcnt++;
			pr_err("[SSP]acc_data(%d), %d %d %d %llu\n", data->bypass_logcnt, 
				data->buf[type].x, data->buf[type].y, data->buf[type].z, event->timestamp);
		}
	}

	if (type == HRM_ECG_LIB_SENSOR) {
		log_count[0]++;
		if (log_count[0] > 255)
			log_count[0] = 0;
		if (!(data->debug_enable & (1ULL << HRM_ECG_LIB_SENSOR))) {
			if (log_count[0] < 255)
				return;
		}

		pr_info("[SSP]ecg_lib_data(%d), %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lld %lld %lld %lld\n",
			log_count[0],
			event->ecg_data_value[0],
			event->ecg_data_value[1],
			event->ecg_data_value[2],
			event->ecg_data_value[3],
			event->ecg_data_value[4],
			event->ecg_data_value[5],
			event->ecg_data_value[6],
			event->ecg_data_value[7],
			event->ecg_data_value[8],
			event->ecg_data_value[9],
			event->ecg_data_value[10],
			event->ecg_data_value[11],
			event->ecg_data_value[12],
			event->ecg_data_value[13],
			event->ecg_data_value[14],
			event->ecg_data_value[15],
			event->ecg_data_value[16],
			event->ecg_data_value[17],
			event->ecg_data_value[18],
			event->ecg_data_value[19],
			event->ecg_data_value[20],
			event->ecg_data_value[21],
			event->ecg_data_value[22],
			event->ecg_data_value[23],
			event->ecg_data_value[24],
			event->ecg_data_value[25],
			event->ecg_data_value[26],
			event->ecg_data_value[27],
			event->ecg_data_value[28],
			event->ecg_data_value[29],
			event->ecg_data_value[30],
			event->ecg_data_value[31],
			((event->timestamp & 0xffff000000000000) >> 48),
			((event->timestamp & 0x0000ffff00000000) >> 32),
			((event->timestamp & 0x00000000ffff0000) >> 16),
			(event->timestamp & 0x000000000000ffff));
	}

	if (type == HRM_BIA_RAW_SENSOR) {
		log_count[1]++;
		if (log_count[1] > 300) {
			log_count[1] = 0;
		}
		if (!(data->debug_enable & (1ULL << HRM_BIA_RAW_SENSOR))) {
			if (log_count[1] < 300) {
				return;
			}
		}

		pr_info("[SSP] bia_raw_data(%d), %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lld %lld %lld %lld\n",
			log_count[1],
			event->bia_data_value[0],
			event->bia_data_value[1],
			event->bia_data_value[2],
			event->bia_data_value[3],
			event->bia_data_value[4],
			event->bia_data_value[5],
			event->bia_data_value[6],
			event->bia_data_value[7],
			event->bia_data_value[8],
			event->bia_data_value[9],
			event->bia_data_value[10],
			event->bia_data_value[11],
			event->bia_data_value[12],
			event->bia_data_value[13],
			event->bia_data_value[14],
			event->bia_data_value[15],
			event->bia_data_value[16],
			event->bia_data_value[17],
			event->bia_data_value[18],
			event->bia_data_value[19],
			event->bia_data_value[20],
			event->bia_data_value[21],
			event->bia_data_value[22],
			event->bia_data_value[23],
			event->bia_data_value[24],
			event->bia_data_value[25],
			event->bia_data_value[26],
			event->bia_data_value[27],
			event->bia_data_value[28],
			event->bia_data_value[29],
			event->bia_data_value[30],
			event->bia_data_value[31],
			((event->timestamp & 0xffff000000000000) >> 48),
			((event->timestamp & 0x0000ffff00000000) >> 32),
			((event->timestamp & 0x00000000ffff0000) >> 16),
			(event->timestamp & 0x000000000000ffff));
	}

	if (type == HRM_RAW_FAC2_SENSOR) {
		log_count[2]++;
		if (log_count[2] > 300) {
			log_count[2] = 0;
		}
		if (!(data->debug_enable & (1ULL << HRM_RAW_FAC2_SENSOR))) {
			if (log_count[2] < 300) {
				return;
			}
		}

		pr_info("[SSP] hrm_raw_fac2_data(%d), %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lld %lld %lld %lld\n",
		log_count[2],
		event->hrm_xtalk2_data[0],
		event->hrm_xtalk2_data[1],
		event->hrm_xtalk2_data[2],
		event->hrm_xtalk2_data[3],
		event->hrm_xtalk2_data[4],
		event->hrm_xtalk2_data[5],
		event->hrm_xtalk2_data[6],
		event->hrm_xtalk2_data[7],
		event->hrm_xtalk2_data[8],
		event->hrm_xtalk2_data[9],
		event->hrm_xtalk2_data[10],
		event->hrm_xtalk2_data[11],
		event->hrm_xtalk2_data[12],
		event->hrm_xtalk2_data[13],
		event->hrm_xtalk2_data[14],
		event->hrm_xtalk2_data[15],
		event->hrm_xtalk2_data[16],
		event->hrm_xtalk2_data[17],
		event->hrm_xtalk2_data[18],
		event->hrm_xtalk2_data[19],
		event->hrm_xtalk2_data[20],
		event->hrm_xtalk2_data[21],
		event->hrm_xtalk2_data[22],
		event->hrm_xtalk2_data[23],
		((event->timestamp & 0xffff000000000000) >> 48),
		((event->timestamp & 0x0000ffff00000000) >> 32),
		((event->timestamp & 0x00000000ffff0000) >> 16),
		(event->timestamp & 0x000000000000ffff));
	}
}

