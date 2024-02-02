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

#ifndef __SSP_PLATFORM_H__
#define __SSP_PLATFORM_H__


#include <linux/kernel.h>

#undef CONFIG_SSP_SLEEP

struct ssp_data;

void *ssp_device_probe(struct device *);

#ifdef CONFIG_SSP_SLEEP
void ssp_device_remove(void *ssp_data);
#endif
void ssp_device_suspend(void *ssp_data, struct device *dev);
void ssp_device_resume(void *ssp_data, struct device *dev);

void ssp_platform_init(void *ssp_data, void *);
void ssp_handle_recv_packet(void *ssp_data, char *packet, int packet_len);
void ssp_platform_start_refrsh_task(void *ssp_data);

int sensorhub_power_on(void *ssp_data);
int sensorhub_reset(void *ssp_data);
int sensorhub_shutdown(void *ssp_data);
int sensorhub_firmware_download(void *ssp_data);

int sensorhub_comms_read(void *ssp_data, u8 *buf, int length, int timeout);
int sensorhub_comms_write(void *ssp_data, u8 *buf, int length, int timeout);

int get_sensorhub_dump_size(void);
void *get_sensorhub_sram_ptr(void);
void ssp_dump_write_file(void *ssp_data, int sec_time, int reason, void *sram_buf, int sram_size, u32 *gpr, int num_gpr);
int sensorhub_dump_get_num_dumptype(void *ssp_data);
void sensorhub_dump_get_filename(void *ssp_data, int index, char *file_name, char *file_type);

bool is_sensorhub_working(void *ssp_data);
int ssp_download_firmware(void *ssp_data, struct device *dev, void *addr);
void ssp_set_firmware_type(void *ssp_data, int type);
int ssp_get_firmware_type(void *ssp_data);
void ssp_chub_bin_logbuf_dram_read(void *ssp_data, char *buf);
char *ssp_get_chub_bin_logbuf_ptr(void *ssp_data);
unsigned int ssp_get_chub_bin_logbuf_size(void *ssp_data);
unsigned int ssp_get_logbuf_write_index(void *ssp_data);
void ssp_device_enable_ap_wakeup(void *ssp_data, struct device *dev);
int ssp_check_hrm_int(void *ssp_data, struct device *dev);
#endif /* __SSP_PLATFORM_H__ */
