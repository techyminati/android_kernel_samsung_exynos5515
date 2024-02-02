/*
 * sec_battery_eng.h
 * Samsung Mobile Sec Battery ENG Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 *
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

#ifndef __SEC_BATTERY_ENG_H
#define __SEC_BATTERY_ENG_H __FILE__

struct device;
struct sec_battery_info;
struct sec_bat_eng_data;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
extern struct sec_bat_eng_data *sec_bat_init_eng_data(struct device *dev);
extern bool sec_bat_set_charge_test(struct sec_battery_info *battery);
#else
inline struct sec_bat_eng_data *sec_bat_init_eng_data(struct device *dev) { return NULL; }
inline bool sec_bat_set_charge_test(struct sec_battery_info *battery) { return false; }
#endif

#endif /* __SEC_BATTERY_ENG_H */
