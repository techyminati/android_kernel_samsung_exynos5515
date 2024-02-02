/*
 * sb_wrl_fw.h
 * Samsung Mobile Wireless Firmware Header
 *
 * Copyright (C) 2022 Samsung Electronics, Inc.
 *
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

#ifndef __SB_WRL_FW_H
#define __SB_WRL_FW_H __FILE__

#include <linux/err.h>

/* Sec Battery Wireless Charger - Firmware */
struct sb_wrl_fw_data {
	unsigned char *data;
	unsigned long size;
};

struct sb_wrl_fw;
struct i2c_client;

typedef void (*cb_fw_result)(struct i2c_client *i2c, int result);

enum sb_wrl_info_mode {
	SB_WRL_OTP_FIRM_RESULT = 0,
	SB_WRL_IC_REVISION,
	SB_WRL_IC_GRADE,
	SB_WRL_OTP_FIRM_VER_BIN,
	SB_WRL_OTP_FIRM_VER,
	SB_WRL_OTP_FIRM_VERIFY,
};

enum sb_wrl_fw_update_mode {
	SB_WRL_RX_SDCARD_MODE = 0,
	SB_WRL_RX_BUILT_IN_MODE,
};

enum sb_wrl_fw_result {
	SB_WRL_FW_RESULT_INIT = -1,
	SB_WRL_FW_RESULT_FAIL,
	SB_WRL_FW_RESULT_PASS,
	SB_WRL_FW_RESULT_RUNNING
};

static inline const char *get_fw_result_str(int result)
{
	switch (result) {
	case SB_WRL_FW_RESULT_INIT:
		return "Init";
	case SB_WRL_FW_RESULT_FAIL:
		return "Fail";
	case SB_WRL_FW_RESULT_PASS:
		return "Pass";
	case SB_WRL_FW_RESULT_RUNNING:
		return "Run";
	}

	return "Unknown";
}

#define SB_WRL_FIRMWARE_DISABLE	(-3000)
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
struct sb_wrl_fw *sb_wrl_fw_register(struct i2c_client *i2c, cb_fw_result func);
int sb_wrl_fw_get_version(struct sb_wrl_fw *fw, int *version);
int sb_wrl_fw_get_state(struct sb_wrl_fw *fw, int *state);
int sb_wrl_fw_update(struct sb_wrl_fw *fw, int mode);
#else
static inline struct sb_wrl_fw *sb_wrl_fw_register(struct i2c_client *i2c, cb_fw_result func)
{ return ERR_PTR(SB_WRL_FIRMWARE_DISABLE); }

static inline int sb_wrl_fw_get_version(struct sb_wrl_fw *fw, int *version)
{ return SB_WRL_FIRMWARE_DISABLE; }

static inline int sb_wrl_fw_get_state(struct sb_wrl_fw *fw, int *state)
{ return SB_WRL_FIRMWARE_DISABLE; }

static inline int sb_wrl_fw_update(struct sb_wrl_fw *fw, int mode)
{ return SB_WRL_FIRMWARE_DISABLE; }
#endif

#endif /* __SB_WRL_FW_H */

