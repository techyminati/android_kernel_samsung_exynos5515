/* soc/samsung/exynos-lpd.h
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS - Header file for exynos low power display support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_LPD_H
#define __EXYNOS_LPD_H

#if defined(CONFIG_EXYNOS_LPD)
#include <linux/notifier.h>

/*
 [LPD Notifier call chain]
 - LPD(BEGIN) -> PPMU(START) -> CHUB -> PPMU(STOP) -> LPD(END)
 - Assigned priotiry value for backward compatibility, will be removed later
 - DO NOT RETURN NOTIFY_STOP or NOTIFY_BAD in call chain fn()
*/

enum exynos_lpd_notifier_action {
	LPD_NOTIFIER_CONFIG, 	// To set panel info
	LPD_NOTIFIER_START_PRE, // For PM control
	LPD_NOTIFIER_START,
	LPD_NOTIFIER_STOP,
	LPD_NOTIFIER_STOP_POST, // For PM control
};

enum exynos_lpd_notifier_priority {
	LPD_NOTIFIER_PRIORITY_LPD_END = 0, 		// dedicated to LPD drv
	LPD_NOTIFIER_PRIORITY_PPMU_STOP = 3,	// PPMU Stop
	LPD_NOTIFIER_PRIORITY_CHUB = 5,
	LPD_NOTIFIER_PRIORITY_PPMU_START = 7,	// PPMU Start
	LPD_NOTIFIER_PRIORITY_LPD_BEGIN = 10,	// dedicated to LPD drv
	LPD_NOTIFIER_PRIORITY_MAX = LPD_NOTIFIER_PRIORITY_LPD_BEGIN,
};


#define DPU_STATE_OFF		0
#define	DPU_STATE_ON		1
#define DPU_STATE_DOZE		2
#define DPU_STATE_DOZE_SUSPEND	3

extern int lpd_notifier_register(struct notifier_block *nb);
extern int lpd_notifier_unregister(struct notifier_block *nb);
extern int lpd_notifier_call(u32 action, void *data);
extern bool disp_is_on(void);
#else /* !CONFIG_EXYNOS_LPD */
static inline bool disp_is_on(void)
{
	return false;
}
#endif

#endif /* __EXYNOS_LPD_H */
