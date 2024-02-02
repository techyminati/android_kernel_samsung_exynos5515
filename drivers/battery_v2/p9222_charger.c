/*
 *  p9222_charger.c
 *  Samsung p9222 Charger Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
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

#include <linux/errno.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/firmware.h>
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/alarmtimer.h>
#include <linux/pm_wakeup.h>
#include <linux/types.h>
#include "include/sec_charging_common.h"
#include "include/charger/p9222_charger.h"
#include "include/sec_battery.h"
#include "include/sb_wrl_def.h"
#include "include/sec_vote_event.h"

/* Vout stabilization time is about 1.5sec after Vrect ocured. */
#define VOUT_STABLILZATION_TIME		1500
#define VOUT_MIN_LEVEL				1000

#define PHM_DETACH_DELAY			1700
#define WPC_AUTH_DELAY_TIME			1
#define UNKNOWN_TX_ID				0xFF

#define SEND_RX_ID_CNT				30
#define SEND_REQ_TX_ID_CNT			15

#define ALIGN_WORK_CHK_CNT		8
#define ALIGN_WORK_DELAY		1000
#define ALIGN_CHK_PERIOD		200
#define ALIGN_WORK_CHK_PERIOD	100
#define MISALIGN_TX_OFF_TIME	10

#define PHM_CHK_DELAY	2000
#define PHM_CHK_CNT		5

#define WAKE_LOCK_TIME_OUT	10000

#define ENABLE 1
#define DISABLE 0
#define CMD_CNT 3

#define P9222_I_OUT_OFFSET		12

static enum power_supply_property wpc_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

extern unsigned int lpcharge;

static int p9222_reg_read(struct i2c_client *client, u16 reg, u8 *val)
{
	struct p9222_charger_data *charger = i2c_get_clientdata(client);
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
	msg[1].len = 1;
	msg[1].buf = rbuf;

	mutex_lock(&charger->io_lock);
	ret = i2c_transfer(client->adapter, msg, 2);
	mutex_unlock(&charger->io_lock);
	if (ret < 0)
	{
		dev_err(&client->dev, "%s: read err reg(0x%x) ret(%d)\n",
			__func__, reg, ret);
		return -1;
	}
	*val = rbuf[0];

	pr_debug("%s: reg = 0x%x, data = 0x%x\n", __func__, reg, *val);

	return ret;
}

static int p9222_reg_read_bytes(struct i2c_client *client, u16 reg, u16 *val)
{
	struct p9222_charger_data *charger = i2c_get_clientdata(client);
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

	mutex_lock(&charger->io_lock);
	ret = i2c_transfer(client->adapter, msg, 2);
	mutex_unlock(&charger->io_lock);
	if (ret < 0)
	{
		dev_err(&client->dev, "%s: read err reg(0x%x) ret(%d)\n",
			__func__, reg, ret);
		return -1;
	}
	*val = (rbuf[1] << 8) | rbuf[0];

	pr_debug("%s: reg = 0x%x, data = 0x%x\n", __func__, reg, *val);

	return ret;
}

static int p9222_reg_write(struct i2c_client *client, u16 reg, u8 val)
{
	struct p9222_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };

	mutex_lock(&charger->io_lock);
	ret = i2c_master_send(client, data, 3);
	mutex_unlock(&charger->io_lock);
	if (ret < 3) {
		dev_err(&client->dev, "%s: write err reg(0x%x) ret(%d)\n",
			__func__, reg, ret);
		return ret < 0 ? ret : -EIO;
	}

	pr_debug("%s: reg = 0x%x, data = 0x%x\n", __func__, reg, val);

	return 0;
}

static int p9222_reg_update(struct i2c_client *client, u16 reg, u8 val, u8 mask)
{
	struct p9222_charger_data *charger = i2c_get_clientdata(client);
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };
	u8 data2;
	int ret;

	ret = p9222_reg_read(client, reg, &data2);
	if (ret >= 0) {
		u8 old_val = data2 & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		data[2] = new_val;

		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, 3);
		mutex_unlock(&charger->io_lock);
		if (ret < 3) {
			dev_err(&client->dev, "%s: i2c write error, reg: 0x%x, ret: %d\n",
				__func__, reg, ret);
			return ret < 0 ? ret : -EIO;
		}
	}
	p9222_reg_read(client, reg, &data2);

	pr_debug("%s: reg = 0x%x, data = 0x%x, mask = 0x%x\n", __func__, reg, val, mask);

	return ret;
}

static int p9222_set_cmd_reg(struct p9222_charger_data *charger, u8 val, u8 mask)
{
	u8 temp = 0;
	int ret = 0, i = 0;

	do {
		pr_info("%s: val = 0x%x, mask = 0x%x\n", __func__, val, mask);
		ret = p9222_reg_update(charger->client, P9222_COMMAND_REG, val, mask); // command
		if (ret < 0)
			break;

		msleep(250);

		ret = p9222_reg_read(charger->client, P9222_COMMAND_REG, &temp); // check out set bit exists
		if (ret < 0)
			break;

	} while ((temp != 0) && (i++ <= 3));

	return ret;
}

static int p9222_write_ap_mode(struct p9222_charger_data *charger, u8 ap_mode)
{
	u8 data, i;
	int ret = -1;

	if (charger->need_margin == 1) {
		charger->need_margin = 0;
		msleep(150);
	}

	for (i = 0; i < 3; i++) {
		p9222_reg_write(charger->client, P9222_AP_BATTERY_MODE, ap_mode);
		p9222_reg_read(charger->client, P9222_AP_BATTERY_MODE, &data);

		if (ap_mode == data) {
			ret = 0;
			break;
		}
		msleep(100);
	}

	return ret;
}

static int p9222_set_watchdog_timer(struct p9222_charger_data *charger, bool state)
{
	u8 data = 0;
	int ret = 0;

	data = (state) ? 0 : (0x1 << P9222_WATCHDOG_DIS_SHIFT);
	ret = p9222_reg_update(charger->client,
		P9222_WPC_FLAG_REG, data, P9222_WATCHDOG_DIS_MASK);

	p9222_reg_read(charger->client, P9222_WPC_FLAG_REG, &data);
	pr_info("%s: set watchdog timer to %s, data = 0x%x\n",
		__func__, GET_BOOL_STR(state), data);
	return ret;
}

static int p9222_send_packet(struct p9222_charger_data *charger, u8 header, u8 rx_data_com, u8 data_val)
{
	int ret = 0;

	ret = p9222_reg_write(charger->client, P9222_PACKET_HEADER, header);
	if (ret < 0) {
		pr_err("%s: failed to write header(0x%x), ret(%d)\n", __func__, header, ret);
		return ret;
	}

	ret = p9222_reg_write(charger->client, P9222_RX_DATA_COMMAND, rx_data_com);
	if (ret < 0) {
		pr_err("%s: failed to write rx_data_com(0x%x), ret(%d)\n", __func__, rx_data_com, ret);
		return ret;
	}

	ret = p9222_reg_write(charger->client, P9222_RX_DATA_VALUE0, data_val);
	if (ret < 0) {
		pr_err("%s: failed to write data_val(0x%x), ret(%d)\n", __func__, data_val, ret);
		return ret;
	}

	ret = p9222_reg_update(charger->client, P9222_COMMAND_REG,
		P9222_CMD_SEND_RX_DATA_MASK, P9222_CMD_SEND_RX_DATA_MASK);
	if (ret < 0) {
		pr_err("%s: failed to send rx data, ret(%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int p9222_get_adc(struct p9222_charger_data *charger, int adc_type)
{
	int ret = 0;
	u8 data[2] = {0,};
	u16 val = 0;

	switch (adc_type) {
	case P9222_ADC_VOUT:
		ret = p9222_reg_read_bytes(charger->client, P9222_ADC_VOUT_L_REG, &val);
		ret = (ret >= 0) ? val : 0;
		break;
	case P9222_ADC_VRECT:
		ret = p9222_reg_read_bytes(charger->client, P9222_ADC_VRECT_L_REG, &val);
		ret = (ret >= 0) ? val : 0;
		break;
	case P9222_ADC_TX_ISENSE:
		ret = p9222_reg_read_bytes(charger->client, P9222_ADC_TX_ISENSE_L_REG, &val);
		ret = (ret >= 0) ? (val & 0x0FFF) : 0;
		break;
	case P9222_ADC_RX_IOUT:
		ret = p9222_reg_read_bytes(charger->client, P9222_ADC_RX_IOUT_L_REG, &val);
		ret = (ret >= 0) ? val : 0;
		break;
	case P9222_ADC_DIE_TEMP:
		ret = p9222_reg_read_bytes(charger->client, P9222_ADC_DIE_TEMP_L_REG, &val);
		ret = (ret >= 0) ? val : 0;
		break;
	case P9222_ADC_ALLIGN_X:
		ret = p9222_reg_read(charger->client, P9222_ADC_ALLIGN_X_REG, &data[0]);
		if (ret >= 0)
			ret = data[0]; // need to check
		else
			ret = 0;
		break;
	case P9222_ADC_ALLIGN_Y:
		ret = p9222_reg_read(charger->client, P9222_ADC_ALLIGN_Y_REG, &data[0]);
		if (ret >= 0)
			ret = data[0]; // need to check
		else
			ret = 0;
		break;
	case P9222_ADC_OP_FRQ:
		ret = p9222_reg_read_bytes(charger->client, P9222_OP_FREQ_L_REG, &val);
		ret = (ret >= 0) ? val : 0;
		break;
	case P9222_ADC_VBAT_RAW:
		ret = p9222_reg_read_bytes(charger->client, P9222_VBAT_L_REG, &val);
		ret = (ret >= 0) ? val : 0;
		break;
	case P9222_ADC_VBAT:
		ret = p9222_reg_read_bytes(charger->client, P9222_ADC_VBAT_L_REG, &val);
		ret = (ret >= 0) ? (val & 0x0FFF) : 0;
		break;
	default:
		break;
	}

	return ret;
}

static bool p9222_is_on_pad(struct p9222_charger_data *charger)
{
	return gpio_is_valid(charger->pdata->wpc_det) ?
		!gpio_get_value(charger->pdata->wpc_det) : false;
}

static bool p9222_is_phm(struct p9222_charger_data *charger)
{
	return (charger->power_hold_mode) ||
		(get_sec_vote_result(charger->phm_vote) != WPC_PHM_OFF);
}

static bool p9222_check_chg_in_status(struct p9222_charger_data *charger)
{
	union power_supply_propval value = {0, };

	psy_do_property(charger->pdata->charger_name, get,
		POWER_SUPPLY_PROP_POWER_NOW, value);

	return (value.intval == ACOK_INPUT) ||
		(p9222_get_adc(charger, P9222_ADC_VOUT) > VOUT_MIN_LEVEL);
}

static bool p9222_wait_chg_in_ok(struct p9222_charger_data *charger, int retry)
{
	bool chg_in_ok;

	do {
		if (!p9222_is_on_pad(charger)) {
			pr_info("%s: pad is off!\n", __func__);
			break;
		}

		chg_in_ok = p9222_check_chg_in_status(charger);

		pr_info("%s: chg_in_ok = %d, retry = %d\n", __func__, chg_in_ok, retry);

		if (chg_in_ok)
			return true;
		msleep(300);

		retry--;
	} while (retry > 0);

	return false;
}

static bool p9222_rx_ic_power_on_check(struct p9222_charger_data *charger)
{
	int vrect;

	vrect = p9222_get_adc(charger, P9222_ADC_VRECT);
	pr_info("%s: rx ic off(vrect : %d)\n", __func__, vrect);
	return (vrect > 0);
}

static int p9222_get_ping_duration(struct p9222_charger_data *charger)
{
	/* ping * 2 + margin */
	return (charger->pdata->ping_duration > 0) ?
		((charger->pdata->ping_duration * 3) + (charger->pdata->ping_duration / 2)) :
		500;
}

static void p9222_rx_ic_reset(struct p9222_charger_data *charger, bool hard_reset)
{
	int ping_duration = PHM_DETACH_DELAY;

	if (!gpio_is_valid(charger->pdata->wpc_en)) {
		pr_info("%s: could not set the wpc_en!!!\n", __func__);
		return;
	}

	if (hard_reset) {
		gpio_direction_output(charger->pdata->wpc_en, 1);
		__pm_wakeup_event(charger->phm_free_ws, jiffies_to_msecs(ping_duration + 300));
		queue_delayed_work(charger->wqueue,
			&charger->phm_free_work, msecs_to_jiffies(ping_duration));
	} else {
		ping_duration = p9222_get_ping_duration(charger);

		gpio_direction_output(charger->pdata->wpc_en, 1);
		msleep(ping_duration);
		gpio_direction_output(charger->pdata->wpc_en, 0);
	}

	pr_info("%s: ping_duration = %d, hard_reset = %s\n",
		__func__, ping_duration, GET_BOOL_STR(hard_reset));
}

static struct p9222_charger_type *
p9222_get_charger_type(struct p9222_charger_type *ptype, int size, int tx_id)
{
	int start, end, mid;

	if (tx_id < 0 || tx_id > 0xFF) {
		pr_info("%s: invalid tx_id(%d)\n", __func__, tx_id);
		return NULL;
	} else if ((ptype == NULL) || (size <= 0)) {
		pr_info("%s: charger type is null\n", __func__);
		return NULL;
	}

	start = 0; end = size;
	while (start <= end) {
		mid = (start + end) / 2;

		if (ptype[mid].compatible_tx_id > tx_id)
			end = mid - 1;
		else if (ptype[mid].compatible_tx_id < tx_id)
			start = mid + 1;
		else
			return &ptype[mid];
	}

	pr_info("%s: failed to find pad type(%d, %d, %d, 0x%x)\n",
		__func__, start, end, mid, tx_id);
	return NULL;
}

static int p9222_get_cc_ap_mode(struct p9222_charger_data *charger, int init_value)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, charger->tx_id);

	return (ptype != NULL) ? ptype->cc_ap_mode : init_value;
}

static int p9222_get_cv_ap_mode(struct p9222_charger_data *charger, int init_value)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, charger->tx_id);

	return (ptype != NULL) ? ptype->cv_ap_mode : init_value;
}

static int p9222_get_half_bridge_ap_mode(struct p9222_charger_data *charger, int init_value)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, charger->tx_id);

	return (ptype != NULL) ? ptype->half_bridge_ap_mode : init_value;
}

static int p9222_get_wpc_en_toggle_time(struct p9222_charger_data *charger)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, charger->tx_id);

	return (ptype != NULL) ?
		ptype->phm_free_toggle_time : charger->pdata->ps_wpc_en_toggle_time;
}

static int p9222_get_cc_cv_threshold(struct p9222_charger_data *charger)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, charger->tx_id);

	return (ptype != NULL) ?
		ptype->cc_cv_threshold : charger->pdata->cc_cv_threshold;
}

static int p9222_get_half_bridge_threshold(struct p9222_charger_data *charger)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, charger->tx_id);

	return (ptype != NULL) ?
		ptype->half_bridge_threshold : charger->pdata->half_bridge_threshold;
}

static int p9222_get_pad_type(struct p9222_charger_data *charger, int tx_id)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, tx_id);

	return (ptype != NULL) ? ptype->tx_pad_type : -EINVAL;
}

static bool p9222_get_phm(struct p9222_charger_data *charger, int tx_id)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, tx_id);

	return (ptype != NULL) ?
		ptype->support_power_hold : false;
}

static bool p9222_is_multi_pad(int pad_type)
{
	return (pad_type == P9222_PAD_TYPE_MULTI) ||
		(pad_type == P9222_PAD_TYPE_FAST_MULTI);
}

static bool p9222_is_illegal_pad(int pad_type)
{
	return (pad_type == P9222_PAD_TYPE_ILLEGAL);
}

static bool p9222_compatible_tx_check(struct p9222_charger_data *charger, int tx_id)
{
	struct p9222_charger_type *ptype = p9222_get_charger_type(
		charger->pdata->charger_type, charger->pdata->num_compatible_tx, tx_id);

	return (ptype != NULL);
}

static int p9222_get_target_vout(struct p9222_charger_data *charger)
{
	int target_vout = charger->pdata->low_vout_level;
	int pad_type = P9222_PAD_TYPE_NORMAL;

	pad_type = p9222_get_pad_type(charger, charger->tx_id);
	if (!p9222_is_multi_pad(pad_type) && !p9222_is_illegal_pad(pad_type))
		return target_vout;

	/* Duo pad(TX id 0x70 ~ 0x7F) apply CEP by itself.
	 * Thus output vout is lower than expected set by ap mode.
	 */
	if (charger->pdata->use_ap_mode_table) {
		u8 ap_mode;
		int vbat;
		int i, j;
		int compare_vout = 0;
		int batt_mode_margin;
		int table_vout = 0;

		vbat = p9222_get_adc(charger, P9222_ADC_VBAT_RAW);
		p9222_reg_read(charger->client, P9222_AP_BATTERY_MODE, &ap_mode);

		for (i = 0; i < charger->pdata->num_ap_mode_table; i++) {
			if (ap_mode == charger->pdata->ap_mode_table_info[i].ap_mode) {
				batt_mode_margin = charger->pdata->ap_mode_table_info[i].batt_mode_margin;
				table_vout = charger->pdata->ap_mode_table_info[i].vout;

				if (batt_mode_margin) {
					compare_vout = table_vout - batt_mode_margin;
					if (vbat <= compare_vout)
						target_vout = table_vout - 150;
					else
						target_vout = (vbat + batt_mode_margin) - 150;
				} else {
					if ((table_vout - 160) < charger->pdata->low_vout_level)
						target_vout = table_vout - 160;
					else {
						target_vout = charger->pdata->low_vout_level;
						for (j = 0; j < charger->pdata->num_ex_low_vout; j++) {
							if (charger->pdata->ex_low_vout[j].tx_id == charger->tx_id)
								target_vout = charger->pdata->ex_low_vout[j].low_vout;
						}
					}
				}
				break;
			}
		}

		pr_debug("%s: ap_mode(%x) table_vout(%d) target_vout(%d) compare_vout(%d) vbat(%d)\n",
			__func__, ap_mode, table_vout, target_vout, compare_vout, vbat);
	}

	return target_vout;
}

static int p9222_unsafe_vout_check(struct p9222_charger_data *charger)
{
	int vout;
	int target_vout;

	vout = p9222_get_adc(charger, P9222_ADC_VOUT);
	target_vout = p9222_get_target_vout(charger);

	pr_info("%s: vout(%d) target_vout(%d)\n", __func__, vout, target_vout);
	if (vout < target_vout)
		return 1;
	return 0;
}

static void p9222_fod_set(struct p9222_charger_data *charger)
{
	int i = 0;

	if (!charger->pdata->fod_data_check)
		return;

	pr_info("%s:\n", __func__);
	for (i = 0; i < P9222_NUM_FOD_REG; i++)
		p9222_reg_write(charger->client, P9222_WPC_FOD_0A_REG + i,
			charger->pdata->fod_data[i]);
}

static bool p9222_send_cs100(struct p9222_charger_data *charger)
{
	int i = 0;

	for (i = 0; i < 6; i++) {
		if (p9222_reg_write(charger->client,
				P9222_CHG_STATUS_REG, 100) < 0)
			return false;

		if (p9222_set_cmd_reg(charger,
				P9222_CMD_SEND_CHG_STS_MASK, P9222_CMD_SEND_CHG_STS_MASK) < 0)
			return false;

		msleep(250);
	}

	return true;
}

static bool p9222_wpc_send_tx_uno_mode_disable(struct p9222_charger_data *charger, int cnt)
{
	bool ic_off_state = false;
	int i;

	for (i = 0; i < cnt; i++) {
		pr_info("%s: (cnt = %d, ic_off_state = %d)\n", __func__, i, ic_off_state);
		if (!p9222_rx_ic_power_on_check(charger)) {
			if (ic_off_state)
				break;

			ic_off_state = true;
		}
		p9222_send_packet(charger, P9222_HEADER_AFC_CONF, 0x19, 0xFF);
		msleep(100);
	}

	return ic_off_state;
}

static bool p9222_send_eop(struct p9222_charger_data *charger, int healt_mode)
{
	int i = 0;
	int ret = 0;

	switch (healt_mode) {
	case POWER_SUPPLY_HEALTH_OVERHEAT:
	case POWER_SUPPLY_HEALTH_OVERHEATLIMIT:
	case POWER_SUPPLY_HEALTH_COLD:
		pr_info("%s: ept-ot\n", __func__);
		for (i = 0; i < CMD_CNT; i++) {
			ret = p9222_reg_write(charger->client,
				P9222_END_POWER_TRANSFER_REG, P9222_EPT_OVER_TEMP);
			if (ret >= 0) {
				p9222_set_cmd_reg(charger,
					P9222_CMD_SEND_EOP_MASK, P9222_CMD_SEND_EOP_MASK);
				msleep(250);
			} else
				break;
		}
		break;
	case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
		pr_info("%s: ept-undervoltage\n", __func__);
		if (p9222_get_pad_type(charger, charger->tx_id) == P9222_PAD_TYPE_D2D) {
			return p9222_wpc_send_tx_uno_mode_disable(charger, 30);
		} else {
			ret = p9222_reg_write(charger->client,
				P9222_END_POWER_TRANSFER_REG, P9222_EPT_OVER_TEMP);
			if (ret >= 0) {
				p9222_set_cmd_reg(charger,
					P9222_CMD_SEND_EOP_MASK, P9222_CMD_SEND_EOP_MASK);
				msleep(250);
			}
		}
		break;
	default:
		break;
	}

	return !p9222_rx_ic_power_on_check(charger);
}

static int p9222_power_hold_mode_set(struct p9222_charger_data *charger, int set)
{
	int i = 0, ret = -1;
	bool acok;

	acok = p9222_check_chg_in_status(charger);
	pr_info("%s: set = %d power_hold_mode = %d tx_id = %x acok = %d\n", __func__,
		set, charger->power_hold_mode, charger->tx_id, acok);

	if (set) {
		bool ic_power_on = p9222_rx_ic_power_on_check(charger);

		if (!ic_power_on) {
			pr_info("%s: no need to set phm\n", __func__);
			ret = 0;
			goto end_phm_set;
		}

		for (i = 0; i <= 3; i++) {
			p9222_send_packet(charger, 0x28, 0x18, 0x01);
			msleep(200);

			ic_power_on = p9222_rx_ic_power_on_check(charger);
			acok = p9222_check_chg_in_status(charger);
			pr_info("%s: check ic_power_on = %s, acok = %s\n",
				__func__, GET_BOOL_STR(ic_power_on), GET_BOOL_STR(acok));
			if (!ic_power_on) {
				ret = 0;
				goto end_phm_set;
			}
		}
	} else {
		int ping_duration = p9222_get_wpc_en_toggle_time(charger);

		if (acok) {
			pr_info("%s: no need to clear phm\n", __func__);
			ret = 0;
			goto end_phm_set;
		} else if (!gpio_is_valid(charger->pdata->wpc_en)) {
			pr_err("%s: invalid wpc_en gpio\n", __func__);
			goto end_phm_set;
		}

		for (i = 0; i < 2; i++) {
			/* power hold mode exit need 2 pings when wpc_en high status */
			gpio_direction_output(charger->pdata->wpc_en, 1);
			msleep(ping_duration);
			gpio_direction_output(charger->pdata->wpc_en, 0);

			if (!p9222_is_on_pad(charger)) {
				pr_err("%s: wpc detached!!\n", __func__);
				ret = 0;
				goto end_phm_set;
			}

			/* Power transfer phase have to keep  over 5sec to Vout LDO turn on
			 * if Vrect is lower than target Vrect.
			 * Thus wait chg_in_ok over 5sec.
			 */
			acok = p9222_wait_chg_in_ok(charger, 20);
			if (acok) {
				msleep(300);
				acok = p9222_check_chg_in_status(charger);
				if (!acok) {
					p9222_rx_ic_reset(charger, false);
					p9222_wait_chg_in_ok(charger, 8);
					pr_err("%s: chg_in err!\n", __func__);
				} else {
					ret = 0;
					goto end_phm_set;
				}
			}
		}
	}

end_phm_set:
	pr_info("%s: end(ret = %d, acok = %d)\n", __func__, ret, acok);

	return ret;
}

static int p9222_normal_full_charge(struct p9222_charger_data *charger)
{
	int i, ret = -1;

	pr_info("%s\n", __func__);

	sec_vote(charger->apm_vote, VOTER_WPC_CONTROL, true, WPC_APM_VOUT_OFF);
	msleep(200);
	sec_vote(charger->ldo_vote, VOTER_WPC_CONTROL, true, WPC_LDO_OFF);

	for (i = 0; i < 3; i++) {
		if (p9222_check_chg_in_status(charger)) {
			msleep(200);
		} else {
			ret = 0;
			break;
		}
	}

	return ret;
}

static int p9222_normal_re_charge(struct p9222_charger_data *charger)
{
	int i, vrect;
	int ret = -1;

	pr_info("%s\n", __func__);

	sec_vote(charger->ldo_vote, VOTER_WPC_CONTROL, false, WPC_LDO_ON);
	sec_vote(charger->apm_vote, VOTER_WPC_CONTROL, true, WPC_APM_VOUT_ON);

	if (p9222_wait_chg_in_ok(charger, 10) == true) {
		for (i = 0; i < 15; i++) {
			vrect = p9222_get_adc(charger, P9222_ADC_VRECT);
			pr_info("%s: %d vrect = %d\n", __func__, i, vrect);

			/* IDT mode raise Vrect to 5.5V
			 * If CEP 10 is adjusted, IDT mode raise Vrect to 4.95V
			 * therefore, considering set marging wait Vrect until over 4.9V
			 */
			if (vrect > 4900) {
				ret = 0;
				break;
			}

			msleep(100);
		}
	}
	sec_vote(charger->apm_vote, VOTER_WPC_CONTROL, false, WPC_APM_NONE);

	pr_info("%s: end(ret = %d)\n", __func__, ret);
	return ret;
}

static int p9222_power_hold_full_charge(struct p9222_charger_data *charger)
{
	int ret = -1;

	pr_info("%s\n", __func__);

	if (charger->support_power_hold)
		ret = p9222_power_hold_mode_set(charger, 1);

	if (ret < 0) {
		charger->phm_set_fail = true;
		ret = p9222_normal_full_charge(charger);
	}

	return ret;
}

static int p9222_power_hold_re_charge(struct p9222_charger_data *charger)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	if (charger->phm_set_fail) {
		charger->phm_set_fail = false;
		ret = p9222_normal_re_charge(charger);
	} else {
		ret = p9222_power_hold_mode_set(charger, 0);
	}

	return ret;
}

#define MAX_SSP_MISSING_CNT	5
static void p9222_check_ssp_missing(struct p9222_charger_data *charger)
{
	if (!charger->ssp_missing) {
		charger->ssp_missing_count = 0;
		return;
	}

	if (++charger->ssp_missing_count >= MAX_SSP_MISSING_CNT) {
		charger->support_power_hold = 0;
		pr_err("%s: fixed support phm flag to 0 because of ssp missing\n", __func__);
	}

	charger->ssp_missing = false;
}

static int p9222_set_phm_vote(void *data, int v)
{
	struct p9222_charger_data *charger = data;
	int ret = 0;

	pr_info("%s: v = %d, charger->charge_mode = %d\n",
		__func__, v, charger->charge_mode);

	switch (v) {
	case WPC_PHM_EPT_ON:
		if (!charger->power_hold_mode) {
			p9222_set_watchdog_timer(charger, false);
			ret = p9222_power_hold_full_charge(charger);
			charger->charge_mode = P9222_CHARGE_MODE_NONE;
			charger->power_hold_mode = true;
		}

		if (p9222_rx_ic_power_on_check(charger)) {
			/* send ept-ot if phm was not set or was pad did not support. */
			pr_info("%s: send ept-ot if PHM was not set or PAD did not support PHM.\n",
				__func__);
			p9222_send_eop(charger, POWER_SUPPLY_HEALTH_OVERHEAT);
		}
		break;
	case WPC_PHM_ON:
		p9222_set_watchdog_timer(charger, false);
		ret = p9222_power_hold_full_charge(charger);
		charger->charge_mode = P9222_CHARGE_MODE_NONE;
		charger->power_hold_mode = true;

		if (charger->support_power_hold == 1) {
			__pm_wakeup_event(charger->power_hold_chk_ws,
				jiffies_to_msecs(WAKE_LOCK_TIME_OUT));
			queue_delayed_work(charger->wqueue,
				&charger->power_hold_chk_work, msecs_to_jiffies(PHM_CHK_DELAY));
		}
		break;
	case WPC_PHM_OFF:
	default:
		if (!charger->wc_w_state) {
			if (charger->support_power_hold == 1)
				p9222_rx_ic_reset(charger, true);
		} else {
			bool run_phm_free = !charger->phm_set_fail;

			p9222_check_ssp_missing(charger);
			charger->charge_mode = P9222_CHARGE_MODE_NONE;
			ret = p9222_power_hold_re_charge(charger);
			charger->need_margin = 1;
			p9222_set_watchdog_timer(charger, true);

			if (run_phm_free) {
				__pm_wakeup_event(charger->phm_free_ws, jiffies_to_msecs(1000));
				queue_delayed_work(charger->wqueue, &charger->phm_free_work, 0);
			}
		}
		charger->power_hold_mode = false;
		break;
	}

	return ret;
}

static bool p9222_check_half_bridge_state(struct p9222_charger_data *charger, int capacity)
{
	int vout = 0;

	vout = p9222_get_adc(charger, P9222_ADC_VOUT);
	pr_info("%s: capacity = %d, vout = %d, chg_mode = %d\n",
		__func__, capacity, vout, charger->charge_mode);

	if (capacity < 0 || vout <= 0)
		return false;

	if (capacity >= p9222_get_half_bridge_threshold(charger))
		return ((vout <= charger->pdata->half_bridge_vout) ||
			(charger->charge_mode == P9222_CHARGE_MODE_HALF_BRIDGE));

	return false;
}

static int p9222_set_apm_vote(void *data, int v)
{
	struct p9222_charger_data *charger = data;
	u8 ap_mode = 0, old_mode = 0;
	int ret = 0, charge_mode = P9222_CHARGE_MODE_NONE;

	ret = p9222_reg_read(charger->client, P9222_AP_BATTERY_MODE, &old_mode);
	if (ret < 0) {
		pr_info("%s: failed to read ap battery mode (%d)\n", __func__, ret);
		charger->charge_mode = P9222_CHARGE_MODE_NONE;
		return ret;
	}

	switch (v) {
	case WPC_APM_TEST:
		ap_mode = P9222_AP_BATTERY_CURR_MEASURE_MODE;
		break;
	case WPC_APM_VOUT_OFF:
		ap_mode = P9222_AP_BATTERY_INCOMPATIBLE_PHP_MODE;
		break;
	case WPC_APM_VOUT_ON:
		ap_mode = P9222_AP_BATTERY_IDT_MODE;
		break;
	case WPC_APM_WAKE_UP:
		ap_mode = P9222_AP_BATTERY_WAKEUP_MODE;
		break;
	case WPC_APM_BOOT:
		ap_mode = charger->pdata->boot_ap_mode;
		charge_mode = charger->charge_mode;
		break;
	case WPC_APM_STORE:
		ap_mode = P9222_AP_BATTERY_BATT_MODE;
		break;
	case WPC_APM_NORMAL:
	default:
	{
		union power_supply_propval value = {0, };
		int capacity = 0;

		value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
		psy_do_property(charger->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
		capacity = value.intval;

		if (p9222_check_half_bridge_state(charger, capacity)) {
			charge_mode = P9222_CHARGE_MODE_HALF_BRIDGE;
			ap_mode = p9222_get_half_bridge_ap_mode(charger, P9222_AP_BATTERY_IDT_MODE);
		} else if (capacity >= p9222_get_cc_cv_threshold(charger)) {
			charge_mode = P9222_CHARGE_MODE_CV;
			ap_mode = p9222_get_cv_ap_mode(charger, P9222_AP_BATTERY_CC_CV_MODE);
		} else {
			charge_mode = P9222_CHARGE_MODE_CC;
			ap_mode = p9222_get_cc_ap_mode(charger, P9222_AP_BATTERY_CC_MODE);
		}
	}
		break;
	}

	if (old_mode != ap_mode)
		ret = p9222_write_ap_mode(charger, ap_mode);

	charger->charge_mode = charge_mode;
	pr_info("%s: v = %d, old_mode = 0x%x, new_mode = 0x%x, ret = %d, charge_mode = %d\n",
		__func__, v, old_mode, ap_mode, ret, charge_mode);
	return ret;
}

static int p9222_set_ldo_vote(void *data, int v)
{
	struct p9222_charger_data *charger = data;
	bool acok = false;
	int ret = 0;

	if (!charger->wc_w_state)
		return 0;

	acok = p9222_check_chg_in_status(charger);
	if (((v == WPC_LDO_ON) && acok) ||
		((v == WPC_LDO_OFF) && !acok)) {
		pr_info("%s: skip (v = %d, acok = %d)\n", __func__, v, acok);
		return 0;
	}

	ret = p9222_reg_update(charger->client, P9222_COMMAND_REG,
		P9222_CMD_TOGGLE_LDO_MASK, P9222_CMD_TOGGLE_LDO_MASK);

	pr_info("%s: v = %d, ret = %d\n", __func__, v, ret);
	return ret;
}


void p9222_send_multi_packet(struct p9222_charger_data *charger, u8 header, u8 rx_data_com, u8 *data_val, int data_size)
{
	int ret;
	int i;

	ret = p9222_reg_write(charger->client, P9222_PACKET_HEADER, header);
	ret = p9222_reg_write(charger->client, P9222_RX_DATA_COMMAND, rx_data_com);

	for (i = 0; i < data_size; i++)
		ret = p9222_reg_write(charger->client, P9222_RX_DATA_VALUE0 + i, data_val[i]);

	p9222_set_cmd_reg(charger, P9222_CMD_SEND_RX_DATA_MASK, P9222_CMD_SEND_RX_DATA_MASK);
}

static bool p9222_send_watchdog_err_packet(struct p9222_charger_data *charger)
{
	bool ic_off_state = false;
	int i = 0;

	pr_info("%s\n", __func__);
	for (i = 0; i <= 3; i++) {
		if (!p9222_rx_ic_power_on_check(charger)) {
			if (ic_off_state)
				break;

			ic_off_state = true;
		}

		p9222_send_multi_packet(charger, 0x18, 0xE7, NULL, 0);
		msleep(200);
	}

	return ic_off_state;
}

static bool p9222_wpc_det_irq_enable(struct p9222_charger_data *charger, bool en_irq)
{
	int ret = 0;

	if (en_irq == charger->irq_wpc_det_enabled)
		return en_irq;

	if (en_irq) {
		ret = enable_irq_wake(charger->pdata->irq_wpc_det);
		if (ret < 0)
			pr_err("%s: failed to enable irq_wake(%d)\n", __func__, ret);
		enable_irq(charger->pdata->irq_wpc_det);
	} else {
		ret = disable_irq_wake(charger->pdata->irq_wpc_det);
		if (ret < 0)
			pr_err("%s: failed to disable irq_wake(%d)\n", __func__, ret);
		disable_irq(charger->pdata->irq_wpc_det);
	}

	charger->irq_wpc_det_enabled = en_irq;
	return en_irq;
}

static int p9222_get_firmware_version(struct p9222_charger_data *charger, int firm_mode)
{
	int version = -1;
	int ret;
	u8 otp_fw_major[2] = {0,};
	u8 otp_fw_minor[2] = {0,};
	u8 tx_fw_major[2] = {0,};
	u8 tx_fw_minor[2] = {0,};

	if (!p9222_is_on_pad(charger))
		return version;

	switch (firm_mode) {
	case P9222_RX_FIRMWARE:
		ret = p9222_reg_read(charger->client, P9222_OTP_FW_MAJOR_REV_L_REG, &otp_fw_major[0]);
		ret = p9222_reg_read(charger->client, P9222_OTP_FW_MAJOR_REV_H_REG, &otp_fw_major[1]);
		if (ret >= 0)
			version =  otp_fw_major[0] | (otp_fw_major[1] << 8);
		pr_debug("%s: rx major firmware version 0x%x\n", __func__, version);

		ret = p9222_reg_read(charger->client, P9222_OTP_FW_MINOR_REV_L_REG, &otp_fw_minor[0]);
		ret = p9222_reg_read(charger->client, P9222_OTP_FW_MINOR_REV_H_REG, &otp_fw_minor[1]);
		if (ret >= 0)
			version =  otp_fw_minor[0] | (otp_fw_minor[1] << 8);
		pr_debug("%s: rx minor firmware version 0x%x\n", __func__, version);
		break;
	case P9222_TX_FIRMWARE:
		ret = p9222_reg_read(charger->client, P9222_SRAM_FW_MAJOR_REV_L_REG, &tx_fw_major[0]);
		ret = p9222_reg_read(charger->client, P9222_SRAM_FW_MAJOR_REV_H_REG, &tx_fw_major[1]);
		if (ret >= 0)
			version =  tx_fw_major[0] | (tx_fw_major[1] << 8);
		pr_debug("%s: tx major firmware version 0x%x\n", __func__, version);

		ret = p9222_reg_read(charger->client, P9222_SRAM_FW_MINOR_REV_L_REG, &tx_fw_minor[0]);
		ret = p9222_reg_read(charger->client, P9222_SRAM_FW_MINOR_REV_H_REG, &tx_fw_minor[1]);
		if (ret >= 0)
			version =  tx_fw_minor[0] | (tx_fw_minor[1] << 8);
		pr_debug("%s: tx minor firmware version 0x%x\n", __func__, version);
		break;
	default:
		pr_err("%s: Wrong firmware mode\n", __func__);
		version = -1;
		break;
	}

	return version;
}

static bool p9222_check_illegal_pad(struct p9222_charger_data *charger, int tx_id)
{
	if (!p9222_is_illegal_pad(p9222_get_pad_type(charger, tx_id)))
		return false;

	/* set values */
	charger->tx_id = tx_id;
	charger->tx_id_checked = true;
	charger->tx_id_check_cnt = 1;
	charger->cable_type = SB_CBL_WRL;

	/* enable wpc det irq */
	p9222_wpc_det_irq_enable(charger, true);

	pr_info("%s: set illegal pad, tx_id = 0x%x\n", __func__, tx_id);
	return true;
}

static void p9222_phm_check_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, power_hold_chk_work.work);

	if (p9222_is_on_pad(charger) && p9222_is_phm(charger)) {
		if (!p9222_rx_ic_power_on_check(charger))
			goto end_phm_chk;

		if (charger->phm_chk_cnt < PHM_CHK_CNT) {
			pr_err("%s: retry to set phm, count = %d\n",
				__func__, charger->phm_chk_cnt);
			if (charger->phm_set_fail)
				p9222_normal_re_charge(charger);

			charger->phm_chk_cnt++;
			charger->phm_set_fail = false;
			sec_vote_refresh(charger->phm_vote);
			return;
		} else {
			charger->support_power_hold = 0;
			pr_err("%s: fixed support phm flag to 0\n", __func__);
		}
	}

end_phm_chk:
	charger->phm_chk_cnt = 0;
	__pm_relax(charger->power_hold_chk_ws);
}

static void p9222_phm_free_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, phm_free_work.work);

	pr_info("%s: wc_w_state = %d\n", __func__, charger->wc_w_state);

	/* do not use the p9222_is_on_pad() */
	if (charger->wc_w_state) {
		if (!p9222_rx_ic_power_on_check(charger) &&
			(charger->phm_chk_cnt < PHM_CHK_CNT)) {
			/* retry phm free */
			charger->phm_chk_cnt++;
			__pm_stay_awake(charger->retry_phm_free_ws);
			queue_delayed_work(charger->wqueue,
				&charger->retry_phm_free_work, msecs_to_jiffies(PHM_CHK_DELAY));
		} else {
			/* restart wpc auth */
			charger->phm_chk_cnt = 0;
			charger->force_wpc_det_chk |= !p9222_check_chg_in_status(charger);
			__pm_stay_awake(charger->wpc_ws);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_det_work, msecs_to_jiffies(500));
		}
	}

	/* recover wpc_en to low */
	gpio_direction_output(charger->pdata->wpc_en, 0);
	__pm_relax(charger->phm_free_ws);
}

static void p9222_retry_phm_free_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, retry_phm_free_work.work);

	pr_info("%s: phm_chk_cnt = %d, wc_w_state = %d\n",
		__func__, charger->phm_chk_cnt, charger->wc_w_state);

	if (p9222_is_on_pad(charger) &&
		!p9222_rx_ic_power_on_check(charger))
		sec_vote_refresh(charger->phm_vote);

	__pm_relax(charger->retry_phm_free_ws);
}

static void p9222_cs100_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, cs100_work.work);

	if (!p9222_is_on_pad(charger)) {
		sec_vote(charger->phm_vote, VOTER_WPC_CS100, false, WPC_PHM_OFF);
		__pm_relax(charger->cs100_ws);
		return;
	}

	charger->cs100_status = p9222_send_cs100(charger);
	sec_vote(charger->phm_vote, VOTER_WPC_CS100, false, WPC_PHM_OFF);

	pr_info("%s: cs100 status = %s\n", __func__, GET_BOOL_STR(charger->cs100_status));
	__pm_relax(charger->cs100_ws);
}

static int p9222_temperature_check(struct p9222_charger_data *charger)
{
	if (charger->temperature >= charger->pdata->tx_off_high_temp) {
		/* send TX watchdog command in high temperature */
		pr_err("%s:HIGH TX OFF TEMP:%d\n", __func__, charger->temperature);
		p9222_send_watchdog_err_packet(charger);
	}

	return 0;
}

static bool p9222_check_low_current(struct p9222_charger_data *charger, int vout, int curr)
{
	if ((vout < VOUT_MIN_LEVEL) ||
		((charger->battery_status != POWER_SUPPLY_STATUS_CHARGING) &&
		(charger->battery_status != POWER_SUPPLY_STATUS_FULL))) {
		charger->low_current_start_time = 0;
		return false;
	}

	if (curr < 10) {
		struct timespec ts = ktime_to_timespec(ktime_get_boottime());

		if (charger->low_current_start_time <= 0) {
			charger->low_current_start_time = ts.tv_sec;
		} else {
			unsigned long low_current_time;

			if (charger->low_current_start_time > ts.tv_sec)
				low_current_time = 0xFFFFFFFF - charger->low_current_start_time + ts.tv_sec;
			else
				low_current_time = ts.tv_sec - charger->low_current_start_time;

			pr_info("%s: now sec(%ld), state(%ld)\n", __func__,
				ts.tv_sec, low_current_time);
			return (low_current_time >= charger->pdata->low_current_expired_time);
		}
	} else
		charger->low_current_start_time = 0;

	return false;
}

static bool p9222_check_abnormal_acok(struct p9222_charger_data *charger)
{
	if ((charger->battery_status != POWER_SUPPLY_STATUS_CHARGING) &&
		(charger->battery_status != POWER_SUPPLY_STATUS_FULL)) {
		charger->ab_acok_count = 0;
		charger->ab_acok_start_time = 0;
		return false;
	}

	if (charger->ab_acok_count > 0) {
		struct timespec ts = ktime_to_timespec(ktime_get_boottime());

		if (charger->ab_acok_start_time <= 0) {
			/* re-set ab_acok_count for re-counting abnormal acok. */
			charger->ab_acok_count = 1;
			pr_info("%s: now sec(%ld), state(%d)\n", __func__, ts.tv_sec, charger->ab_acok_count);
			charger->ab_acok_start_time = ts.tv_sec;
			/* Prevent sleep state for checking abnormal acok count. */
			__pm_wakeup_event(charger->ab_acok_ws,
				jiffies_to_msecs(HZ * (charger->pdata->ab_acok_time + 1)));
		} else {
			unsigned long ab_acok_time;

			if (charger->ab_acok_start_time > ts.tv_sec)
				ab_acok_time = 0xFFFFFFFF - charger->ab_acok_start_time + ts.tv_sec;
			else
				ab_acok_time = ts.tv_sec - charger->ab_acok_start_time;

			pr_info("%s: now sec(%ld), state(%ld, %d)\n", __func__,
				ts.tv_sec, ab_acok_time, charger->ab_acok_count);
			if (charger->ab_acok_count >= charger->pdata->ab_acok_count) {
				if (ab_acok_time <= charger->pdata->ab_acok_time)
					return true;
			} else {
				if (ab_acok_time <= charger->pdata->ab_acok_time)
					return false;
			}
			charger->ab_acok_count = 0;
			charger->ab_acok_start_time = 0;
		}
	} else
		charger->ab_acok_start_time = 0;

	return false;
}

static bool p9222_check_darkzone_state(struct p9222_charger_data *charger, int vrect, int vout)
{
	int target_vout = 0;

	if ((charger->store_mode) ||
		(p9222_get_pad_type(charger, charger->tx_id) == P9222_PAD_TYPE_NORMAL)) {
		charger->darkzone_reset = false;
		goto init_darkzone_flag;
	}

	target_vout = p9222_get_target_vout(charger);
	pr_info("%s: %d <-> %d, %d\n", __func__, target_vout, vrect, vout);

	if ((vrect < target_vout) && (vout < target_vout) &&
		(charger->battery_health == POWER_SUPPLY_HEALTH_GOOD) &&
		(!charger->swelling_mode) &&
		(charger->battery_chg_mode != SEC_BATTERY_CHARGING_NONE)) {
		struct timespec c_ts = ktime_to_timespec(ktime_get_boottime());

		if (charger->darkzone_start_time == 0) {
			charger->darkzone_start_time = c_ts.tv_sec;
		} else {
			unsigned long darkzone_time;

			if (charger->darkzone_start_time > c_ts.tv_sec)
				darkzone_time = 0xFFFFFFFF - charger->darkzone_start_time + c_ts.tv_sec;
			else
				darkzone_time = c_ts.tv_sec - charger->darkzone_start_time;

			pr_info("%s: darkzone(%ld)\n", __func__, darkzone_time);
			return (darkzone_time >= charger->pdata->darkzone_expired_time);
		}

		return false;
	} else if (vout >= target_vout) {
		charger->darkzone_reset = false;
	}

init_darkzone_flag:
	charger->darkzone_start_time = 0;
	return false;
}

static void p9222_darkzone_work(struct work_struct *work)
{
	struct p9222_charger_data *charger = container_of(work,
		struct p9222_charger_data, darkzone_work.work);
	bool wc_w_state = p9222_is_on_pad(charger);

	pr_info("%s: wc_w_state(%s <--> %s)\n", __func__,
		GET_BOOL_STR(charger->wc_w_state), GET_BOOL_STR(wc_w_state));
	if (!wc_w_state || !gpio_is_valid(charger->pdata->wpc_en)) {
		__pm_relax(charger->darkzone_ws);
		return;
	}

	if (charger->wc_w_state) {
		union power_supply_propval value = {0, };

		charger->darkzone_reset = true;
		value.intval = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		psy_do_property(charger->pdata->battery_name, set,
			POWER_SUPPLY_PROP_HEALTH, value);
	}

	/* hard reset IC */
	p9222_rx_ic_reset(charger, true);
	__pm_relax(charger->darkzone_ws);
}

static void p9222_init_wa_flag(struct p9222_charger_data *charger)
{
	charger->darkzone_start_time = 0;
	charger->low_current_start_time = 0;
	charger->ab_acok_count = 0;
	charger->ab_acok_start_time = 0;
}

static void p9222_wpc_align_check_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, align_check_work.work);

	struct timespec current_ts = {0, };
	union power_supply_propval value = {0, };
	long checking_time = 0;
	static int d2d_vout_sum, d2d_align_work_cnt;
	int vout = 0, vout_avr = 0;

	if (!p9222_is_on_pad(charger))
		goto end_align_work;

	vout = p9222_get_adc(charger, P9222_ADC_VOUT);
	if (charger->d2d_align_check_start.tv_sec == 0) {
		get_monotonic_boottime(&charger->d2d_align_check_start);
		d2d_align_work_cnt = 0;
		d2d_vout_sum = 0;
	}
	get_monotonic_boottime(&current_ts);
	checking_time = current_ts.tv_sec - charger->d2d_align_check_start.tv_sec;

	d2d_vout_sum += vout;
	d2d_align_work_cnt++;
	vout_avr = d2d_vout_sum / d2d_align_work_cnt;

	pr_info("%s: vout(%d), vout_avr(%d), work_cnt(%d), checking_time(%ld)\n",
		__func__, vout, vout_avr, d2d_align_work_cnt, checking_time);

	if (d2d_align_work_cnt >= ALIGN_WORK_CHK_CNT) {
		if (vout_avr >= p9222_get_target_vout(charger)) {
			pr_info("%s: Finish to check (Align)\n", __func__);

			value.intval = charger->d2d_vout_strength = 100;
			psy_do_property(charger->pdata->battery_name,
				set, POWER_SUPPLY_PROP_WPC_FREQ_STRENGTH, value);

			goto end_align_work;
		} else if (checking_time >= MISALIGN_TX_OFF_TIME) {
			pr_info("%s: Keep Misalign during %d secs (Tx off)\n",
				__func__, MISALIGN_TX_OFF_TIME);

			if (p9222_rx_ic_power_on_check(charger)) {
				p9222_wpc_send_tx_uno_mode_disable(charger, 30);
				p9222_rx_ic_reset(charger, true);

				goto end_align_work;
			} else {
				queue_delayed_work(charger->wqueue,
					&charger->align_check_work, msecs_to_jiffies(ALIGN_CHK_PERIOD));
			}
		} else {
			pr_info("%s: Continue to check until %d secs (Misalign)\n",
				__func__, MISALIGN_TX_OFF_TIME);
			value.intval = charger->d2d_vout_strength = 0;
			psy_do_property(charger->pdata->battery_name,
				set, POWER_SUPPLY_PROP_WPC_FREQ_STRENGTH, value);

			d2d_align_work_cnt = 0;
			d2d_vout_sum = 0;
			queue_delayed_work(charger->wqueue,
				&charger->align_check_work, msecs_to_jiffies(ALIGN_CHK_PERIOD));
		}
	} else {
		queue_delayed_work(charger->wqueue,
			&charger->align_check_work, msecs_to_jiffies(ALIGN_WORK_CHK_PERIOD));
	}

	return;

end_align_work:
	sec_vote(charger->phm_vote, VOTER_WPC_ALIGN_CHK, false, WPC_PHM_OFF);
	charger->d2d_checking_align = false;
	charger->d2d_align_check_start.tv_sec = 0;
	__pm_relax(charger->align_check_ws);
}

static void p9222_wpc_align_check(struct p9222_charger_data *charger, unsigned int work_delay)
{
	bool is_power_hold_mode = false;

	if (!is_d2d_wrl_type(charger->cable_type)) {
		pr_info("%s: return, cable_type(%d)\n", __func__, charger->cable_type);
		return;
	}

	if (charger->d2d_checking_align) {
		pr_info("%s: return, d2d_checking_align(%d)\n", __func__, charger->d2d_checking_align);
		return;
	}

	is_power_hold_mode = p9222_is_phm(charger);
	if (!charger->is_charging || is_power_hold_mode) {
		pr_info("%s: return, is_charging(%d), is_power_hold_mode(%d)\n",
			__func__, charger->is_charging, is_power_hold_mode);
		return;
	}

	if (charger->d2d_vout_strength >= 100) {
		if (!p9222_rx_ic_power_on_check(charger)) {
			sec_vote(charger->phm_vote, VOTER_WPC_TX_PHM, true, WPC_PHM_ON);
			pr_info("%s: return, set phm state to tx_phm\n", __func__);
			return;
		}

		if (!p9222_unsafe_vout_check(charger)) {
			pr_info("%s: return, safe vout\n", __func__);
			return;
		}
	}

	pr_info("%s: start\n", __func__);
	sec_vote(charger->phm_vote, VOTER_WPC_ALIGN_CHK, true, WPC_PHM_OFF);

	charger->d2d_checking_align = true;	
	__pm_wakeup_event(charger->align_check_ws, jiffies_to_msecs(WAKE_LOCK_TIME_OUT));
	queue_delayed_work(charger->wqueue, &charger->align_check_work, msecs_to_jiffies(work_delay));
}

static bool p9222_check_cable_type(int chg_cable_type, int batt_cable_type)
{
	if (chg_cable_type == batt_cable_type)
		return false;

	return ((is_wireless_type(chg_cable_type) && is_nocharge_type(batt_cable_type)) ||
		(is_nocharge_type(chg_cable_type) && is_wireless_type(batt_cable_type)));
}

static int p9222_monitor_work(struct p9222_charger_data *charger)
{
	int vrect;
	int vout = 0;
	int freq = 0;
	int curr = 0, vol = 0;
	u8 ap_mode = 0, vbat_monitor = 0;
	u8 full_bridge = 0;
	u8 m_depth = 0;
	int capacity;
	union power_supply_propval value = {0, };

	value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
	psy_do_property(charger->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CAPACITY, value);
	capacity = value.intval;

	psy_do_property(charger->pdata->battery_name, get,
		POWER_SUPPLY_PROP_HEALTH, value);
	charger->battery_health = value.intval;

	psy_do_property(charger->pdata->battery_name, get,
		POWER_SUPPLY_PROP_STATUS, value);
	charger->battery_status = value.intval;

	psy_do_property(charger->pdata->battery_name, get,
		POWER_SUPPLY_PROP_CHARGE_NOW, value);
	charger->battery_chg_mode = value.intval;

	psy_do_property(charger->pdata->battery_name, get,
		POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, value);
	charger->swelling_mode = value.intval;

	if (!is_wireless_type(charger->cable_type)) {
		pr_debug("%s: pad off!\n", __func__);

		/* logging for battery_dump */
		snprintf(charger->d_buf, 128, "%d,%d",
			p9222_is_on_pad(charger), p9222_check_chg_in_status(charger));
		return 0;
	}

	vrect = p9222_get_adc(charger, P9222_ADC_VRECT);
	/* If vrect output normally, other reg is read normllay. */
	if (vrect) {
		vout = p9222_get_adc(charger, P9222_ADC_VOUT);
		freq = p9222_get_adc(charger, P9222_ADC_OP_FRQ);
		curr = p9222_get_adc(charger, P9222_ADC_RX_IOUT);
		vol = p9222_get_adc(charger, P9222_ADC_VBAT_RAW);

		p9222_reg_read(charger->client, P9222_RECT_OPER_MODE, &full_bridge);
		full_bridge = (P9222_RECT_OPER_MASK & full_bridge);

		p9222_reg_read(charger->client, P9222_AP_BATTERY_MODE, &ap_mode);
		p9222_reg_read(charger->client, P9222_WPC_FLAG_REG, &vbat_monitor);
	}
	p9222_reg_read(charger->client, P9222_MODULATION_COM_DEPTH, &m_depth);

	pr_info("%s: vrect(%dmV) vout(%dmV) iout(%dmA) vbat(%dmV) ap_mode(0x%02x) SOC(%d) "
		"phm(%d) chg_mode(%d) "
		"fw(%x) vbat_monitor(%x) tx_id(%x) freq(%d) full(%x) err(%x) md(%x)\n",
		__func__, vrect, vout, curr, vol, ap_mode, capacity,
		charger->power_hold_mode, charger->charge_mode,
		charger->pdata->otp_firmware_ver, vbat_monitor,
		charger->tx_id, freq, full_bridge, charger->err_status, m_depth);

	/* logging for battery_dump */
	snprintf(charger->d_buf, 128, "%d,%d,%d,%d,%d,%d,0x%02x,%d,%d,"
		"0x%x,0x%x,0x%x,%d,0x%x,0x%x,0x%x",
		p9222_is_on_pad(charger), p9222_check_chg_in_status(charger),
		vrect, vout, curr, vol, ap_mode, charger->power_hold_mode,
		charger->charge_mode, charger->pdata->otp_firmware_ver,
		vbat_monitor, charger->tx_id, freq, full_bridge, charger->err_status, m_depth);

	if (!p9222_is_phm(charger)) {
		if (p9222_check_darkzone_state(charger, vrect, vout)) {
			p9222_send_eop(charger, POWER_SUPPLY_HEALTH_UNDERVOLTAGE);
			pr_info("%s: Unsafe voltage\n", __func__);

			__pm_wakeup_event(charger->darkzone_ws, jiffies_to_msecs(WAKE_LOCK_TIME_OUT));
			queue_delayed_work(charger->wqueue, &charger->darkzone_work, 0);
		} else if (p9222_check_low_current(charger, vout, curr) ||
				p9222_check_abnormal_acok(charger)) {
			pr_info("%s: low iout or abnormal acokr event detected!!!\n", __func__);
			sec_vote(charger->phm_vote, VOTER_WPC_CONTROL, true, WPC_PHM_ON);
			sec_votef(VN_CHG_EN, VOTER_WPC_CONTROL, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			sec_votef(VN_CHG_EN, VOTER_WPC_CONTROL, false, false);
			sec_vote(charger->phm_vote, VOTER_WPC_CONTROL, false, WPC_PHM_OFF);
			sec_vote(charger->apm_vote, VOTER_CABLE, true, WPC_APM_WAKE_UP);

			p9222_init_wa_flag(charger);
		} else {
			p9222_wpc_align_check(charger, 0);
		}
	} else {
		p9222_init_wa_flag(charger);
	}

	return 0;
}

static int p9222_monitor_wdt_kick(struct p9222_charger_data *charger)
{
	if ((charger->pdata->watchdog_test == false) &&
		(charger->wc_w_state)) {
		p9222_reg_update(charger->client, P9222_COMMAND_REG,
			P9222_CMD_SS_WATCHDOG_MASK, P9222_CMD_SS_WATCHDOG_MASK);
	}
	return 0;
}

static int p9222_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct p9222_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = p9222_get_firmware_version(charger, P9222_RX_FIRMWARE);
		pr_info("%s: rx major firmware version 0x%x\n", __func__, ret);

		if (ret >= 0)
			val->intval = 1;
		else
			val->intval = 0;

		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = (charger->darkzone_reset) ?
			POWER_SUPPLY_HEALTH_UNDERVOLTAGE : POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL:
		return -ENODATA;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->strval = charger->d_buf;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_debug("%s: cable_type =%d\n ", __func__, charger->cable_type);
		val->intval = charger->cable_type;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = p9222_is_on_pad(charger);
		pr_debug("%s: is on chg pad = %d\n ", __func__, val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = charger->temperature;
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		val->intval = charger->pdata->charging_mode;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->pdata->ic_on_mode || (charger->wc_w_state))
			val->intval = p9222_get_adc(charger, P9222_ADC_RX_IOUT);
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW: /* vout */
		if (charger->pdata->ic_on_mode || (charger->wc_w_state))
			val->intval = p9222_get_adc(charger, P9222_ADC_VOUT);
		else
			val->intval = 0;
		break;

	case POWER_SUPPLY_PROP_ENERGY_AVG: /* vrect */
		if (charger->pdata->ic_on_mode || (charger->wc_w_state))
			val->intval = p9222_get_adc(charger, P9222_ADC_VRECT);
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_WPC_FREQ_STRENGTH:
		pr_info("%s: d2d_vout_strength = %d\n",
			__func__, charger->d2d_vout_strength);
		if (!p9222_is_on_pad(charger))
			val->intval = 0;
		else
			val->intval = (charger->power_hold_mode) ?
				100 : charger->d2d_vout_strength;
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ:
				val->intval = p9222_get_adc(charger, P9222_ADC_OP_FRQ);
			pr_info("%s: Operating FQ %dkHz\n", __func__, val->intval);
				break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ_STRENGTH:
			val->intval = charger->d2d_vout_strength;
			pr_info("%s: vout strength = (%d)\n",
				__func__, charger->d2d_vout_strength);
				break;
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			if (p9222_check_cable_type(charger->cable_type, val->intval)) {
				/* try to run wpc_det work for cable type */
				charger->force_wpc_det_chk = (is_wireless_type(charger->cable_type) != charger->wc_w_state);
				queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
			} else {
				p9222_monitor_work(charger);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_POWER_SHARING_CHARGE:
			val->intval = (p9222_get_pad_type(charger, charger->tx_id) == P9222_PAD_TYPE_D2D);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID:
			val->intval = charger->tx_id;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN:
			val->intval = p9222_get_adc(charger, P9222_ADC_RX_IOUT);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_PHM:
			val->intval = charger->power_hold_mode;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void p9222_chg_set_muic_msg(struct p9222_charger_data *charger,
		enum power_supply_muic_msg muic_msg)
{
	if (!p9222_is_on_pad(charger))
		return;

	switch (muic_msg) {
	case POWER_SUPPLY_MUIC_ACOK_FALLING:
		pr_info("%s: acok falling!!\n", __func__);
		break;
	case POWER_SUPPLY_MUIC_ACOK_RISING:
		pr_info("%s: acok rising!!\n", __func__);
		charger->ab_acok_count++;
		break;
	default:
		break;
	}
}

static int p9222_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct p9222_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (val->intval == POWER_SUPPLY_STATUS_FULL) {
			if ((charger->battery_status != POWER_SUPPLY_STATUS_FULL) ||
				(!charger->cs100_status)) {
				int work_delay = p9222_rx_ic_power_on_check(charger) ?
					0 : VOUT_STABLILZATION_TIME;

				pr_info("%s: set green led (work_delay = %d)\n", __func__, work_delay);
				sec_vote(charger->phm_vote, VOTER_WPC_CS100, true, WPC_PHM_OFF);
				__pm_wakeup_event(charger->cs100_ws, jiffies_to_msecs(WAKE_LOCK_TIME_OUT));
				queue_delayed_work(charger->wqueue,
					&charger->cs100_work, msecs_to_jiffies(work_delay));
			}
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		break;
	case POWER_SUPPLY_PROP_TEMP:
		charger->temperature = val->intval;
		p9222_temperature_check(charger);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		charger->pdata->ic_on_mode = !!(val->intval);
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		switch (val->intval) {
		case SEC_BAT_CHG_MODE_CHARGING:
			charger->is_charging = true;
			break;
		case SEC_BAT_CHG_MODE_BUCK_OFF:
		case SEC_BAT_CHG_MODE_CHARGING_OFF:
		default:
			charger->is_charging = false;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_WDT_KICK:
			p9222_monitor_wdt_kick(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_CHARGE_MONITOR:
			if (get_sec_vote_result(charger->apm_vote) == WPC_APM_WAKE_UP)
				sec_vote(charger->apm_vote, VOTER_CABLE, true, WPC_APM_NORMAL);
			else
				sec_vote_refresh(charger->apm_vote);
			break;
		case POWER_SUPPLY_EXT_PROP_STORE_MODE:
			charger->store_mode = val->intval;
			break;
		case POWER_SUPPLY_EXT_PROP_MUIC_MSG:
			p9222_chg_set_muic_msg(charger, val->intval);
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void p9222_wpc_auth_check_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, wpc_auth_check_work.work);

	union power_supply_propval value;

	pr_info("%s: wc_w_state = %d, tx_id = %d\n",
		__func__, charger->wc_w_state, charger->tx_id);

	if (!p9222_is_on_pad(charger) ||
		p9222_compatible_tx_check(charger, charger->tx_id)) {
		__pm_relax(charger->wpc_auth_check_ws);
		return;
	}

	if (charger->pdata->support_legacy_pad == 0) {
		if (!p9222_send_watchdog_err_packet(charger))
			p9222_send_eop(charger, POWER_SUPPLY_HEALTH_UNDERVOLTAGE);

		value.intval = charger->cable_type = SB_CBL_ERR_WRL;
	} else {
		value.intval = charger->cable_type = SB_CBL_INCOMP_WRL;
		if (charger->tx_id == 0)
			charger->tx_id = UNKNOWN_TX_ID;
	}
	psy_do_property(charger->pdata->battery_name, set,
		POWER_SUPPLY_PROP_ONLINE, value);

	__pm_relax(charger->wpc_auth_check_ws);
}

static int p9222_wpc_auth_delayed_work(struct p9222_charger_data *charger, int sec)
{
	int delay = sec * 1000;
	int lock_timeout = sec + (sec / 2);

	pr_info("%s: delay=%d lock_timeout = %d\n", __func__, delay, lock_timeout);

	cancel_delayed_work(&charger->wpc_auth_check_work);

	if (!charger->wpc_auth_check_ws->active)
		__pm_wakeup_event(charger->wpc_auth_check_ws, jiffies_to_msecs(lock_timeout * HZ));

	queue_delayed_work(charger->wqueue,
		&charger->wpc_auth_check_work, msecs_to_jiffies(delay));
	return 0;
}

static void p9222_wpc_id_request_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, wpc_id_request_work.work);
	int work_delay = 1000;

	if (!p9222_is_on_pad(charger)) {
		pr_info("%s: skip this work because of no vbus\n", __func__);
		goto end_work;
	}

	charger->tx_id_check_cnt++;
	if ((charger->tx_id_checked) &&
		(charger->tx_id_check_cnt > 1)) {
		work_delay = 500;
		pr_info("%s: send RX-ID(%x) to TX (%d)\n",
			__func__, charger->pdata->rx_id, charger->tx_id_check_cnt);
		if (p9222_send_packet(charger, P9222_HEADER_AFC_CONF,
				P9222_RX_DATA_COM_RX_ID, (u8)charger->pdata->rx_id) < 0)
			goto end_work;

		if (charger->tx_id_check_cnt >= SEND_RX_ID_CNT)
			goto end_work;
	} else {
		pr_info("%s: request TX-ID to TX (%d)\n",
			__func__, charger->tx_id_check_cnt);
		if (p9222_send_packet(charger, P9222_HEADER_AFC_CONF,
			P9222_RX_DATA_COM_REQ_TX_ID, 0x0) < 0)
			goto end_work;

		if (charger->tx_id_check_cnt >= SEND_REQ_TX_ID_CNT) {
			p9222_wpc_auth_delayed_work(charger, WPC_AUTH_DELAY_TIME);
			goto end_work;
		}
	}

	queue_delayed_work(charger->wqueue,
		&charger->wpc_id_request_work, msecs_to_jiffies(work_delay));
	return;

end_work:
	charger->tx_id_check_cnt = 0;
	sec_vote(charger->phm_vote, VOTER_WPC_ID_CHK, false, WPC_PHM_OFF);
	__pm_relax(charger->wpc_id_request_ws);
}

static void p9222_enable_vbat_monitoring(struct p9222_charger_data *charger)
{
	if (charger->pdata->can_vbat_monitoring) {
		p9222_reg_write(charger->client, P9222_VRECT_SET_REG, 9);
		p9222_reg_update(charger->client, P9222_WPC_FLAG_REG,
			P9222_VBAT_MONITORING_MODE, P9222_VBAT_MONITORING_MODE_MASK);
	}
}

static void p9222_wpc_det_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, wpc_det_work.work);
	union power_supply_propval value;
	bool wc_w_state;
	u8 pad_mode;
	u8 vrect;

	__pm_stay_awake(charger->wpc_ws);

	wc_w_state = p9222_is_on_pad(charger);
	pr_info("%s: w(%d to %d) force(%d)\n",
		__func__, charger->wc_w_state, wc_w_state, charger->force_wpc_det_chk);
	if (!(charger->force_wpc_det_chk) && (charger->wc_w_state == wc_w_state)) {
		__pm_relax(charger->wpc_ws);
		return;
	}
	charger->force_wpc_det_chk = false;
	charger->wc_w_state = wc_w_state;
	sec_vote(charger->phm_vote, VOTER_WPC_ID_CHK, wc_w_state, WPC_PHM_OFF);
	sec_vote(charger->phm_vote, VOTER_WPC_DET, !wc_w_state, WPC_PHM_OFF);
	sec_vote(charger->phm_vote, VOTER_WPC_TX_PHM, false, WPC_PHM_OFF);

	if (wc_w_state) {
		/* enable wpc det irq */
		p9222_wpc_det_irq_enable(charger, true);

		/* set fod value */
		if(charger->pdata->fod_data_check)
			p9222_fod_set(charger);

		/* enable Mode Change INT */
		p9222_reg_update(charger->client, P9222_INT_ENABLE_L_REG,
			P9222_STAT_MODE_CHANGE_MASK, P9222_STAT_MODE_CHANGE_MASK);

		/* read vrect adjust */
		p9222_reg_read(charger->client, P9222_VRECT_SET_REG, &vrect);

		/* read pad mode */
		p9222_reg_read(charger->client, P9222_SYS_OP_MODE_REG, &pad_mode);

		if (!is_wireless_type(charger->cable_type))
			charger->cable_type = SB_CBL_WRL;
		value.intval = charger->cable_type;
		psy_do_property(charger->pdata->battery_name, set,
			POWER_SUPPLY_PROP_ONLINE, value);

		pr_info("%s: wpc activated(vrect = 0x%x, pad_mode = 0x%x)\n",
			__func__, vrect, pad_mode);

		/* set watchdog */
		p9222_set_watchdog_timer(charger, true);

		sec_vote(charger->apm_vote, VOTER_CABLE, true, WPC_APM_NORMAL);

		__pm_stay_awake(charger->wpc_id_request_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_id_request_work, 0);

		if (charger->cs100_status) {
			sec_vote(charger->phm_vote, VOTER_WPC_CS100, true, WPC_PHM_OFF);
			__pm_stay_awake(charger->cs100_ws);
			queue_delayed_work(charger->wqueue,
				&charger->cs100_work, msecs_to_jiffies(VOUT_STABLILZATION_TIME));
		}
	} else {
		/* disable wpc det irq */
		p9222_wpc_det_irq_enable(charger, false);

		/* Send last tx_id to battery to cound tx_id */ 
		value.intval = charger->tx_id;
		psy_do_property(charger->pdata->wireless_name,
			set, POWER_SUPPLY_PROP_AUTHENTIC, value);

		charger->cable_type = SB_CBL_NONE;
		charger->charge_mode = P9222_CHARGE_MODE_NONE;
		charger->support_power_hold = 0;
		charger->tx_id = 0x0;
		charger->tx_id_checked = false;
		charger->phm_set_fail = false;
		charger->err_status = 0;
		charger->d2d_vout_strength = 100;
		charger->darkzone_reset = false;
		charger->phm_chk_cnt = 0;
		charger->d2d_checking_align = false;
		charger->d2d_align_check_start.tv_sec = 0;
		charger->tx_id_check_cnt = 0;
		charger->cs100_status = false;

		charger->ssp_missing = false;
		charger->ssp_missing_count = 0;
		p9222_init_wa_flag(charger);

		value.intval = charger->cable_type;
		psy_do_property(charger->pdata->battery_name, set,
			POWER_SUPPLY_PROP_ONLINE, value);

		pr_info("%s: wpc deactivated, set V_INT as PD\n", __func__);

		/* relax wake lock */
		__pm_relax(charger->wpc_tx_id_check_ws);
		__pm_relax(charger->wpc_auth_check_ws);
		__pm_relax(charger->wpc_id_request_ws);
		__pm_relax(charger->power_hold_chk_ws);
		__pm_relax(charger->retry_phm_free_ws);
		__pm_relax(charger->darkzone_ws);
		__pm_relax(charger->align_check_ws);
		__pm_relax(charger->cs100_ws);

		/* cancel work */
		cancel_delayed_work(&charger->wpc_isr_work);
		cancel_delayed_work(&charger->wpc_auth_check_work);
		cancel_delayed_work(&charger->wpc_id_request_work);
		cancel_delayed_work(&charger->power_hold_chk_work);
		cancel_delayed_work(&charger->retry_phm_free_work);
		cancel_delayed_work(&charger->darkzone_work);
		cancel_delayed_work(&charger->align_check_work);
		cancel_delayed_work(&charger->cs100_work);

		/* init vote */
		sec_vote(charger->phm_vote, VOTER_WPC_CS100, false, WPC_PHM_OFF);
		sec_vote(charger->phm_vote, VOTER_WPC_ALIGN_CHK, false, WPC_PHM_OFF);
		sec_vote(charger->apm_vote, VOTER_TEST, false, WPC_APM_NONE);
		sec_vote(charger->apm_vote, VOTER_WPC_CONTROL, false, WPC_APM_NONE);
		sec_vote(charger->apm_vote, VOTER_CABLE, false, WPC_APM_NONE);
		sec_vote(charger->ldo_vote, VOTER_WPC_CONTROL, false, WPC_LDO_ON);
	}

	__pm_relax(charger->wpc_ws);
}

static void p9222_curr_measure_work(struct work_struct *work)
{
	struct p9222_charger_data *charger = container_of(work,
		struct p9222_charger_data, curr_measure_work.work);
	union power_supply_propval value = {0,};
	int i = 0;
	int acok_cnt = 0;
	bool valid_acok = false;

	pr_info("%s\n", __func__);

	while (i < 100) {
		if (!p9222_is_on_pad(charger)) {
			pr_info("%s: pad is off!\n", __func__);
			break;
		}

		if (p9222_check_chg_in_status(charger) == true) {
			if (acok_cnt > 10) {
				pr_info("%s: valid acok. go to measurement mode\n", __func__);
				valid_acok = true;
				break;
			}
			acok_cnt++;
		} else {
			acok_cnt = 0;
		}
		i++;
		msleep(100);
	}

	if (!valid_acok) {
		pr_err("%s: acok not stable.\n", __func__);
		return;
	}

	sec_vote(charger->apm_vote, VOTER_TEST, true, WPC_APM_TEST);

	psy_do_property(charger->pdata->charger_name, set,
		POWER_SUPPLY_EXT_PROP_FORCED_JIG_MODE, value);
}

static const char *p9222_get_tx_chg_stop_str(u8 data)
{
	switch (data) {
	case P9222_TX_CHG_STOP_TX_OTP:
		return "TX OTP";
	case P9222_TX_CHG_STOP_TX_OCP:
		return "TX OCP";
	case P9222_TX_CHG_STOP_DARKZONE:
		return "DARKZONE";
	case P9222_TX_CHG_STOP_FOD:
		return "FOD";
	case P9222_TX_CHG_STOP_AUTH_FAIL:
		return "AUTH FAIL";
	case P9222_TX_CHG_STOP_DISCON_FOD:
		return "DISCON FOD";
	case P9222_TX_CHG_STOP_OC_FOD:
		return "OC FOD";
	case P9222_TX_CHG_STOP_WATCHDOG:
		return "WATCHDOG";
	}

	return "RESERVED";
}

static void p9222_set_tx_chg_stop(struct p9222_charger_data *charger, u8 data)
{
	switch (data) {
	case P9222_TX_CHG_STOP_DARKZONE:
	case P9222_TX_CHG_STOP_TX_OTP:
	case P9222_TX_CHG_STOP_TX_OCP:
	case P9222_TX_CHG_STOP_FOD:
	case P9222_TX_CHG_STOP_AUTH_FAIL:
	case P9222_TX_CHG_STOP_DISCON_FOD:
	case P9222_TX_CHG_STOP_OC_FOD:
	case P9222_TX_CHG_STOP_WATCHDOG:
	default:
		break;
	}

	pr_info("%s: received chg_stop(%s)\n",
		__func__, p9222_get_tx_chg_stop_str(data));
}

static void p9222_wpc_isr_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, wpc_isr_work.work);
	u8 cmd_data, val_data, data;

	if (!p9222_is_on_pad(charger)) {
		pr_info("%s: wc_w_state is 0. exit wpc_isr_work.\n", __func__);
		goto end_wpc_isr_work;
	}

	p9222_reg_read(charger->client, P9222_TX_DATA_COMMAND, &cmd_data);
	p9222_reg_read(charger->client, P9222_TX_DATA_VALUE0, &val_data);
	pr_info("%s: cmd(%x) val(%x)\n", __func__, cmd_data, val_data);

	switch (cmd_data) {
	case P9222_TX_DATA_COM_CHG_STOP:
		p9222_set_tx_chg_stop(charger, val_data);
		break;
	case P9222_TX_DATA_COM_TX_ID:
		if (!is_wireless_type(charger->cable_type)) {
			if ((p9222_is_multi_pad(p9222_get_pad_type(charger, val_data))) ||
				(p9222_check_illegal_pad(charger, val_data))) {
				pr_info("%s: wait acok falling.", __func__);
			} else {
				queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
				pr_info("%s: use tx_id_irq to fast detect the wireless charging.\n", __func__);
			}
			break;
		} else if (p9222_compatible_tx_check(charger, charger->tx_id)) {
			if (p9222_is_phm(charger)) {
				pr_info("%s: abnormal case - PHM was recovered by SSP Missing, try to start WPC auth again and set PHM.\n",
					__func__);
				charger->ssp_missing =
					p9222_is_multi_pad(p9222_get_pad_type(charger, charger->tx_id));

				sec_vote(charger->phm_vote, VOTER_WPC_TX_PHM, false, WPC_PHM_OFF);

				charger->force_wpc_det_chk = true;
				__pm_stay_awake(charger->phm_free_ws);
				queue_delayed_work(charger->wqueue, &charger->phm_free_work, msecs_to_jiffies(1000));
				break;
			} else if ((charger->tx_id == val_data) && (charger->tx_id_checked)) {
				pr_info("%s: Skip the behavior below if it is the same TX ID(0x%x) or already processed TX(%s).\n",
					__func__, val_data, GET_BOOL_STR(charger->tx_id_checked));
				break;
			}
			/* reset tx_id_checked to false because tx id changed. */
			charger->tx_id_checked = false;
			charger->tx_id_check_cnt = 0;
		} else {
			if (is_incomp_wrl_type(charger->cable_type)) {
				pr_info("%s: skip work to keep incompatible pad type.\n", __func__);
				break;
			}
		}

		charger->tx_id = val_data;
		/* Check TX ID */
		if (p9222_compatible_tx_check(charger, val_data)) {
			union power_supply_propval value = {0,};
			int pad_type = P9222_PAD_TYPE_NORMAL;
			bool sup_phm = false;

			pad_type = p9222_get_pad_type(charger, charger->tx_id);
			sup_phm = p9222_get_phm(charger, charger->tx_id);
			pr_info("%s: data = 0x%x, type = %d, sup_phm = %d, compatible TX pad\n",
				__func__, val_data, pad_type, sup_phm);

			charger->support_power_hold = sup_phm;
			switch (pad_type) {
			case P9222_PAD_TYPE_NORMAL:
				charger->cable_type = SB_CBL_INBOX_WRL;
				break;
			case P9222_PAD_TYPE_D2D:
				charger->cable_type = SB_CBL_D2D_WRL;
				p9222_wpc_align_check(charger, ALIGN_WORK_DELAY);
				break;
			case P9222_PAD_TYPE_FAST_MULTI:
				charger->cable_type = SB_CBL_FM_WRL;
				break;
			case P9222_PAD_TYPE_MULTI:
			default:
				charger->cable_type = SB_CBL_WRL;
				break;
			}

			value.intval = charger->cable_type;
			psy_do_property(charger->pdata->battery_name, set,
				POWER_SUPPLY_PROP_ONLINE, value);

			charger->tx_id_checked = true;
			charger->tx_id_check_cnt = 1;
			sec_vote_refresh(charger->apm_vote);
		} else if (val_data == P9222_FACTORY_MODE_TX_ID) {
			pr_info("%s: id 0x%x current measure TX pad\n", __func__, val_data);
			queue_delayed_work(charger->wqueue, &charger->curr_measure_work,
				msecs_to_jiffies(2000));
		} else {
			pr_info("%s: incompatible TX pad\n", __func__);
		}
		break;
	case P9222_TX_DATA_COM_INCOMPATIBLE_PAD:
		pr_info("%s: incompatible TX pad\n", __func__);
		break;
	default:
		pr_info("%s: invalid cmd type\n", __func__);
		break;
	}

	p9222_reg_read(charger->client, P9222_INT_L_REG, &data);
	if (data & P9222_STAT_TX_DATA_RECEIVED_MASK) {
		p9222_reg_write(charger->client, P9222_INT_CLEAR_L_REG,
			P9222_INT_TX_DATA_RECEIVED);
	}

end_wpc_isr_work:
	__pm_relax(charger->wpc_tx_id_check_ws);
}

static irqreturn_t p9222_wpc_det_irq_thread(int irq, void *irq_data)
{
	struct p9222_charger_data *charger = irq_data;
	bool pdet_state = p9222_is_on_pad(charger);

	pr_info("%s: pdet_state = %d!\n", __func__, pdet_state);

	/* prevent recognition during ping duration time.*/
	if (!pdet_state) {
		charger->force_wpc_det_chk = true;
		queue_delayed_work(charger->wqueue,
			&charger->wpc_det_work, msecs_to_jiffies(40));
	} else {
		queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
	}

	return IRQ_HANDLED;
}

static irqreturn_t p9222_wpc_irq_thread(int irq, void *irq_data)
{
	struct p9222_charger_data *charger = irq_data;
	int ret;
	u8 irq_src[2];
	u8 reg_data;
	u8 data1, data2;

	__pm_stay_awake(charger->wpc_ws);
	mutex_lock(&charger->charger_lock);

	pr_info("%s: start, status = %d\n", __func__, gpio_get_value(charger->pdata->wpc_int));

	/* check again firmare version support vbat monitoring */
	if (!charger->pdata->otp_firmware_ver) {
		ret = p9222_get_firmware_version(charger, P9222_RX_FIRMWARE);
		if (ret > 0) {
			pr_debug("%s: rx major firmware version 0x%x\n", __func__, ret);
			charger->pdata->otp_firmware_ver = ret;
		}
	}

	ret = p9222_reg_read(charger->client, P9222_INT_L_REG, &irq_src[0]);
	ret = p9222_reg_read(charger->client, P9222_INT_H_REG, &irq_src[1]);
	if (ret < 0) {
		pr_err("%s: Failed to read interrupt source: %d\n",
			__func__, ret);
		__pm_relax(charger->wpc_ws);
		mutex_unlock(&charger->charger_lock);
		return IRQ_NONE;
	}

	pr_info("%s: interrupt source(0x%x)\n", __func__, irq_src[1] << 8 | irq_src[0]);

	if(irq_src[0] & P9222_STAT_MODE_CHANGE_MASK) {
		ret = p9222_reg_read(charger->client, P9222_SYS_OP_MODE_REG, &reg_data);
		pr_info("%s: MODE CHANGE IRQ - SYS OP MODE = 0x%x\n", __func__, reg_data);
	}

	if(irq_src[0] & P9222_STAT_VOUT_MASK) {
		ret = p9222_reg_read(charger->client, P9222_INT_STATUS_L_REG, &reg_data);
		pr_info("%s: VOUT IRQ - STAT VOUT = 0x%x\n", __func__, reg_data);
	}

	if(irq_src[0] & P9222_STAT_TX_DATA_RECEIVED_MASK) {
		pr_info("%s: TX RECEIVED IRQ\n", __func__);

		p9222_reg_read(charger->client, P9222_TX_DATA_COMMAND, &data1);
		p9222_reg_read(charger->client, P9222_TX_DATA_VALUE0, &data2);
		pr_info("%s: P9222_TX_DATA_COMMAND = %x tx_id = %x\n", __func__, data1, data2);

		if (!delayed_work_pending(&charger->wpc_isr_work)) {
			__pm_wakeup_event(charger->wpc_tx_id_check_ws, jiffies_to_msecs(HZ * 5));
			queue_delayed_work(charger->wqueue, &charger->wpc_isr_work, msecs_to_jiffies(0));
		}
	}

	if (irq_src[1] & P9222_STAT_OVER_CURR_MASK)
		pr_info("%s: OVER CURRENT IRQ\n", __func__);

	if (irq_src[1] & P9222_STAT_OVER_TEMP_MASK)
		pr_info("%s: OVER TEMP IRQ\n", __func__);

	if (irq_src[1] & P9222_STAT_TX_CONNECT_MASK)
		pr_info("%s: TX CONNECT IRQ\n", __func__);

	if ((irq_src[1] << 8 | irq_src[0]) != 0) {
		msleep(5);

		/* clear intterupt */
		p9222_reg_write(charger->client, P9222_INT_CLEAR_L_REG, irq_src[0]);
		p9222_reg_write(charger->client, P9222_INT_CLEAR_H_REG, irq_src[1]);
		p9222_set_cmd_reg(charger, 0x20, P9222_CMD_CLEAR_INT_MASK); // command
	}
	/* debug */
	__pm_relax(charger->wpc_ws);
	mutex_unlock(&charger->charger_lock);

	pr_info("%s: finish, status = %d\n", __func__, gpio_get_value(charger->pdata->wpc_int));
	return IRQ_HANDLED;
}

static irqreturn_t p9222_irq_handler(int irq, void *irq_data)
{
	/* prevent to run irq_thread during probe because of wireless scenario */
	return IS_ERR_OR_NULL(((struct p9222_charger_data *)irq_data)->psy_chg) ?
		IRQ_HANDLED : IRQ_WAKE_THREAD;
}

enum {
	WPC_FW_VER = 0,
	WPC_IBUCK,
	WPC_WATCHDOG,
	WPC_ADDR,
	WPC_SIZE,
	WPC_DATA,
};


static struct device_attribute p9222_attributes[] = {
	SEC_WPC_ATTR(fw),
	SEC_WPC_ATTR(ibuck),
	SEC_WPC_ATTR(watchdog_test),
	SEC_WPC_ATTR(addr),
	SEC_WPC_ATTR(size),
	SEC_WPC_ATTR(data),
};

ssize_t sec_wpc_show_attrs(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct p9222_charger_data *charger = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - p9222_attributes;
	int i = 0;

	switch (offset) {
	case WPC_FW_VER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", charger->pdata->otp_firmware_ver);
		break;
	case WPC_IBUCK:
	{
		int ibuck = 0;

		ibuck = p9222_get_adc(charger, P9222_ADC_RX_IOUT);
		ibuck -= P9222_I_OUT_OFFSET;
		pr_info("%s: raw iout %dmA\n", __func__, ibuck);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", ibuck);
	}
		break;
	case WPC_WATCHDOG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", charger->pdata->watchdog_test);
		break;
	case WPC_ADDR:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", charger->addr);
		break;
	case WPC_SIZE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", charger->size);
		break;
	case WPC_DATA:
		if (charger->size == 0)
			charger->size = 1;

		if (charger->size + charger->addr <= 0xFFFF) {
			u8 data;
			int j;

			for (j = 0; j < charger->size; j++) {
				if (p9222_reg_read(charger->client, charger->addr + j, &data) < 0) {
					dev_info(charger->dev, "%s: read fail\n", __func__);
					i += scnprintf(buf + i, PAGE_SIZE - i,
						"addr: 0x%x read fail\n", charger->addr + j);
					continue;
				}
				i += scnprintf(buf + i, PAGE_SIZE - i,
					"addr: 0x%x, data: 0x%x\n", charger->addr + j, data);
			}
		}
		break;
	default:
		break;
	}

	return i;
}

ssize_t sec_wpc_store_attrs(
					struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct p9222_charger_data *charger =
		power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - p9222_attributes;
	int x, ret = -EINVAL;

	switch (offset) {
	case WPC_WATCHDOG:
		sscanf(buf, "%d\n", &x);
		if (x == 1)
			charger->pdata->watchdog_test = true;
		else if (x == 0)
			charger->pdata->watchdog_test = false;
		else
			pr_debug("%s: non valid input\n", __func__);
		pr_info("%s: watchdog test is set to %d\n", __func__, charger->pdata->watchdog_test);
		ret = count;
		break;
	case WPC_ADDR:
		if (sscanf(buf, "0x%x\n", &x) == 1) {
			charger->addr = x;
		}
		ret = count;
		break;
	case WPC_SIZE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			charger->size = x;
		}
		ret = count;
		break;
	case WPC_DATA:
		if (sscanf(buf, "0x%x", &x) == 1) {
			u8 data = x;
			if (p9222_reg_write(charger->client, charger->addr, data) < 0)
			{
				dev_info(charger->dev,
						"%s: addr: 0x%x write fail\n", __func__, charger->addr);
			}
		}
		ret = count;
		break;
	default:
		break;
	}
	return ret;
}

static int sec_wpc_create_attrs(struct device *dev)
{
	unsigned long i;
	int rc;

	for (i = 0; i < ARRAY_SIZE(p9222_attributes); i++) {
		rc = device_create_file(dev, &p9222_attributes[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	while (i--)
		device_remove_file(dev, &p9222_attributes[i]);
create_attrs_succeed:
	return rc;
}

static int p9222_wpc_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
	struct p9222_charger_data *charger =
		container_of(nb, struct p9222_charger_data, wpc_nb);

	pr_info("%s: action=%lu, attached_dev=%d\n", __func__, action, attached_dev);

	switch (attached_dev) {
	case ATTACHED_DEV_TA_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			p9222_set_watchdog_timer(charger, true);
			p9222_get_firmware_version(charger, P9222_RX_FIRMWARE);
			p9222_get_firmware_version(charger, P9222_TX_FIRMWARE);
		}
		break;
	case ATTACHED_DEV_WIRELESS_TA_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH) {
			pr_info("%s: MUIC_NOTIFY_CMD_DETACH\n", __func__);
			p9222_wpc_align_check(charger, 0);
			charger->force_wpc_det_chk = !p9222_rx_ic_power_on_check(charger);
		} else if (action == MUIC_NOTIFY_CMD_ATTACH) {
			pr_info("%s: MUIC_NOTIFY_CMD_ATTACH\n", __func__);
			queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
			p9222_enable_vbat_monitoring(charger);
		}
		break;
	default:
		break;
	}

	return 0;
}

static void p9222_init_work(struct work_struct *work)
{
	struct p9222_charger_data *charger =
		container_of(work, struct p9222_charger_data, init_work.work);

	if (!p9222_is_on_pad(charger)) {
		pr_info("%s: does not need to check irq and case\n", __func__);
	} else if (p9222_rx_ic_power_on_check(charger)) {
		/* clear irq */
		int ret = 0;
		u8 irq_src[2];

		ret = p9222_reg_read(charger->client, P9222_INT_L_REG, &irq_src[0]);
		ret = p9222_reg_read(charger->client, P9222_INT_H_REG, &irq_src[1]);
		if (ret < 0) {
			pr_err("%s: Failed to read interrupt source: %d\n",
				__func__, ret);
		} else if ((irq_src[1] << 8 | irq_src[0]) != 0) {
			pr_info("%s: clear irq 0x%x\n", __func__, (irq_src[1] << 8 | irq_src[0]));

			if (irq_src[0] & P9222_STAT_TX_DATA_RECEIVED_MASK) {
				u8 cmd_data, val_data;

				p9222_reg_read(charger->client, P9222_TX_DATA_COMMAND, &cmd_data);
				p9222_reg_read(charger->client, P9222_TX_DATA_VALUE0, &val_data);
				pr_info("%s: P9222_TX_DATA_COMMAND = %x data = %x\n", __func__, cmd_data, val_data);

				if (cmd_data == P9222_TX_DATA_COM_TX_ID)
					p9222_check_illegal_pad(charger, val_data);
			}

			/* clear intterupt */
			p9222_reg_write(charger->client, P9222_INT_CLEAR_L_REG, irq_src[0]);
			p9222_reg_write(charger->client, P9222_INT_CLEAR_H_REG, irq_src[1]);
			p9222_set_cmd_reg(charger, 0x20, P9222_CMD_CLEAR_INT_MASK); // command
		}
	} else {
		/* abnormal case - reset IC */
		pr_info("%s: anormal case - reset IC\n", __func__);
		__pm_stay_awake(charger->darkzone_ws);
		queue_delayed_work(charger->wqueue, &charger->darkzone_work, 0);
	}

	/* run wpc det work for detacting wireless charger */
	queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);

#if defined(CONFIG_MUIC_NOTIFIER)
	muic_notifier_register(&charger->wpc_nb, p9222_wpc_handle_notification, MUIC_NOTIFY_DEV_CHARGER);
#endif
}

static int p9222_parse_dt(struct device *dev, p9222_charger_platform_data_t *pdata)
{
	struct device_node *np = dev->of_node;
	const u32 *p;
	int len, ret, i;
	enum of_gpio_flags irq_gpio_flags;
	/*changes can be added later, when needed*/

	ret = of_property_read_string(np,
		"p9222,charger_name", (char const **)&pdata->charger_name);
	if (ret) {
		pr_info("%s: Charger name is Empty\n", __func__);
		pdata->charger_name = "sec-charger";
	}

	ret = of_property_read_string(np,
		"p9222,fuelgauge_name", (char const **)&pdata->fuelgauge_name);
	if (ret) {
		pr_info("%s: Fuelgauge name is Empty\n", __func__);
		pdata->fuelgauge_name = "sec-fuelgauge";
	}

	ret = of_property_read_string(np,
		"p9222,battery_name", (char const **)&pdata->battery_name);
	if (ret) {
		pr_info("%s: battery_name is Empty\n", __func__);
		pdata->battery_name = "battery";
	}

	ret = of_property_read_string(np,
		"p9222,wireless_name", (char const **)&pdata->wireless_name);
	if (ret) {
		pr_info("%s: wireless_name is Empty\n", __func__);
		pdata->wireless_name = "wireless";
	}

	ret = of_property_read_string(np, "p9222,wireless_charger_name",
		(char const **)&pdata->wireless_charger_name);
	if (ret) {
		pr_info("%s: wireless_charger_name is Empty\n", __func__);
		pdata->wireless_charger_name = "wpc";
	}

	/* wpc_det */
	ret = pdata->wpc_det = of_get_named_gpio_flags(np, "p9222,wpc_det",
			0, &irq_gpio_flags);
	if (ret < 0) {
		dev_err(dev, "%s: can't get wpc_det\r\n", __func__);
	} else {
		if (gpio_is_valid(pdata->wpc_det)) {
		    ret = gpio_request(pdata->wpc_det, "wpc_det");
		    if (ret) {
				dev_err(dev, "%s: can't get wpc_det\r\n", __func__);
		    } else {
		    	ret = gpio_direction_input(pdata->wpc_det);
				if (ret) {
					dev_err(dev, "%s: wpc_det set input fail\r\n", __func__);
				}
				pdata->irq_wpc_det = gpio_to_irq(pdata->wpc_det);
				pr_info("%s: wpc_det = 0x%x, irq_wpc_det = 0x%x (%d)\n", __func__,
					pdata->wpc_det, pdata->irq_wpc_det, pdata->irq_wpc_det);
		    }
		}
	}

	/* wpc_int */
	ret = pdata->wpc_int = of_get_named_gpio_flags(np, "p9222,wpc_int",
			0, &irq_gpio_flags);
	if (ret < 0) {
		dev_err(dev, "%s: can't wpc_int\r\n", __func__);
	} else {
		pdata->irq_wpc_int = gpio_to_irq(pdata->wpc_int);
		pr_info("%s: wpc_int = 0x%x, irq_wpc_int = 0x%x\n", __func__,
			pdata->wpc_int, pdata->irq_wpc_int);
	}

	/* wpc_en */
	ret = pdata->wpc_en = of_get_named_gpio_flags(np, "p9222,wpc_en",
			0, &irq_gpio_flags);
	if (ret < 0) {
		dev_err(dev, "%s: can't wpc_en\r\n", __func__);
	} else {
		if (gpio_is_valid(pdata->wpc_en)) {
		    ret = gpio_request(pdata->wpc_en, "wpc_en");
			if (ret)
				dev_err(dev, "%s: can't get wpc_en\r\n", __func__);
			else
				gpio_direction_output(pdata->wpc_en, 0);
		}
	}

	ret = of_property_read_u32(np, "p9222,cc_cv_threshold",
		&pdata->cc_cv_threshold);
	if (ret < 0) {
		pr_err("%s: error reading cc_cv_threshold(%d)\n", __func__, ret);
		pdata->cc_cv_threshold = 880;
	}

	ret = of_property_read_u32(np, "p9222,half_bridge_threshold",
		&pdata->half_bridge_threshold);
	if (ret < 0) {
		pr_err("%s: error reading half_bridge_threshold(%d)\n", __func__, ret);
		pdata->half_bridge_threshold = 950;
	}

	ret = of_property_read_u32(np, "p9222,half_bridge_vout",
		&pdata->half_bridge_vout);
	if (ret < 0) {
		pr_err("%s: error reading half_bridge_vout(%d)\n", __func__, ret);
		pdata->half_bridge_vout = 4500;
	}

	ret = of_property_read_u32(np, "p9222,darkzone_expired_time",
		&pdata->darkzone_expired_time);
	if (ret < 0) {
		pr_err("%s: error reading darkzone_expired_time(%d)\n", __func__, ret);
		pdata->darkzone_expired_time = 120;
	}

	ret = of_property_read_u32(np, "p9222,low_current_expired_time",
		&pdata->low_current_expired_time);
	if (ret < 0) {
		pr_err("%s: error reading low_current_expired_time(%d)\n", __func__, ret);
		pdata->low_current_expired_time = 300;
	}

	ret = of_property_read_u32(np, "p9222,ab_acok_count",
		&pdata->ab_acok_count);
	if (ret < 0) {
		pr_err("%s: error reading ab_acok_count(%d)\n", __func__, ret);
		pdata->ab_acok_count = 60;
	}

	ret = of_property_read_u32(np, "p9222,ab_acok_time",
		&pdata->ab_acok_time);
	if (ret < 0) {
		pr_err("%s: error reading ab_acok_time(%d)\n", __func__, ret);
		pdata->ab_acok_time = 10;
	}

	ret = of_property_read_u32(np, "p9222,rx_id",
		&pdata->rx_id);
	if (ret < 0) {
		pr_err("%s: error reading rx_id %d\n", __func__, ret);
		pdata->rx_id = 0;
	}

	pdata->can_vbat_monitoring = of_property_read_bool(np, "p9222,vbat-monitoring");

	p = of_get_property(np, "p9222,charger_type", &len);
	if (p) {
		pdata->num_compatible_tx = len / sizeof(struct p9222_charger_type);
		pdata->charger_type = kzalloc(len, GFP_KERNEL);

		ret = of_property_read_u32_array(np, "p9222,charger_type",
				 (u32 *)pdata->charger_type, len/sizeof(u32));
		if (ret) {
			pr_err("%s: failed to read p9222,charger_type: %d\n", __func__, ret);
			kfree(pdata->charger_type);
			pdata->charger_type = NULL;
			pdata->num_compatible_tx = 0;
		} else {
			for (i = 0; i < pdata->num_compatible_tx; i++)
				pr_info("charger_type tx_id = 0x%02x power_hold = %d tx_pad_type(%d) cc(%x) cv(%x) halfbridge(%x) phm_toggle(%d)\n",
					pdata->charger_type[i].compatible_tx_id,
					pdata->charger_type[i].support_power_hold,
					pdata->charger_type[i].tx_pad_type,
					pdata->charger_type[i].cc_ap_mode,
					pdata->charger_type[i].cv_ap_mode,
					pdata->charger_type[i].half_bridge_ap_mode,
					pdata->charger_type[i].phm_free_toggle_time);
		}
	}

	p = of_get_property(np, "p9222,sec_mode_data", &len);
	if (p) {
		pdata->num_sec_mode = len / sizeof(struct p9222_sec_mode_config_data);
		pdata->sec_mode_config_data = kzalloc(len, GFP_KERNEL);
		ret = of_property_read_u32_array(np, "p9222,sec_mode_data",
				 (u32 *)pdata->sec_mode_config_data, len/sizeof(u32));
		if (ret) {
			pr_err("%s: failed to read p9222,sec_mode_data: %d\n", __func__, ret);
			kfree(pdata->sec_mode_config_data);
			pdata->sec_mode_config_data = NULL;
			pdata->num_sec_mode = 0;
		}
		pr_err("%s: num_sec_mode : %d\n", __func__, pdata->num_sec_mode);
		for (len = 0; len < pdata->num_sec_mode; ++len)
			pr_err("mode %d : vrect:%d, vout:%d\n",
				len, pdata->sec_mode_config_data[len].vrect, pdata->sec_mode_config_data[len].vout);
	} else
		pr_err("%s: there is no p9222,sec_mode_data\n", __func__);

	p = of_get_property(np, "p9222,op_freq", &len);
	if (p) {
		pdata->num_op_freq_list = len / sizeof(struct p9222_op_freq_info);
		pdata->op_freq_info = kzalloc(len, GFP_KERNEL);

		ret = of_property_read_u32_array(np, "p9222,op_freq",
				 (u32 *)pdata->op_freq_info, len/sizeof(u32));
		if (ret) {
			pr_err("%s: failed to read p9222,op_freq: %d\n", __func__, ret);
			kfree(pdata->op_freq_info);
			pdata->op_freq_info = NULL;
			pdata->num_op_freq_list = 0;
		} else {
			for (i = 0; i < pdata->num_op_freq_list; i++)
				pr_info("op_freq_info op_freq = %d d2d_vout_strength(%d)\n",
					pdata->op_freq_info[i].d2d_vout_buf,
					pdata->op_freq_info[i].d2d_vout_strength);
		}
	}

	p = of_get_property(np, "p9222,ex_low_vout_level", &len);
	if (p) {
		pdata->num_ex_low_vout = len / sizeof(struct p9222_ex_low_vout_info);
		pdata->ex_low_vout = kzalloc(len, GFP_KERNEL);
		if (pdata->ex_low_vout == NULL) {
			pr_err("%s: line(%d) Memory is not enough.\n", __func__, __LINE__);
			return -ENOMEM;
		}

		ret = of_property_read_u32_array(np, "p9222,ex_low_vout_level",
				 (u32 *)pdata->ex_low_vout, len/sizeof(u32));
		if (ret) {
			pr_err("%s: failed to read p9222,ex_low_vout_level: %d\n", __func__, ret);
			kfree(pdata->ex_low_vout);
			pdata->ex_low_vout = NULL;
			pdata->num_ex_low_vout = 0;
		} else {
			for (i = 0; i < pdata->num_ex_low_vout; i++)
				pr_info("ex_low_vout_level tx_id = 0x%02x vout = %d\n",
					pdata->ex_low_vout[i].tx_id,
					pdata->ex_low_vout[i].low_vout);
		}
	}

	p = of_get_property(np, "p9222,ap_mode_table", &len);
	if (p) {
		pdata->num_ap_mode_table = len / sizeof(struct p9222_ap_table_info);
		pdata->ap_mode_table_info = kzalloc(len, GFP_KERNEL);
		if (pdata->ap_mode_table_info == NULL) {
			pr_err("%s: line(%d) Memory is not enough.\n", __func__, __LINE__);
			return -ENOMEM;
		}

		ret = of_property_read_u32_array(np, "p9222,ap_mode_table",
				 (u32 *)pdata->ap_mode_table_info, len/sizeof(u32));
		if (ret) {
			pr_err("%s: failed to read p9222,ap_mode_table: %d\n", __func__, ret);
			kfree(pdata->ap_mode_table_info);
			pdata->ap_mode_table_info = NULL;
			pdata->num_ap_mode_table = 0;
			pdata->use_ap_mode_table = false;
		} else {
			pdata->use_ap_mode_table = true;
			for (i = 0; i < pdata->num_ap_mode_table; i++)
				pr_info("ap_mode_table_info ap_mode = %x vout(%d) vrect(%d) margin(%d)\n",
					pdata->ap_mode_table_info[i].ap_mode,
					pdata->ap_mode_table_info[i].vout,
					pdata->ap_mode_table_info[i].vrect,
					pdata->ap_mode_table_info[i].batt_mode_margin);
		}
	} else {
		pdata->use_ap_mode_table = false;
	}

	ret = of_property_read_u32(np, "p9222,tx-off-high-temp",
		&pdata->tx_off_high_temp);
	if (ret) {
		pr_info("%s: TX-OFF-TEMP is Empty\n", __func__);
		pdata->tx_off_high_temp = INT_MAX;
	}

	ret = of_property_read_u32(np, "p9222,ping_duration",
		&pdata->ping_duration);
	if (ret) {
		pr_info("%s: ping_duration Empty\n", __func__);
		pdata->ping_duration = 350;
	}

	ret = of_property_read_u32(np, "p9222,ps_wpc_en_toggle_time",
		&pdata->ps_wpc_en_toggle_time);
	if (ret) {
		pr_info("%s: ps_wpc_en_toggle_time Empty\n", __func__);
		pdata->ps_wpc_en_toggle_time = 510;
	}

	ret = of_property_read_u32(np, "p9222,support_legacy_pad",
		&pdata->support_legacy_pad);
	if (ret) {
		pr_info("%s: support_legacy_pad Empty\n", __func__);
		pdata->support_legacy_pad = 0;
	}

	ret = of_property_read_u32(np, "p9222,boot_ap_mode",
		&pdata->boot_ap_mode);
	if (ret)
		pdata->boot_ap_mode = P9222_AP_BATTERY_BOOT_MODE;
	pr_info("%s: boot_ap_mode(%x)\n", __func__, pdata->boot_ap_mode);

	ret = of_property_read_u32(np, "p9222,low_vout_level",
		&pdata->low_vout_level);
	if (ret) {
		pr_info("%s: low_vout_level Empty\n", __func__);
		pdata->low_vout_level = 4500;
	}

	ret = of_property_read_u32(np, "p9222,enable_batt_mode_headroom_margin",
		&pdata->enable_batt_mode_headroom_margin);
	if (ret)
		pdata->enable_batt_mode_headroom_margin = 1;
	pr_info("%s: enable_batt_mode_headroom_margin(%x)\n", __func__, pdata->enable_batt_mode_headroom_margin);

	return 0;
}

static const struct power_supply_desc wpc_power_supply_desc = {
	.name = "wpc",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = wpc_props,
	.num_properties = ARRAY_SIZE(wpc_props),
	.get_property = p9222_chg_get_property,
	.set_property = p9222_chg_set_property,
};

static int p9222_charger_probe(
						struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct p9222_charger_data *charger;
	p9222_charger_platform_data_t *pdata = client->dev.platform_data;
	struct power_supply_config wpc_cfg = {};
	int ret = 0;

	dev_info(&client->dev,
		"%s: p9222 Charger Driver Loading\n", __func__);

	pdata = devm_kzalloc(&client->dev, sizeof(p9222_charger_platform_data_t), GFP_KERNEL);
	if (!pdata) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}
	ret = p9222_parse_dt(&client->dev, pdata);
	if (ret < 0)
		return ret;

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (charger == NULL) {
		dev_err(&client->dev, "Memory is not enough.\n");
		ret = -ENOMEM;
		goto err_wpc_nomem;
	}
	charger->dev = &client->dev;

	ret = i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK);
	if (!ret) {
		ret = i2c_get_functionality(client->adapter);
		dev_err(charger->dev, "I2C functionality is not supported.\n");
		ret = -ENOSYS;
		goto err_i2cfunc_not_support;
	}
	charger->client = client;
	charger->pdata = pdata;

	i2c_set_clientdata(client, charger);

	mutex_init(&charger->io_lock);
	mutex_init(&charger->charger_lock);

	charger->phm_vote = sec_vote_init(VN_WPC_PHM, SEC_VOTE_MAX, VOTER_MAX, false,
		sec_voter_name, p9222_set_phm_vote, charger);
	if (!charger->phm_vote) {
		dev_err(&client->dev, "%s: failed to init phm_vote\n", __func__);
		ret = -ENOMEM;
		goto err_phm_vote;
	}
	change_sec_voter_pri(charger->phm_vote, VOTER_WPC_CS100, VOTE_PRI_7);
	change_sec_voter_pri(charger->phm_vote, VOTER_WPC_ALIGN_CHK, VOTE_PRI_8);
	change_sec_voter_pri(charger->phm_vote, VOTER_WPC_ID_CHK, VOTE_PRI_9);
	change_sec_voter_pri(charger->phm_vote, VOTER_WPC_DET, VOTE_PRI_10);
	/* init phm vote */
	charger->wc_w_state = false;
	sec_vote(charger->phm_vote, VOTER_WPC_DET, true, WPC_PHM_OFF);

	charger->apm_vote = sec_vote_init(VN_WPC_APM, SEC_VOTE_MAX, VOTER_MAX, WPC_APM_NONE,
		sec_voter_name, p9222_set_apm_vote, charger);
	if (!charger->apm_vote) {
		dev_err(&client->dev, "%s: failed to init apm_vote\n", __func__);
		ret = -ENOMEM;
		goto err_apm_vote;
	}

	charger->ldo_vote = sec_vote_init(VN_WPC_LDO, SEC_VOTE_MAX, VOTER_MAX, WPC_LDO_ON,
		sec_voter_name, p9222_set_ldo_vote, charger);
	if (!charger->ldo_vote) {
		dev_err(&client->dev, "%s: failed to init ldo_vote\n", __func__);
		ret = -ENOMEM;
		goto err_ldo_vote;
	}

	charger->pdata->ic_on_mode = false;

	charger->cable_type = SB_CBL_NONE;
	charger->pdata->tx_firmware_result = P9222_FW_RESULT_DOWNLOADING;
	charger->pdata->watchdog_test = false;
	charger->pdata->charging_mode = SEC_INITIAL_MODE;

	charger->support_power_hold = 0;
	charger->charge_mode = P9222_CHARGE_MODE_NONE;
	charger->store_mode = 0;
	charger->tx_id_checked = false;
	charger->tx_id = 0x0;
	charger->force_wpc_det_chk = p9222_is_on_pad(charger);
	charger->irq_wpc_det_enabled = true;
	charger->err_status = 0;
	charger->temperature = 250;
	charger->d2d_vout_strength = 100;

	charger->wqueue = create_singlethread_workqueue("p9222_workqueue");
	if (!charger->wqueue) {
		dev_err(&client->dev, "%s: failed to create wqueue\n", __func__);
		ret = -ENOMEM;
		goto err_wqueue;
	}

	charger->wpc_ws = wakeup_source_register(charger->dev, "p9222-wpc");
	charger->wpc_tx_id_check_ws = wakeup_source_register(charger->dev, "p9222-wpc_tx_id_check");
	charger->wpc_auth_check_ws = wakeup_source_register(charger->dev, "p9222-wpc_auth_check");
	charger->power_hold_chk_ws = wakeup_source_register(charger->dev, "p9222-power_hold_chk");
	charger->wpc_id_request_ws = wakeup_source_register(charger->dev, "p9222-wpc_id_request");
	charger->align_check_ws = wakeup_source_register(charger->dev, "p9222-align_check");
	charger->phm_free_ws = wakeup_source_register(charger->dev, "p9222-phm_free");
	charger->retry_phm_free_ws = wakeup_source_register(charger->dev, "p9222-retry_phm_free");
	charger->cs100_ws = wakeup_source_register(charger->dev, "p9222-cs100");
	charger->darkzone_ws = wakeup_source_register(charger->dev, "p9222-darkzone");
	charger->ab_acok_ws = wakeup_source_register(charger->dev, "p9222-ab_acok");

	INIT_DELAYED_WORK(&charger->align_check_work, p9222_wpc_align_check_work);
	INIT_DELAYED_WORK(&charger->phm_free_work, p9222_phm_free_work);
	INIT_DELAYED_WORK(&charger->retry_phm_free_work, p9222_retry_phm_free_work);
	INIT_DELAYED_WORK(&charger->cs100_work, p9222_cs100_work);
	INIT_DELAYED_WORK(&charger->darkzone_work, p9222_darkzone_work);
	INIT_DELAYED_WORK(&charger->curr_measure_work, p9222_curr_measure_work);
	INIT_DELAYED_WORK(&charger->wpc_auth_check_work, p9222_wpc_auth_check_work);
	INIT_DELAYED_WORK(&charger->power_hold_chk_work, p9222_phm_check_work);
	INIT_DELAYED_WORK(&charger->wpc_id_request_work, p9222_wpc_id_request_work);

	INIT_DELAYED_WORK(&charger->wpc_det_work, p9222_wpc_det_work);
	INIT_DELAYED_WORK(&charger->wpc_isr_work, p9222_wpc_isr_work);
	INIT_DELAYED_WORK(&charger->init_work, p9222_init_work);

	/* wpc_det */
	if (charger->pdata->irq_wpc_det >= 0) {
		ret = request_threaded_irq(charger->pdata->irq_wpc_det,
				p9222_irq_handler, p9222_wpc_det_irq_thread,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
				IRQF_ONESHOT,
				"wpc-det", charger);
		if (ret) {
			dev_err(&client->dev,
				"%s: Failed to Reqeust IRQ\n", __func__);
			goto err_supply_unreg;
		}

		ret = enable_irq_wake(charger->pdata->irq_wpc_det);
		if (ret < 0)
			dev_err(&client->dev, "%s: Failed to Enable DET Wakeup Source(%d)\n",
				__func__, ret);

		p9222_wpc_det_irq_enable(charger, false);
	}

	/* wpc_irq */
	if (charger->pdata->irq_wpc_int >= 0) {
		ret = request_threaded_irq(charger->pdata->irq_wpc_int,
				p9222_irq_handler, p9222_wpc_irq_thread,
				IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT,
				"wpc-irq", charger);
		if (ret) {
			dev_err(&client->dev,
				"%s: Failed to Reqeust IRQ\n", __func__);
			goto err_supply_unreg;
		}

		ret = enable_irq_wake(charger->pdata->irq_wpc_int);
		if (ret < 0)
			dev_err(&client->dev, "%s: Failed to Enable INT Wakeup Source(%d)\n",
				__func__, ret);
	}

	wpc_cfg.drv_data = charger;
	if (!wpc_cfg.drv_data) {
		dev_err(&client->dev,
			"%s: Failed to Register psy_chg\n", __func__);
		goto err_supply_unreg;
	}

	charger->psy_chg = power_supply_register(&client->dev,
		&wpc_power_supply_desc, &wpc_cfg);
	if (IS_ERR(charger->psy_chg)) {
		goto err_supply_unreg;
	}

	ret = sec_wpc_create_attrs(&charger->psy_chg->dev);
	if (ret) {
		dev_err(&client->dev,
			"%s: Failed to Register psy_chg\n", __func__);
		goto err_pdata_free;
	}

	/* moved init process to init work because of msleep in p9222_set_cmd_reg */
	queue_delayed_work(charger->wqueue, &charger->init_work, 0);

	dev_info(&client->dev,
		"%s: p9222 Charger Driver Loaded\n", __func__);

	//device_init_wakeup(charger->dev, 1);
	return 0;
err_pdata_free:
	power_supply_unregister(charger->psy_chg);
err_supply_unreg:
	wakeup_source_unregister(charger->wpc_ws);
	wakeup_source_unregister(charger->wpc_tx_id_check_ws);
	wakeup_source_unregister(charger->wpc_auth_check_ws);
err_wqueue:
	sec_vote_destroy(charger->ldo_vote);
err_ldo_vote:
	sec_vote_destroy(charger->apm_vote);
err_apm_vote:
	sec_vote_destroy(charger->phm_vote);
err_phm_vote:
	mutex_destroy(&charger->io_lock);
	mutex_destroy(&charger->charger_lock);
err_i2cfunc_not_support:
	kfree(charger);
err_wpc_nomem:
	devm_kfree(&client->dev, pdata);
	return ret;
}

#if defined CONFIG_PM
static int p9222_charger_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct p9222_charger_data *charger = i2c_get_clientdata(i2c);

	pr_info("%s: mode[%d]\n", __func__, charger->pdata->charging_mode);

	if (device_may_wakeup(charger->dev)){
		enable_irq_wake(charger->pdata->irq_wpc_int);
		enable_irq_wake(charger->pdata->irq_wpc_det);
	}

	disable_irq(charger->pdata->irq_wpc_int);
	return 0;
}

static int p9222_charger_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct p9222_charger_data *charger = i2c_get_clientdata(i2c);

	pr_info("%s: mode[%d] charger->charge_mode %d charger->pdata->otp_firmware_ver = %x\n",
		__func__, charger->pdata->charging_mode, charger->charge_mode,
		charger->pdata->otp_firmware_ver);

	if (device_may_wakeup(charger->dev)) {
		disable_irq_wake(charger->pdata->irq_wpc_int);
		disable_irq_wake(charger->pdata->irq_wpc_det);
	}

	enable_irq(charger->pdata->irq_wpc_int);
	return 0;
}
#else
#define p9222_charger_suspend NULL
#define p9222_charger_resume NULL
#endif

static void p9222_charger_shutdown(struct i2c_client *client)
{
	struct p9222_charger_data *charger = i2c_get_clientdata(client);

	pr_debug("%s\n", __func__);

	if ((!p9222_is_on_pad(charger)) ||
		!gpio_is_valid(charger->pdata->wpc_en))
		return;

	p9222_wpc_det_irq_enable(charger, false);
	disable_irq(charger->pdata->irq_wpc_int);

	/* needs more delay to reset IC at PHM.(170ms : ping delay) */
	gpio_direction_output(charger->pdata->wpc_en, 1);
	msleep(PHM_DETACH_DELAY);
}

static const struct i2c_device_id p9222_charger_id[] = {
	{"p9222-charger", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, p9222_charger_id);

static struct of_device_id p9222_i2c_match_table[] = {
	{ .compatible = "p9222,i2c"},
	{},
};

const struct dev_pm_ops p9222_charger_pm = {
	.suspend = p9222_charger_suspend,
	.resume = p9222_charger_resume,
};

static struct i2c_driver p9222_charger_driver = {
	.driver = {
		.name	= "p9222-charger",
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &p9222_charger_pm,
#endif /* CONFIG_PM */
		.of_match_table = p9222_i2c_match_table,
	},
	.shutdown	= p9222_charger_shutdown,
	.probe	= p9222_charger_probe,
	//.remove	= p9222_charger_remove,
	.id_table	= p9222_charger_id,
};

static int __init p9222_charger_init(void)
{
	pr_debug("%s\n", __func__);
	return i2c_add_driver(&p9222_charger_driver);
}

static void __exit p9222_charger_exit(void)
{
	pr_debug("%s\n", __func__);
	i2c_del_driver(&p9222_charger_driver);
}

module_init(p9222_charger_init);
module_exit(p9222_charger_exit);

MODULE_DESCRIPTION("Samsung p9222 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
