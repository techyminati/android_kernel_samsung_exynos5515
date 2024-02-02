/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * notifer and sysevent interface to communicate with other drivers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef ___SAMSUNG_LPD_NOTI_H__
#define ___SAMSUNG_LPD_NOTI_H__

#include <linux/types.h>
#include <linux/notifier.h>
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
#include <soc/samsung/sysevent.h>
#endif

struct lpd_notifier {
	struct notifier_block itmon;
	struct notifier_block panic;
	struct notifier_block begin;
	struct notifier_block end;
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
	struct sysevent_desc chub_desc;
	struct sysevent_device *chub_dev;
#endif
};

enum lpd_notifier_steps {
	LPD_NOTIFIER_STEPS_BEGIN,
	LPD_NOTIFIER_STEPS_END,
};

#endif
