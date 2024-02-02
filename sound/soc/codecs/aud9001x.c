/*
 * Copyright (c) 2014 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/samsung/abox.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/input.h>
#include <linux/completion.h>

#include <linux/mfd/samsung/s2mpw03-regulator.h>
#include <soc/samsung/acpm_mfd.h>
#include <sound/samsung/abox.h>
#include "aud9001x.h"

#define AUD9001X_SAMPLE_RATE_48KHZ	48000
#define AUD9001X_SAMPLE_RATE_192KHZ	192000

#define CONFIG_SND_SOC_SAMSUNG_VERBOSE_DEBUG 1
#ifdef CONFIG_SND_SOC_SAMSUNG_VERBOSE_DEBUG
#ifdef dev_dbg
#undef dev_dbg
#endif
#ifdef dev_info
#undef dev_info
#endif
#if 1 /* if: print option */
#define dev_dbg dev_err
#define dev_info dev_err
#else /* else: print option */
static void no_dev_dbg(void *v, char *s, ...)
{
}
#define dev_dbg no_dev_dbg
#define dev_info no_dev_dbg
#endif /* endif: print option */
#endif

static int aud9001x_disable(struct device *dev);
static int aud9001x_enable(struct device *dev);
extern void s2mpw03_ext_bus_set(bool flag);

static inline void aud9001x_usleep(unsigned int u_sec)
{
	usleep_range(u_sec, u_sec + 10);
}

static int aud9001x_pmic_read_reg(u8 reg, u8 *dest)
{
	int ret;

	ret = exynos_acpm_read_reg(PMIC_ADDR, reg, dest);

	if (ret) {
		pr_err("[%s] acpm ipc read fail!\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(aud9001x_pmic_read_reg);

static int aud9001x_pmic_write_reg(u8 reg, u8 value)
{
	int ret;

	ret = exynos_acpm_write_reg(PMIC_ADDR, reg, value);

	if (ret) {
		pr_err("[%s] acpm ipc write fail!\n", __func__);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(aud9001x_pmic_write_reg);

static int aud9001x_pmic_update_reg(u8 reg, u8 value, u8 mask)
{
	int ret;

	ret = exynos_acpm_update_reg(PMIC_ADDR, reg, value, mask);

	if (ret) {
		pr_err("[%s] acpm ipc write fail!\n", __func__);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(aud9001x_pmic_update_reg);

static int aud9001x_reg_hw_write(void *context, unsigned int reg,
		unsigned int value)
{
	return exynos_acpm_write_reg(CODEC_ADDR, (u8)reg, (u8)value);
}

static int aud9001x_reg_hw_read(void *context, unsigned int reg,
		unsigned int *value)
{
	return exynos_acpm_read_reg(CODEC_ADDR, (u8)reg, (u8 *)value);
}

static const struct regmap_bus aud9001x_bus = {
	.reg_write = aud9001x_reg_hw_write,
	.reg_read = aud9001x_reg_hw_read,
};

/**
 * Return value:
 * true: if the register value cannot be cached, hence we have to read from the
 * hardware directly
 * false: if the register value can be read from cache
 */
static bool aud9001x_volatile_register(struct device *dev, unsigned int reg)
{
	/**
	 * For all the registers for which we want to restore the value during
	 * regcache_sync operation, we need to return true here. For registers
	 * whose value need not be cached and restored should return false here.
	 *
	 * For the time being, let us cache the value of all registers other
	 * than the IRQ pending and IRQ status registers.
	 */
	switch (reg) {
	case AUD9001X_01_IRQ1 ... AUD9001X_04_IRQ4:
	case AUD9001X_60_IRQ_SENSOR:
	case AUD9001X_67_CED_CHK1 ... AUD9001X_69_CED_CHK3:
	case AUD9001X_80_PDB_ACC1 ... AUD9001X_81_PDB_ACC2:
	case AUD9001X_94_BST_CTL:
		return true;
	default:
		return false;
	}
}

/**
 * Return value:
 * true: if the register value can be read
 * flase: if the register cannot be read
 */
static bool aud9001x_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case AUD9001X_01_IRQ1 ... AUD9001X_0F_IO_CTRL1:
	case AUD9001X_10_PD_REF ... AUD9001X_1D_SV_DA:
	case AUC9001X_2B_AVC_Para40:
	case AUD9001X_32_VOL_SPK ... AUD9001X_36_CTRL_SPK:
	case AUD9001X_40_PD ... AUD9001X_4C_AVOLR1_SUB:
	case AUD9001X_50_DAC1 ... AUD9001X_54_AVC1:
	case AUD9001X_60_IRQ_SENSOR:
	case AUD9001X_67_CED_CHK1 ... AUD9001X_69_CED_CHK3:
	case AUD9001X_6C_SW_RESERV:
	case AUD9001X_71_CLK2_COD ... AUD9001X_77_CHOP_DA:
	case AUD9001X_80_PDB_ACC1 ... AUD9001X_81_PDB_ACC2:
	case AUD9001X_92_BOOST_HR0 ... AUD9001X_9F_BGC4_2:
	case AUD9001X_B0_CTR_SPK_MU1 ... AUD9001X_B2_CTR_SPK_MU3:
	case AUD9001X_F2_AVC_PARA18 ... AUD9001X_F3_AVC_PARA19:
	case AUD9001X_F9_AVC_PARA25 ... AUD9001X_FC_AVC_PARA28:
		return true;
	default:
		return false;
	}
}

static bool aud9001x_writeable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case AUD9001X_01_IRQ1 ... AUD9001X_0F_IO_CTRL1:
	case AUD9001X_10_PD_REF ... AUD9001X_1D_SV_DA:
	case AUC9001X_2B_AVC_Para40:
	case AUD9001X_32_VOL_SPK ... AUD9001X_36_CTRL_SPK:
	case AUD9001X_40_PD ... AUD9001X_4C_AVOLR1_SUB:
	case AUD9001X_50_DAC1 ... AUD9001X_54_AVC1:
	case AUD9001X_6C_SW_RESERV:
	case AUD9001X_71_CLK2_COD ... AUD9001X_77_CHOP_DA:
	case AUD9001X_80_PDB_ACC1 ... AUD9001X_81_PDB_ACC2:
	case AUD9001X_92_BOOST_HR0 ... AUD9001X_94_BST_CTL:
	case AUD9001X_97_VBAT_M0 ... AUD9001X_99_TEST_VBAT:
	case AUD9001X_9D_BGC4_0 ... AUD9001X_9F_BGC4_2:
	case AUD9001X_B0_CTR_SPK_MU1 ... AUD9001X_B2_CTR_SPK_MU3:
	case AUD9001X_F2_AVC_PARA18 ... AUD9001X_F3_AVC_PARA19:
	case AUD9001X_F9_AVC_PARA25 ... AUD9001X_FC_AVC_PARA28:
		return true;
	default:
		return false;
	}
}

/**
 * "i3c" string should be described in the name field
 * this will be used for the i3c inteface,
 * when read/write operations are used in the regmap driver.
 * APM functions will be called instead of the I2C
 * refer to the "drivers/base/regmap/regmap-i2c.c
 */
const struct regmap_config aud9001x_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.name = "i3c, AUD9001X",
	.max_register = AUD9001X_MAX_REGISTER,
	.readable_reg = aud9001x_readable_register,
	.writeable_reg = aud9001x_writeable_register,
	.volatile_reg = aud9001x_volatile_register,
	.use_single_rw = true,
	.cache_type = REGCACHE_RBTREE,
};


/*
 * aud9001x_adc_digital_mute() - Set ADC digital Mute
 *
 * @codec: SoC audio codec device
 * @channel: Digital mute control for ADC channel
 * @on: mic mute is true, mic unmute is false
 *
 * Desc: When ADC path turn on, analog block noise can be recorded.
 * For remove this, ADC path was muted always except that it was used.
 */
void aud9001x_adc_digital_mute(struct aud9001x_priv *aud9001x, bool on)
{
	struct device *dev = aud9001x->dev;

	mutex_lock(&aud9001x->adc_mute_lock);

	if (on)
		snd_soc_component_update_bits(aud9001x->codec, AUD9001X_46_ADC1,
				ADC1_MUTE_AD1_EN_MASK, ADC1_MUTE_AD1_EN_MASK);
	else
		snd_soc_component_update_bits(aud9001x->codec, AUD9001X_46_ADC1,
				ADC1_MUTE_AD1_EN_MASK, 0);

	mutex_unlock(&aud9001x->adc_mute_lock);

	dev_info(dev, "%s called, %s work done.\n", __func__, on ? "Mute" : "Unmute");
}
EXPORT_SYMBOL_GPL(aud9001x_adc_digital_mute);

/*
 * DAC(Rx) functions
 */
/*
 * aud9001x_dac_soft_mute() - Set DAC soft mute
 *
 * @codec: SoC audio codec device
 * @channel: Soft mute control for DAC channel
 * @on: dac mute is true, dac unmute is false
 *
 * Desc: When DAC path turn on, analog block noise can be played.
 * For remove this, DAC path was muted always except that it was used.
 */
static void aud9001x_dac_soft_mute(struct aud9001x_priv *aud9001x, bool on)
{
	struct device *dev = aud9001x->dev;

	if (on)
		snd_soc_component_update_bits(aud9001x->codec, AUD9001X_50_DAC1,
				DAC1_SFMU_DA_MASK, DAC1_SFMU_DA_MASK);
	else
		snd_soc_component_update_bits(aud9001x->codec, AUD9001X_50_DAC1,
				DAC1_SFMU_DA_MASK, 0);

	dev_info(dev, "%s called, %s work done.\n", __func__, on ? "Mute" : "Unmute");
}

static void aud9001x_adc_mute_work(struct work_struct *work)
{
	struct aud9001x_priv *aud9001x =
		container_of(work, struct aud9001x_priv, adc_mute_work.work);
	unsigned int chop_val;

	chop_val = snd_soc_component_read32(aud9001x->codec, AUD9001X_6C_SW_RESERV);
	dev_info(aud9001x->dev, "%s called, chop: 0x%02X\n", __func__, chop_val);

	if (aud9001x->capture_on)
		aud9001x_adc_digital_mute(aud9001x, false);
}

static void aud9001x_chk_clk(struct snd_soc_component *codec)
{
	unsigned int lrclk, bclk, temp;

	temp = snd_soc_component_read32(codec, AUD9001X_67_CED_CHK1);
	temp = (temp & 0xF);
	lrclk = (temp << 8) + snd_soc_component_read32(codec, AUD9001X_68_CED_CHK2);
	bclk = snd_soc_component_read32(codec, AUD9001X_69_CED_CHK3);

	dev_info(codec->dev, "%s called, LRCLK: 0x%04X, BCLK: 0x%02X\n",
			__func__, lrclk, bclk);
}

static int dvmid_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);

	dev_info(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* DMIC Clock Pre-Disable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				DMIC_CLK0_EN_MASK | DMIC_CLK1_EN_MASK, 0);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* DMIC Clock Pre-Disable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				DMIC_CLK0_EN_MASK | DMIC_CLK1_EN_MASK, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	}

	return 0;
}

static int dmic1_pga_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);

	dev_info(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* DMIC1 IO Enable */
		snd_soc_component_update_bits(codec, AUD9001X_0F_IO_CTRL1,
				IO_CTRL1_DMIC_DATA1_IO_EN | IO_CTRL1_DMIC_CLK1_IO_TN,
				IO_CTRL1_DMIC_DATA1_IO_EN | IO_CTRL1_DMIC_CLK1_IO_TN);

		/* DMIC Clock Gating Enable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				PDB_ADCDIG_MASK, PDB_ADCDIG_MASK);
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* DMIC1 Clock Enable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				DMIC_CLK0_EN_MASK, DMIC_CLK0_EN_MASK);

		/* Recording Digital Reset */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				RSTB_DAT_AD_MASK, 0);
		aud9001x_usleep(1000);
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				RSTB_DAT_AD_MASK, RSTB_DAT_AD_MASK);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* DMIC1 Clock Disable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				DMIC_CLK0_EN_MASK, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* DMIC Clock Gating Disable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				PDB_ADCDIG_MASK, 0);

		/* DMIC1 IO Disable */
		snd_soc_component_update_bits(codec, AUD9001X_0F_IO_CTRL1,
				IO_CTRL1_DMIC_DATA1_IO_EN | IO_CTRL1_DMIC_CLK1_IO_TN, 0);
		break;
	}

	return 0;
}

static int dmic2_pga_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);

	dev_info(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* DMIC2 IO Enable */
		snd_soc_component_update_bits(codec, AUD9001X_0F_IO_CTRL1,
				IO_CTRL1_DMIC_DATA2_IO_EN | IO_CTRL1_DMIC_CLK2_IO_TN,
				IO_CTRL1_DMIC_DATA2_IO_EN | IO_CTRL1_DMIC_CLK2_IO_TN);

		/* DMIC Clock Gating Enable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				PDB_ADCDIG_MASK, PDB_ADCDIG_MASK);
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* DMIC2 Clock Enable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				DMIC_CLK1_EN_MASK, DMIC_CLK1_EN_MASK);

		/* Recording Digital Reset */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				RSTB_DAT_AD_MASK, 0);
		aud9001x_usleep(1000);
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				RSTB_DAT_AD_MASK, RSTB_DAT_AD_MASK);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* DMIC2 Clock Disable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				DMIC_CLK1_EN_MASK, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* DMIC Clock Gating Disable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				PDB_ADCDIG_MASK, 0);

		/* DMIC2 IO Disable */
		snd_soc_component_update_bits(codec, AUD9001X_0F_IO_CTRL1,
				IO_CTRL1_DMIC_DATA2_IO_EN | IO_CTRL1_DMIC_CLK2_IO_TN, 0);
		break;
	}

	return 0;
}

static int adc_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);
	dev_info(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		aud9001x_chk_clk(codec);
		break;
	case SND_SOC_DAPM_POST_PMU:
		cancel_delayed_work(&aud9001x->adc_mute_work);
		queue_delayed_work(aud9001x->adc_mute_wq, &aud9001x->adc_mute_work,
				msecs_to_jiffies(aud9001x->dmic_delay));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		aud9001x_adc_digital_mute(aud9001x, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	}

	return 0;
}

static int dac_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);

	dev_info(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* DCT Clock Invert */
		snd_soc_component_update_bits(codec, AUD9001X_72_CLK3_COD,
				CLK_DA_INV_MASK, CLK_DA_INV_MASK);
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* Cap Init */
		snd_soc_component_write(codec, AUD9001X_DB_CTRL_SPKS1, 0xA8);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	}

	return 0;
}

static int spkdrv_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	dev_info(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (aud9001x->regulator_count == 0) {
			aud9001x->regulator_count = 1;
			regulator_enable(aud9001x->vdd2);
			dev_info(codec->dev, "%s enable vdd2: %d\n",
					__func__, aud9001x->regulator_count);
			aud9001x_usleep(1000);
		}
		/* Boost Control Enable */
		snd_soc_component_update_bits(codec, AUD9001X_94_BST_CTL,
				BOOST_CTRL_EN_MASK, BOOST_CTRL_EN_MASK);
		snd_soc_component_update_bits(codec, AUD9001X_94_BST_CTL,
				CTBV_BST_PRESET_MASK, 0);

		/* Digital clock Gating Enable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				PDB_DACDIG_MASK, PDB_DACDIG_MASK);

		aud9001x_chk_clk(codec);
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* SPK APW On */
		snd_soc_component_update_bits(codec, AUD9001X_18_PWAUTO_DA,
				APW_SPK_MASK, APW_SPK_MASK);

		/* SPK AMP delay */
		msleep(105);

		/* DCT Pre-Charge Off */
		snd_soc_component_update_bits(codec, AUD9001X_14_PD_DA1,
				EN_DCT_PREQ_MASK, 0);

		/* Playback Digital Reset */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				RSTB_DAT_DA_MASK, 0);
		aud9001x_usleep(1000);
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				RSTB_DAT_DA_MASK, RSTB_DAT_DA_MASK);

		/* DAC Unmute */
		aud9001x_dac_soft_mute(aud9001x, false);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* DAC Mute */
		aud9001x_dac_soft_mute(aud9001x, true);

		/* SPK APW Off */
		snd_soc_component_update_bits(codec, AUD9001X_18_PWAUTO_DA,
				APW_SPK_MASK, 0);

		aud9001x_usleep(1000);

		/* DCT Pre-Charge On */
		snd_soc_component_update_bits(codec, AUD9001X_14_PD_DA1,
				EN_DCT_PREQ_MASK, EN_DCT_PREQ_MASK);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Digital clock Gating Disable */
		snd_soc_component_update_bits(codec, AUD9001X_40_PD,
				PDB_DACDIG_MASK, 0);

		/* Boost Control Disable */
		snd_soc_component_update_bits(codec, AUD9001X_94_BST_CTL,
				BOOST_CTRL_EN_MASK, 0);
		snd_soc_component_update_bits(codec, AUD9001X_94_BST_CTL,
				CTBV_BST_PRESET_MASK, CTBV_BST_PRESET_MASK);
		if (aud9001x->regulator_count == 1) {
			aud9001x->regulator_count = 0;
			regulator_disable(aud9001x->vdd2);
			dev_info(codec->dev, "%s disable vdd2: %d\n",
					__func__, aud9001x->regulator_count);
		}
		break;
	}

	return 0;
}

/**
 * aud9001x_dvol_adc_dac_tlv
 *
 * Map as per data-sheet:
 * 0x00 ~ 0xE0 : +42dB to -70dB, step 0.5dB
 * 0xE1 ~ 0xE5 : -72dB to -80dB, step 2dB
 * 0xE6 : -82.4dB
 * 0xE7 ~ 0xE9 : -84.3dB to -96.3dB, step 6dB
 *
 * When the map is in descending order, we need to set the invert bit
 * and arrange the map in ascending order. The offsets are calculated as
 * (max - offset).
 *
 * offset_in_table = max - offset_actual;
 *
 * DAC_DAL, reg(0x51), shift(0), width(8), invert(1), max(0xE9)
 * DAC_DAR, reg(0x52), shift(0), width(8), invert(1), max(0xE9)
 */
static const unsigned int aud9001x_dvol_adc_dac_tlv[] = {
	TLV_DB_RANGE_HEAD(4),
	0x00, 0x02, TLV_DB_SCALE_ITEM(-9630, 600, 0),
	0x03, 0x03, TLV_DB_SCALE_ITEM(-8240, 0, 0),
	0x04, 0x09, TLV_DB_SCALE_ITEM(-8000, 200, 0),
	0x0A, 0xE9, TLV_DB_SCALE_ITEM(-6950, 50, 0),
};

/**
 * aud9001x_dvol_dmic_tlv
 *
 * Map as per data-sheet:
 * 0x00 ~ 0x0F : +0 to 7.5 level, step 0.5 level
 *
 */
static const unsigned int aud9001x_dvol_dmic_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	0x00, 0x0F, TLV_DB_SCALE_ITEM(-750, 50, 0),
};

/**
 * aud9001x_ng_threshold_tlv
 * 0x00 ~ 0x81 : 0 to 130 level, step 1 level
 *
 * Threshold = tlv - 130dB
 */
static const unsigned int aud9001x_ng_threshold_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	0x00, 0x82, TLV_DB_SCALE_ITEM(-13000, 100, 0),
};

/**
 * aud9001x_ng_lvl_tlv
 * 0x00 ~ 0xE9 : 0 to 233 level, step 1 level
 */
static const unsigned int aud9001x_ng_lvl_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	0x00, 0xE9, TLV_DB_SCALE_ITEM(0, 100, 0),
};

static int dmic_bias1_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	u8 dmic_bias;
	int dmic_bias_flag;

	aud9001x_pmic_read_reg(PMIC_DMIC_BIAS_REG, &dmic_bias);
	dmic_bias_flag = dmic_bias & DMIC_BIAS1_EN_MASK;

	ucontrol->value.integer.value[0] = dmic_bias_flag;

	return 0;
}

static int dmic_bias1_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	int value = ucontrol->value.integer.value[0];

	if (value)
		aud9001x_pmic_update_reg(PMIC_DMIC_BIAS_REG,
				DMIC_BIAS1_EN_MASK, DMIC_BIAS1_EN_MASK);
	else
		aud9001x_pmic_update_reg(PMIC_DMIC_BIAS_REG,
				0, DMIC_BIAS1_EN_MASK);

	dev_info(codec->dev, "%s: dmic bias1 : %s\n", __func__,
			(value) ? "on":"off");

	return 0;
}

static int dmic_bias2_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	u8 dmic_bias;
	int dmic_bias_flag;

	aud9001x_pmic_read_reg(PMIC_DMIC_BIAS_REG, &dmic_bias);
	dmic_bias_flag = dmic_bias & DMIC_BIAS2_EN_MASK;

	ucontrol->value.integer.value[0] = dmic_bias_flag;

	return 0;
}

static int dmic_bias2_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	int value = ucontrol->value.integer.value[0];

	if (value)
		aud9001x_pmic_update_reg(PMIC_DMIC_BIAS_REG,
				DMIC_BIAS2_EN_MASK, DMIC_BIAS2_EN_MASK);
	else
		aud9001x_pmic_update_reg(PMIC_DMIC_BIAS_REG,
				0, DMIC_BIAS2_EN_MASK);

	dev_info(codec->dev, "%s: dmic bias2 : %s\n", __func__,
			(value) ? "on":"off");

	return 0;
}

/*
 * aud9001x_adc_dat_src - I2S channel input data selection
 *
 * Map as per data-sheet:
 * 00 : ADC Left Channel Data
 * 01 : ADC Right Channel Data
 * 10 : Not used
 * 11 : Fuel-gauge output data
 *
 * SEL_ADC0 : reg(0x44), shift(0), width(2)
 * SEL_ADC1 : reg(0x44), shift(2), width(2)
 * SEL_ADC2 : reg(0x44), shift(4), width(2)
 * SEL_ADC3 : reg(0x44), shift(6), width(2)
 */

static const char * const aud9001x_adc_dat_src[] = {
	"ADCL", "ADCR", "Zero", "FGDATA"
};

static SOC_ENUM_SINGLE_DECL(aud9001x_adc_dat_enum0, AUD9001X_44_IF1_FORM4,
		SEL_ADC0_SHIFT, aud9001x_adc_dat_src);

static SOC_ENUM_SINGLE_DECL(aud9001x_adc_dat_enum1, AUD9001X_44_IF1_FORM4,
		SEL_ADC1_SHIFT, aud9001x_adc_dat_src);

static SOC_ENUM_SINGLE_DECL(aud9001x_adc_dat_enum2, AUD9001X_44_IF1_FORM4,
		SEL_ADC2_SHIFT, aud9001x_adc_dat_src);

static SOC_ENUM_SINGLE_DECL(aud9001x_adc_dat_enum3, AUD9001X_44_IF1_FORM4,
		SEL_ADC3_SHIFT, aud9001x_adc_dat_src);

static int dmic_delay_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	ucontrol->value.integer.value[0] = aud9001x->dmic_delay;

	return 0;
}

static int dmic_delay_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);
	int value = ucontrol->value.integer.value[0];

	aud9001x->dmic_delay = value;

	return 0;
}

static const char * const aud9001x_hpf_sel_text[] = {
	"HPF-238Hz", "HPF-200Hz", "HPF-100Hz", "HPF-15Hz"
};

static SOC_ENUM_SINGLE_DECL(aud9001x_hpf_sel_enum, AUD9001X_46_ADC1,
		ADC1_HPF_SEL1_SHIFT, aud9001x_hpf_sel_text);

static const char * const aud9001x_hpf_channel_text[] = {
	"Off", "On"
};

static SOC_ENUM_SINGLE_DECL(aud9001x_hpf_channel_enum, AUD9001X_46_ADC1,
		ADC1_HPF_EN1_SHIFT, aud9001x_hpf_channel_text);

/**
 * mono_mix_mode
 *
 * Selecting the Mode of Mono Mixer (inside DAC block)
 */
static const char * const aud9001x_mono_mix_mode_text[] = {
	"Disable", "R", "L", "LR-Invert",
	"(L+R)/2", "L+R"
};

static SOC_ENUM_SINGLE_DECL(aud9001x_mono_mix_mode_enum, AUD9001X_50_DAC1,
		DAC1_MONOMIX_SHIFT, aud9001x_mono_mix_mode_text);

/*
 * aud9001x_dac_dat_src - I2S channel input data selection
 *
 * Map as per data-sheet:
 * 00 : DAC1 Output Data
 * 01 : DAC2 Output Data
 * 10 : DAC3 Output Data
 * 11 : DAT4 Output Data
 *
 * SEL_DAC0 : reg(0x45), shift(0), width(2)
 * SEL_DAC1 : reg(0x45), shift(4), width(2)
 */

static const char * const aud9001x_dac_dat_src[] = {
	"DAC1", "DAC2", "DAC3", "DAC4"
};

static SOC_ENUM_SINGLE_DECL(aud9001x_dac_dat_enum0, AUD9001X_45_IF1_FORM5,
		SEL_DAC0_SHIFT, aud9001x_dac_dat_src);

static SOC_ENUM_SINGLE_DECL(aud9001x_dac_dat_enum1, AUD9001X_45_IF1_FORM5,
		SEL_DAC1_SHIFT, aud9001x_dac_dat_src);

static int codec_enable_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	ucontrol->value.integer.value[0] = !(aud9001x->is_suspend);

	return 0;
}

static int codec_enable_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	int value = ucontrol->value.integer.value[0];

	if (value)
		aud9001x_enable(codec->dev);
	else
		aud9001x_disable(codec->dev);

	dev_info(codec->dev, "%s: codec enable : %s\n", __func__,
			(value) ? "on":"off");

	return 0;
}

static int noise_gate_adcc_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	unsigned int val;

	val = snd_soc_component_read32(codec, AUD9001X_FC_AVC_PARA28);
	val = (val & PARA28_NG_EN_ADCC_MASK) >> PARA28_NG_EN_ADCC_SHIFT;
	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int noise_gate_adcc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	int value = ucontrol->value.integer.value[0];

	if (value)
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_ADCC_MASK, PARA28_NG_EN_ADCC_MASK);
	else
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_ADCC_MASK, 0);

	dev_info(codec->dev, "%s: NoiseGate ADCC : %s\n", __func__,
			(value) ? "enable":"disable");

	return 0;
}

static int noise_gate_adcr_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	unsigned int val;

	val = snd_soc_component_read32(codec, AUD9001X_FC_AVC_PARA28);
	val = (val & PARA28_NG_EN_ADCR_MASK) >> PARA28_NG_EN_ADCR_SHIFT;
	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int noise_gate_adcr_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	int value = ucontrol->value.integer.value[0];

	if (value)
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_ADCR_MASK, PARA28_NG_EN_ADCR_MASK);
	else
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_ADCR_MASK, 0);

	dev_info(codec->dev, "%s: NoiseGate ADCR : %s\n", __func__,
			(value) ? "enable":"disable");

	return 0;
}

static int noise_gate_adcl_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	unsigned int val;

	val = snd_soc_component_read32(codec, AUD9001X_FC_AVC_PARA28);
	val = (val & PARA28_NG_EN_ADCL_MASK) >> PARA28_NG_EN_ADCL_SHIFT;
	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int noise_gate_adcl_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	int value = ucontrol->value.integer.value[0];

	if (value)
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_ADCL_MASK, PARA28_NG_EN_ADCL_MASK);
	else
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_ADCL_MASK, 0);

	dev_info(codec->dev, "%s: NoiseGate ADCL : %s\n", __func__,
			(value) ? "enable":"disable");

	return 0;
}

static int noise_gate_rcv_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	unsigned int val;

	val = snd_soc_component_read32(codec, AUD9001X_FC_AVC_PARA28);
	val = (val & PARA28_NG_EN_RCV_MASK) >> PARA28_NG_EN_RCV_SHIFT;
	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int noise_gate_rcv_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = snd_soc_kcontrol_component(kcontrol);
	int value = ucontrol->value.integer.value[0];

	if (value)
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_RCV_MASK, PARA28_NG_EN_RCV_MASK);
	else
		snd_soc_component_update_bits(codec, AUD9001X_FC_AVC_PARA28,
				PARA28_NG_EN_RCV_MASK, 0);

	dev_info(codec->dev, "%s: NoiseGate RCV : %s\n", __func__,
			(value) ? "enable":"disable");

	return 0;
}


/**
 * struct snd_kcontrol_new aud9001x_snd_control
 *
 * Every distinct bit-fields within the CODEC SFR range may be considered
 * as a control elements. Such control elements are defined here.
 *
 * Depending on the access mode of these registers, different macros are
 * used to define these control elements.
 *
 * SOC_ENUM: 1-to-1 mapping between bit-field value and provided text
 * SOC_SINGLE: Single register, value is a number
 * SOC_SINGLE_TLV: Single register, value corresponds to a TLV scale
 * SOC_SINGLE_TLV_EXT: Above + custom get/set operation for this value
 * SOC_SINGLE_RANGE_TLV: Register value is an offset from minimum value
 * SOC_DOUBLE: Two bit-fields are updated in a single register
 * SOC_DOUBLE_R: Two bit-fields in 2 different registers are updated
 */

/**
 * All the data goes into aud9001x_snd_controls.
 * All path inter-connections goes into aud9001x_dapm_routes
 */
static const struct snd_kcontrol_new aud9001x_snd_controls[] = {
	/*
	 * ADC(Tx) path control
	 */
	SOC_SINGLE_TLV("ADC Left Gain", AUD9001X_4B_AVOLL1_SUB,
			AD_DVOL_SHIFT,
			AD_DVOL_MAXNUM, 1, aud9001x_dvol_adc_dac_tlv),

	SOC_SINGLE_TLV("ADC Right Gain", AUD9001X_4C_AVOLR1_SUB,
			AD_DVOL_SHIFT,
			AD_DVOL_MAXNUM, 1, aud9001x_dvol_adc_dac_tlv),

	SOC_SINGLE_TLV("DMIC Gain", AUD9001X_4A_DMIC_CTR2,
			DMIC_GAIN_SHIFT,
			DMIC_GAIN_LEVEL_7_5, 0, aud9001x_dvol_dmic_tlv),

	SOC_SINGLE_EXT("DMIC Bias1", SND_SOC_NOPM, 0, 1, 0,
			dmic_bias1_get, dmic_bias1_put),

	SOC_SINGLE_EXT("DMIC Bias2", SND_SOC_NOPM, 0, 1, 0,
			dmic_bias2_get, dmic_bias2_put),

	SOC_ENUM("ADC DAT Mux0", aud9001x_adc_dat_enum0),

	SOC_ENUM("ADC DAT Mux1", aud9001x_adc_dat_enum1),

	SOC_ENUM("ADC DAT Mux2", aud9001x_adc_dat_enum2),

	SOC_ENUM("ADC DAT Mux3", aud9001x_adc_dat_enum3),

	SOC_SINGLE_EXT("DMIC delay", SND_SOC_NOPM, 0, 1000, 0,
			dmic_delay_get, dmic_delay_put),

	/*
	 * HPF Tuning Control
	 */
	SOC_ENUM("HPF Tuning", aud9001x_hpf_sel_enum),

	SOC_ENUM("HPF Center", aud9001x_hpf_channel_enum),

	/*
	 * DAC(Rx) path control
	 */
	SOC_DOUBLE_R_TLV("DAC Gain", AUD9001X_51_DVOLL, AUD9001X_52_DVOLR,
			DA_DVOL_SHIFT,
			DA_DVOL_MAXNUM, 1, aud9001x_dvol_adc_dac_tlv),

	SOC_ENUM("MonoMix Mode", aud9001x_mono_mix_mode_enum),

	SOC_ENUM("DAC DAT Mux0", aud9001x_dac_dat_enum0),

	SOC_ENUM("DAC DAT Mux1", aud9001x_dac_dat_enum1),

 	/*
	 * Noise Gate control
	 */
	SOC_SINGLE_EXT("NoiseGate ADCL", SND_SOC_NOPM, 0, 1, 0,
			noise_gate_adcl_get, noise_gate_adcl_put),
	SOC_SINGLE_EXT("NoiseGate ADCR", SND_SOC_NOPM, 0, 1, 0,
			noise_gate_adcr_get, noise_gate_adcr_put),
	SOC_SINGLE_EXT("NoiseGate ADCC", SND_SOC_NOPM, 0, 1, 0,
			noise_gate_adcc_get, noise_gate_adcc_put),
	SOC_SINGLE_EXT("NoiseGate RCV", SND_SOC_NOPM, 0, 1, 0,
			noise_gate_rcv_get, noise_gate_rcv_put),

	SOC_SINGLE_TLV("NG Threshold", AUD9001X_F9_AVC_PARA25,
			AVC_ADC_Channel_MUTE_LVL_SHIFT,
			ADC_NG_THRESHOLD_MAX, 0, aud9001x_ng_threshold_tlv),

	SOC_SINGLE_TLV("NG Level", AUD9001X_FA_AVC_PARA26,
			AVC_ADC_NOISE_GATE_LVL_SHIFT,
			ADC_NG_LVL_MAX, 0, aud9001x_ng_lvl_tlv),

	/*
	 * Codec control
	 */
	SOC_SINGLE_EXT("Codec Enable", SND_SOC_NOPM, 0, 1, 0,
			codec_enable_get, codec_enable_put),
};

/* DMIC1 DMIC1 L [6:4] */
static const char * const aud9001x_dmicl_src1[] = {
	"Zero Data1", "Zero Data2", "Zero Data3", "Not Use",
	"DMIC1L ADCL", "DMIC1R ADCL", "DMIC2L ADCL", "DMIC2R ADCL"
};

/* DMIC1 DMIC1 R [2:0] */
static const char * const aud9001x_dmicr_src1[] = {
	"Zero Data1", "Zero Data2", "Zero Data3", "Not Use",
	"DMIC1L ADCR", "DMIC1R ADCR", "DMIC2L ADCR", "DMIC2R ADCR"
};

static SOC_ENUM_SINGLE_DECL(aud9001x_dmicl_enum1, AUD9001X_49_DMIC_CTR1,
		SEL_DMIC_1L_SHIFT, aud9001x_dmicl_src1);

static SOC_ENUM_SINGLE_DECL(aud9001x_dmicr_enum1, AUD9001X_49_DMIC_CTR1,
		SEL_DMIC_1R_SHIFT, aud9001x_dmicr_src1);

static const struct snd_kcontrol_new aud9001x_dmicl_mux1 =
		SOC_DAPM_ENUM("DMICL Mux1", aud9001x_dmicl_enum1);

static const struct snd_kcontrol_new aud9001x_dmicr_mux1 =
		SOC_DAPM_ENUM("DMICR Mux1", aud9001x_dmicr_enum1);

static const struct snd_kcontrol_new spk_on[] = {
	SOC_DAPM_SINGLE("SPK On", AUD9001X_6C_SW_RESERV,
			EN_SPK_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new dmic1_on[] = {
	SOC_DAPM_SINGLE("DMIC1 On", AUD9001X_6C_SW_RESERV,
			EN_DMIC1_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new dmic2_on[] = {
	SOC_DAPM_SINGLE("DMIC2 On", AUD9001X_6C_SW_RESERV,
			EN_DMIC2_SHIFT, 1, 0),
};

static const struct snd_soc_dapm_widget aud9001x_dapm_widgets[] = {
	/*
	 * ADC(Tx) dapm widget
	 */
	SND_SOC_DAPM_INPUT("IN1L"),
	SND_SOC_DAPM_INPUT("IN2L"),

	SND_SOC_DAPM_SUPPLY("DVMID", SND_SOC_NOPM, 0, 0, dvmid_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("DMIC1_PGA", SND_SOC_NOPM, 0, 0, NULL, 0, dmic1_pga_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("DMIC2_PGA", SND_SOC_NOPM, 0, 0, NULL, 0, dmic2_pga_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SWITCH("DMIC1", SND_SOC_NOPM, 0, 0, dmic1_on),
	SND_SOC_DAPM_SWITCH("DMIC2", SND_SOC_NOPM, 0, 0, dmic2_on),

	SND_SOC_DAPM_MUX("DMICL Mux1", SND_SOC_NOPM, 0, 0, &aud9001x_dmicl_mux1),
	SND_SOC_DAPM_MUX("DMICR Mux1", SND_SOC_NOPM, 0, 0, &aud9001x_dmicr_mux1),

	SND_SOC_DAPM_ADC_E("ADC", "AIF Capture", SND_SOC_NOPM, 0, 0, adc_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_ADC_E("ADC", "AIF2 Capture", SND_SOC_NOPM, 0, 0, adc_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),

	/*
	 * DAC(Rx) dapm widget
	 */
	SND_SOC_DAPM_SWITCH("SPK", SND_SOC_NOPM, 0, 0, spk_on),

	SND_SOC_DAPM_OUT_DRV_E("SPKDRV", SND_SOC_NOPM, 0, 0, NULL, 0, spkdrv_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_DAC_E("DAC", "AIF Playback", SND_SOC_NOPM, 0, 0, dac_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DAC_E("DAC", "AIF2 Playback", SND_SOC_NOPM, 0, 0, dac_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_OUTPUT("SPKOUTLN"),
};

static const struct snd_soc_dapm_route aud9001x_dapm_routes[] = {
	/*
	 * ADC(Tx) dapm route
	 */

	{"DMIC1_PGA", NULL, "IN1L"},
	{"DMIC1_PGA", NULL, "DVMID"},
	{"DMIC1", "DMIC1 On", "DMIC1_PGA"},

	{"DMIC2_PGA", NULL, "IN2L"},
	{"DMIC2_PGA", NULL, "DVMID"},
	{"DMIC2", "DMIC2 On", "DMIC2_PGA"},

	{"DMICL Mux1", "DMIC1L ADCL", "DMIC1"},
	{"DMICL Mux1", "DMIC1R ADCL", "DMIC1"},
	{"DMICL Mux1", "DMIC2L ADCL", "DMIC2"},
	{"DMICL Mux1", "DMIC2R ADCL", "DMIC2"},

	{"DMICR Mux1", "DMIC1L ADCR", "DMIC1"},
	{"DMICR Mux1", "DMIC1R ADCR", "DMIC1"},
	{"DMICR Mux1", "DMIC2L ADCR", "DMIC2"},
	{"DMICR Mux1", "DMIC2R ADCR", "DMIC2"},

	{"ADC", NULL, "DMICL Mux1"},
	{"ADC", NULL, "DMICR Mux1"},

	{"AIF Capture", NULL, "ADC"},
	{"AIF2 Capture", NULL, "ADC"},

	/*
	 * DAC(Rx) dapm route
	 */
	{"DAC", NULL, "AIF Playback"},
	{"DAC", NULL, "AIF2 Playback"},

	{"SPKDRV", NULL, "DAC"},
	{"SPK", "SPK On", "SPKDRV"},
	{"SPKOUTLN", NULL, "SPK"},
};

static int aud9001x_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *codec = dai->component;

	dev_dbg(codec->dev, "%s called. fmt: %d\n", __func__, fmt);

	return 0;
}

static int aud9001x_dai_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_component *codec = dai->component;

	dev_dbg(codec->dev, "(%s) %s completed\n",
			substream->stream ? "C" : "P", __func__);

	return 0;
}

static void aud9001x_dai_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_component *codec = dai->component;
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	dev_dbg(codec->dev, "(%s) %s completed\n",
			substream->stream ? "C" : "P", __func__);

	if (substream->stream)
		aud9001x->capture_on = false;
	else
		aud9001x->playback_on = false;
}


static int aud9001x_dai_hw_free(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_component *codec = dai->component;

	dev_dbg(codec->dev, "(%s) %s completed\n",
			substream->stream ? "C" : "P", __func__);

	return 0;
}

/*
 * playback_hw_params() - Register setting for playback
 *
 * @codec: SoC audio codec device
 * @cur_aifrate: current sample rate
 *
 * Desc: Set codec register related sample rate format before playback.
 */
static void playback_hw_params(struct snd_soc_component *codec,
		unsigned int cur_aifrate)
{
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	dev_info(codec->dev, "%s called. priv_aif: %d, cur_aif %d\n",
			__func__, aud9001x->playback_aifrate, cur_aifrate);

	aud9001x->playback_on = true;
}

/*
 * capture_hw_params() - Register setting for capture
 *
 * @codec: SoC audio codec device
 * @cur_aifrate: current sample rate
 *
 * Desc: Set codec register related sample rate format before capture.
 */
static void capture_hw_params(struct snd_soc_component *codec,
		unsigned int cur_aifrate)
{
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	dev_info(codec->dev, "%s called. priv_aif: %d, cur_aif %d\n",
			__func__, aud9001x->capture_aifrate, cur_aifrate);
	aud9001x->capture_on = true;

	aud9001x_adc_digital_mute(aud9001x, true);
}

static int aud9001x_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_component *codec = dai->component;
	unsigned int cur_aifrate, width, channels;

	cur_aifrate = params_rate(params);
	width = params_width(params);
	channels = params_channels(params);

	dev_info(codec->dev, "(%s) %s called. aifrate: %d, width: %d, channels: %d\n",
			substream->stream ? "C" : "P", __func__, cur_aifrate, width, channels);

	switch (width) {
	case BIT_RATE_16:
		/* I2S 16bit setting */
		snd_soc_component_update_bits(codec, AUD9001X_42_IF1_FORM2,
				I2S1_DL_MASK, I2S1_DL_16BIT << I2S1_DL_SHIFT);

		/* I2S 32fs setting */
		snd_soc_component_update_bits(codec, AUD9001X_43_IF1_FORM3,
				I2S1_XFS_MASK, I2S1_XFS_32FS << I2S1_XFS_SHIFT);
		break;
	case BIT_RATE_24:
		/* I2S 24bit setting */
		snd_soc_component_update_bits(codec, AUD9001X_42_IF1_FORM2,
				I2S1_DL_MASK, I2S1_DL_24BIT << I2S1_DL_SHIFT);

		/* I2S 48fs setting */
		snd_soc_component_update_bits(codec, AUD9001X_43_IF1_FORM3,
				I2S1_XFS_MASK, I2S1_XFS_48FS << I2S1_XFS_SHIFT);
		break;
	case BIT_RATE_32:
		/* I2S 32bit setting */
		snd_soc_component_update_bits(codec, AUD9001X_42_IF1_FORM2,
				I2S1_DL_MASK, I2S1_DL_32BIT << I2S1_DL_SHIFT);

		/* I2S 64fs setting */
		snd_soc_component_update_bits(codec, AUD9001X_43_IF1_FORM3,
				I2S1_XFS_MASK, I2S1_XFS_64FS << I2S1_XFS_SHIFT);
		break;
	default:
		dev_err(codec->dev, "%s: bit rate error!\n", __func__);
		/* I2S 16bit setting */
		snd_soc_component_update_bits(codec, AUD9001X_42_IF1_FORM2,
				I2S1_DL_MASK, I2S1_DL_16BIT << I2S1_DL_SHIFT);

		/* I2S 32fs setting */
		snd_soc_component_update_bits(codec, AUD9001X_43_IF1_FORM3,
				I2S1_XFS_MASK, I2S1_XFS_32FS << I2S1_XFS_SHIFT);
		break;
	}

	if (substream->stream)
		capture_hw_params(codec, cur_aifrate);
	else
		playback_hw_params(codec, cur_aifrate);

	return 0;
}

static const struct snd_soc_dai_ops aud9001x_dai_ops = {
	.set_fmt = aud9001x_dai_set_fmt,
	.startup = aud9001x_dai_startup,
	.shutdown = aud9001x_dai_shutdown,
	.hw_params = aud9001x_dai_hw_params,
	.hw_free = aud9001x_dai_hw_free,
};

#define AUD9001X_RATES		SNDRV_PCM_RATE_8000_192000

#define AUD9001X_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE |		\
		SNDRV_PCM_FMTBIT_S20_3LE |	\
		SNDRV_PCM_FMTBIT_S24_LE |	\
		SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver aud9001x_dai[] = {
	{
		.name = "aud9001x-aif",
		.id = 1,
		.playback = {
			.stream_name = "AIF Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = AUD9001X_RATES,
			.formats = AUD9001X_FORMATS,
		},
		.capture = {
			.stream_name = "AIF Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = AUD9001X_RATES,
			.formats = AUD9001X_FORMATS,
		},
		.ops = &aud9001x_dai_ops,
		.symmetric_rates = 1,
	},
	{
		.name = "aud9001x-aif2",
		.id = 2,
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AUD9001X_RATES,
			.formats = AUD9001X_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AUD9001X_RATES,
			.formats = AUD9001X_FORMATS,
		},
		.ops = &aud9001x_dai_ops,
		.symmetric_rates = 1,
	},
};

static void aud9001x_i2c_parse_dt(struct aud9001x_priv *aud9001x)
{
#if 0
	struct device *dev = aud9001x->dev;
	struct device_node *np = dev->of_node;
	int ret;

	/* Tuning AVC slope parameter */
	ret = of_property_read_u32(dev->of_node, "avc-slope-param1", &slope_param);
	if (!ret)
		if (slope_param < AUD9001X_AVC_SLOPE_PARAM_MIN)
			aud9001x->avc_slope_param1 = AUD9001X_AVC_SLOPE_PARAM_MIN;
		else if (slope_param > AUD9001X_AVC_SLOPE_PARAM_MAX)
			aud9001x->avc_slope_param1 = AUD9001X_AVC_SLOPE_PARAM_MAX;
		else
			aud9001x->avc_slope_param1 = slope_param;
	else
		aud9001x->avc_slope_param1 = AUD9001X_AVC_SLOPE_PARAM1_DEFAULT;

	ret = of_property_read_u32(dev->of_node, "avc-slope-param2", &slope_param);
	if (!ret)
		if (slope_param < AUD9001X_AVC_SLOPE_PARAM_MIN)
			aud9001x->avc_slope_param2 = AUD9001X_AVC_SLOPE_PARAM_MIN;
		else if (slope_param > AUD9001X_AVC_SLOPE_PARAM_MAX)
			aud9001x->avc_slope_param2 = AUD9001X_AVC_SLOPE_PARAM_MAX;
		else
			aud9001x->avc_slope_param2 = slope_param;
	else
		aud9001x->avc_slope_param2 = AUD9001X_AVC_SLOPE_PARAM2_DEFAULT;

	/* Tuning Speaker Boost Headroom */
	ret = of_property_read_u32(dev->of_node, "boost-hr0", &boost_param);
	if (!ret)
		if (boost_param < AUD9001X_SPK_BST_HR_MIN)
			aud9001x->boost_hr0 = AUD9001X_SPK_BST_HR_MIN;
		else if (boost_param > AUD9001X_SPK_BST_HR_MAX)
			aud9001x->boost_hr0 = AUD9001X_SPK_BST_HR_MAX;
		else
			aud9001x->boost_hr0 = boost_param;
	else
		aud9001x->boost_hr0 = AUD9001X_SPK_BST_HR0_DEFAULT;

	ret = of_property_read_u32(dev->of_node, "boost-hr1", &boost_param);
	if (!ret)
		if (boost_param < AUD9001X_SPK_BST_HR_MIN)
			aud9001x->boost_hr1 = AUD9001X_SPK_BST_HR_MIN;
		else if (boost_param > AUD9001X_SPK_BST_HR_MAX)
			aud9001x->boost_hr1 = AUD9001X_SPK_BST_HR_MAX;
		else
			aud9001x->boost_hr1 = boost_param;
	else
		aud9001x->boost_hr1 = AUD9001X_SPK_BST_HR1_DEFAULT;
#endif
}

/**
 * aud9001x_codec_initialize : To be called if f/w update fails
 *
 * In case the firmware is not present or corrupt, we should still be able to
 * run the codec with decent parameters. This values are updated as per the
 * latest stable firmware.
 *
 * The values provided in this function are hard-coded register values, and we
 * need not update these values as per bit-fields.
 */
static void aud9001x_register_initialize(void *context)
{
	struct snd_soc_component *codec = (struct snd_soc_component *)context;
//	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	dev_dbg(codec->dev, "%s called, setting defaults\n", __func__);

#ifdef CONFIG_PM
	pm_runtime_get_sync(codec->dev);
#else
	aud9001x_enable(codec->dev);
#endif

	snd_soc_component_write(codec, AUD9001X_80_PDB_ACC1, 0x02);
	aud9001x_usleep(15000);

	/* DCT pre-charge On */
	snd_soc_component_write(codec, AUD9001X_14_PD_DA1, 0x20);

	/* IO Setting for DMIC */
	snd_soc_component_write(codec, AUD9001X_0F_IO_CTRL1, 0x00);
	snd_soc_component_write(codec, AUD9001X_40_PD, 0x58);

	/* I2S interface setting */
	snd_soc_component_write(codec, AUD9001X_41_IF1_FORM1, 0x00);
	snd_soc_component_write(codec, AUD9001X_42_IF1_FORM2, 0x10);
	snd_soc_component_write(codec, AUD9001X_43_IF1_FORM3, 0x20);
	snd_soc_component_write(codec, AUD9001X_44_IF1_FORM4, 0xE4);
	snd_soc_component_write(codec, AUD9001X_45_IF1_FORM5, 0x10);

	/* DMIC control setting */
	snd_soc_component_write(codec, AUD9001X_49_DMIC_CTR1, 0x46);
	snd_soc_component_write(codec, AUD9001X_4A_DMIC_CTR2, 0x28);

	/* ADC digital volume setting */
	snd_soc_component_write(codec, AUD9001X_4B_AVOLL1_SUB, 0x54);
	snd_soc_component_write(codec, AUD9001X_4C_AVOLR1_SUB, 0x54);

	/* SPK Init */
	snd_soc_component_update_bits(codec, AUD9001X_72_CLK3_COD,
			CLK_DA_INV_MASK, CLK_DA_INV_MASK);
	snd_soc_component_write(codec, AUD9001X_DB_CTRL_SPKS1, 0xA8);
	snd_soc_component_update_bits(codec, AUD9001X_18_PWAUTO_DA,
			FAST_CHG_MASK, FAST_CHG_1 << FAST_CHG_SHIFT);

	/* AVC LR Setting */
	snd_soc_component_update_bits(codec, AUC9001X_2B_AVC_Para40,
			MUTE_CH_SEL_MASK, MUTE_CH_SEL_L_AND_R << MUTE_CH_SEL_SHIFT);

	/* DAC digital volume setting */
	snd_soc_component_write(codec, AUD9001X_51_DVOLL, 0x54);
	snd_soc_component_write(codec, AUD9001X_52_DVOLR, 0x54);

	/* DAC, ADC digital mute */
	snd_soc_component_write(codec, AUD9001X_50_DAC1, 0x02);
	snd_soc_component_write(codec, AUD9001X_46_ADC1, 0x0D);

	/* All boot time hardware access is done. Put the device to sleep. */
#ifdef CONFIG_PM
	pm_runtime_put_sync(codec->dev);
#else
	aud9001x_disable(codec->dev);
#endif
}

static int aud9001x_codec_probe(struct snd_soc_component *codec)
{
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	pr_err("Codec Driver Probe: (%s).\n", __func__);
	aud9001x->codec = codec;

	aud9001x->vdd = devm_regulator_get(codec->dev, "vdd_ldo26");
	if (IS_ERR(aud9001x->vdd)) {
		dev_warn(codec->dev, "failed to get regulator vdd\n");
		return PTR_ERR(aud9001x->vdd);
	}

	aud9001x->vdd2 = devm_regulator_get(codec->dev, "vdd_codec");
	if (IS_ERR(aud9001x->vdd2)) {
		dev_warn(codec->dev, "failed to get regulator vdd2\n");
		return PTR_ERR(aud9001x->vdd2);
	}

	mutex_init(&aud9001x->adc_mute_lock);

	msleep(20);

	aud9001x_i2c_parse_dt(aud9001x);

	aud9001x_register_initialize(codec);

	/* it should be modify to move machine driver */
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(codec), "SPKOUTLN");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(codec), "IN1L");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(codec), "IN2L");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(codec), "AIF Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(codec), "AIF Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(codec), "AIF2 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(codec), "AIF2 Capture");
	snd_soc_dapm_sync(snd_soc_component_get_dapm(codec));

	dev_info(codec->dev, "Codec Probe Done.\n");
	aud9001x->is_probe_done = true;

	return 0;
}

static void aud9001x_regulators_enable(struct snd_soc_component *codec)
{
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	regulator_enable(aud9001x->vdd);
	aud9001x_usleep(1000);
	dev_dbg(codec->dev, "%s vdd\n", __func__);
}

static void aud9001x_regulators_disable(struct snd_soc_component *codec)
{
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	regulator_disable(aud9001x->vdd);
	dev_dbg(codec->dev, "%s vdd\n", __func__);
}

static void aud9001x_codec_remove(struct snd_soc_component *codec)
{
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	dev_dbg(codec->dev, "(*) %s called\n", __func__);

	destroy_workqueue(aud9001x->adc_mute_wq);

	aud9001x_regulators_disable(codec);
}

static struct snd_soc_component_driver soc_codec_dev_aud9001x = {
	.probe = aud9001x_codec_probe,
	.remove = aud9001x_codec_remove,
	.controls = aud9001x_snd_controls,
	.num_controls = ARRAY_SIZE(aud9001x_snd_controls),
	.dapm_widgets = aud9001x_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(aud9001x_dapm_widgets),
	.dapm_routes = aud9001x_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(aud9001x_dapm_routes),
	.use_pmdown_time = false,
	.idle_bias_on = false,
};

static int aud9001x_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	struct aud9001x_priv *aud9001x;
	int ret;

	dev_info(&i2c->dev, "Codec I2C Probe: (%s) name: %s, i2c addr: 0x%02x\n",
	__func__, id->name, (int)i2c->addr);

	aud9001x = kzalloc(sizeof(struct aud9001x_priv), GFP_KERNEL);
	if (aud9001x == NULL)
		return -ENOMEM;

	aud9001x->dev = &i2c->dev;
	aud9001x->is_suspend = false;
	aud9001x->is_probe_done = false;
	aud9001x->playback_on = false;
	aud9001x->capture_on = false;
	aud9001x->regulator_count = 0;
	aud9001x->playback_aifrate = 0;
	aud9001x->capture_aifrate = 0;
	aud9001x->dmic_delay = 200;

	i2c_set_clientdata(i2c, aud9001x);

	aud9001x->regmap = devm_regmap_init(aud9001x->dev, &aud9001x_bus,
			aud9001x, &aud9001x_regmap);
	if (IS_ERR(aud9001x->regmap)) {
		dev_err(&i2c->dev, "Failed to allocate regmap: %li\n",
				PTR_ERR(aud9001x->regmap));
		ret = -ENOMEM;
		goto err;
	}

	/* initialize workqueue */
	INIT_DELAYED_WORK(&aud9001x->adc_mute_work, aud9001x_adc_mute_work);
	aud9001x->adc_mute_wq = create_singlethread_workqueue("adc_mute_wq");
	if (aud9001x->adc_mute_wq == NULL) {
		dev_err(aud9001x->dev, "Failed to create adc_mute_wq\n");
		return -ENOMEM;
	}

	ret = snd_soc_register_component(&i2c->dev, &soc_codec_dev_aud9001x,
			aud9001x_dai, ARRAY_SIZE(aud9001x_dai));
	if (ret < 0) {
		dev_err(&i2c->dev, "Failed to register codec: %d\n", ret);
		goto err;
	}

#ifdef CONFIG_PM
	pm_runtime_enable(aud9001x->dev);
#endif
	return ret;

err:
	kfree(aud9001x);
	return ret;
}


static int aud9001x_i2c_remove(struct i2c_client *i2c)
{
	struct aud9001x_priv *aud9001x = dev_get_drvdata(&i2c->dev);

	snd_soc_unregister_component(&i2c->dev);
	kfree(aud9001x);
	return 0;
}

/**
 * aud9001x_post_fw_update_success: To be called after f/w update
 *
 * The firmware may be enabling some of the path and power registers which are
 * used during path enablement. We need to keep the values of these registers
 * consistent so that the functionality of the codec driver doesn't change
 * because of the firmware.
 */

static void aud9001x_regmap_sync(struct device *dev)
{
	struct aud9001x_priv *aud9001x = dev_get_drvdata(dev);
	unsigned char reg[AUD9001X_MAX_REGISTER] = {0,};
	int i;

	/* Read from Cache */
	for (i = 0; i < AUD9001X_REGCACHE_SYNC_END_REG; i++)
		if (aud9001x_readable_register(dev, i) &&
				(!aud9001x_volatile_register(dev, i)))
			reg[i] = (unsigned char)
				snd_soc_component_read32(aud9001x->codec, i);

	regcache_cache_bypass(aud9001x->regmap, true);

	/* Update HW */
	for (i = 0; i < AUD9001X_REGCACHE_SYNC_END_REG ; i++)
		if (aud9001x_writeable_register(dev, i) &&
				(!aud9001x_volatile_register(dev, i)))
			snd_soc_component_write(aud9001x->codec, i, reg[i]);

	regcache_cache_bypass(aud9001x->regmap, false);
}

static void aud9001x_reg_restore(struct snd_soc_component *codec)
{
	struct aud9001x_priv *aud9001x = snd_soc_component_get_drvdata(codec);

	snd_soc_component_update_bits(codec, AUD9001X_80_PDB_ACC1,
			EN_ACC_CLK_MASK, EN_ACC_CLK_MASK);

	/* Give 15ms delay before storing the otp values */
	aud9001x_usleep(15000);

	if (!aud9001x->is_probe_done) {
		msleep(40);

		aud9001x_regmap_sync(codec->dev);
	} else
		aud9001x_regmap_sync(codec->dev);
}

static int aud9001x_enable(struct device *dev)
{
	struct aud9001x_priv *aud9001x = dev_get_drvdata(dev);

	dev_dbg(dev, "(*) %s\n", __func__);
	s2mpw03_ext_bus_set(true);

	abox_enable_mclk(true);

	aud9001x_regulators_enable(aud9001x->codec);

	aud9001x->is_suspend = false;

	/* Disable cache_only feature and sync the cache with h/w */
	regcache_cache_only(aud9001x->regmap, false);
	aud9001x_reg_restore(aud9001x->codec);
	s2mpw03_ext_bus_set(false);
	return 0;
}

static int aud9001x_disable(struct device *dev)
{
	struct aud9001x_priv *aud9001x = dev_get_drvdata(dev);

	dev_dbg(dev, "(*) %s\n", __func__);

	aud9001x->is_suspend = true;

	/* As device is going to suspend-state, limit the writes to cache */
	regcache_cache_only(aud9001x->regmap, true);

	aud9001x_regulators_disable(aud9001x->codec);

	abox_enable_mclk(false);

	return 0;
}

static int aud9001x_sys_suspend(struct device *dev)
{
#ifndef CONFIG_PM
	if (abox_is_on()) {
		dev_dbg(dev, "(*)Don't suspend aud9001x, cp functioning\n");
		return 0;
	}
	dev_dbg(dev, "(*) %s\n", __func__);
	aud9001x_disable(dev);
#endif

	return 0;
}

static int aud9001x_sys_resume(struct device *dev)
{
#ifndef CONFIG_PM
	struct aud9001x_priv *aud9001x = dev_get_drvdata(dev);

	if (!aud9001x->is_suspend) {
		dev_dbg(dev, "(*)codec9001x not resuming, cp functioning\n");
		return 0;
	}
	dev_dbg(dev, "(*) %s\n", __func__);
	aud9001x_enable(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM
static int aud9001x_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "(*) %s\n", __func__);
	aud9001x_enable(dev);

	return 0;
}

static int aud9001x_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "(*) %s\n", __func__);
	aud9001x_disable(dev);

	return 0;
}
#endif

static const struct dev_pm_ops aud9001x_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(
			aud9001x_sys_suspend,
			aud9001x_sys_resume
			)
#ifdef CONFIG_PM
		SET_RUNTIME_PM_OPS(
				aud9001x_runtime_suspend,
				aud9001x_runtime_resume,
				NULL
				)
#endif
};

static const struct i2c_device_id aud9001x_i2c_id[] = {
	{ "aud9001x", 9001 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, aud9001x_i2c_id);

const struct of_device_id aud9001x_of_match[] = {
	{ .compatible = "codec,aud9001x", },
	{},
};

static struct i2c_driver aud9001x_i2c_driver = {
	.driver = {
		.name = "aud9001x",
		.owner = THIS_MODULE,
		.pm = &aud9001x_pm,
		.of_match_table = of_match_ptr(aud9001x_of_match),
	},
	.probe = aud9001x_i2c_probe,
	.remove = aud9001x_i2c_remove,
	.id_table = aud9001x_i2c_id,
};

module_i2c_driver(aud9001x_i2c_driver);

MODULE_DESCRIPTION("ASoC AUD9001X driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:AUD9001X-codec");
