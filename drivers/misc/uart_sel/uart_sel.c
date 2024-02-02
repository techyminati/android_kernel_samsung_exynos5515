/*
 * Copyright (C) 2014 Samsung Electronics Co.Ltd
 * http://www.samsung.com
 *
 * UART Switch driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
*/

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#ifdef CONFIG_DRV_SAMSUNG
#include <linux/sec_class.h>
#endif

#ifdef CONFIG_MUIC_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif

#include <linux/mcu_ipc.h>
#include <linux/uart_sel.h>

enum uart_direction_t uart_dir = AP;
struct uart_sel_data *switch_data = NULL;

#define SYSREG_APM_BASE			0x12820000
#define JTAG_OVER_USB			0x0600
#define SEL_TXD_GPIO_UART_DEBUG_0	0x062c
#define SEL_TXD_GPIO_UART_DEBUG_1	0x0630
#define SEL_TXD_USB_PHY			0x0634
#define SEL_RXD_WLBT_UART		0x0638
#define SEL_RXD_GNSS_UART		0x064C
#define SEL_RXD_CP_UART			0x0650
#define SEL_RXD_DBGCORE_UART		0x0654
#define SEL_RXD_AP_UART_DEBUG		0x0658

#define APBIF_PMU_ALIVE			0x12860000
#define USB20_PHY_CONFIGURATION		0x072c

#define USB20DRD_CTRL0_BASE		0x10520000
#define HSP_CTRL			0x0054

void __iomem *sysreg_apm_ioaddr;
void __iomem *apbif_pmu_alive_ioaddr;
void __iomem *usb20drd_ctrl0_ioaddr;

static void uart_dir_work(void);

#ifdef __UART_SEL_DEBUG_
int test_cp_uart_set_read(void)
{
#define CP_UART_CFG_ADDR	0x11860760

	int ret = 0;
	void __iomem *CP_UART_CFG_AKQ;

	CP_UART_CFG_AKQ = devm_ioremap(switch_data->dev, CP_UART_CFG_ADDR, SZ_64);

	usel_err("__raw_readl(CP_UART_CFG_AKQ): 0x%08x\n", __raw_readl(CP_UART_CFG_AKQ));

	devm_iounmap(switch_data->dev, CP_UART_CFG_AKQ);

	return ret;
}
#endif

int config_pmu_uart_sel(int path)
{
	int ret	= 0;
	u32 reg = 0;

	usel_err("%s: setting path to %s\n", __func__, path ? "CP" : "AP");

	__raw_writel(0x1, apbif_pmu_alive_ioaddr + USB20_PHY_CONFIGURATION);
	usel_err("USB20_PHY_CONFIGURATION: 0x%08x\n", __raw_readl(apbif_pmu_alive_ioaddr + USB20_PHY_CONFIGURATION));

	__raw_writel(0x4, sysreg_apm_ioaddr + JTAG_OVER_USB);
	usel_err("JTAG_OVER_USB: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + JTAG_OVER_USB));

	reg = __raw_readl(usb20drd_ctrl0_ioaddr + HSP_CTRL) | (0x3 << 24);
	__raw_writel(reg, usb20drd_ctrl0_ioaddr + HSP_CTRL);
	usel_err("HSP_CTRL: 0x%08x\n", __raw_readl(usb20drd_ctrl0_ioaddr + HSP_CTRL));

	if (path == AP) {
		__raw_writel(0x0, sysreg_apm_ioaddr + SEL_TXD_USB_PHY);
		usel_err("SEL_TXD_USB_PHY: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + SEL_TXD_USB_PHY));
		__raw_writel(0x3, sysreg_apm_ioaddr + SEL_RXD_AP_UART_DEBUG);
		usel_err("SEL_RXD_AP_UART_DEBUG: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + SEL_RXD_AP_UART_DEBUG));

		__raw_writel(0x0, sysreg_apm_ioaddr + SEL_RXD_CP_UART);
		usel_err("SEL_RXD_CP_UART: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + SEL_RXD_CP_UART));
	} else {
		__raw_writel(0x2, sysreg_apm_ioaddr + SEL_TXD_USB_PHY);
		usel_err("SEL_TXD_USB_PHY: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + SEL_TXD_USB_PHY));
		__raw_writel(0x1, sysreg_apm_ioaddr + SEL_RXD_AP_UART_DEBUG);
		usel_err("SEL_RXD_AP_UART_DEBUG: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + SEL_RXD_AP_UART_DEBUG));
		__raw_writel(0x3, sysreg_apm_ioaddr + SEL_RXD_CP_UART);
		usel_err("SEL_RXD_CP_UART: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + SEL_RXD_CP_UART));
	}

#ifdef __UART_SEL_DEBUG_
	//check value
	test_cp_uart_set_read();
#endif

	return ret;
}

void disable_uart_connect(void)
{
	u32 reg = 0;

	usel_err("disable uart connect\n");

	__raw_writel(0x0, apbif_pmu_alive_ioaddr + USB20_PHY_CONFIGURATION);
	usel_err("USB20_PHY_CONFIGURATION: 0x%08x\n", __raw_readl(apbif_pmu_alive_ioaddr + USB20_PHY_CONFIGURATION));

	__raw_writel(0x0, sysreg_apm_ioaddr + JTAG_OVER_USB);
	usel_err("JTAG_OVER_USB: 0x%08x\n", __raw_readl(sysreg_apm_ioaddr + JTAG_OVER_USB));

	usel_err("HSP_CTRL: 0x%08x\n", __raw_readl(usb20drd_ctrl0_ioaddr + HSP_CTRL));
	reg = ~(0x3 << 24) & __raw_readl(usb20drd_ctrl0_ioaddr + HSP_CTRL);
	__raw_writel(reg, usb20drd_ctrl0_ioaddr + HSP_CTRL);
	usel_err("HSP_CTRL: 0x%08x\n", __raw_readl(usb20drd_ctrl0_ioaddr + HSP_CTRL));
}

static int set_uart_sel(void)
{
	int ret;

	if (switch_data->uart_switch_sel == CP) {
		usel_err("Change Uart to CP\n");
		ret = config_pmu_uart_sel(CP);

		if (ret < 0)
			usel_err("%s: ERR! write Fail: %d\n", __func__, ret);
	} else {
		usel_err("Change UART to AP\n");
		ret = config_pmu_uart_sel(AP);

		if (ret < 0)
			usel_err("%s: ERR! write Fail: %d\n", __func__, ret);
	}
	uart_dir_work();

	return 0;
}

static int uart_direction(char *str)
{
	uart_dir = strstr(str, "CP") ? CP : AP;
	usel_err("%s: uart direction: %s (%d)\n", __func__, str, uart_dir);

	return 0;
}
__setup("uart_sel=", uart_direction);

static void uart_dir_work(void)
{
#if defined(CONFIG_SEC_MODEM_S5000AP)
	u32 info_value = 0;

	if (switch_data == NULL)
		return;

	info_value = (switch_data->uart_connect && switch_data->uart_switch_sel);

	if (info_value != mbox_extract_value(MCU_CP, switch_data->mbx_ap_united_status,
				switch_data->sbi_uart_noti_mask, switch_data->sbi_uart_noti_pos)) {
		usel_err("%s: change uart state to %s\n", __func__,
			info_value ? "CP" : "AP");

		mbox_update_value(MCU_CP, switch_data->mbx_ap_united_status, info_value,
			switch_data->sbi_uart_noti_mask, switch_data->sbi_uart_noti_pos);
		if (mbox_extract_value(MCU_CP, switch_data->mbx_ap_united_status,
			switch_data->sbi_uart_noti_mask, switch_data->sbi_uart_noti_pos))
			mbox_set_interrupt(MCU_CP, switch_data->int_uart_noti);
	}
#endif
}

#if defined(CONFIG_SEC_MODEM_S5000AP)
void cp_recheck_uart_dir(void)
{
	u32 mbx_uart_noti;

	mbx_uart_noti = mbox_extract_value(MCU_CP, switch_data->mbx_ap_united_status,
			switch_data->sbi_uart_noti_mask, switch_data->sbi_uart_noti_pos);
	if (switch_data->uart_switch_sel != mbx_uart_noti)
		usel_err("Uart notifier data is not matched with mbox!\n");

	if (uart_dir == CP) {
		switch_data->uart_connect = true;
		switch_data->uart_switch_sel = CP;
		usel_err("Forcely changed to CP uart!!\n");
		set_uart_sel();
	}
	uart_dir_work();
}
EXPORT_SYMBOL_GPL(cp_recheck_uart_dir);
#endif

#ifdef CONFIG_MUIC_NOTIFIER
static int switch_handle_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;

	usel_err("%s: action=%lu attached_dev=%d\n", __func__, action, (int)attached_dev);

	if ((attached_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC) || /* 17 */
		(attached_dev == ATTACHED_DEV_JIG_UART_ON_MUIC) || /* 21 */
		(attached_dev == ATTACHED_DEV_JIG_UART_ON_VB_MUIC) || /* 22 */
		(attached_dev == ATTACHED_DEV_JIG_UART_OFF_VB_MUIC) || /* 18 */
		(attached_dev == ATTACHED_DEV_JIG_USB_OFF_MUIC)) { /* 23 */
		switch (action) {
		case MUIC_NOTIFY_CMD_DETACH: /* 0 */
		case MUIC_NOTIFY_CMD_LOGICALLY_DETACH: /* 3 */
			switch_data->uart_connect = false;
			disable_uart_connect();
			uart_dir_work();
			break;
		case MUIC_NOTIFY_CMD_ATTACH: /* 1 */
		case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH: /* 4 */
			switch_data->uart_connect = true;
			set_uart_sel();
			break;
		}
	}
	return 0;
}
#endif

static ssize_t usb_sel_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	return sprintf(buf, "PDA\n");
}

static ssize_t usb_sel_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	return count;
}

static ssize_t
uart_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "Uart direction: ");

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, (switch_data->uart_switch_sel == AP) ?
			"AP\n":"CP\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t
uart_sel_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	usel_err("%s Change UART port path\n", __func__);

	if (!strncasecmp(buf, "AP", 2)) {
		switch_data->uart_switch_sel = AP;
		set_uart_sel();
	} else if (!strncasecmp(buf, "CP", 2)) {
		switch_data->uart_switch_sel = CP;
		set_uart_sel();
	} else {
		usel_err("%s invalid value\n", __func__);
	}

	return count;
}

#if defined(CONFIG_MUIC_NOTIFIER)
static ssize_t attached_dev_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	ssize_t ret = 0;

	if (switch_data->muic_get_attached_dev)
		switch_data->attached_dev = switch_data->muic_get_attached_dev();

	switch (switch_data->attached_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		ret = sprintf(buf, "%s\n", "JIG UART OFF");
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		ret = sprintf(buf, "%s\n", "JIG UART OFF/VB");
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		ret = sprintf(buf, "%s\n", "JIG UART ON");
		break;
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		ret = sprintf(buf, "%s\n", "JIG UART ON/VB");
		break;
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		ret = sprintf(buf, "%s\n", "JIG USB OFF");
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		ret = sprintf(buf, "%s\n", "JIG USB ON");
		break;
	case ATTACHED_DEV_USB_MUIC:
		ret = sprintf(buf, "%s\n", "USB");
		break;
	case ATTACHED_DEV_TA_MUIC:
		ret = sprintf(buf, "%s\n", "TA");
		break;
	default:
		ret = sprintf(buf, "%s\n", "No VPS");
		break;
	}

	return ret;
}
#endif

static DEVICE_ATTR(usb_sel, 0664, usb_sel_show, usb_sel_store);
static DEVICE_ATTR(uart_sel, 0664, uart_sel_show, uart_sel_store);
#if defined(CONFIG_MUIC_NOTIFIER)
static DEVICE_ATTR(attached_dev, 0444, attached_dev_show, NULL);
#endif

static struct attribute *modemif_sysfs_attributes[] = {
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_sel.attr,
#if defined(CONFIG_MUIC_NOTIFIER)
	&dev_attr_attached_dev.attr,
#endif
	NULL
};

static const struct attribute_group uart_sel_sysfs_group = {
	.attrs = modemif_sysfs_attributes,
};

static int uart_sel_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int err = 0;

	usel_err("%s: uart_sel probe start.\n", __func__);

	switch_data = devm_kzalloc(dev, sizeof(struct uart_sel_data), GFP_KERNEL);
	switch_data->dev = dev;

	err = of_property_read_u32(dev->of_node, "int_ap2cp_uart_noti",
			&switch_data->int_uart_noti);
	if (err) {
		usel_err("SWITCH_SEL parse error! [id]\n");
		return err;
	}
	err = of_property_read_u32(dev->of_node, "mbx_ap2cp_united_status",
			&switch_data->mbx_ap_united_status);
	err = of_property_read_u32(dev->of_node, "sbi_uart_noti_mask",
			&switch_data->sbi_uart_noti_mask);
	err = of_property_read_u32(dev->of_node, "sbi_uart_noti_pos",
			&switch_data->sbi_uart_noti_pos);
	err = of_property_read_u32(dev->of_node, "use_usb_phy",
			&switch_data->use_usb_phy);

	usel_err("SWITCH_SEL use_usb_phy [%d]\n", switch_data->use_usb_phy);

	if (err) {
		usel_err("SWITCH_SEL parse error! [id]\n");
		return err;
	}

	err = device_create_file(dev, &dev_attr_uart_sel);
	if (err) {
		usel_err("can't create uart_sel device file!!!\n");
	}

	switch_data->uart_switch_sel = (uart_dir == AP) ? AP : CP;
	switch_data->uart_connect = false;

	sysreg_apm_ioaddr = devm_ioremap(dev, SYSREG_APM_BASE, SZ_32K);
	if(!sysreg_apm_ioaddr)
		usel_err("sysreg_apm_ioaddr allocated fail\n");

	apbif_pmu_alive_ioaddr = devm_ioremap(dev, APBIF_PMU_ALIVE, SZ_32K);
	if(!apbif_pmu_alive_ioaddr) {
		usel_err("apbif_pmu_alive_ioaddr allocatd fail\n");
	}

	usb20drd_ctrl0_ioaddr = devm_ioremap(dev, USB20DRD_CTRL0_BASE, SZ_32K);
	if(!usb20drd_ctrl0_ioaddr) {
		usel_err("usb20drd_ctrl0_ioaddr allocated fail\n");
	}

#if defined(CONFIG_MUIC_NOTIFIER)
	switch_data->muic_get_attached_dev = muic_notifier_attached_dev_info;
	switch_data->uart_notifier.notifier_call = switch_handle_notification;
	muic_notifier_register(&switch_data->uart_notifier, switch_handle_notification,
					MUIC_NOTIFY_DEV_USB);
#endif

#ifdef CONFIG_DRV_SAMSUNG
	if (IS_ERR(switch_device)) {
		usel_err("%s Failed to create device(switch)!\n", __func__);
		return -ENODEV;
	}

	/* create sysfs group */
	err = sysfs_create_group(&switch_device->kobj, &uart_sel_sysfs_group);
	if (err) {
		usel_err("%s: failed to create modemif sysfs attribute group\n",
			__func__);
		return -ENODEV;
	}
#endif

	return 0;
}

static int __exit uart_sel_remove(struct platform_device *pdev)
{
	/* TODO */
	return 0;
}

#ifdef CONFIG_PM
static int uart_sel_suspend(struct device *dev)
{
	/* TODO */
	return 0;
}

static int uart_sel_resume(struct device *dev)
{
        if (switch_data->uart_connect == true) {
                usel_err("resume\n");
                set_uart_sel();
        }

	return 0;
}
#else
#define uart_sel_suspend NULL
#define uart_sel_resume NULL
#endif

static const struct dev_pm_ops uart_sel_pm_ops = {
	.suspend = uart_sel_suspend,
	.resume = uart_sel_resume,
};

static const struct of_device_id exynos_uart_sel_dt_match[] = {
		{ .compatible = "samsung,exynos-uart-sel", },
		{},
};
MODULE_DEVICE_TABLE(of, exynos_uart_sel_dt_match);

static struct platform_driver uart_sel_driver = {
	.probe		= uart_sel_probe,
	.remove		= uart_sel_remove,
	.driver		= {
		.name = "uart_sel",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_uart_sel_dt_match),
		.pm = &uart_sel_pm_ops,
	},
};
module_platform_driver(uart_sel_driver);

MODULE_DESCRIPTION("UART SEL driver");
MODULE_AUTHOR("<hy50.seo@samsung.com>");
MODULE_LICENSE("GPL");
