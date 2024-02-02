/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"

/*************************************************************************/
/* SSP Kernel -> HAL input event function                                */
/*************************************************************************/

#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR

#define SSP_WHEEL_NAME		"detent_bezel"

/* bezel */
#define BZ_CW				1
#define BZ_NA				0
#define BZ_CC				-1

void report_hallic_data(struct ssp_data *data, struct sensor_value *hidata)
{
	if (hidata->direction == 0) {
		
		return;
	} else {
		data->buf[DIGITAL_HALL_EVENT_SENSOR].direction = hidata->direction;

		input_report_rel(data->hallic_input_dev, REL_WHEEL, hidata->direction);
		input_sync(data->hallic_input_dev);

		data->wheel_moved = true;
		data->uWheelCnt += 1;

		ssp_wake_lock_timeout(data->ssp_wake_lock, msecs_to_jiffies(600));// 500

		pr_info("[SSP]%s,[%d][%d]-[%d:%d]\n", __func__, hidata->direction, hidata->interrupt, data->irq_state, data->irq_cnt);
	}
}
#endif

void remove_event_symlink(struct ssp_data *data)
{
	if (data->hallic_input_dev)
		sensors_remove_symlink(data->hallic_input_dev);
}

int initialize_event_symlink(struct ssp_data *data)
{
	int iRet = 0;

	if (data->hallic_input_dev) {
		iRet = sensors_create_symlink(data->hallic_input_dev);
		if (iRet < 0)
			goto err_sysfs_create_link;
	}
	return SUCCESS;

err_sysfs_create_link:
	remove_event_symlink(data);
	pr_err("[SSP]: %s - could not create event symlink\n", __func__);

	return FAIL;
}

void remove_input_dev(struct ssp_data *data)
{
	if (data->hallic_input_dev) {
		input_unregister_device(data->hallic_input_dev);
		pr_err("[SSP]: %s, unregister hall ic input device\n", __func__);
	}
	if (data->motion_input_dev) {
		input_unregister_device(data->motion_input_dev);
		pr_err("[SSP]: %s, unregister motion input device\n", __func__);
	}
}

int initialize_input_dev(struct ssp_data *data)
{
	int iRet = 0;

#ifdef CONFIG_SENSORS_SSP_HALLIC_SENSOR
	data->hallic_input_dev = input_allocate_device();
	if (data->hallic_input_dev == NULL)
		goto err_initialize_input_dev;

	__set_bit(EV_REL, data->hallic_input_dev->evbit);
	__set_bit(EV_KEY, data->hallic_input_dev->evbit);
	//__set_bit(REL_X, data->hallic_input_dev->relbit);
	//__set_bit(REL_Y, data->hallic_input_dev->relbit);
	__set_bit(BTN_LEFT, data->hallic_input_dev->keybit);
	__set_bit(REL_WHEEL, data->hallic_input_dev->relbit);

	//input_set_capability(data->hallic_input_dev, EV_REL, REL_X);
	//input_set_capability(data->hallic_input_dev, EV_REL, REL_Y);
	input_set_capability(data->hallic_input_dev, EV_REL, REL_WHEEL);

	data->hallic_input_dev->name = SSP_WHEEL_NAME;
	data->hallic_input_dev->id.bustype = BUS_VIRTUAL;

	iRet = input_register_device(data->hallic_input_dev);
	if (iRet < 0) {
		input_free_device(data->hallic_input_dev);
		goto err_initialize_input_dev;
	}
	input_set_drvdata(data->hallic_input_dev, data);
#endif

#ifdef CONFIG_SENSORS_SSP_LPM_MOTION
	data->motion_input_dev = input_allocate_device();
	if (data->motion_input_dev == NULL)
		goto err_initialize_input_dev;

	data->motion_input_dev->name = "LPM_MOTION";
	input_set_capability(data->motion_input_dev, EV_KEY, KEY_HOMEPAGE);
	input_set_capability(data->motion_input_dev, EV_ABS, ABS_X);
	input_set_abs_params(data->motion_input_dev, ABS_X, 0, 3, 0, 0);

	iRet = input_register_device(data->motion_input_dev);
	if (iRet < 0) {
		input_free_device(data->motion_input_dev);
		goto err_initialize_input_dev;
	}
	input_set_drvdata(data->motion_input_dev, data);
#endif

	return SUCCESS;

err_initialize_input_dev:
	remove_input_dev(data);

	return ERROR;
}
