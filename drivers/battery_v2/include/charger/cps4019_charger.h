/*
 * cps4019_charger.h
 * Samsung CPS4019 Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
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

#ifndef __CPS4019_CHARGER_H
#define __CPS4019_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include "../sec_charging_common.h"
#include "../sec_battery_vote.h"
#include "../sb_wrl_def.h"
#include "../sb_wrl_data.h"
#include "../sb_wrl_fw.h"
#include "../../core/sb_notify.h"
#include "../../core/sb_pqueue.h"
#include "../../core/sb_misc.h"

extern unsigned int lpcharge;

#define CPS4019_FW_MAJOR_REV_L_REG		0x0004
#define CPS4019_FW_MAJOR_REV_H_REG		0x0005
#define CPS4019_FW_MINOR_REV_L_REG		0x0006
#define CPS4019_FW_MINOR_REV_H_REG		0x0007

/* INT & STATUS L */
#define CPS4019_STAT_VOUT_MASK			(1 << 7)
#define CPS4019_STAT_VRECT_MASK			(1 << 6)
#define CPS4019_OP_MODE_MASK			(1 << 5)
#define CPS4019_OVER_VOLTAGE_MASK		(1 << 4)
#define CPS4019_OVER_CURRENT_MASK		(1 << 3)
#define CPS4019_OVER_TEMP_MASK			(1 << 2)
#define CPS4019_UNDER_VOLTAGE_MASK		(1 << 1)
#define CPS4019_TX_DATA_RECV_MASK		(1)
/* INT & STATUS H */
#define CPS4019_AC_MISSING_DET_MASK		(1 << 7)
#define CPS4019_DATA_STORE_MASK			(1 << 6)
#define CPS4019_GP2_SAMPLE_DONE_MASK	(1 << 5)
#define CPS4019_ADT_SENT_MASK			(1 << 3)
#define CPS4019_ADT_RECV_MASK			(1 << 2)
#define CPS4019_ADT_ERROR_MASK			(1 << 1)
#define CPS4019_ADT_DATA_MASK			(0x7 << 1)
#define CPS4019_APP2_REQUEST_MASK		(1)

#define CPS4019_STATUS_L_REG			0x0020
#define CPS4019_STATUS_H_REG			0x0021
#define CPS4019_INT_L_REG				0x0022
#define CPS4019_INT_H_REG				0x0023
#define CPS4019_INT_ENABLE_L_REG		0x0024
#define CPS4019_INT_ENABLE_H_REG		0x0025
#define CPS4019_INT_CLEAR_L_REG			0x0026
#define CPS4019_INT_CLEAR_H_REG			0x0027

#define CPS4019_SYS_OP_MODE_REG			0x0028
#define CPS4019_SYS_OP_MODE_INIT		0
#define CPS4019_SYS_OP_MODE_WPC			1

#define CPS4019_CHG_STATUS_REG			0x002C
#define CPS4019_EPT_REG					0x002D
/* End of Power Transfer Register, EPT (0x3B) (RX only) */
#define CPS4019_EPT_UNKNOWN				0
#define CPS4019_EPT_END_OF_CHG			1
#define CPS4019_EPT_INT_FAULT			2
#define CPS4019_EPT_OVER_TEMP			3
#define CPS4019_EPT_OVER_VOL			4
#define CPS4019_EPT_OVER_CURR			5
#define CPS4019_EPT_BATT_FAIL			6
#define CPS4019_EPT_RECONFIG			7

#define CPS4019_VOUT_SET_L_REG			0x0030
#define CPS4019_VOUT_SET_H_REG			0x0031
#define CPS4019_VRECT_ADJ_REG			0x0032
#define CPS4019_ADC_VOUT_L_REG			0x0034
#define CPS4019_ADC_VOUT_H_REG			0x0035
#define CPS4019_ADC_VRECT_L_REG			0x0036
#define CPS4019_ADC_VRECT_H_REG			0x0037
#define CPS4019_ADC_IOUT_L_REG			0x0038
#define CPS4019_ADC_IOUT_H_REG			0x0039
#define CPS4019_ADC_VSYS_L_REG			0x003A
#define CPS4019_ADC_VSYS_H_REG			0x003B
#define CPS4019_ADC_DIE_TEMP_L_REG		0x003C
#define CPS4019_ADC_DIE_TEMP_H_REG		0x003D
#define CPS4019_ADC_OP_FREQ_L_REG		0x003E
#define CPS4019_ADC_OP_FREQ_H_REG		0x003F
#define CPS4019_ADC_PING_FREQ_L_REG		0x0040
#define CPS4019_ADC_PING_FREQ_H_REG		0x0041
enum {
	CPS4019_ADC_VOUT = 0,
	CPS4019_ADC_VRECT,
	CPS4019_ADC_IOUT,
	CPS4019_ADC_VSYS,	/* VBAT */
	CPS4019_ADC_DIE_TEMP,
	CPS4019_ADC_OP_FRQ,
	CPS4019_ADC_PING_FRQ,
};

#define CPS4019_DC_VOUT_HDR_REG			0x0044
#define CPS4019_FX_VOUT_HDR_REG			0x0045
#define CPS4019_DC_VOUT_HIGHEST_L_REG	0x0046
#define CPS4019_DC_VOUT_HIGHEST_H_REG	0x0047

#define CPS4019_COMMAND_L_REG			0x0050
#define CPS4019_CMD_WATCHDOG_EN_MASK	(1 << 7)
#define CPS4019_CMD_WATCHDOG_RST_MASK	(1 << 6)
#define CPS4019_CMD_CLEAR_INT_MASK		(1 << 5)
#define CPS4019_CMD_SEND_CHG_STS_MASK	(1 << 4)
#define CPS4019_CMD_SEND_EOP_MASK		(1 << 3)
#define CPS4019_CMD_MCU_RST_MASK		(1 << 2)
#define CPS4019_CMD_TOGGLE_LDO_MASK		(1 << 1)
#define CPS4019_CMD_SEND_RX_DATA_MASK	(1)

#define CPS4019_COMMAND_H_REG				0x0051
#define CPS4019_CMD_SEND_ADT_MASK			(1 << 2)
#define CPS4019_CMD_SAMPLE_CALL_VOLT_MASK	(1 << 1)
#define CPS4019_CMD_SAMPLE_LOAD_VOLT_MASK	(1)

#define CPS4019_PPP_HEADER_REG			0x0052
#define CPS4019_HEADER_ESS				0x01	/* END SIGNAL STRENGTH */
#define CPS4019_HEADER_EPT				0x02	/* END POWER TRANSFER */
#define CPS4019_HEADER_ECE				0x03	/* END CONTROL ERROR */
#define CPS4019_HEADER_ERP				0x04	/* END RECEIVED POWER */
#define CPS4019_HEADER_ECS				0x05	/* POWER CONTROL HOLD */
#define CPS4019_HEADER_PCH_OFF			0x06
#define CPS4019_HEADER_AFC_CONF			0x28
#define CPS4019_HEADER_CONFIGURATION	0x51
#define CPS4019_HEADER_IDENTIFICATION	0x71
#define CPS4019_HEADER_EXTENDED_IDENT	0x81

#define CPS4019_RX_DATA_COM_REG			0x0053
#define CPS4019_RX_DATA_VALUE_REG		0x0054
#define CPS4019_RX_DATA_MAX				7

#define CPS4019_TX_DATA_COM_REG			0x005B
#define CPS4019_TX_DATA_VALUE_REG		0x005C
#define CPS4019_TX_DATA_MAX				3

#define CPS4019_RECT_MODE_REG			0x006A

#define CPS4019_VOUT_MODE_REG			0x006B
#define CPS4019_VOUT_MODE_SHIFT			0
#define CPS4019_VOUT_MODE_MASK			(0x7 << CPS4019_VOUT_MODE_SHIFT)

#define CPS4019_VBAT_HEADROOM_REG		0x006D

#define CPS4019_VOUT_LOWEST_L_REG		0x006F
#define CPS4019_VOUT_LOWEST_H_REG		0x0070

#define CPS4019_DATA_MODE_REG			0x0076
#define CPS4019_DATA_STORE_EN_MASK		(1 << 7)

#define CPS4019_BT_VOUT_HDR_REG			0x007E
#define CPS4019_I_CHG_SET_REG			0x007F

#define CPS4019_ADT_TYPE_REG			0x0200
#define CPS4019_ADT_TYPE_SHIFT			3
enum {
	CPS4019_ADT_TYPE_END		= 0,
	CPS4019_ADT_TYPE_GENERAL,
	CPS4019_ADT_TYPE_AUTH,

	CPS4019_ADT_TYPE_RESET		= 0x12
};

#define CPS4019_ADT_DATA_SIZE_REG		0x0201
#define CPS4019_ADT_DATA_L_REG			0x0202
#define CPS4019_ADT_DATA_H_REG			0x0265
#define CPS4019_ADT_MAX_SIZE			(CPS4019_ADT_DATA_H_REG - CPS4019_ADT_DATA_L_REG)

#define CPS_IRQ_DISWAKE		(1 << 4)
#define CPS_IRQ_ENWAKE		(1 << 3)
#define CPS_IRQ_DISNOSYNC	(1 << 2)
#define CPS_IRQ_DIS			(1 << 1)
#define CPS_IRQ_EN			(1)

struct cps4019_gpio {
	const char *name;
	int gpio;
	int irq;
	unsigned int flags;
	unsigned int state;
};

enum {
	CPS4019_GPIO_GP1 = 0,
	CPS4019_GPIO_PDETB,
	CPS4019_GPIO_INTB,

	CPS4019_GPIO_MAX
};

struct cps4019_charger_platform_data {
	char *wireless_charger_name;
	char *charger_name;
	char *fuelgauge_name;
	char *wireless_name;
	char *battery_name;

	struct cps4019_gpio gpios[CPS4019_GPIO_MAX];
	int wpc_en;
	int wpc_ping_en;

	int cc_cv_threshold; /* cc/cv threshold */
	unsigned int half_bridge_threshold;
	unsigned int half_bridge_vout;
	unsigned int darkzone_expired_time;
	unsigned int low_current_expired_time;
	unsigned int ab_acok_count;
	unsigned int ab_acok_time;

	bool default_voreg;

	bool ic_on_mode;
	int wc_ic_grade;
	int wc_ic_rev;
	int otp_firmware_ver;
	int vout;
	int vrect;

	int tx_off_high_temp;
	int ping_duration;
	int rx_id;
	int support_legacy_pad;

	union sb_wrl_vm boot_mode;

	struct sb_wrl_tx *tx_table;
	unsigned int tx_table_size;
};

#define CPS4019_OP_FREQ_CHECK_BUF_NUM		3
#define cps4019_charger_platform_data_t \
	struct cps4019_charger_platform_data

struct cps4019_charger_data {
	struct i2c_client				*client;
	struct device					*dev;
	cps4019_charger_platform_data_t	*pdata;
	struct mutex io_lock;
	struct mutex charger_lock;
	struct mutex auth_lock;

	struct sb_wrl_fw *fw;
	struct sb_pqueue *pq;
	struct sb_misc *misc;

	struct mutex gpio_lock;
	int det_gpio_type;

	struct power_supply *psy_chg;
	struct wakeup_source *wpc_ws;
	struct wakeup_source *det_ws;
	struct wakeup_source *wpc_data_check_ws;
	struct wakeup_source *wpc_id_check_ws;
	struct workqueue_struct *wqueue;
	struct delayed_work	init_work;
	struct delayed_work	wpc_det_work;
	struct delayed_work	wpc_isr_work;
	struct delayed_work wpc_id_check_work;

	struct wakeup_source *wpc_id_request_ws;
	struct delayed_work	wpc_id_request_work;

	struct wakeup_source *power_hold_chk_ws;
	struct delayed_work power_hold_chk_work;

	struct wakeup_source *phm_free_ws;
	struct delayed_work phm_free_work;

	struct wakeup_source *retry_phm_free_ws;
	struct delayed_work retry_phm_free_work;

	struct wakeup_source *cs100_ws;
	struct delayed_work cs100_work;

	struct wakeup_source *darkzone_ws;
	struct delayed_work darkzone_work;

	struct wakeup_source *ab_acok_ws;

	struct wakeup_source *check_fw_ws;
	struct delayed_work check_fw_work;

	struct wakeup_source *check_auth_ws;
	struct delayed_work auth_done_work;

	struct wakeup_source *auth_retry_ws;
	struct delayed_work auth_retry_work;

	struct wakeup_source *wpc_set_ws;
	struct delayed_work wpc_set_work;

	struct	notifier_block sb_nb;
	struct	notifier_block wpc_nb;

	struct sec_vote *en_vote;

	struct sec_vote *det_vote;
	struct sec_vote *fcc_vote;

	struct sec_vote *phm_vote;
	struct sec_vote *ldo_vote;

	struct sec_vote *vm_vote;
	struct sec_vote *dc_hdr_vote;
	struct sec_vote *ichg_vote;
	struct sec_vote *dc_high_vout_vote;
	struct sec_vote *low_vout_vote;
	struct sec_vote *bt_vbat_hdr_vote;
	struct sec_vote *bt_hdr_vote;
	struct sec_vote *vout_vote;
	struct sec_vote *fx_hdr_vote;

	struct wakeup_source *align_check_ws;
	struct delayed_work align_check_work;
	int d2d_vout_strength;
	struct timespec d2d_align_check_start;
	bool d2d_checking_align;

	struct sb_wrl_tx now_tx;

	bool wc_w_state;
	bool is_charging;
	bool force_wpc_det_chk;

	u16 addr;
	int size;
	char d_buf[128];

	int temperature;

	/* sec_battery's battery health */
	int battery_health;

	/* sec_battery's battery status */
	int battery_status;

	/* sec_battery's charge mode */
	int battery_chg_mode;

	/* sec_battery's swelling mode */
	int swelling_mode;

	/* sec_battery's store(LDU) mode */
	int store_mode;

	/* check is recharge status */
	int is_recharge;

	int cable_type;

	/* TX id checked twice, if tx id checked normarlly, skip second tx id check */
	unsigned int tx_id_check_cnt;

	/* RX PWR checked */
	unsigned int rx_pwr_check_cnt;

	/* AUTH checked */
	unsigned int auth_check_cnt;

	/* charge mode cc/cv mode */
	int charge_mode;

	/* cs100 flag */
	bool cs100_status;

	int err_status;

	unsigned int phm_chk_cnt;
	bool phm_set_fail;

	/* watchdog test */
	bool watchdog_test;

	/* wireless wa */
	bool ssp_missing;
	unsigned int ssp_missing_count;
	bool darkzone_reset;
	unsigned long darkzone_start_time;
	unsigned long low_current_start_time;
	unsigned int ab_acok_count;
	unsigned long ab_acok_start_time;
};

ssize_t sec_wpc_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t sec_wpc_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SEC_WPC_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sec_wpc_show_attrs,					\
	.store = sec_wpc_store_attrs,					\
}

#endif /* __CPS4019_CHARGER_H */
