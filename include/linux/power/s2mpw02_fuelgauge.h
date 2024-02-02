/*
 * s2mpw02_fuelgauge.h
 * Samsung S2MPW02 Fuel Gauge Header
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

#ifndef __S2MPW02_FUELGAUGE_H
#define __S2MPW02_FUELGAUGE_H __FILE__
#include <linux/mfd/samsung/s2mpw02.h>
#include <linux/mfd/samsung/s2mpw02-regulator.h>

#if defined(ANDROID_ALARM_ACTIVATED)
#include <linux/android_alarm.h>
#endif

#include <linux/battery/sec_charging_common.h>

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */

#define MASK(width, shift)      (((0x1 << (width)) - 1) << shift)

#define S2MPW02_FG_REG_STATUS		0x00
#define S2MPW02_FG_REG_STATUS2		0x01
#define S2MPW02_FG_REG_INT			0x02
#define S2MPW02_FG_REG_INTM			0x03
#define S2MPW02_FG_REG_RVBAT		0x04
#define S2MPW02_FG_REG_ROCV			0x06
#define S2MPW02_FG_REG_RSOC			0x08
#define S2MPW02_FG_REG_RTEMP		0x0A
#define S2MPW02_FG_REG_RTEMP2		0x0B
#define S2MPW02_FG_REG_RBATCAP		0x0C
#define S2MPW02_FG_REG_RZADJ		0x0E
#define S2MPW02_FG_REG_RZADJ2		0x0F
#define S2MPW02_FG_REG_RBATZ0		0x10
#define S2MPW02_FG_REG_RBATZ1		0x12
#define S2MPW02_FG_REG_IRQ_LVL		0x14
#define S2MPW02_FG_REG_IRQ_LVL2		0x15
#define S2MPW02_FG_REG_CONFIG		0x16
#define S2MPW02_FG_REG_CONFIG2		0x17
#define S2MPW02_FG_REG_MONOUT_CFG	0x19
#define S2MPW02_FG_REG_CURR			0x1A
#define S2MPW02_FG_REG_CURR2		0x1B
#define S2MPW02_FG_REG_ADC_DATA		0x22
#define S2MPW02_FG_REG_ADC_DATA2	0x23
#define S2MPW02_FG_REG_VTS_SHIFT	0x24
#define S2MPW02_FG_REG_VTL_SHIFT	0x25
#define S2MPW02_FG_REG_OTP67		0x2C


/* S2MPW02_FG_REG_STATUS2 */
#define VBAT_L_SHIFT				0
#define VBAT_L_MASK					BIT(VBAT_L_SHIFT)
#define SOC_L_SHIFT					1
#define SOC_L_MASK					BIT(SOC_L_SHIFT)

/* S2MPW02_FG_REG_INTM */
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

/* S2MPW02_FG_REG_LVL */
#define SOC_L_LVL_SHIFT				4

/* S2MPW02_FG_REG_CONFIG */
#define DUMP_DONE_SHIFT				0
#define DUMP_DONE_WIDTH				2
#define DUMP_DONE_MASK				MASK(DUMP_DONE_WIDTH, DUMP_DONE_SHIFT)
#define RESTART_SHIFT				4
#define RESTART_WIDTH				2
#define RESTART_MASK				MASK(RESTART_WIDTH, RESTART_SHIFT)

/* S2MPW02_FG_REG_CONFIG2 */
#define CHG_I2C_EN_SHIFT			2
#define CHG_I2C_EN_MASK				BIT(CHG_I2C_EN_SHIFT)
#define CHG_I2C_SHIFT				3
#define CHG_I2C_MASK				BIT(CHG_I2C_SHIFT)

/* S2MPW02_FG_REG_MONSEL_CFG */
#define MONOUT_SEL_SHIFT			0
#define MONOUT_SEL_WIDTH			4
#define MONOUT_SEL_MASK				MASK(MONOUT_SEL_WIDTH, MONOUT_SEL_SHIFT)
#define MONOUT_SEL_RCUR				5

struct sec_fg_info {
	/* test print count */
	int pr_cnt;
	/* full charge comp */
	/* struct delayed_work     full_comp_work; */
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

struct s2mpw02_fuelgauge_platform_data {
	int capacity_max;
	int capacity_max_margin;
	int capacity_min;
	int capacity_calculation_type;
	int fuel_alert_soc;
	int fullsocthr;
	int fg_irq;

	char *fuelgauge_name;

	bool repeated_fuelalert;

	struct sec_charging_current *charging_current;
};

struct s2mpw02_fuelgauge_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic;

	struct mutex            fuelgauge_mutex;
	struct s2mpw02_fuelgauge_platform_data *pdata;
	struct power_supply	*psy_fg;
	struct power_supply_desc psy_fg_desc;
	int dev_id;
	/* struct delayed_work isr_work; */

	int cable_type;
	bool is_charging;

	/* HW-dedicated fuel guage info structure
	* used in individual fuel gauge file only
	* (ex. dummy_fuelgauge.c)
	*/
	struct sec_fg_info      info;
	bool is_fuel_alerted;
	/* need to check(kernel version up)*/
	//struct wake_lock fuel_alert_wake_lock;
	struct delayed_work scaled_work;

	unsigned int capacity_old;      /* only for atomic calculation */
	unsigned int capacity_max;      /* only for dynamic calculation */
	unsigned int scaled_capacity_max;
	unsigned int standard_capacity;

	bool initial_update_of_soc;
	struct mutex fg_lock;
	struct delayed_work isr_work;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	unsigned int pre_soc;
	int fg_irq;

	/* temperature level */
	int before_temp_level;
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
#endif /* __S2MPW02_FUELGAUGE_H */
