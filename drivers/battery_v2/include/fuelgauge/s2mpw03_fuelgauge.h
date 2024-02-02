/*
 * s2mpw03_fuelgauge.h - Header of S2MPW03 Fuel Gauge
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

#ifndef __S2MPW03_FUELGAUGE_H
#define __S2MPW03_FUELGAUGE_H __FILE__
#include <linux/mfd/samsung/s2mpw03.h>
#include <linux/mfd/samsung/s2mpw03-regulator.h>

#if defined(ANDROID_ALARM_ACTIVATED)
#include <linux/android_alarm.h>
#endif

#include "../sec_charging_common.h"
#include "../sec_battery_vote.h"

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */

#define MASK(width, shift) (((0x1 << (width)) - 1) << shift)

#define FG_DATA(fname) (fuelgauge->pdata->age_data_info[fname])

#define S2MPW03_FG_REG_STATUS		0x00
#define S2MPW03_FG_REG_STATUS2		0x01
#define S2MPW03_FG_REG_INT			0x02
#define S2MPW03_FG_REG_INTM			0x03
#define S2MPW03_FG_REG_RVBAT		0x04
#define S2MPW03_FG_REG_ROCV			0x06
#define S2MPW03_FG_REG_RSOC			0x08
#define S2MPW03_FG_REG_RTEMP		0x0A
#define S2MPW03_FG_REG_RTEMP2		0x0B
#define S2MPW03_FG_REG_RBATCAP		0x0C
#define S2MPW03_FG_REG_RZADJ		0x0E
#define S2MPW03_FG_REG_RZADJ2		0x0F
#define S2MPW03_FG_REG_RBATZ0		0x10
#define S2MPW03_FG_REG_RBATZ1		0x12
#define S2MPW03_FG_REG_IRQ_LVL		0x14
#define S2MPW03_FG_REG_IRQ_LVL2		0x15
#define S2MPW03_FG_REG_CONFIG		0x16
#define S2MPW03_FG_REG_CONFIG2		0x17
#define S2MPW03_FG_REG_MONOUT_CFG	0x19
#define S2MPW03_FG_REG_CURR			0x1A
#define S2MPW03_FG_REG_CURR2		0x1B
#define S2MPW03_FG_REG_VBATI2C		0x1C
#define S2MPW03_FG_REG_ADC_DATA		0x22
#define S2MPW03_FG_REG_ADC_DATA2	0x23
#define S2MPW03_FG_REG_VTS_SHIFT	0x24
#define S2MPW03_FG_REG_VTL_SHIFT	0x25
#define S2MPW03_FG_REG_OTP_01_L		0x26
#define S2MPW03_FG_REG_WCURR_SHIFT	0x28
#define S2MPW03_FG_OFFSET			0x2B
#define S2MPW03_FG_REG_TSAMPLE		0x2C

#define S2MPW03_FG_REG_PARAM1		0x2E
#define S2MPW03_FG_REG_PARAM2		0xBC

#define S2MPW03_FG_TABLE1_NUM		5
#define S2MPW03_FG_TABLE2_NUM		3
#define S2MPW03_FG_PARAM1_NUM		88
#define S2MPW03_FG_PARAM2_NUM		22

#define	S2MPW03_FG_WA_ADDR		0x19

/* S2MPW03_FG_REG_STATUS2 */
#define VBAT_L_SHIFT				0
#define VBAT_L_MASK					BIT(VBAT_L_SHIFT)
#define SOC_L_SHIFT					1
#define SOC_L_MASK					BIT(SOC_L_SHIFT)

/* S2MPW03_FG_REG_INTM */
#define VBAT_L_IM_SHIFT				0
#define VBAT_L_IM_MASK				BIT(VBAT_L_IM_SHIFT)
#define SOC_L_IM_SHIFT				1
#define SOC_L_IM_MASK				BIT(SOC_L_IM_SHIFT)
#define IDLE_ST_IM_SHIFT			2
#define IDLE_ST_IM_MASK				BIT(IDLE_ST_IM_SHIFT)
#define INIT_ST_IM_SHIFT			3
#define INIT_ST_IM_MASK				BIT(INIT_ST_IM_SHIFT)
#define FG_IF_EN_SHIFT				7
#define FG_IF_EN_MASK				BIT(FG_IF_EN_SHIFT)

/* S2MPW03_FG_REG_LVL */
#define SOC_L_LVL_SHIFT				4

/* S2MPW03_FG_REG_CONFIG */
#define DUMP_DONE_SHIFT				0
#define DUMP_DONE_WIDTH				2
#define DUMP_DONE_MASK				MASK(DUMP_DONE_WIDTH, DUMP_DONE_SHIFT)
#define RESTART_SHIFT				4
#define RESTART_WIDTH				2
#define RESTART_MASK				MASK(RESTART_WIDTH, RESTART_SHIFT)

/* S2MPW03_FG_REG_CONFIG2 */
#define CHG_I2C_EN_SHIFT			2
#define CHG_I2C_EN_MASK				BIT(CHG_I2C_EN_SHIFT)
#define CHG_I2C_SHIFT				3
#define CHG_I2C_MASK				BIT(CHG_I2C_SHIFT)

/* S2MPW03_FG_REG_MONSEL_CFG */
#define MONOUT_SEL_SHIFT			0
#define MONOUT_SEL_WIDTH			4
#define MONOUT_SEL_MASK				MASK(MONOUT_SEL_WIDTH, MONOUT_SEL_SHIFT)
#define MONOUT_SEL_VTS				1
#define MONOUT_SEL_VTL				2
#define MONOUT_SEL_VDIFF			3
#define MONOUT_SEL_RS				4
#define MONOUT_SEL_RCUR				5

#define MONOUT_RS_VALID				1000

#define FG_SAMPLE_SHIFT				0
#define FG_SAMPLE_WIDTH				3
#define FG_SAMPLE_MASK				MASK(FG_SAMPLE_WIDTH, FG_SAMPLE_SHIFT)

#define FG_SAMPLE_TIME_1S			0
#define FG_SAMPLE_TIME_2S			1
#define FG_SAMPLE_TIME_4S			2
#define FG_SAMPLE_TIME_8S			3
#define FG_SAMPLE_TIME_16S			4

#define S2MPW03_FG_SENSING_VBATFG	(0 << 7)
#define S2MPW03_FG_SENSING_VSYS		(1 << 7)
#define S2MPW03_FG_SENSING_MASK		(1 << 7)

#define AVG_VAL_CNT 5

#define ABN_MONOUT_WA	3
#define ABN_SOC_WA		5
#define ABN_STUCK_WA	7

extern int factory_mode;

ssize_t s2mpw03_fg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t s2mpw03_fg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);
#define s2mpw03_FG_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = s2mpw03_fg_show_attrs,			\
	.store = s2mpw03_fg_store_attrs,			\
}
enum {
	ADDR = 0,
	DATA,
	DATA_ALL,
};

struct sec_fg_info {
	/* test print count */
	int pr_cnt;
	/* full charge comp */
	/* struct delayed_work full_comp_work; */
	u32 previous_fullcap;
	u32 previous_vffullcap;
	/* low battery comp */
	int low_batt_comp_flag;
	/* low battery boot */
	int low_batt_boot_flag;
	bool is_low_batt_alarm;

	/* battery info */
	u32 soc;

	/* miscellaneous */
	unsigned long fullcap_check_interval;
	int full_check_flag;
	bool is_first_check;
};

struct fg_age_data_info {
	u8 model_table1[S2MPW03_FG_TABLE1_NUM];
	u8 model_table2[S2MPW03_FG_TABLE2_NUM];
	/* battery model data */
	u8 model_param1[S2MPW03_FG_PARAM1_NUM];
	u8 model_param2[S2MPW03_FG_PARAM2_NUM];
};

struct s2mpw03_fuelgauge_platform_data {
	int capacity_max;
	int capacity_max_margin;
	int capacity_min;
	int capacity_calculation_type;
	int fuel_alert_soc;
	int fullsocthr;
	int fg_irq;

	char *fuelgauge_name;
	char *charger_name;
	int v_check;
	bool repeated_fuelalert;
	unsigned int capacity_full;

	union {
		unsigned int flag_value;

		struct {
			unsigned monout_wa : 1,
				soc_wa : 1,
				stuck_wa : 1;

			unsigned resv : 29;
		} flag;
	};

	struct fg_age_data_info *age_data_info;
};

struct avg_data {
	unsigned int avg_cnt;
	int avg_val[AVG_VAL_CNT];
	int avg_sum;
};

struct s2mpw03_fuelgauge_data {
	struct device *dev;
	struct i2c_client *i2c;
	struct i2c_client *pmic;
	struct i2c_client *rtc;
	struct i2c_client *common;

	struct mutex fuelgauge_mutex;
	struct s2mpw03_fuelgauge_platform_data *pdata;
	struct power_supply	*psy_fg;
	struct power_supply_desc psy_fg_desc;
	int dev_id;
	/* struct delayed_work isr_work; */

	int irq_fg_vbat_low;
	int irq_fg_soc_low;

	int cable_type;
	bool is_charging;

	bool factory_mode;

	/* HW-dedicated fuel guage info structure
	* used in individual fuel gauge file only
	* (ex. dummy_fuelgauge.c)
	*/
	struct sec_fg_info info;
	bool is_fuel_alerted;
	/* need to check(kernel version up)*/
	struct wakeup_source *fuel_alert_ws;
	struct delayed_work scaled_work;

	unsigned int capacity_old; /* only for atomic calculation */
	unsigned int capacity_max; /* only for dynamic calculation */
	unsigned int scaled_capacity_max;
	unsigned int standard_capacity;
	unsigned int rawsoc_old;
	unsigned int rawocv;
	unsigned short voltage_old;
	int v_check_cnt;
	int raw_capacity;
	int vbat_old;

	unsigned long updated_time;

	bool initial_update_of_soc;
	bool sleep_initial_update_of_soc;
	struct mutex fg_lock;
	struct mutex rst_lock;
	struct delayed_work isr_work;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	unsigned int pre_soc;

	struct avg_data avg_vol;
	struct avg_data avg_curr;

	/* temperature level */
	int before_temp_level;

#ifdef CONFIG_HW_SUSPEND_ENERGY_ESTIMATOR
	/* suspend current */
	int suspend_current;
#endif
	int fg_num_age_step;
	int fg_num_age_step_ext;
	int fg_age_step;

	int avg_batt[3];
	int avg_bat;
	char d_buf[128];
	unsigned int err_cnt;
	unsigned int wa_cnt;

	u8 reg_0x26;
	u8 reg_0x27;
	u8 reg_0x28;
	u8 reg_0x29;
	u8 reg_0x2A;
	u8 reg_0x2B;

	u8 read_reg;
};

enum {
	TEMP_LEVEL_VERY_LOW = 0,
	TEMP_LEVEL_LOW,
	TEMP_LEVEL_MID,
	TEMP_LEVEL_HIGH,
};

enum {
	SCALED_VAL_UNKNOWN = 0,
	SCALED_VAL_NO_EXIST,
	SCALED_VAL_EXIST,
};
#endif /* __S2MPW03_FUELGAUGE_H */
