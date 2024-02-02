/*
 *  Copyright (C) 2018, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef __SSP_SENSORLIST_H__
#define __SSP_SENSORLIST_H__

#include "ssp.h"

/* Sensors's reporting mode */
#define REPORT_MODE_CONTINUOUS          0
#define REPORT_MODE_ON_CHANGE           1
#define REPORT_MODE_SPECIAL             2
#define REPORT_MODE_UNKNOWN             3

#define SENSOR_INFO_UNKNOWN                                     {"", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_ACCELEROMETER                               {"accelerometer_sensor", true, REPORT_MODE_CONTINUOUS, 6, 6}
#define SENSOR_INFO_GYRO                                        {"gyro_sensor", true, REPORT_MODE_CONTINUOUS, 6, 6}
#define SENSOR_INFO_ACCELEROMETER_32G                           {"accelerometer32g_sensor", true, REPORT_MODE_CONTINUOUS, 6, 6}
#define SENSOR_INFO_DIGITAL_HALL_RAW                            {"digital_hall_raw", true, REPORT_MODE_CONTINUOUS, 2, 2}
#define SENSOR_INFO_DIGITAL_HALL_EVENT                        	{"digital_hall_event", false, REPORT_MODE_UNKNOWN, 4, 4}
#define SENSOR_INFO_PRESSURE                                    {"pressure_sensor", true, REPORT_MODE_CONTINUOUS, 6, 12}
#define SENSOR_INFO_GESTURE                                     {"gesture_sensor", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_DIGITAL_HALL_SUB_RAW                       	{"digital_hall_sub_raw", true, REPORT_MODE_CONTINUOUS, 2, 2}
#define SENSOR_INFO_SENSOR8                            		    {"sensor8", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_LIGHT                                       {"light_sensor", true, REPORT_MODE_ON_CHANGE, 2, 4}
#define SENSOR_INFO_AUTO_BRIGHTNESS                             {"auto_brightness_sensor", true, REPORT_MODE_CONTINUOUS, 5, 5}
#define SENSOR_INFO_ORIENTATION                                 {"orientation_sensor", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_ROTATION_VECTOR                             {"rotation_vector_sensor", true, REPORT_MODE_CONTINUOUS, 17, 17}
#define SENSOR_INFO_GAME_ROTATION_VECTOR                        {"game_rotation_vector_sensor", true, REPORT_MODE_CONTINUOUS, 17, 17}
#define SENSOR_INFO_GYRO_UNCALIBRATED                           {"uncal_gyro_sensor", true, REPORT_MODE_CONTINUOUS, 12, 12}
#define SENSOR_INFO_LINEAR_ACCEL                                {"linear_accel_sensor", true, REPORT_MODE_CONTINUOUS, 6, 6}
#define SENSOR_INFO_SENSOR16                                    {"sensor16", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_HRM_ECG_LIB                                 {"hrm_ecg_lib_sensor", true, REPORT_MODE_CONTINUOUS, 4*HRM_ECG_LIB_DATA_SIZE, 4*HRM_ECG_LIB_DATA_SIZE}
#define SENSOR_INFO_HRM_RAW                                     {"hrm_raw_sensor", true, REPORT_MODE_ON_CHANGE, 64, 64}
#define SENSOR_INFO_HRM_RAW_FAC                                 {"hrm_raw_fac_sensor", false, REPORT_MODE_ON_CHANGE, 4*HRM_EOL_DATA_SIZE, 4*HRM_EOL_DATA_SIZE}
#define SENSOR_INFO_HRM_LIB                                     {"hrm_lib_sensor", true, REPORT_MODE_ON_CHANGE, 8, 8}
#define SENSOR_INFO_HRM_RAW_FAC2                                {"hrm_raw_fac2_sensor", false, REPORT_MODE_ON_CHANGE, 4*HRM_XTALK2_DATA_SIZE, 4*HRM_XTALK2_DATA_SIZE}
#define SENSOR_INFO_HRM_BIA_RAW                                 {"hrm_bia_raw_sensor", true, REPORT_MODE_CONTINUOUS, 4*HRM_BIA_RAW_DATA_SIZE, 4*HRM_BIA_RAW_DATA_SIZE}
#define SENSOR_INFO_TEMPERATURE                                 {"temperature_sensor", true, REPORT_MODE_CONTINUOUS, TEMPERATURE_DATA_SIZE, TEMPERATURE_DATA_SIZE}
#ifdef CONFIG_SENSORS_SSP_MAGNETIC_32BIT
#define SENSOR_INFO_GEOMAGNETIC_UNCALIB                         {"geomagnetic_uncalib_sensor", true, REPORT_MODE_CONTINUOUS, 25, 25}
#else
#define SENSOR_INFO_GEOMAGNETIC_UNCALIB                         {"geomagnetic_uncalib_sensor", true, REPORT_MODE_CONTINUOUS, 13, 13}
#endif
#define SENSOR_INFO_GEOMAGNETIC_CALIB                           {"geomagnetic_calib_sensor", true, REPORT_MODE_CONTINUOUS, 8, 8}
#define SENSOR_INFO_GEOMAGNETIC_POWER                           {"geomagnetic_power_sensor", true, REPORT_MODE_CONTINUOUS, 6, 6}
#define SENSOR_INFO_SENSOR27                                    {"sensor27", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_SENSOR28                                    {"sensor28", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_SENSOR29                                    {"sensor29", false, REPORT_MODE_UNKNOWN, 0, 0}

#define SENSOR_INFO_GRIP                                        {"grip_sensor", false, REPORT_MODE_UNKNOWN, 9, 9}
#define SENSOR_INFO_GRIP_RAW                                    {"grip_raw_sensor", false, REPORT_MODE_UNKNOWN, 0, 0}
#define SENSOR_INFO_SCONTEXT                                    {"scontext_iio", true, REPORT_MODE_CONTINUOUS, 0, SCONTEXT_DATA_SIZE}
#endif

