/*
 *  Copyright (C) 2018, Samsung Electronics Co. Ltd. All Rights Reserved.
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
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
//#include <linux/io.h>
//#include <linux/slab.h>
//#include <linux/vmalloc.h>

#include "ssp.h"
#include "ssp_platform.h"
#include "ssp_dump.h"
#include "ssp_iio.h"

#define SENSORHUB_DUMP_DIR				"/data/vendor/w_sensorhub"//"/opt/usr/data/gnss"
#define BUGREPORT_DIR					"/data/system/sensor_service"
#define NANOHUB_DUMP_DIR				"/data/log"
#define NANOHUB_LOG_FILE_PATH			"/sys/kernel/debug/nanohub/log"

#define SENSORHUB_DUMP_INFO_LEN_MAX		500
#define SENSORHUB_DUMP_MAX				2
#define NANOHUB_LOG_MAX					(5)

#define SENSORHUB_DUMP_NOTI_EVENT               0xFC

static char *sensorhub_dump;
static int sensorhub_dump_size;
static bool bSdumpRefreshed;
static char *sensorhub_info;
static int sensorhub_info_size;

#define NANOHUB_LOG_MODE_RESET		(1)
#define NANOHUB_LOG_MODE_AUTO		(2)
#define NANOHUB_LOG_MODE_MANUAL		(3)
#define NANOHUB_LOG_MODE_BUGREPORT	(4)

#if 0
void free_shub_dump_memory(void)
{
	if (sensorhub_dump == NULL)
		return;

	ssp_infof("free sensorhub_dump memory(%d)-0x%x", sensorhub_dump_size, sensorhub_dump);
	kvfree(sensorhub_dump);
	sensorhub_dump = NULL;
	sensorhub_dump_size = 0;
}
#endif

void initialize_ssp_dump(struct ssp_data *data)
{
	ssp_infof();

	if (sensorhub_dump == NULL) {
		sensorhub_dump_size = get_sensorhub_dump_size();
		if (sensorhub_dump_size == 0)
			return;

		sensorhub_dump = (char *)kvzalloc(sensorhub_dump_size, GFP_KERNEL);
		if (PTR_ERR_OR_ZERO(sensorhub_dump)) {
			ssp_infof("memory alloc failed");
			sensorhub_dump = NULL;
			sensorhub_dump_size = 0;
			return;
		}
		ssp_infof("dump size %d", sensorhub_dump_size);
	}

	if (sensorhub_info == NULL) {
		sensorhub_info = (char *)kvzalloc(SENSORHUB_DUMP_INFO_LEN_MAX, GFP_KERNEL);
		if (PTR_ERR_OR_ZERO(sensorhub_info)) {
			ssp_infof("memory alloc failed");
			sensorhub_info = NULL;
			sensorhub_info_size = 0;
			return;
		}
		ssp_infof("sensorhub_info size %d", SENSORHUB_DUMP_INFO_LEN_MAX);
	}
}

void reset_noti_task(struct work_struct *work)
{
	struct ssp_data *data = container_of((struct work_struct *)work,
							struct ssp_data, work_reset_noti);
	char buffer[4] = {0x02, 0x01, 0x00, 0x00};

	buffer[2] = SENSORHUB_DUMP_NOTI_EVENT;
	buffer[3] = data->chub_err_type;
	report_scontext_data(data, buffer, 4);

	ssp_infof("");
}

void write_ssp_dump_file(struct ssp_data *data, char *info, void *dump, int size, int type, u32 *gpr, int num_gpr)
{
	char *pbuf;
	int i;
	struct timeval cur_time;
	struct tm t;

	if ((dump == NULL) || (size == 0)) {
		ssp_errf("dump memory or size error 0x%x(%d)", dump, size);
		return;
	} else if (sensorhub_dump == NULL) {
		ssp_errf("dump ptr is NULL");
		return;
	} else if (size != sensorhub_dump_size) {
		ssp_errf("dump size mis-match(%d:%d)", size, sensorhub_dump_size);
		return;
	}

	ssp_infof("allocated memeory(%d)-0x%x", sensorhub_dump_size, sensorhub_dump);

	if (sensorhub_info != NULL) {
		pbuf = sensorhub_info;
		do_gettimeofday(&cur_time);
		time64_to_tm(cur_time.tv_sec, 0, &t);

		pbuf += sprintf(pbuf, "%s\n", info);
		pbuf += sprintf(pbuf, "%02u[%d-%d %d:%d:%d] Sensor:0x%llx, Lib:0x%llx, RC:%u, CC:%u, DC:%u, TC:%u\n",
			data->reset_type, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
			data->uSensorState, data->uLibraryState, data->uResetCnt, data->uComFailCnt,
			data->uDumpCnt, data->uTimeOutCnt);

		pbuf += sprintf(pbuf, "=====================================\n");
		pbuf += sprintf(pbuf, "CHUB CPU register dump\n");
		for (i = 0 ; i <= num_gpr-2 ; i++)
			pbuf += sprintf(pbuf, "R%02d		: %08x\n", i, gpr[i]);
		pbuf += sprintf(pbuf, "PC		: %08x\n", gpr[num_gpr-1]);
		pbuf += sprintf(pbuf, "=====================================\n");

		sensorhub_info_size = pbuf - sensorhub_info;

		ssp_errf("%s %d\n", __func__, sensorhub_info_size);
		//pr_err("%s", sensorhub_info);
	}

	//memcpy(sensorhub_info, info, sizeof(sensorhub_info));
	memcpy_fromio(sensorhub_dump, dump, sensorhub_dump_size);
	bSdumpRefreshed = true;

	data->chub_err_type = type;
	queue_work(data->debug_wq, &data->work_reset_noti);
}

static int ssp_nanohub_log_store(struct ssp_data *data, int mode, char *out_name)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *filp = NULL;
	char *buffer = NULL;
	char file_path[64] = {0,};
	int size = SZ_512K * 4;
	struct timespec ts;
	struct rtc_time t;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &t);

	nanohub_err("[SSP] %s, %d-%02d-%02d %02d:%02d:%02d.%03lu UTC\n", __func__,
		t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour,
		t.tm_min, t.tm_sec, (ts.tv_nsec)/1000000);
	nanohub_err("============================================================\n");

#ifdef CONFIG_SENSORS_SSP_HRM_ADI	
	if(data->hrm_vender == 1) nanohub_err("HRM_ADI(%d/%d)\n", data->hrm_reading1, data->hrm_reading2);
	else if(data->hrm_vender == 0) nanohub_err("HRM_TI(%d/%d)\n", data->hrm_reading1, data->hrm_reading2);
	else nanohub_err("HRM_NONE(%d/%d)\n", data->hrm_reading1, data->hrm_reading2);
#endif

	// 1. get ptr & size of nanohub log buffer
	buffer = ssp_get_chub_bin_logbuf_ptr(data);
	size = ssp_get_chub_bin_logbuf_size(data);

	// 2. make log file name
	if (mode == NANOHUB_LOG_MODE_RESET) {
		snprintf(file_path, sizeof(file_path), "%s/sensorhub-nanohub-log-%d.txt", SENSORHUB_DUMP_DIR, data->dump_index);
	} else if (mode == NANOHUB_LOG_MODE_AUTO) {
		snprintf(file_path, sizeof(file_path), "%s/nanohub-log-%d.txt", SENSORHUB_DUMP_DIR, data->log_index);
	} else if (mode == NANOHUB_LOG_MODE_BUGREPORT) {
		snprintf(file_path, sizeof(file_path), "%s/bugreport-nanohub-log.txt", BUGREPORT_DIR);
		snprintf(out_name, sizeof(file_path), "bugreport-nanohub-log.txt");
	} else if (mode == NANOHUB_LOG_MODE_MANUAL) {
		snprintf(file_path, sizeof(file_path), "%s/dumpstate-nanohub-log.txt", SENSORHUB_DUMP_DIR);
		snprintf(out_name, sizeof(file_path), "dumpstate-nanohub-log.txt");
	} else {
		snprintf(out_name, sizeof(file_path), "nanohub-log-%d%02d%02d%02d%02d.txt",
		t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min);
		snprintf(file_path, sizeof(file_path), "%s/%s", SENSORHUB_DUMP_DIR, out_name);
	}

	// 3. store nanohub log file to /data/vendor/w_sensorhub
	mutex_lock(&data->file_mutex);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(file_path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (IS_ERR(filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(filp);
		ssp_errf("open '%s' file fail: %d\n", file_path, ret);
		mutex_unlock(&data->file_mutex);
		return ret;
	}
	// 3-1. mark time info at the start of the log
	if (sensorhub_info != NULL && mode == NANOHUB_LOG_MODE_AUTO) {
		char *pbuf;
		pbuf = sensorhub_info;
		pbuf += sprintf(pbuf, "[%d-%02d-%02d %02d:%02d:%02d UTC]\n",
			t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
		pbuf += sprintf(pbuf, "===============================================================\n");
		sensorhub_info_size = pbuf - sensorhub_info;

		ssp_errf("%s %d\n", __func__, sensorhub_info_size);

		if (sensorhub_info_size < 100) {
			ret = vfs_write(filp, sensorhub_info, sensorhub_info_size, &filp->f_pos);
			if (ret < 0)
				ssp_errf("can't write dump info to file (%d)", ret);
		}
	}
	// 3-2. store nanohub log
	ret = vfs_write(filp, buffer, size, &filp->f_pos);
	if (ret < 0) {
		ssp_errf("Can't write dump to file (%d)", ret);
	}
	filp_close(filp, NULL);
	set_fs(old_fs);

	mutex_unlock(&data->file_mutex);

	ssp_errf("%s - Finished (%s, %d, %d)\n", __func__, file_path, ret, size);

	if (mode == 2)
		data->log_index = (data->log_index+1) % NANOHUB_LOG_MAX;

	return SUCCESS;
}

void ssp_dump_file_store(struct ssp_data *data, void *buf, int size)
{
	char file_path[64] = {0,};
	char info_path[64] = {0,};
	struct file *filp;
	mm_segment_t old_fs;
	int ret;
	struct timeval cur_time;
	struct tm t;

	do_gettimeofday(&cur_time);
	time64_to_tm(cur_time.tv_sec, 0, &t);

	ssp_infof();


	/* dump file */
	snprintf(file_path, sizeof(file_path), "%s/sensorhub-%d.dump", SENSORHUB_DUMP_DIR, data->dump_index);
	ssp_infof("%s", file_path);
	snprintf(info_path, sizeof(info_path), "%s/sensorhub-info-%d.txt", SENSORHUB_DUMP_DIR, data->dump_index);

	mutex_lock(&data->file_mutex);

	old_fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(file_path, O_CREAT | O_WRONLY, 0666);
	if (IS_ERR(filp)) {
		nanohub_err("open '%s' file fail: %d\n", file_path, (int)PTR_ERR(filp));
		goto out;
	}
	ret = vfs_write(filp, buf, size, &filp->f_pos);
	if (ret < 0) {
		nanohub_err("Can't write dump to file (%d)", ret);
		filp_close(filp, NULL);
		goto out;
	}
	filp_close(filp, NULL);

	/* info file */
	if (sensorhub_info != NULL) {
		filp = filp_open(info_path, O_CREAT | O_WRONLY, 0666);
		if (IS_ERR(filp)) {
			nanohub_err("open '%s' file fail: %d\n", info_path, (int)PTR_ERR(filp));
			goto out;
		}
		ret = vfs_write(filp, sensorhub_info, sensorhub_info_size, &filp->f_pos);
		if (ret < 0)
			nanohub_err("can't write dump info to file (%d)", ret);

		filp_close(filp, NULL);
	}
	data->dump_index = (data->dump_index+1) % SENSORHUB_DUMP_MAX;

out:
	set_fs(old_fs);
	mutex_unlock(&data->file_mutex);

	ssp_nanohub_log_store(data, NANOHUB_LOG_MODE_RESET, NULL);
}

void ssp_dump_file(struct ssp_data *data, int *size)
{
	ssp_infof(" Refreshed :%d ", bSdumpRefreshed);
	if (sensorhub_dump == NULL) {
		ssp_errf("sensorhub_dump is NULL");
		return;
	}
	if (bSdumpRefreshed) {
		ssp_dump_file_store(data, sensorhub_dump, sensorhub_dump_size);
		*size = sensorhub_dump_size;
		bSdumpRefreshed = false;
	} else {
		char out_name[64] = {0,};
		ssp_nanohub_log_store(data, NANOHUB_LOG_MODE_AUTO, out_name);
	}
}

void ssp_sram_dump_store(struct ssp_data *data, int mode)
{
    char file_path[64] = {0,};
    struct file *filp;
    mm_segment_t old_fs;
    int ret;
    void *buf = get_sensorhub_sram_ptr();  // ipc_get_base(IPC_REG_DUMP);
    int size = get_sensorhub_dump_size();
    struct timespec ts;
    struct rtc_time t;

    if (!is_sensorhub_working(data)) {
		ssp_errf("sram dump fail: sensorhub is not working\n");
		return;
    }

    getnstimeofday(&ts);
    rtc_time_to_tm(ts.tv_sec, &t);

    //memcpy_fromio(sensorhub_dump, buf, size);
    nanohub_err("[SSP] %s, %d-%02d-%02d %02d:%02d:%02d.%03lu UTC\n", __func__,
		t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour,
		t.tm_min, t.tm_sec, (ts.tv_nsec)/1000000);
	nanohub_err("============================================================\n");

    /* dump file */
	if (mode == NANOHUB_LOG_MODE_BUGREPORT) {
		snprintf(file_path, sizeof(file_path), "%s/sram.dump", BUGREPORT_DIR);
	} else if (mode == NANOHUB_LOG_MODE_MANUAL) {
		snprintf(file_path, sizeof(file_path), "%s/dumpstate-sram.dump", SENSORHUB_DUMP_DIR);
	} else {
		snprintf(file_path, sizeof(file_path), "%s/sram-%d%02d%02d%02d%02d.dump", SENSORHUB_DUMP_DIR,
		t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min);
	}
    ssp_infof("%s", file_path);

	mutex_lock(&data->file_mutex);

    old_fs = get_fs();
    set_fs(get_ds());

    filp = filp_open(file_path, O_CREAT | O_WRONLY, 0666);
    if (IS_ERR(filp)) {
		ssp_errf("open '%s' file fail: %d\n", file_path, (int)PTR_ERR(filp));
		goto out;
    }
    ret = vfs_write(filp, buf, size, &filp->f_pos);
    if (ret < 0) {
		ssp_errf("Can't write dump to file (%d)", ret);
		filp_close(filp, NULL);
		goto out;
    }
    filp_close(filp, NULL);
	ssp_errf("%s - Finished (%s, %d, %d)\n", __func__, file_path, ret, size);
out:
    set_fs(old_fs);
	mutex_unlock(&data->file_mutex);
}

int ssp_dumpstate(struct ssp_data *data, char *out_name)
{
	int ret;

	ret = ssp_nanohub_log_store(data, NANOHUB_LOG_MODE_MANUAL, out_name);
	ssp_sram_dump_store(data, NANOHUB_LOG_MODE_MANUAL);

	return (ret == SUCCESS) ? SUCCESS : FAIL;
}

int ssp_bugreport(struct ssp_data *data, char *out_name)
{
	int ret;

	ret = ssp_nanohub_log_store(data, NANOHUB_LOG_MODE_BUGREPORT, out_name);
	ssp_sram_dump_store(data, NANOHUB_LOG_MODE_BUGREPORT);

	return (ret == SUCCESS) ? SUCCESS : FAIL;
}

unsigned int ssp_nanohub_log_buf_check(struct ssp_data *data)
{
	//char buffer[4] = {0x02, 0x01, 0xFC, 0xA0};
	unsigned int widx;

	widx = ssp_get_logbuf_write_index(data);

	if (widx < data->uLogWIdx && data->uLogWIdx != 0xFFFFFFFF) {
		data->is_log_full = true;
		//report_scontext_data(data, buffer, 4);
	}
	data->uLogWIdx = widx;

	return widx;
}


