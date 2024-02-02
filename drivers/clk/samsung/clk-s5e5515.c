/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Common Clock Framework support for s5e5515 SoC.
 */

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <soc/samsung/cal-if.h>
#include <dt-bindings/clock/s5e5515.h>

#include "../../soc/samsung/cal-if/s5e5515/cmucal-vclk.h"
#include "../../soc/samsung/cal-if/s5e5515/cmucal-node.h"
#include "../../soc/samsung/cal-if/s5e5515/cmucal-qch.h"
#include "../../soc/samsung/cal-if/s5e5515/clkout_s5e5515.h"

#include "composite.h"

static struct samsung_clk_provider *s5e5515_clk_provider;
/*
 * list of controller registers to be saved and restored during a
 * suspend/resume cycle.
 */
/* fixed rate clocks generated outside the soc */
struct samsung_fixed_rate s5e5515_fixed_rate_ext_clks[] __initdata = {
	FRATE(OSCCLK, "fin_pll", NULL, 0, 26000000),
};

/* HWACG VCLK */

struct init_vclk s5e5515_cmgp_hwacg_vclks[] __initdata = {
	HWACG_VCLK(DOUT_CLK_CMGP_BUS, DIV_CLK_APM_BUS, "DOUT_CLK_CMGP_BUS", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_USI0, MUX_CLK_CMGP_USI0, "UMUX_CLK_CMGP_USI0", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_USI1, MUX_CLK_CMGP_USI1, "UMUX_CLK_CMGP_USI1", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_USI2, MUX_CLK_CMGP_USI2, "UMUX_CLK_CMGP_USI2", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_USI3, MUX_CLK_CMGP_USI3, "UMUX_CLK_CMGP_USI3", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_I2C0, MUX_CLK_CMGP_I2C0, "UMUX_CLK_CMGP_I2C0", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_I2C1, MUX_CLK_CMGP_I2C1, "UMUX_CLK_CMGP_I2C1", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_I2C2, MUX_CLK_CMGP_I2C2, "UMUX_CLK_CMGP_I2C2", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_I2C3, MUX_CLK_CMGP_I2C3, "UMUX_CLK_CMGP_I2C3", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_I2C3, MUX_CLK_CMGP_I2C4, "UMUX_CLK_CMGP_I2C4", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_I2C3, MUX_CLK_CMGP_I2C5, "UMUX_CLK_CMGP_I2C5", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(UMUX_CLK_CMGP_I2C3, MUX_CLK_CMGP_I2C6, "UMUX_CLK_CMGP_I2C6", NULL, 0, VCLK_GATE, NULL),

	HWACG_VCLK(GATE_ADC_CMGP_S0, ADC_CMGP_QCH_S0, "GATE_ADC_CMGP_S0", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_ADC_CMGP_S1, ADC_CMGP_QCH_S1, "GATE_ADC_CMGP_S1", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_DMYADC_CMGP_ADC, ADC_CMGP_QCH_ADC, "GATE_DMYADC_CMGP_ADC", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_CMGP_CMU_CMGP, CMGP_CMU_CMGP_QCH, "GATE_CMGP_CMU_CMGP", NULL, 0, VCLK_GATE, NULL),

	HWACG_VCLK(GATE_I2C_CMGP0, I2C_CMGP0_QCH, "GATE_I2C_CMGP0", "UMUX_CLK_CMGP_I2C0", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_CMGP1, I2C_CMGP1_QCH, "GATE_I2C_CMGP1", "UMUX_CLK_CMGP_I2C1", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_CMGP2, I2C_CMGP2_QCH, "GATE_I2C_CMGP2", "UMUX_CLK_CMGP_I2C2", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_CMGP3, I2C_CMGP3_QCH, "GATE_I2C_CMGP3", "UMUX_CLK_CMGP_I2C3", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_CMGP4, I2C_CMGP4_QCH, "GATE_I2C_CMGP4", "UMUX_CLK_CMGP_I2C4", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_CMGP5, I2C_CMGP5_QCH, "GATE_I2C_CMGP5", "UMUX_CLK_CMGP_I2C5", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_CMGP6, I2C_CMGP6_QCH, "GATE_I2C_CMGP6", "UMUX_CLK_CMGP_I2C6", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI_CMGP0, USI_CMGP0_QCH, "GATE_USI_CMGP0", "UMUX_CLK_CMGP_USI0", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI_CMGP1, USI_CMGP1_QCH, "GATE_USI_CMGP1", "UMUX_CLK_CMGP_USI1", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI_CMGP2, USI_CMGP2_QCH, "GATE_USI_CMGP2", "UMUX_CLK_CMGP_USI2", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI_CMGP3, USI_CMGP3_QCH, "GATE_USI_CMGP3", "UMUX_CLK_CMGP_USI3", 0, VCLK_GATE, NULL),
};

struct init_vclk s5e5515_core_hwacg_vclks[] __initdata = {
	HWACG_VCLK(UMUX_CLKCMU_CORE_BUS, MUX_CLKCMU_CORE_BUS_USER, "UMUX_CLKCMU_CORE_BUS", NULL, 0, VCLK_GATE, NULL),

	HWACG_VCLK(GATE_PDMA_CORE_QCH, PDMA_CORE_QCH, "GATE_PDMA_CORE_QCH", "UMUX_CLKCMU_CORE_BUS", 0, VCLK_GATE, NULL),
};


struct init_vclk s5e5515_dpu_hwacg_vclks[] __initdata = {
	HWACG_VCLK(UMUX_CLKCMU_DPU_BUS, MUX_CLKCMU_DPU_BUS_USER, "UMUX_CLKCMU_DPU_BUS", NULL, 0, 0, NULL),

	HWACG_VCLK(GATE_DMYAUD_QCH_CPU, AUD_QCH_CPU, "GATE_DMYAUD_QCH_CPU", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_AUD_QCH_BCLK0, AUD_QCH_BCLK0, "GATE_AUD_QCH_BCLK0", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_AUD_QCH_BCLK1, AUD_QCH_BCLK1, "GATE_AUD_QCH_BCLK1", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_AUD_QCH_BCLK2, AUD_QCH_BCLK1, "GATE_AUD_QCH_BCLK2", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_DPU_QCH_S_DECON, DPU_QCH_S_DECON, "GATE_DPU_QCH_S_DECON", "UMUX_CLKCMU_DPU_BUS", 0, VCLK_GATE, NULL),
};

struct init_vclk s5e5515_fsys_hwacg_vclks[] __initdata = {
	HWACG_VCLK(UMUX_CLKCMU_FSYS_MMC_CARD, MUX_CLKCMU_FSYS_MMC_CARD_USER, "UMUX_CLKCMU_FSYS_MMC_CARD", NULL, 0, 0, NULL),
	HWACG_VCLK(UMUX_CLKCMU_FSYS_MMC_EMBD, MUX_CLKCMU_FSYS_MMC_EMBD_USER, "UMUX_CLKCMU_FSYS_MMC_EMBD", NULL, 0, 0, NULL),
	HWACG_VCLK(UMUX_CLKCMU_FSYS_USB20DRD, MUX_CLKCMU_FSYS_USB20DRD_USER, "UMUX_CLKCMU_FSYS_USB20DRD", NULL, 0, 0, NULL),

	HWACG_VCLK(GATE_MMC_CARD_QCH, MMC_CARD_QCH, "GATE_MMC_CARD_QCH", "UMUX_CLKCMU_FSYS_MMC_CARD", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_MMC_EMBD_QCH, MMC_EMBD_QCH, "GATE_MMC_EMBD_QCH", "UMUX_CLKCMU_FSYS_MMC_EMBD", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USB20DRD_TOP_QCH_LINK, USB20DRD_TOP_QCH_LINK, "GATE_USB20DRD_TOP_QCH_LINK", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USB20DRD_TOP_QCH_20CTRL, USB20DRD_TOP_QCH_20CTRL, "GATE_USB20DRD_TOP_QCH_20CTRL", NULL, 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_DMYUSB20DRD_TOP_QCH_REFCLK, USB20DRD_TOP_QCH_REFCLK, "GATE_DMYUSB20DRD_TOP_QCH_REFCLK", NULL, 0, VCLK_GATE, NULL),
};

struct init_vclk s5e5515_mfc_hwacg_vclks[] __initdata = {
	HWACG_VCLK(UMUX_CLKCMU_MFC_BUSD, MUX_CLKCMU_MFC_BUSD_USER, "UMUX_CLKCMU_MFC_BUSD", NULL, 0, 0, NULL),

	HWACG_VCLK(GATE_MFC_QCH, MFC_QCH, "GATE_MFC_QCH", NULL, 0, VCLK_GATE, NULL),
};

struct init_vclk s5e5515_peri_hwacg_vclks[] __initdata = {
	HWACG_VCLK(UMUX_CLKCMU_PERI_BUS, MUX_CLKCMU_PERI_BUS_USER, "UMUX_CLKCMU_PERI_BUS", NULL, 0, 0, NULL),
	HWACG_VCLK(UMUX_CLKCMU_PERI_IP, MUX_CLKCMU_PERI_IP_USER, "UMUX_CLKCMU_PERI_IP", "UMUX_CLKCMU_PERI_BUS", 0, 0, NULL),

	HWACG_VCLK(GATE_I2C_0_QCH, I2C_0_QCH, "GATE_I2C_0_QCH", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_1_QCH, I2C_1_QCH, "GATE_I2C_1_QCH", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_2_QCH, I2C_2_QCH, "GATE_I2C_2_QCH", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_I2C_3_QCH, I2C_3_QCH, "GATE_I2C_3_QCH", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_MCT, MCT_QCH, "GATE_MCT", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_PWM_MOTOR, PWM_MOTOR_QCH, "GATE_PWM_MOTOR", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI00_I2C, USI00_I2C_QCH, "GATE_USI00_I2C", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI00_USI, USI00_USI_QCH, "GATE_USI00_USI", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI_UART, USI_UART_QCH, "GATE_USI_UART", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_USI_SPI, USI_SPI_QCH, "GATE_USI_SPI", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),	
	HWACG_VCLK(GATE_WDT_CLUSTER0, WDT0_QCH, "GATE_WDT_CLUSTER0", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
	HWACG_VCLK(GATE_WDT_CLUSTER1, WDT1_QCH, "GATE_WDT_CLUSTER1", "UMUX_CLKCMU_PERI_IP", 0, VCLK_GATE, NULL),
};

struct init_vclk s5e5515_vts_hwacg_vclks[] __initdata = {
	HWACG_VCLK(UMUX_CLKCMU_VTS_BUS, MUX_CLKCMU_VTS_BUS_USER, "UMUX_CLKCMU_VTS_BUS", NULL, 0, 0, NULL),

	HWACG_VCLK(GATE_VTS_CPU, CORTEXM4INTEGRATION_QCH_CPU, "GATE_VTS_CPU", "UMUX_CLKCMU_VTS_BUS", 0, VCLK_GATE | VCLK_NOINIT, NULL),
};

/* Special VCLK */
struct init_vclk s5e5515_apm_vclks[] __initdata = {
	VCLK(APM_BUS, DIV_CLK_APM_BUS, "APM_BUS", 0, 0, NULL),
};

struct init_vclk s5e5515_top_vclks[] __initdata = {
	VCLK(DOUT_CLKCMU_DPU_AUD_CPU, CLKCMU_DPU_AUD_CPU, "DOUT_CLKCMU_DPU_AUD_CPU", 0, 0, NULL),
	VCLK(DOUT_CLKCMU_DPU_AUD_BUS, CLKCMU_DPU_AUD_BUS, "DOUT_CLKCMU_DPU_AUD_BUS", 0, 0, NULL),
};

struct init_vclk s5e5515_cmgp_vclks[] __initdata = {
	VCLK(CMGP_PERI, CLKCMU_CMGP_PERI, "CMGP_PERI", 0, 0, NULL),
	VCLK(CMGP_ADC, DIV_CLK_CMGP_ADC, "CMGP_ADC", 0, 0, NULL),
	VCLK(CMGP_I2C0, VCLK_DIV_CLK_CMGP_I2C0, "CMGP_I2C0", 0, 0, NULL),
	VCLK(CMGP_I2C1, VCLK_DIV_CLK_CMGP_I2C1, "CMGP_I2C1", 0, 0, NULL),
	VCLK(CMGP_I2C2, VCLK_DIV_CLK_CMGP_I2C2, "CMGP_I2C2", 0, 0, NULL),
	VCLK(CMGP_I2C3, VCLK_DIV_CLK_CMGP_I2C3, "CMGP_I2C3", 0, 0, NULL),
	VCLK(CMGP_I2C4, VCLK_DIV_CLK_CMGP_I2C4, "CMGP_I2C4", 0, 0, NULL),
	VCLK(CMGP_I2C5, VCLK_DIV_CLK_CMGP_I2C5, "CMGP_I2C5", 0, 0, NULL),
	VCLK(CMGP_I2C6, VCLK_DIV_CLK_CMGP_I2C6, "CMGP_I2C6", 0, 0, NULL),
	VCLK(CMGP_USI0, VCLK_DIV_CLK_CMGP_USI0, "CMGP_USI0", 0, 0, NULL),
	VCLK(CMGP_USI1, VCLK_DIV_CLK_CMGP_USI1, "CMGP_USI1", 0, 0, NULL),
	VCLK(CMGP_USI2, VCLK_DIV_CLK_CMGP_USI2, "CMGP_USI2", 0, 0, NULL),
	VCLK(CMGP_USI3, VCLK_DIV_CLK_CMGP_USI3, "CMGP_USI3", 0, 0, NULL),
};

struct init_vclk s5e5515_dpu_vclks[] __initdata = {
	VCLK(PLL_OUT_AUD, PLL_AUD, "PLL_OUT_AUD", 0, 0, NULL),
	VCLK(DOUT_AUD_AUDIF, CLKCMU_DPU_AUDIF, "DOUT_AUD_AUDIF", 0, 0, NULL),
	VCLK(DOUT_AUD_ACLK, DIV_CLK_DPU_AUD_CPU_ACLK, "DOUT_AUD_ACLK", 0, 0, NULL),
	VCLK(DOUT_AUD_UAIF0, DIV_CLK_AUD_UAIF0, "DOUT_AUD_UAIF0", 0, 0, NULL),
	VCLK(DOUT_AUD_UAIF1, DIV_CLK_AUD_UAIF1, "DOUT_AUD_UAIF1", 0, 0, NULL),
	VCLK(DOUT_AUD_UAIF2, DIV_CLK_AUD_UAIF2, "DOUT_AUD_UAIF2", 0, 0, NULL),

	VCLK(DOUT_CLK_DPU_AUD_DMIC, CLK_DPU_AUD_DMIC, "DOUT_CLK_DPU_AUD_DMIC", 0, 0, NULL),
	VCLK(DOUT_CLK_DPU_AUD_MCLK, CLK_DPU_AUD_MCLK, "DOUT_CLK_DPU_AUD_MCLK", 0, 0, NULL),
	VCLK(DOUT_CLK_DPU_AUD_CNT, DIV_CLK_DPU_AUD_CNT, "DOUT_CLK_DPU_AUD_CNT", 0, 0, NULL),
};

struct init_vclk s5e5515_fsys_vclks[] __initdata = {
	VCLK(FSYS_BUS, CLKCMU_FSYS_BUS, "FSYS_BUS", 0, 0, NULL),
	VCLK(FSYS_MMC_EMBD, CLKCMU_FSYS_MMC_EMBD, "FSYS_MMC_EMBD", 0, 0, NULL),
	VCLK(FSYS_USB20DRD, CLKCMU_FSYS_USB20DRD, "FSYS_USB20DRD", 0, 0, NULL),
	VCLK(FSYS_MMC_CARD, CLKCMU_FSYS_MMC_CARD, "FSYS_MMC_CARD", 0, 0, NULL),
};

struct init_vclk s5e5515_peri_vclks[] __initdata = {
	VCLK(PERI_BUS, CLKCMU_PERI_BUS, "PERI_BUS", 0, 0, NULL),
	VCLK(PERI_UART, CLKCMU_PERI_UART, "PERI_UART", 0, 0, NULL),
	VCLK(PERI_I2C_0, DIV_CLK_PERI_I2C_0, "PERI_I2C_0", 0, 0, NULL),
	VCLK(PERI_I2C_1, DIV_CLK_PERI_I2C_1, "PERI_I2C_1", 0, 0, NULL),
	VCLK(PERI_I2C_2, DIV_CLK_PERI_I2C_2, "PERI_I2C_2", 0, 0, NULL),
	VCLK(PERI_I2C_3, DIV_CLK_PERI_I2C_3, "PERI_I2C_3", 0, 0, NULL),
	VCLK(PERI_SPI, DIV_CLK_PERI_SPI, "PERI_SPI", 0, 0, NULL),
	VCLK(PERI_USI00_I2C, DIV_CLK_PERI_USI00_I2C, "PERI_USI00_I2C", 0, 0, NULL),
	VCLK(PERI_USI00_USI, DIV_CLK_PERI_USI00_USI, "PERI_USI00_USI", 0, 0, NULL),
};

struct init_vclk s5e5515_vts_vclks[] __initdata = {
//	VCLK(CLK_VTS_DMIC_IF, DIV_CLK_VTS_DMIC_IF, "CLK_VTS_DMIC_IF", 0, VCLK_NOINIT, NULL),
//	VCLK(CLK_VTS_DMIC, DIV_CLK_VTS_DMIC, "CLK_VTS_DMIC", 0, VCLK_NOINIT, NULL),
//	VCLK(CLK_VTS_DMIC_IF_DIV2, DIV_CLK_VTS_DMIC_IF_DIV2, "CLK_VTS_DMIC_IF_DIV2", 0, VCLK_NOINIT, NULL),
	VCLK(DOUT_DIV_CLK_VTS_DMIC, DIV_CLK_VTS_DMIC, "DOUT_DIV_CLK_VTS_DMIC", 0, 0, NULL),
	VCLK(DOUT_DIV_CLK_VTS_DMIC_IF, DIV_CLK_VTS_DMIC_IF, "DOUT_DIV_CLK_VTS_DMIC_IF", 0, 0, NULL),
	VCLK(DOUT_DIV_CLK_VTS_DMIC_IF_DIV2, DIV_CLK_VTS_DMIC_IF_DIV2, "DOUT_DIV_CLK_VTS_DMIC_IF_DIV2", 0, 0, NULL),
	VCLK(DOUT_DIV_CLK_VTS_BUS, DIV_CLK_VTS_BUS, "DOUT_DIV_CLK_VTS_BUS", 0, 0, NULL),
};

/* Clock out */
struct init_vclk s5e5515_clkout_vclks[] __initdata = {
	VCLK(OSC_NFC, VCLK_CLKOUT1, "OSC_NFC", 0,0, NULL),
	VCLK(OSC_AUD, VCLK_CLKOUT0, "OSC_AUD", 0,0, NULL),
};

static __initdata struct of_device_id ext_clk_match[] = {
	{.compatible = "samsung,s5e5515-oscclk", .data = (void *)0},
	{},
};

void s5e5515_vclk_init(void)
{
	/* Common clock init */
}

extern void __init samsung_register_of_fixed_ext(
		struct samsung_clk_provider *ctx,
		struct samsung_fixed_rate *fixed_rate_clk,
		unsigned int nr_fixed_rate_clk,
		struct of_device_id *clk_matches);

extern void __init samsung_register_vclk(struct samsung_clk_provider *ctx,
		struct init_vclk *list, unsigned int nr_vclk);
/* register s5e5515 clocks */
void __init s5e5515_clk_init(struct device_node *np)
{
	void __iomem *reg_base;
	int ret;

	if (np) {
		reg_base = of_iomap(np, 0);
		if (!reg_base)
			panic("%s: failed to map registers\n", __func__);
	} else {
		panic("%s: unable to determine soc\n", __func__);
	}

	ret = cal_if_init(np);
	if (ret)
		panic("%s: unable to initialize cal-if\n", __func__);

	s5e5515_clk_provider = samsung_clk_init(np, reg_base, CLK_NR_CLKS);
	if (!s5e5515_clk_provider)
		panic("%s: unable to allocate context.\n", __func__);

	samsung_register_of_fixed_ext(s5e5515_clk_provider, s5e5515_fixed_rate_ext_clks,
			ARRAY_SIZE(s5e5515_fixed_rate_ext_clks),
			ext_clk_match);
	/* register HWACG vclk */
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_cmgp_hwacg_vclks, ARRAY_SIZE(s5e5515_cmgp_hwacg_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_core_hwacg_vclks, ARRAY_SIZE(s5e5515_core_hwacg_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_dpu_hwacg_vclks, ARRAY_SIZE(s5e5515_dpu_hwacg_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_fsys_hwacg_vclks, ARRAY_SIZE(s5e5515_fsys_hwacg_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_mfc_hwacg_vclks, ARRAY_SIZE(s5e5515_mfc_hwacg_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_peri_hwacg_vclks, ARRAY_SIZE(s5e5515_peri_hwacg_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_vts_hwacg_vclks, ARRAY_SIZE(s5e5515_vts_hwacg_vclks));

	/* register special vclk */
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_apm_vclks, ARRAY_SIZE(s5e5515_apm_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_top_vclks, ARRAY_SIZE(s5e5515_top_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_cmgp_vclks, ARRAY_SIZE(s5e5515_cmgp_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_dpu_vclks, ARRAY_SIZE(s5e5515_dpu_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_fsys_vclks, ARRAY_SIZE(s5e5515_fsys_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_peri_vclks, ARRAY_SIZE(s5e5515_peri_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_vts_vclks, ARRAY_SIZE(s5e5515_vts_vclks));
	samsung_register_vclk(s5e5515_clk_provider, s5e5515_clkout_vclks, ARRAY_SIZE(s5e5515_clkout_vclks));

	/* register DVFS vclk */
	//samsung_register_vclk(s5e5515_clk_provider, s5e5515_dvfs_vclks, ARRAY_SIZE(s5e5515_dvfs_vclks));

	clk_register_fixed_factor(NULL, "pwm-clock", "fin_pll",CLK_SET_RATE_PARENT, 1, 1);

	samsung_clk_of_add_provider(np, s5e5515_clk_provider);

	late_time_init = s5e5515_vclk_init;

	pr_info("s5e5515: Clock setup completed\n");
}

CLK_OF_DECLARE(s5e5515_clk, "samsung,s5e5515-clock", s5e5515_clk_init);
