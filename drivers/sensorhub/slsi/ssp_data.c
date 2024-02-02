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
#include <linux/math64.h>
#include <linux/sched.h>
#include "ssp_iio.h"
#include "ssp_scontext.h"
#include "ssp_dump.h"
#include "ssp_lpm.h"

/* SSP -> AP Instruction */
#define MSG2AP_INST_BYPASS_DATA			0x37
#define MSG2AP_INST_LIBRARY_DATA		0x01
#define MSG2AP_INST_DEBUG_DATA			0x03
#define MSG2AP_INST_BIG_DATA			0x04
#define MSG2AP_INST_META_DATA			0x05
#define MSG2AP_INST_TIME_SYNC			0x06
#define MSG2AP_INST_RESET				0x07
#define MSG2AP_INST_GYRO_CAL			0x08
#define MSG2AP_INST_MAG_CAL				0x09
#define MSG2AP_INST_DUMP_DATA			0xDD

#define U64_MS2NS 1000000ULL
#define U64_US2NS 1000ULL
#define U64_MS2US 1000ULL
#define MS_IDENTIFIER 1000000000U

#define SSP_TIMESTAMP_SYNC_TIMER_SEC	(5 * HZ)
#define TIMESTAMP_LENGTH				(8)

/*************************************************************************/
/* SSP parsing the dataframe                                             */
/*************************************************************************/

u64 get_current_timestamp(void)
{
	u64 timestamp;
	struct timespec ts;

	ts = ktime_to_timespec(ktime_get_boottime());
	timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

	return timestamp;
}

static void get_timestamp(struct ssp_data *data, char *pchRcvDataFrame,
		int *iDataIdx, struct sensor_value *sensorsdata,
		struct ssp_time_diff *sensortime, int iSensorData, u16 total_cnt)
{
	u64 shubTimeNs = 0;
	u64 current_timestamp = get_current_timestamp();
	static u64 check_timestamp;
	s64 diff = 0;
	
	memcpy(&shubTimeNs, pchRcvDataFrame + *iDataIdx, 8);

	if (data->debug_enable & (1ULL << 62)) {
		pr_info("[SSP]%s: %llu(%d)  %llu\n", __func__, shubTimeNs, iSensorData, current_timestamp);
	}
	sensorsdata->timestamp = shubTimeNs;
	*iDataIdx += 8;

	if (iSensorData == DIGITAL_HALL_EVENT_SENSOR || iSensorData == AUTO_BRIGHTNESS_SENSOR) {
		return;
	}

	diff = (s64)(shubTimeNs - current_timestamp);

	if (diff > (s64)(100 * U64_US2NS) || (diff < (s64)(-3 * U64_MS2NS) && current_timestamp - check_timestamp > 50 * U64_MS2NS)) {	
		data->ts_diff[0] = iSensorData;
		memcpy(&data->ts_diff[1], &diff, 8);
		queue_work(data->ts_diff_sync_wq, &data->work_ts_diff_sync);
		if (data->debug_enable & (1ULL << 62)) {
			pr_err("[SSP] %s: diff_time: %lld(%d)\n", __func__, diff, iSensorData);
		}
		check_timestamp = current_timestamp;
	}
}


#ifdef CONFIG_SENSORS_SSP_PRESSURE_SENSOR
static void get_pressure_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	s16 temperature = 0;
	memcpy(&sensorsdata->pressure[0], pchRcvDataFrame + *iDataIdx, 4);
	memcpy(&temperature, pchRcvDataFrame + *iDataIdx + 4, 2);
	sensorsdata->pressure[1] = temperature;
	*iDataIdx += 6;
}
#endif

void get_sensordata(struct ssp_data *data, char *pchRcvDataFrame, int *iDataIdx, int sensor_type, struct sensor_value *sensorsdata)
{
	if (sensor_type == PRESSURE_SENSOR) {
		get_pressure_sensordata(pchRcvDataFrame, iDataIdx, sensorsdata);
		return;
	}
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, data->info[sensor_type].get_data_len);
	*iDataIdx += data->info[sensor_type].get_data_len;
	memcpy(&data->buf[sensor_type], (char *)sensorsdata, data->info[sensor_type].get_data_len);
}

int handle_big_data(struct ssp_data *data,
	char *pchRcvDataFrame, int *pDataIdx)
{
	struct ssp_big *big = kzalloc(sizeof(*big), GFP_KERNEL);

	if (!big)
		return -ENOMEM;

	big->data = data;
	big->bigType = pchRcvDataFrame[(*pDataIdx)++];

	/* WOM does not use length & addr inform */
	if (big->bigType == BIG_TYPE_WOM) {
		pr_info("[SSP]: %s: we've got WOM send big cmd\n", __func__);
	} else {
		memcpy(&big->length, pchRcvDataFrame + *pDataIdx, 4);
		*pDataIdx += 4;
		memcpy(&big->addr, pchRcvDataFrame + *pDataIdx, 4);
		*pDataIdx += 4;
	}

	if (big->bigType >= BIG_TYPE_MAX) {
		pr_warn("[SSP]: %s: Unsupported big_type : %u\n",
			__func__, big->bigType);
		kfree(big);
		return FAIL;
	}

	if (!data->ssp_big_task[big->bigType]) {
		pr_warn("[SSP]: %s: !!No handler for big_type : %u\n",
			__func__, big->bigType);
		kfree(big);
		return FAIL;
	}

	INIT_WORK(&big->work, data->ssp_big_task[big->bigType]);
	queue_work(data->debug_wq, &big->work);
	return SUCCESS;
}

void refresh_task(struct work_struct *work)
{
	struct ssp_data *data = container_of((struct delayed_work *)work,
			struct ssp_data, work_refresh);

	if (data->bSspShutdown == true) {
		pr_err("[SSP]: %s - ssp already shutdown\n", __func__);
		return;
	}

	ssp_wake_lock(data->ssp_wake_lock);
	pr_err("[SSP]: %s\n", __func__);
	data->uResetCnt++;

	if (data->uResetCnt == 1) {
		initialize_ssp_dump(data);
		init_gpm8_sfrs();
	}
	if (initialize_mcu(data) > 0) {
		sync_sensor_state(data);
		report_scontext_notice_data(data, MSG2SSP_AP_STATUS_RESET);
		enable_timestamp_sync_timer(data);
		if (data->uLastAPState != 0) {
			pr_err("[SSP]%s, Send last AP state(%d)\n", __func__,
				data->uLastAPState);
			ssp_send_cmd(data, data->uLastAPState, 0);
		}
		data->uTimeOutCnt = 0;

#ifdef CONFIG_SENSORS_SSP_AP_WAKEUP
		//ssp_enable_irq(data, 1);
#endif
	} else
		data->uSensorState = 0;

	data->is_reset_started = false;
	ssp_wake_unlock(data->ssp_wake_lock);
#if SSP_STATUS_MONITOR
	data->bRefreshing =  false;
#endif
}

int queue_refresh_task(struct ssp_data *data, int delay)
{
	cancel_delayed_work_sync(&data->work_refresh);

	//INIT_DELAYED_WORK(&data->work_refresh, refresh_task);
	queue_delayed_work(data->debug_wq, &data->work_refresh,
			msecs_to_jiffies(delay));
	return SUCCESS;
}

static void ssp_sensorhub_log(const char *func_name,
				const char *data, int length)
{
	char buf[6];
	char *log_str;
	int log_size;
	int i;

	if (likely(length <= BIG_DATA_SIZE))
		log_size = length;
	else
		log_size = PRINT_TRUNCATE * 2 + 1;

	log_size = sizeof(buf) * log_size + 1;
	log_str = kzalloc(log_size, GFP_ATOMIC);
	if (unlikely(!log_str)) {
		ssp_errf("allocate memory for data log err");
		return;
	}

	for (i = 0; i < length; i++) {
		if (length < BIG_DATA_SIZE ||
			i < PRINT_TRUNCATE || i >= length - PRINT_TRUNCATE) {
			snprintf(buf, sizeof(buf), "%d", (signed char)data[i]);
			strlcat(log_str, buf, log_size);

			if (i < length - 1)
				strlcat(log_str, ", ", log_size);
		}
		if (length > BIG_DATA_SIZE && i == PRINT_TRUNCATE)
			strlcat(log_str, "..., ", log_size);
	}

	pr_info("[SSP]: %s - %s (%d)\n", func_name, log_str, length);
	kfree(log_str);
}

int parse_dataframe(struct ssp_data *data,
	char *pchRcvDataFrame, int iLength)
{
	int iDataIdx, iSensorData;
	u16 length = 0, total_cnt = 0;
	struct sensor_value sensorsdata;
	struct ssp_time_diff sensortime;
	s16 caldata[3] = { 0, };
	int iRet = FAIL;
#ifdef CONFIG_SENSORS_SSP_MAGNETIC_SENSOR
	s8 bz_mag[3] = {0, };
#endif

	sensortime.time_diff = 0;
	data->uIrqCnt++;

	for (iDataIdx = 0; iDataIdx < iLength;) {
		switch (pchRcvDataFrame[iDataIdx++]) {
		case MSG2AP_INST_BYPASS_DATA:
			iSensorData = pchRcvDataFrame[iDataIdx++];
			if ((iSensorData < 0) || (iSensorData >= SENSOR_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d\n", __func__,
						iSensorData);
				ssp_sensorhub_log(__func__, pchRcvDataFrame, iLength);
				return ERROR;
			}

			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
			sensortime.batch_count = sensortime.batch_count_fixed = length;
			sensortime.batch_mode = length > 1 ? BATCH_MODE_RUN : BATCH_MODE_NONE;

			/* For batch_mode debugging */
			if (sensortime.batch_mode) {
				pr_info("[SSP]: current batch cnt:%d, type:%d, udelay:%lld, latency:%d\n",
					sensortime.batch_count, iSensorData, data->adDelayBuf[iSensorData],
					data->batchLatencyBuf[iSensorData]);
				total_cnt = sensortime.batch_count;
			}

			do {
				get_sensordata(data, pchRcvDataFrame, &iDataIdx, iSensorData, &sensorsdata);
				get_timestamp(data, pchRcvDataFrame, &iDataIdx, &sensorsdata, &sensortime, iSensorData, total_cnt);
#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR
				if (iSensorData == DIGITAL_HALL_EVENT_SENSOR) {
					report_hallic_data(data, &sensorsdata);
					if (data->wheel_test) {
						digital_hall_send_uevent(data);
					}
				} else
#endif
					report_sensor_data(data, iSensorData, &sensorsdata);
				sensortime.batch_count--;
			} while ((sensortime.batch_count > 0) && (iDataIdx < iLength));

			if (sensortime.batch_count > 0)
				pr_err("[SSP]: %s batch count error (%d)\n", __func__, sensortime.batch_count);
			data->reportedData[iSensorData] = true;
			break;
		case MSG2AP_INST_DEBUG_DATA:
			iSensorData = print_mcu_debug(pchRcvDataFrame, &iDataIdx, iLength, data);
			if (iSensorData) {
				pr_err("[SSP]: %s - Mcu data frame3 error %d\n", __func__,
						iSensorData);
				return ERROR;
			}
			break;
		case MSG2AP_INST_LIBRARY_DATA:
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
#ifdef CONFIG_SENSORS_SSP_LPM_MOTION
			if (data->bLpModeEnabled == true)
				iRet = ssp_parse_motion(data, pchRcvDataFrame,
							iDataIdx, iDataIdx + length);
			else
				report_scontext_data(data, pchRcvDataFrame + iDataIdx, length);
#else
			report_scontext_data(data, pchRcvDataFrame + iDataIdx, length);
#endif
			iDataIdx += length;
			break;
		case MSG2AP_INST_BIG_DATA:
			iRet = handle_big_data(data,
					pchRcvDataFrame, &iDataIdx);
			if (iRet < 0) {
				pr_err("[SSP]%s-Mcu handle BIG data err(%d)\n",
					__func__, iRet);
				return ERROR;
			}
			break;
		case MSG2AP_INST_META_DATA:
			sensorsdata.meta_data.what = pchRcvDataFrame[iDataIdx++];
			sensorsdata.meta_data.sensor = pchRcvDataFrame[iDataIdx++];
			if ((sensorsdata.meta_data.sensor < 0) || (sensorsdata.meta_data.sensor >= SENSOR_TYPE_SCONTEXT_T)) {
				pr_err("mcu meta data sensor dataframe err %d", sensorsdata.meta_data.sensor);
				return ERROR;
			}
			report_meta_data(data, &sensorsdata);
			break;
		case MSG2AP_INST_TIME_SYNC:
			data->bTimeSyncing = true;
			break;
		case MSG2AP_INST_RESET:
			pr_info("[SSP]%s-Reset MSG received from MCU.\n",
				__func__);
			reset_mcu_recovery(data, RESET_TYPE_CHUB_SILENT_RESET);
			iDataIdx += 2;
			break;
		case MSG2AP_INST_DUMP_DATA:
			pr_info("[SSP]%s-Dump MSG received from MCU but Chub not support this feature!!!\n",
				__func__);
//			debug_crash_dump(data, pchRcvDataFrame, iLength);
//			handle_dump_data(data, pchRcvDataFrame, iLength);
			return SUCCESS;
			break;
		case MSG2AP_INST_GYRO_CAL:
			pr_info("[SSP]: %s - Gyro caldata received from MCU\n",  __func__);
			memcpy(caldata, pchRcvDataFrame + iDataIdx, sizeof(caldata));
			ssp_wake_lock(data->ssp_wake_lock);
			save_gyro_caldata(data, caldata);
			ssp_wake_unlock(data->ssp_wake_lock);
			iDataIdx += sizeof(caldata);
			break;
#ifdef CONFIG_SENSORS_SSP_MAGNETIC_SENSOR
		case MSG2AP_INST_MAG_CAL:
			pr_info("[SSP]: %s - Mag caldata received from MCU\n",  __func__);
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
			memcpy(bz_mag, pchRcvDataFrame + iDataIdx, sizeof(bz_mag));
			//ssp_sensorhub_log(__func__, pchRcvDataFrame + iDataIdx + sizeof(bz_mag), iLength - (iDataIdx + sizeof(bz_mag)));
			ssp_wake_lock(data->ssp_wake_lock);
			parse_mag_cal_data(data, pchRcvDataFrame + iDataIdx + sizeof(bz_mag), bz_mag[1]);
			ssp_wake_unlock(data->ssp_wake_lock);
			iDataIdx += length;
			break;
#endif
		}
	}

	return SUCCESS;
}

void ssp_timestamp_sync_cmd(struct ssp_data *data)
{
	u64 cur_time = 0;
	int ret;

	cur_time = get_current_timestamp();

	ret = ssp_send_write_cmd(data, SYNC_TIMESTAMP, 0, (char *)&cur_time, TIMESTAMP_LENGTH, __func__);
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - TIMESTAMP_SYNC CMD fail %d\n", __func__, ret);
	}

	pr_err("[SSP] %s: %llu\n", __func__, cur_time);
}


static void timestamp_sync_work_func(struct work_struct *work)
{
	struct ssp_data *data = container_of(work, struct ssp_data, work_ts_sync);
	char buffer[4] = {0x02, 0x01, 0xFC, 0xA0};

	ssp_timestamp_sync_cmd(data);

	if (data->is_log_full) {
		report_scontext_data(data, buffer, 4);
		data->is_log_full = false;
	}
}

static void timestamp_diff_sync_work_func(struct work_struct *work)
{
	struct ssp_data *data = container_of(work, struct ssp_data, work_ts_diff_sync);
	s64 diff_time = 0;
	int ret;

	ret = ssp_send_write_cmd(data, SYNC_TIMESTAMP_OVER, 0, (char *)data->ts_diff, TIMESTAMP_LENGTH + 1, __func__);
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - TIMESTAMP_SYNC CMD fail %d\n", __func__, ret);
	}

	if (data->debug_enable & (1ULL << 62)) {
		memcpy(&diff_time, &data->ts_diff[1], 8);
		pr_err("[SSP] %s: diff_time: %lld(%d)\n", __func__, diff_time, data->ts_diff[0]);
	}
}

static void timestamp_sync_timer_func(struct timer_list *timer)
{
	struct ssp_data *data = container_of(timer, struct ssp_data, ts_sync_timer);

	queue_work(data->ts_sync_wq, &data->work_ts_sync);
	mod_timer(&data->ts_sync_timer, round_jiffies_up(jiffies + SSP_TIMESTAMP_SYNC_TIMER_SEC));
}

void enable_timestamp_sync_timer(struct ssp_data *data)
{
	mod_timer(&data->ts_sync_timer, round_jiffies_up(jiffies + SSP_TIMESTAMP_SYNC_TIMER_SEC));
}

void disable_timestamp_sync_timer(struct ssp_data *data)
{
	del_timer_sync(&data->ts_sync_timer);
	cancel_work_sync(&data->work_ts_sync);
}

int initialize_timestamp_sync_timer(struct ssp_data *data)
{
	timer_setup(&data->ts_sync_timer, (void *)timestamp_sync_timer_func, 0);

	data->ts_sync_wq = create_singlethread_workqueue("ssp_ts_sync_wq");
	if (!data->ts_sync_wq)
		return ERROR;

	INIT_WORK(&data->work_ts_sync, timestamp_sync_work_func);
	return SUCCESS;
}

int initialize_timestamp_diff_sync_work(struct ssp_data *data)
{
	data->ts_diff_sync_wq = create_singlethread_workqueue("ssp_ts_diff_sync_wq");
	if (!data->ts_diff_sync_wq)
		return ERROR;

	INIT_WORK(&data->work_ts_diff_sync, timestamp_diff_sync_work_func);
	return SUCCESS;
}


void initialize_function_pointer(struct ssp_data *data)
{
    data->ssp_big_task[BIG_TYPE_READ_LIB] = ssp_read_big_library_task;
}
