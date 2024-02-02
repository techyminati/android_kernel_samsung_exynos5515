/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * YooMi Tak <youmi.tak@samsung.com>
 *
 */

#ifndef __SSP_COMM_H__
#define __SSP_COMM_H__


#include "ssp.h"

int ssp_send_status(struct ssp_data *data, char command);
int enable_sensor(struct ssp_data *data, unsigned int type, u8 *buf, int buf_len);
int disable_sensor(struct ssp_data *data, unsigned int type, u8 *buf, int buf_len);

#endif /* __SSP_COMM_H__ */
