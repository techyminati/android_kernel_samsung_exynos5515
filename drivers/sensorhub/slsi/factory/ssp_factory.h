/*
 *  Copyright (C) 2015, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef __SSP_FACTORY_H__
#define __SSP_FACTORY_H__

#include "../ssp.h"

struct ssp_data;

void initialize_bmi168_accel_factorytest(struct ssp_data *);
void initialize_bmi168_gyro_factorytest(struct ssp_data *);
void initialize_k6ds_accel_factorytest(struct ssp_data *);
void initialize_k6ds_gyro_factorytest(struct ssp_data *);
void initialize_lsm6dsl_accel_factorytest(struct ssp_data *);
void initialize_lsm6dsl_accel1_factorytest(struct ssp_data *);
void initialize_lsm6dsl_gyro_factorytest(struct ssp_data *);
void initialize_yas537_magnetic_factorytest(struct ssp_data *);
void initialize_ak09918c_magnetic_factorytest(struct ssp_data *);
void initialize_lps25h_pressure_factorytest(struct ssp_data *);
void initialize_opt3007_light_factorytest(struct ssp_data *);
void initialize_tsl2584_light_factorytest(struct ssp_data *);
void initialize_cm3323_light_factorytest(struct ssp_data *);
void initialize_shtw1_skintemp_factorytest(struct ssp_data *);
void initialize_ad45251_hrm_factorytest(struct ssp_data *);
void initialize_pps960_hrm_factorytest(struct ssp_data *);
void initialize_pps960_ecg_factorytest(struct ssp_data *);
void initialize_pps960_bia_factorytest(struct ssp_data *);
void initialize_max86902_hrm_factorytest(struct ssp_data *);
void initialize_max86902_uv_factorytest(struct ssp_data *);
void initialize_hm121_gsr_factorytest(struct ssp_data *);
void initialize_ads1292_ecg_factorytest(struct ssp_data *);
void initialize_sx9310_grip_factorytest(struct ssp_data *);
void initialize_ak09973d_digital_hall_factorytest(struct ssp_data *);
void initialize_thermistor_factorytest(struct ssp_data *);
void initialize_mlx90632_temperature_factorytest(struct ssp_data *);

void remove_bmi168_accel_factorytest(struct ssp_data *);
void remove_bmi168_gyro_factorytest(struct ssp_data *);
void remove_k6ds_accel_factorytest(struct ssp_data *);
void remove_k6ds_gyro_factorytest(struct ssp_data *);
void remove_lsm6dsl_accel_factorytest(struct ssp_data *);
void remove_lsm6dsl_accel1_factorytest(struct ssp_data *);
void remove_lsm6dsl_gyro_factorytest(struct ssp_data *);
void remove_yas537_magnetic_factorytest(struct ssp_data *);
void remove_ak09918c_magnetic_factorytest(struct ssp_data *);
void remove_lps25h_pressure_factorytest(struct ssp_data *);
void remove_opt3007_light_factorytest(struct ssp_data *);
void remove_tsl2584_light_factorytest(struct ssp_data *);
void remove_cm3323_light_factorytest(struct ssp_data *);
void remove_shtw1_skintemp_factorytest(struct ssp_data *);
void remove_ad45251_hrm_factorytest(struct ssp_data *);
void remove_pps960_hrm_factorytest(struct ssp_data *);
void remove_pps960_ecg_factorytest(struct ssp_data *);
void remove_pps960_bia_factorytest(struct ssp_data *);
void remove_max86902_hrm_factorytest(struct ssp_data *);
void remove_max86902_uv_factorytest(struct ssp_data *);
void remove_hm121_gsr_factorytest(struct ssp_data *);
void remove_ads1292_ecg_factorytest(struct ssp_data *);
void remove_sx9310_grip_factorytest(struct ssp_data *);
void remove_ak09973d_digital_hall_factorytest(struct ssp_data *);
void remove_thremistor_factorytest(struct ssp_data *);
void remove_mlx90632_temperature_factorytest(struct ssp_data *);


int bmi168_accel_open_calibration(struct ssp_data *);
int bmi168_gyro_open_calibration(struct ssp_data *);
int k6ds_accel_open_calibration(struct ssp_data *);
int k6ds_gyro_open_calibration(struct ssp_data *);
int lsm6dsl_accel_open_calibration(struct ssp_data *);
int lsm6dsl_accel_open_calibration1(struct ssp_data *);
int lsm6dsl_gyro_open_calibration(struct ssp_data *);
int lps25h_pressure_open_calibration(struct ssp_data *);
int ad45251_hrm_open_calibration(struct ssp_data *);
int pps960_hrm_open_calibration(struct ssp_data *);
int read_bia_nv_param(struct ssp_data *data);
int pps960_set_bia_calibration(struct ssp_data *data);
int ak09918c_mag_open_calibration(struct ssp_data *);

int bmi168_set_gyro_cal(struct ssp_data *);
int bmi168_set_accel_cal(struct ssp_data *);
int k6ds_set_gyro_cal(struct ssp_data *);
int k6ds_set_accel_cal(struct ssp_data *);
int lsm6dsl_set_gyro_cal(struct ssp_data *);
int lsm6dsl_set_accel_cal(struct ssp_data *);
int lsm6dsl_set_accel_cal1(struct ssp_data *);
int lps25h_set_pressure_cal(struct ssp_data *);
int ad45251_set_hrm_calibration(struct ssp_data *);
int pps960_set_hrm_calibration(struct ssp_data *);
int ak09918c_set_mag_cal(struct ssp_data *);
int ak09918c_save_mag_cal_data(struct ssp_data *);

int initialize_yas537_magnetic_sensor(struct ssp_data *);
int initialize_magnetic_sensor(struct ssp_data *);
int mag_open_calibration(struct ssp_data *);
int set_mag_cal(struct ssp_data *);
int save_mag_cal_data(struct ssp_data *);
int set_pdc_matrix(struct ssp_data *);

s32 light_get_lux(struct ssp_data *data, uint16_t, uint8_t);

#endif
