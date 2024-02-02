/*
 *  Copyright (C) 2021, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef __SSP_MOTOR_H__
#define __SSP_MOTOR_H__

#define VIB_EVENT_PLAY	(1)
#define VIB_EVENT_STOP	(0)

void initialize_motor_callback(struct ssp_data *data);

#endif // __SSP_MOTOR_H__