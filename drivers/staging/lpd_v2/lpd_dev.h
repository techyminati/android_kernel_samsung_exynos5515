/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * character device driver for User Application
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef ___SAMSUNG_LPD_DEV_H__
#define ___SAMSUNG_LPD_DEV_H__
#include <linux/types.h>
#include <linux/cdev.h>

struct lpd_chardev_info {
	struct cdev cdev;
	dev_t devid;
	struct class *class;
	struct device *csdev;
};


#define LPD_IOCTL_SET_META_DATA			_IOW('l', 202, struct lpd_meta_info)
#define LPD_IOCTL_SET_SENSOR_DATA		_IOW('l', 203, struct lpd_sensor_data)

#define LPD_IOCTL_PREPARE_UPDATE_META_DATA	_IOW('l', 101, struct lpd_meta_info)
#define LPD_IOCTL_UPDATE_META_DATA		_IOW('l', 102, struct lpd_meta_info)


#define CDEV_NAME				"exynos_lpd"

#endif
