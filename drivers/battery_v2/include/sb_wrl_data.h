/*
 * sb_wrl_data.h
 * Samsung Mobile Wireless TRX Data Define Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
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

#ifndef __SB_WRL_DATA_H
#define __SB_WRL_DATA_H __FILE__

/* Sec Battery Wireless Charger - TRX data */
#define SB_WRL_TX_DATA_CMD_UNKNOWN		0x00
#define SB_WRL_TX_DATA_CMD_TX_ID		0x01
#define SB_WRL_TX_DATA_CMD_AFC_TX		0x02
#define SB_WRL_TX_DATA_CMD_ACK			0x03
#define SB_WRL_TX_DATA_CMD_NAK			0x04
#define SB_WRL_TX_DATA_CMD_CHG_STOP		0x05
#define SB_WRL_TX_DATA_CMD_RX_PWR		0x0A
#define SB_WRL_TX_DATA_CMD_TX_PWR_BUDG	0x0C
#define SB_WRL_TX_DATA_CMD_INCOMP_PAD	0x19

#define SB_WRL_RX_DATA_CMD_UNKNOWN		0x00
#define SB_WRL_RX_DATA_CMD_REQ_TX_ID	0x01
#define SB_WRL_RX_DATA_CMD_CHG_STATUS	0x05
#define SB_WRL_RX_DATA_CMD_AFC_SET		0x06
#define SB_WRL_RX_DATA_CMD_AFC_DEBOUNCE	0x07
#define SB_WRL_RX_DATA_CMD_SID_TAG		0x08
#define SB_WRL_RX_DATA_CMD_SID_TOKEN	0x09
#define SB_WRL_RX_DATA_CMD_TX_STANBY	0x0A
#define SB_WRL_RX_DATA_CMD_LED_CTRL		0x0B
#define SB_WRL_RX_DATA_CMD_REQ_AFC		0x0C
#define SB_WRL_RX_DATA_CMD_FAN_CTRL		0x0D
#define SB_WRL_RX_DATA_CMD_RX_ID		0x0E
#define SB_WRL_RX_DATA_CMD_REQ_TX_PWR_BUDG	0x21

#define SB_WRL_TX_CHG_STOP_TX_OTP		0x12
#define SB_WRL_TX_CHG_STOP_TX_OCP		0x13
#define SB_WRL_TX_CHG_STOP_DARKZONE		0x14
#define SB_WRL_TX_CHG_STOP_FOD			0x20 ... 0x27
#define SB_WRL_TX_CHG_STOP_AUTH_FAIL	0x30
#define SB_WRL_TX_CHG_STOP_DISCON_FOD	0x31
#define SB_WRL_TX_CHG_STOP_OC_FOD		0x32
#define SB_WRL_TX_CHG_STOP_WATCHDOG		0x33

/* value of TX POWER BUDGET */
#define SB_WRL_TX_PWR_BUDG_NONE		0x00
#define SB_WRL_TX_PWR_BUDG_2W		0x14
#define SB_WRL_TX_PWR_BUDG_5W		0x32

/* value of RX POWER BUDGET */
#define SB_WRL_RX_PWR_2W		0x14
#define SB_WRL_RX_PWR_5W		0x32

struct sb_wrl_trx_data {
	unsigned char	cmd;
	unsigned char	value[7];
};

static inline const char *get_tx_chg_stop_str(unsigned char data)
{
	switch (data) {
	case SB_WRL_TX_CHG_STOP_TX_OTP:
		return "TX OTP";
	case SB_WRL_TX_CHG_STOP_TX_OCP:
		return "TX OCP";
	case SB_WRL_TX_CHG_STOP_DARKZONE:
		return "DARKZONE";
	case SB_WRL_TX_CHG_STOP_FOD:
		return "FOD";
	case SB_WRL_TX_CHG_STOP_AUTH_FAIL:
		return "AUTH FAIL";
	case SB_WRL_TX_CHG_STOP_DISCON_FOD:
		return "DISCON FOD";
	case SB_WRL_TX_CHG_STOP_OC_FOD:
		return "OC FOD";
	case SB_WRL_TX_CHG_STOP_WATCHDOG:
		return "WATCHDOG";
	}

	return "RESERVED";
}

#define auth_log(str, ...)	pr_info("[WC-AUTH]:%s: "str, __func__, ##__VA_ARGS__)

/* Sec Battery Wireless Charger - MISC data */
enum sb_wrl_misc_dir_type {
	SB_WRL_MISC_DIR_OUT = 0,
	SB_WRL_MISC_DIR_IN,
};

struct sb_wrl_misc_data {
	unsigned short pid; /* Product ID */
	char type; /* swam_data_type */
	char dir; /* swam_direction_type */
	unsigned int size; /* data size */
#ifdef CONFIG_COMPAT
	compat_uptr_t data; /* data pointer */
#else
	void __user *data; /* data pointer */
#endif
};

#define SB_WRL_MISC_CMD _IOWR('B', 0, struct sb_wrl_misc_data)

/* Sec Battery Wireless Charger - AUTH */
enum sb_wrl_auth_status {
	SB_WRL_AUTH_NONE = 0,
	SB_WRL_AUTH_START,
	SB_WRL_AUTH_RECEIVED,
	SB_WRL_AUTH_SENT,
	SB_WRL_AUTH_FAIL,
	SB_WRL_AUTH_ERROR,
	SB_WRL_AUTH_TIMEOUT,

	SB_WRL_AUTH_PASS = 8,
	SB_WRL_AUTH_RETRY,
};


static inline const char *get_auth_status_str(int data)
{
	switch (data) {
	case SB_WRL_AUTH_NONE:
		return "None";
	case SB_WRL_AUTH_START:
		return "Start";
	case SB_WRL_AUTH_RECEIVED:
		return "Received";
	case SB_WRL_AUTH_SENT:
		return "Sent";

	case SB_WRL_AUTH_FAIL:
		return "Fail";
	case SB_WRL_AUTH_ERROR:
		return "Error";
	case SB_WRL_AUTH_TIMEOUT:
		return "Timeout";

	case SB_WRL_AUTH_PASS:
		return "Pass";
	case SB_WRL_AUTH_RETRY:
		return "Retry";
	}

	return "None";
}

#endif /* __SB_WRL_DATA_H */

