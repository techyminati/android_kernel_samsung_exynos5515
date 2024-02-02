
/*
 * s2mpw03-irq.c - Interrupt controller support for S2MPW03
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/mfd/samsung/s2mpw03.h>
#include <linux/mfd/samsung/s2mpw03-regulator.h>

#include <sound/soc.h>
//#include <sound/cod3035x.h>

#if defined(CONFIG_CHARGER_S2MPW03)
#include <linux/power/s2mpw03_charger.h>
#endif
#if defined(CONFIG_FUELGAUGE_S2MPW03)
#include <linux/power/s2mpw03_fuelgauge.h>
#endif

#define S2MPW03_IBI_CNT		4

static u8 irq_reg[S2MPW03_IRQ_GROUP_NR] = {0};

static const u8 s2mpw03_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[PMIC_INT1] = 	S2MPW03_PMIC_REG_INT1M,
	[PMIC_INT2] =	S2MPW03_PMIC_REG_INT2M,
	[PMIC_INT3] = 	S2MPW03_PMIC_REG_INT3M,
#if defined(CONFIG_CHARGER_S2MPW03)
	[CHG_INT1] = 	S2MPW03_CHG_REG_INT1M,
	[CHG_INT2] = 	S2MPW03_CHG_REG_INT2M,
	[CHG_INT3] = 	S2MPW03_CHG_REG_INT3M,
	[CHG_INT4] = 	S2MPW03_CHG_REG_INT4M,
#endif
#if defined(CONFIG_FUELGAUGE_S2MPW03)
	[FG_INT] = S2MPW03_FG_REG_INT,
#endif
};

static struct i2c_client *get_i2c(struct s2mpw03_dev *s2mpw03,
				enum s2mpw03_irq_source src)
{
	switch (src) {
	case PMIC_INT1 ... PMIC_INT3:
		return s2mpw03->pmic;
#if defined(CONFIG_CHARGER_S2MPW03)
	case CHG_INT1 ... CHG_INT4:
		return s2mpw03->charger;
#endif
#if defined(CONFIG_FUELGAUGE_S2MPW03)
	case FG_INT:
		return s2mpw03->fuelgauge;
#endif
	default:
		return ERR_PTR(-EINVAL);
	}
}

struct s2mpw03_irq_data {
	int mask;
	enum s2mpw03_irq_source group;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }
static const struct s2mpw03_irq_data s2mpw03_irqs[] = {
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_PWRONR_INT1,	PMIC_INT1, 1 << 1),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_PWRONF_INT1,	PMIC_INT1, 1 << 0),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_JIGONBF_INT1,	PMIC_INT1, 1 << 2),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_JIGONBR_INT1,	PMIC_INT1, 1 << 3),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_ACOKBF_INT1,	PMIC_INT1, 1 << 4),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_ACOKBR_INT1,	PMIC_INT1, 1 << 5),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_PWRON1S_INT1,	PMIC_INT1, 1 << 6),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_MRB_INT1,		PMIC_INT1, 1 << 7),

	DECLARE_IRQ(S2MPW03_PMIC_IRQ_RTC60S_INT2,	PMIC_INT2, 1 << 0),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_RTCA1_INT2,	PMIC_INT2, 1 << 1),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_RTCA0_INT2,	PMIC_INT2, 1 << 2),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_SMPL_INT2,		PMIC_INT2, 1 << 3),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_RTC1S_INT2,	PMIC_INT2, 1 << 4),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_WTSR_INT2,		PMIC_INT2, 1 << 5),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_WRSTB_INT2,	PMIC_INT2, 1 << 7),

	DECLARE_IRQ(S2MPW03_PMIC_IRQ_120C_INT3,		PMIC_INT3, 1 << 0),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_140C_INT3,		PMIC_INT3, 1 << 1),
	DECLARE_IRQ(S2MPW03_PMIC_IRQ_TSD_INT3,		PMIC_INT3, 1 << 2),

#if defined(CONFIG_CHARGER_S2MPW03)
	DECLARE_IRQ(S2MPW03_CHG_IRQ_RECHG_INT1,		CHG_INT1, 1 << 0),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_CHGDONE_INT1,	CHG_INT1, 1 << 1),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_TOPOFF_INT1,	CHG_INT1, 1 << 2),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_PRECHG_INT1,	CHG_INT1, 1 << 3),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_CHGSTS_INT1,	CHG_INT1, 1 << 4),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_CIN2BAT_INT1,	CHG_INT1, 1 << 5),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_CHGVINOVP_INT1,	CHG_INT1, 1 << 6),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_CHGVIN_INT1,	CHG_INT1, 1 << 7),

	DECLARE_IRQ(S2MPW03_CHG_IRQ_BAT_LVL_ON_INT2,	CHG_INT2, 1 << 1),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_TMROUT_INT2,		CHG_INT2, 1 << 2),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_CHGR_TSD_INT2,		CHG_INT2, 1 << 3),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_ADPATH_INT2,		CHG_INT2, 1 << 4),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_FCHG_INT2,			CHG_INT2, 1 << 5),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_A2D_CHGINOK_INT2,	CHG_INT2, 1 << 6),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_BATDET_INT2,		CHG_INT2, 1 << 7),

	DECLARE_IRQ(S2MPW03_CHG_IRQ_WDT_INT3,		CHG_INT3, 1 << 7),

	DECLARE_IRQ(S2MPW03_CHG_IRQ_JIGON_INT4,			CHG_INT4, 1 << 0),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_RID_ATTACH_INT4,	CHG_INT4, 1 << 1),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_FACT_LEAK_INT4,		CHG_INT4, 1 << 2),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_UART_BOOT_ON_INT4,	CHG_INT4, 1 << 3),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_UART_BOOT_OFF_INT4,	CHG_INT4, 1 << 4),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_USB_BOOT_ON_INT4,	CHG_INT4, 1 << 5),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_USB_BOOT_OFF_INT4,	CHG_INT4, 1 << 6),
	DECLARE_IRQ(S2MPW03_CHG_IRQ_UART_CABLE_INT4,	CHG_INT4, 1 << 7),
#endif

#if defined(CONFIG_FUELGAUGE_S2MPW03)
	DECLARE_IRQ(S2MPW03_FG_IRQ_VBAT_L_INT,		FG_INT, 1 << 0),
	DECLARE_IRQ(S2MPW03_FG_IRQ_SOC_L_INT,		FG_INT, 1 << 1),
	DECLARE_IRQ(S2MPW03_FG_IRQ_IDLE_ST_INT,		FG_INT, 1 << 2),
	DECLARE_IRQ(S2MPW03_FG_IRQ_INIT_ST_INT,		FG_INT, 1 << 3),
#endif
};

static void s2mpw03_irq_lock(struct irq_data *data)
{
	struct s2mpw03_dev *s2mpw03 = irq_get_chip_data(data->irq);

	mutex_lock(&s2mpw03->irqlock);
}

static void s2mpw03_irq_sync_unlock(struct irq_data *data)
{
	struct s2mpw03_dev *s2mpw03 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < S2MPW03_IRQ_GROUP_NR; i++) {
		u8 mask_reg = s2mpw03_mask_reg[i];
		struct i2c_client *i2c = get_i2c(s2mpw03, i);

		if (mask_reg == S2MPW03_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		s2mpw03->irq_masks_cache[i] = s2mpw03->irq_masks_cur[i];

		s2mpw03_write_reg(i2c, s2mpw03_mask_reg[i],
				s2mpw03->irq_masks_cur[i]);
	}

	mutex_unlock(&s2mpw03->irqlock);
}

static const inline struct s2mpw03_irq_data *
irq_to_s2mpw03_irq(struct s2mpw03_dev *s2mpw03, int irq)
{
	return &s2mpw03_irqs[irq - s2mpw03->irq_base];
}

static void s2mpw03_irq_mask(struct irq_data *data)
{
	struct s2mpw03_dev *s2mpw03 = irq_get_chip_data(data->irq);
	const struct s2mpw03_irq_data *irq_data =
	    irq_to_s2mpw03_irq(s2mpw03, data->irq);

	if (irq_data->group >= S2MPW03_IRQ_GROUP_NR)
		return;

	s2mpw03->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void s2mpw03_irq_unmask(struct irq_data *data)
{
	struct s2mpw03_dev *s2mpw03 = irq_get_chip_data(data->irq);
	const struct s2mpw03_irq_data *irq_data =
	    irq_to_s2mpw03_irq(s2mpw03, data->irq);

	if (irq_data->group >= S2MPW03_IRQ_GROUP_NR)
		return;

	s2mpw03->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static struct irq_chip s2mpw03_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock		= s2mpw03_irq_lock,
	.irq_bus_sync_unlock	= s2mpw03_irq_sync_unlock,
	.irq_mask		= s2mpw03_irq_mask,
	.irq_unmask		= s2mpw03_irq_unmask,
};

int codec_notifier_flag = 0;
/*
void set_codec_notifier_flag()
{
	codec_notifier_flag = 1;
}
*/

static void s2mpw03_report_irq(struct s2mpw03_dev *s2mpw03)
{
	int i;

	/* Apply masking */
	for (i = 0; i < S2MPW03_IRQ_GROUP_NR; i++)
		irq_reg[i] &= ~s2mpw03->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < S2MPW03_IRQ_NR; i++) {
		if (irq_reg[s2mpw03_irqs[i].group] & s2mpw03_irqs[i].mask)
			handle_nested_irq(s2mpw03->irq_base + i);
	}
}

static int s2mpw03_power_key_detection(struct s2mpw03_dev *s2mpw03)
{
	int ret, i;
	u8 val;

	/* Determine falling/rising edge for PWR_ON key */
	if ((irq_reg[PMIC_INT1] & 0x3) == 0x3) {
		ret = s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_STATUS1, &val);
		if (ret) {
			pr_err("%s: fail to read register\n", __func__);
			goto power_key_err;
		}
		irq_reg[PMIC_INT1] &= 0xFC;

		if (val & S2MPW03_STATUS1_PWRON) {
			irq_reg[PMIC_INT1] = S2MPW03_RISING_EDGE;
			s2mpw03_report_irq(s2mpw03);

			/* clear irq */
			for (i = 0; i < S2MPW03_IRQ_GROUP_NR; i++)
				irq_reg[i] &= 0x00;

			irq_reg[PMIC_INT1] = S2MPW03_FALLING_EDGE;
		} else {
			irq_reg[PMIC_INT1] = S2MPW03_FALLING_EDGE;
			s2mpw03_report_irq(s2mpw03);

			/* clear irq */
			for (i = 0; i < S2MPW03_IRQ_GROUP_NR; i++)
				irq_reg[i] &= 0x00;

			irq_reg[PMIC_INT1] = S2MPW03_RISING_EDGE;
		}
	}
	return 0;

power_key_err:
	return 1;
}

static void s2mpw03_pending_clear(struct s2mpw03_dev *s2mpw03)
{
	u32 val;

	val = readl(s2mpw03->mem_sysreg_pending_base_ap);
	writel(val, s2mpw03->mem_sysreg_pending_base_ap);

	val = readl(s2mpw03->mem_sysreg_pending_base_pmu);
	writel(val, s2mpw03->mem_sysreg_pending_base_pmu);
}

static irqreturn_t s2mpw03_irq_thread(int irq, void *data)
{
	struct s2mpw03_dev *s2mpw03 = data;
	u8 ibi_src[S2MPW03_IBI_CNT] = {0};
	u32 val;
	int i, ret;

	/* Clear interrupt pending bit */
	s2mpw03_pending_clear(s2mpw03);

	/* Read VGPIO_RX_MONITOR */
	val = readl(s2mpw03->mem_base);

	for (i = 0; i < S2MPW03_IBI_CNT; i++) {
		ibi_src[i] = val & 0xFF;
		val = (val >> 8);
	}

	pr_info("%s ibi_src[0]: %02x\n", __func__, ibi_src[0]);

	if (ibi_src[0] & S2MPW03_IBI0_PMIC_M) {
		/* PMIC_INT */
		ret = s2mpw03_bulk_read(s2mpw03->pmic, S2MPW03_PMIC_REG_INT1,
				S2MPW03_NUM_IRQ_PMIC_REGS, &irq_reg[PMIC_INT1]);
		if (ret) {
			pr_err("%s:%s Failed to read pmic interrupt: %d\n",
				MFD_DEV_NAME, __func__, ret);
			return IRQ_NONE;
		}

		queue_delayed_work(s2mpw03->irq_wqueue, &s2mpw03->irq_work, 0);

		/* Power-key W/A */
		ret = s2mpw03_power_key_detection(s2mpw03);
		if (ret)
			pr_err("%s: POWER-KEY detection error\n", __func__);
	}
#if defined(CONFIG_CHARGER_S2MPW03)
	if (ibi_src[0] & S2MPW03_IRQSRC_CHG) {
		/* CHG_INT */
		ret = s2mpw03_bulk_read(s2mpw03->charger, S2MPW03_CHG_REG_INT1,
				S2MPW03_NUM_IRQ_CHG_REGS, &irq_reg[CHG_INT1]);
		if (ret) {
			pr_err("%s() Failed to read charger interrupt: %d\n",
				__func__, ret);
			return IRQ_NONE;
		}

		pr_info("%s() CHARGER intrrupt(0x%02x, 0x%02x, 0x%02x, 0x%02x)\n",
				__func__, irq_reg[CHG_INT1], irq_reg[CHG_INT2],
				irq_reg[CHG_INT3], irq_reg[CHG_INT4]);
	}
#endif
#if defined(CONFIG_FUELGAUGE_S2MPW03)
	if (ibi_src[0] & S2MPW03_IRQSRC_FG) {
		/* FG_INT */
		ret = s2mpw03_read_reg(s2mpw03->fuelgauge, S2MPW03_FG_REG_INT, &irq_reg[FG_INT]);
		if (ret) {
			pr_err("%s() Failed to read fuelgauge interrupt: %d\n",
				__func__, ret);
			return IRQ_NONE;
		}

		pr_info("%s() FUELGAUGE interrupt(0x%02x)\n", __func__, irq_reg[FG_INT]);
	}
#endif

		/* Report IRQ */
		s2mpw03_report_irq(s2mpw03);

	return IRQ_HANDLED;
}

static void s2mpw03_irq_work_func(struct work_struct *work)
{
	pr_info("%s: main pmic interrupt(0x%02hhx, 0x%02hhx, 0x%02hhx)\n",
		 __func__, irq_reg[PMIC_INT1], irq_reg[PMIC_INT2], irq_reg[PMIC_INT3]);
}

static int s2mpw03_set_wqueue(struct s2mpw03_dev *s2mpw03)
{
	s2mpw03->irq_wqueue = create_singlethread_workqueue("s2mpw03-wqueue");
	if (!s2mpw03->irq_wqueue) {
		pr_err("%s: fail to create workqueue\n", __func__);
		return 1;
	}

	INIT_DELAYED_WORK(&s2mpw03->irq_work, s2mpw03_irq_work_func);

	return 0;
}

static void s2mpw03_set_vgpio_monitor(struct s2mpw03_dev *s2mpw03)
{
	s2mpw03->mem_base = ioremap(VGPIO_I3C_BASE + VGPIO_MONITOR_ADDR, SZ_32);
	if (s2mpw03->mem_base == NULL)
		pr_info("%s: fail to allocate memory\n", __func__);
}

int s2mpw03_irq_init(struct s2mpw03_dev *s2mpw03)
{
	int i;
	int ret;
	u8 i2c_data;
	int cur_irq;

	if (!s2mpw03->irq_base) {
		dev_err(s2mpw03->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&s2mpw03->irqlock);

	/* Enable fg interrupt */
	ret = s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_IRQ_M, &i2c_data);
	i2c_data |= S2MPW03_FG_IBI_EN_M;
	s2mpw03_write_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_IRQ_M, i2c_data);

	/* Set VGPIO monitor */
	s2mpw03_set_vgpio_monitor(s2mpw03);

	s2mpw03->mem_sysreg_pending_base_ap = ioremap(SYSREG_VGPIO2AP + INTC0_IPEND, SZ_32);
	s2mpw03->mem_sysreg_pending_base_pmu = ioremap(SYSREG_VGPIO2PMU + INTC0_IPEND, SZ_32);

	/* Set workqueue */
	s2mpw03_set_wqueue(s2mpw03);

	/* Mask individual interrupt sources */
	for (i = 0; i < S2MPW03_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;

		s2mpw03->irq_masks_cur[i] = 0xff;
		s2mpw03->irq_masks_cache[i] = 0xff;

		i2c = get_i2c(s2mpw03, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (s2mpw03_mask_reg[i] == S2MPW03_REG_INVALID)
			continue;

		s2mpw03_write_reg(i2c, s2mpw03_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < S2MPW03_IRQ_NR; i++) {
		cur_irq = i + s2mpw03->irq_base;
		irq_set_chip_data(cur_irq, s2mpw03);
		irq_set_chip_and_handler(cur_irq, &s2mpw03_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	s2mpw03_write_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_INTSRC_MASK, 0xff);

	/* Unmask s2mpw03 interrupt */
	ret = s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		pr_err("%s:%s fail to read intsrc mask reg\n",
					 MFD_DEV_NAME, __func__);
		return ret;
	}

	//if(s2mpw03->codec)
	i2c_data &= ~(S2MPW03_IRQSRC_CODEC);

	i2c_data &= ~(S2MPW03_IRQSRC_PMIC);	/* Unmask pmic interrupt */
	s2mpw03_write_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_INTSRC_MASK,
			   i2c_data);

#if defined(CONFIG_CHARGER_S2MPW03)
	i2c_data &= ~(S2MPW03_IRQSRC_CHG);	/* Unmask charger interrupt */
#endif

	s2mpw03_write_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_INTSRC_MASK, i2c_data);

	pr_info("%s:%s s2mpw03_PMIC_REG_INTSRC_MASK=0x%02x\n",
			MFD_DEV_NAME, __func__, i2c_data);

	s2mpw03->irq = irq_of_parse_and_map(s2mpw03->dev->of_node, 0);
	ret = request_threaded_irq(s2mpw03->irq, NULL, s2mpw03_irq_thread,
				   IRQF_ONESHOT, "s2mpw03-irq", s2mpw03);

	if (ret) {
		dev_err(s2mpw03->dev, "Failed to request IRQ %d: %d\n",
			s2mpw03->irq, ret);
		destroy_workqueue(s2mpw03->irq_wqueue);
		return ret;
	}

	return 0;
}

void s2mpw03_irq_exit(struct s2mpw03_dev *s2mpw03)
{
	if (s2mpw03->irq)
		free_irq(s2mpw03->irq, s2mpw03);

	iounmap(s2mpw03->mem_base);
	iounmap(s2mpw03->mem_sysreg_pending_base_ap);
	iounmap(s2mpw03->mem_sysreg_pending_base_pmu);
	cancel_delayed_work_sync(&s2mpw03->irq_work);
	destroy_workqueue(s2mpw03->irq_wqueue);
}
