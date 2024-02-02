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

#ifndef __SSP_LPM_H__
#define __SSP_LPM_H__

int ssp_parse_motion(struct ssp_data *data, char *dataframe, int start, int end);
void set_lpm_mode(struct ssp_data *data);
int initialize_lpm_motion(struct ssp_data *data);

#endif // __SSP_LPM_H__