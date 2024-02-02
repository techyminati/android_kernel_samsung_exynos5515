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

#include "ssp_scontext.h"
#include "ssp_comm.h"
#include "ssp_iio.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#define SCONTEXT_BIG_NOTI_EVENT               0xFB

void ssp_scontext_log(const char *func_name, const char *data, int length)
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

	//ssp_info(": %s - %s (%d)\n", func_name, log_str, length);
	nanohub_info(": %s - %s (%d)\n", func_name, log_str, length);

	kfree(log_str);
}

static ssize_t shub_big_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *battr, char *buf,
				  loff_t off, size_t size)
{
	memcpy_fromio(buf, battr->private + off, size);

	//ssp_infof("shub_big_read : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
    //    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	return size;
}

BIN_ATTR_RO(shub_big, 0);

struct bin_attribute *ssp_big_bin_attrs[] = {
	&bin_attr_shub_big,
};

int initialize_ssp_big(struct ssp_data *data)
{
	int i, ret;
	ssp_infof();

	if (data->big_events.library_length == 0)
		return -1;

	if (PTR_ERR_OR_ZERO(data->big_events.library_data)) {
		ssp_infof("memory alloc failed");
		return -1;
	}

	ssp_infof("big size %d", data->big_events.library_length);

	bin_attr_shub_big.size = data->big_events.library_length;
	bin_attr_shub_big.private = data->big_events.library_data;

	for (i = 0; i < ARRAY_SIZE(ssp_big_bin_attrs); i++) {
		struct bin_attribute *battr = ssp_big_bin_attrs[i];

		ret = device_create_bin_file(data->mcu_device, battr);
		if (ret < 0) {
			ssp_errf("Failed to create file: %s %d",
				battr->attr.name, ret);
			return -1;
		}
	}
	return 0;
}

void remove_ssp_big(struct ssp_data *data)
{
	int i;
	if (!PTR_ERR_OR_ZERO(data->big_events.library_data)) {
		kvfree(data->big_events.library_data);

		ssp_errf("%s\n", __func__);

		for (i = 0; i < ARRAY_SIZE(ssp_big_bin_attrs); i++) {
			if (ssp_big_bin_attrs[i] != NULL)
				device_remove_bin_file(data->mcu_device, ssp_big_bin_attrs[i]);
		}
	}
}

void ssp_read_big_library_task(struct work_struct *work)
{
	char *rbuf = NULL;
	int rbuf_length = 0;
	struct ssp_big *big = container_of(work, struct ssp_big, work);
	struct ssp_msg *msg = NULL;
	int buf_len, residue, ret = 0, index = 0, pos = 0;
	char buffer[5];

	ssp_infof("%s\n", __func__);

	//mutex_lock(&hub_data->big_events_lock);
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (big->data->big_events.library_data) {
		ssp_infof("!!WARNING big data didn't get from FW\n");
		ssp_scontext_log(__func__,
			big->data->big_events.library_data,
			big->data->big_events.library_length);
		remove_ssp_big(big->data);
	}

	residue = big->length;
	big->data->big_events.library_length = big->length;
	big->data->big_events.library_data = kvzalloc(big->length, GFP_KERNEL);
	if (initialize_ssp_big(big->data) == -1) {
		//remove_ssp_big(hub_data->ssp_data);
		kvfree(big->data->big_events.library_data);
		goto out;
	}

	while (residue > 0) {
		buf_len = residue > DATA_PACKET_SIZE
			? DATA_PACKET_SIZE : residue;

		msg->cmd = MSG2SSP_AP_GET_BIG_DATA;
		msg->length = buf_len;
		msg->options = AP2HUB_READ | (index++ << SSP_INDEX);
		msg->data = big->addr;
		msg->buffer = big->data->big_events.library_data + pos;
		msg->free_buffer = 0;

		ret = ssp_send_command(big->data, msg->cmd, msg->options, 1000,
						msg->buffer, msg->length, &rbuf, &rbuf_length, msg->data);

		if (ret != SUCCESS) {
			ssp_errf("read big data err(%d)", ret);
			remove_ssp_big(big->data);
			goto out;
		}
		memcpy((big->data->big_events.library_data + pos), rbuf, rbuf_length);

		pos += buf_len;
		residue -= buf_len;

		ssp_infof("read big data (%5d / %5d)", pos, big->length);
	}
	//ssp_infof("big data : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", big->data->big_events.library_data[0], big->data->big_events.library_data[1],
	//    big->data->big_events.library_data[2], big->data->big_events.library_data[3], big->data->big_events.library_data[4],
	//    big->data->big_events.library_data[5], big->data->big_events.library_data[6], big->data->big_events.library_data[7]);

	buffer[0] = SCONTEXT_BIG_NOTI_EVENT;
	buffer[1] = (big->length & 0xff000000) >> 24;
	buffer[2] = (big->length & 0x00ff0000) >> 16;
	buffer[3] = (big->length & 0x0000ff00) >> 8;
	buffer[4] = (big->length & 0x000000ff);

	report_sensorhub_data(big->data, buffer);

out:
	kfree(rbuf);
	kfree(big);
	kfree(msg);

	//mutex_unlock(&hub_data->big_events_lock);
}

static int ssp_scontext_send_cmd(struct ssp_data *data,
			const char *buf, int count)
{
	int ret = 0;
	struct timespec ts;
	struct rtc_time t;

	if (buf[2] < MSG2SSP_AP_STATUS_LCD_ON ||
	    buf[2] > MSG2SSP_AP_STATUS_POW_DISCONNECTED) {
		ssp_errf("INST_LIB_NOTI err(%d)", buf[2]);
		return -EINVAL;
	}

	ret = ssp_send_cmd(data, buf[2], 0);

    if(buf[2] == MSG2SSP_AP_STATUS_LCD_OFF) {
		ssp_timestamp_sync_cmd(data);
    }

	if (buf[2] == MSG2SSP_AP_STATUS_LCD_ON ||
		buf[2] == MSG2SSP_AP_STATUS_LCD_OFF)
		data->uLastAPState = buf[2];

	if (buf[2] == MSG2SSP_AP_STATUS_LCD_ON ||
	    buf[2] == MSG2SSP_AP_STATUS_LCD_OFF) {
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &t);
		//ssp_timestamp_sync_cmd(data);

		nanohub_err("[SSP] %d-%02d-%02d %02d:%02d:%02d.%03lu UTC (%s)\n",
			t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour,
			t.tm_min, t.tm_sec, (ts.tv_nsec)/1000000,
			(buf[2]==MSG2SSP_AP_STATUS_LCD_ON?"LCD ON":"LCD OFF"));
	}
	return ret;
}

static int ssp_scontext_send_instruction(struct ssp_data *data,
					const char *buf, int count)
{
	unsigned char instruction = buf[0];

	if (buf[0] == MSG2SSP_INST_LIBRARY_REMOVE) {
#ifdef CONFIG_SLEEP_MONITOR
		data->service_mask = data->service_mask & (~(1ULL<<buf[1]));
#endif
		instruction = REMOVE_LIBRARY;
	} else if (buf[0] == MSG2SSP_INST_LIBRARY_ADD) {
#ifdef CONFIG_SLEEP_MONITOR
		data->service_mask = data->service_mask | 1ULL<<buf[1];
#endif
		instruction = ADD_LIBRARY;
	} else if (instruction > MSG2SSP_MAX_INST_VALUE) {
		ssp_errf("Invalid INST CMD value : 0x%x !!", instruction);
		return -EINVAL;
	}

	return send_instruction(data, instruction,
		(unsigned char)buf[1], (unsigned char *)(buf+2), count-2);
}

int ssp_scontext_test_write(struct ssp_data *data, const char *buffer, int length)
{
	int ret = 0;

	if (!is_sensorhub_working(data)) {
		ssp_errf("stop sending library data(is not working)");
		return -EIO;
	}

	if (unlikely(length < 2)) {
		ssp_errf("library data length err(%d)", (int)length);
		return -EINVAL;
	}

	ssp_scontext_log(__func__, buffer, length);

	if (buffer[0] == MSG2SSP_INST_LIB_NOTI)
		ret = ssp_scontext_send_cmd(data, buffer, length);
	else
		ret = ssp_scontext_send_instruction(data, buffer, length);

	if (unlikely(ret < 0)) {
		ssp_errf("send library data err(%d)", ret);
		if (ret == ERROR)
			return -EIO;
		else if (ret == FAIL)
			return -EAGAIN;
	}

	return 0;
}

static ssize_t ssp_scontext_write(struct file *file, const char __user *buf,
				size_t count, loff_t *pos)
{
	struct ssp_data *data = container_of(file->private_data, struct ssp_data, scontext_device);
	int ret = 0;
	char *buffer = NULL;

	if (!is_sensorhub_working(data)) {
		ssp_errf("stop sending library data(is not working)");
		return -EIO;
	}

	if (unlikely(count < 2 || count > 4096)) {
		ssp_errf("library data length err(%d)", (int)count);
		return -EINVAL;
	}

	buffer = kzalloc(count * sizeof(char), GFP_KERNEL);
	if (buffer == NULL) {
		ssp_errf("memory allocation err (count: %llu)", count);
		return -ENOMEM;
	}
	
	ret = copy_from_user(buffer, buf, count);
	if (unlikely(ret)) {
		ssp_errf("memcpy for kernel buffer err");
		ret = -EFAULT;
		goto exit;
	}

	ssp_scontext_log(__func__, buffer, count);

	if (buffer[0] == MSG2SSP_INST_LIB_NOTI)
		ret = ssp_scontext_send_cmd(data, buffer, count);
	else
		ret = ssp_scontext_send_instruction(data, buffer, count);

	if (unlikely(ret < 0)) {
		ssp_errf("send library data err(%d)", ret);
		if (ret == ERROR)
			ret = -EIO;
		else if (ret == FAIL)
			ret = -EAGAIN;

		goto exit;
	}

	ret = count;

exit:
	kfree(buffer);
	return ret;
}

static struct file_operations ssp_scontext_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.write = ssp_scontext_write,
};

int ssp_scontext_initialize(struct ssp_data *data)
{
	int ret;
	ssp_dbgf("----------");

	/* register scontext misc device */
	data->scontext_device.minor = MISC_DYNAMIC_MINOR;
	data->scontext_device.name = "ssp_sensorhub";//"ssp_sensorhub_iio";
	data->scontext_device.fops = &ssp_scontext_fops;

	ret = misc_register(&data->scontext_device);
	if (ret < 0)
		ssp_errf("register scontext misc device err(%d)", ret);

	return ret;
}

void ssp_scontext_remove(struct ssp_data *data)
{
	ssp_scontext_fops.write = NULL;
	misc_deregister(&data->scontext_device);
}

MODULE_DESCRIPTION("Seamless Sensor Platform(SSP) sensorhub driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
