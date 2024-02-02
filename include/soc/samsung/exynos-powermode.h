/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - PMU(Power Management Unit) support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EXYNOS_POWERMODE_H
#define __EXYNOS_POWERMODE_H __FILE__
#include <soc/samsung/cal-if.h>

/*
 * Event codes for PM states
 */
enum exynos_pm_event {
	/* CPU is entering the LPA state */
	LPA_ENTER,

	/* CPU failed to enter the LPA state */
	LPA_ENTER_FAIL,

	/* CPU is exiting the LPA state */
	LPA_EXIT,

	/* CPU is entering the SICD/SICD_AUD state */
	SICD_ENTER,
	SICD_AUD_ENTER,

	/* CPU is exiting the SICD/SICD_AUD state */
	SICD_EXIT,
	SICD_AUD_EXIT,
};

#ifdef CONFIG_CPU_IDLE
int exynos_pm_register_notifier(struct notifier_block *nb);
int exynos_pm_unregister_notifier(struct notifier_block *nb);
int exynos_pm_notify(enum exynos_pm_event event);
#else
static inline int exynos_pm_register_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int exynos_pm_unregister_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int exynos_pm_notify(enum exynos_pm_event event)
{
	return 0;
}
#endif
extern int exynos_prepare_sys_powerdown(enum sys_powerdown mode);
extern void exynos_wakeup_sys_powerdown(enum sys_powerdown mode, bool early_wakeup);
extern void exynos_prepare_cp_call(void);
extern void exynos_wakeup_cp_call(bool early_wakeup);
extern int exynos_rtc_wakeup(void);

static char *sys_powerdown_str[NUM_SYS_POWERDOWN] = {
	"SICD",
	"SICD_CPD",
	"AFTR",
	"STOP",
	"LPD",
	"LPA",
	"ALPA",
	"DSTOP",
	"SLEEP",
	"SLEEP_VTS_ON",
	"SLEEP_AUD_ON",
	"FAPO",
};

static inline char* get_sys_powerdown_str(int mode)
{
	return sys_powerdown_str[mode];
}

#define for_each_syspwr_mode(mode)				\
	for ((mode) = 0; (mode) < NUM_SYS_POWERDOWN; (mode)++)

/**
 * external driver APIs
 */
#ifdef CONFIG_SERIAL_SAMSUNG
extern void s3c24xx_serial_fifo_wait(void);
#else
static inline void s3c24xx_serial_fifo_wait(void) { }
#endif

#ifdef CONFIG_PINCTRL_EXYNOS
extern u64 exynos_get_eint_wake_mask(void);
#else
static inline u64 exynos_get_eint_wake_mask(void) { return 0xffffffffL; }
#endif

#endif /* __EXYNOS_POWERMODE_H */
