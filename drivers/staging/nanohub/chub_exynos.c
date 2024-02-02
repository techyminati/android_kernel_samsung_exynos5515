// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CHUB IF Driver Exynos specific code
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 * Authors:
 *	 Sukwon Ryu <sw.ryoo@samsung.com>
 *
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/gpio.h>
#include <linux/of_reserved_mem.h>

#ifdef CONFIG_SENSOR_DRV
#include "main.h"
#endif
#include "comms.h"
#include "chub.h"
#include "ipc_chub.h"
#include "chub_dbg.h"
#include "chub_exynos.h"

#include <soc/samsung/cal-if.h>

#if defined(CONFIG_SOC_S5E5515) && defined(CONFIG_EXYNOS_LPD)
#include <linux/time.h>

static unsigned short gpio_peri_addr[] = {
        0x0000, 0x0004, 0x0008, 0x000C, 0x0010, 0x0014, 0x0020, 0x0024, 0x0028,
        0x002C, 0x0030, 0x0034, 0x0040, 0x0044, 0x0048, 0x004C, 0x0050, 0x0054,
        0x0060, 0x0064, 0x0068, 0x006C, 0x0070, 0x0074, 0x0080, 0x0084, 0x0088,
        0x008C, 0x0090, 0x0094, 0x00A0, 0x00A4, 0x00A8, 0x00AC, 0x00B0, 0x00B4,
        0x00C0, 0x00C4, 0x00C8, 0x00CC, 0x00D0, 0x00D4, 0x00E0, 0x00E4, 0x00E8,
        0x00EC, 0x00F0, 0x00F4, 0x0100, 0x0104, 0x0108, 0x010C, 0x0110, 0x0114,
        0x0120, 0x0124, 0x0128, 0x012C, 0x0130, 0x0134, 0x0140, 0x0144, 0x0148,
        0x014C, 0x0150, 0x0154, 0x0160, 0x0164, 0x0168, 0x016C, 0x0170, 0x0174,
        0x0180, 0x0184, 0x0188, 0x018C, 0x0190, 0x0194, 0x01A0, 0x01A4, 0x01A8,
        0x01AC, 0x01B0, 0x01B4, 0x01C0, 0x01C4, 0x01C8, 0x01CC, 0x01D0, 0x01D4,
        0x0700, 0x0704, 0x0708, 0x070C, 0x0710, 0x0714, 0x0718, 0x071C, 0x0720,
        0x0724, 0x0728, 0x072C, 0x0730, 0x0734, 0x0738, 0x0800, 0x0804, 0x0808,
        0x080C, 0x0810, 0x0814, 0x0818, 0x081C, 0x0820, 0x0824, 0x0828, 0x082C,
        0x0830, 0x0834, 0x0838, 0x0900, 0x0904, 0x0908, 0x090C, 0x0910, 0x0914,
        0x0918, 0x091C, 0x0920, 0x0924, 0x0928, 0x092C, 0x0930, 0x0934, 0x0938,
        0x0A00, 0x0A04, 0x0A08, 0x0A0C, 0x0A10, 0x0A14, 0x0A18, 0x0A1C, 0x0A20,
        0x0A24, 0x0A28, 0x0A2C, 0x0A30, 0x0A34, 0x0A38, 0x0B14, 0x0B18, 0x0B1C,
        0x0B20, 0x0B24, 0x0B28, 0x0B2C, 0x0B30, 0x0B34, 0x0B38, 0x0B3C, 0x0B40,
        0x0B44, 0x0B48, 0x0B4C, 0xFFFF,
};
#endif

int contexthub_blk_poweron(struct contexthub_ipc_info *chub)
{
	/* only for soc default power down */
#if defined(CONFIG_SOC_EXYNOS9110)
	int val, ret;
	int trycnt = 0;

	if ((IS_ERR_OR_NULL(chub->pmu_osc_rco)) ||
	    (IS_ERR_OR_NULL(chub->pmu_chub_ctrl)) ||
	    (IS_ERR_OR_NULL(chub->pmu_chub_reset_stat))){
		nanohub_err("%s: pmu registers are not completed\n", __func__);
		return -1;
	}
	/* pmu MUX Unset */
	val = __raw_readl(chub->pmu_osc_rco + REG_CTRL_REFCLK_PMU);
	__raw_writel((val & ~(0x1 << 4)), chub->pmu_osc_rco + REG_CTRL_REFCLK_PMU);

	val = __raw_readl(chub->pmu_osc_rco + REG_CTRL_REFCLK_CHUB_VTS);
	__raw_writel((val & ~(0x1 << 4)), chub->pmu_osc_rco + REG_CTRL_REFCLK_CHUB_VTS);

	/* CHUB Block Reset Release */
	val = __raw_readl(chub->pmu_chub_ctrl);
	__raw_writel((val | (0x1 << 9)), chub->pmu_chub_ctrl);

	/* Check Reset Sequence Status */
	do {
		msleep(WAIT_TIMEOUT_MS / 1000);
		val = __raw_readl(chub->pmu_chub_reset_stat);
		val = (val >> 12) & 0x7;
		if (++trycnt > WAIT_TRY_CNT)
		break;
	} while (val != 0x5);

	/* pmu MUX Set */
	val = __raw_readl(chub->pmu_osc_rco + REG_CTRL_REFCLK_PMU);
	__raw_writel((val | (0x1 << 4)), chub->pmu_osc_rco + REG_CTRL_REFCLK_PMU);

	val = __raw_readl(chub->pmu_osc_rco + REG_CTRL_REFCLK_CHUB_VTS);
	__raw_writel((val | (0x1 << 4)), chub->pmu_osc_rco + REG_CTRL_REFCLK_CHUB_VTS);

	ret = exynos_smc(SMC_CMD_CONN_IF,
			 (uint64_t)EXYNOS_CHUB << 32 | EXYNOS_SET_CONN_TZPC, 0, 0);
	if (ret) {
		nanohub_err("%s: exynos_smc failed\n", __func__);
		return ret;
	}
        /* pmu rtc_control Set */
	val = __raw_readl(chub->pmu_rtc_ctrl);
	__raw_writel((val | (0x1 << 0)), chub->pmu_rtc_ctrl);

	/* Set CMU_CHUB CHUB_BUS as 49.152Mhz CLK_RCO_VTS in FW */
	chub->clkrate = 24576000 * 2;
	nanohub_info("%s clk selection of CMU_CHUB is %lu.\n", __func__, chub->clkrate);
#elif defined(CONFIG_SOC_S5E5515)
	return cal_chub_on();
#endif
	return 0;
}

#if defined(CONFIG_EXYNOS_LPD)
u64 get_time_lpd(void)
{
	struct tm time;
	union lpd_time dst;

	time64_to_tm(ktime_get_real_seconds() - (sys_tz.tz_minuteswest * 60), 0, &time);
	dst.tm_sec = time.tm_sec;
	dst.tm_min = time.tm_min;
	dst.tm_hour = time.tm_hour;
	dst.tm_mday = time.tm_mday;
	dst.tm_mon = time.tm_mon;
	dst.tm_year = time.tm_year;
	dst.tm_wday = time.tm_wday;
	dst.tm_yday = time.tm_yday;

	nanohub_info("%s: %04u-%02u-%02u %02u:%02u:%02u (tz %d) (wday %u) (yday %u)\n", __func__,
		     1900 + dst.tm_year, dst.tm_mon + 1, dst.tm_mday,
		     dst.tm_hour, dst.tm_min, dst.tm_sec,
		     -sys_tz.tz_minuteswest/60, dst.tm_wday, dst.tm_yday);

	return dst.lpd_time64;
}

void get_memory_lpd(struct contexthub_ipc_info *chub)
{
	int i;
	struct device_node *lpd;
	struct reserved_mem *lpd_rmem;
	unsigned long base, size;

	if (IS_ENABLED(CONFIG_SOC_S5E5515)) {
		for (i = 0 ; i < BAAW_MAX_WINDOWS * 4 ; i++) {
			if (!chub->baaw_info.baaw_d_chub_values[i])
				break;
		}
		if (i > (BAAW_MAX_WINDOWS - 1) * 4) {
			dev_err(chub->dev, "%s: no space in baaw window!\n", __func__);
			return;
		}
		lpd = of_parse_phandle(chub->dev->of_node, "memory-region", 1);
		if (!lpd) {
			dev_err(chub->dev, "%s: lpd node not found\n", __func__);
			return;
		}
		lpd_rmem = of_reserved_mem_lookup(lpd);
		if (!lpd_rmem) {
			dev_err(chub->dev, "%s: lpd_rmem not found\n", __func__);
			return;
		}

		dev_info(chub->dev, "%s: %s: 0x%x, 0x%x\n", __func__,
			     lpd_rmem->name, lpd_rmem->base, lpd_rmem->size);
		base = (unsigned long)lpd_rmem->base >> 12;
		size = (unsigned long)lpd_rmem->size >> 12;
		chub->baaw_info.baaw_d_chub_values[i++] = 0x40000 + base;
		chub->baaw_info.baaw_d_chub_values[i++] = 0x40000 + base + size;
		chub->baaw_info.baaw_d_chub_values[i++] = base;
		chub->baaw_info.baaw_d_chub_values[i++] = BAAW_RW_ACCESS_ENABLE;
	}
}

void contexthub_peri_gpio_value(struct contexthub_ipc_info *chub)
{
	int i;
	struct gpio_peri *dst;

	dst = ipc_get_base(IPC_REG_DUMP) + ipc_get_size(IPC_REG_DUMP) - 1024 * 6;

	//nanohub_info("%s start at %x\n", __func__, ipc_get_size(IPC_REG_DUMP) - 1024 * 6);
	for (i = 0 ; gpio_peri_addr[i] != 0xffff ; i++, dst++) {
		dst->value = __raw_readl(chub->gpio_peri + gpio_peri_addr[i]);
	}

}

void contexthub_peri_gpio_addr(void)
{
	int i;
	struct gpio_peri *dst;

	dst = ipc_get_base(IPC_REG_DUMP) + ipc_get_size(IPC_REG_DUMP) - 1024 * 6;
	//nanohub_info("%s start at %x\n", __func__, ipc_get_size(IPC_REG_DUMP) - 1024 * 6);
	for (i = 0 ; gpio_peri_addr[i] != 0xffff ; i++, dst++) {
		dst->addr = gpio_peri_addr[i];
	}
}
#endif

int contexthub_soc_poweron(struct contexthub_ipc_info *chub)
{
	int ret;

	/* pmu reset-release on CHUB */
	ret = cal_chub_reset_release();

	return ret;
}

int contexthub_core_reset(struct contexthub_ipc_info *chub)
{
	nanohub_err("%s: core reset is not supported\n", __func__);
	return -EINVAL;
}

int contexthub_disable_pin(struct contexthub_ipc_info *chub)
{
	int i;
	u32 irq;

	if (!chub->irq_pin_len) {
		nanohub_dev_info(chub->dev, "%s: no irq pin found\n", __func__);
		return -1;
	}

	for (i = 0; i < chub->irq_pin_len; i++) {
		irq = gpio_to_irq(chub->irq_pins[i]);
		disable_irq_nosync(irq);
		nanohub_dev_info(chub->dev, "%s: %d irq (pin:%d) is for chub. disable it\n",
				 __func__, irq, chub->irq_pins[i]);
	}

	return 0;
}

int contexthub_get_qch_base(struct contexthub_ipc_info *chub)
{
	return 0;
}

static struct clk *contexthub_devm_clk_prepare(struct device *dev, const char *name)
{
	struct clk *clk = NULL;
	int ret;

	clk = devm_clk_get(dev, name);
	if (IS_ERR(clk)) {
		nanohub_dev_err(dev, "Failed to get clock %s\n", name);
		goto error;
	}

	ret = clk_prepare(clk);
	if (ret < 0) {
		nanohub_dev_err(dev, "Failed to prepare clock %s\n", name);
		goto error;
	}

	ret = clk_enable(clk);
	if (ret < 0) {
		nanohub_dev_err(dev, "Failed to enable clock %s\n", name);
		goto error;
	}

error:
	return clk;
}

int contexthub_set_clk(struct contexthub_ipc_info *chub)
{
	struct clk *clk;

	clk = contexthub_devm_clk_prepare(chub->dev, "chub_bus");
	if (!clk)
		return -ENODEV;
	chub->clkrate = clk_get_rate(clk);
	if (!chub->clkrate) {
		dev_info(chub->dev, "clk not set, default %lu\n", chub->clkrate);
		chub->clkrate = 400000000;
	}
	return 0;
}

int contexthub_get_clock_names(struct contexthub_ipc_info *chub)
{
	return 0;
}
