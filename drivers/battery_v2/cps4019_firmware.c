/*
 *  cps4019_firmware.c
 *  Samsung CPS4019 Firmware Driver
 *
 *  Copyright (C) 2021 Samsung Electronics
 * Yeongmi Ha <yeongmi86.ha@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/regmap.h>
#include <linux/pm_wakeup.h>
#include <linux/workqueue.h>

#include "include/sb_wrl_fw.h"
#include "include/charger/cps4019_firmware.h"

#include "core/sb_notify.h"
#include "include/sec_battery.h"

#define fw_log(str, ...) pr_info("[CPS4019-FW]:%s: "str, __func__, ##__VA_ARGS__)

#define FW_MODULE_NAME		"cps4019-fw"

struct sb_wrl_fw {
	struct i2c_client 		*parent;
	struct regmap 			*rm;
	struct wakeup_source	*ws;
	struct delayed_work		work;
	struct mutex			mlock;
	cb_fw_result			func;

	/* status */
	int state;
	int mode;
};

static int i2c_read_u16(struct i2c_client *client, unsigned short reg, unsigned short *value)
{
	int ret;
	struct i2c_msg msg[2];
	u8 wbuf[2];
	u8 rbuf[2];

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = rbuf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0) 
		*value = (rbuf[1] << 8) | rbuf[0];

	return ret;
}

static int regmap_write_u32(struct regmap *rm, unsigned int reg, unsigned int value)
{
	return regmap_bulk_write(rm, reg, &value, 4);
}

static int regmap_read_u32(struct regmap *rm, unsigned int reg, unsigned int *value)
{
	unsigned char buf[4];
	int ret = 0;

	ret = regmap_bulk_read(rm, reg, buf, 4);
	if (!ret)
		*value = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

	return ret;
}

/* big & little endian convert */
static int ble_convert(int data)
{
    char  *p;
    char  tmp[4];
    
    p = (char *)(&data);
    tmp[0] = p[3];
    tmp[1] = p[2];
    tmp[2] = p[1];
    tmp[3] = p[0];
    return *(int *)(tmp);
}

#define CMD_RUNNING		0x66
#define CMD_PASS		0x55
#define CMD_FAIL		0xAA
#define CMD_ILLEGAL		0x40
static int cmd_wait(struct regmap *rm)
{
	int ret = 0 , cnt = 0, value = CMD_RUNNING;

	while ((value == CMD_RUNNING) &&
			(cnt <= 3000)) {
		usleep_range(5000, 6000);

		ret = regmap_read_u32(rm, 0x200005F8, &value);
		if (ret < 0) {
			cnt += 1000;
		} else {
			cnt++;
			value = ble_convert(value);
		}
	}

	fw_log("ret = %d, cnt = %d, value = 0x%x\n", ret, cnt, value);
	return value;
}

static int get_fw_data(const char *path, struct sb_wrl_fw_data *fw_data)
{
	mm_segment_t old_fs;
	struct file *fp;
	long nread;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(path, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		goto err_open;
	}

	fw_data->size = fp->f_path.dentry->d_inode->i_size;
	if (fw_data->size <= 0) {
		ret = -ENODEV;
		goto err_open;
	}

	fw_data->data = kmalloc(fw_data->size, GFP_KERNEL);
	if (!fw_data->data) {
		ret = -ENOMEM;
		goto err_open;
	}

	nread = vfs_read(fp, (char __user *)fw_data->data, fw_data->size, &fp->f_pos);
	if (nread != fw_data->size) {
		ret = -EIO;
		goto err_read;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);
	
	return 0;

err_read:
	kfree(fw_data->data);
	fw_data->data = NULL;
err_open:
	fw_data->size = 0;
	set_fs(old_fs);
	return ret;
}

static void init_fw_data(struct sb_wrl_fw_data *fw_data, int mode)
{
	if (mode == SB_WRL_RX_SDCARD_MODE)
		kfree(fw_data->data);

	fw_data->data = NULL;
	fw_data->size = 0;
}

#define SKIP_APP1_CRC	0x1
static int run_fw(struct sb_wrl_fw *fw, struct sb_wrl_fw_data *app_bl, struct sb_wrl_fw_data *app, int skip_flag)
{
	int ret;

	fw_log("Step1, load to sram\n");
	ret = regmap_write_u32(fw->rm, 0xFFFFFF00, ble_convert(0x0E000000));
	fw_log("enable 32bit i2c (%d)\n", ret);
	ret = regmap_write_u32(fw->rm, 0x40040070, ble_convert(0x0000A061));
	fw_log("write password (%d)\n", ret);
	ret = regmap_write_u32(fw->rm, 0x40040004, ble_convert(0x00000008));
	fw_log("reset and halt mcu (%d)\n", ret);

	ret = regmap_bulk_write(fw->rm, 0x20000000, app_bl->data, app_bl->size);
	fw_log("loaded sram hex (%d)\n", ret);

	ret = regmap_write_u32(fw->rm, 0x40040010, ble_convert(0x00000001));
	fw_log("triming load function is disabled (%d)\n", ret);
	ret = regmap_write_u32(fw->rm, 0x40040004, ble_convert(0x00000066));
	fw_log("enable remap function and reset the mcu (%d)\n", ret);

	msleep(10);

	ret = regmap_write_u32(fw->rm, 0xFFFFFF00, ble_convert(0x0E000000));
	fw_log("enable 32bit i2c (%d)\n", ret);

	msleep(10);

	fw_log("Step2, bootloader crc check\n");
	ret = regmap_write_u32(fw->rm, 0x200005FC, ble_convert(0x000000B0));
	fw_log("cmd crc_test (%d)\n", ret);
	ret = cmd_wait(fw->rm);
	if (ret != CMD_PASS) {
		fw_log("bootloader crc fail\n");
		return SB_WRL_FW_RESULT_FAIL;
	}

	fw_log("Step3, app1 crc check\n");
	if (skip_flag & SKIP_APP1_CRC) {
		fw_log("Step3, skip\n");
	} else {
		ret = regmap_write_u32(fw->rm, 0x200005FC, ble_convert(0x000000A0));
		fw_log("cmd crc_app1 (%d)\n", ret);
		ret = cmd_wait(fw->rm);
		if (ret != CMD_PASS) {
			fw_log("app1 crc fail\n");
			return SB_WRL_FW_RESULT_FAIL;
		}
	}

{
	int i, cfg_buf_size = 64;
	int *p;
	
	fw_log("Step4, load firmware to MTP\n");
	p = (int *)app->data;

	for (i = 0; i < (app->size / 4); i++)
		p[i] = ble_convert(p[i]);

	ret = regmap_write_u32(fw->rm, 0x200005F4, ble_convert(cfg_buf_size));
	fw_log("write buf size (%d)\n", ret);

	ret = regmap_write_u32(fw->rm, 0x200005FC, ble_convert(0x00000060));
	fw_log("cmd eraser_0 (%d)\n", ret);
	ret = cmd_wait(fw->rm);
	if (ret != CMD_PASS) {
		fw_log("erase fail\n");
		return SB_WRL_FW_RESULT_FAIL;
	}

	for (i = 0; i < (app->size / 4 / cfg_buf_size); i++) {
		unsigned int reg = 0x20000600, cmd_value = 0x00000010;

		if (i & 0x1) {
			reg = 0x20000700;
			cmd_value = 0x00000020;
		}

		ret = regmap_bulk_write(fw->rm, reg, p, cfg_buf_size * 4);
		fw_log("bulk write - idx = %d, reg = 0x%x, reg = %d\n", i, reg, ret);
		p = p + cfg_buf_size;

		ret = cmd_wait(fw->rm);
		if (ret != CMD_PASS) {
			fw_log("write %s data to MTP fail\n", (i & 0x1) ? "buffer 1" : "buffer 0");
			return SB_WRL_FW_RESULT_FAIL;
		}

		ret = regmap_write_u32(fw->rm, 0x200005FC, ble_convert(cmd_value));
		fw_log("cmd buffer (%d)\n", ret);
	}

	ret = cmd_wait(fw->rm);
	if (ret != CMD_PASS) {
		fw_log("write %s data to MTP fail\n", (i & 0x1) ? "buffer 1" : "buffer 0");
		return SB_WRL_FW_RESULT_FAIL;
	}
}

	fw_log("Step5, check app CRC\n");
	ret = regmap_write_u32(fw->rm, 0x200005FC, ble_convert(0x00000090));
	fw_log("cmd crc_app (%d)\n", ret);
	ret = cmd_wait(fw->rm);
	if (ret != CMD_PASS) {
		fw_log("app crc fail\n");
		return SB_WRL_FW_RESULT_FAIL;
	}

	fw_log("Step6, write mcu start flag\n");
	ret = regmap_write_u32(fw->rm, 0x200005FC, ble_convert(0x00000080));
	fw_log("cmd wr_flag (%d)\n", ret);
	ret = cmd_wait(fw->rm);
	if (ret != CMD_PASS) {
		fw_log("write mcu start flag fail\n");
		return SB_WRL_FW_RESULT_FAIL;
	}

	ret = regmap_write_u32(fw->rm, 0x40040004, ble_convert(0x00000001));
	fw_log("reset all system (%d)\n", ret);

	msleep(1000);

{
	u16 chip_id = 0;

	fw_log("Step7, check chip id\n");
	ret = i2c_read_u16(fw->parent, 0x0000, &chip_id);
	fw_log("read chip_id (ret = %d, id = 0x%x)\n", ret, chip_id);
	if (chip_id != 0x4019) {
		fw_log("check chip_id fail\n");
		return SB_WRL_FW_RESULT_FAIL;
	}
}
	return SB_WRL_FW_RESULT_PASS;
}

static int load_fw_by_bin(struct sb_wrl_fw_data *app_bl, struct sb_wrl_fw_data *app)
{
	app_bl->data = kmalloc(CPS4019_APP2_BL_SIZE, GFP_KERNEL);
	if (!app_bl->data) {
		fw_log("failed to alloc app2 bl\n");
		return -ENOMEM;
	}
	app_bl->size = CPS4019_APP2_BL_SIZE;
	memcpy(app_bl->data, CPS4019_APP2_BL, CPS4019_APP2_BL_SIZE);

	app->data = kmalloc(CPS4019_APP2_SIZE, GFP_KERNEL);
	if (!app->data) {
		fw_log("failed to alloc app2\n");
		return -ENOMEM;
	}
	app->size = CPS4019_APP2_SIZE;
	memcpy(app->data, CPS4019_APP2, CPS4019_APP2_SIZE);

	return 0;
}

static int load_fw_by_sdcard(const char *app_bl_path, struct sb_wrl_fw_data *app_bl,
									const char *app_path, struct sb_wrl_fw_data *app)
{
	int ret = 0;

	init_fw_data(app_bl, SB_WRL_RX_BUILT_IN_MODE);
	init_fw_data(app, SB_WRL_RX_BUILT_IN_MODE);

	ret = get_fw_data(app_bl_path, app_bl);
	if (ret < 0) {
		fw_log("failed to read firmware (%s, ret = %d)\n", app_bl_path, ret);
		return ret;
	}

	ret = get_fw_data(app_path, app);
	if (ret < 0) {
		fw_log("failed to read firmware (%s, ret = %d)\n", app_path, ret);
		return ret;
	}

	return 0;
}

static void set_misc_event(bool state)
{
	struct sbn_bit_event misc;

	misc.value = (state) ? BATT_MISC_EVENT_WIRELESS_FW_UPDATE : 0;
	misc.mask = BATT_MISC_EVENT_WIRELESS_FW_UPDATE;

	sb_notify_call(SB_NOTIFY_EVENT_MISC, (sb_data *)&misc);
}

static void cb_work(struct work_struct *work)
{
	struct sb_wrl_fw *fw = container_of(work, struct sb_wrl_fw, work.work);
	struct sb_wrl_fw_data app_bl, app;
	int state = SB_WRL_FW_RESULT_FAIL;

	__pm_stay_awake(fw->ws);
	fw_log("start\n");

	mutex_lock(&fw->mlock);
	fw->state = SB_WRL_FW_RESULT_RUNNING;
	set_misc_event(true);
	mutex_unlock(&fw->mlock);

	fw_log("run fw - mode = %d\n", fw->mode);
	switch (fw->mode) {
	case SB_WRL_RX_BUILT_IN_MODE:
		/* try to update app2 only */
		if (!load_fw_by_bin(&app_bl, &app))
			state = run_fw(fw, &app_bl, &app, 0);

		init_fw_data(&app_bl, SB_WRL_RX_SDCARD_MODE);
		init_fw_data(&app, SB_WRL_RX_SDCARD_MODE);
		break;
	case SB_WRL_RX_SDCARD_MODE:
		/* try to update app1 */
		if (!load_fw_by_sdcard(CPS4019_APP1_BL_PATH, &app_bl, CPS4019_APP1_PATH, &app))
			state = run_fw(fw, &app_bl, &app, SKIP_APP1_CRC);
		init_fw_data(&app_bl, SB_WRL_RX_SDCARD_MODE);
		init_fw_data(&app, SB_WRL_RX_SDCARD_MODE);

		/* try to update app2 */
		if (!load_fw_by_sdcard(CPS4019_APP2_BL_PATH, &app_bl, CPS4019_APP2_PATH, &app))
			state = run_fw(fw, &app_bl, &app, 0);
		init_fw_data(&app_bl, SB_WRL_RX_SDCARD_MODE);
		init_fw_data(&app, SB_WRL_RX_SDCARD_MODE);
		break;
	default:
		state = SB_WRL_FW_RESULT_PASS;
		break;
	}
	fw_log("%s firmware update\n",
		(state == SB_WRL_FW_RESULT_PASS) ? "success" : "fail");

	if (fw->func)
		fw->func(fw->parent, fw->state);

	mutex_lock(&fw->mlock);
	fw->state = state;
	set_misc_event(false);
	mutex_unlock(&fw->mlock);

	__pm_relax(fw->ws);
}

static const struct regmap_config rm_cfg = {
    .reg_bits = 32,
    .val_bits = 8,
};

struct sb_wrl_fw *sb_wrl_fw_register(struct i2c_client *i2c, cb_fw_result func)
{
	struct sb_wrl_fw *fw = NULL;
	struct regmap *rm = NULL;

	if (IS_ERR_OR_NULL(i2c))
		return ERR_PTR(-EINVAL);

	rm = devm_regmap_init_i2c(i2c, &rm_cfg);
	if (IS_ERR(rm))
		return ERR_PTR(PTR_ERR(rm));

	fw = kzalloc(sizeof(struct sb_wrl_fw), GFP_KERNEL);
	if (!fw)
		return ERR_PTR(-ENOMEM);

	fw->parent = i2c;
	fw->func = func;
	fw->rm = rm;
	fw->ws = wakeup_source_register(&i2c->dev, FW_MODULE_NAME);
	INIT_DELAYED_WORK(&fw->work, cb_work);
	mutex_init(&fw->mlock);

	fw->state = SB_WRL_FW_RESULT_INIT;
	fw->mode = SB_WRL_RX_BUILT_IN_MODE;

	fw_log("finished!!\n");
	return fw;
}

int sb_wrl_fw_get_version(struct sb_wrl_fw *fw, int *version)
{
	if (IS_ERR_OR_NULL(fw) || (version == NULL))
		return -EINVAL;

	*version = CPS4019_APP2_VERSION;
	return 0;
}

int sb_wrl_fw_get_state(struct sb_wrl_fw *fw, int *state)
{
	if (IS_ERR_OR_NULL(fw) || (state == NULL))
		return -EINVAL;

	mutex_lock(&fw->mlock);
	*state = fw->state;
	mutex_unlock(&fw->mlock);
	fw_log("state = %s\n", get_fw_result_str(fw->state));
	return 0;
}

int sb_wrl_fw_update(struct sb_wrl_fw *fw, int mode)
{
	unsigned int work_state;

	if (IS_ERR_OR_NULL(fw))
		return -EINVAL;

	if ((mode != SB_WRL_RX_SDCARD_MODE) &&
		(mode != SB_WRL_RX_BUILT_IN_MODE))
		return -EINVAL;

	work_state = work_busy(&fw->work.work);
	fw_log("work_state = 0x%x, fw_state = %d\n", work_state, fw->state);
	if (work_state & (WORK_BUSY_PENDING | WORK_BUSY_RUNNING))
		return -EBUSY;

	mutex_lock(&fw->mlock);
	fw->state = SB_WRL_FW_RESULT_INIT;
	fw->mode = mode;
	schedule_delayed_work(&fw->work, 0);
	mutex_unlock(&fw->mlock);

	return 0;
}

