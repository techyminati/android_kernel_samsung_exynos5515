/* drivers/battery_2/s2mpw03_muic.c
 * S2MPW03 MUIC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/mfd/samsung/s2mpw03.h>
#include <linux/mfd/samsung/s2mpw03-regulator.h>
#include <linux/version.h>
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include "include/charger/s2mpw03_charger.h"

struct s2mpw03_muic {
	struct s2mpw03_dev	*iodev;
	struct i2c_client	*client;

	struct workqueue_struct	*wqueue;
	struct delayed_work 	init_work;
	struct delayed_work 	attach_work;
	struct delayed_work 	detach_work;

	char*	charger_name;
	char*	fuelgauge_name;
	char*	wireless_charger_name;

	int irq_uart_off;
	int irq_uart_on;
	int irq_usb_off;
	int irq_usb_on;
	int irq_uart_cable;
	int irq_fact_leakage;
	int irq_jigon;
	int irq_acokf;
	int irq_acokr;
	int irq_rid_attach;

	muic_attached_dev_t	muic_dev;

	/* model dependent muic platform data */
	struct muic_platform_data	*pdata;

	struct wakeup_source *muic_isr_ws;
	int muic_isr_wakelocked;
};

static void s2mpw03_muic_set_wakelock(struct s2mpw03_muic *muic, int enable)
{
	if (enable) {
		if (!muic->muic_isr_wakelocked) {
			__pm_stay_awake(muic->muic_isr_ws);
			muic->muic_isr_wakelocked = 1;
		}
	} else {
		if (muic->muic_isr_wakelocked) {
			__pm_relax(muic->muic_isr_ws);
			muic->muic_isr_wakelocked = 0;
		}
	}
}

static int s2mpw03_wpc_online_check(struct s2mpw03_muic *muic)
{
	union power_supply_propval val = {0,};

	psy_do_property(muic->wireless_charger_name, get,
		POWER_SUPPLY_PROP_PRESENT, val);
	return val.intval;
}

static int s2mpw03_send_muic_msg_to_wpc(struct s2mpw03_muic *muic, enum power_supply_muic_msg muic_msg)
{
	union power_supply_propval val = {muic_msg, };
	int ret = 0;

	ret = psy_do_property(muic->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_MUIC_MSG, val);
	return ret;
}

static void s2mpw03_set_factory_mode(struct s2mpw03_muic *muic, int factory_mode)
{
	union power_supply_propval val = {0,};

	val.intval = factory_mode;
	psy_do_property(muic->charger_name, set,
		POWER_SUPPLY_EXT_PROP_FACTORY_MODE, val);

	psy_do_property(muic->fuelgauge_name, set,
		POWER_SUPPLY_EXT_PROP_FACTORY_MODE, val);
}

static void s2mpw03_set_muic_dev(struct s2mpw03_muic *muic, muic_attached_dev_t muic_dev)
{
	pr_info("%s %d -> %d\n", __func__, muic->muic_dev, muic_dev);
	if (muic_dev != muic->muic_dev) {
#if defined(CONFIG_MUIC_NOTIFIER)
		if (muic_dev == ATTACHED_DEV_NONE_MUIC)
			muic_notifier_detach_attached_dev(muic_dev);
		else {
			if (muic->muic_dev != ATTACHED_DEV_NONE_MUIC)
				muic_notifier_detach_attached_dev(muic->muic_dev);
			muic_notifier_attach_attached_dev(muic_dev);
		}
#endif
		muic->muic_dev = muic_dev;
	}
}

static muic_attached_dev_t s2mpw03_check_muic_attached_dev(struct s2mpw03_muic *muic, bool is_on)
{
	muic_attached_dev_t muic_dev = ATTACHED_DEV_NONE_MUIC;

	if (is_on) {
		unsigned char chg_sts1, chg_sts4;

		s2mpw03_read_reg(muic->client, S2MPW03_CHG_REG_STATUS1, &chg_sts1);
		s2mpw03_read_reg(muic->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);

		if (chg_sts1 & CHG_ACOK_STATUS_MASK) {
			if (chg_sts4 & UART_CABLE_MASK) {
				muic_dev = ATTACHED_DEV_USB_MUIC;
				pr_info("%s: USB/UART connected\n", __func__);
			} else if (chg_sts4 & USB_BOOT_ON_MASK) {
				muic_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
				pr_info("%s: JIG_ID USB ON ( 301K ) connected\n", __func__);
			} else if (chg_sts4 & USB_BOOT_OFF_MASK) {
				muic_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
				pr_info("%s: JIG_ID USB OFF ( 255K ) connected\n", __func__);
			} else  {
				if (chg_sts4 & UART_BOOT_ON_MASK) {
					muic_dev = ATTACHED_DEV_JIG_UART_ON_VB_MUIC;
					pr_info("%s: JIG_ID UART ON ( 619K ) + VB connected\n", __func__);
				} else if (chg_sts4 & UART_BOOT_OFF_MASK) {
					//muic_dev = ATTACHED_DEV_JIG_UART_OFF_VB_MUIC;
					muic_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
					pr_info("%s: JIG_ID UART OFF ( 523K ) + VB connected\n", __func__);
				} else {
					if (s2mpw03_wpc_online_check(muic)) {
						muic_dev = ATTACHED_DEV_WIRELESS_TA_MUIC;
						pr_info("%s: WIRELESS TA connected\n", __func__);
					} else {
#if defined(CONFIG_S2MPW03_TA_DETECT)
						if (chg_sts4 == CHG_STATUS4_JIGON_STS) {
							muic_dev = ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC;
							pr_info("%s: JIGON + VB connected\n", __func__);
						} else {
							muic_dev = ATTACHED_DEV_USB_MUIC;
							pr_info("%s: USB connected\n", __func__);
						}
#else
						muic_dev = ATTACHED_DEV_TA_MUIC;
						pr_info("%s: TA connected\n", __func__);
#endif
					}
				}
			}
		} else {
			if (chg_sts4 & USB_BOOT_OFF_MASK) {
				muic_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
				pr_info("%s: JIG_ID USB OFF ( 255K ) connected\n", __func__);
			} else if (chg_sts4 & UART_BOOT_ON_MASK) {
				muic_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
				pr_info("%s: JIG_ID UART ON ( 619K ) connected\n", __func__);
			} else if (chg_sts4 & UART_BOOT_OFF_MASK) {
				muic_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
				pr_info("%s: JIG_ID UART OFF ( 523K ) connected\n", __func__);
			}
		}

		pr_info("%s: sts1 = 0x%x, stat4 = 0x%x, muic_dev = %d\n",
			__func__, chg_sts1, chg_sts4, muic_dev);
	}

	return muic_dev;
}

static void s2mpw03_muic_init_work(struct work_struct *work)
{
	struct s2mpw03_muic *muic =
		container_of(work, struct s2mpw03_muic, init_work.work);
	muic_attached_dev_t muic_dev = ATTACHED_DEV_NONE_MUIC;
	unsigned char pmic_sts1, chg_sts4;
	int ret = 0;

	ret = s2mpw03_read_reg(muic->iodev->pmic, S2MPW03_PMIC_REG_STATUS1, &pmic_sts1);
	if (ret < 0)
		pr_err("%s: failed to read PMIC_REG_STATUS1, ret = %d\n", __func__, ret);
	ret = s2mpw03_read_reg(muic->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);
	if (ret < 0)
		pr_err("%s: failed to read S2MPW03_CHG_REG_STATUS4, ret = %d\n", __func__, ret);
	pr_info("%s: read pmic_sts1 = 0x%x, chg_sts4 = 0x%x\n", __func__, pmic_sts1, chg_sts4);

	muic_dev = s2mpw03_check_muic_attached_dev(muic, !(pmic_sts1 & ACOKB_STATUS_MASK));
	switch (muic_dev) {
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		s2mpw03_set_factory_mode(muic, true);
		break;
	default:
		if (chg_sts4 & FACT_LEAKAGE_MASK)
			s2mpw03_set_factory_mode(muic, true);
		break;
	}

	pr_info("%s: muic_dev = %d\n", __func__, muic_dev);
	s2mpw03_set_muic_dev(muic, muic_dev);
}

static void s2mpw03_muic_attach_work(struct work_struct *work)
{
	struct s2mpw03_muic *muic =
		container_of(work, struct s2mpw03_muic, attach_work.work);

	s2mpw03_set_muic_dev(muic,
		s2mpw03_check_muic_attached_dev(muic, true));

	switch (muic->muic_dev) {
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		s2mpw03_set_factory_mode(muic, true);
		break;
	default:
		break;
	}

	pr_info("%s: muic_dev = %d\n", __func__, muic->muic_dev);
	s2mpw03_muic_set_wakelock(muic, 0);
}

static void s2mpw03_muic_detach_work(struct work_struct *work)
{
	struct s2mpw03_muic *muic =
		container_of(work, struct s2mpw03_muic, detach_work.work);

	s2mpw03_set_muic_dev(muic,
		s2mpw03_check_muic_attached_dev(muic, false));
	pr_info("%s: muic_dev = %d\n", __func__, muic->muic_dev);
	s2mpw03_muic_set_wakelock(muic, 0);
}

static const char* s2mpw03_muic_irq_str(struct s2mpw03_muic *muic, int irq)
{
	if (irq == muic->irq_uart_cable)
		return "UART CABLE";
	if (irq == muic->irq_acokf)
		return "ACOK FALLING";
	if (irq == muic->irq_acokr)
		return "ACOK RISING";
	if (irq == muic->irq_usb_off)
		return "USB OFF";
	if (irq == muic->irq_usb_on)
		return "USB ON";
	if (irq == muic->irq_uart_off)
		return "UART OFF";
	if (irq == muic->irq_uart_on)
		return "UART ON";

	return "UNKNOWN";
}

static irqreturn_t s2mpw03_muic_isr(int irq, void *data)
{
	struct s2mpw03_muic *muic = data;

	pr_info("%s: irq:%d, %s\n",
		__func__, irq, s2mpw03_muic_irq_str(muic, irq));

	if (irq == muic->irq_acokf) {
		s2mpw03_muic_set_wakelock(muic, 1);
		s2mpw03_send_muic_msg_to_wpc(muic, POWER_SUPPLY_MUIC_ACOK_FALLING);
		queue_delayed_work(muic->wqueue,
			&muic->attach_work, msecs_to_jiffies(150));
	} else if (irq == muic->irq_acokr) {
		s2mpw03_muic_set_wakelock(muic, 1);
		s2mpw03_send_muic_msg_to_wpc(muic, POWER_SUPPLY_MUIC_ACOK_RISING);
		queue_delayed_work(muic->wqueue,
			&muic->detach_work, msecs_to_jiffies(0));
	} else {
		if ((irq == muic->irq_usb_off) || \
			(irq == muic->irq_usb_on) || \
			(irq == muic->irq_uart_off) || \
			(irq == muic->irq_uart_on)) {
			s2mpw03_muic_set_wakelock(muic, 1);
			queue_delayed_work(muic->wqueue,
				&muic->attach_work, msecs_to_jiffies(100));
		}
	}

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _dev_id, _name)	\
do {	\
	ret = request_threaded_irq(_irq, NULL, s2mpw03_muic_isr,	\
				0, _name, _dev_id);	\
	if (ret < 0) {	\
		pr_err("%s: Failed to request s2mpw03 muic IRQ #%d: %d\n",	\
				__func__, _irq, ret);	\
		_irq = 0;	\
	}	\
} while (0)

#define FREE_IRQ(_irq, _dev_id)	\
do {	\
	if (_irq) {	\
		free_irq(_irq, _dev_id);	\
		pr_info("%s: IRQ(%d) free done\n", __func__, _irq);	\
	}	\
} while (0)

static void s2mpw03_muic_free_irqs(struct s2mpw03_muic *muic)
{
	/* free MUIC IRQ */
	FREE_IRQ(muic->irq_uart_off, muic);
	FREE_IRQ(muic->irq_uart_on, muic);
	FREE_IRQ(muic->irq_usb_off, muic);
	FREE_IRQ(muic->irq_usb_on, muic);
	FREE_IRQ(muic->irq_uart_cable, muic);
	FREE_IRQ(muic->irq_fact_leakage, muic);
	FREE_IRQ(muic->irq_jigon, muic);
}

struct s2mpw03_muic *s2mpw03_init_muic(struct platform_device *pdev)
{
	struct s2mpw03_dev *s2mpw03 = dev_get_drvdata(pdev->dev.parent);
	struct s2mpw03_muic *muic;
	struct device_node *np;
	int irq_base, ret;

	muic = devm_kzalloc(&pdev->dev, sizeof(struct s2mpw03_muic), GFP_KERNEL);
	if (!muic)
		return ERR_PTR(-ENOMEM);

	muic->iodev = s2mpw03;
	muic->client = s2mpw03->charger;
	muic->muic_dev = ATTACHED_DEV_NONE_MUIC;

	/* get charger & fuelgauge & wireless_charger 's name by dt */
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
	} else {
		ret = of_property_read_string(np,
			"battery,charger_name", (char const **)&muic->charger_name);
		if (ret)
			pr_info("%s: Charger name is Empty\n", __func__);

		ret = of_property_read_string(np,
			"battery,fuelgauge_name", (char const **)&muic->fuelgauge_name);
		if (ret)
			pr_info("%s: Fuelgauge name is Empty\n", __func__);

		ret = of_property_read_string(np,
			"battery,wireless_charger_name", (char const **)&muic->wireless_charger_name);
		if (ret)
			pr_info("%s: Wireless charger name is Empty\n", __func__);
	}

	muic->pdata = &muic_pdata;
	if (muic->pdata->init_switch_dev_cb)
		muic->pdata->init_switch_dev_cb();

	muic->muic_isr_ws = wakeup_source_register(muic->iodev->dev, "muic_isr");
	muic->muic_isr_wakelocked = 0;

	/* init work */
	muic->wqueue = create_singlethread_workqueue("s2mpw03-muic");
	INIT_DELAYED_WORK(&muic->init_work, s2mpw03_muic_init_work);
	INIT_DELAYED_WORK(&muic->attach_work, s2mpw03_muic_attach_work);
	INIT_DELAYED_WORK(&muic->detach_work, s2mpw03_muic_detach_work);

	queue_delayed_work(muic->wqueue, &muic->init_work, msecs_to_jiffies(100));

	/* request MUIC IRQ */
	irq_base = muic->iodev->irq_base;
#if 0
	muic->irq_fact_leakage = irq_base + S2MPW03_CHG_IRQ_FACT_LEAKAGE_INT3;
	REQUEST_IRQ(muic->irq_fact_leakage, muic, "muic-fact_leakage");
#endif
	muic->irq_uart_cable = irq_base + S2MPW03_CHG_IRQ_UART_CABLE_INT4;
	REQUEST_IRQ(muic->irq_uart_cable, muic, "muic-uart_cable");

	muic->irq_usb_off = irq_base + S2MPW03_CHG_IRQ_USB_BOOT_OFF_INT4;
	REQUEST_IRQ(muic->irq_usb_off, muic, "muic-usb_off");

	muic->irq_usb_on = irq_base + S2MPW03_CHG_IRQ_USB_BOOT_ON_INT4;
	REQUEST_IRQ(muic->irq_usb_on, muic, "muic-usb_on");

	muic->irq_uart_off = irq_base + S2MPW03_CHG_IRQ_UART_BOOT_OFF_INT4;
	REQUEST_IRQ(muic->irq_uart_off, muic, "muic-uart_off");

	muic->irq_uart_on = irq_base + S2MPW03_CHG_IRQ_UART_BOOT_ON_INT4;
	REQUEST_IRQ(muic->irq_uart_on, muic, "muic-uart_on");

	muic->irq_acokf = irq_base + S2MPW03_PMIC_IRQ_ACOKBF_INT1;
	REQUEST_IRQ(muic->irq_acokf, muic, "muic-acokf");

	muic->irq_acokr = irq_base + S2MPW03_PMIC_IRQ_ACOKBR_INT1;
	REQUEST_IRQ(muic->irq_acokr, muic, "muic-acokr");
	pr_err("%s:usb_off(%d), usb_on(%d), uart_off(%d), uart_on(%d), jig_on(%d), muic-acokf(%d), muic-acokr(%d)\n",
		__func__, muic->irq_usb_off, muic->irq_usb_on, muic->irq_uart_off, muic->irq_uart_on,
		muic->irq_jigon, muic->irq_acokf, muic->irq_acokr);

	pr_info("%s:[BATT] S2MPW03 MUIC driver loaded OK\n", __func__);
	return muic;
}

void s2mpw03_muic_unregister(struct s2mpw03_muic *muic)
{
	if (muic->pdata->cleanup_switch_dev_cb)
		muic->pdata->cleanup_switch_dev_cb();
	wakeup_source_unregister(muic->muic_isr_ws);
	s2mpw03_muic_free_irqs(muic);
	destroy_workqueue(muic->wqueue);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("MUIC driver for S2MPW03");
