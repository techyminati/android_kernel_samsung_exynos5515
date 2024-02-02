/*
 * Platform Dependent file for Samsung Exynos
 *
 * Portions of this code are copyright (c) 2021 Cypress Semiconductor Corporation
 *
 * Copyright (C) 1999-2017, Broadcom Corporation
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: dhd_custom_exynos.c 700280 2017-05-18 11:09:25Z $
 */
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/unistd.h>
#include <linux/bug.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/wlan_plat.h>
#if defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810) || \
	defined(CONFIG_SOC_EXYNOS9820)
#include <linux/exynos-pci-ctrl.h>
#endif /* CONFIG_SOC_EXYNOS8895 || CONFIG_SOC_EXYNOS9810 || CONFIG_SOC_EXYNOS9820 */

#if defined(CONFIG_64BIT)
#include <asm-generic/gpio.h>
#else
#if !defined(CONFIG_ARCH_SWA100) && !defined(CONFIG_MACH_UNIVERSAL7580)
#include <mach/gpio.h>
#endif /* !CONFIG_ARCH_SWA100 && !CONFIG_MACH_UNIVERSAL7580 */
#endif /* CONFIG_64BIT */

#if defined(CONFIG_MACH_UNIVERSAL7580) || defined(CONFIG_MACH_UNIVERSAL5430) || \
	defined(CONFIG_MACH_UNIVERSAL5422)
#include <mach/irqs.h>
#endif /* CONFIG_MACH_UNIVERSAL7580 || CONFIG_MACH_UNIVERSAL5430 || CONFIG_MACH_UNIVERSAL5422 */
#if defined(CONFIG_SEC_SYSFS)
#include <linux/sec_sysfs.h>
#elif defined(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif /* CONFIG_SEC_SYSFS */

#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_SOC_S5E5515)
#define PINCTL_DELAY 150
#endif /* CONFIG_MACH_A7LTE || CONFIG_SOC_S5E5515 */
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
extern int ifx_init_wlan_mem(void);
extern void *ifx_wlan_mem_prealloc(int section, unsigned long size);
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

#define WIFI_TURNON_DELAY	200
static int wlan_pwr_on = -1;

#ifdef CONFIG_BCMDHD_OOB_HOST_WAKE
static int wlan_host_wake_irq = 0;
EXPORT_SYMBOL(wlan_host_wake_irq);
static unsigned int wlan_host_wake_up = -1;
#endif /* CONFIG_BCMDHD_OOB_HOST_WAKE */
#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_SOC_S5E5515)
extern struct device *mmc_dev_for_wlan;
#endif /* CONFIG_MACH_A7LTE || CONFIG_SOC_S5E5515 */

#ifdef CONFIG_BCMDHD_PCIE
#define EXYNOS_PCIE_RC_ONOFF
#endif /* CONFIG_BCMDHD_PCIE */

#ifdef EXYNOS_PCIE_RC_ONOFF
#ifdef CONFIG_MACH_UNIVERSAL5433
#define SAMSUNG_PCIE_CH_NUM
#elif defined(CONFIG_MACH_UNIVERSAL7420) || defined(CONFIG_MACH_EXSOM7420)
#define SAMSUNG_PCIE_CH_NUM 1
#elif defined(CONFIG_SOC_EXYNOS8890)|| defined(CONFIG_SOC_EXYNOS8895) || \
	defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
#define SAMSUNG_PCIE_CH_NUM 0
#endif // endif
#ifdef CONFIG_MACH_UNIVERSAL5433
extern void exynos_pcie_pm_resume(void);
extern void exynos_pcie_pm_suspend(void);
#else
extern void exynos_pcie_pm_resume(int);
extern void exynos_pcie_pm_suspend(int);
#endif /* CONFIG_MACH_UNIVERSAL5433 */
#endif /* EXYNOS_PCIE_RC_ONOFF */

#if (defined(CONFIG_MACH_UNIVERSAL3475) || defined(CONFIG_SOC_EXYNOS7870) || \
	defined(CONFIG_MACH_UNIVERSAL7580) || defined(CONFIG_SOC_S5E5515))
extern struct mmc_host *wlan_mmc;
extern void mmc_ctrl_power(struct mmc_host *host, bool onoff);
#endif /* MACH_UNIVERSAL3475 || SOC_EXYNOS7870 || MACH_UNIVERSAL7580 */

static int
dhd_wlan_power(int onoff)
{
#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_SOC_S5E5515)
	struct pinctrl *pinctrl = NULL;
#endif /* CONFIG_MACH_A7LTE || CONFIG_SOC_S5E5515 */

	printk(KERN_INFO"------------------------------------------------");
	printk(KERN_INFO"------------------------------------------------\n");
	printk(KERN_INFO"%s Enter: power %s\n", __FUNCTION__, onoff ? "on" : "off");

#ifdef EXYNOS_PCIE_RC_ONOFF
	if (!onoff) {
		exynos_pcie_pm_suspend(SAMSUNG_PCIE_CH_NUM);
	}

	if (gpio_direction_output(wlan_pwr_on, onoff)) {
		printk(KERN_ERR "%s failed to control WLAN_REG_ON to %s\n",
			__FUNCTION__, onoff ? "HIGH" : "LOW");
		return -EIO;
	}

	if (onoff) {
#if defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810) || \
	defined(CONFIG_SOC_EXYNOS9820)
		printk(KERN_ERR "%s Disable L1ss EP side\n", __FUNCTION__);
		exynos_pcie_l1ss_ctrl(0, PCIE_L1SS_CTRL_WIFI);
#endif /* CONFIG_SOC_EXYNOS8895 || CONFIG_SOC_EXYNOS9810 || CONFIG_SOC_EXYNOS9820 */
		exynos_pcie_pm_resume(SAMSUNG_PCIE_CH_NUM);
	}
#else
#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_SOC_S5E5515)
	if (onoff) {
		pinctrl = devm_pinctrl_get_select(mmc_dev_for_wlan, "sdio_wifi_on");
		if (IS_ERR(pinctrl))
			printk(KERN_INFO "%s WLAN SDIO GPIO control error\n", __FUNCTION__);
		msleep(PINCTL_DELAY);
	}
#endif /* CONFIG_MACH_A7LTE || CONFIG_SOC_S5E5515 */

	if (gpio_direction_output(wlan_pwr_on, onoff)) {
		printk(KERN_ERR "%s failed to control WLAN_REG_ON to %s\n",
			__FUNCTION__, onoff ? "HIGH" : "LOW");
		return -EIO;
	}

#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_SOC_S5E5515)
	if (!onoff) {
		pinctrl = devm_pinctrl_get_select(mmc_dev_for_wlan, "sdio_wifi_off");
		if (IS_ERR(pinctrl))
			printk(KERN_INFO "%s WLAN SDIO GPIO control error\n", __FUNCTION__);
	}
#endif /* CONFIG_MACH_A7LTE || CONFIG_SOC_S5E5515 */
#if (defined(CONFIG_MACH_UNIVERSAL3475) || defined(CONFIG_SOC_EXYNOS7870) || \
	defined(CONFIG_MACH_UNIVERSAL7580) || defined(CONFIG_SOC_S5E5515))
	if (wlan_mmc)
		mmc_ctrl_power(wlan_mmc, onoff);
#endif /* MACH_UNIVERSAL3475 || SOC_EXYNOS7870 || MACH_UNIVERSAL7580 */
#endif /* EXYNOS_PCIE_RC_ONOFF */
	return 0;
}

static int
dhd_wlan_reset(int onoff)
{
	return 0;
}

#ifndef CONFIG_BCMDHD_PCIE
extern void (*notify_func_callback)(void *dev_id, int state);
extern void *mmc_host_dev;

static int
dhd_wlan_set_carddetect(int val)
{
	pr_err("%s: notify_func=%p, mmc_host_dev=%p, val=%d\n",
		__FUNCTION__, notify_func_callback, mmc_host_dev, val);

	if (notify_func_callback) {
		notify_func_callback(mmc_host_dev, val);
	} else {
		pr_warning("%s: Nobody to notify\n", __FUNCTION__);
	}

	return 0;
}
#endif /* !CONFIG_BCMDHD_PCIE */

int __init
ifx_wlan_init_gpio(void)
{
	const char *wlan_node = "samsung,ifx-wlan";
	struct device_node *root_node = NULL;
	struct device *wlan_dev;

	wlan_dev = sec_device_find("wlan");
	if (wlan_dev == NULL) {
		wlan_dev = sec_device_create(NULL, "wlan");
	}

	root_node = of_find_compatible_node(NULL, NULL, wlan_node);
	if (!root_node) {
		printk(KERN_ERR "%s: failed to get device node of ifx-wlan\n",
			__FUNCTION__);
		return -ENODEV;
	}

	/* ========== WLAN_PWR_EN ============ */
	wlan_pwr_on = of_get_gpio(root_node, 0);
	if (!gpio_is_valid(wlan_pwr_on)) {
		WARN(1, "Invalied gpio pin : %d\n", wlan_pwr_on);
		return -ENODEV;
	}

	if (gpio_request(wlan_pwr_on, "WLAN_REG_ON")) {
		WARN(1, "fail to request gpio(WLAN_REG_ON)\n");
		return -ENODEV;
	}
#ifdef CONFIG_BCMDHD_PCIE
	gpio_direction_output(wlan_pwr_on, 1);
	msleep(WIFI_TURNON_DELAY);
#else
	gpio_direction_output(wlan_pwr_on, 0);
#endif /* CONFIG_BCMDHD_PCIE */
	gpio_export(wlan_pwr_on, 1);
	if (wlan_dev)
		gpio_export_link(wlan_dev, "WLAN_REG_ON", wlan_pwr_on);

#ifdef EXYNOS_PCIE_RC_ONOFF
	exynos_pcie_pm_resume(SAMSUNG_PCIE_CH_NUM);
#endif /* EXYNOS_PCIE_RC_ONOFF */

#ifdef CONFIG_BCMDHD_OOB_HOST_WAKE
	/* ========== WLAN_HOST_WAKE ============ */
	wlan_host_wake_up = of_get_gpio(root_node, 1);
	if (!gpio_is_valid(wlan_host_wake_up)) {
		WARN(1, "Invalied gpio pin : %d\n", wlan_host_wake_up);
		return -ENODEV;
	}

	if (gpio_request(wlan_host_wake_up, "WLAN_HOST_WAKE")) {
		WARN(1, "fail to request gpio(WLAN_HOST_WAKE)\n");
		return -ENODEV;
	}
	gpio_direction_input(wlan_host_wake_up);
	gpio_export(wlan_host_wake_up, 1);
	if (wlan_dev)
		gpio_export_link(wlan_dev, "WLAN_HOST_WAKE", wlan_host_wake_up);

	wlan_host_wake_irq = gpio_to_irq(wlan_host_wake_up);
#endif /* CONFIG_BCMDHD_OOB_HOST_WAKE */

	return 0;
}

void
interrupt_set_cpucore(int set, unsigned int dpc_cpucore, unsigned int primary_cpucore)
{
	printk(KERN_INFO "%s: set: %d\n", __FUNCTION__, set);
	if (set) {
#if defined(CONFIG_MACH_UNIVERSAL5422)
		irq_set_affinity(EXYNOS5_IRQ_HSMMC1, cpumask_of(dpc_cpucore));
		irq_set_affinity(EXYNOS_IRQ_EINT16_31, cpumask_of(dpc_cpucore));
#endif /* CONFIG_MACH_UNIVERSAL5422 */
#if defined(CONFIG_MACH_UNIVERSAL5430)
		irq_set_affinity(IRQ_SPI(226), cpumask_of(dpc_cpucore));
		irq_set_affinity(IRQ_SPI(2), cpumask_of(dpc_cpucore));
#endif /* CONFIG_MACH_UNIVERSAL5430 */
#if defined(CONFIG_MACH_UNIVERSAL7580)
		irq_set_affinity(IRQ_SPI(246), cpumask_of(dpc_cpucore));
#endif /* CONFIG_MACH_UNIVERSAL7580 */
	} else {
#if defined(CONFIG_MACH_UNIVERSAL5422)
		irq_set_affinity(EXYNOS5_IRQ_HSMMC1, cpumask_of(primary_cpucore));
		irq_set_affinity(EXYNOS_IRQ_EINT16_31, cpumask_of(primary_cpucore));
#endif /* CONFIG_MACH_UNIVERSAL5422 */
#if defined(CONFIG_MACH_UNIVERSAL5430)
		irq_set_affinity(IRQ_SPI(226), cpumask_of(primary_cpucore));
		irq_set_affinity(IRQ_SPI(2), cpumask_of(primary_cpucore));
#endif /* CONFIG_MACH_UNIVERSAL5430 */
#if defined(CONFIG_MACH_UNIVERSAL7580)
		irq_set_affinity(IRQ_SPI(246), cpumask_of(primary_cpucore));
#endif /* CONFIG_MACH_UNIVERSAL7580 */
	}
}

#if defined(CONFIG_BCMDHD_OOB_HOST_WAKE) && defined(CONFIG_BCMDHD_GET_OOB_STATE)
int
ifx_get_wlan_oob_gpio(void)
{
	return gpio_is_valid(wlan_host_wake_up) ?
		gpio_get_value(wlan_host_wake_up) : -1;
}
EXPORT_SYMBOL(ifx_get_wlan_oob_gpio);
#endif /* CONFIG_BCMDHD_OOB_HOST_WAKE && CONFIG_BCMDHD_GET_OOB_STATE */

struct resource ifx_wlan_resources = {
	.name	= "bcmdhd_wlan_irq",
	.start	= 0,
	.end	= 0,
	.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE |
#ifdef CONFIG_BCMDHD_PCIE
	IORESOURCE_IRQ_HIGHEDGE,
#else
	IORESOURCE_IRQ_HIGHLEVEL,
#endif /* CONFIG_BCMDHD_PCIE */
};
EXPORT_SYMBOL(ifx_wlan_resources);

struct wifi_platform_data ifx_wlan_control = {
	.set_power	= dhd_wlan_power,
	.set_reset	= dhd_wlan_reset,
#ifndef CONFIG_BCMDHD_PCIE
	.set_carddetect	= dhd_wlan_set_carddetect,
#endif /* !CONFIG_BCMDHD_PCIE */
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	.mem_prealloc	= ifx_wlan_mem_prealloc,
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */
};
EXPORT_SYMBOL(ifx_wlan_control);

int __init
ifx_wlan_init(void)
{
	int ret;

	printk(KERN_INFO "%s: START.......\n", __FUNCTION__);
	ret = ifx_wlan_init_gpio();
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to initiate GPIO, ret=%d\n",
			__FUNCTION__, ret);
		goto fail;
	}

#ifdef CONFIG_BCMDHD_OOB_HOST_WAKE
	ifx_wlan_resources.start = wlan_host_wake_irq;
	ifx_wlan_resources.end = wlan_host_wake_irq;
#endif /* CONFIG_BCMDHD_OOB_HOST_WAKE */

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	ret = ifx_init_wlan_mem();
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to alloc reserved memory,"
			" ret=%d\n", __FUNCTION__, ret);
	}
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

fail:
	return ret;
}

#if defined(CONFIG_MACH_UNIVERSAL7420) || defined(CONFIG_SOC_EXYNOS8890) || \
	defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810) || \
	defined(CONFIG_SOC_EXYNOS9820)
#if defined(CONFIG_DEFERRED_INITCALLS)
deferred_module_init(ifx_wlan_init);
#else
late_initcall(ifx_wlan_init);
#endif /* CONFIG_DEFERRED_INITCALLS */
#else
device_initcall(ifx_wlan_init);
#endif /* CONFIG Exynos PCIE Platforms */
