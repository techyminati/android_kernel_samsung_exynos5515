/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS Power mode
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>

#include <soc/samsung/exynos-pm.h>
#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-powermode.h>

#ifdef CONFIG_CPU_IDLE
static DEFINE_RWLOCK(exynos_pm_notifier_lock);
static RAW_NOTIFIER_HEAD(exynos_pm_notifier_chain);

int exynos_pm_register_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	write_lock_irqsave(&exynos_pm_notifier_lock, flags);
	ret = raw_notifier_chain_register(&exynos_pm_notifier_chain, nb);
	write_unlock_irqrestore(&exynos_pm_notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_pm_register_notifier);

int exynos_pm_unregister_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	write_lock_irqsave(&exynos_pm_notifier_lock, flags);
	ret = raw_notifier_chain_unregister(&exynos_pm_notifier_chain, nb);
	write_unlock_irqrestore(&exynos_pm_notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_pm_unregister_notifier);

static int __exynos_pm_notify(enum exynos_pm_event event, int nr_to_call, int *nr_calls)
{
	int ret;

	ret = __raw_notifier_call_chain(&exynos_pm_notifier_chain, event, NULL,
		nr_to_call, nr_calls);

	return notifier_to_errno(ret);
}

int exynos_pm_notify(enum exynos_pm_event event)
{
	int nr_calls;
	int ret = 0;

	read_lock(&exynos_pm_notifier_lock);
	ret = __exynos_pm_notify(event, -1, &nr_calls);
	read_unlock(&exynos_pm_notifier_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_pm_notify);
#endif /* CONFIG_CPU_IDLE */

struct exynos_powermode_info {
	/*
	 * While system boot, wakeup_mask and idle_ip_mask is intialized with
	 * device tree. These are used by system power mode.
	 */
	unsigned int	num_wakeup_mask;
	unsigned int	*wakeup_mask_offset;
	unsigned int	*wakeup_mask[NUM_SYS_POWERDOWN];
};

static struct exynos_powermode_info *pm_config;

/******************************************************************************
 *                              System power mode                             *
 ******************************************************************************/
#define PMU_EINT_WAKEUP_MASK	0x60C
static void exynos_set_wakeupmask(enum sys_powerdown mode)
{
	int i;
	exynos_set_eint_wakeupmask();

	for (i = 0; i < pm_config->num_wakeup_mask; i++)
		exynos_pmu_write(pm_config->wakeup_mask_offset[i],
				pm_config->wakeup_mask[mode][i]);
}
int exynos_prepare_sys_powerdown(enum sys_powerdown mode)
{
	int ret;

	exynos_set_wakeupmask(mode);

	ret = cal_pm_enter(mode);
	if (ret) {
		pr_err("CAL Fail to set powermode\n");
		goto out;
	}

	switch (mode) {
	case SYS_SICD:
		exynos_pm_notify(SICD_ENTER);
		break;
	default:
		break;
	}

out:
	return ret;
}

void exynos_wakeup_sys_powerdown(enum sys_powerdown mode, bool early_wakeup)
{
	if (early_wakeup)
		cal_pm_earlywakeup(mode);
	else
		cal_pm_exit(mode);

	switch (mode) {
	case SYS_SICD:
		exynos_pm_notify(SICD_EXIT);
		break;
	default:
		break;
	}
}

/******************************************************************************
 *                               Extern function                              *
 ******************************************************************************/
int exynos_rtc_wakeup(void)
{
#define WAKEUP_MASK_RTC_TICK	BIT(2)
#define WAKEUP_MASK_RTC_ALARM	BIT(1)
	unsigned int sleep_mask = pm_config->wakeup_mask[SYS_SLEEP][0];

	if (!(sleep_mask & WAKEUP_MASK_RTC_ALARM) ||
			!(sleep_mask & WAKEUP_MASK_RTC_TICK))
		return 0;

	return -ENXIO;
}

/******************************************************************************
 *                            Driver initialization                           *
 ******************************************************************************/
static int alloc_wakeup_mask(int num_wakeup_mask)
{
	unsigned int mode;

	pm_config->wakeup_mask_offset = kzalloc(sizeof(unsigned int) * num_wakeup_mask,
			GFP_KERNEL);
	if (!pm_config->wakeup_mask_offset)
		return -ENOMEM;

	for_each_syspwr_mode(mode) {
		pm_config->wakeup_mask[mode] = kzalloc(sizeof(unsigned int) * num_wakeup_mask,
				GFP_KERNEL);

		if (!pm_config->wakeup_mask[mode])
			goto free_reg_offset;
	}

	return 0;

free_reg_offset:
	for_each_syspwr_mode(mode)
		if (pm_config->wakeup_mask[mode])
			kfree(pm_config->wakeup_mask[mode]);

	kfree(pm_config->wakeup_mask_offset);

	return -ENOMEM;
}

static int parsing_dt_wakeup_mask(struct device_node *np)
{
	int ret;
	struct device_node *root, *child;
	unsigned int mode, mask_index = 0;

	root = of_find_node_by_name(np, "wakeup-masks");
	pm_config->num_wakeup_mask = of_get_child_count(root);

	ret = alloc_wakeup_mask(pm_config->num_wakeup_mask);
	if (ret)
		return ret;

	for_each_child_of_node(root, child) {
		for_each_syspwr_mode(mode) {
			ret = of_property_read_u32_index(child, "mask",
				mode, &pm_config->wakeup_mask[mode][mask_index]);
			if (ret)
				return ret;
		}

		ret = of_property_read_u32(child, "reg-offset",
				&pm_config->wakeup_mask_offset[mask_index]);
		if (ret)
			return ret;

		mask_index++;
	}

	return 0;
}

static int __init exynos_powermode_init(void)
{
	struct device_node *np;
	int ret;

	pm_config = kzalloc(sizeof(struct exynos_powermode_info), GFP_KERNEL);
	if (pm_config == NULL) {
		pr_err("%s: failed to allocate exynos_powermode_info\n", __func__);
		return -ENOMEM;
	}

	np = of_find_node_by_name(NULL, "exynos-powermode");
	ret = parsing_dt_wakeup_mask(np);
	if (ret)
		pr_warn("Fail to initialize the wakeup mask with err = %d\n", ret);

	return 0;
}
arch_initcall(exynos_powermode_init);

