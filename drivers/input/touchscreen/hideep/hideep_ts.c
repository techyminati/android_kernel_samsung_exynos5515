// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2012 Hideep, Inc.
 * kim.liao@hideep.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "hideep.h"

#define GET_PAYLOAD_SIZE_FROM_HEADER(x) ((x[1] >> 6) ? ((x[2] * 256 + x[3]) * (x[7] + 1)) : (x[2] * x[3] * (x[7] + 1)))

#define DEFAULT_TIM	0x03
#define SET_FLASH_DISABLE	hideep_pgm_w_reg(ts, FLASH_CON, 0x00000000)
#define SET_FLASH_ENABLE(CE)	hideep_pgm_w_reg(ts, FLASH_CON, 0x01 | CE)
#define SET_FLASH_SIG(X, Y)	hideep_pgm_w_reg(ts, FLASH_BASE + ts->nvm_ops.HIDEEP_FLASH_PIO_SIG + (X), (Y))

#define NVM_DEFAULT_PAGE            0
#define NVM_SFR_WPAGE               1
#define NVM_SFR_RPAGE               2

#define NVM_W_SFR(x, y) \
{ \
	SET_FLASH_ENABLE(ts->nvm_ops.HIDEEP_FLASH_WRITE_ENABLE); \
	SET_FLASH_SIG(x, y); \
	SET_FLASH_ENABLE(0); \
}

u32 logflags = 0;
static struct hideep_debug_dev_t *hdd;
static char nbuff[HIDEEP_PROC_SIZE];

static int hideep_load_fw_from_bin(struct hideep_t *ts);
static void hideep_print_info_work(struct work_struct *work);

int hideep_pgm_w_mem(struct hideep_t *ts, u32 addr, struct pgm_packet *packet, u32 len)
{
	int ret = 0;
	int i;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		return -EIO;
	}

	if ((len % 4) != 0)
		return -1;

	packet->header.w[0] = htonl((0x80 | (len / 4-1)));
	packet->header.w[1] = htonl(addr);

	for (i = 0; i < len / sizeof(u32); i++) {
		packet->payload[i] = htonl(packet->payload[i]);
	}

	mutex_lock(&ts->i2c_mutex);
	ret = i2c_master_send(ts->client, (u8 *)&packet->header.b[3], (len + 5));
	mutex_unlock(&ts->i2c_mutex);

	if (ts->debug_flag & HIDEEP_DEBUG_PRINT_I2C_WRITE_CMD) {
		pr_info("sec_input:i2c_cmd: W: %08x | ", addr);
		for (i = 0; i < len / sizeof(u32); i++)
			pr_cont("%02x%02x%02x%02x ", packet->payload[i] & 0xFF,
			(packet->payload[i] >> 8) & 0xFF, (packet->payload[i] >> 16) & 0xFF,
			(packet->payload[i] >> 24) & 0xFF);
		pr_cont("\n");
	}

	if (ret < 0)
		goto err;

err:
	return ret;
}

int hideep_pgm_r_mem(struct hideep_t *ts, u32 addr, struct pgm_packet *packet, u32 len)
{
	int ret = 0;
	int i;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		return -EIO;
	}

	if ((len % 4) != 0)
		return -1;

	mutex_lock(&ts->i2c_mutex);

	packet->header.w[0] = htonl((0x00 | (len / 4-1)));
	packet->header.w[1] = htonl(addr);

	ret = i2c_master_send(ts->client, (u8 *)&packet->header.b[3], 5);

	if (ret < 0)
		goto err;

	ret = i2c_master_recv(ts->client, (u8 *)packet->payload, len);
	if (ret < 0)
		goto err;

	for (i = 0; i < len / sizeof(u32); i++)
		packet->payload[i] = htonl(packet->payload[i]);

	if (ts->debug_flag & HIDEEP_DEBUG_PRINT_I2C_READ_CMD) {
		pr_info("sec_input:i2c_cmd: R: %08X | ", addr);
		for (i = 0; i < len / sizeof(u32); i++)
			pr_cont("%02x%02x%02x%02x ", packet->payload[i] & 0xFF,
			(packet->payload[i] >> 8) & 0xFF, (packet->payload[i] >> 16) & 0xFF,
			(packet->payload[i] >> 24) & 0xFF);
		pr_cont("\n");
	}

err:
	mutex_unlock(&ts->i2c_mutex);
	return ret;
}

int hideep_pgm_r_reg(struct hideep_t *ts, u32 addr, u32 *val)
{
	int ret = 0;
	struct pgm_packet packet;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

	packet.header.w[0] = htonl(0x00);
	packet.header.w[1] = htonl(addr);

	ret = hideep_pgm_r_mem(ts, addr, &packet, 4);
	if (ret < 0)
		goto err;

	*val = packet.payload[0];

err:
	return ret;
}

int hideep_pgm_w_reg(struct hideep_t *ts, u32 addr, u32 data)
{
	int ret = 0;
	struct pgm_packet packet;

	packet.header.w[0] = htonl(0x80);
	packet.header.w[1] = htonl(addr);
	packet.payload[0] = data;

	ret = hideep_pgm_w_mem(ts, addr, &packet, 4);

	return ret;
}

static void get_dwz_from_binary(const u8 *pres, struct dwz_info_t *dwz_info)
{
	memcpy(dwz_info, pres + HIDEEP_DWZ_INFO_OFS, sizeof(struct dwz_info_t));
}

static void hideep_pgm_reset(struct hideep_t *ts, u32 food)
{
	input_info(true, &ts->client->dev, "%s\n", __func__);

	hideep_pgm_w_reg(ts, SYSCON_WDT_CNT, food);
	hideep_pgm_w_reg(ts, SYSCON_WDT_CON, 0x03);
	hideep_pgm_w_reg(ts, SYSCON_WDT_CON, 0x01);
}

static int hideep_enter_pgm(struct hideep_t *ts)
{
	int ret = 0;
	int retry_count = 10;
	int retry = 0;
	u32 status = 0;
	u32 pattern = 0xDF9DAF39;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	while (retry < retry_count) {
		i2c_master_send(ts->client, (u8 *)&pattern, 4);
		usleep_range(1000, 1000);

		/* flush invalid Tx load register */
		hideep_pgm_w_reg(ts, ESI_TX_INVALID, 0x01);

		hideep_pgm_r_reg(ts, SYSCON_PGM_ID, &status);

		if (status == htonl(pattern)) {
			input_info(true, &ts->client->dev, "%s: found magic code\n", __func__);
			break;
		} else {
			retry++;
			input_err(true, &ts->client->dev, "%s: error(%08x)\n", __func__, status);
		}
	}

	if (retry < retry_count) {
		hideep_pgm_w_reg(ts, SYSCON_WDT_CON, 0x00);
		hideep_pgm_w_reg(ts, SYSCON_SPC_CON, 0x00);// remap
		hideep_pgm_w_reg(ts, SYSCON_CLK_ENA, 0xFF);// Clock Enable for YRAM/DEMOD/ACU/MSP/CM0
		hideep_pgm_w_reg(ts, SYSCON_CLK_CON, 0x01);// Select MOSC
		hideep_pgm_w_reg(ts, SYSCON_PWR_CON, 0x01);
		hideep_pgm_w_reg(ts, FLASH_TIM, 0x03);
		hideep_pgm_w_reg(ts, FLASH_CACHE_CFG, 0x00);// cache disable
		hideep_pgm_w_reg(ts, FLASH_CACHE_CFG, 0x02);// cache flush..

		usleep_range(1000, 1000);
	} else {
		ret = -1;
	}

	return ret;
}

static int hideep_load_vr_info(struct hideep_t *ts)
{
	int ret = 0;

	input_dbg(true, &ts->client->dev, "%s\n", __func__);

	ret = ts->hideep_api->i2c_read(ts, HIDEEP_VR_INFO_ADDR, (u8 *)ts->vr_info, sizeof(struct vr_info_t));
	if (ret < 0)
		goto err;

	if (ts->vr_info->module_id == 0xFF)
		ts->vr_info->module_id = 0x00;

	ts->p_data->img_version_of_ic[0] = (u8)(ts->vr_info->product_code & 0xFF);
	ts->p_data->img_version_of_ic[1] = ts->vr_info->model_name[5];
	ts->p_data->img_version_of_ic[2] = (u8)(ts->vr_info->module_id & 0xFF);
	ts->p_data->img_version_of_ic[3] = ts->vr_info->release_ver;

	input_info(true, &ts->client->dev, "%s: IC version info : %x.%x.%x.%x\n",
			__func__, ts->p_data->img_version_of_ic[0], ts->p_data->img_version_of_ic[1],
			ts->p_data->img_version_of_ic[2],ts->p_data->img_version_of_ic[3]);

	input_info(true, &ts->client->dev, "%s: num_tx: %d, num_rx: %d\n",
			__func__, ts->vr_info->num_tx, ts->vr_info->num_rx);

err:
	return ret;
}

static int hideep_nvm_unlock(struct hideep_t *ts)
{
	int ret = 0;
	u32 trim_data = 8;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	SET_FLASH_ENABLE(0);
	ret = hideep_pgm_w_reg(ts, FLASH_CFG, NVM_SFR_RPAGE);
	if (ts->nvm_ops.support_ecc)
		hideep_pgm_w_reg(ts, FLASH_ECC_CON, 0x03);

	SET_FLASH_DISABLE;
	ret = hideep_pgm_r_reg(ts, 0x00000000, &trim_data);
	input_info(true, &ts->client->dev, "%s: trim_data = %08x\n",
			__func__, trim_data);
	trim_data = (trim_data & 0x0000F000);

	SET_FLASH_ENABLE(0);

	ret = hideep_pgm_w_reg(ts, FLASH_CFG, NVM_DEFAULT_PAGE);

	if (ts->nvm_ops.support_ecc)
		hideep_pgm_w_reg(ts, FLASH_ECC_CON, 0x0);

	ret = hideep_pgm_w_reg(ts, FLASH_CFG, NVM_SFR_WPAGE);
	if (ts->nvm_ops.support_ecc)
		hideep_pgm_w_reg(ts, FLASH_ECC_CON, 0x3);

	NVM_W_SFR(0, 0x28170EA0 | trim_data);
	NVM_W_SFR(4, 0x0E5203FF);
	NVM_W_SFR(8, 0x8C203D0C);
	NVM_W_SFR(NVM_MASK_OFS, ts->nvm_ops.unlock_mask);

	SET_FLASH_DISABLE;

	ret = hideep_pgm_w_reg(ts, FLASH_CFG, NVM_DEFAULT_PAGE);

	if (ts->nvm_ops.support_ecc)
		hideep_pgm_w_reg(ts, FLASH_ECC_CON, 0);

	return ret;
}

static bool is_flash_ready(struct hideep_t *ts)
{
	u32 status = 0;
	int limit_n = 100;

	while (limit_n-- > 0) {
		usleep_range(1000, 1000);
		hideep_pgm_r_reg(ts, FLASH_STA, &status);
		if ((status) != 0)
			return true;
	}
	return false;
}

static int hideep_erase_page(struct hideep_t *ts, u32 addr)
{
	int ret = 0;
	u32 pio_cmd = 0;
	u32 pio_addr = addr;

	input_dbg(true, &ts->client->dev, "%s: address : %08x\n", __func__, addr);

	hideep_pgm_w_reg(ts, FLASH_TIM, DEFAULT_TIM);

	/********************************************************
	 * erase
	 ********************************************************/
	pio_addr = pio_addr & ~(ts->nvm_ops.page_size - 1);
	pio_addr &= ts->nvm_ops.HIDEEP_NVM_ADDR_MASK;

	//1. checking of flash ready strobe
	ret = (is_flash_ready(ts)) ? 0 : -1;
	if (ret < 0)
		goto err;

	//2. make the command for page erase
	pio_cmd = ts->nvm_ops.HIDEEP_PERASE | pio_addr;

	//3. run the erase command and enter into pio mode
	ret = SET_FLASH_ENABLE(0);
	ret = SET_FLASH_ENABLE(ts->nvm_ops.HIDEEP_FLASH_ERASE_ENABLE);
	ret = SET_FLASH_SIG(pio_cmd, 0);

	//4. checking of flash ready strobe
	ret = (is_flash_ready(ts)) ? 0 : -1;
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: erase : error(%08x)\n", __func__, pio_addr);
		goto err;
	}

	//5. release pio mode and then disable flashing
	ret = SET_FLASH_SIG(0, 0);
	SET_FLASH_DISABLE;

err:
	return ret;
}

static int hideep_program_page(struct hideep_t *ts, u32 addr, struct pgm_packet *packet_w)
{
	int ret = 0;
	u32 pio_cmd = 0;
	u32 pio_addr = addr;
	u32 i = 0;

	input_dbg(true, &ts->client->dev, "%s: address : %08x\n", __func__, addr);

	/********************************************************
	 * write
	 ********************************************************/
	pio_addr = pio_addr & ~(ts->nvm_ops.page_size - 1);
	pio_addr &= ts->nvm_ops.HIDEEP_NVM_ADDR_MASK;

	//1. checking of flash ready strobe
	ret = (is_flash_ready(ts)) ? 0 : -1;
	if (ret < 0)
		goto err;

	//2. make the command for page write
	pio_cmd = ts->nvm_ops.HIDEEP_WRONLY;

	SET_FLASH_ENABLE(0);
	SET_FLASH_ENABLE(ts->nvm_ops.HIDEEP_FLASH_WRITE_ENABLE);

	usleep_range(1000, 1000);

	//3. run the write command and enter into pio mode
	if (ts->nvm_ops.support_nvm_extra_outer_work)
		SET_FLASH_SIG(pio_cmd+pio_addr, packet_w->payload[0]);
	for (i = 0; i < ts->nvm_ops.HIDEEP_NVM_PAGE_SIZE >> 2; i++)
		SET_FLASH_SIG(pio_cmd + pio_addr + (i << 2), packet_w->payload[i]);

	if (ts->nvm_ops.support_nvm_extra_outer_work)
		SET_FLASH_SIG(pio_cmd + (ts->nvm_ops.HIDEEP_NVM_PAGE_SIZE - 4), packet_w->payload[i - 1]);

	//4. release pio mode
	SET_FLASH_ENABLE(0);

	//5. checking of flash ready strobe
	ret = (is_flash_ready(ts)) ? 0 : -1;

	//6. disable flashing
	SET_FLASH_DISABLE;

err:
	return ret;
}

static int hideep_program_nvm(struct hideep_t *ts, const u8 *ucode, int len, int offset, u8 *old_fw)
{
	int i = 0;
	int ret = 0;
	int len_r = 0;
	int len_w = 0;
	int addr = 0;
	u32 pages = 0;

	struct pgm_packet packet_w;

	ts->hideep_api->reset_ic(ts);
	ret = hideep_enter_pgm(ts);
	if (ret < 0)
		goto err;

	if (ts->nvm_ops.support_lock)
		hideep_nvm_unlock(ts);

	/* Nvm page all erase */
	addr = offset;
	pages = (len + ts->nvm_ops.page_size - 1) / ts->nvm_ops.page_size;

	input_info(true, &ts->client->dev, "%s: erase pages : %d\n", __func__,  pages);

	for (i = 0; i < pages; i++) {
		hideep_erase_page(ts, addr);
		addr += ts->nvm_ops.page_size;
	}

	/* Nvm exec arer write */
	addr = offset + HIDEEP_BOOT_LEN;
	len_r = len - HIDEEP_BOOT_LEN;
	pages = (len_r + ts->nvm_ops.page_size - 1) / ts->nvm_ops.page_size;

	input_info(true, &ts->client->dev, "%s: exec pages : %d\n", __func__,  pages);

	for (i = 0; i < pages; i++) {
		len_w = min_t(size_t, len_r, ts->nvm_ops.page_size);

		/* write page */
		memcpy(packet_w.payload, &(ucode[addr]), len_w);

		ret = hideep_program_page(ts, addr, &packet_w);
		usleep_range(1000, 1000);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: error(%08x)\n", __func__,  addr);

		addr += ts->nvm_ops.page_size;
		len_r -= ts->nvm_ops.page_size;
	}

	/* Nvm boot arer write */
	addr = offset;
	len_r = HIDEEP_BOOT_LEN;
	pages = (len_r + ts->nvm_ops.page_size - 1) / ts->nvm_ops.page_size;

	input_info(true, &ts->client->dev, "%s: boot pages : %d\n", __func__,  pages);

	for (i = 0; i < pages; i++) {
		len_w = min_t(size_t, len_r, ts->nvm_ops.page_size);

		/* write page */
		memcpy(packet_w.payload, &(ucode[addr]), len_w);

		ret = hideep_program_page(ts, addr, &packet_w);
		usleep_range(1000, 1000);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: error(%08x)\n", __func__,  addr);

		addr += ts->nvm_ops.page_size;
		len_r -= ts->nvm_ops.page_size;
	}

	hideep_pgm_reset(ts, 1000);
	msleep(50);

err:
	return ret;
}

static int hideep_verify_nvm(struct hideep_t *ts, const u8 *ucode, u32 len, u32 offset)
{
	int i = 0;
	int ret = 0;
	u8 page_chk = 0;
	u32 addr = offset;
	u32 pages = (len + ts->nvm_ops.page_size - 1) / ts->nvm_ops.page_size;
	int len_r = len;
	int len_v = len_r;

	struct pgm_packet packet_r;

	for (i = 0; i < pages; i++) {
		if (len_r >= ts->nvm_ops.page_size)
			len_v = ts->nvm_ops.page_size;

		ret = hideep_pgm_r_mem(ts, 0x00000000 + addr, &packet_r, ts->nvm_ops.page_size);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to read mem: %d\n", __func__, ret);
			return ret;
		}

		page_chk = memcmp(&(ucode[addr]), &packet_r.payload[0], len_v);

		if (page_chk != 0) {
			input_err(true, &ts->client->dev, "%s: error(addr : %d)\n", __func__,  addr);

			ret = -1;
		}

		addr += ts->nvm_ops.page_size;
		len_r -= ts->nvm_ops.page_size;
		len_v = len_r;
	}

	return ret;
}

static void hideep_read_nvm(struct hideep_t *ts, u8 *data, int len, int offset)
{
	int ret = 0;
	int pages, i;
	int len_r, len_v;
	int addr = offset;

	struct pgm_packet packet_r;

	pages = (len + ts->nvm_ops.page_size - 1) / ts->nvm_ops.page_size;
	len_r = len;
	len_v = len_r;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ts->hideep_api->reset_ic(ts);

	ret = hideep_enter_pgm(ts);
	if (ret < 0)
		goto err;

	for (i = 0; i < pages; i++) {
		if (len_r >= ts->nvm_ops.page_size)
			len_v = ts->nvm_ops.page_size;

		ret = hideep_pgm_r_mem(ts, 0x00000000 + addr, &packet_r, ts->nvm_ops.page_size);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to read mem: %d\n", __func__, ret);
			return;
		}
		memcpy(&(data[i * ts->nvm_ops.page_size]), &packet_r.payload[0], len_v);

		addr += ts->nvm_ops.page_size;
		len_r -= ts->nvm_ops.page_size;
		len_v = len_r;
	}

	hideep_pgm_reset(ts, 1000);
	msleep(50);

err:
	return;
}

static int hideep_fw_verify_run(struct hideep_t *ts, u8 *fw, size_t len, int offset)
{
	int ret = 0;
	int retry = 3;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = hideep_enter_pgm(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed enter pgm\n", __func__);
		goto err;
	}

	while (retry--) {
		ret = hideep_verify_nvm(ts, fw, len, offset);
		if (ret == 0) {
			input_info(true, &ts->client->dev, "%s: update success\n", __func__);
			break;
		}
		input_err(true, &ts->client->dev, "%s: download fw failed(%d)\n", __func__, retry);
	}

	ret = (retry == 0) ? -1 : 0;
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fw update failed\n", __func__);
	}

err:
	hideep_pgm_reset(ts, 1000);
	msleep(50);

	return ret;
}

static int hideep_firmware_integrity(struct hideep_t *ts)
{
	int ret;
	int retry_cnt = 3;
	u16 magic = 0;

	while (retry_cnt--) {
		ret = ts->hideep_api->i2c_read(ts, HIDEEP_INFO_ADDR, (u8 *)&magic, sizeof(magic));
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to read magic word(cnt:%d)\n", __func__, retry_cnt);
			msleep(50);
			continue;
		}

		if (magic == HIDEEP_INTEGRITY_CHECK_PASS) {
			input_info(true, &ts->client->dev, "%s: firmware integrity pass\n", __func__);
			return 0;
		} else if (magic == HIDEEP_INTEGRITY_CHECK_FAIL) {
			input_err(true, &ts->client->dev, "%s: failed to integrity check. reboot\n", __func__);
			ts->hideep_api->reset_ic(ts);
			break;
		} else {
			input_err(true, &ts->client->dev, "%s: magic word is wrong: 0x%04x\n", __func__, magic);
			msleep(100);
			continue;
		}
	}

	return -1;
}

static int hideep_wr_firmware(struct hideep_t *ts, u8 *code, int len, int offset, bool mode)
{
	int ret = 0;
	int firm_len;
	struct dwz_info_t bin_dwz_info;

	firm_len = len;

	input_info(true, &ts->client->dev, "%s: fw len : %d, define size : %d\n",
			__func__, firm_len, ts->fw_size);

	if (firm_len > ts->fw_size)
		firm_len = ts->fw_size;

	/* comparing & programming each page, if the memory of specified page is exactly same, no need to update. */
	ret = hideep_program_nvm(ts, code, firm_len, offset, NULL);
	if (ret < 0)
		goto err;

#ifdef PGM_VERIFY
	get_dwz_from_binary(code, &bin_dwz_info);
	if (bin_dwz_info.boot_ver == 1) {
		input_info(true, &ts->client->dev, "%s: old firmware verify check\n", __func__);
		ret = hideep_fw_verify_run(ts, code, firm_len, offset);
	} else {
		msleep(50);
		ret = hideep_firmware_integrity(ts);
	}

	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: PGM fw verify failed\n", __func__);
		goto err;
	}
#endif

	if (mode == true)
		ts->hideep_api->get_vr_info(ts);

err:
	return ret;
}

static void hideep_save_version_of_bin(struct hideep_t *ts, struct dwz_info_t *dwz_info)
{
	ts->p_data->img_version_of_bin[0] = (u8)(dwz_info->product_code & 0xFF);
	ts->p_data->img_version_of_bin[1] = dwz_info->model_name[5];
	ts->p_data->img_version_of_bin[2] = (u8)(dwz_info->module_id & 0xFF);
	ts->p_data->img_version_of_bin[3] = dwz_info->release_ver;

	input_info(true, &ts->client->dev, "%s: img_ver of bin = %x.%x.%x.%x\n", __func__,
			ts->p_data->img_version_of_bin[0],
			ts->p_data->img_version_of_bin[1],
			ts->p_data->img_version_of_bin[2],
			ts->p_data->img_version_of_bin[3]);
}

static int hideep_is_update_necessary(struct hideep_t *ts, const u8 *pres)
{
	struct dwz_info_t bin_dwz_info;
	int i;

	get_dwz_from_binary(pres, &bin_dwz_info);

	hideep_save_version_of_bin(ts, &bin_dwz_info);

	if (ts->p_data->img_version_of_ic[3] == 0xffff)
		return 1;

	if (ts->p_data->bringup == 4) {
		input_err(true, &ts->client->dev, "%s: bringup 4. force update.\n", __func__);
		return 1;
	}

	/* check f/w version
	 * ver[0] : IC version
	 * ver[1] : Project version
	 * ver[2] : Panel infomation
	 */
	for (i = 0; i < 2; i++) {
		if (ts->p_data->img_version_of_ic[i] != ts->p_data->img_version_of_bin[i]) {
			if (ts->p_data->bringup == 3) {
				input_err(true, &ts->client->dev, "%s: bringup 3. force update\n", __func__);
				return 1;
			}

			input_err(true, &ts->client->dev, "%s: not matched version info\n", __func__);
			return 0;
		}
	}

	if (gpio_is_valid(ts->p_data->tsp_id)) {
		if (ts->p_data->img_version_of_ic[2] != ts->p_data->img_version_of_bin[2]) {
			input_err(true, &ts->client->dev, "%s: panel changed. force update\n", __func__);
			return 1;
		}
	} else {
		if (ts->p_data->img_version_of_ic[2] != ts->p_data->img_version_of_bin[2]) {
			input_err(true, &ts->client->dev, "%s: not matched panel version info\n", __func__);
			return 0;
		}
	}

	if (ts->p_data->img_version_of_ic[3] < ts->p_data->img_version_of_bin[3])
		return 1;

	return 0;
}

static int hideep_update_all_firmware(struct hideep_t *ts, const char *fn)
{
	int ret = 0;
	const struct firmware *fw_entry;
	u8 *fw_buf;

	input_dbg(true, &ts->client->dev, "%s\n", __func__);

	ret = request_firmware(&fw_entry, fn, &ts->client->dev);
	if (ret != 0) {
		input_err(true, &ts->client->dev, "%s: fail request_firmware(%d)\n", __func__, ret);
		return ret;
	}

	if (!ts->manually_update) {
		ret = hideep_is_update_necessary(ts, fw_entry->data);
		if (ret == 0) {
			input_info(true, &ts->client->dev, "%s: no need update\n", __func__);
			release_firmware(fw_entry);
			return 0;
		}
	}

	fw_buf = kzalloc(fw_entry->size, GFP_USER);
	if (!fw_buf) {
		input_err(true, &ts->client->dev, "%s: can't allocate  memory for fw_buf\n", __func__);
		release_firmware(fw_entry);
		return -ENOMEM;
	}

	memcpy(fw_buf, fw_entry->data, fw_entry->size);

	/* chip specific code for flash fuse */
	mutex_lock(&ts->dev_mutex);

	ts->dev_state = state_updating;

	ret = hideep_wr_firmware(ts, fw_buf, fw_entry->size, 0, true);

	ts->dev_state = state_normal;

	mutex_unlock(&ts->dev_mutex);

	kfree(fw_buf);
	release_firmware(fw_entry);

	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write firmware(%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int hideep_update_part_firmware(struct hideep_t *ts, u8 *code, int len, int offset, bool mode)
{
	int ret = 0;

	mutex_lock(&ts->dev_mutex);

	ret = hideep_wr_firmware(ts, code, len, offset, false);

	mutex_unlock(&ts->dev_mutex);

	return ret;
}

static int hideep_config_for_nvm(struct hideep_t *ts)
{
	switch (ts->chip_id_sfr) {
	case HIDEEP_LIME:
		ts_log_info("used lime IC");
		ts->fw_size = 1024 * 64;
		ts->nvm_ops.page_size = 128;
		ts->nvm_ops.support_lock = true;
		ts->nvm_ops.support_nvm_extra_outer_work = true;
		ts->nvm_ops.unlock_mask = 0x0031027B;
		ts->nvm_ops.support_ecc = false;
		ts->ic_name = "lime";
		ts->nvm_ops.HIDEEP_PERASE = (0x00040000);
		ts->nvm_ops.HIDEEP_WRONLY = (0x00100000);
		ts->nvm_ops.HIDEEP_NVM_PAGE_SIZE = (128);
		ts->nvm_ops.HIDEEP_NVM_ADDR_MASK = (0x0000FFFC);
		ts->nvm_ops.HIDEEP_FLASH_PIO_SIG = (0x00400000);
		ts->nvm_ops.HIDEEP_FLASH_ERASE_ENABLE = 1 << 1;
		ts->nvm_ops.HIDEEP_FLASH_WRITE_ENABLE = 1 << 1;
		break;
	case HIDEEP_LEMON:
		ts_log_info("used lemon IC");
		ts->fw_size = 1024 * 128;
		ts->nvm_ops.page_size = 128;
		ts->nvm_ops.support_lock = true;
		ts->nvm_ops.support_nvm_extra_outer_work = true;
		ts->nvm_ops.unlock_mask = 0x00303270;
		ts->nvm_ops.support_ecc = true;
		ts->ic_name = "lemon";
		ts->nvm_ops.HIDEEP_PERASE = (0x00080000);
		ts->nvm_ops.HIDEEP_WRONLY = (0x00200000);
		ts->nvm_ops.HIDEEP_NVM_PAGE_SIZE = (128);
		ts->nvm_ops.HIDEEP_NVM_ADDR_MASK = (0x0001FFFC);
		ts->nvm_ops.HIDEEP_FLASH_PIO_SIG = (0x00800000);
		ts->nvm_ops.HIDEEP_FLASH_ERASE_ENABLE = 1 << 1;
		ts->nvm_ops.HIDEEP_FLASH_WRITE_ENABLE = 1 << 1;
		break;
	case HIDEEP_ORANGE:
		ts_log_info("used orange IC");
		ts->fw_size = 1024 * 192;
		ts->nvm_ops.page_size = 512;
		ts->nvm_ops.support_lock = false;
		ts->nvm_ops.support_nvm_extra_outer_work = false;
		ts->nvm_ops.unlock_mask = 0x00000000;
		ts->nvm_ops.support_ecc = true;
		ts->ic_name = "orange";
		ts->nvm_ops.HIDEEP_MCS = (0x00400000);
		ts->nvm_ops.HIDEEP_PERASE = (0x00400000);
		ts->nvm_ops.HIDEEP_WRONLY = (0x00400000);
		ts->nvm_ops.HIDEEP_NVM_PAGE_SIZE = (512);
		ts->nvm_ops.HIDEEP_NVM_ADDR_MASK = (0x0003FFFC);
		ts->nvm_ops.HIDEEP_FLASH_PIO_SIG = (0x00800000);
		ts->nvm_ops.HIDEEP_FLASH_ERASE_ENABLE = (0x08000080);
		ts->nvm_ops.HIDEEP_FLASH_WRITE_ENABLE = (0x08000100);
		break;
	case HIDEEP_GARNET:
		ts_log_info("used garnet IC");
		ts->fw_size = 1024 * 64;
		ts->nvm_ops.page_size = 512;
		ts->nvm_ops.support_lock = false;
		ts->nvm_ops.support_nvm_extra_outer_work = false;
		ts->nvm_ops.unlock_mask = 0x00000000;
		ts->nvm_ops.support_ecc = false;
		ts->ic_name = "garnet";
		ts->nvm_ops.HIDEEP_MCS = (0x00400000);
		ts->nvm_ops.HIDEEP_PERASE = (0x00400000);
		ts->nvm_ops.HIDEEP_WRONLY = (0x00400000);
		ts->nvm_ops.HIDEEP_NVM_PAGE_SIZE = (512);
		ts->nvm_ops.HIDEEP_NVM_ADDR_MASK = (0x0003FFFC);
		ts->nvm_ops.HIDEEP_FLASH_PIO_SIG = (0x00800000);
		ts->nvm_ops.HIDEEP_FLASH_ERASE_ENABLE = (0x08000080);
		ts->nvm_ops.HIDEEP_FLASH_WRITE_ENABLE = (0x08000100);
		break;
	default:
		ts_log_info("ic code is wrong");
		return -EINVAL;
	}

	return 0;
}

static int hideep_config_by_chip_id(struct hideep_t *ts)
{
	int error;
	u32 chip_id;

	error = ts->hideep_api->i2c_read(ts, HIDEEP_CHIP_ID_ADDR, (u8 *)&chip_id, 2);
	if (error < 0)
		return -EIO;

	ts->chip_id_sfr = chip_id & 0xFFFF;
	error = hideep_config_for_nvm(ts);

	return error;
}

void hideep_isp_init(struct hideep_t *ts)
{
	ts->hideep_api->config_ic_feature = hideep_config_by_chip_id;
	ts->hideep_api->update_all = hideep_update_all_firmware;
	ts->hideep_api->update_part = hideep_update_part_firmware;
	ts->hideep_api->get_vr_info = hideep_load_vr_info;
	ts->hideep_api->sp_func = hideep_read_nvm;
}

static int hideep_i2c_recv(u32 len)
{
	int ret = 0;

	mutex_lock(&hdd->ts->i2c_mutex);
	ret = i2c_master_recv(hdd->ts->client, hdd->vr_buff, len);
	mutex_unlock(&hdd->ts->i2c_mutex);
	if (ret < 0)
		goto i2c_err;

	ts_log_info("(%d)", len);
	return ret;

i2c_err:
	ts_log_err("i2c_err");
	return ret;
}

static int hideep_i2c_send(u32 len)
{
	int ret = 0;
	u8 *buff = hdd->vr_buff;

	mutex_lock(&hdd->ts->i2c_mutex);
	ret = i2c_master_send(hdd->ts->client, buff, len);
	mutex_unlock(&hdd->ts->i2c_mutex);
	if (ret < 0)
		goto i2c_err;

	ts_log_info("(%d)", len);
	return ret;

i2c_err:
	ts_log_err("i2c_err");
	return ret;
}

static int hideep_get_vreg(u32 addr, u32 len)
{
	int ret = 0;

	ret = hdd->ts->hideep_api->i2c_read(hdd->ts, addr, hdd->vr_buff, len);
	if (ret < 0)
		goto i2c_err;

	ts_log_ic_dbg("(0x%02x:%d)", addr, len);
	return ret;

i2c_err:
	ts_log_err("i2c_err");
	return ret;
}

static int hideep_set_vreg(u32 addr, u32 len)
{
	int ret = 0;
	int wr_remain = len;
	int vr_addr = addr;
	int wr_len = len;
	u8 *buff = hdd->vr_buff;

	do {
		if (wr_remain >=  MAX_VR_BUFF) {
			wr_len = MAX_VR_BUFF;
		} else
			wr_len = wr_remain;

	ret = hdd->ts->hideep_api->i2c_write(hdd->ts, vr_addr, buff, wr_len);
		if (ret < 0)
			goto i2c_err;

		wr_remain -= MAX_VR_BUFF;
		vr_addr += MAX_VR_BUFF;
		buff += MAX_VR_BUFF;
	} while (wr_remain > 0);

	ts_log_ic_dbg("(0x%02x:%d)", addr, len);
	return ret;

i2c_err:
	ts_log_err("i2c_err");
	return ret;
}

static int hideep_download_uc(const char __user *uc, size_t count, int offset)
{
	int ret;
	u8 *ucode;
	int len = offset;

	ts_log_ic_dbg("count = %zu", count);

	len += count;
	if (len > hdd->ts->fw_size) {
		ts_log_err("over size data!!!");
		return -1;
	}

	ucode = vmalloc(count);

	ret = copy_from_user(ucode + offset, uc, count);
	if (ret < 0) {
		ts_log_err("ADDR_UC : copy_to_user");
		kfree(ucode);
		return 0;
	}

	disable_irq(hdd->ts->client->irq);
	hdd->ts->interrupt_state = 0;
	hdd->ts->hideep_api->update_part(hdd->ts, ucode, count, offset, false);
	enable_irq(hdd->ts->client->irq);
	hdd->ts->interrupt_state = 1;

	vfree(ucode);

	ts_log_ic_dbg("Download_uc(%zu)", count);

	return count;
}

static int hideep_debug_open(struct inode *inode, struct file *file)
{
	hdd->release_flag = false;

	file->private_data = hdd;
	ts_log_ic_dbg("hideep_debug_open");

	return 0;
}

static int hideep_debug_release(struct inode *inode, struct file *file)
{
	ts_log_ic_dbg("");
	if (!hdd->release_flag)
		return -1;

	hdd->release_flag = false;
	file->private_data = NULL;

	return 0;
}

static u32 hideep_debug_poll(struct file *file, struct poll_table_struct *wait)
{
	u32 mask = 0;
	struct hideep_debug_dev_t *drv_info;

	if (file->private_data == NULL)
		return 0;

	drv_info = file->private_data;

	poll_wait(file, &drv_info->i_packet, wait);

	if (drv_info->ready) {
		disable_irq(drv_info->ts->client->irq);
		drv_info->ts->interrupt_state = 0;
		mask |= POLLIN | POLLRDNORM;
		drv_info->ready = 0;
	}

	return mask;
}

static ssize_t hideep_debug_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	int ret = -1;
	ssize_t rd_len = 0;
	u8 *rd_buffer = NULL;
	u8 *data = NULL;
	struct hideep_debug_dev_t *drv_info = file->private_data;

	ts_log_ic_dbg("count = %zu", count);

	if (file->private_data == NULL)
		return 0;

	if (count > MAX_VR_BUFF || count < 1)
		return 0;

	drv_info->vr_size = count;
	rd_len = count;

	data = kmalloc(rd_len, GFP_KERNEL);

	if (*offset <= HIDEEP_VR_ADDR_LEN) {
		// if offset is not belong to any special command
		if ((*offset & HIDEEP_MAX_RAW_LEN) && (*offset < HIDEEP_MAX_RAW_LEN) && (drv_info->debug_enable == true)) {
			mutex_lock(&drv_info->ts->dev_mutex);
			rd_buffer = drv_info->img_buff;
			ret = 0;
			mutex_unlock(&drv_info->ts->dev_mutex);
		} else {
			ret = hideep_get_vreg(*offset, rd_len);
			rd_buffer = drv_info->vr_buff;
		}

		if (ret < 0)
			rd_len = 0;
	} else if (*offset & (HIDEEP_I2C_BYPASS)) {
		// if offset is belong to special command "i2c bypass"
		ret = hideep_i2c_recv(rd_len);
		if (ret < 0) {
			ts_log_err("ret = %d", ret);
			rd_len = 0;
		} else {
			rd_buffer = drv_info->vr_buff;
		}
	} else if (*offset & HIDEEP_NVM_DOWNLOAD) {
		// if offset is belong to special command "nvm download"
		rd_len = count;
		memset(data, 0x0, rd_len);

		disable_irq(drv_info->ts->client->irq);
		drv_info->ts->interrupt_state = 0;
		mutex_lock(&drv_info->ts->dev_mutex);

		drv_info->ts->hideep_api->sp_func(drv_info->ts, data, rd_len, *offset & 0xfffff);

		mutex_unlock(&drv_info->ts->dev_mutex);
		enable_irq(drv_info->ts->client->irq);
		drv_info->ts->interrupt_state = 1;

		rd_buffer = data;
	} else {
		ts_log_err("undefined address");
		kfree(data);
		return 0;
	}

	ret = copy_to_user(buf, rd_buffer, rd_len);

	if (drv_info->debug_enable == true && drv_info->ready == 0) {
		ts_log_ic_dbg("");
		enable_irq(drv_info->ts->client->irq);
		drv_info->ts->interrupt_state = 1;
	}

	if (ret < 0) {
		ts_log_err("error : copy_to_user");
		kfree(data);
		return -EFAULT;
	}

	kfree(data);
	return rd_len;
}

static ssize_t hideep_debug_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	int ret;
	struct hideep_debug_dev_t *drv_info = file->private_data;
	int wr_len = 0;

	ts_log_ic_dbg("count = %zu", count);
	if (file->private_data == NULL)
		return 0;

	if (*offset <= HIDEEP_VR_ADDR_LEN) {
		// if offset is not belong to any special command
		wr_len = count;
		if (wr_len > MAX_VR_BUFF) {
			ts_log_err("buff overflow");
			return -E2BIG;
		}

		ret = copy_from_user(drv_info->vr_buff, buf, wr_len);
		if (ret < 0) {
			ts_log_err("error : copy_to_user");
			return -EFAULT;
		}

		ret = hideep_set_vreg(*offset, wr_len);
		if (ret < 0)
			wr_len = 0;
	}  else if (*offset & (HIDEEP_I2C_BYPASS)) {
		// if offset is belong to special command "i2c bypass"
		wr_len = count;
		if (wr_len > MAX_VR_BUFF) {
			ts_log_err("buff overflow");
			return -E2BIG;
		}

		ret = copy_from_user(drv_info->vr_buff, buf, wr_len);
		ret = hideep_i2c_send(wr_len);
	} else if (*offset & HIDEEP_NVM_DOWNLOAD) {
		// if offset is belong to special command "nvm download"
		wr_len = hideep_download_uc(buf, count, *offset & 0xfffff);
	} else {
		ts_log_err("hideep_write : undefined address, 0x%08x", (int)*offset);
		return 0;
	}

	return wr_len;
}

static loff_t hideep_debug_llseek(struct file *file, loff_t off, int whence)
{
	loff_t newpos;
	struct hideep_debug_dev_t *drv_info = file->private_data;

	ts_log_ic_dbg("off = 0x%08x, whence = %d", (u32)off, whence);
	if (file->private_data == NULL)
		return -EFAULT;

	switch (whence) {
	/* SEEK_SET */
	case 0:
		newpos = off;
		break;
		/* SEEK_CUR */
	case 1:
		ts_log_ic_dbg("set mode off = 0x%08x", (u32)off);
		if (off & HIDEEP_RELEASE_FLAG) {
			ts_log_ic_dbg("set release flag");
			drv_info->release_flag = true;
		}
		newpos = file->f_pos;
		break;
		/* SEEK_END */
	case 2:
		newpos = file->f_pos;
		break;
	default:
		return -EINVAL;
	}

	if (newpos < 0)
		return -EINVAL;

	file->f_pos = newpos;

	return newpos;
}

static const struct file_operations hideep_debug_fops = {
	.owner = THIS_MODULE,
	.open = hideep_debug_open,
	.poll = hideep_debug_poll,
	.release = hideep_debug_release,
	.read = hideep_debug_read,
	.write = hideep_debug_write,
	.llseek = hideep_debug_llseek,
};

static struct miscdevice hideep_debug_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = HIDEEP_DEBUG_DEVICE_NAME,
	.fops = &hideep_debug_fops
};

void hideep_debug_uninit()
{
	if (hdd->vr_buff)
		kfree(hdd->vr_buff);

	if (hdd->img_buff)
		kfree(hdd->img_buff);

	misc_deregister(&hideep_debug_dev);
}

int hideep_debug_init(struct hideep_t *ts)
{
	int ret = 0;

	hdd = &ts->debug_dev;

	ret = misc_register(&hideep_debug_dev);
	if (ret) {
		ts_log_err("hideep debug device register fail!!!");
		goto fail;
	}

	init_waitqueue_head(&hdd->i_packet);

	hdd->ts = ts;
	hdd->debug_enable = false;

	hdd->img_size = 0;
	hdd->vr_size = 0;

	hdd->vr_buff = vmalloc(MAX_VR_BUFF);
	hdd->img_buff = vmalloc(MAX_RAW_SIZE);

	memset(hdd->vr_buff, 0x0, MAX_VR_BUFF);
	memset(hdd->img_buff, 0x0, MAX_RAW_SIZE);

	if (!hdd->vr_buff || !hdd->img_buff)
		goto fail;

	ts_log_info("hideep_debug_init....");
	return 0;

fail:
	if (hdd->vr_buff)
		vfree(hdd->vr_buff);
	if (hdd->img_buff)
		vfree(hdd->img_buff);
	return ret;
}

static ssize_t write_fw(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int mode = 0;
	int ret;
	struct hideep_t *ts = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 10, &mode);
	if (ret < 0)
		return ret;

	if (mode == 1) {
		ts->manually_update = true;
		hideep_load_fw_from_bin(ts);
	}

	return count;
}

static ssize_t read_fw_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	struct hideep_t *ts = dev_get_drvdata(dev);

	len = scnprintf(buf, PAGE_SIZE, "%02X\n", ts->vr_info->release_ver);
	return len;
}

static ssize_t logflag_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	ts_log_info("enter");
	len = scnprintf(buf, PAGE_SIZE, "Debug\t= 0x01\nCoord\t= 0x02\nI2C\t= 0x04\nUpdate\t= 0x08\nIC Debug = 0x10\nPower\t= 0x20\nLog Flag = 0x%02x\n", logflags);
	ts_log_debug("logflag = 0x%02x ", logflags);

	return len;
}

static ssize_t logflag_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int temp = 0;
	int ret;
	struct hideep_t *ts = dev_get_drvdata(dev);

	mutex_lock(&ts->dev_mutex);

	ret = kstrtoint(buf, 10, &temp);
	if (ret < 0)
		return ret;

	ts_log_info("temp flag = %02x", temp);

	logflags &= 0x00;

	if (temp & HIDEEP_LOG_DEBUG)
		logflags |= HIDEEP_LOG_DEBUG;
	if (temp & HIDEEP_LOG_COORD)
		logflags |= HIDEEP_LOG_COORD;
	if (temp & HIDEEP_LOG_I2C)
		logflags |= HIDEEP_LOG_I2C;
	if (temp & HIDEEP_LOG_UPDATE)
		logflags |= HIDEEP_LOG_UPDATE;
	if (temp & HIDEEP_LOG_IC_DBG)
		logflags |= HIDEEP_LOG_IC_DBG;
	if (temp & HIDEEP_LOG_POWER)
		logflags |= HIDEEP_LOG_POWER;

	ts_log_info("enter, logflag = %02x", logflags);

	mutex_unlock(&ts->dev_mutex);

	return count;
}

static ssize_t fw_ver_ic_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct hideep_t *ts = dev_get_drvdata(dev);

	len = scnprintf(buf, PAGE_SIZE, "%s%02X%02X%02X%02X\n",
			"HD", (u8)(ts->vr_info->product_code & 0xFF),
			ts->vr_info->model_name[5], (u8)(ts->vr_info->module_id & 0xFF),
			ts->vr_info->release_ver);

	return len;
}

static ssize_t fw_ver_bin_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	int len = 0;
	const struct firmware *fw_entry;
	struct hideep_t *ts = dev_get_drvdata(dev);
	struct dwz_info_t dwz_info = {0,};
	char fw_path[SEC_TS_MAX_FW_PATH];

	if (!ts->p_data->firmware_name) {
		ts_log_err("%s: firmware name does not declair in dts\n", __func__);
		return -ENOENT;
	}

	snprintf(fw_path, SEC_TS_MAX_FW_PATH, "%s", ts->p_data->firmware_name);

	ret = request_firmware(&fw_entry, fw_path, &ts->client->dev);
	if (ret != 0) {
		ts_log_err("request_firmware : fail(%d)", ret);
		return -EIO;
	}

	get_dwz_from_binary((u8 *)fw_entry->data, &dwz_info);

	ts_log_info("release version : %04x", ts->vr_info->release_ver);
	ts_log_info("factory ID : %02x", ts->vr_info->factory_id);
	len = scnprintf(buf, PAGE_SIZE, "%s%02X%02X%02X%02X\n",
			"HD", (u8)(dwz_info.product_code & 0xFF),
			dwz_info.model_name[5], (u8)(dwz_info.module_id & 0xFF),
			dwz_info.release_ver);

	len = scnprintf(buf, PAGE_SIZE,
			"release ver: %04x\nfactory ID : %02x\n",
			ts->vr_info->release_ver, ts->vr_info->factory_id);

	return len;
}

static ssize_t power_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct hideep_t *ts = dev_get_drvdata(dev);

	len = scnprintf(buf, PAGE_SIZE, "power status : %s\n", (ts->dev_state == state_init) ? "off" : "on");

	return len;
}

static ssize_t power_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct hideep_t *ts = dev_get_drvdata(dev);
	int on = 0;
	int ret;

	ret = kstrtoint(buf, 10, &on);
	if (ret < 0)
		return ret;

	if (on) {
		ts->hideep_api->power(ts, on);
		ts->dev_state = state_normal;
		ts->hideep_api->reset_ic(ts);
	} else {
		ts->hideep_api->power(ts, on);
		ts->dev_state = state_init;
	}

	return count;
}

static ssize_t debug_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct hideep_t *ts = dev_get_drvdata(dev);

	len = scnprintf(buf, PAGE_SIZE, "debug mode : %s\n", (ts->debug_dev.debug_enable == 0) ? "off" : "on");

	return len;
}

static ssize_t debug_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int on = 0;
	int ret;
	u8 data[2] = {0,};
	struct hideep_t *ts = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 10, &on);
	if (ret < 0)
		return ret;

	if (on) {
		ts->debug_dev.debug_enable = 1;
		ts->dev_state = state_debugging;
	} else {
		ts->debug_dev.debug_enable = 0;
		ts->dev_state = state_normal;
		/* set touch mode */
		data[0] = 0x00;
		data[1] = 0x00;
		ts->hideep_api->i2c_write(ts, HIDEEP_OPMODE_CMD, data, 2);
		if (ts->interrupt_state == 0) {
			ts_log_info("");
			data[0] = 0x5A;
			ts->hideep_api->i2c_write(ts, HIDEEP_INTCLR_CMD, data, 1);
			enable_irq(ts->client->irq);
			ts->interrupt_state = 1;
		}
	}

	return count;
}

static DEVICE_ATTR(update, 0664, read_fw_info, write_fw);
static DEVICE_ATTR(fw_ver_ic, 0664, fw_ver_ic_show,  NULL);
static DEVICE_ATTR(fw_ver_bin, 0664, fw_ver_bin_show, NULL);
static DEVICE_ATTR(logflag, 0664, logflag_show, logflag_store);
static DEVICE_ATTR(power_en, 0664, power_status, power_control);
static DEVICE_ATTR(debug_en, 0664, debug_status, debug_mode);

static struct attribute *hideep_ts_sysfs_entries[] = {
	&dev_attr_update.attr,
	&dev_attr_fw_ver_ic.attr,
	&dev_attr_fw_ver_bin.attr,
	&dev_attr_logflag.attr,
	&dev_attr_power_en.attr,
	&dev_attr_debug_en.attr,
	NULL
};

static struct attribute_group hideep_ts_attr_group = {
	.attrs  = hideep_ts_sysfs_entries,
};

static ssize_t enabled_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct hideep_t *ts = dev_get_drvdata(dev);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->enabled);
	return scnprintf(buf, PAGE_SIZE, "%d", ts->enabled);
}

static ssize_t enabled_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct hideep_t *ts = dev_get_drvdata(dev);
	int value;
	int err;

	err = kstrtoint(buf, 10, &value);
	if (err < 0) {
		input_info(true, &ts->client->dev, "%s: invalid string: %d\n", __func__, err);
		return err;
	}

	if (!ts->info_work_done) {
		input_info(true, &ts->client->dev, "%s: not finished info work\n", __func__);
		return count;
	}

	if (ts->enabled == value) {
		input_info(true, &ts->client->dev, "%s: already %s\n", __func__, value ? "enabled" : "disabled");
		return count;
	}

	if (value > 0) {
		hideep_resume(&ts->client->dev);
		ts->suspended = 0;
	} else {
		ts->suspended = 1;
		hideep_suspend(&ts->client->dev);
	}

	ts->enabled = value;
	return count;
}

static DEVICE_ATTR(enabled, 0664, enabled_show, enabled_store);

static struct attribute *input_attributes[] = {
	&dev_attr_enabled.attr,
	NULL,
};

static struct attribute_group input_attr_group = {
	.attrs = input_attributes,
};

int hideep_sysfs_init(struct hideep_t *ts)
{
	int ret = 0;
	struct i2c_client *client = ts->client;

	/* Create the files associated with this kobject */
	ret = sysfs_create_group(&client->dev.kobj, &hideep_ts_attr_group);

	ts_log_info("device : %s ", client->dev.kobj.name);
	ret = sysfs_create_link(NULL, &client->dev.kobj, "hideep");

	if (ret)
		ts_log_err("%s: Fail create link error = %d\n", __func__, ret);

	return ret;
}

int hideep_sysfs_exit(struct hideep_t *ts)
{
	struct  i2c_client *client = ts->client;

	sysfs_remove_link(&client->dev.kobj, "hideep");
	sysfs_remove_group(&client->dev.kobj, &hideep_ts_attr_group);

	return 0;
}

static ssize_t hideep_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
	struct hideep_t *ts = hdd->ts;
	loff_t pos = *offset;
	ssize_t count;
	size_t size;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, nbuff);

	size = strnlen(nbuff, HIDEEP_PROC_SIZE);

	if (pos >= size)
		return 0;

	count = min(len, size);

	if ((pos + count) > size)
		count = size - pos;

	if (copy_to_user(buf, nbuff + pos, count))
		return -EFAULT;

	*offset += count;

	return count;
}

static const struct file_operations hideep_proc_ops = {
	.owner = THIS_MODULE,
	.read = hideep_proc_read,
};

static void hideep_proc_init(struct hideep_t *ts)
{
	static struct proc_dir_entry *entry;

	if (ts->vr_info->num_tx && ts->vr_info->num_rx) {
		ts->proc_data = kzalloc(HIDEEP_PROC_SIZE, GFP_KERNEL);
		if (!ts->proc_data)
			return;
	}

	entry = proc_create("tsp_raw_data", S_IFREG | S_IRUGO, NULL, &hideep_proc_ops);
	if (!entry)
		return;

	proc_set_size(entry, HIDEEP_PROC_SIZE);
}

static int hideep_pwr_on(struct hideep_t *ts)
{
	int ret = 0;
	struct hideep_platform_data_t *pdata;

	pdata = ts->p_data;

	if (pdata->support_ldo_control) {
		if (!IS_ERR(pdata->vcc_vid)) {
			ret = regulator_enable(pdata->vcc_vid);
			if (ret)
				input_err(true, &ts->client->dev, "%s: Regulator vid disable failed ret=%d\n", __func__, ret);
		}

		ret = regulator_enable(pdata->vcc_vdd);
		if (ret)
			input_err(true, &ts->client->dev, "%s: Regulator vdd disable failed ret=%d\n", __func__, ret);
	}

	if (pdata->support_reset_control) {
		usleep_range(5000, 5000);

		if (pdata->reset_gpio > 0) {
			input_info(true, &ts->client->dev, "%s: enable the reset_gpio\n", __func__);
			gpio_set_value(pdata->reset_gpio, 1);
		}
	}
	if (!ts->probe_done && pdata->regulator_boot_on)
		return ret;
	else
		msleep(150);
	return ret;
}

static int hideep_pwr_off(struct hideep_t *ts)
{
	int ret = 0;
	struct hideep_platform_data_t *pdata;

	pdata = ts->p_data;

	if (pdata->support_reset_control) {
		if (pdata->reset_gpio > 0) {
			input_info(true, &ts->client->dev, "%s: disable the reset_gpio\n", __func__);
			gpio_set_value(pdata->reset_gpio, 0);
		}
	}

	if (pdata->support_ldo_control) {
		ret = regulator_disable(pdata->vcc_vdd);
		if (ret)
			input_err(true, &ts->client->dev, "%s: Regulator vdd disable failed ret=%d\n", __func__, ret);

		if (!IS_ERR(pdata->vcc_vid)) {
			ret = regulator_disable(pdata->vcc_vid);
			if (ret)
				input_err(true, &ts->client->dev, "%s: Regulator vid disable failed ret=%d\n", __func__, ret);
		}
	}

	return ret;
}

static void hideep_power(struct hideep_t *ts, int on)
{
	if (on) {
		input_info(true, &ts->client->dev, "%s: power on\n", __func__);
		hideep_pwr_on(ts);
	} else {
		input_info(true, &ts->client->dev, "%s: power off\n", __func__);
		hideep_pwr_off(ts);
	}
}

static int hideep_i2c_read(struct hideep_t *ts, u16 addr, u8 *buf, u16 len)
{
	int ret = -1;
	int retry = SEC_TS_I2C_RETRY_CNT;
	int i;
	u8 *buff;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

	buff = kzalloc(len, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	mutex_lock(&ts->i2c_mutex);

	do {
		/* check power status */

		ret = i2c_master_send(ts->client, (char *)&addr, 2);
		if (ret < 0)
			goto i2c_err;

		usleep_range(200, 210);

		ret = i2c_master_recv(ts->client, buff, len);
		if (ret == len)
			break;

		if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
			input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
			goto i2c_err;
		}

		if (retry < SEC_TS_I2C_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: I2C retry(%d)\n",
					__func__, SEC_TS_I2C_RETRY_CNT - retry);
		}
	} while (--retry);

	mutex_unlock(&ts->i2c_mutex);

	memcpy(buf, buff, len);
	if (ts->debug_flag & HIDEEP_DEBUG_PRINT_I2C_READ_CMD) {
		pr_info("sec_input:i2c_cmd: R: %02x | ", addr);
		for (i = 0; i < len; i++)
			pr_cont("%02x ", buf[i]);
		pr_cont("\n");
	}

	kfree(buff);
	return  0;
i2c_err:
	mutex_unlock(&ts->i2c_mutex);
	kfree(buff);
	return -1;
}

static int hideep_i2c_write(struct hideep_t *ts, u16 addr, u8 *buf, u16 len)
{
	int i = 0;
	int ret = -1;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		return -EIO;
	}

	mutex_lock(&ts->i2c_mutex);

	// data mangling..
	ts->i2c_buf[0] = (addr >> 0) & 0xFF;
	ts->i2c_buf[1] = (addr >> 8) & 0xFF;
	memcpy(&ts->i2c_buf[2], buf, len);

	ret = i2c_master_send(ts->client, (char *)ts->i2c_buf, len + 2);
	if (ret < 0)
		goto i2c_err;

	mutex_unlock(&ts->i2c_mutex);

	if (ts->debug_flag & HIDEEP_DEBUG_PRINT_I2C_WRITE_CMD) {
		pr_info("sec_input:i2c_cmd: W: %02x | ", addr);
		for (i = 0; i < len; i++)
			pr_cont("%02x ", buf[i]);
		pr_cont("\n");
	}

	return  0;

i2c_err:
	mutex_unlock(&ts->i2c_mutex);
	return -1;
}

int hideep_fix_active_mode(struct hideep_t *ts, u8 enable)
{
	int ret;
	u8 cmd;

	cmd = enable;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, enable ? "FIX" : "RELEASE");

	ret = ts->hideep_api->i2c_write(ts, HIDEEP_FIX_ACTIVE_CMD, &cmd, 1);
	if (ret < 0)
		input_info(true, &ts->client->dev, "%s: failed to set mode\n", __func__);

	return ret;
}

void hideep_check_ic_status(struct hideep_t *ts)
{
	struct debug_state *debug_state;
	u8 data[30] = {0};

	ts->hideep_api->i2c_read(ts, HIDEEP_READ_IC_INFO, (u8 *)&data, sizeof(struct debug_state));

	debug_state = (struct debug_state *)data;

	input_info(true, &ts->client->dev, "%s: crc boot:(%x,%x) crc exec (%x,%x), "
		"mode:%d, mode_lock:%d, noise:%d, c:%d, wc:%d, glove:%d, wet:%d, deadz:%d, "
		" module_test:%d, wheel:%d, baseline init p,abn,aot:(%d,%d,%d), "
		"vsync expire count:%d\n", __func__, debug_state->crc_bin_boot,
		debug_state->crc_calc_boot, debug_state->crc_bin_exec,
		debug_state->crc_calc_exec, debug_state->mode_status,
		debug_state->mode_forcing_state, debug_state->noise_mode,
		debug_state->normal_charger, debug_state->wireless_charger,
		debug_state->high_sensitivity_mode, debug_state->wet_mode,
		debug_state->deadzone_expansion_mode, debug_state->module_test_mode,
		debug_state->digital_wheel_mode, debug_state->baseline_init_count_palm,
		debug_state->baseline_init_count_abnomal, debug_state->baseline_init_count_aot,
		debug_state->vsync_abnomal_expire_count);
}

void hideep_print_info(struct hideep_t *ts)
{
	if (!ts)
		return;

	if (!ts->client)
		return;

	ts->print_info_cnt_open++;

	if (ts->print_info_cnt_open > 0xfff0)
		ts->print_info_cnt_open = 0;

	if (ts->touch_count == 0)
		ts->print_info_cnt_release++;

	if (ts->print_info_cnt_open == 1)
		hideep_check_ic_status(ts);

	input_info(true, &ts->client->dev,
			"tc:%d aot:%d glove:%d bezel:%d c:%d wc:%d// v:%02X%02X%02X // #%d %d\n",
			ts->touch_count, ts->aot_enable, ts->glove_mode, ts->bezel_enable,
			ts->charger_mode, ts->wireless_charger_mode,
			ts->p_data->img_version_of_ic[1], ts->p_data->img_version_of_ic[2],
			ts->p_data->img_version_of_ic[3], ts->print_info_cnt_open,
			ts->print_info_cnt_release);
}

static void hideep_print_info_work(struct work_struct *work)
{
	struct hideep_t *ts = container_of(work, struct hideep_t,
			work_print_info.work);

	hideep_print_info(ts);

	if (!ts->shutdown_is_on_going)
		schedule_delayed_work(&ts->work_print_info, msecs_to_jiffies(30000));
}

static void hideep_read_info_work(struct work_struct *work)
{
	struct hideep_t *ts = container_of(work, struct hideep_t,
			work_read_info.work);
	int ii, jj;
	char temp[128];

	hideep_run_rawdata_read_all(ts);
	ts->info_work_done = true;

	snprintf(temp, 128, "[sec_input] hideep_rawdata_start\n");
	strlcat(nbuff, temp, sizeof(nbuff));
	snprintf(temp, 128, "[sec_input] 1rc\n");
	strlcat(nbuff, temp, sizeof(nbuff));
	for (jj = 0; jj < ts->vr_info->num_rx; jj++) {
		snprintf(temp, 128, "[%d] ", jj);
		strlcat(nbuff, temp, sizeof(nbuff));
		for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
			snprintf(temp, 128, "%d ", ts->proc_data[jj * ts->vr_info->num_tx + ii]);
			strlcat(nbuff, temp, sizeof(nbuff));
		}
		snprintf(temp, 128, "\n");
		strlcat(nbuff, temp, sizeof(nbuff));
	}

	snprintf(temp, 128, "[sec_input] selfraw\n");
	strlcat(nbuff, temp, sizeof(nbuff));
	for (ii = 0; ii < ts->vr_info->num_tx + ts->vr_info->num_rx; ii++) {
		snprintf(temp, 128, "%d ", ts->proc_data[(ts->vr_info->num_tx * ts->vr_info->num_rx) + ii]);
		strlcat(nbuff, temp, sizeof(nbuff));
	}
	snprintf(temp, 128, "\n");
	strlcat(nbuff, temp, sizeof(nbuff));

	snprintf(temp, 128, "[sec_input] smart short\n");
	strlcat(nbuff, temp, sizeof(nbuff));
	for (ii = 0; ii < ts->vr_info->num_tx; ii++) {
		snprintf(temp, 128, "%d ", ts->proc_data[(ts->vr_info->num_tx * ts->vr_info->num_rx) + (ts->vr_info->num_tx + ts->vr_info->num_rx) + ii]);
		strlcat(nbuff, temp, sizeof(nbuff));
	}
	snprintf(temp, 128, "\n");
	strlcat(nbuff, temp, sizeof(nbuff));

	if (ts->p_data->model_num >= 5) {
		snprintf(temp, 128, "[sec_input] channel short\n");
		strlcat(nbuff, temp, sizeof(nbuff));
		for (ii = 0; ii < 4; ii++) {
			snprintf(temp, 128, "[sec_input] %s\n", (ii == 0) ? "TX-GND" : (ii == 1) ? "TX-TX" : (ii == 2) ? "RX-GND" : "RX-RX");
			strlcat(nbuff, temp, sizeof(nbuff));
			for (jj = 0; jj < ts->vr_info->num_tx; jj++) {
				snprintf(temp, 128, "%d ", ts->proc_data[((ts->vr_info->num_tx * ts->vr_info->num_rx) +
						(ts->vr_info->num_tx + ts->vr_info->num_rx) + ts->vr_info->num_tx) + ii * ts->vr_info->num_tx + jj]);
				strlcat(nbuff, temp, sizeof(nbuff));
			}
			snprintf(temp, 128, "\n");
			strlcat(nbuff, temp, sizeof(nbuff));
		}
	}

	snprintf(temp, 128, "[sec_input] hideep_rawdata_end\n");
	strlcat(nbuff, temp, sizeof(nbuff));

	if (ts->shutdown_is_on_going) {
		input_err(true, &ts->client->dev, "%s done, do not run work\n", __func__);
		return;
	} else
		schedule_work(&ts->work_print_info.work);
}

static void hideep_release_all_finger(struct hideep_t *ts)
{
	int id;
	int i;

	for (i = 0; i < ts->tch_count; i++) {
		id = (ts->touch_evt[i].index >> 0) & 0x0F;
		input_mt_slot(ts->input_dev, id);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		input_report_key(ts->input_dev, BTN_TOUCH, false);
		input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);
		input_report_key(ts->input_dev, KEY_SLEEP, false);
		ts->palm_flag = 0;

		if ((ts->touch_evt[i].flag == HIDEEP_COORDINATE_ACTION_PRESS) ||
				(ts->touch_evt[i].flag == HIDEEP_COORDINATE_ACTION_MOVE)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
					"[RA] tID:%d dd:%d,%d tc:%d lx:%d ly:%d lz:%d\n",
					id, ts->touch_evt[i].x - ts->p_x,
					ts->touch_evt[i].y - ts->p_y, ts->touch_count,
					ts->touch_evt[i].x, ts->touch_evt[i].y);
#else
			input_info(true, &ts->client->dev,
					"[RA] tID:%d dd:%d,%d tc:%d\n",
					id, ts->touch_evt[i].x - ts->p_x,
					ts->touch_evt[i].y - ts->p_y, ts->touch_count);
#endif
		}
	}

	ts->touch_count = 0;

	input_mt_sync_frame(ts->input_dev);
	input_sync(ts->input_dev);
}

static void  hideep_send_wakeup_event(struct hideep_t *ts)
{
	input_mt_slot(ts->input_dev, 0);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, 100);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, 100);
	input_report_key(ts->input_dev, BTN_TOUCH, 1);
	input_sync(ts->input_dev);

//	msleep(20);

	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_mt_report_pointer_emulation(ts->input_dev, false);
	input_sync(ts->input_dev);
}

static void hideep_read_event(struct hideep_t *ts)
{
	int id;
	int i;
	u8 btn_up = 0;
#ifdef HIDEEP_PROTOCOL_2_0W
	u16 angle = 0;
	u8 bz_cw = 0;
	u8 bz_ccw = 0;
#endif

	for (i = 0; i < ts->tch_count; i++) {
		id = (ts->touch_evt[i].index >> 0) & 0x0F;
		btn_up = (ts->touch_evt[i].flag >> HIDEEP_MT_RELEASED) & 0x01;

#ifdef HIDEEP_PROTOCOL_2_0W
		angle = ts->touch_evt[i].angle;
		bz_cw = (ts->touch_evt[i].flag >> HIDEEP_MT_WHEEL_CW) & 0x01;
		bz_ccw = (ts->touch_evt[i].flag >> HIDEEP_MT_WHEEL_CCW) & 0x01;
#endif

#ifdef HIDEEP_SUPPORT_PALM
		if (ts->touch_evt[i].type == HIDEEP_MT_PALM_TOUCH) {
			ts->palm_flag |= (1 << id);
			input_info(true, &ts->client->dev, "palm %d\n", ts->palm_flag);
			input_report_key(ts->input_dev, KEY_SLEEP, true);
			input_sync(ts->input_dev);
			input_report_key(ts->input_dev, KEY_SLEEP, false);
			input_sync(ts->input_dev);
		}
#endif
		switch (ts->touch_evt[i].type) {
		case HIDEEP_MT_FINGER_TOUCH:
			input_mt_slot(ts->input_dev, id);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, (btn_up == 0));

			if (btn_up == 0) {
				input_report_key(ts->input_dev, BTN_TOUCH, 1);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->touch_evt[i].x);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->touch_evt[i].y);
			}

			if (ts->touch_evt[i].x == 0 && ts->touch_evt[i].y == 0) {
				input_report_key(ts->input_dev, KEY_INT_CANCEL, 1);
				input_sync(ts->input_dev);
				input_report_key(ts->input_dev, KEY_INT_CANCEL, 0);
				input_sync(ts->input_dev);
			}

			if (ts->touch_evt[i].flag == HIDEEP_COORDINATE_ACTION_PRESS) {
				ts->p_x = ts->touch_evt[i].x;
				ts->p_y = ts->touch_evt[i].y;
				ts->touch_count++;

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				input_info(true, &ts->client->dev,
						"[P] tID:%d,%d x:%d y:%d tc:%d type:%X noise:%d, w:%d, g:%d, mode:%d\n",
						id, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
						ts->touch_evt[i].x, ts->touch_evt[i].y,
						ts->touch_count, ts->touch_evt[i].type,
						(ts->fw_state >> 5) & 0x01, (ts->fw_state >> 7) & 0x01,
						(ts->fw_state >> 6) & 0x01, (ts->fw_state >> 3) & 0x03);
#else
				input_info(true, &ts->client->dev,
						"[P] tID:%d,%d tc:%d type:%X noise:%d, w:%d, g:%d, mode:%d\n",
						id, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
						ts->touch_count, ts->touch_evt[i].type,
						(ts->fw_state >> 5) & 0x01, (ts->fw_state >> 7) & 0x01,
						(ts->fw_state >> 6) & 0x01, (ts->fw_state >> 3) & 0x03);
#endif
			} else if (ts->touch_evt[i].flag == HIDEEP_COORDINATE_ACTION_RELEASE) {
				ts->touch_count--;
				if (ts->touch_count <= 0) {
					input_report_key(ts->input_dev, BTN_TOUCH, 0);
					ts->touch_count = 0;
				}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				input_info(true, &ts->client->dev,
						"[R] tID:%d dd:%d,%d tc:%d lx:%d ly:%d noise:%d, w:%d, g:%d, mode:%d\n",
						id, ts->touch_evt[i].x - ts->p_x,
						ts->touch_evt[i].y - ts->p_y, ts->touch_count,
						ts->touch_evt[i].x, ts->touch_evt[i].y,
						(ts->fw_state >> 5) & 0x01, (ts->fw_state >> 7) & 0x01,
						(ts->fw_state >> 6) & 0x01, (ts->fw_state >> 3) & 0x03);
#else
				input_info(true, &ts->client->dev,
						"[R] tID:%d dd:%d,%d tc:%d noise:%d, w:%d, g:%d, mode:%d\n",
						id, ts->touch_evt[i].x - ts->p_x,
						ts->touch_evt[i].y - ts->p_y, ts->touch_count,
						(ts->fw_state >> 5) & 0x01, (ts->fw_state >> 7) & 0x01,
						(ts->fw_state >> 6) & 0x01, (ts->fw_state >> 3) & 0x03);
#endif
			} else if (ts->touch_evt[i].flag == HIDEEP_COORDINATE_ACTION_MOVE) {
				input_dbg(true, &ts->client->dev,
						"[M] tID:%d,%d x:%d y:%d tc:%d type:%X noise:%d, w:%d, g:%d, mode:%d\n",
						id, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
						ts->touch_evt[i].x, ts->touch_evt[i].y,
						ts->touch_count, ts->touch_evt[i].type,
						(ts->fw_state >> 5) & 0x01, (ts->fw_state >> 7) & 0x01,
						(ts->fw_state >> 6) & 0x01, (ts->fw_state >> 3) & 0x03);
			}
			break;

		case HIDEEP_GESTURE_SINGLE_TAP:
			if (ts->aot_enable) {
				hideep_send_wakeup_event(ts);
				input_info(true, &ts->client->dev, "%s: SINGLE TAP\n", __func__);
			}
			break;
		case HIDEEP_MT_WHEEL_TOUCH:
#ifdef HIDEEP_PROTOCOL_2_0W
			if (bz_cw || bz_ccw) {
				hideep_release_all_finger(ts);
				if (ts->p_data->support_bezel_detent && ts->bezel_enable) {
					if (bz_cw) {
						input_info(true, &ts->client->dev, "[CW]: angle: %d\n", angle);
						input_report_rel(ts->wheel_dev, REL_WHEEL, BZ_CW);
						input_report_abs(ts->wheel_dev, ABS_MISC, (angle - 1) * 2);
					} else if (bz_ccw) {
						input_info(true, &ts->client->dev, "[CCW]: angle: %d\n", angle);
						input_report_rel(ts->wheel_dev, REL_WHEEL, BZ_CC);
						input_report_abs(ts->wheel_dev, ABS_MISC, (angle - 1) * 2);
					}
				}
				input_sync(ts->wheel_dev);
			}
#endif
			break;
		default:
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_err(true, &ts->client->dev, "%s: Do not support touch type.(%d)(%d,%d)\n",
					__func__, ts->touch_evt[i].type, ts->touch_evt[i].x, ts->touch_evt[i].y);
#else
			input_err(true, &ts->client->dev, "%s: Do not support touch type.(%d)\n",
					__func__, ts->touch_evt[i].type);
#endif
			break;
		}
	}
	input_mt_sync_frame(ts->input_dev);
	input_sync(ts->input_dev);
}

static int hideep_get_event(struct hideep_t *ts)
{
	int ret;
	int touch_count;
	int info_size;

	ts_log_coord("mt = %d", ts->tch_count);

	/* get touch event information */
	if (ts->tch_count > HIDEEP_MT_MAX)
		ts->tch_count = 0;

	touch_count = ts->tch_count;

	if (ts->tch_count > 0) {
		info_size = ts->tch_count * sizeof(struct hideep_mt_t);
		ret = hideep_i2c_read(ts, HIDEEP_TOUCH_DATA_ADDR, (u8 *)ts->touch_evt, info_size);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: read I2C error\n", __func__);
			goto i2c_err;
		}
	}

	return touch_count;

i2c_err:
	input_err(true, &ts->client->dev, "%s: I2C error\n", __func__);
	return -1;
}

static irqreturn_t hideep_irq_task(int irq, void *handle)
{
	u8 i2c_buff[2];
	int ret = 0;
	int i2c_retry = 0;
	int t_evt;

	struct hideep_t *ts = (struct hideep_t *) handle;

	if (ts->power_status == SEC_INPUT_STATE_LPM) {
		wake_lock_timeout(&ts->wakelock, msecs_to_jiffies(500));

		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret == 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return IRQ_HANDLED;
		}

		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return IRQ_HANDLED;
		}

		input_info(true, &ts->client->dev, "%s: run LPM interrupt handler, %d\n", __func__, ret);
	}

	do {
		ret = hideep_i2c_read(ts, HIDEEP_EVENT_COUNT_ADDR, (u8 *)&i2c_buff, 2);
		i2c_retry = i2c_retry + 1;
	} while ((ret < 0) && i2c_retry < 10);

	if (ret < 0)
		return IRQ_HANDLED;

	ts->tch_count = i2c_buff[0];
	ts->fw_state = i2c_buff[1];

	t_evt = hideep_get_event(ts);

	if (t_evt >= 0 && ts->tch_count > 0)
		hideep_read_event(ts);
	return IRQ_HANDLED;
}

static int hideep_capability(struct hideep_t *ts)
{
	ts->input_dev->name = "sec_touchscreen";
	ts->input_dev->id.bustype = BUS_I2C;

	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(KEY_SLEEP, ts->input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	set_bit(KEY_INT_CANCEL, ts->input_dev->keybit);

	input_mt_init_slots(ts->input_dev, HIDEEP_MT_MAX, INPUT_MT_DIRECT);

	input_set_abs_params(ts->input_dev, ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->p_data->max_x - 1, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->p_data->max_y - 1, 0, 0);

	return 0;
}

static int hideep_setup_bezel_input(struct hideep_t *ts)
{
	int ret = 0;

	ts->wheel_dev = input_allocate_device();
	if (!ts->wheel_dev) {
		input_err(true, &ts->client->dev, "%s: can't allocate memory for wheel_dev\n", __func__);
		return -ENOMEM;
	}

	set_bit(EV_REL, ts->wheel_dev->evbit);
	set_bit(REL_WHEEL, ts->wheel_dev->relbit);
	set_bit(REL_X, ts->wheel_dev->relbit);

	set_bit(EV_KEY, ts->wheel_dev->evbit);
	set_bit(BTN_LEFT, ts->wheel_dev->keybit);

	input_set_capability(ts->wheel_dev, EV_REL, REL_X);
	input_set_capability(ts->wheel_dev, EV_REL, REL_WHEEL);

	/*
	 * 10': 360 / 10 * 2 = 72 : 0 ~ 71
	 * device.res = 36 / (2 * pi) = 5.730
	 * 12': 360 / 12 * 2 = 60 : 0 ~ 59
	 * device.res = 30 / (2 * pi) = 4.774
	 * 15': 360 / 15 * 2 = 48 : 0 ~ 47
	 * device.res = 24 / (2 * pi) = 3.820
	 */
	if (ts->p_data->model_num == 5)
		input_set_abs_params(ts->wheel_dev, ABS_MISC, 0, 59, 0, 0);//12 degree
	else
		input_set_abs_params(ts->wheel_dev, ABS_MISC, 0, 47, 0, 0);//15 degree

	ts->wheel_dev->name = HIDEEP_WHEEL_NAME;
	ts->wheel_dev->id.bustype = BUS_VIRTUAL;

	ret = input_register_device(ts->wheel_dev);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: can't register wheel_dev\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static void hideep_reset_ic(struct hideep_t *ts)
{
	struct hideep_platform_data_t *pdata;
	int ret;
	u8 data[2] = { 0 };
#if 0
	u8 cmd = 0x01;
#endif

	pdata = ts->p_data;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	if (pdata->support_reset_control) {
		if (pdata->reset_gpio > 0) {
			input_info(true, &ts->client->dev, "%s: enable the reset_gpio\n", __func__);
			gpio_set_value(pdata->reset_gpio, 1);
			usleep_range(1000, 1000);
			gpio_set_value(pdata->reset_gpio, 0);
			msleep(20);
			gpio_set_value(pdata->reset_gpio, 1);
			msleep(150);
		}
#if 1	//HW reset
	} else {
		ts->hideep_api->power(ts, false);
		msleep(30);
		ts->hideep_api->power(ts, true);
	}
#else	//SW reset
	} else {
		ret = hideep_i2c_write(ts, HIDEEP_RESET_CMD, &cmd, 1);
		if (ret)
			input_err(true, &ts->client->dev, "%s: reset cmd write failed\n", __func__);

		msleep(150);
	}
#endif
	if (ts->probe_done) {
		if (ts->glove_mode) {
			ret = ts->hideep_api->i2c_write(ts, HIDEEP_GLOVE_MODE_CMD, &ts->glove_mode, 1);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: failed to set glove mode.\n", __func__);
			}
			usleep_range(12000, 12000);
		}
		if (ts->bezel_enable) {
			ret = ts->hideep_api->i2c_write(ts, HIDEEP_BEZEL_ENABLE_CMD, &ts->bezel_enable, 1);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: failed to set bezel enable.\n", __func__);
			}
			usleep_range(12000, 12000);
		}
		if (ts->charger_mode) {
			data[0] = ts->charger_mode;
			data[1] = 0x00;
			ret = ts->hideep_api->i2c_write(ts, HIDEEP_CHARGER_MODE_CMD, data, 2);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: failed to set charger mode.\n", __func__);
			}
			usleep_range(12000, 12000);
		}
		if (ts->wireless_charger_mode) {
			data[0] = ts->wireless_charger_mode;
			data[1] = 0x01;
			ret = ts->hideep_api->i2c_write(ts, HIDEEP_CHARGER_MODE_CMD, data, 2);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: failed to set wireless charger mode.\n", __func__);
			}
			usleep_range(12000, 12000);
		}
	}
}

static void hideep_init_ic(struct hideep_t *ts)
{
	int ret = 0;
	struct hideep_platform_data_t *pdata;

	pdata = ts->p_data;

	if (pdata->support_reset_control) {
		if (pdata->reset_gpio > 0) {
			ret = gpio_request(pdata->reset_gpio, "reset_gpio");
			if (ret)
				input_info(true, &ts->client->dev, "%s: unable to request gpio[%d]\n", __func__, pdata->reset_gpio);
			else
				gpio_direction_output(pdata->reset_gpio, 1);
		}
	}

	gpio_direction_input(pdata->irq_gpio);

	/* power on */
	ts->hideep_api->power(ts, true);
	ts->power_status = SEC_INPUT_STATE_POWER_ON;
	ts->dev_state = state_init;
	if (pdata->regulator_boot_on)
		ts->hideep_api->reset_ic(ts);

}

int hideep_set_lowpowermode(struct hideep_t *ts, u8 mode)
{
	int ret = 0;
	int retrycnt = 0;
	char para = 0;
	struct irq_desc *desc = irq_to_desc(ts->client->irq);

	input_err(true, &ts->client->dev, "%s: %s\n", __func__,
			mode == TO_LOWPOWER_MODE ? "ENTER" : "EXIT");

	if (mode == TO_LOWPOWER_MODE) {
		if (device_may_wakeup(&ts->client->dev))
			enable_irq_wake(ts->client->irq);
	} else {
		if (device_may_wakeup(&ts->client->dev))
			disable_irq_wake(ts->client->irq);
	}

	if (ts->power_status == SEC_INPUT_STATE_LPM) {
		ts->hideep_api->reset_ic(ts);
		ts->power_status = SEC_INPUT_STATE_POWER_ON;
		goto i2c_error;
	}

retry_pmode:
	ret = hideep_i2c_write(ts, HIDEEP_IDLE_MODE_CMD, &mode, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed\n", __func__);
		goto i2c_error;
	}

	if (mode == TO_TOUCH_MODE || retrycnt > 0)
		msleep(100);
	else
		msleep(40);

	ret = hideep_i2c_read(ts, HIDEEP_MODE_STATE_CHECK_REG, &para, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read power mode failed!\n", __func__);
		goto i2c_error;
	} else {
		input_info(true, &ts->client->dev, "%s: power mode - write(%d) read(%d)\n", __func__, mode, para);
	}

	if (mode == TO_LOWPOWER_MODE) {
		if (para != LPM_IDLE) {
			retrycnt++;
			if (retrycnt < 5)
				goto retry_pmode;
			else
				goto mode_change_failed;
		}
	} else {
		if (para == LPM_IDLE) {
			retrycnt++;
			if (retrycnt < 5)
				goto retry_pmode;
			else
				goto mode_change_failed;
		}
	}

	hideep_release_all_finger(ts);	//need to check func whether it needs metex lock or not

	if (mode == TO_LOWPOWER_MODE)
		ts->power_status = SEC_INPUT_STATE_LPM;
	else
		ts->power_status = SEC_INPUT_STATE_POWER_ON;

i2c_error:
	input_info(true, &ts->client->dev, "%s: end %d/%d\n", __func__, ret, desc->depth);
	return ret;

mode_change_failed:
	input_info(true, &ts->client->dev, "%s: fail %d/%d\n", __func__, ret, desc->depth);
	ts->hideep_api->reset_ic(ts);	//temp
	return -1;
}

int hideep_start_device(struct hideep_t *ts)
{
	int ret;
	u8 data[2] = { 0 };

	input_info(true, &ts->client->dev, "%s\n", __func__);

	if (ts->power_status == SEC_INPUT_STATE_POWER_ON) {
		input_err(true, &ts->client->dev, "%s: already power on\n", __func__);
		goto out;
	}

	hideep_release_all_finger(ts);

	ts->hideep_api->power(ts, true);
	ts->power_status = SEC_INPUT_STATE_POWER_ON;

	/* check 98AA */
	ret = hideep_firmware_integrity(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to ic init.\n", __func__);
	}

	if (ts->glove_mode) {
		ret = ts->hideep_api->i2c_write(ts, HIDEEP_GLOVE_MODE_CMD, &ts->glove_mode, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to set glove mode.\n", __func__);
		}
		usleep_range(12000, 12000);
	}
	if (ts->bezel_enable) {
		ret = ts->hideep_api->i2c_write(ts, HIDEEP_BEZEL_ENABLE_CMD, &ts->bezel_enable, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to set bezel enable.\n", __func__);
		}
		usleep_range(12000, 12000);
	}
	if (ts->charger_mode) {
		data[0] = ts->charger_mode;
		data[1] = 0x00;
		ret = ts->hideep_api->i2c_write(ts, HIDEEP_CHARGER_MODE_CMD, data, 2);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to set charger mode.\n", __func__);
		}
		usleep_range(12000, 12000);
	}
	if (ts->wireless_charger_mode) {
		data[0] = ts->wireless_charger_mode;
		data[1] = 0x00;
		ret = ts->hideep_api->i2c_write(ts, HIDEEP_CHARGER_MODE_CMD, data, 2);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to set wireless charger mode.\n", __func__);
		}
		usleep_range(12000, 12000);
	}

	enable_irq(ts->client->irq);
	if (device_may_wakeup(&ts->client->dev))
		enable_irq_wake(ts->client->irq);
out:
	return 0;
}

int hideep_stop_device(struct hideep_t *ts)
{
	input_info(true, &ts->client->dev, "%s\n", __func__);

	if (ts->power_status == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: already power off\n", __func__);
		goto out;
	}

	disable_irq(ts->client->irq);
	if (device_may_wakeup(&ts->client->dev))
		disable_irq_wake(ts->client->irq);


	hideep_release_all_finger(ts);

	ts->power_status = SEC_INPUT_STATE_POWER_OFF;
	ts->hideep_api->power(ts, false);
out:
	return 0;
}

int hideep_resume(struct device *dev)
{
	struct hideep_t *ts = dev_get_drvdata(dev);
	int ret;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	mutex_lock(&ts->dev_mutex);

	if (ts->dev_state == state_normal)
		goto hideep_resume_exit;

	input_info(true, &ts->client->dev, "%s: not waiting.\n", __func__);
	ts->dev_state = state_normal;

	if (ts->power_status == SEC_INPUT_STATE_LPM) {
		hideep_set_lowpowermode(ts, TO_TOUCH_MODE);
	} else {
		ret = hideep_start_device(ts);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: Failed to start device\n", __func__);
	}

	if (ts->fix_active_mode)
		hideep_fix_active_mode(ts, ts->fix_active_mode);

hideep_resume_exit:
	mutex_unlock(&ts->dev_mutex);

	cancel_delayed_work(&ts->work_print_info);
	ts->print_info_cnt_open = 0;
	ts->print_info_cnt_release = 0;
	schedule_work(&ts->work_print_info.work);

	input_info(true, &ts->client->dev, "%s: done(0x%08x)\n", __func__, ts->chip_id_sfr);
	return 0;
}

int hideep_suspend(struct device *dev)
{
	struct hideep_t *ts = dev_get_drvdata(dev);

	input_info(true, &ts->client->dev, "%s\n", __func__);

	mutex_lock(&ts->dev_mutex);
	if (ts->dev_state == state_sleep)
		goto hideep_suspend_exit;

	input_info(true, &ts->client->dev, "%s: not waiting\n", __func__);
	ts->dev_state = state_sleep;

	cancel_delayed_work(&ts->work_print_info);
	hideep_print_info(ts);

	if (ts->aot_enable)
		hideep_set_lowpowermode(ts, TO_LOWPOWER_MODE);
	else
		hideep_stop_device(ts);

hideep_suspend_exit:
	mutex_unlock(&ts->dev_mutex);
	input_info(true, &ts->client->dev, "%s: exit\n", __func__);
	return 0;
}

#ifdef CONFIG_FB
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;

	struct hideep_t *ts = container_of(self, struct hideep_t, fb_notif);

	if ((evdata) && (evdata->data) && (event == FB_EVENT_BLANK) && (ts) && (ts->client)) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK) {
			input_info(true, &ts->client->dev, "%s: resume\n", __func__);
			if (ts->suspended == 1) {
				hideep_resume(&ts->client->dev);
				ts->suspended = 0;
			}
		} else if (*blank == FB_BLANK_POWERDOWN) {
			input_info(true, &ts->client->dev, "%s: suspend\n", __func__);
			if (ts->suspended == 0) {
				ts->suspended = 1;
				hideep_suspend(&ts->client->dev);
			}
		}
	}

	return 0;
}
#endif

static int hideep_load_fw(struct hideep_t *ts, int update_type, const char *file_path)
{
	struct file *fp;
	struct dwz_info_t dwz_info = {0,};
	mm_segment_t old_fs;
	long fw_size, nread;
	int error = 0;
#ifdef SUPPORT_FW_SIGNED
	bool is_fw_signed = false;
	long spu_ret = 0;
	long ori_size = 0;
#endif
	int ret = 0;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_path, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		input_err(true, &ts->client->dev, "%s: failed to open %s.\n", __func__,
				file_path);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (fw_size > 0) {
		unsigned char *fw_data;

		fw_data = vzalloc(fw_size);
		if (!fw_data) {
			input_err(true, &ts->client->dev, "%s: failed to alloc mem\n", __func__);
			error = -ENOMEM;
			filp_close(fp, NULL);
			goto open_err;
		}
		nread = vfs_read(fp, (char __user *)fw_data,
				fw_size, &fp->f_pos);

		input_info(true, &ts->client->dev,
				"%s: start, file path %s, size %ld Bytes\n",
				__func__, file_path, fw_size);

		if (nread != fw_size) {
			input_err(true, &ts->client->dev,
					"%s: failed to read firmware file, nread %ld Bytes\n",
					__func__, nread);
			error = -EIO;
		} else {
			get_dwz_from_binary((u8 *)fw_data, &dwz_info);

			input_info(true, &ts->client->dev, "%s: firmware version %02X%02X%02X\n",
				__func__, dwz_info.model_name[5], (u8)(dwz_info.module_id & 0xFF),
				dwz_info.release_ver);

			if (ts->client->irq) {
				disable_irq(ts->client->irq);
				ts->interrupt_state = 0;
			}

#ifdef SUPPORT_FW_SIGNED
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			if (update_type == TSP_SPU)
				is_fw_signed = true;
#else
			if (update_type == TSP_SPU || update_type == TSP_SDCARD)
				is_fw_signed = true;
#endif

			if (update_type == TSP_VERIFICATION) {
				ori_size = fw_size - SPU_METADATA_SIZE(TSP);
				spu_ret = spu_firmware_signature_verify("TSP", fw_data, fw_size);
				if (spu_ret != ori_size) {
					input_err(true, &ts->client->dev, "%s: signature verify failed, spu_ret:%ld, ori_size:%ld\n",
						__func__, spu_ret, ori_size);
					error = -EPERM;
				}
				goto done;
			} else if (is_fw_signed) {
				ori_size = fw_size - SPU_METADATA_SIZE(TSP);
				if ((update_type == TSP_SPU) && (ts->p_data->img_version_of_ic[0] == (dwz_info.product_code & 0xFF) &&
					ts->p_data->img_version_of_ic[1] == dwz_info.model_name[5] &&
					ts->p_data->img_version_of_ic[2] == (dwz_info.module_id & 0xFF))) {
					if (ts->p_data->img_version_of_ic[3] >= dwz_info.release_ver) {
						input_info(true, &ts->client->dev, "%s: img version: %02X%02X%02X%02X/%08X exit\n",
							__func__, ts->p_data->img_version_of_ic[0], ts->p_data->img_version_of_ic[1],
							ts->p_data->img_version_of_ic[2], ts->p_data->img_version_of_ic[3]);
						error = 0;
						input_info(true, &ts->client->dev, "%s: skip spu\n", __func__);
						goto done;
					} else {
						input_info(true, &ts->client->dev, "%s: run spu\n", __func__);
					}
				} else if ((update_type == TSP_SDCARD) && (ts->p_data->img_version_of_ic[0] == (dwz_info.product_code & 0xFF) &&
					ts->p_data->img_version_of_ic[1] == dwz_info.model_name[5])) {
					input_info(true, &ts->client->dev, "%s: run sfu\n", __func__);
				} else {
					input_info(true, &ts->client->dev, "%s: not matched product version\n", __func__);
					error = -ENOENT;
					goto done;
				}

				spu_ret = spu_firmware_signature_verify("TSP", fw_data, fw_size);
				if (spu_ret != ori_size) {
					input_err(true, &ts->client->dev, "%s: signature verify failed, spu_ret:%ld, ori_size:%ld\n",
						__func__, spu_ret, ori_size);
					error = -1;
					goto done;
				}
			}
#endif

			/* firmup temp -> need to fix it*/
			mutex_lock(&ts->dev_mutex);
			ts->dev_state = state_updating;
			ret = hideep_wr_firmware(ts, fw_data, fw_size, 0, true);
			ts->dev_state = state_normal;
			mutex_unlock(&ts->dev_mutex);
			if (ret < 0) {
				error = -1;
				goto done;
			}
		}

done:
		if (error < 0)
			input_err(true, &ts->client->dev, "%s: failed update firmware\n",
					__func__);

		if (ts->client->irq) {
			enable_irq(ts->client->irq);
			ts->interrupt_state = 1;
		}
		vfree(fw_data);
	}

	filp_close(fp, NULL);

open_err:
	set_fs(old_fs);
	return error;
}

static int hideep_load_fw_from_bin(struct hideep_t *ts)
{
	int error = 0;
	char fw_path[SEC_TS_MAX_FW_PATH];

	if (ts->p_data->bringup == 1) {
		error = -1;
		input_info(true, &ts->client->dev, "%s: can't update for bringup:%d\n",
				__func__, ts->p_data->bringup);
		return error;
	}

	if (ts->p_data->firmware_name)
		snprintf(fw_path, SEC_TS_MAX_FW_PATH, "%s", ts->p_data->firmware_name);
	else
		return 0;

	if (ts->client->irq) {
		disable_irq(ts->client->irq);
		ts->interrupt_state = 0;
	}

	ts->manually_update = true;
	if (hideep_update_all_firmware(ts, fw_path) < 0) {
		error = -1;
	}

	if (ts->client->irq) {
		enable_irq(ts->client->irq);
		ts->interrupt_state = 1;
	}

	return error;
}

int hideep_firmware_update_on_hidden_menu(struct hideep_t *ts, int update_type)
{
	int ret = SEC_ERROR;

	switch (update_type) {
	case TSP_BUILT_IN:
		ret = hideep_load_fw_from_bin(ts);
		break;
	case TSP_SDCARD:
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		ret = hideep_load_fw(ts, update_type, TSP_PATH_EXTERNAL_FW);
#else
		ret = hideep_load_fw(ts, update_type, TSP_PATH_EXTERNAL_FW_SIGNED);
#endif
		break;
	case TSP_SPU:
	case TSP_VERIFICATION:
		ret = hideep_load_fw(ts, update_type, TSP_PATH_SPU_FW_SIGNED);
		break;
	default:
		input_err(true, &ts->client->dev, "%s: Not support command[%d]\n",
				__func__, update_type);
		break;
	}
	return ret;
}

static int hideep_firmware_update_on_probe(struct hideep_t *ts)
{
	struct hideep_platform_data_t *pdata = ts->p_data;
	char fw_path[SEC_TS_MAX_FW_PATH];
	int ret = 0;

	input_info(true, &ts->client->dev, "%s: start\n", __func__);

	if (pdata->bringup == 1) {
		input_info(true, &ts->client->dev, "%s: bringup 1 skip update\n", __func__);
		ts->hideep_api->get_vr_info(ts);
		return 0;
	}

	ret = hideep_firmware_integrity(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fw integrity check. force update\n", __func__);
		ts->manually_update = true;
		ts->chip_id_sfr = HIDEEP_GARNET;
		hideep_config_for_nvm(ts);
	} else {
		ret = ts->hideep_api->config_ic_feature(ts);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to get ic feature. force update\n", __func__);
			ts->manually_update = true;
			ts->chip_id_sfr = HIDEEP_GARNET;
			hideep_config_for_nvm(ts);
		} else {
			ts->manually_update = false;
			ts->hideep_api->get_vr_info(ts);
		}
	}

	disable_irq(ts->client->irq);
	ts->interrupt_state = 0;

	if (ts->p_data->firmware_name)
		snprintf(fw_path, SEC_TS_MAX_FW_PATH, "%s", ts->p_data->firmware_name);
	else {
		input_err(true, &ts->client->dev, "%s: firmware name does not declair in dts\n", __func__);
		return 0;
	}

	ret = ts->hideep_api->update_all(ts, fw_path);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: firmware update failed(%d)\n", __func__, ret);
		goto out;
	}

	ret = ts->hideep_api->config_ic_feature(ts);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: failed to get ic feature\n", __func__);

out:
	enable_irq(ts->client->irq);
	ts->interrupt_state = 1;

	return ret;
}

static int hideep_init_resolution(struct hideep_t *ts)
{
	int ret;
	u16 x_res = 0;
	u16 y_res = 0;
	u16 w_max = 0;
	u16 z_max = 0;

	ret = hideep_i2c_read(ts, 0x0028, (u8 *)&x_res, sizeof(u16));
	if (ret) {
		input_err(true, &ts->client->dev, "%s: fail to read x resolution\n", __func__, ret);
		goto err;
	}

	ret = hideep_i2c_read(ts, 0x002A, (u8 *)&y_res, sizeof(u16));
	if (ret) {
		input_err(true, &ts->client->dev, "%s: fail to read y resolution\n", __func__, ret);
		goto err;
	}

	ret = hideep_i2c_read(ts, 0x0048, (u8 *)&w_max, sizeof(u16));
	if (ret) {
		input_err(true, &ts->client->dev, "%s: fail to read w max\n", __func__, ret);
		goto err;
	}

	ret = hideep_i2c_read(ts, 0x004A, (u8 *)&z_max, sizeof(u16));
	if (ret) {
		input_err(true, &ts->client->dev, "%s: fail to read z max\n", __func__, ret);
		goto err;
	}

	if ((x_res > 0) && (y_res > 0)) {
		ts->p_data->max_x = x_res;
		ts->p_data->max_y = y_res;
		input_info(true, &ts->client->dev, "%s: set resolution x, y: (%d, %d)\n",
			__func__, ts->p_data->max_x, ts->p_data->max_y);
	} else {
		input_err(true, &ts->client->dev, "%s: wrong x,y resolution(%d,%d)\n",
			__func__, x_res, y_res);
		ret = -1;
		goto err;
	}

	if (w_max > 0) {
		ts->p_data->max_w = w_max;
		input_info(true, &ts->client->dev, "%s: set max w: (%d)\n",
			__func__, ts->p_data->max_w);
	} else {
		ts->p_data->max_w = 255;
		input_info(true, &ts->client->dev, "%s: set default max w: (%d)\n",
			__func__, ts->p_data->max_w);
	}

	if (z_max > 0) {
		ts->p_data->max_z = z_max;
		input_info(true, &ts->client->dev, "%s: set max z: (%d)\n",
			__func__, ts->p_data->max_z);
	} else {
		ts->p_data->max_z = 65535;
		input_info(true, &ts->client->dev, "%s: set default max z: (%d)\n",
			__func__, ts->p_data->max_z);
	}

	return 0;

err:
	input_err(true, &ts->client->dev, "%s: set dt x,y resolution(%d,%d)\n",
			__func__, ts->p_data->max_x, ts->p_data->max_y);
	return ret;
}

static int hideep_parse_dts(struct device *dev, struct hideep_platform_data_t *pdata)
{
	int ret = 0;
	u32 coords[4] = {0,};
	struct device_node *np;
	int count = 0;

	np = dev->of_node;

	ret = of_property_read_u32_array(np, "hideep,max_coords", coords, 2);
	if (ret) {
		input_err(true, dev,"%s: Failed to get max_coords property\n", __func__);
		pdata->max_x = 0;
		pdata->max_y = 0;
	} else {
		pdata->max_x = coords[0];
		pdata->max_y = coords[1];
	}

	/* device tree infomation get */
	pdata->irq_gpio = of_get_named_gpio(np, "hideep,irq_gpio", 0);
	if (pdata->irq_gpio < 0) {
		input_err(true, dev, "%s: of_get_named_gpio failed: irq_gpio %d\n", __func__,
				pdata->irq_gpio);
		return -EINVAL;
	}

	pdata->support_reset_control = of_property_read_bool(np, "support_reset_control");
	if (pdata->support_reset_control) {
		pdata->reset_gpio = of_get_named_gpio(np, "hideep,reset_gpio", 0);
		if (pdata->irq_gpio < 0) {
			input_err(true, dev, "%s: of_get_named_gpio failed: reset_gpio %d\n", __func__,
					pdata->reset_gpio);
			return -EINVAL;
		}
	}

	pdata->support_ldo_control = of_property_read_bool(np, "support_ldo_control");
	if (pdata->support_ldo_control) {
		pdata->vcc_vdd = regulator_get(dev, "tsp_avdd_ldo");
		if (IS_ERR_OR_NULL(pdata->vcc_vdd)) {
			input_err(true, dev, "%s: Failed to get %s regulator.\n",
					__func__, "tsp_avdd_ldo");
			ret = PTR_ERR(pdata->vcc_vdd);
			return -EINVAL;
		}
		pdata->vcc_vid = regulator_get(dev, "tsp_io_ldo");
		if (IS_ERR_OR_NULL(pdata->vcc_vid)) {
			input_err(true, dev, "%s: Failed to get %s regulator.\n",
					__func__, "tsp_avdd_ldo");
		}
	}

	ret = of_property_read_string(np, "hideep,interrupt_mode", &pdata->int_mode);
	if (ret) {
		input_err(true, dev, "%s: Not set interrupt mode. set low level", __func__);
		pdata->int_mode = "low-level";
	} else
		input_info(true, dev, "%s: p_data->int_mode = %s", __func__, pdata->int_mode);


	pdata->tsp_id = of_get_named_gpio(np, "hideep,tsp-id_gpio", 0);
	if (gpio_is_valid(pdata->tsp_id)) {
		pdata->tspid_val = gpio_get_value(pdata->tsp_id);
		input_info(true, dev, "%s: TSP_ID : %d\n", __func__, pdata->tspid_val);
	} else {
		input_err(true, dev, "%s: not use tsp-id gpio\n", __func__);
	}

	count = of_property_count_strings(np, "hideep,firmware_name");
	if (count <= 0) {
		pdata->firmware_name = NULL;
	} else {
		if (gpio_is_valid(pdata->tsp_id)) {
			of_property_read_string_index(np, "hideep,firmware_name",
							pdata->tspid_val, &pdata->firmware_name);
		} else {
			of_property_read_string_index(np, "hideep,firmware_name",
							0, &pdata->firmware_name);
		}
	}

	pdata->support_bezel_detent = of_property_read_bool(np, "support_bezel_detent");
	pdata->regulator_boot_on = of_property_read_bool(np, "sec,regulator_boot_on");

	if (of_property_read_u32(np, "hideep,bringup", &pdata->bringup) < 0)
		pdata->bringup = 0;
	if (of_property_read_u32(np, "hideep,model_num", &pdata->model_num) < 0)
		pdata->model_num = 4;

	input_info(true, dev, "%s: max(%d/%d) FW:%s, HW reset:%d, ldo control:%d Bezel:%d Bringup:%d, model_num:%d\n",
			__func__, pdata->max_x, pdata->max_y, pdata->firmware_name,
			pdata->support_reset_control,	pdata->support_ldo_control,
			pdata->support_bezel_detent, pdata->bringup, pdata->model_num);

	return 0;
}

static int hideep_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct hideep_platform_data_t *p_data;
	struct vr_info_t *vr_info;
	struct hideep_function_list_t *function;
	struct hideep_t *ts;

	logflags = HIDEEP_LOG_SETTING;

	input_info(true, &client->dev, "%s\n", __func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		input_err(true, &client->dev, "%s : Do not load driver due to : lpm %d\n",
				__func__, lpcharge);
		return -ENODEV;
	}
#endif

	/* check whether i2c bus is available or not */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: EIO err!\n", __func__);
		ret = -ENODEV;
		return ret;
	}

	/* init platform data structure */
	p_data = devm_kzalloc(&client->dev, sizeof(struct hideep_platform_data_t), GFP_KERNEL);
	if (!p_data) {
		ret = -ENOMEM;
		input_err(true, &client->dev, "%s: Failed to allocate platform data\n", __func__);
		goto hideep_probe_pdata_memory_err;
	}

	/* init hideep_t structure */
	ts = devm_kzalloc(&client->dev, sizeof(struct hideep_t), GFP_KERNEL);
	if (!ts) {
		ret = -ENOMEM;
		goto hideep_probe_ts_memory_err;
	}

	/* init vr_info memory */
	vr_info = devm_kzalloc(&client->dev, sizeof(struct vr_info_t), GFP_KERNEL);
	function = devm_kzalloc(&client->dev, sizeof(struct hideep_function_list_t), GFP_KERNEL);

	ts->client = client;
	ts->p_data = p_data;
	ts->vr_info = vr_info;
	ts->hideep_api = function;

	ts->hideep_api->i2c_read = hideep_i2c_read;
	ts->hideep_api->i2c_write = hideep_i2c_write;
	ts->hideep_api->reset_ic = hideep_reset_ic;
	ts->hideep_api->power = hideep_power;

	ret = hideep_parse_dts(&client->dev, p_data);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: parse dt failed\n", __func__);
		goto hideep_probe_ts_parse_dt_err;
	}

	INIT_DELAYED_WORK(&ts->work_print_info, hideep_print_info_work);
	INIT_DELAYED_WORK(&ts->work_read_info, hideep_read_info_work);

	i2c_set_clientdata(client, ts);

	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->dev_mutex);

	wake_lock_init(&ts->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");
	init_completion(&ts->resume_done);
	complete_all(&ts->resume_done);

	/* hardware init : reset & power control */
	hideep_init_ic(ts);

	/* init for isp function (pgm mode setting), checking of ic type and then set up flash information */
	hideep_isp_init(ts);

	ret = hideep_firmware_update_on_probe(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fw update failed\n", __func__);
		goto hideep_probe_fw_update;
	}

	hideep_init_resolution(ts);

	/* init input device */
	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		input_err(true, &ts->client->dev, "%s: can't allocate memory for input_dev\n", __func__);
		ret = -ENOMEM;
		goto hideep_probe_input_dev_memory_err;
	}

	if (ts->p_data->support_bezel_detent) {
		/* init bezel wheel */
		hideep_setup_bezel_input(ts);
	}

	/* init min-max range of events */
	hideep_capability(ts);

	ret = input_register_device(ts->input_dev);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: can't register input_dev\n", __func__);
		ret = -ENOMEM;
		goto hideep_probe_register_input_dev_err;
	}

	input_set_drvdata(ts->input_dev, ts);

	/* interrupt set */
	ts->client->irq = gpio_to_irq(p_data->irq_gpio);
	input_info(true, &ts->client->dev, "%s: irq: %d\n", __func__, ts->client->irq);

	if (ts->client->irq <= 0) {
		input_err(true, &ts->client->dev, "%s: can't be assigned irq\n", __func__);
		goto hideep_probe_assigned_irq_err;
	}

	if (ts->client->irq) {
		ret = request_threaded_irq(ts->client->irq, NULL, hideep_irq_task,
				(IRQF_TRIGGER_LOW | IRQF_ONESHOT),
				ts->client->name,
				ts);

		disable_irq(ts->client->irq);
		ts->interrupt_state = 0;
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: fail to get irq(0x%08x)\n", __func__, ret);
			goto hideep_probe_request_irq_err;
		}
	}

	device_init_wakeup(&ts->client->dev, true);

	/* debug option : for JNI interface */
	ret = hideep_debug_init(ts);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: fail init hideep debug(0x%x)\n", __func__, ret);
		ret = -1;
		goto hideep_probe_debug_init_err;
	}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	ret = hideep_sysfs_init(ts);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: fail init hideep sys(0x%x)\n", __func__, ret);
		ret = -1;
		goto hideep_probe_sysfs_init_err;
	}
#endif

	ret = sec_ts_fn_init(ts);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: fail to init fn\n", __func__);
		goto hideep_probe_fn_init_err;
	}

	ret = sysfs_create_group(&ts->input_dev->dev.kobj, &input_attr_group);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to create enabled node\n", __func__);

#ifdef CONFIG_FB
	ts->suspended = 0;
	ts->fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&ts->fb_notif);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: Unable to register fb_notifier(%d)\n",
				__func__, ret);
		ret = -1;
		goto hideep_probe_register_fb_err;
	}
#endif

	hideep_proc_init(ts);

	ts->dev_state = state_normal;
	enable_irq(ts->client->irq);
	ts->interrupt_state = 1;

	ts->probe_done = true;
	ts->enabled = true;

	input_info(true, &ts->client->dev, "%s: done\n", __func__);

	schedule_work(&ts->work_read_info.work);

	return 0;

#ifdef CONFIG_FB
hideep_probe_register_fb_err:
#endif
hideep_probe_fn_init_err:
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	hideep_sysfs_exit(ts);
hideep_probe_sysfs_init_err:
#endif
	hideep_debug_uninit();

hideep_probe_debug_init_err:
hideep_probe_request_irq_err:
	free_irq(ts->client->irq, ts);

hideep_probe_assigned_irq_err:
	input_unregister_device(ts->input_dev);
	if (ts->p_data->support_bezel_detent)
		input_unregister_device(ts->wheel_dev);

hideep_probe_register_input_dev_err:
	input_free_device(ts->input_dev);
	if (ts->p_data->support_bezel_detent)
		input_free_device(ts->wheel_dev);

hideep_probe_input_dev_memory_err:
hideep_probe_fw_update:
	wake_lock_destroy(&ts->wakelock);
	ts->hideep_api->power(ts, false);
hideep_probe_ts_parse_dt_err:
	regulator_put(p_data->vcc_vdd);
	if (!IS_ERR_OR_NULL(p_data->vcc_vid))
		regulator_put(p_data->vcc_vid);
hideep_probe_ts_memory_err:
hideep_probe_pdata_memory_err:
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, ret);
	return ret;
}

static int hideep_remove(struct i2c_client *client)
{
	struct hideep_t *ts = i2c_get_clientdata(client);

	input_info(true, &ts->client->dev, "%s\n", __func__);
	/* need to move in shutdown func */
	ts->shutdown_is_on_going = true;

	cancel_delayed_work_sync(&ts->work_print_info);

#ifdef CONFIG_FB
	if (fb_unregister_client(&ts->fb_notif))
		input_err(true, &ts->client->dev, "%s: Error occurred while unregistering fb_notifier\n", __func__);
#endif

	ts->hideep_api->power(ts, false);
	ts->power_status = SEC_INPUT_STATE_POWER_OFF;
	free_irq(client->irq, ts);

	input_unregister_device(ts->input_dev);
	input_free_device(ts->input_dev);
	if (ts->p_data->support_bezel_detent) {
		input_unregister_device(ts->wheel_dev);
		input_free_device(ts->wheel_dev);
	}
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	hideep_sysfs_exit(ts);
#endif
	sec_ts_fn_remove(ts);

	wake_lock_destroy(&ts->wakelock);

	hideep_debug_uninit();

	regulator_put(ts->p_data->vcc_vdd);
	if (!IS_ERR_OR_NULL(ts->p_data->vcc_vid))
		regulator_put(ts->p_data->vcc_vid);

	return 0;
}
/*
static void hideep_shutdown(struct i2c_client *client)
{
	struct hideep_t *ts = i2c_get_clientdata(client);

	input_info(true, &ts->client->dev, "%s\n", __func__);

	hideep_remove(client);
}
*/
static const struct i2c_device_id hideep_dev_idtable[] = {
	{ HIDEEP_I2C_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, hideep_dev_idtable);

#ifdef CONFIG_OF
static const struct of_device_id hideep_match_table[] = {
	{.compatible = "hideep,hideep_ts"},
	{},
};
MODULE_DEVICE_TABLE(of, hideep_match_table);
#endif

#ifdef CONFIG_PM
static int hideep_pm_suspend(struct device *dev)
{
	struct hideep_t *ts = dev_get_drvdata(dev);

	input_dbg(true, &ts->client->dev, "%s\n", __func__);

	reinit_completion(&ts->resume_done);

	return 0;
}

static int hideep_pm_resume(struct device *dev)
{
	struct hideep_t *ts = dev_get_drvdata(dev);

	input_dbg(true, &ts->client->dev, "%s\n", __func__);

	complete_all(&ts->resume_done);

	return 0;
}

static const struct dev_pm_ops hideep_pm_ops = {
	.suspend = hideep_pm_suspend,
	.resume = hideep_pm_resume,
};
#endif

static struct i2c_driver hideep_driver = {
	.probe = hideep_probe,
	.remove = hideep_remove,
//	.shutdown	= hideep_shutdown,
	.id_table = hideep_dev_idtable,
	.driver = {
		.name = HIDEEP_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hideep_match_table),
#ifdef CONFIG_PM
		.pm = &hideep_pm_ops,
#endif
	},
};

static int hideep_init(void)
{
	pr_info("[sec_input] %s: enter\n", __func__);

	return i2c_add_driver(&hideep_driver);
}

static void hideep_exit(void)
{
	i2c_del_driver(&hideep_driver);
}

module_init(hideep_init);
module_exit(hideep_exit);

MODULE_DESCRIPTION("Driver for HiDeep Touchscreen Controller");
MODULE_AUTHOR("logan.ji@hideep.com");
MODULE_VERSION("1.5");
MODULE_LICENSE("GPL");
