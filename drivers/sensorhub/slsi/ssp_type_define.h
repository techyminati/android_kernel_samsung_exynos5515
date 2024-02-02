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

#ifndef __SSP_TYPE_DEFINE_H__
#define __SSP_TYPE_DEFINE_H__

/* LIBRARY_TYPE */
//typedef enum _SH_LIBRARY_TYPE {
#define SH_LIBRARY_GPS_SENSOR						0
#define SH_LIBRARY_GPS_PREFIX						1  // GPS Pre fix library
#define SH_LIBRARY_GPS_AR_ADVANCE					2
#define SH_LIBRARY_PEDOMETER						3

#define SH_LIBRARY_AUTOSESSION_EXERCISE_MONITOR		4
#define SH_LIBRARY_GPS_BIG_DATA						5
#define SH_LIBRARY_STAIR_TRACKER					6
#define SH_LIBRARY_AUTOROTATION						7

#define SH_LIBRARY_MOVE_DETECTION					8
#define SH_LIBRARY_ASM								9
#define SH_LIBRARY_SLOCATION						10
#define SH_LIBRARY_ELEVATOR_TRACKER					11

#define SH_LIBRARY_LOOPBACK							12
#define SH_LIBRARY_BARO_INDICATOR					13
#define SH_LIBRARY_BARO_ALARM						14
#define SH_LIBRARY_SWIMMING							15

#define SH_LIBRARY_AIRPLANE							16
#define SH_LIBRARY_OUTDOOR_SWIMMING					17
#define SH_LIBRARY_AUTO_SWIMMING					18
#define SH_LIBRARY_WRIST_UP							19

#define SH_LIBRARY_BEZELMAG							20
#define SH_LIBRARY_FALL_DETECTION					21
#define SH_LIBRARY_MOVEMENT_ALERT					22
#define SH_LIBRARY_CONTINUOUS_SPO2					23

#define SH_LIBRARY_AOD_GEAR							24
#define SH_LIBRARY_BLOODPRESSURE					25
#define SH_LIBRARY_ACTIVITY_TRACKER					26
#define SH_LIBRARY_TIME_SYNC_ENGINE					27

#define SH_LIBRARY_PPG_DATA							28
#define SH_LIBRARY_CALORIE_CORE						29
#define SH_LIBRARY_EXERCISE_CALORIE_PROVIDER		30
#define SH_LIBRARY_AFE_SENSOR_LOGGING				31

#define SH_LIBRARY_MR2_SIGNIFICANT_MOTION			32
#define SH_LIBRARY_MR2_UNCALIBRATED_GYRO			33
#define SH_LIBRARY_MR2_ROTATION_VECTOR				34
#define SH_LIBRARY_MM_AFE							35

#define SH_LIBRARY_HRM								36
#define SH_LIBRARY_SLEEPMONITOR						37
#define SH_LIBRARY_SLEEP_DETECT						38
#define SH_LIBRARY_SLEEP_ANALYZER					39

#define SH_LIBRARY_VO2_MONITOR						40
#define SH_LIBRARY_SPO2								41
#define SH_LIBRARY_SENSOR_REG						42
#define SH_LIBRARY_HR_EXTERNAL						43

#define SH_LIBRARY_HEALTHYPACE						44
#define SH_LIBRARY_EXERCISE							45
#define SH_LIBRARY_HEARTRATE						46
#define SH_LIBRARY_GPS_BATCH						47

#define SH_LIBRARY_INACTIVETIMER					48
#define SH_LIBRARY_PEDOCALIBRATION					49
#define SH_LIBRARY_AUTO_BRIGHTNESS					50
#define SH_LIBRARY_WRISTDETECT						51

#define SH_LIBRARY_RUNNING_DYNAMICS					52
#define SH_LIBRARY_DAILY_HEARTRATE					53
#define SH_LIBRARY_WEARONOFF_MONITOR				54
#define SH_LIBRARY_CYCLE_MONITOR					55

#define SH_LIBRARY_NOMOVEMENT_DETECTION				56
#define SH_LIBRARY_ACTIVITY_LEVEL_MONITOR			57
#define SH_LIBRARY_WRIST_DOWN						58
#define SH_LIBRARY_AUTO_HAND_WASH					59

#define SH_LIBRARY_STRESS_MONITOR					60
#define SH_LIBRARY_GYROCALIBRATION					61
#define SH_LIBRARY_DEBUG_MSG						62
#define SH_LIBRARY_DYNAMIC_WORKOUT					63

#define SH_LIBRARY_RV								64
#define SH_LIBRARY_GRV								SH_LIBRARY_RV // 64
#define SH_LIBRARY_LINEAR_ACCEL						65
#define SH_LIBRARY_SIOP								66

#define SH_LIBRARY_MAX								(SH_LIBRARY_SIOP+1)


#endif

