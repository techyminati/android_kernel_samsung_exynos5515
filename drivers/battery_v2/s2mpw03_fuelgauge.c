/*
 * s2mpw03_fuelgauge.c - S2MPW03 Fuel Gauge Driver
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/of_gpio.h>
#include <linux/mfd/samsung/s2mpw03.h>
#include <linux/mfd/samsung/rtc-s2mp.h>
#include <linux/mfd/samsung/s2mpw03-regulator.h>
#include "include/fuelgauge/s2mpw03_fuelgauge.h"
#include "include/sec_vote_event.h"

#define MAX_RAW_SOC	10000

static enum power_supply_property s2mpw03_fuelgauge_props[] = {
};

static int s2mpw03_fg_rewrite_bat_param(struct s2mpw03_fuelgauge_data *fuelgauge, int step);
static int s2mpw03_get_ocv(struct s2mpw03_fuelgauge_data *fuelgauge);
static int s2mpw03_get_vbat(struct s2mpw03_fuelgauge_data *fuelgauge);

int s2mpw03_fg_read_reg(struct s2mpw03_fuelgauge_data *fuelgauge, u8 reg, u8 *dest)
{
	int ret;
	s2mpw03_ext_bus_set(true);
	ret = s2mpw03_read_reg(fuelgauge->i2c, reg, dest);
	if (ret < 0)
		fuelgauge->err_cnt++;

	s2mpw03_ext_bus_set(false);
	return ret;
}

int s2mpw03_fg_bulk_read(struct s2mpw03_fuelgauge_data *fuelgauge, u8 reg, int count, u8 *buf)
{
	int ret;
	
	s2mpw03_ext_bus_set(true);
	ret = s2mpw03_bulk_read(fuelgauge->i2c, reg, count, buf);
	if (ret < 0)
		fuelgauge->err_cnt++;

	s2mpw03_ext_bus_set(false);
	return ret;
}

int s2mpw03_fg_write_reg(struct s2mpw03_fuelgauge_data *fuelgauge, u8 reg, u8 value)
{
	int ret;
	
	s2mpw03_ext_bus_set(true);
	ret = s2mpw03_write_reg(fuelgauge->i2c, reg, value);
	if (ret < 0)
		fuelgauge->err_cnt++;

	s2mpw03_ext_bus_set(false);
	return ret;
}

int s2mpw03_fg_write_and_verify_reg(struct s2mpw03_fuelgauge_data *fuelgauge, u8 reg, u8 value)
{
	int ret, i = 0;
	int i2c_corrupted_cnt = 0;
	u8 temp = 0;

	s2mpw03_ext_bus_set(true);
	ret = s2mpw03_write_reg(fuelgauge->i2c, reg, value);
	if (ret < 0) {
		for (i = 0; i < 3; i++) {
			ret = s2mpw03_write_reg(fuelgauge->i2c, reg, value);
			if (ret >= 0)
				break;
		}

		if (i >= 3)
			pr_err("%s: Error(%d)\n", __func__, ret);
	}

	s2mpw03_read_reg(fuelgauge->i2c, reg, &temp);
	while ((temp != value) && (i2c_corrupted_cnt < 5)) {
		pr_err("%s: I2C write Incorrect! REG: 0x%x Expected: 0x%x Real-Value: 0x%x\n",
				__func__, reg, value, temp);
		ret = s2mpw03_write_reg(fuelgauge->i2c, reg, value);
		s2mpw03_read_reg(fuelgauge->i2c, reg, &temp);
		i2c_corrupted_cnt++;
	}

	if (i2c_corrupted_cnt == 5)
		pr_err("%s: I2C write failed REG: 0x%x Expected: 0x%x\n",
				__func__, reg, value);

	s2mpw03_ext_bus_set(false);
	return ret;
}


int s2mpw03_fg_bulk_write(struct s2mpw03_fuelgauge_data *fuelgauge, u8 reg, int count, u8 *buf)
{
	int ret;
	
	s2mpw03_ext_bus_set(true);
	ret = s2mpw03_bulk_write(fuelgauge->i2c, reg, count, buf);
	if (ret < 0)
		fuelgauge->err_cnt++;
	s2mpw03_ext_bus_set(false);

	return ret;
}

int s2mpw03_fg_update_reg(struct s2mpw03_fuelgauge_data *fuelgauge, u8 reg, u8 val, u8 mask)
{
	int ret;
	
	s2mpw03_ext_bus_set(true);
	ret = s2mpw03_update_reg(fuelgauge->i2c, reg, val, mask);
	if (ret < 0)
		fuelgauge->err_cnt++;
	s2mpw03_ext_bus_set(false);

	return ret;
}

static void s2mpw03_fg_test_read(struct s2mpw03_fuelgauge_data *fuelgauge, bool read_param)
{
	u8 data;
	char str[2000] = {0,};
	char l_str[256] = {0,};
	int l_size = 256;
	int i;	

	for (i = S2MPW03_FG_REG_STATUS; i <= S2MPW03_FG_REG_CONFIG2; i++) {
		s2mpw03_fg_read_reg(fuelgauge, i, &data);
		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
		if ((i == S2MPW03_FG_REG_RVBAT) || (i == S2MPW03_FG_REG_RVBAT + 1) ||
			((i >= S2MPW03_FG_REG_RTEMP2) && (i <= S2MPW03_FG_REG_RZADJ2)) ||
			(i == S2MPW03_FG_REG_IRQ_LVL2)) {
			snprintf(l_str+strlen(l_str), l_size, "0x%02x,", data);
			l_size = sizeof(l_str) - strlen(l_str);
		}
	}

	s2mpw03_read_reg(fuelgauge->common, 0x15, &data);
	sprintf(str+strlen(str), "clock:0x%02x, ", data);

	s2mpw03_read_reg(fuelgauge->pmic, S2MPW03_PMIC_REG_LDO_CTRL2, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", S2MPW03_PMIC_REG_LDO_CTRL2, data);

	if (read_param) {
		sprintf(str+strlen(str), "PARAM1: ");
		for (i = 0; i < S2MPW03_FG_PARAM1_NUM; i++) {
			s2mpw03_fg_read_reg(fuelgauge, i + S2MPW03_FG_REG_PARAM1, &data);
			sprintf(str+strlen(str), "0x%02x ", data);
		}

		sprintf(str+strlen(str), ", PARAM2: ");
		for (i = 0; i < S2MPW03_FG_PARAM2_NUM; i++) {
			s2mpw03_fg_read_reg(fuelgauge, i + S2MPW03_FG_REG_PARAM2, &data);
			sprintf(str+strlen(str), "0x%02x ", data);
		}
	}

	snprintf(fuelgauge->d_buf, 128, "%s%d,%d,%d,%d,", l_str,
		(fuelgauge->err_cnt % 10000), (fuelgauge->wa_cnt % 10000),
		fuelgauge->rawsoc_old, fuelgauge->rawocv);

	pr_err("[FG]:%s\n", str);
}

static unsigned int s2mpw03_get_ocv_from_soc(struct s2mpw03_fuelgauge_data *fuelgauge, unsigned int soc)
{
	unsigned int soc_lut[22], ocv_lut[22];
	int i = 0;
	u16 ocv_temp = 0, soc_temp = 0;
	unsigned int ocv = 0;
	struct fg_age_data_info *bat_data;
	int step = 0;

	if (fuelgauge->fg_age_step >= 0)
		step = fuelgauge->fg_age_step;

	bat_data = &FG_DATA(step);

	for (i = 0; i < 22; i++) {
		ocv_temp = (bat_data->model_param1[i*2] & 0xFF) | ((bat_data->model_param1[i*2+1] & 0x0F) << 8);
		soc_temp = (bat_data->model_param1[i*2+44] & 0xFF) | ((bat_data->model_param1[i*2+45] & 0x0F) << 8);

		ocv_lut[i] = ((ocv_temp * 1000) >> 11) + 3000;
		soc_lut[i] = ((soc_temp * 10000) >> 11);
	}

	/* SOC - OCV table log
	for (i = 0; i < 22; i++)
		pr_info("%s LUT[%d] : SOC(%d) -> OCV(%d)\n", __func__, i, soc_lut[i], ocv_lut[i]);
	*/

	for (i = 0; i < 21; i++) {
		if (soc > soc_lut[i+1])
			break;
	}

	if (i >= 21) {
		ocv = s2mpw03_get_ocv(fuelgauge);
	} else {
		ocv = (ocv_lut[i] - ocv_lut[i+1]) * (soc - soc_lut[i+1]);
		ocv = ocv / (soc_lut[i] - soc_lut[i+1]);
		ocv += ocv_lut[i+1];
	}

	if (ocv < 3200 || ocv > 4500)
		ocv = s2mpw03_get_ocv(fuelgauge);

	pr_info("%s, soc(%d), lut(%d), ocv(%d)\n", __func__, soc, i, ocv);
	return ocv;

}

static u16 s2mpw03_get_dumpdone_vbat(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	unsigned int soc = fuelgauge->capacity_old * 100;
	unsigned int ocv = 0;
	u16 ret = 0;
	u32 temp = 0;

	ocv = s2mpw03_get_ocv_from_soc(fuelgauge, soc);
	temp = ((ocv - 3000) << 16) / 6000;
	ret = temp & 0xFFFF;

	pr_info("%s, soc(%d), ret(0x%04x)\n", __func__, soc, ret);
	return ret;
}

static void s2mpw03_update_trim_value(struct s2mpw03_fuelgauge_data *fuelgauge, bool dump)
{
	u8 data[6];
	u8 temp;

	if (dump) {
		/* before dumpdone, trim values set default */
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x26, 0x00);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x27, 0x00);
		s2mpw03_fg_read_reg(fuelgauge, 0x28, &temp);
		temp &= 0xF0;
		temp |= 0x03;
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x28, temp);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x29, 0x00);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x2A, 0x00);
		s2mpw03_fg_read_reg(fuelgauge, 0x2B, &temp);
		temp &= 0xF0;
		temp |= 0x03;
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x2B, temp);
	} else {
		/* after dumpdone */
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x26, fuelgauge->reg_0x26);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x27, fuelgauge->reg_0x27);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x28, fuelgauge->reg_0x28);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x29, fuelgauge->reg_0x29);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x2A, fuelgauge->reg_0x2A);
		s2mpw03_fg_write_and_verify_reg(fuelgauge, 0x2B, fuelgauge->reg_0x2B);
	}
	s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_OTP_01_L, 6, data);
	pr_info("%s, read trim data(%02x, %02x, %02x, %02x, %02x, %02x)\n",
			__func__, data[0], data[1], data[2], data[3], data[4], data[5]);

}

static void s2mpw03_save_trim_value(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	int i;
	u8 data[6], check_data[6];

	for (i = 0; i < 5; i++) {
		s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_OTP_01_L, 6, data);
		s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_OTP_01_L, 6, check_data);
		if ((data[0] == check_data[0]) && (data[1] == check_data[1]) && (data[2] == check_data[2]) &&
				(data[3] == check_data[3]) && (data[4] == check_data[4]) && (data[5] == check_data[5]))
			break;
	}

	fuelgauge->reg_0x26 = data[0];
	fuelgauge->reg_0x27 = data[1];
	fuelgauge->reg_0x28 = data[2];
	fuelgauge->reg_0x29 = data[3];
	fuelgauge->reg_0x2A = data[4];
	fuelgauge->reg_0x2B = data[5];

	pr_info("%s, save trim data(%02x, %02x, %02x, %02x, %02x, %02x)\n",
			__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
}


static void s2mpw03_restart_gauging(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 temp;

	mutex_lock(&fuelgauge->rst_lock);

	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG2,
			(CHG_I2C_EN_MASK | CHG_I2C_MASK), (CHG_I2C_EN_MASK | CHG_I2C_MASK));

	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG,
			(DUMP_DONE_MASK | RESTART_MASK), (DUMP_DONE_MASK | RESTART_MASK));
	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_CONFIG, &temp);

	dev_info(&fuelgauge->i2c->dev, "%s: dump done 0x%x\n", __func__, temp);

	msleep(500);
	mutex_unlock(&fuelgauge->rst_lock);
}

static void s2mpw03_runtime_dumpdone(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u16 dump_vbat = 0;
	u8 temp;

	/* Logging W/A start, 0x019[0] = 1 */
	s2mpw03_write_reg(fuelgauge->common, S2MPW03_FG_WA_ADDR, 0x01);

	dump_vbat = s2mpw03_get_dumpdone_vbat(fuelgauge);

	/* charger off */
	sec_votef(VN_CHG_EN, VOTER_S2MPW03_CONTROL, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

	s2mpw03_update_trim_value(fuelgauge, true);

	s2mpw03_fg_write_and_verify_reg(fuelgauge, S2MPW03_FG_REG_VBATI2C, dump_vbat & 0xFF);
	s2mpw03_fg_write_and_verify_reg(fuelgauge, S2MPW03_FG_REG_VBATI2C + 1, ((dump_vbat & 0xFF00) >> 8));
	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_VTS_SHIFT, &temp);
	temp &= 0xFE;
	temp |= 0x01;
	s2mpw03_fg_write_and_verify_reg(fuelgauge, S2MPW03_FG_REG_VTS_SHIFT, temp);

	s2mpw03_fg_rewrite_bat_param(fuelgauge, fuelgauge->fg_age_step);
	s2mpw03_restart_gauging(fuelgauge);

	s2mpw03_update_trim_value(fuelgauge, false);
	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_VTS_SHIFT, &temp);
	temp &= 0xFE;
	s2mpw03_fg_write_and_verify_reg(fuelgauge, S2MPW03_FG_REG_VTS_SHIFT, temp);

	/* charger on */
	sec_votef(VN_CHG_EN, VOTER_S2MPW03_CONTROL, false, false);

	/* Logging W/A finish, 0x019[0] = 0 */
	s2mpw03_write_reg(fuelgauge->common, S2MPW03_FG_WA_ADDR, 0x00);

	pr_info("%s, dump_vbat(0x%04x), capacity_old(%d)\n",
			__func__, dump_vbat, fuelgauge->capacity_old);
}

static void s2mpw03_alert_init(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data;
	u8 temp = 0;

	/* VBAT Threshold setting: 2.8V */
	data = 0x00;

	/* SOC Threshold setting */
	if (fuelgauge->pdata->fuel_alert_soc <= 10)
		temp = fuelgauge->pdata->fuel_alert_soc;
	else if (fuelgauge->pdata->fuel_alert_soc > 35)
		temp = 0xF;
	else
		temp = ((fuelgauge->pdata->fuel_alert_soc - 10) / 5) + 10;
	data = data | (temp << SOC_L_LVL_SHIFT);

	s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_IRQ_LVL, data);

	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_IRQ_LVL, &data);
	dev_info(&fuelgauge->i2c->dev, "%s: FG_REG_IRQ_LVL = 0x%02x\n",
		__func__, data);
}

static bool s2mpw03_check_status(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	bool ret = false;

	/* check if Smn was generated */
	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_STATUS, 2, data) < 0)
		return ret;

	dev_dbg(&fuelgauge->i2c->dev, "%s: status to (%02x%02x)\n",
		__func__, data[1], data[0]);

	if (data[1] & SOC_L_MASK)
		return true;
	else
		return false;
}

static int s2mpw03_set_temperature(struct s2mpw03_fuelgauge_data *fuelgauge,
			int temperature)
{
	dev_info(&fuelgauge->i2c->dev, "%s: do not control temperature\n", __func__);
	return temperature;
}

static int s2mpw03_get_temperature(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	s32 temperature = 0;

	/*
	 * use monitor regiser.
	 * monitor register default setting is temperature
	 */
	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RTEMP, 2, data) < 0)
		return -ERANGE;

	/* data[] store 2's compliment format number */
	if (data[0] & (0x1 << 7)) {
		/* Negative */
		temperature = ((~(data[0])) & 0xFF) + 1;
		temperature *= -10;
	} else {
		temperature = data[0] & 0x7F;
		temperature *= 10;
	}

	dev_dbg(&fuelgauge->i2c->dev, "%s: temperature (%d)\n",
		__func__, temperature);

	return temperature;
}

static int s2mpw03_get_ocv(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u32 rocv = 0;
	u16 compliment;

	mutex_lock(&fuelgauge->fg_lock);

	/* OCV */
	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_ROCV, 2, data) < 0) {
		mutex_unlock(&fuelgauge->fg_lock);
		return -EINVAL;
	}

	mutex_unlock(&fuelgauge->fg_lock);

	compliment = (data[1] << 8) | (data[0]);
	rocv = ((data[0] + (data[1] << 8)) * 1000) >> 13;

	dev_info(&fuelgauge->i2c->dev, "%s: rocv %d\n", __func__, rocv);

	return rocv;
}

static int s2mpw03_get_soc_after_runtime_dumpdone(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2], check_data[2];
	u16 compliment;
	int rsoc, i = 0;

	mutex_lock(&fuelgauge->fg_lock);

	for (i = 0; i < 5; i++) {
		if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RSOC, 2, data) < 0)
			goto err;
		if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RSOC, 2, check_data) < 0)
			goto err;
		if ((data[0] == check_data[0]) && (data[1] == check_data[1]))
			break;
		dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d) check_data0 (%d) check_data1 (%d)\n",
				__func__, data[0], data[1], check_data[0], check_data[1]);
	}

	mutex_unlock(&fuelgauge->fg_lock);

	dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d)\n", __func__, data[0], data[1]);
	compliment = (data[1] << 8) | (data[0]);

	/* data[] store 2's compliment format number */
	if (compliment & (0x1 << 15)) {
		/* Negative */
		rsoc = ((~compliment) & 0xFFFF) + 1;
		rsoc = (rsoc * (-10000)) / (0x1 << 12);
	} else {
		rsoc = compliment & 0x7FFF;
		rsoc = ((rsoc * 10000) / (0x1 << 12));
	}
	return rsoc;

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

#if defined(CONFIG_SEC_FACTORY)
static bool s2mpw03_check_monout_wa(struct s2mpw03_fuelgauge_data *fuelgauge, int ocv)
{
	return false;
}

static bool s2mpw03_check_soc_wa(struct s2mpw03_fuelgauge_data *fuelgauge, int rsoc)
{
	return false;
}
#else
static int s2mpw03_get_monoutsel(struct s2mpw03_fuelgauge_data *fuelgauge, int mon)
{
	int ret = 0;
	u8 data[2];
	u16 compliment, status;
	struct fg_age_data_info *bat_data;
	int step = 0;

	if (fuelgauge->fg_age_step >= 0)
		step = fuelgauge->fg_age_step;

	bat_data = &FG_DATA(step);

	mutex_lock(&fuelgauge->fg_lock);
	mon &= MONOUT_SEL_MASK;
	mon |= (bat_data->model_table2[1] & 0xF0);
	s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_MONOUT_CFG, mon);

	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_CURR, 2, data) < 0) {
		mutex_unlock(&fuelgauge->fg_lock);
		return -EINVAL;
	}
	mutex_unlock(&fuelgauge->fg_lock);

	compliment = (data[1] << 8) | (data[0]);

	status = compliment & (0x1 << 15);
	if (status) {
		ret = ((~compliment) & 0xFFFF) + 1;
		ret = (ret * 1000) >> 12;
		ret = ret * -1;
	} else {
		ret = compliment & 0x7FFF;
		ret = (ret * 1000) >> 12;
	}

	return ret;
}

static bool s2mpw03_check_monout_wa(struct s2mpw03_fuelgauge_data *fuelgauge, int ocv)
{
	int rs = 0, vdiff = 0, vts = 0, vtl = 0;

	if (!fuelgauge->pdata->flag.monout_wa)
		return false;

	if (fuelgauge->factory_mode)
		return false;

	vts = s2mpw03_get_monoutsel(fuelgauge, MONOUT_SEL_VTS);
	vtl = s2mpw03_get_monoutsel(fuelgauge, MONOUT_SEL_VTL);
	vdiff = s2mpw03_get_monoutsel(fuelgauge, MONOUT_SEL_VDIFF);
	rs = s2mpw03_get_monoutsel(fuelgauge, MONOUT_SEL_RS);
	if (rs == -EINVAL)
		rs = MONOUT_RS_VALID;
	dev_info(&fuelgauge->i2c->dev, "%s: ocv %d, rs %d, vdiff %d, vts %d, vtl %d\n",
		__func__, ocv, rs, vdiff, vts, vtl);

	/* Normal Range : 3.0V < ocv < 4.5V, rs > 0.3, -0.3 < vts & vtl < 0.3, -1.5 < vdiff < 1.5 */
	return (ocv < 3000 || ocv > 4500) ||
		(rs < 50) ||
		(vts > 300 || vts < -300) ||
		(vtl > 300 || vtl < -300) ||
		(vdiff < -1500 || vdiff > 1500);
}

static bool s2mpw03_check_soc_wa(struct s2mpw03_fuelgauge_data *fuelgauge, int rsoc)
{
	unsigned int rsoc_old;
	int vbat_old, vbat_now;
	unsigned int ocv_now, ocv_old;

	if (!fuelgauge->pdata->flag.soc_wa)
		return false;

	rsoc_old = fuelgauge->rawsoc_old;
	ocv_old = s2mpw03_get_ocv_from_soc(fuelgauge, rsoc_old);
	ocv_now = s2mpw03_get_ocv_from_soc(fuelgauge, rsoc);
	vbat_old = fuelgauge->vbat_old;
	vbat_now = s2mpw03_get_vbat(fuelgauge);
	dev_info(&fuelgauge->i2c->dev, "[DEBUG]%s: rsoc_old(%d), rsoc(%d), ocv_old(%d), ocv_now(%d), vbat_old(%d), vbat_now(%d)\n",
		__func__, rsoc_old, rsoc, ocv_old, ocv_now, vbat_old, vbat_now);

	return (rsoc_old != 0) && (vbat_old != 0) &&
		(abs(rsoc_old - rsoc) > 500) &&
		((abs(ocv_old - ocv_now) * 7) > (abs(vbat_old - vbat_now) * 10));
}
#endif

static bool s2mpw03_check_stuck_wa(struct s2mpw03_fuelgauge_data *fuelgauge, u16 vbat)
{
	if (!fuelgauge->pdata->flag.stuck_wa)
		return false;

	if (fuelgauge->factory_mode)
		return false;

	if (fuelgauge->voltage_old == vbat) {
		fuelgauge->v_check_cnt++;
		pr_info("%s v_check_cnt(%d) ", __func__, fuelgauge->v_check_cnt);

		if (fuelgauge->v_check_cnt >= fuelgauge->pdata->v_check) {
			fuelgauge->v_check_cnt = 0;
			return true;
		}
	} else {
		fuelgauge->v_check_cnt = 0;
	}

	return false;
}

/* soc should be 0.01% unit */
static int s2mpw03_get_soc(struct s2mpw03_fuelgauge_data *fuelgauge, union power_supply_propval *val)
{
	u8 data[2], check_data[2];
	u16 compliment;
	int rsoc, i;
	int ocv = 0;

	ocv = s2mpw03_get_ocv(fuelgauge);
	if (s2mpw03_check_monout_wa(fuelgauge, ocv)) {
		pr_info("%s abnormal current detected!\n", __func__);
		s2mpw03_runtime_dumpdone(fuelgauge);
		fuelgauge->wa_cnt += ABN_MONOUT_WA;
	}

	mutex_lock(&fuelgauge->fg_lock);

	for (i = 0; i < 5; i++) {
		if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RSOC, 2, data) < 0)
			goto err;
		if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RSOC, 2, check_data) < 0)
			goto err;
		if ((data[0] == check_data[0]) && (data[1] == check_data[1]))
			break;
		dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d) check_data0 (%d) check_data1 (%d)\n",
				__func__, data[0], data[1], check_data[0], check_data[1]);
	}

	mutex_unlock(&fuelgauge->fg_lock);

	dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d)\n", __func__, data[0], data[1]);
	compliment = (data[1] << 8) | (data[0]);

	/* data[] store 2's compliment format number */
	if (compliment & (0x1 << 15)) {
		/* Negative */
		rsoc = ((~compliment) & 0xFFFF) + 1;
		rsoc = (rsoc * (-10000)) / (0x1 << 12);
	} else {
		rsoc = compliment & 0x7FFF;
		rsoc = ((rsoc * 10000) / (0x1 << 12));
	}

	if (s2mpw03_check_soc_wa(fuelgauge, rsoc)) {
		dev_info(&fuelgauge->i2c->dev, "%s: abnormal vbat detected!\n", __func__);
		s2mpw03_runtime_dumpdone(fuelgauge);
		rsoc = s2mpw03_get_soc_after_runtime_dumpdone(fuelgauge);
		fuelgauge->wa_cnt += ABN_SOC_WA;
	}

	dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: raw capacity (0x%x:%d)\n", __func__,
		compliment, rsoc);

	fuelgauge->rawsoc_old = rsoc;
	fuelgauge->rawocv = ocv;

	val->intval = min(rsoc, MAX_RAW_SOC) / 10;
	return 0;

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

static int s2mpw03_get_rawsoc(struct s2mpw03_fuelgauge_data *fuelgauge, union power_supply_propval *val)
{
	u8 data[2], check_data[2];
	u16 compliment;
	int rsoc, i;

	mutex_lock(&fuelgauge->fg_lock);

	for (i = 0; i < 5; i++) {
		if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RSOC, 2, data) < 0)
			goto err;
		if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RSOC, 2, check_data) < 0)
			goto err;
		if ((data[0] == check_data[0]) && (data[1] == check_data[1]))
			break;
		dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d) check_data0 (%d) check_data1 (%d)\n",
			__func__, data[0], data[1], check_data[0], check_data[1]);
	}

	mutex_unlock(&fuelgauge->fg_lock);

	dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d)\n", __func__, data[0], data[1]);
	compliment = (data[1] << 8) | (data[0]);

	/* data[] store 2's compliment format number */
	if (compliment & (0x1 << 15)) {
		/* Negative */
		rsoc = ((~compliment) & 0xFFFF) + 1;
		rsoc = (rsoc * (-10000)) / (0x1 << 12);
	} else {
		rsoc = compliment & 0x7FFF;
		rsoc = ((rsoc * 10000) / (0x1 << 12));
	}

	val->intval = min(rsoc, MAX_RAW_SOC);
	return 0;

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

static int s2mpw03_get_current(struct s2mpw03_fuelgauge_data *fuelgauge)
{	
	u8 data[2];
	u16 compliment;
	int curr = 0;
	struct fg_age_data_info *bat_data;
	int step = 0;

	if (fuelgauge->fg_age_step >= 0)
		step = fuelgauge->fg_age_step;

	bat_data = &FG_DATA(step);

	mutex_lock(&fuelgauge->fg_lock);

	s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_MONOUT_CFG, 
		(MONOUT_SEL_RCUR | (bat_data->model_table2[1] & 0xF0)));

	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_CURR, 2, data) < 0) {
		mutex_unlock(&fuelgauge->fg_lock);
		return -EINVAL;
	}

	mutex_unlock(&fuelgauge->fg_lock);

	compliment = (data[1] << 8) | (data[0]);
	dev_dbg(&fuelgauge->i2c->dev, "%s: rCUR_CC(0x%4x)\n", __func__, compliment);

	if (compliment & (0x1 << 15)) { /* Charging */
		curr = ((~compliment) & 0xFFFF) + 1;
		curr = (curr * 1000) >> 12;
	} else { /* dischaging */
		curr = compliment & 0x7FFF;
		curr = (curr * (-1000)) >> 12;
	}

	dev_info(&fuelgauge->i2c->dev, "%s: current (%d)mA (0x%4x)\n", __func__, curr, compliment);
	return curr;
}

static int s2mpw03_get_avg_val(struct avg_data *fg, int val)
{
	unsigned int index = 0;

	index = fg->avg_cnt;
	index %= AVG_VAL_CNT;
	fg->avg_sum -= fg->avg_val[index];
	fg->avg_val[index] = val;
	fg->avg_sum += val;
	fg->avg_cnt++;

	return (fg->avg_cnt < AVG_VAL_CNT) ? 0 : (fg->avg_sum / AVG_VAL_CNT);
}

#ifdef CONFIG_HW_SUSPEND_ENERGY_ESTIMATOR
static int s2mpw03_get_current_suspend(struct s2mpw03_fuelgauge_data *fuelgauge, int type)
{
	int val = -1;

	if (type == POWER_SUPPLY_CURRENT_SUSPEND_CHARGING ||
		type == POWER_SUPPLY_CURRENT_SUSPEND_DISCHARGING) {
		val = fuelgauge->suspend_current;
		fuelgauge->suspend_current = 0;
	}

	return val;
}

static int s2mpw03_calc_current_suspend(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment, status;
	int curr = 0;
	struct fg_age_data_info *bat_data;
	int step = 0;

	if (fuelgauge->fg_age_step >= 0)
		step = fuelgauge->fg_age_step;

	bat_data = &FG_DATA(step);

	mutex_lock(&fuelgauge->fg_lock);

	s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_MONOUT_CFG, 
		(MONOUT_SEL_RCUR | (bat_data->model_table2[1] & 0xF0)));

	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_CURR, 2, data) < 0) {
		mutex_unlock(&fuelgauge->fg_lock);
		return -EINVAL;
	}

	mutex_unlock(&fuelgauge->fg_lock);

	compliment = (data[1] << 8) | (data[0]);

	status = compliment & (0x1 << 15);
	if (status) { /* Charging */
		curr = ((~compliment) & 0xFFFF) + 1;
		curr = (curr * 1000) >> 12;
	} else { /* dischaging */
		curr = compliment & 0x7FFF;
		curr = (curr * 1000) >> 12;
		curr = curr * -1;
	}
	fuelgauge->suspend_current = curr;

	pr_info("%s: status=%d: current=%d: compliment=0x%x\n",
		__func__, status, curr, compliment);

	return 0;
}
#endif

static int s2mpw03_get_vbat(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2], val;
	u32 vbat = 0;
	u16 compliment;
	struct timespec64 ts = {0, };

	ts = ktime_to_timespec64(ktime_get_boottime());

	if (!fuelgauge->factory_mode &&
		(fuelgauge->updated_time > 0) &&
		(ts.tv_sec < fuelgauge->updated_time + 4)) {
		pr_info("%s not enough time to update voltage\n", __func__);
		return fuelgauge->vbat_old;
	}

	fuelgauge->updated_time = ts.tv_sec;

	mutex_lock(&fuelgauge->fg_lock);

	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RVBAT, 2, data) < 0) {
		mutex_unlock(&fuelgauge->fg_lock);
		return -EINVAL;
	}

	mutex_unlock(&fuelgauge->fg_lock);

	dev_dbg(&fuelgauge->i2c->dev, "%s: data0 (%d) data1 (%d)\n", __func__, data[0], data[1]);
	vbat = ((data[0] + (data[1] << 8)) * 1000) >> 13;
	compliment = (data[1] << 8) | (data[0]);

	dev_info(&fuelgauge->i2c->dev, "%s: vbat (%d, 0x%4x)\n", __func__, vbat, compliment);

	if (s2mpw03_check_stuck_wa(fuelgauge, compliment)) {
		pr_info("%s : voltage stuck detected!\n", __func__);
		/* FG Clock Switching W/A */
		/* 0x17[6] = 0 */
		s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG2, 0x00, 0x40);
		s2mpw03_fg_write_reg(fuelgauge, 0x25, 0x78);
		s2mpw03_write_reg(fuelgauge->common, 0x0D, 0x40);
		usleep_range(2000, 3000);
		s2mpw03_fg_write_reg(fuelgauge, 0x25, 0x70);
		s2mpw03_write_reg(fuelgauge->common, 0x0D, 0x00);

		s2mpw03_runtime_dumpdone(fuelgauge);
		fuelgauge->wa_cnt += ABN_STUCK_WA;
	}

	fuelgauge->voltage_old = compliment;
	fuelgauge->vbat_old = vbat;

	if (vbat >= 4400) {
		s2mpw03_get_ocv(fuelgauge);
		dev_info(&fuelgauge->i2c->dev, "%s: LDO_CTRL4(0x4E): (0x%x)\n", __func__, val);
	}

	return vbat;
}

/* capacity is 0.1% unit */
static void s2mpw03_fg_get_scaled_capacity(
		struct s2mpw03_fuelgauge_data *fuelgauge,
		union power_supply_propval *val)
{
	int rawsoc = val->intval;

	val->intval = (val->intval < fuelgauge->pdata->capacity_min) ?
		0 : ((val->intval - fuelgauge->pdata->capacity_min) * 1000 /
		(fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));

	dev_info(&fuelgauge->i2c->dev,
			"%s: capacity_max(%d), scaled capacity (%d.%d), raw_soc(%d.%d)\n",
			__func__, fuelgauge->capacity_max,
			val->intval/10, val->intval%10, rawsoc/10, rawsoc%10);
}

/* capacity is integer */
static void s2mpw03_fg_get_atomic_capacity(
		struct s2mpw03_fuelgauge_data *fuelgauge,
		union power_supply_propval *val)
{
	if (fuelgauge->pdata->capacity_calculation_type &
		SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC) {
		if (fuelgauge->capacity_old < val->intval)
			val->intval = fuelgauge->capacity_old + 1;
		else if (fuelgauge->capacity_old > val->intval)
			val->intval = fuelgauge->capacity_old - 1;
	}

	/* keep SOC stable in abnormal status */
	if (fuelgauge->pdata->capacity_calculation_type &
		SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL) {
		if (!fuelgauge->is_charging &&
			fuelgauge->capacity_old < val->intval) {
			dev_err(&fuelgauge->i2c->dev,
				"%s: capacity (old %d : new %d)\n",
				__func__, fuelgauge->capacity_old, val->intval);
			val->intval = fuelgauge->capacity_old;
		}
	}

	/* updated old capacity */
	fuelgauge->capacity_old = val->intval;
}

static int s2mpw03_fg_check_capacity_max(
		struct s2mpw03_fuelgauge_data *fuelgauge, int capacity_max)
{
	int new_capacity_max = capacity_max;

	if (new_capacity_max < (fuelgauge->pdata->capacity_max -
				fuelgauge->pdata->capacity_max_margin - 10)) {
		new_capacity_max =
			(fuelgauge->pdata->capacity_max -
			fuelgauge->pdata->capacity_max_margin);

		dev_info(&fuelgauge->i2c->dev, "%s: set capacity max(%d --> %d)\n",
				__func__, capacity_max, new_capacity_max);
	}

	return min(new_capacity_max, (MAX_RAW_SOC / 10));
}

static void s2mpw03_fg_calculate_dynamic_scale(
		struct s2mpw03_fuelgauge_data *fuelgauge, int capacity)
{
	union power_supply_propval raw_soc_val = {0, };
	int min_cap = fuelgauge->pdata->capacity_max - fuelgauge->pdata->capacity_max_margin;

	if ((capacity > 100) || ((capacity * 10) < min_cap)) {
		pr_err("%s: invalid capacity(%d)\n", __func__, capacity);
		return;
	}

	if (s2mpw03_get_rawsoc(fuelgauge, &raw_soc_val) < 0) {
		pr_err("%s: failed to read raw soc\n", __func__);
		return;
	}

	raw_soc_val.intval = (raw_soc_val.intval) / 10;
	if (raw_soc_val.intval < min_cap) {
		pr_info("%s: raw soc(%d) is very low(%d), skip routine\n",
			__func__, raw_soc_val.intval, min_cap);
		return;
	}

	/* calculate CapMax */
	fuelgauge->capacity_max = (raw_soc_val.intval * 100 / (capacity + 1));
	fuelgauge->capacity_old = capacity;

	fuelgauge->capacity_max =
		s2mpw03_fg_check_capacity_max(fuelgauge,
		fuelgauge->capacity_max);

	pr_info("%s: %d is used for capacity_max, capacity(%d)\n",
		__func__, fuelgauge->capacity_max, capacity);
}

static bool s2mpw03_fuelgauge_fuelalert_init(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2];

	/* 1. Set s2mpw03 alert configuration. */
	s2mpw03_alert_init(fuelgauge);

	if (s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_INT, 2, data) < 0)
		return false;

	/*Enable VBAT, SOC */
	data[1] &= ~(VBAT_L_IM_MASK | SOC_L_IM_MASK);
	/*Disable IDLE_ST, INIT_ST */
	data[1] |= (IDLE_ST_IM_MASK | INIT_ST_IM_MASK);
	s2mpw03_fg_bulk_write(fuelgauge, S2MPW03_FG_REG_INT, 2, data);

	dev_dbg(&fuelgauge->i2c->dev, "%s: irq_reg(%02x%02x) irq(%d)\n",
		__func__, data[1], data[0], fuelgauge->pdata->fg_irq);

	s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_INT, 2, data);
	dev_info(&fuelgauge->i2c->dev, "%s: data(0x%x, 0x%x)\n",
		__func__, data[1], data[0]);

	return true;
}

bool s2mpw03_fuelgauge_is_fuelalerted(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	return s2mpw03_check_status(fuelgauge);
}

bool s2mpw03_hal_fg_fuelalert_process(void *irq_data, bool is_fuel_alerted)
{
	struct s2mpw03_fuelgauge_data *fuelgauge = irq_data;
	int ret;

	ret = s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_INT, 0x00);
	if (ret < 0)
		dev_err(&fuelgauge->i2c->dev, "%s: Error(%d)\n", __func__, ret);

	return ret;
}

bool s2mpw03_hal_fg_full_charged(struct i2c_client *client)
{
	return true;
}

static int s2mpw03_fg_rewrite_bat_param(struct s2mpw03_fuelgauge_data *fuelgauge, int step)
{
	struct fg_age_data_info *bat_data = &FG_DATA(step);
	int i;

	mutex_lock(&fuelgauge->fg_lock);
	/*Set 0x17[4] to restore batt param at the bootloader when abnormal power off*/
	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG2, 0x10, 0x10);

	for (i = 0; i < S2MPW03_FG_TABLE1_NUM; i++)
		s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_RTEMP2 + i,
				bat_data->model_table1[i]);

	s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_IRQ_LVL2,
			bat_data->model_table2[0]);

	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_MONOUT_CFG, 
			bat_data->model_table2[1], 0xF0);

	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_WCURR_SHIFT, 
			bat_data->model_table2[2], 0xF0);

	for (i = 0; i < S2MPW03_FG_PARAM1_NUM; i++)
		s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_PARAM1 + i,
				bat_data->model_param1[i]);

	for (i = 0; i < S2MPW03_FG_PARAM2_NUM; i++)
		s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_PARAM2 + i,
				bat_data->model_param2[i]);

	/* Successfully finished, clear 0x17[4] */
	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG2, 0x00, 0x10);

	mutex_unlock(&fuelgauge->fg_lock);
	pr_info("%s, re-write all battery parameter!\n", __func__);
	return 1;
}

static int s2mpw03_fg_update_bat_param(struct s2mpw03_fuelgauge_data *fuelgauge, int step)
{
	struct fg_age_data_info *bat_data = &FG_DATA(step);
	int i, ret = 0;
	u8 param0, param1, param2;
	u8 batcap0, batcap1;
	u8 chg1, chg2;

	pr_info("%s start\n", __func__);
	mutex_lock(&fuelgauge->fg_lock);

	if (!fuelgauge->fg_num_age_step)
		goto exit_fg_update_bat_param;

	fuelgauge->fg_age_step = step;

	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_RBATCAP, &batcap0);
	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_RBATCAP + 1, &batcap1);

	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_RZADJ2, &chg1);
	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_IRQ_LVL2, &chg2);

	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_PARAM1, &param0);
	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_PARAM1 + 1, &param1);
	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_PARAM1 + 2, &param2);

	if ((param0 != bat_data->model_param1[0]) ||
		(param1 != bat_data->model_param1[1]) ||
		(param2 != bat_data->model_param1[2]) ||
		(batcap0 != bat_data->model_table1[1]) ||
		(batcap1 != bat_data->model_table1[2]) ||
		(chg1 != bat_data->model_table1[4]) ||
		(chg2 != bat_data->model_table2[0])) {

		/* charger off */
		sec_votef(VN_CHG_EN, VOTER_S2MPW03_CONTROL, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

		/*Set 0x17[4] to restore batt param at the bootloader when abnormal power off*/
		s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG2, 0x10, 0x10);

		for (i = 0; i < S2MPW03_FG_TABLE1_NUM; i++)
			s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_RTEMP2 + i,
				bat_data->model_table1[i]);

		s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_IRQ_LVL2,
			bat_data->model_table2[0]);

		s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_MONOUT_CFG, 
			bat_data->model_table2[1], 0xF0);

		s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_WCURR_SHIFT, 
				bat_data->model_table2[2], 0xF0);

		for (i = 0; i < S2MPW03_FG_PARAM1_NUM; i++)
			s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_PARAM1 + i,
				bat_data->model_param1[i]);

		for (i = 0; i < S2MPW03_FG_PARAM2_NUM; i++)
			s2mpw03_fg_write_reg(fuelgauge, S2MPW03_FG_REG_PARAM2 + i,
				bat_data->model_param2[i]);

		/* Successfully finished, clear 0x17[4] */
		s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG2, 0x00, 0x10);

		s2mpw03_restart_gauging(fuelgauge);

		/* charger on */
		sec_votef(VN_CHG_EN, VOTER_S2MPW03_CONTROL, false, false);

		ret = 1;
	}

exit_fg_update_bat_param:
	mutex_unlock(&fuelgauge->fg_lock);
	pr_info("%s end\n", __func__);
	return ret;
}

static int s2mpw03_fuelgauge_get_age_step(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	u8 data[2] = {0,};
	int i;

	s2mpw03_fg_bulk_read(fuelgauge, S2MPW03_FG_REG_RBATCAP, 2, data);

	for (i = 0; i < fuelgauge->fg_num_age_step; i++) {
		if ((fuelgauge->pdata->age_data_info[i].model_table1[1] == data[0]) &&
			(fuelgauge->pdata->age_data_info[i].model_table1[2] == data[1])) {
			return i;
		}
	}

	return -1;
}

static int s2mpw03_fuelgauge_set_fg_reset_case(
			struct s2mpw03_fuelgauge_data *fuelgauge,	u8 value, u8 mask)
{
	int ret = 0;

	ret = s2mpw03_update_reg(fuelgauge->rtc, S2MP_RTC_REG_ETC1, value, mask);
	if (ret < 0) {
		pr_err("%s: fail to update fg reset case(%d)\n", __func__, ret);
		return ret;
	}

	s2mpw03_read_reg(fuelgauge->rtc, S2MP_RTC_REG_ETC1, &value);
	pr_info("%s: check fg reset case = 0x%x\n",
		__func__, ((value & FG_RESET_MASK) >> FG_RESET_SHIFT));
	return ret;
}

static int s2mpw03_fuelgauge_monitor_work(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	int age_step;

	age_step = s2mpw03_fuelgauge_get_age_step(fuelgauge);
	pr_info("%s: age_step(%d) cap_max(%d)\n",
		__func__, age_step, fuelgauge->capacity_max);

	s2mpw03_fg_test_read(fuelgauge, false);
	return 0;
}

static int s2mpw03_fg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct s2mpw03_fuelgauge_data *fuelgauge = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		return -ENODATA;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->strval = fuelgauge->d_buf;
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		switch (val->intval) {
		case SEC_BATTERY_CAPACITY_DESIGNED:
			val->intval = fuelgauge->pdata->capacity_full;
			break;
		case SEC_BATTERY_CAPACITY_ABSOLUTE:
			val->intval = 0;
			break;
		case SEC_BATTERY_CAPACITY_TEMPERARY:
			val->intval = 0;
			break;
		case SEC_BATTERY_CAPACITY_CURRENT:
			val->intval = 0;
			break;
		case SEC_BATTERY_CAPACITY_AGEDCELL:
			val->intval = 0;
			break;
		case SEC_BATTERY_CAPACITY_CYCLE:
			val->intval = 0;
			break;
		case SEC_BATTERY_CAPACITY_FULL:
			val->intval = fuelgauge->pdata->capacity_full;
			break;
		}
		break;
		/* Cell voltage (VCELL, mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = s2mpw03_get_vbat(fuelgauge);
		break;
		/* Additional Voltage Information (mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		switch (val->intval) {
		case SEC_BATTERY_VOLTAGE_AVERAGE:
			val->intval = s2mpw03_get_avg_val(&fuelgauge->avg_vol, s2mpw03_get_vbat(fuelgauge));
			if (!val->intval)
				val->intval = s2mpw03_get_vbat(fuelgauge);
			dev_info(&fuelgauge->i2c->dev, "%s: avg_voltage (%d)mV\n", __func__,
				val->intval);
			break;
		case SEC_BATTERY_VOLTAGE_OCV:
			val->intval = s2mpw03_get_ocv(fuelgauge);
			break;
		}
		break;
		/* Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval = s2mpw03_get_current(fuelgauge) * 1000;
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			val->intval = s2mpw03_get_current(fuelgauge);
			break;
		}
		break;
		/* Average Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval = s2mpw03_get_avg_val(&fuelgauge->avg_curr,
					s2mpw03_get_current(fuelgauge)) * 1000;
			dev_info(&fuelgauge->i2c->dev, "%s: avg_current (%d)uA\n", __func__,
				val->intval);
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			val->intval = s2mpw03_get_avg_val(&fuelgauge->avg_curr,
					s2mpw03_get_current(fuelgauge));
			dev_info(&fuelgauge->i2c->dev, "%s: avg_current (%d)mA\n", __func__,
				val->intval);
			break;
		}
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RAW) {
			ret = s2mpw03_get_rawsoc(fuelgauge, val);
		} else if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE) {
			val->intval = fuelgauge->raw_capacity;
		} else {
			ret = s2mpw03_get_soc(fuelgauge, val);
			if (ret < 0) {
				pr_err("%s: failed to get soc(%d), keep state(%d)\n",
					__func__, ret, fuelgauge->capacity_old);
				val->intval = fuelgauge->capacity_old;
				break;
			}

			if (fuelgauge->pdata->capacity_calculation_type &
				(SEC_FUELGAUGE_CAPACITY_TYPE_SCALE |
					SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE)) {
				s2mpw03_fg_get_scaled_capacity(fuelgauge, val);

				if (val->intval > 1010) {
					pr_info("%s : scaled capacity (%d)\n", __func__, val->intval);
					s2mpw03_fg_calculate_dynamic_scale(fuelgauge, fuelgauge->capacity_old);
				}
			}

			/* capacity should be between 0% and 100%
			 * (0.1% degree)
			 */
			if (val->intval > 1000)
				val->intval = 1000;
			if (val->intval < 0)
				val->intval = 0;
			fuelgauge->raw_capacity = val->intval;

			/* get only integer part */
			val->intval /= 10;

			/* check whether doing the __pm_relax */
			if ((val->intval > fuelgauge->pdata->fuel_alert_soc) &&
					fuelgauge->is_fuel_alerted) {
				__pm_relax(fuelgauge->fuel_alert_ws);
				s2mpw03_fuelgauge_fuelalert_init(fuelgauge);
			}

			/* (Only for atomic capacity)
			 * In initial time, capacity_old is 0.
			 * and in resume from sleep,
			 * capacity_old is too different from actual soc.
			 * should update capacity_old
			 * by val->intval in booting or resume.
			 */
			if (fuelgauge->initial_update_of_soc) {
				/* updated old capacity */
				fuelgauge->capacity_old = val->intval;
				fuelgauge->initial_update_of_soc = false;
				break;
			}

			if ((fuelgauge->sleep_initial_update_of_soc) &&
				(fuelgauge->capacity_old >= val->intval)) {
				/* updated old capacity in case of resume */
				fuelgauge->capacity_old = val->intval;
				fuelgauge->sleep_initial_update_of_soc = false;
				break;
			}

			if (fuelgauge->pdata->capacity_calculation_type &
				(SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC |
				SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL))
				s2mpw03_fg_get_atomic_capacity(fuelgauge, val);
		}
		break;
	/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
	/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = s2mpw03_get_temperature(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = fuelgauge->capacity_max;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = 0;
		break;
#ifdef CONFIG_HW_SUSPEND_ENERGY_ESTIMATOR
	case POWER_SUPPLY_PROP_CURRENT_SUSPEND:
		val->intval = s2mpw03_get_current_suspend(fuelgauge, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = fuelgauge->pdata->capacity_full * fuelgauge->raw_capacity;
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING:
		{
			u8 data = 0;

			s2mpw03_read_reg(fuelgauge->pmic, S2MPW03_PMIC_REG_LDO_CTRL2, &data);
			val->intval = !!(data & S2MPW03_FG_SENSING_VSYS);
		}
			break;
		case POWER_SUPPLY_EXT_PROP_FG_INBAT_VOLTAGE:
			val->intval = s2mpw03_get_vbat(fuelgauge);
			s2mpw03_restart_gauging(fuelgauge);
			break;
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			val->intval = 0;
			s2mpw03_fuelgauge_monitor_work(fuelgauge);
			break;
		case POWER_SUPPLY_EXT_PROP_LONG_LIFE_STEP:
			val->intval = s2mpw03_fuelgauge_get_age_step(fuelgauge);
			break;
		default:
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int s2mpw03_fg_set_property(struct power_supply *psy,
	enum power_supply_property psp, const union power_supply_propval *val)
{
	struct s2mpw03_fuelgauge_data *fuelgauge = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		if (fuelgauge->pdata->capacity_calculation_type &
				SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE)
			s2mpw03_fg_calculate_dynamic_scale(fuelgauge, val->intval);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		fuelgauge->cable_type = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		switch (val->intval) {
		case SEC_BAT_CHG_MODE_BUCK_OFF:
		case SEC_BAT_CHG_MODE_CHARGING_OFF:
			fuelgauge->is_charging = false;
			break;
		case SEC_BAT_CHG_MODE_CHARGING:
			fuelgauge->is_charging = true;
			break;
		};
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RESET) {
			s2mpw03_restart_gauging(fuelgauge);
			fuelgauge->initial_update_of_soc = true;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		s2mpw03_set_temperature(fuelgauge, val->intval);
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
	case POWER_SUPPLY_PROP_ENERGY_FULL:
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		dev_info(&fuelgauge->i2c->dev,
			"%s: capacity_max changed, %d -> %d\n",
			__func__, fuelgauge->capacity_max, val->intval);
		fuelgauge->capacity_max = s2mpw03_fg_check_capacity_max(fuelgauge, val->intval);
		fuelgauge->initial_update_of_soc = true;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		/* s2mpw03_fg_reset_capacity_by_jig_connection(fuelgauge->i2c); */
		break;
	case POWER_SUPPLY_PROP_CALIBRATE:
		dev_info(&fuelgauge->i2c->dev,
			"%s: POWER_SUPPLY_PROP_CALIBRATE\n", __func__);
		s2mpw03_restart_gauging(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING:
			if (val->intval == 1) {
				pr_info("%s fg sensing switch to Vsys\n", __func__);
				s2mpw03_update_reg(fuelgauge->pmic, S2MPW03_PMIC_REG_LDO_CTRL2,
					S2MPW03_FG_SENSING_VSYS, S2MPW03_FG_SENSING_MASK);
				s2mpw03_restart_gauging(fuelgauge);
			} else if (val->intval == 0) {
				pr_info("%s fg sensing switch to Vbat\n", __func__);
				s2mpw03_update_reg(fuelgauge->pmic, S2MPW03_PMIC_REG_LDO_CTRL2,
					S2MPW03_FG_SENSING_VBATFG, S2MPW03_FG_SENSING_MASK);
				msleep(1000);
			} else {
				pr_err("%s() invalid param\n", __func__);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_UPDATE_BATTERY_DATA:
			s2mpw03_fg_update_bat_param(fuelgauge, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_FACTORY_MODE:
			if (val->intval) {
				s2mpw03_fuelgauge_set_fg_reset_case(fuelgauge,
					FG_RESET_BY_JIG, FG_RESET_BY_JIG);
				fuelgauge->factory_mode = val->intval;
				pr_info("%s, factory mode\n", __func__);
			}
			break;
		default:
			break;
		}
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static void s2mpw03_fg_isr_work(struct work_struct *work)
{
	struct s2mpw03_fuelgauge_data *fuelgauge =
		container_of(work, struct s2mpw03_fuelgauge_data, isr_work.work);
	u8 fg_alert_status = 0;

	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_STATUS2, &fg_alert_status);
	dev_info(&fuelgauge->i2c->dev, "%s : fg_alert_status(0x%x)\n",
		__func__, fg_alert_status);

	fg_alert_status &= (VBAT_L_MASK | SOC_L_MASK);
	if (fg_alert_status & VBAT_L_MASK)
		pr_info("%s : Battery Voltage is very Low!\n", __func__);

	if (fg_alert_status & SOC_L_MASK)
		pr_info("%s : Battery Level is Very Low!\n", __func__);

	if (!fg_alert_status)
		pr_info("%s : SOC and VBAT are OK!\n", __func__);
	__pm_relax(fuelgauge->fuel_alert_ws);
}

static irqreturn_t s2mpw03_fg_irq_thread(int irq, void *irq_data)
{
	struct s2mpw03_fuelgauge_data *fuelgauge = irq_data;
	u8 fg_irq = 0;

	s2mpw03_fg_read_reg(fuelgauge, S2MPW03_FG_REG_INT, &fg_irq);
	dev_info(&fuelgauge->i2c->dev, "%s: fg_irq(0x%x)\n",
		__func__, fg_irq);
	__pm_stay_awake(fuelgauge->fuel_alert_ws);
	schedule_delayed_work(&fuelgauge->isr_work, 0);

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static int s2mpw03_fuelgauge_parse_dt(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	struct device_node *np = of_find_node_by_name(NULL, "s2mpw03-fuelgauge");
	int ret;
	const u32 *p;
	int len, i;

	/* reset, irq gpio info */
	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
	} else {
		fuelgauge->pdata->fg_irq = of_get_named_gpio(np, "fuelgauge,fuel_int", 0);
		if (fuelgauge->pdata->fg_irq < 0)
			pr_err("%s error reading fg_irq = %d\n",
				__func__, fuelgauge->pdata->fg_irq);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max",
				&fuelgauge->pdata->capacity_max);
		if (ret < 0)
			pr_err("%s error reading capacity_max %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max_margin",
				&fuelgauge->pdata->capacity_max_margin);
		if (ret < 0)
			pr_err("%s error reading capacity_max_margin %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_min",
				&fuelgauge->pdata->capacity_min);
		if (ret < 0)
			pr_err("%s error reading capacity_min %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_calculation_type",
				&fuelgauge->pdata->capacity_calculation_type);
		if (ret < 0)
			pr_err("%s error reading capacity_calculation_type %d\n",
				__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,voltage_check_count",
				&fuelgauge->pdata->v_check);
		if (ret < 0) {
			fuelgauge->pdata->v_check = 20;
			pr_err("%s voltage check count default(20)\n", __func__);
		}

		ret = of_property_read_u32(np, "fuelgauge,fuel_alert_soc",
				&fuelgauge->pdata->fuel_alert_soc);
		if (ret < 0)
			pr_err("%s error reading pdata->fuel_alert_soc %d\n",
				__func__, ret);

		fuelgauge->pdata->repeated_fuelalert = of_property_read_bool(np,
				"fuelgauge,repeated_fuelalert");

		ret = of_property_read_u32(np, "fuelgauge,capacity_full",
				&fuelgauge->pdata->capacity_full);
		if (ret < 0)
			pr_err("%s error reading pdata->capacity_full %d\n",
				__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,flag",
				&fuelgauge->pdata->flag_value);
		if (ret < 0) {
			pr_err("%s error reading pdata->flag %d\n",
				__func__, ret);

			/* default enable (monout, soc, stuck wa) */
			fuelgauge->pdata->flag_value = 0x7;
		}
		pr_info("%s: flag = 0x%x\n", __func__, fuelgauge->pdata->flag_value);

		/* get battery_params node */
		p = of_get_property(np, "fuelgauge,battery_data", &len);
		if (!p)
			pr_info("%s battery_params node NULL\n", __func__);
		else {
			fuelgauge->fg_num_age_step = len / sizeof(struct fg_age_data_info);
			fuelgauge->pdata->age_data_info = kzalloc(len, GFP_KERNEL);
			ret = of_property_read_u8_array(np, "fuelgauge,battery_data",
					(u8 *)fuelgauge->pdata->age_data_info, len/sizeof(u8));
			pr_info("%s: fg_num_age_step %d len = %d\n",
				__func__, fuelgauge->fg_num_age_step, len);

			for (i = 0; i < fuelgauge->fg_num_age_step; i++)
				pr_info("%s: age_step = %d batcap = 0x%02x%02x "
					"model_param1[0] = 0x%02x model_param2[0] = 0x%02x\n",
					__func__, i,
					fuelgauge->pdata->age_data_info[i].model_table1[2],
					fuelgauge->pdata->age_data_info[i].model_table1[1],
					fuelgauge->pdata->age_data_info[i].model_param1[0],
					fuelgauge->pdata->age_data_info[i].model_param2[0]);
		}

		np = of_find_node_by_name(NULL, "battery");
		if (!np) {
			pr_err("%s np NULL\n", __func__);
		} else {
			ret = of_property_read_string(np,
				"battery,fuelgauge_name",
				(char const **)&fuelgauge->pdata->fuelgauge_name);
			ret = of_property_read_string(np,
				"battery,charger_name",
				(char const **)&fuelgauge->pdata->charger_name);
		}
	}

	return 0;
}

static struct of_device_id s2mpw03_fuelgauge_match_table[] = {
	{ .compatible = "samsung,s2mpw03-fuelgauge",},
	{},
};
#else
static int s2mpw03_fuelgauge_parse_dt(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	return -ENOSYS;
}

#define s2mpw03_fuelgauge_match_table NULL
#endif /* CONFIG_OF */

#define REQUEST_IRQ(_irq, _dev_id, _name) \
do { \
	ret = request_threaded_irq(_irq, NULL, s2mpw03_fg_irq_thread, \
				0, _name, _dev_id);	\
	if (ret < 0) { \
		pr_err("%s: Failed to request s2mpw03 FG IRQ #%d: %d\n", \
				__func__, _irq, ret); \
		_irq = 0; \
	} \
} while (0)

static int s2mpw03_fg_irq_init(struct s2mpw03_dev *s2mpw03, struct s2mpw03_fuelgauge_data *fuelgauge)
{
	int ret = 0;

	if (s2mpw03->irq_base > 0) {
		int irq_base = s2mpw03->irq_base;

		/* request FG IRQ */
		fuelgauge->irq_fg_vbat_low = irq_base + S2MPW03_FG_IRQ_VBAT_L_INT;
		REQUEST_IRQ(fuelgauge->irq_fg_vbat_low, fuelgauge, "fg-vbat-low");

		fuelgauge->irq_fg_soc_low = irq_base + S2MPW03_FG_IRQ_SOC_L_INT;
		REQUEST_IRQ(fuelgauge->irq_fg_soc_low, fuelgauge, "fg-vbat-low");
	}

	return ret;
}

#define FREE_IRQ(_irq, _dev_id, _name) \
do { \
	if (_irq) { \
		free_irq(_irq, _dev_id); \
		pr_info("%s: IRQ(%d):%s free done\n", \
				__func__, _irq, _name); \
	} \
} while (0)

static void s2mpw03_fg_free_irqs(struct s2mpw03_fuelgauge_data *fuelgauge)
{
	pr_info("%s\n", __func__);

	/* free FG IRQ */
	FREE_IRQ(fuelgauge->irq_fg_vbat_low, fuelgauge, "fg-vbat-low");
	FREE_IRQ(fuelgauge->irq_fg_soc_low, fuelgauge, "fg-vbat-low");
}

static struct device_attribute s2mpw03_attrs[] = {
	s2mpw03_FG_ATTR(addr),
	s2mpw03_FG_ATTR(data),
	s2mpw03_FG_ATTR(data_all),
};

static int s2mpw03_fg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < (int)ARRAY_SIZE(s2mpw03_attrs); i++) {
		rc = device_create_file(dev, &s2mpw03_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &s2mpw03_attrs[i]);
	return rc;
}

ssize_t s2mpw03_fg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct s2mpw03_fuelgauge_data *fuelgauge = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - s2mpw03_attrs;
	int i = 0;
	u8 data, addr;

	switch (offset) {
	case ADDR:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "0x%02X\n", fuelgauge->read_reg);
		break;
	case DATA:
		if (s2mpw03_fg_read_reg(fuelgauge, fuelgauge->read_reg, &data) < 0) {
			dev_info(fuelgauge->dev,
				"%s: addr: 0x%x read fail\n", __func__, fuelgauge->read_reg);
			i += scnprintf(buf + i, PAGE_SIZE - i,
			       "0x%02X[FAIL]\n", fuelgauge->read_reg);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i,
			       "0x%02X[0x%02X]\n", fuelgauge->read_reg, data);
		}
		break;
	case DATA_ALL:
		for (addr = S2MPW03_FG_REG_STATUS; addr < (S2MPW03_FG_REG_PARAM2 + S2MPW03_FG_PARAM2_NUM); addr++) {
			if (s2mpw03_fg_read_reg(fuelgauge, addr, &data) < 0) {
				dev_info(fuelgauge->dev,
					"%s: addr: 0x%x read fail\n", __func__, addr);
				i += scnprintf(buf + i, PAGE_SIZE - i,
				       "0x%02X[FAIL],", addr);
			} else {
				i += scnprintf(buf + i, PAGE_SIZE - i,
				       "0x%02X[0x%02X],", addr, data);
			}
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "\n");
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t s2mpw03_fg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct s2mpw03_fuelgauge_data *fuelgauge = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - s2mpw03_attrs;
	int ret = 0;
	unsigned int addr, data;

	switch (offset) {
	case ADDR:
		if (sscanf(buf, "0x%8x", &addr) == 1) {
			dev_info(fuelgauge->dev,
					"%s: addr: 0x%x\n", __func__, addr);
			if (addr < (S2MPW03_FG_REG_PARAM2 + S2MPW03_FG_PARAM2_NUM)) {
				fuelgauge->read_reg = addr;
			} else {
				dev_info(fuelgauge->dev,"%s: addr: 0x%x is wrong\n",
					__func__, addr);
				return -EINVAL;
			}
		}
		ret = count;
		break;
	case DATA:
		if (sscanf(buf, "0x%8x", &data) == 1) {
			dev_info(fuelgauge->dev,
					"%s: data: 0x%x\n", __func__, data);
			if (s2mpw03_fg_write_reg(fuelgauge, fuelgauge->read_reg, data) < 0) {
				dev_info(fuelgauge->dev,
					"%s: addr: 0x%x write fail\n", __func__, fuelgauge->read_reg);
				return -EINVAL;
			}
		}
		ret = count;
		break;
	case DATA_ALL:
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int s2mpw03_fuelgauge_probe(struct platform_device *pdev)
{
	struct s2mpw03_dev *s2mpw03 = dev_get_drvdata(pdev->dev.parent);
	struct s2mpw03_fuelgauge_data *fuelgauge;
	struct power_supply_config psy_cfg = {};
	union power_supply_propval raw_soc_val = {0, };
	int ret = 0;

	pr_info("%s: S2MPW03 Fuelgauge Driver Loading\n", __func__);

	fuelgauge = kzalloc(sizeof(*fuelgauge), GFP_KERNEL);
	if (!fuelgauge)
		return -ENOMEM;

	mutex_init(&fuelgauge->fg_lock);
	mutex_init(&fuelgauge->rst_lock);

	fuelgauge->dev = &pdev->dev;
	fuelgauge->i2c = s2mpw03->fuelgauge;
	fuelgauge->pmic = s2mpw03->pmic;
	fuelgauge->rtc = s2mpw03->rtc;
	fuelgauge->common = s2mpw03->i2c;
	fuelgauge->avg_batt[0] = 0;
	fuelgauge->avg_batt[1] = 0;
	fuelgauge->avg_batt[2] = 0;
	fuelgauge->err_cnt = 0;
	fuelgauge->wa_cnt = 0;

	/* fuelgauge i2c write enable */
	// will be moved to bootloader
	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_INTM,
		FG_IF_EN_MASK, FG_IF_EN_MASK);

	fuelgauge->pdata = devm_kzalloc(&pdev->dev, sizeof(*(fuelgauge->pdata)),
			GFP_KERNEL);
	if (!fuelgauge->pdata) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_parse_dt_nomem;
	}

	ret = s2mpw03_fuelgauge_parse_dt(fuelgauge);
	if (ret < 0)
		goto err_parse_dt;

	platform_set_drvdata(pdev, fuelgauge);

	if (fuelgauge->pdata->fuelgauge_name == NULL)
		fuelgauge->pdata->fuelgauge_name = "s2mpw03-fuelgauge";

	fuelgauge->psy_fg_desc.name          = fuelgauge->pdata->fuelgauge_name;
	fuelgauge->psy_fg_desc.type          = POWER_SUPPLY_TYPE_UNKNOWN;
	fuelgauge->psy_fg_desc.get_property  = s2mpw03_fg_get_property;
	fuelgauge->psy_fg_desc.set_property  = s2mpw03_fg_set_property;
	fuelgauge->psy_fg_desc.properties    = s2mpw03_fuelgauge_props;
	fuelgauge->psy_fg_desc.num_properties =
			ARRAY_SIZE(s2mpw03_fuelgauge_props);

	fuelgauge->factory_mode = factory_mode;

	/* init voltage */
	s2mpw03_get_vbat(fuelgauge);

	/* temperature level init */
	fuelgauge->before_temp_level = TEMP_LEVEL_MID;

	fuelgauge->capacity_max = fuelgauge->pdata->capacity_max;
	if (s2mpw03_get_rawsoc(fuelgauge, &raw_soc_val) >= 0) {
		raw_soc_val.intval = raw_soc_val.intval / 10;

		if (raw_soc_val.intval > fuelgauge->capacity_max)
			s2mpw03_fg_calculate_dynamic_scale(fuelgauge, 100);

		/* update capacity old */
		s2mpw03_fg_get_scaled_capacity(fuelgauge, &raw_soc_val);
		fuelgauge->capacity_old = raw_soc_val.intval;
	}

	psy_cfg.drv_data = fuelgauge;
	fuelgauge->psy_fg = power_supply_register(&pdev->dev, &fuelgauge->psy_fg_desc, &psy_cfg);
	if (IS_ERR(fuelgauge->psy_fg)) {
		pr_err("%s: Failed to Register psy_fg\n", __func__);
		ret = PTR_ERR(fuelgauge->psy_fg);
		goto err_data_free;
	}

	ret = s2mpw03_fg_create_attrs(&fuelgauge->psy_fg->dev);
	if (ret)
		dev_err(s2mpw03->dev,
			"%s : Failed to create_attrs\n", __func__);

	if (fuelgauge->pdata->fuel_alert_soc >= 0) {
		s2mpw03_fuelgauge_fuelalert_init(fuelgauge);
		fuelgauge->fuel_alert_ws = wakeup_source_register(
						fuelgauge->dev, "fuel_alerted");
		INIT_DELAYED_WORK(&fuelgauge->isr_work, s2mpw03_fg_isr_work);

		ret = s2mpw03_fg_irq_init(s2mpw03, fuelgauge);
		if (ret) {
			pr_err("[fg] %s: failed to init fg irq(%d)\n", __func__, ret);
			goto fail_init_irq;
		}
	}

	s2mpw03_save_trim_value(fuelgauge);
	s2mpw03_update_trim_value(fuelgauge, false);

	fuelgauge->sleep_initial_update_of_soc = false;
	fuelgauge->initial_update_of_soc = true;

	/* FG Clock Switching W/A */
	/* 0x17[6] = 0 */
	s2mpw03_fg_update_reg(fuelgauge, S2MPW03_FG_REG_CONFIG2,
		0x00, 0x40);
	s2mpw03_fg_write_reg(fuelgauge, 0x25, 0x78);
	s2mpw03_write_reg(fuelgauge->common, 0x0D, 0x40);
	usleep_range(2000, 3000);
	s2mpw03_fg_write_reg(fuelgauge, 0x25, 0x70);
	s2mpw03_write_reg(fuelgauge->common, 0x0D, 0x00);

	s2mpw03_fg_write_reg(fuelgauge, 0x18, 0x24);
	s2mpw03_write_reg(fuelgauge->common, 0x15, 0x03);

	fuelgauge->fg_age_step = s2mpw03_fuelgauge_get_age_step(fuelgauge);

	pr_info("%s: S2MPW03 Fuelgauge Driver Loaded\n", __func__);
	s2mpw03_fg_test_read(fuelgauge, true);
	return 0;

fail_init_irq:
	s2mpw03_fg_free_irqs(fuelgauge);
	power_supply_unregister(fuelgauge->psy_fg);
err_data_free:
	if (pdev->dev.of_node)
		kfree(fuelgauge->pdata);

err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&fuelgauge->fg_lock);
	mutex_destroy(&fuelgauge->rst_lock);
	kfree(fuelgauge);

	return ret;
}

static void s2mpw03_fuelgauge_shutdown(struct platform_device *pdev)
{
}

static int s2mpw03_fuelgauge_remove(struct platform_device *pdev)
{
	struct s2mpw03_fuelgauge_data *fuelgauge = platform_get_drvdata(pdev);

	if (fuelgauge->pdata->fuel_alert_soc >= 0)
		wakeup_source_unregister(fuelgauge->fuel_alert_ws);

	return 0;
}

#if defined CONFIG_PM
static int s2mpw03_fuelgauge_suspend(struct device *dev)
{
	return 0;
}

static int s2mpw03_fuelgauge_resume(struct device *dev)
{
	struct s2mpw03_fuelgauge_data *fuelgauge = dev_get_drvdata(dev);
#ifdef CONFIG_HW_SUSPEND_ENERGY_ESTIMATOR
	s2mpw03_calc_current_suspend(fuelgauge);
#endif
	fuelgauge->sleep_initial_update_of_soc = !fuelgauge->is_charging;

	return 0;
}
#else
#define s2mpw03_fuelgauge_suspend NULL
#define s2mpw03_fuelgauge_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(s2mpw03_fuelgauge_pm_ops, s2mpw03_fuelgauge_suspend,
		s2mpw03_fuelgauge_resume);

static struct platform_driver s2mpw03_fuelgauge_driver = {
	.driver = {
		.name = "s2mpw03-fuelgauge",
		.owner = THIS_MODULE,
		.pm = &s2mpw03_fuelgauge_pm_ops,
		.of_match_table = s2mpw03_fuelgauge_match_table,
	},
	.probe = s2mpw03_fuelgauge_probe,
	.remove = s2mpw03_fuelgauge_remove,
	.shutdown = s2mpw03_fuelgauge_shutdown,
};

static int __init s2mpw03_fuelgauge_init(void)
{
	int ret = 0;
	pr_info("%s: S2MPW03 Fuelgauge Init\n", __func__);
	ret = platform_driver_register(&s2mpw03_fuelgauge_driver);

	return ret;
}

static void __exit s2mpw03_fuelgauge_exit(void)
{
	platform_driver_unregister(&s2mpw03_fuelgauge_driver);
}
device_initcall(s2mpw03_fuelgauge_init);
module_exit(s2mpw03_fuelgauge_exit);

MODULE_DESCRIPTION("Samsung S2MPW03 Fuel Gauge Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
