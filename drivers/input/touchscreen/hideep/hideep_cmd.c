/* hideep_cmd.c
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "hideep.h"

static ssize_t support_feature_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	u32 features = 0;

	features |= (1 << 0);

	input_info(true, &ts->client->dev, "%s: festures: 0x%X\n", __func__, features);

	return snprintf(buf, PAGE_SIZE, "%d", features);
}

static DEVICE_ATTR(support_feature, 0444, support_feature_show, NULL);

static struct attribute *sysfs_attributes[] = {
	&dev_attr_support_feature.attr,
	NULL,
};

static struct attribute_group sysfs_attr_group = {
	.attrs = sysfs_attributes,
};

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[64] = { 0 };
	int retval = 0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	retval = hideep_firmware_update_on_hidden_menu(ts, sec->cmd_param[0]);
	if (retval < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &ts->client->dev, "%s: failed [%d]\n", __func__, retval);
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, &ts->client->dev, "%s: success [%d]\n", __func__, retval);
	}
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "HD%02X%02X%02X%02X",
			ts->p_data->img_version_of_bin[0], ts->p_data->img_version_of_bin[1],
			ts->p_data->img_version_of_bin[2], ts->p_data->img_version_of_bin[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[16] = { 0 };
	char model[16] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = ts->hideep_api->get_vr_info(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: firmware version read error\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "HD%02X%02X%02X%02X",
		ts->p_data->img_version_of_ic[0], ts->p_data->img_version_of_ic[1],
		ts->p_data->img_version_of_ic[2], ts->p_data->img_version_of_ic[3]);
	snprintf(model, sizeof(model), "HD%02X%02X",
		ts->p_data->img_version_of_ic[0], ts->p_data->img_version_of_ic[1]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
		sec_cmd_set_cmd_result_all(sec, model, strnlen(model, sizeof(model)), "FW_MODEL");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[16] = { 0 };

	strncpy(buff, "HIDEEP", sizeof(buff));
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[16] = { 0 };

	strncpy(buff, "iST207WQ", sizeof(buff));
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[16] = { 0 };

	snprintf(buff, 16, "%d", ts->vr_info->num_tx);
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}
static void get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[16] = { 0 };

	snprintf(buff, 16, "%d", ts->vr_info->num_rx);
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void check_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[3] = { 0x00, 0x00, 0xC0 };
	u8 data[2] = { 0 };
	u16 result;
	int ret;
	int retry = 0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	disable_irq(ts->client->irq);
	
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_CONNECTION_CHECK_CMD, cmd, 3);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send command\n", __func__);
		goto err_conn_check;
	}

	msleep(100);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_conn_check;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_conn_check;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_conn_check;
	}

	result = (data[1] & 0xFF) << 8 | (data[0] & 0xFF);
	if (result != HIDEEP_TSP_CONNECTION_PASS) {
		input_err(true, &ts->client->dev, "%s: result failed(%X)\n",
				__func__, result);
		goto err_conn_check;
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;

err_conn_check:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_smart_short_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 *sdata;
	int fail = 0;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc((ts->vr_info->num_tx + ts->vr_info->num_rx) * 2, GFP_KERNEL);
	if (!data)
		goto err_smart_short_read;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x07;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_1D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_smart_short_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_smart_short_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_smart_short_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_smart_short_read;
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sdata = (s16 *)data;
	pr_info("[sec_input:hiddep]:");
	for (jj = 0; jj < ts->vr_info->num_tx; jj++) {
		pr_cont(" %d", sdata[jj]);
		if (sdata[jj])
			fail |= 0x1;
		if (!ts->info_work_done) {
			if (ts->proc_data)
				ts->proc_data[(ts->vr_info->num_tx * ts->vr_info->num_rx) +
				(ts->vr_info->num_tx + ts->vr_info->num_rx)+ jj] = sdata[jj];
		}
	}
	pr_cont("\n");
	

	snprintf(buff, sizeof(buff), "%d", fail);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SMART_SHORT_TX");
	}

	fail = 0;
	pr_info("[sec_input:hiddep]:");
	for (ii = ts->vr_info->num_tx; ii < (ts->vr_info->num_tx + ts->vr_info->num_rx); ii++) {
		pr_cont(" %d", sdata[jj]);
		if (sdata[jj])
			fail |= 0x1;
	}
	pr_cont("\n");

	snprintf(buff, sizeof(buff), "%d", fail);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SMART_SHORT_RX");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	return;

err_smart_short_read:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SMART_SHORT_TX");
	}
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_channel_short_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 *sdata;
	char temp[CMD_RESULT_WORD_LEN];

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * 4 * 2, GFP_KERNEL);
	if (!data)
		goto err_smart_short_read;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x0A;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_1D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_smart_short_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_smart_short_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_smart_short_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * 4 * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_smart_short_read;
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sdata = (s16 *)data;
	/*
	 * 0: RX-GND, 1: RX-RX, 2: TX-GND, 3: TX-TX
	 */
	for (ii = 0; ii < 4; ii++) {
		pr_cont("[sec_input]: hiddpe: %s: %s:", __func__, (ii == 0) ? "TX-GND" : (ii == 1) ? "TX-TX" : (ii == 2) ? "RX-GND" : "RX-RX");
		for (jj = 0; jj < ts->vr_info->num_tx; jj++) {
			pr_cont(" %d", sdata[ii * ts->vr_info->num_tx + jj]);

			if (ts->proc_data)
				ts->proc_data[((ts->vr_info->num_tx * ts->vr_info->num_rx) +
						(ts->vr_info->num_tx + ts->vr_info->num_rx) + ts->vr_info->num_tx) +
						ii * ts->vr_info->num_tx + jj] = sdata[ii * ts->vr_info->num_tx + jj];

			if ((ii == 3) && jj == (ts->vr_info->num_tx - 1))
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d", sdata[ii * ts->vr_info->num_tx + jj]);
			else
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", sdata[ii * ts->vr_info->num_tx + jj]);
			strlcat(buff, temp, SEC_CMD_STR_LEN);

		}
		pr_cont("\n");
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	return;

err_smart_short_read:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_lp_scan_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 data[2];
	int ret;
	int retry = 0;
	s16 max = 0;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	disable_irq(ts->client->irq);

	cmd[0] = 0x09;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_1D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_lp_scan_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_lp_scan_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_lp_scan_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_lp_scan_read;
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	memcpy(&max, data, 2);

	snprintf(buff, sizeof(buff), "%d", max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "LP_SCAN");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

err_lp_scan_read:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "LP_SCAN");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_self_raw_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 min, max;
	s16 *sdata;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc((ts->vr_info->num_tx + ts->vr_info->num_rx) * 2, GFP_KERNEL);
	if (!data)
		goto err_self_raw_read;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x02;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_1D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_self_raw_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_self_raw_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_self_raw_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_self_raw_read;
	}

	sdata = (s16 *)data;
	pr_info("[sec_input:hiddep]:");
	for (jj = 0; jj < ts->vr_info->num_tx + ts->vr_info->num_rx; jj++) {
		pr_cont(" %d", sdata[jj]);
		if (!ts->info_work_done) {
			if (ts->proc_data)
				ts->proc_data[ts->vr_info->num_tx * ts->vr_info->num_rx + jj] = sdata[jj];
		}
	}
	pr_cont("\n");
	
	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
		if (ii == 0)
			min = max = sdata[ii];
		
		min = min(min, sdata[ii]);
		max = max(max, sdata[ii]);
	}

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_X");
	}

	for (ii = ts->vr_info->num_tx; ii < (ts->vr_info->num_tx + ts->vr_info->num_rx); ii++) {
		if (ii == ts->vr_info->num_tx)
			min = max = sdata[ii];
		
		min = min(min, sdata[ii]);
		max = max(max, sdata[ii]);
	}

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_Y");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	return;

err_self_raw_read:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_X");
	}
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_self_raw_x_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char temp[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int jj;
	s16 *sdata;
	char *buff;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc((ts->vr_info->num_tx + ts->vr_info->num_rx) * 2, GFP_KERNEL);
	if (!data)
		goto err_alloc_mem_data;

	buff = kzalloc((ts->vr_info->num_tx + ts->vr_info->num_rx) * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err_alloc_mem_buff;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x02;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_1D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_self_raw_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_self_raw_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_self_raw_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_self_raw_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_tx; jj++) {
		snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", sdata[jj]);
		strncat(buff, (char *)temp, CMD_RESULT_WORD_LEN);
		memset(temp, 0x00, SEC_CMD_STR_LEN);			
	}
	
	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->vr_info->num_tx * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	kfree(buff);
	return;

err_self_raw_read:
	enable_irq(ts->client->irq);
	kfree(buff);
err_alloc_mem_buff:
	kfree(data);
err_alloc_mem_data:
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_self_raw_y_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char temp[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int jj;
	s16 *sdata;
	char *buff;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc((ts->vr_info->num_tx + ts->vr_info->num_rx) * 2, GFP_KERNEL);
	if (!data)
		goto err_alloc_mem_data;

	buff = kzalloc((ts->vr_info->num_tx + ts->vr_info->num_rx) * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err_alloc_mem_buff;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x02;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_1D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_self_raw_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_self_raw_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_self_raw_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_self_raw_read;
	}

	sdata = (s16 *)data;
	for (jj =  ts->vr_info->num_tx; jj < (ts->vr_info->num_tx + ts->vr_info->num_rx); jj++) {
		snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", sdata[jj]);
		strncat(buff, (char *)temp, CMD_RESULT_WORD_LEN);
		memset(temp, 0x00, SEC_CMD_STR_LEN);			
	}
	
	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->vr_info->num_rx * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	kfree(buff);
	return;

err_self_raw_read:
	enable_irq(ts->client->irq);
	kfree(buff);
err_alloc_mem_buff:
	kfree(data);
err_alloc_mem_data:
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_raw_multi_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 min, max;
	s16 *sdata;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_raw_multi_read;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x00;
	cmd[1] = 0x01;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 2);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_raw_multi_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_raw_multi_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_raw_multi_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_raw_multi_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		pr_info("[sec_input:hiddep]: [%d]", jj);
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			pr_cont(" %d", sdata[ii * (ts->vr_info->num_tx) + jj]);
		}
		pr_cont("\n");
	}
	
	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	for (ii = 0; ii < (ts->vr_info->num_rx * ts->vr_info->num_tx); ii++) {
		if(ii == 0)
			min = max = sdata[ii];
		
		min = min(min, sdata[ii]);
		max = max(max, sdata[ii]);
	}

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_MULTI");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;

err_raw_multi_read:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_MULTI");
	}
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_raw_multi_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char temp[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 *sdata;
	char *buff;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_alloc_mem_data;

	buff = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err_alloc_mem_buff;

	disable_irq(ts->client->irq);

	cmd[0] = 0x00;
	cmd[1] = 0x01;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 2);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_raw_multi_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_raw_multi_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_raw_multi_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_raw_multi_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", sdata[ii * (ts->vr_info->num_tx) + jj]);
			strncat(buff, (char *)temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);			
		}
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);


	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	kfree(buff);
	return;

err_raw_multi_read:
	enable_irq(ts->client->irq);
	kfree(buff);
err_alloc_mem_buff:
	kfree(data);
err_alloc_mem_data:
	
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_rawdata_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	u8 header[8];
	int ret;
	int ii, jj;
	int size;
	s16 *sdata;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 4, GFP_KERNEL);
	if (!data)
		goto err_fix_active_cmd;

	disable_irq(ts->client->irq);

	cmd[0] = 0x01;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_FIX_ACTIVE_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to set active mode\n", __func__);
		goto err_fix_active_cmd;
	}

	msleep(20);

	cmd[0] = 0x00;
	cmd[1] = 0x12;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_OPMODE_CMD, cmd, 2);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_rawdata_read;
	}

	cmd[0] = 0x5A;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_SYNC_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send sync command\n", __func__);
		goto err_rawdata_read;
	}

	msleep(20);

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_HEADER_INFO, header, 8);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read header info\n", __func__);
		goto err_rawdata_read;
	}
	input_info(true, &ts->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
			header[0], header[1], header[2], header[3], header[4], header[5], header[6], header[7]);

	size = GET_PAYLOAD_SIZE_FROM_HEADER(header);

	if ((size == 0) || (size > ts->vr_info->num_tx * ts->vr_info->num_rx * 4))
		goto err_rawdata_read;

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, size);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_rawdata_read;
	}

	cmd[0] = 0x00;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_FIX_ACTIVE_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to unset active mode\n", __func__);
		goto err_rawdata_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		pr_info("[sec_input:hiddep]: [%d]", jj);
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			pr_cont(" %d", sdata[jj * ts->vr_info->num_tx + ii]);
		}
		pr_cont("\n");
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;

err_rawdata_read:
	cmd[0] = 0x00;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_FIX_ACTIVE_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to unset active mode\n", __func__);
	}
	kfree(data);
err_fix_active_cmd:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_raw_delta_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	u8 header[8];
	int ret;
	int ii, jj;
	int size;
	s16 *sdata;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 4, GFP_KERNEL);
	if (!data)
		goto err_fix_active_cmd;

	disable_irq(ts->client->irq);

	cmd[0] = 0x01;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_FIX_ACTIVE_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to set active mode\n", __func__);
		goto err_fix_active_cmd;
	}

	msleep(20);

	cmd[0] = 0x00;
	cmd[1] = 0x15;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_OPMODE_CMD, cmd, 2);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_raw_delta_read;
	}

	cmd[0] = 0x5A;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_SYNC_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send sync command\n", __func__);
		goto err_raw_delta_read;
	}

	msleep(20);

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_HEADER_INFO, header, 8);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read header info\n", __func__);
		goto err_raw_delta_read;
	}
	input_info(true, &ts->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
			header[0], header[1], header[2], header[3], header[4], header[5], header[6], header[7]);

	size = GET_PAYLOAD_SIZE_FROM_HEADER(header);

	if ((size == 0) || (size > ts->vr_info->num_tx * ts->vr_info->num_rx * 4))
		goto err_raw_delta_read;

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, size);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_raw_delta_read;
	}

	cmd[0] = 0x00;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_FIX_ACTIVE_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to unset active mode\n", __func__);
		goto err_raw_delta_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		pr_info("[sec_input:hiddep]: [%d]", jj);
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			pr_cont(" %d", sdata[jj * ts->vr_info->num_tx + ii]);
		}
		pr_cont("\n");
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;

err_raw_delta_read:
	cmd[0] = 0x00;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_FIX_ACTIVE_CMD, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to unset active mode\n", __func__);
	}
	kfree(data);
err_fix_active_cmd:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_1rc_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 min, max;
	s16 *sdata;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_1rc_read;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x03;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_1rc_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_1rc_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_1rc_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_1rc_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		pr_info("[sec_input:hiddep]: [%d]", jj);
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			pr_cont(" %d", sdata[ii * (ts->vr_info->num_tx) + jj]);
			if (!ts->info_work_done) {
				if (ts->proc_data)
					ts->proc_data[jj * ts->vr_info->num_tx + ii] = sdata[ii * ts->vr_info->num_tx + jj];
			}
		}
		pr_cont("\n");
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	for (ii = 0; ii < (ts->vr_info->num_rx * ts->vr_info->num_tx); ii++) {
		if(ii == 0)
			min = max = sdata[ii];
		
		min = min(min, sdata[ii]);
		max = max(max, sdata[ii]);
	}

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "1RC_RAW_DATA");
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	return;

err_1rc_read:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "1RC_RAW_DATA");
	}
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_1rc_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char temp[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 *sdata;
	char *buff;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_alloc_mem_data;

	buff = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err_alloc_mem_buff;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x03;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_1rc_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_1rc_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_1rc_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_1rc_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", sdata[ii * (ts->vr_info->num_tx) + jj]);
			strncat(buff, (char *)temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);			
		}
	}
	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	kfree(buff);
	return;

err_1rc_read:
	enable_irq(ts->client->irq);
	kfree(buff);
err_alloc_mem_buff:
	kfree(data);
err_alloc_mem_data:
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_1rc_gap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char temp[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 *sdata;
	s16 *gap_buf;
	char *buff;
	int cur, right, down;
	int gap1, gap2;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_alloc_mem_data;

	gap_buf = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!gap_buf)
		goto err_alloc_gap_buf;

	buff = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err_alloc_mem_buff;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x03;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_1rc_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_1rc_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_1rc_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_1rc_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			cur = sdata[ii * (ts->vr_info->num_tx) + jj];

			if (jj == ts->vr_info->num_rx-1) {
				gap1 = 0;
			} else {			
				right = sdata[ii * (ts->vr_info->num_tx) + jj + 1];
				gap1 = (abs(cur - right) * 100) / max(cur, right);
			}
			
			if (ii == ts->vr_info->num_tx-1) {
				gap2 = 0;
			} else {
				down = sdata[(ii + 1) * (ts->vr_info->num_tx) + jj];
				gap2 = (abs(cur - down) * 100) / max(cur, down);
			}

			gap_buf[ii * (ts->vr_info->num_tx) + jj] = max(gap1, gap2);

			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", gap_buf[ii * (ts->vr_info->num_tx) + jj]);
			strncat(buff, (char *)temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);	
		}		
	}

	/* for debugging */
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		pr_info("[sec_input:hiddep]: [%d]", jj);
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			pr_cont(" %d", gap_buf[ii * (ts->vr_info->num_tx) + jj]);
		}
		pr_cont("\n");
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	kfree(gap_buf);
	kfree(buff);
	return;

err_1rc_read:
	enable_irq(ts->client->irq);
	kfree(buff);
err_alloc_mem_buff:
	kfree(gap_buf);
err_alloc_gap_buf:
	kfree(data);	
err_alloc_mem_data:
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_eop_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 min, max;
	s16 *sdata;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_eop_read;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x04;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_eop_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_eop_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_eop_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_eop_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		pr_info("[sec_input:hiddep]: [%d]", jj);
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			pr_cont(" %d", sdata[ii * (ts->vr_info->num_tx) + jj]);
		}
		pr_cont("\n");
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);


	for (ii = 0; ii < (ts->vr_info->num_rx * ts->vr_info->num_tx); ii++) {
		if(ii == 0)
			min = max = sdata[ii];
		
		min = min(min, sdata[ii]);
		max = max(max, sdata[ii]);
	}

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "EOP_RAW_DATA");
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	return;

err_eop_read:
	enable_irq(ts->client->irq);
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "EOP_RAW_DATA");
	}
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(data);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_eop_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char temp[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 *sdata;
	char *buff;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_alloc_mem_data;

	buff = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err_alloc_mem_buff;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x04;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_eop_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_eop_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_eop_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_eop_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", sdata[ii * (ts->vr_info->num_tx) + jj]);
			strncat(buff, (char *)temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);			
		}
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	kfree(buff);
	return;

err_eop_read:
	enable_irq(ts->client->irq);
	kfree(buff);
err_alloc_mem_buff:
	kfree(data);
err_alloc_mem_data:
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_eop_gap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char temp[SEC_CMD_STR_LEN] = {0};
	u8 cmd[2];
	u8 *data;
	int ret;
	int retry = 0;
	int ii, jj;
	s16 *gap_buf;
	s16 *sdata;
	char *buff;
	int cur, right, down;
	int gap1, gap2;

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * 2, GFP_KERNEL);
	if (!data)
		goto err_alloc_mem_data;

	gap_buf = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!gap_buf)
		goto err_alloc_gap_buf;

	buff = kzalloc(ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto err_alloc_mem_buff;

	disable_irq(ts->client->irq);
	
	cmd[0] = 0x04;
	ret = ts->hideep_api->i2c_write(ts, HIDEEP_2D_TEST_COMMAND, cmd, 1);
	if (ret < 0) {
		input_info(true, &ts->client->dev, "%s: failed to send opmode command\n", __func__);
		goto err_eop_read;
	}

	msleep(20);

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_READ_DATA_READY_CHECK, data, 1);
		if (ret < 0)
			goto err_eop_read;

		if (data[0] == HIDEEP_DATA_READY_RETURN) {
			input_info(true, &ts->client->dev, "%s: data ready(%x)\n", __func__, data[0]);
			break;
		}

		input_info(true, &ts->client->dev, "%s: data ready failed(%d)\n", __func__, retry);
		msleep(20);
		retry++;
	};

	if (retry > SEC_TS_WAIT_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
		goto err_eop_read;
	}

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_CMD_CONNECTION_RESULT_REG, data, ts->vr_info->num_tx * ts->vr_info->num_rx * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: connection result read failed\n", __func__);
		goto err_eop_read;
	}

	sdata = (s16 *)data;
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			cur = sdata[ii * (ts->vr_info->num_tx) + jj];

			if (jj == ts->vr_info->num_rx-1) {
				gap1 = 0;
			} else {			
				right = sdata[ii * (ts->vr_info->num_tx) + jj + 1];
				gap1 = (abs(cur - right) * 100) / max(cur, right);
			}
			
			if (ii == ts->vr_info->num_tx-1) {
				gap2 = 0;
			} else {
				down = sdata[(ii + 1) * (ts->vr_info->num_tx) + jj];
				gap2 = (abs(cur - down) * 100) / max(cur, down);
			}

			gap_buf[ii * (ts->vr_info->num_tx) + jj] = max(gap1, gap2);

			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", gap_buf[ii * (ts->vr_info->num_tx) + jj]);
			strncat(buff, (char *)temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);	
		}		
	}

	/* for debugging */
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		pr_info("[sec_input:hiddep]: [%d]", jj);
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			pr_cont(" %d", gap_buf[ii * (ts->vr_info->num_tx) + jj]);
		}
		pr_cont("\n");
	}

	enable_irq(ts->client->irq);

	ts->hideep_api->reset_ic(ts);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->vr_info->num_tx * ts->vr_info->num_rx * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(data);
	kfree(gap_buf);
	kfree(buff);
	return;

err_eop_read:
	enable_irq(ts->client->irq);
	kfree(buff);
err_alloc_mem_buff:
	kfree(gap_buf);
err_alloc_gap_buf:
	kfree(data);	
err_alloc_mem_data:
	ts->hideep_api->reset_ic(ts);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);

	input_info(true, &ts->client->dev, "%s\n", __func__);
	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);
/*
	if (ts->tsp_dump_lock == 1) {
		input_err(true, &ts->client->dev, "%s: already checking now\n", __func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}
*/
	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: IC is power off\n", __func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);
	run_smart_short_read(sec);
	run_lp_scan_read(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, &ts->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void run_rawdata_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	run_1rc_read(sec);
	run_self_raw_read(sec);
	run_smart_short_read(sec);

	if (ts->p_data->model_num >= 5)
		run_channel_short_read_all(sec);

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s\n", __func__);

}

void hideep_run_rawdata_read_all(struct hideep_t *ts)
{
	run_rawdata_read_all(&ts->sec);
}

static void fix_active_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		ts->fix_active_mode = sec->cmd_param[0];
		ret = hideep_fix_active_mode(ts, ts->fix_active_mode);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed, retval = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s cmd_param: %d\n", __func__, buff, sec->cmd_param[0]);
}

static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		ts->glove_mode = sec->cmd_param[0];
		ret = ts->hideep_api->i2c_write(ts, HIDEEP_GLOVE_MODE_CMD, &ts->glove_mode, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed, retval = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s cmd_param: %d\n", __func__, buff, sec->cmd_param[0]);
}

#ifndef CONFIG_SEC_FACTORY
static void aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec_cmd_set_cmd_exit(sec);
		return;
	}

	ts->aot_enable = !!sec->cmd_param[0];

	mutex_lock(&ts->dev_mutex);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF && ts->aot_enable == 1) {
		hideep_start_device(ts);
		ts->dev_state = state_sleep;
		hideep_set_lowpowermode(ts, TO_LOWPOWER_MODE);
	} else if (ts->power_status == SEC_INPUT_STATE_LPM && ts->aot_enable == 0) {
		ts->dev_state = state_sleep;
		hideep_stop_device(ts);
	}

	mutex_unlock(&ts->dev_mutex);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	return;
}
#endif

static void bezel_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		ts->bezel_enable = sec->cmd_param[0];
		ret = ts->hideep_api->i2c_write(ts, HIDEEP_BEZEL_ENABLE_CMD, &ts->bezel_enable, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed, retval = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	char data = 0;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		data = sec->cmd_param[0];

		ret = ts->hideep_api->i2c_write(ts, HIDEEP_DEAD_ZONE_CMD, &data, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed, retval = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s cmd_param: %d\n", __func__, buff, sec->cmd_param[0]);
}

static void wireless_charger_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 data[2] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		ts->wireless_charger_mode = sec->cmd_param[0];
		data[0] = sec->cmd_param[0];
		data[1] = 0x01;

		ret = ts->hideep_api->i2c_write(ts, HIDEEP_CHARGER_MODE_CMD, data, 2);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed, retval = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s cmd_param: %d\n", __func__, buff, sec->cmd_param[0]);
}

static void charger_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 data[2] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		ts->charger_mode = sec->cmd_param[0];
		data[0] = sec->cmd_param[0];
		data[1] = 0x00;

		ret = ts->hideep_api->i2c_write(ts, HIDEEP_CHARGER_MODE_CMD, data, 2);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed, retval = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s cmd_param: %d\n", __func__, buff, sec->cmd_param[0]);
}

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct hideep_t *ts = container_of(sec, struct hideep_t, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	ts->debug_flag = sec->cmd_param[0];

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "NA");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);
}

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("check_connection", check_connection),},
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},

	{SEC_CMD("run_smart_short_read", run_smart_short_read),},
	{SEC_CMD("run_channel_short_read_all", run_channel_short_read_all),},
	{SEC_CMD("run_lp_scan_read", run_lp_scan_read),},
	{SEC_CMD("run_self_raw_read", run_self_raw_read),},
	{SEC_CMD("run_self_raw_x_read_all", run_self_raw_x_read_all),},
	{SEC_CMD("run_self_raw_y_read_all", run_self_raw_y_read_all),},

	{SEC_CMD("run_raw_multi_read", run_raw_multi_read),},
	{SEC_CMD("run_raw_multi_read_all", run_raw_multi_read_all),},
	{SEC_CMD("run_rawdata_read", run_rawdata_read),},
	{SEC_CMD("run_raw_delta_read", run_raw_delta_read),},
	{SEC_CMD("run_1rc_read", run_1rc_read),},	// channel open
	{SEC_CMD("run_1rc_read_all", run_1rc_read_all),},
	{SEC_CMD("run_1rc_gap_read_all", run_1rc_gap_read_all),},
	{SEC_CMD("run_eop_read", run_eop_read),},	// rx/rx channel short
	{SEC_CMD("run_eop_read_all", run_eop_read_all),},
	{SEC_CMD("run_eop_gap_read_all", run_eop_gap_read_all),},
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD_H("fix_active_mode", fix_active_mode),},
	{SEC_CMD_H("glove_mode", glove_mode),},
#ifndef CONFIG_SEC_FACTORY
	{SEC_CMD_H("aot_enable", aot_enable),},
#endif
	{SEC_CMD_H("bezel_enable", bezel_enable),},
	{SEC_CMD_H("dead_zone_enable", dead_zone_enable),},
	{SEC_CMD_H("wireless_charger_mode", wireless_charger_mode),},
	{SEC_CMD_H("charger_mode", charger_mode),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

int sec_ts_fn_init(struct hideep_t *ts)
{
	int retval = 0;

	retval = sec_cmd_init(&ts->sec, sec_cmds,
			ARRAY_SIZE(sec_cmds), SEC_CLASS_DEVT_TSP);
	if (retval < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to sec_cmd_init\n", __func__);
		goto exit;
	}

	retval = sysfs_create_group(&ts->sec.fac_dev->kobj,
			&sysfs_attr_group);
	if (retval < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to create sysfs attributes\n", __func__);
		goto exit;
	}

	retval = sysfs_create_link(&ts->sec.fac_dev->kobj,
			&ts->input_dev->dev.kobj, "input");
	if (retval < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to create input symbolic link\n",
				__func__);
		goto exit;
	}

	return 0;

exit:
	return retval;
}

void sec_ts_fn_remove(struct hideep_t *ts)
{
	input_err(true, &ts->client->dev, "%s\n", __func__);

	sysfs_delete_link(&ts->sec.fac_dev->kobj, &ts->input_dev->dev.kobj, "input");

	sysfs_remove_group(&ts->sec.fac_dev->kobj,
			&sysfs_attr_group);

	sec_cmd_exit(&ts->sec, SEC_CLASS_DEVT_TSP);

}
