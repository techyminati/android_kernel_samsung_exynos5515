#ifndef __IST207W_MOTOR_H__
#define __IST207W_MOTOR_H__
/*
** =============================================================================
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
** File:
**     ist207w_motor.h
**
** Description:
**     Header file for ist207w_motor.c
**
** =============================================================================
*/

#include <linux/regmap.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/pm_wakeup.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/regulator/consumer.h>
#include <linux/time.h>


#define HAPTICS_DEVICE_NAME "ist207w_motor"

#define	MOTOR_REG_MODE_00		0x00
#define	MOTOR_REG_MODE_01		0x01
#define	MOTOR_REG_AP_COMMUNICATION_EN	0x02
#define	MOTOR_REG_COMMUNICATION_FLAG 0x0F
#define	MOTOR_REG_SOFT_EN		0x10
#define	MOTOR_REG_STRENGTH		0x11
#define	MOTOR_REG_ADC_SAMPLING_TIME	0x12
#define	MOTOR_REG_MODE_13		0x13
#define	MOTOR_REG_OVER_DRV		0x14
#define	MOTOR_REG_START_STRENGTH	0x19
#define	MOTOR_REG_SEARCH_DRV_RATIO1	0x1A
#define	MOTOR_REG_SEARCH_DRV_RATIO2	0x1B
#define	MOTOR_REG_SEARCH_DRV_RATIO3	0x1C
#define	MOTOR_REG_DRV_FREQ_H		0x1F
#define	MOTOR_REG_DRV_FREQ_L		0x20
#define	MOTOR_REG_RESO_FREQ_H		0x21
#define	MOTOR_REG_RESO_FREQ_L		0x22
#define	MOTOR_REG_FUNC_ENABLE		0x23
#define	MOTOR_REG_OUTPUT		0x24
#define	MOTOR_REG_FIFO_INTERVAL		0x27
#define	MOTOR_REG_FIFO_DATA		0x28
#define	MOTOR_REG_FIFO_STRENGTH		0x2A

#define	MOTOR_REG_MODE_00_MASK		0x17
#define	MODE_00_PWM			0x14
#define	MODE_00_I2C_LRA			0x05
#define	MODE_00_STOP_LRA		0x00
#define	MODE_00_I2C_ERM			0x05
#define	MODE_00_STOP_ERM		0x00

#define	MOTOR_REG_MODE_01_MASK		0x1B
#define MODE_01_SHIFT_DRV_MODE		0x04
#define MODE_01_SHIFT_NBREAK_EN		0x03
#define MODE_01_SHIFT_PGA_BEMP_GAIN	0x00
#define MODE_01_DEFAULT_PGA_BEMP_GAIN	0x02

#define	MOTOR_REG_SOFT_EN_MASK		0x01
#define SOFT_DISABLE			0x00
#define SOFT_ENABLE			0x01
#define FIFO_ENABLE			0x05

#define	MOTOR_REG_STRENGTH_MASK		0x7F

#define	MOTOR_REG_MODE_13_MASK		0xF9
#define MODE_13_SHIFT_RESO_CAL_MD	0x07
#define MODE_13_SHIFT_RESO_CAL_EN	0x06
#define MODE_13_SHIFT_OUT_PWM_FREQ	0x05
#define MODE_13_SHIFT_SIN_OUT_EN	0x04
#define MODE_13_SHIFT_PWR_CAL_EN	0x03
#define MODE_13_SHIFT_ERM_NLRA		0x00

#define MOTOR_REG_OVER_DRV_EN_MASK	0x01
#define MOTOR_REG_OVER_DRV_CNT_MASK	0x7F
#define OVER_DRV_SHIFT_EN		7
#define OVER_DRV_SHIFT_CNT		0

#define MAX_INTENSITY_VALUE 10000
#define DEFAULT_MOTOR_FREQ 205
#define DEFAULT_MOTOR_STRENGTH 0x56
#define DEFAULT_BRAKE_DELAY 0
#define DEFAULT_ADC_SAMPLING_TIME 0x86
#define DEFAULT_SOFT_EN_DELAY 0
#define MOTOR_VCC 3300000
#define MOTOR_CLK 25000000

enum VIBRATOR_CONTROL {
	VIBRATOR_DISABLE = 0,
	VIBRATOR_ENABLE = 1,
};

enum actuator_type {
	ACTUATOR_LRA = 0,
	ACTUATOR_ERM = 1
};

enum loop_type {
	OPEN_LOOP = 0,
	CLOSED_LOOP = 1
};

struct reg_data {
	u32 addr;
	u32 data;
};

struct ist207w_motor_pdata {
	struct reg_data *init_regs;
	enum actuator_type motor_type;
	enum loop_type	meLoop;
	int frequency;		/* in 0.1 hz*/
	int strength;
	int fifo_strength;
	int count_init_regs;
	int break_delay;
	int en_gpio;
	int adc_sampling_time;
	int soft_en_delay;
	int overdrive_num;
	int motor_start_data;
	int motor_stop_data;
	const char *regulator_name;
	bool break_mode;
	const char *vib_type;
	const char *file_path;
};

struct ist207w_motor_data {
	struct device *dev;
	struct regmap *mpRegmap;
	struct i2c_client *client;
	struct ist207w_motor_pdata *msPlatData;
	struct regulator *regulator;
	struct work_struct trigger_init;
	struct mutex lock;
	struct timespec64 last_motor_off;
	struct dentry *debug_d;
	int en_gpio;
	__u16 overdrive;
	__u16 intensity;
	__u16 fifo_intensity;
	__u16 frequency;
	unsigned char mnDeviceID;
	const char *fifo_data_addr;
	int fifo_data_size;
	bool running;
	bool is_suspend;

	struct sec_vibrator_drvdata sec_vib_ddata;
	/* using ff_runble effect */
	/* struct ff_rumble_effect {
	 __u16 strong_magnitude; // strong_magnitude[15:15] = 0 (Reserved)
				 // strong_magnitude[14:14]
				 //   = Overdrive : 0 (Off) or 1 (On)
				 // strong_magnitude[13:0]
				 //   = Intensity Value : 0 (Stop)
				 //     or 1 ~ 10000 (Intensity)
	 __u16 weak_magnitude;	 // weak_magnitude[15:13]
				 //   = Intensity Level : 1 ~ 5
				 // weak_magnitude[12:0]
				 //   = Frequency in 0.1Hz :
				 //     0 (Use default frequency)
				 //     1 ~ 8191 (0.1 ~ 819.1 Hz)
	}; */
};

int ist207w_motor_reset_handler(void);
bool ist207w_motor_is_running(void);

#endif
