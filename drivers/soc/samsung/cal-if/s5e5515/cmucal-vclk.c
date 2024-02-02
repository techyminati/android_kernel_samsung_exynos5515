#include "../cmucal.h"
#include "cmucal-node.h"
#include "cmucal-vclk.h"
#include "cmucal-vclklut.h"

/* DVFS VCLK -> Clock Node List */
enum clk_id cmucal_vclk_vdd_alive[] = {
	DIV_CLK_APM_BUS,
	CLKCMU_VTS_BUS,
	DIV_CLK_CMGP_USI0,
	DIV_CLK_CMGP_USI1,
	DIV_CLK_CMGP_USI2,
	DIV_CLK_CMGP_USI3,
};
enum clk_id cmucal_vclk_vdd_int[] = {
	DIV_CLK_DPU_BUSP,
	CLKCMU_DPU_BUS,
	MUX_CLKCMU_DPU_BUS,
	CLKCMU_FSYS_MMC_CARD,
	CLKCMU_FSYS_MMC_EMBD,
	MUX_CLKCMU_CPUCL0_CPU,
	CLKCMU_G3D_BUS,
	MUX_CLKCMU_G3D_BUS,
	MUX_CLKCMU_DPU_AUD_CPU,
	MUX_CLKCMU_MIF_SWITCH,
	CLKCMU_MFC_BUS,
	MUX_CLKCMU_MFC_BUS,
	CLKCMU_CORE_BUS,
	MUX_CLKCMU_CORE_BUS,
	CLKCMU_CORE_SSS,
	MUX_CLKCMU_CORE_SSS,
	PLL_MIF,
	CLKCMU_FSYS_BUS,
	MUX_CLKCMU_FSYS_BUS,
	CLKCMU_DPU_AUD_BUS,
	MUX_CLKCMU_DPU_AUD_BUS,
};
/* SPECIAL VCLK -> Clock Node List */
enum clk_id cmucal_vclk_mux_clk_apm_i3c_pmic[] = {
	MUX_CLK_APM_I3C_PMIC,
	DIV_CLK_APM_I3C_PMIC,
};
enum clk_id cmucal_vclk_clkcmu_apm_bus[] = {
	CLKCMU_APM_BUS,
	MUX_CLKCMU_APM_BUS,
};
enum clk_id cmucal_vclk_mux_clk_apm_timer[] = {
	MUX_CLK_APM_TIMER,
};
enum clk_id cmucal_vclk_mux_clk_chub_timer[] = {
	MUX_CLK_CHUB_TIMER,
};
enum clk_id cmucal_vclk_clk_cmgp_adc[] = {
	CLK_CMGP_ADC,
	DIV_CLK_CMGP_ADC,
};
enum clk_id cmucal_vclk_clkcmu_cmgp_peri[] = {
	CLKCMU_CMGP_PERI,
	MUX_CLKCMU_CMGP_PERI,
};
enum clk_id cmucal_vclk_mux_cmu_cmuref[] = {
	MUX_CMU_CMUREF,
	DIV_CLK_CMU_CMUREF,
};
enum clk_id cmucal_vclk_mux_core_cmuref[] = {
	MUX_CORE_CMUREF,
};
enum clk_id cmucal_vclk_clkcmu_cmu_boost[] = {
	CLKCMU_CMU_BOOST,
};
enum clk_id cmucal_vclk_mux_cpucl0_cmuref[] = {
	MUX_CPUCL0_CMUREF,
};
enum clk_id cmucal_vclk_clkcmu_cpucl0_busp[] = {
	CLKCMU_CPUCL0_BUSP,
	MUX_CLKCMU_CPUCL0_BUSP,
};
enum clk_id cmucal_vclk_mux_clk_aud_uaif0[] = {
	MUX_CLK_AUD_UAIF0,
	DIV_CLK_AUD_UAIF0,
	MUX_CLK_AUD_UAIF1,
	DIV_CLK_AUD_UAIF1,
	MUX_CLK_AUD_UAIF2,
	DIV_CLK_AUD_UAIF2,
	DIV_CLK_DPU_AUD_CPU_PCLKDBG,
	DIV_CLK_DPU_AUD_CNT,
};
enum clk_id cmucal_vclk_clkcmu_dpu_audif[] = {
	CLKCMU_DPU_AUDIF,
	MUX_CLKCMU_DPU_AUDIF,
};
enum clk_id cmucal_vclk_mux_mif_cmuref[] = {
	MUX_MIF_CMUREF,
};
enum clk_id cmucal_vclk_clkcmu_mif_busp[] = {
	CLKCMU_MIF_BUSP,
	MUX_CLKCMU_MIF_BUSP,
};
enum clk_id cmucal_vclk_clkcmu_fsys_usb20drd[] = {
	CLKCMU_FSYS_USB20DRD,
	MUX_CLKCMU_FSYS_USB20DRD,
};
enum clk_id cmucal_vclk_clkcmu_peri_uart[] = {
	CLKCMU_PERI_UART,
	MUX_CLKCMU_PERI_UART,
};
enum clk_id cmucal_vclk_div_clk_apm_uart_dbgcore[] = {
	DIV_CLK_APM_UART_DBGCORE,
	MUX_CLK_APM_UART_DBGCORE,
};
enum clk_id cmucal_vclk_div_clk_chub_usi0[] = {
	DIV_CLK_CHUB_USI0,
	MUX_CLK_CHUB_USI0,
};
enum clk_id cmucal_vclk_clkcmu_chub_bus[] = {
	CLKCMU_CHUB_BUS,
	MUX_CLKCMU_CHUB_BUS,
};
enum clk_id cmucal_vclk_div_clk_chub_i2c0[] = {
	DIV_CLK_CHUB_I2C0,
	MUX_CLK_CHUB_I2C0,
};
enum clk_id cmucal_vclk_div_clk_chub_i2c1[] = {
	DIV_CLK_CHUB_I2C1,
	MUX_CLK_CHUB_I2C1,
};
enum clk_id cmucal_vclk_div_clk_cmgp_usi0[] = {
	DIV_CLK_CMGP_USI0,
	MUX_CLK_CMGP_USI0,
};
enum clk_id cmucal_vclk_div_clk_cmgp_usi1[] = {
	DIV_CLK_CMGP_USI1,
	MUX_CLK_CMGP_USI1,
};
enum clk_id cmucal_vclk_div_clk_cmgp_usi2[] = {
	DIV_CLK_CMGP_USI2,
	MUX_CLK_CMGP_USI2,
};
enum clk_id cmucal_vclk_div_clk_cmgp_usi3[] = {
	DIV_CLK_CMGP_USI3,
	MUX_CLK_CMGP_USI3,
};
enum clk_id cmucal_vclk_div_clk_cmgp_i2c0[] = {
	DIV_CLK_CMGP_I2C0,
	MUX_CLK_CMGP_I2C0,
};
enum clk_id cmucal_vclk_div_clk_cmgp_i2c1[] = {
	DIV_CLK_CMGP_I2C1,
	MUX_CLK_CMGP_I2C1,
};
enum clk_id cmucal_vclk_div_clk_cmgp_i2c2[] = {
	DIV_CLK_CMGP_I2C2,
	MUX_CLK_CMGP_I2C2,
};
enum clk_id cmucal_vclk_div_clk_cmgp_i2c3[] = {
	DIV_CLK_CMGP_I2C3,
	MUX_CLK_CMGP_I2C3,
};
enum clk_id cmucal_vclk_div_clk_cmgp_i2c4[] = {
	DIV_CLK_CMGP_I2C4,
	MUX_CLK_CMGP_I2C4,
};
enum clk_id cmucal_vclk_div_clk_cmgp_i2c5[] = {
	DIV_CLK_CMGP_I2C5,
	MUX_CLK_CMGP_I2C5,
};
enum clk_id cmucal_vclk_div_clk_cmgp_i2c6[] = {
	DIV_CLK_CMGP_I2C6,
	MUX_CLK_CMGP_I2C6,
};
enum clk_id cmucal_vclk_ap2cp_shared0_pll_clk[] = {
	AP2CP_SHARED0_PLL_CLK,
};
enum clk_id cmucal_vclk_div_clk_cpucl0_shortstop[] = {
	DIV_CLK_CPUCL0_SHORTSTOP,
};
enum clk_id cmucal_vclk_clkcmu_cpucl0_cpu[] = {
	CLKCMU_CPUCL0_CPU,
};
enum clk_id cmucal_vclk_div_clk_cluster_atclk[] = {
	DIV_CLK_CLUSTER_ATCLK,
};
enum clk_id cmucal_vclk_clkcmu_dpu_aud_cpu[] = {
	CLKCMU_DPU_AUD_CPU,
};
enum clk_id cmucal_vclk_clk_dpu_aud_dmic[] = {
	CLK_DPU_AUD_DMIC,
};
enum clk_id cmucal_vclk_clk_dpu_aud_mclk[] = {
	CLK_DPU_AUD_MCLK,
};
enum clk_id cmucal_vclk_div_clk_peri_i2c_0[] = {
	DIV_CLK_PERI_I2C_0,
};
enum clk_id cmucal_vclk_clkcmu_peri_ip[] = {
	CLKCMU_PERI_IP,
	MUX_CLKCMU_PERI_IP,
};
enum clk_id cmucal_vclk_div_clk_peri_spi[] = {
	DIV_CLK_PERI_SPI,
	MUX_CLK_PERI_SPI,
};
enum clk_id cmucal_vclk_div_clk_peri_usi00_i2c[] = {
	DIV_CLK_PERI_USI00_I2C,
};
enum clk_id cmucal_vclk_div_clk_peri_usi00_usi[] = {
	DIV_CLK_PERI_USI00_USI,
	MUX_CLK_PERI_USI00_USI,
};
enum clk_id cmucal_vclk_div_clk_peri_i2c_1[] = {
	DIV_CLK_PERI_I2C_1,
};
enum clk_id cmucal_vclk_div_clk_peri_i2c_2[] = {
	DIV_CLK_PERI_I2C_2,
};
enum clk_id cmucal_vclk_div_clk_peri_i2c_3[] = {
	DIV_CLK_PERI_I2C_3,
};
/* COMMON VCLK -> Clock Node List */
enum clk_id cmucal_vclk_blk_cmu[] = {
	MUX_CLKCMU_FSYS_MMC_CARD,
	MUX_CLKCMU_FSYS_MMC_EMBD,
	AP2CP_SHARED2_PLL_CLK,
	CLKCMU_PERI_BUS,
	MUX_CLKCMU_PERI_BUS,
	CLKCMU_CPUCL0_DBG,
	MUX_CLKCMU_CPUCL0_DBG,
	AP2CP_SHARED1_PLL_CLK,
	MUX_CLKCMU_MODEM_SHARED1,
	PLL_SHARED0,
	PLL_SHARED1,
	PLL_AUD,
};
enum clk_id cmucal_vclk_blk_s2d[] = {
	MUX_CLK_S2D_CORE,
	PLL_MIF_S2D,
};
enum clk_id cmucal_vclk_blk_apm[] = {
	MUX_CLK_APM_BUS,
	MUX_CLKCMU_VTS_BUS,
};
enum clk_id cmucal_vclk_blk_chub[] = {
	DIV_CLK_CHUB_BUS,
	MUX_CLK_CHUB_BUS,
};
enum clk_id cmucal_vclk_blk_core[] = {
	MUX_CLK_CORE_GIC,
	DIV_CLK_CORE_BUSP,
};
enum clk_id cmucal_vclk_blk_cpucl0[] = {
	DIV_CLK_CLUSTER_ACLK,
	DIV_CLK_CLUSTER_PCLK,
	DIV_CLK_CLUSTER_PERIPHCLK,
	MUX_CLK_CPUCL0_CPU,
	DIV_CLK_CPUCL0_BUSP,
};
enum clk_id cmucal_vclk_blk_dpu[] = {
	DIV_CLK_DPU_AUD_CPU_ACLK,
	MUX_CLK_DPU_AUD_CPU_HCH,
};
enum clk_id cmucal_vclk_blk_vts[] = {
	DIV_CLK_VTS_BUS,
	MUX_CLK_VTS_BUS,
	DIV_CLK_VTS_DMIC,
	DIV_CLK_VTS_DMIC_IF_DIV2,
	DIV_CLK_VTS_DMIC_IF,
};
enum clk_id cmucal_vclk_blk_g3d[] = {
	DIV_CLK_G3D_BUSP,
};
enum clk_id cmucal_vclk_blk_mfc[] = {
	DIV_CLK_MFC_BUSP,
};
/* GATE VCLK -> Clock Node List */
enum clk_id cmucal_vclk_ip_apbif_rtc_top[] = {
	GOUT_BLK_APM_UID_APBIF_RTC_TOP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apm_cmu_apm[] = {
	CLK_BLK_APM_UID_APM_CMU_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_dp_apm[] = {
	CLK_BLK_APM_UID_XIU_DP_APM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_mailbox_cp_gnss[] = {
	CLK_BLK_APM_UID_MAILBOX_CP_GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_cp_wpan[] = {
	CLK_BLK_APM_UID_MAILBOX_CP_WPAN_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_wpan[] = {
	CLK_BLK_APM_UID_MAILBOX_AP_WPAN_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_gpio_alive[] = {
	CLK_BLK_APM_UID_APBIF_GPIO_ALIVE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_pmu_alive[] = {
	CLK_BLK_APM_UID_APBIF_PMU_ALIVE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_rtc[] = {
	CLK_BLK_APM_UID_APBIF_RTC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_rtc_chub[] = {
	CLK_BLK_APM_UID_APBIF_RTC_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_vgpio2ap[] = {
	CLK_BLK_APM_UID_APBIF_SYSREG_VGPIO2AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_vgpio2apm[] = {
	CLK_BLK_APM_UID_APBIF_SYSREG_VGPIO2APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_vgpio2pmu[] = {
	CLK_BLK_APM_UID_APBIF_SYSREG_VGPIO2PMU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_alive_0[] = {
	CLK_BLK_APM_UID_D_TZPC_ALIVE_0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_grebe_apm[] = {
	CLK_BLK_APM_UID_GREBE_APM_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_grebe_dbgcore[] = {
	CLK_BLK_APM_UID_GREBE_DBGCORE_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_hw_scandump_clkstop_ctrl[] = {
	CLK_BLK_APM_UID_HW_SCANDUMP_CLKSTOP_CTRL_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_i3c_pmic[] = {
	CLK_BLK_APM_UID_I3C_PMIC_IPCLKPORT_I_PCLK,
	CLK_BLK_APM_UID_I3C_PMIC_IPCLKPORT_I_SCLK,
};
enum clk_id cmucal_vclk_ip_intmem_alive[] = {
	CLK_BLK_APM_UID_INTMEM_ALIVE_IPCLKPORT_ACLK,
	CLK_BLK_APM_UID_INTMEM_ALIVE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_ap[] = {
	CLK_BLK_APM_UID_MAILBOX_APM_AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_chub[] = {
	CLK_BLK_APM_UID_MAILBOX_APM_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_cp[] = {
	CLK_BLK_APM_UID_MAILBOX_APM_CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_gnss[] = {
	CLK_BLK_APM_UID_MAILBOX_APM_GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_vts[] = {
	CLK_BLK_APM_UID_MAILBOX_APM_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_wlbt[] = {
	CLK_BLK_APM_UID_MAILBOX_APM_WLBT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_chub[] = {
	CLK_BLK_APM_UID_MAILBOX_AP_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_cp[] = {
	CLK_BLK_APM_UID_MAILBOX_AP_CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_cp_s[] = {
	CLK_BLK_APM_UID_MAILBOX_AP_CP_S_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_dbgcore[] = {
	CLK_BLK_APM_UID_MAILBOX_AP_DBGCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_gnss[] = {
	CLK_BLK_APM_UID_MAILBOX_AP_GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_wlan[] = {
	CLK_BLK_APM_UID_MAILBOX_AP_WLAN_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_cp_chub[] = {
	CLK_BLK_APM_UID_MAILBOX_CP_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_cp_wlan[] = {
	CLK_BLK_APM_UID_MAILBOX_CP_WLAN_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_gnss_chub[] = {
	CLK_BLK_APM_UID_MAILBOX_GNSS_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_gnss_wlbt[] = {
	CLK_BLK_APM_UID_MAILBOX_GNSS_WLBT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_wlbt_abox[] = {
	CLK_BLK_APM_UID_MAILBOX_WLBT_ABOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_wlbt_chub0[] = {
	CLK_BLK_APM_UID_MAILBOX_WLBT_CHUB0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_wlbt_chub1[] = {
	CLK_BLK_APM_UID_MAILBOX_WLBT_CHUB1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_pmu_intr_gen[] = {
	CLK_BLK_APM_UID_PMU_INTR_GEN_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_rom_crc32_host[] = {
	CLK_BLK_APM_UID_ROM_CRC32_HOST_IPCLKPORT_ACLK,
	CLK_BLK_APM_UID_ROM_CRC32_HOST_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_c_chub[] = {
	CLK_BLK_APM_UID_SLH_AXI_MI_C_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_c_gnss[] = {
	CLK_BLK_APM_UID_SLH_AXI_MI_C_GNSS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_c_modem[] = {
	CLK_BLK_APM_UID_SLH_AXI_MI_C_MODEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_c_vts[] = {
	CLK_BLK_APM_UID_SLH_AXI_MI_C_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_c_wlbt[] = {
	CLK_BLK_APM_UID_SLH_AXI_MI_C_WLBT_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_alive[] = {
	CLK_BLK_APM_UID_SLH_AXI_MI_P_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_d_alive[] = {
	CLK_BLK_APM_UID_SLH_AXI_SI_D_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_g_dbgcore[] = {
	CLK_BLK_APM_UID_SLH_AXI_SI_G_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_g_scan2dram[] = {
	CLK_BLK_APM_UID_SLH_AXI_SI_G_SCAN2DRAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_lp_chub[] = {
	CLK_BLK_APM_UID_SLH_AXI_SI_LP_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_lp_vts[] = {
	CLK_BLK_APM_UID_SLH_AXI_SI_LP_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_apm[] = {
	CLK_BLK_APM_UID_SYSREG_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_uart_dbgcore[] = {
	CLK_BLK_APM_UID_UART_DBGCORE_IPCLKPORT_IPCLK,
	CLK_BLK_APM_UID_UART_DBGCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_apm[] = {
	CLK_BLK_APM_UID_WDT_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_dbgcore[] = {
	CLK_BLK_APM_UID_WDT_DBGCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_c_chub[] = {
	GOUT_BLK_CHUB_UID_SLH_AXI_SI_C_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_chub_cmu_chub[] = {
	CLK_BLK_CHUB_UID_CHUB_CMU_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_chub1[] = {
	CLK_BLK_CHUB_UID_WDT_CHUB1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi_ds_64to32_chub[] = {
	CLK_BLK_CHUB_UID_AXI_DS_64TO32_CHUB_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_baaw_c_chub[] = {
	CLK_BLK_CHUB_UID_BAAW_C_CHUB_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_baaw_d_chub[] = {
	CLK_BLK_CHUB_UID_BAAW_D_CHUB_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_bps_axi_lp_chub[] = {
	CLK_BLK_CHUB_UID_BPS_AXI_LP_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_bps_axi_p_chub[] = {
	CLK_BLK_CHUB_UID_BPS_AXI_P_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_cm4_chub[] = {
	CLK_BLK_CHUB_UID_CM4_CHUB_IPCLKPORT_FCLK,
};
enum clk_id cmucal_vclk_ip_dapasync_chub[] = {
	CLK_BLK_CHUB_UID_DAPASYNC_CHUB_IPCLKPORT_DAPCLKM,
};
enum clk_id cmucal_vclk_ip_d_tzpc_chub[] = {
	CLK_BLK_CHUB_UID_D_TZPC_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_chub0[] = {
	CLK_BLK_CHUB_UID_I2C_CHUB0_IPCLKPORT_IPCLK,
	CLK_BLK_CHUB_UID_I2C_CHUB0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_chub1[] = {
	CLK_BLK_CHUB_UID_I2C_CHUB1_IPCLKPORT_IPCLK,
	CLK_BLK_CHUB_UID_I2C_CHUB1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_pdma_chub[] = {
	CLK_BLK_CHUB_UID_PDMA_CHUB_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmu_chub[] = {
	CLK_BLK_CHUB_UID_PPMU_CHUB_IPCLKPORT_ACLK,
	CLK_BLK_CHUB_UID_PPMU_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_pwm_chub[] = {
	CLK_BLK_CHUB_UID_PWM_CHUB_IPCLKPORT_I_PCLK_S0,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_lp_chub[] = {
	CLK_BLK_CHUB_UID_SLH_AXI_MI_LP_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_chub[] = {
	CLK_BLK_CHUB_UID_SLH_AXI_MI_P_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_d_chub[] = {
	CLK_BLK_CHUB_UID_SLH_AXI_SI_D_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sweeper_c_chub[] = {
	CLK_BLK_CHUB_UID_SWEEPER_C_CHUB_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sweeper_d_chub[] = {
	CLK_BLK_CHUB_UID_SWEEPER_D_CHUB_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_chub[] = {
	CLK_BLK_CHUB_UID_SYSREG_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_timer_chub[] = {
	CLK_BLK_CHUB_UID_TIMER_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi_chub[] = {
	CLK_BLK_CHUB_UID_USI_CHUB_IPCLKPORT_IPCLK,
	CLK_BLK_CHUB_UID_USI_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_chub0[] = {
	CLK_BLK_CHUB_UID_WDT_CHUB0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_dp_chub[] = {
	CLK_BLK_CHUB_UID_XIU_DP_CHUB_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_yamin_mcu_chub[] = {
	CLK_BLK_CHUB_UID_YAMIN_MCU_CHUB_IPCLKPORT_CLKIN,
	CLK_BLK_CHUB_UID_YAMIN_MCU_CHUB_IPCLKPORT_DBGCLK,
	CLK_BLK_CHUB_UID_YAMIN_MCU_CHUB_IPCLKPORT_IWICCLK,
};
enum clk_id cmucal_vclk_ip_cmgp_cmu_cmgp[] = {
	CLK_BLK_CMGP_UID_CMGP_CMU_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp6[] = {
	CLK_BLK_CMGP_UID_I2C_CMGP6_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_I2C_CMGP6_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp4[] = {
	CLK_BLK_CMGP_UID_I2C_CMGP4_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_I2C_CMGP4_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp2[] = {
	CLK_BLK_CMGP_UID_I2C_CMGP2_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_I2C_CMGP2_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp1[] = {
	CLK_BLK_CMGP_UID_I2C_CMGP1_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_I2C_CMGP1_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp0[] = {
	CLK_BLK_CMGP_UID_I2C_CMGP0_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_I2C_CMGP0_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp3[] = {
	CLK_BLK_CMGP_UID_I2C_CMGP3_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_I2C_CMGP3_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp5[] = {
	CLK_BLK_CMGP_UID_I2C_CMGP5_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_I2C_CMGP5_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_alive_1[] = {
	CLK_BLK_CMGP_UID_D_TZPC_ALIVE_1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_cmgp2wlbt[] = {
	CLK_BLK_CMGP_UID_APBIF_SYSREG_CMGP2WLBT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_cmgp2pmu_chub[] = {
	CLK_BLK_CMGP_UID_APBIF_SYSREG_CMGP2PMU_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_cmgp2pmu_ap[] = {
	CLK_BLK_CMGP_UID_APBIF_SYSREG_CMGP2PMU_AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_cmgp2gnss[] = {
	CLK_BLK_CMGP_UID_APBIF_SYSREG_CMGP2GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_cmgp2cp[] = {
	CLK_BLK_CMGP_UID_APBIF_SYSREG_CMGP2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_sysreg_cmgp2chub[] = {
	CLK_BLK_CMGP_UID_APBIF_SYSREG_CMGP2CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_adc_cmgp[] = {
	CLK_BLK_CMGP_UID_ADC_CMGP_IPCLKPORT_PCLK_S0,
	CLK_BLK_CMGP_UID_ADC_CMGP_IPCLKPORT_PCLK_S1,
};
enum clk_id cmucal_vclk_ip_sysreg_cmgp[] = {
	CLK_BLK_CMGP_UID_SYSREG_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp0[] = {
	CLK_BLK_CMGP_UID_USI_CMGP0_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_USI_CMGP0_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp1[] = {
	CLK_BLK_CMGP_UID_USI_CMGP1_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_USI_CMGP1_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp2[] = {
	CLK_BLK_CMGP_UID_USI_CMGP2_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_USI_CMGP2_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp3[] = {
	CLK_BLK_CMGP_UID_USI_CMGP3_IPCLKPORT_PCLK,
	CLK_BLK_CMGP_UID_USI_CMGP3_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_apbif_gpio_cmgp[] = {
	CLK_BLK_CMGP_UID_APBIF_GPIO_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_pdma0[] = {
	GOUT_BLK_CORE_UID_AD_APB_PDMA0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_ad_apb_spdma[] = {
	GOUT_BLK_CORE_UID_AD_APB_SPDMA_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_baaw_p_gnss[] = {
	GOUT_BLK_CORE_UID_BAAW_P_GNSS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_baaw_p_chub[] = {
	GOUT_BLK_CORE_UID_BAAW_P_CHUB_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_baaw_p_modem[] = {
	GOUT_BLK_CORE_UID_BAAW_P_MODEM_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_baaw_p_wlbt[] = {
	GOUT_BLK_CORE_UID_BAAW_P_WLBT_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_gic400_aihwacg[] = {
	GOUT_BLK_CORE_UID_GIC400_AIHWACG_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_mfc[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_vts[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_d_dpu[] = {
	GOUT_BLK_CORE_UID_LH_AXI_MI_D_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_fsys[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_FSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d0_modem[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D0_MODEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d1_modem[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D1_MODEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_alive[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_chub[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_cssys[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_gnss[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_GNSS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_d_wlbt[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_MI_D_WLBT_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_alive[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_chub[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_cpucl0[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_dpu[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_fsys[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_FSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_gnss[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_GNSS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_mfc[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_vts[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_modem[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_MODEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_peri[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_PERI_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_wlbt[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_WLBT_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_pdma_core[] = {
	GOUT_BLK_CORE_UID_PDMA_CORE_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sfr_apbif_cmu_topc[] = {
	GOUT_BLK_CORE_UID_SFR_APBIF_CMU_TOPC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sirex[] = {
	GOUT_BLK_CORE_UID_SIREX_IPCLKPORT_I_ACLK,
	GOUT_BLK_CORE_UID_SIREX_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_spdma_core[] = {
	GOUT_BLK_CORE_UID_SPDMA_CORE_IPCLKPORT_ACLK_PDMA1,
};
enum clk_id cmucal_vclk_ip_sysreg_core[] = {
	GOUT_BLK_CORE_UID_SYSREG_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_d_cpucl0[] = {
	GOUT_BLK_CORE_UID_LH_AXI_MI_D_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_rtic[] = {
	GOUT_BLK_CORE_UID_RTIC_IPCLKPORT_I_ACLK,
	GOUT_BLK_CORE_UID_RTIC_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_sss[] = {
	GOUT_BLK_CORE_UID_SSS_IPCLKPORT_I_PCLK,
	GOUT_BLK_CORE_UID_SSS_IPCLKPORT_I_ACLK,
};
enum clk_id cmucal_vclk_ip_baaw_p_vts[] = {
	GOUT_BLK_CORE_UID_BAAW_P_VTS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_core[] = {
	GOUT_BLK_CORE_UID_D_TZPC_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_d_g3d[] = {
	GOUT_BLK_CORE_UID_LH_AXI_MI_D_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_g3d[] = {
	GOUT_BLK_CORE_UID_SLH_AXI_SI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ad_dap_cssys_cpucl0[] = {
	GOUT_BLK_CORE_UID_AD_DAP_CSSYS_CPUCL0_IPCLKPORT_DAPCLKM,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_p_mif[] = {
	CLK_BLK_CORE_UID_SLH_AXI_SI_P_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_d0_mif[] = {
	CLK_BLK_CORE_UID_LH_AXI_SI_D0_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_d1_mif[] = {
	CLK_BLK_CORE_UID_LH_AXI_SI_D1_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_d3_mif[] = {
	CLK_BLK_CORE_UID_LH_AXI_SI_D3_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_core_cmu_core[] = {
	CLK_BLK_CORE_UID_CORE_CMU_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_trex_p_core[] = {
	CLK_BLK_CORE_UID_TREX_P_CORE_IPCLKPORT_ACLK_GIC,
	CLK_BLK_CORE_UID_TREX_P_CORE_IPCLKPORT_ACLK_P,
	CLK_BLK_CORE_UID_TREX_P_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_d2_mif[] = {
	GOUT_BLK_CORE_UID_LH_AXI_SI_D2_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_trex_d_core[] = {
	CLK_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_ACLK_D,
	CLK_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_ACLK_P,
	CLK_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_ACLK_SSS,
	CLK_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_cmu_core_cmuref[] = {
	CLK_BLK_CORE_UID_CMU_CORE_CMUREF_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_SYSREG_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_cluster0[] = {
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_ATCLK,
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_GICCLK,
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_PCLK,
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_PERIPHCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_SLH_AXI_MI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_D_TZPC_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_XIU_P_CPUCL0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmu_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_PPMU_CPUCL0_IPCLKPORT_PCLK,
	GOUT_BLK_CPUCL0_UID_PPMU_CPUCL0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_g_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_SLH_AXI_MI_G_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_ig_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_LH_AXI_SI_IG_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_dp_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_XIU_DP_DBGCORE_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_gray2bin_atclk[] = {
	GOUT_BLK_CPUCL0_UID_GRAY2BIN_ATCLK_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_cpucl0_cmu_cpucl0[] = {
	CLK_BLK_CPUCL0_UID_CPUCL0_CMU_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_cssys_dbg[] = {
	GOUT_BLK_CPUCL0_UID_CSSYS_DBG_IPCLKPORT_PCLKDBG,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_d_cssys[] = {
	GOUT_BLK_CPUCL0_UID_SLH_AXI_SI_D_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_secjtag[] = {
	GOUT_BLK_CPUCL0_UID_SECJTAG_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_d_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_LH_AXI_SI_D_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_ig_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_LH_AXI_MI_IG_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_p_xiu2cssys[] = {
	GOUT_BLK_CPUCL0_UID_AD_APB_P_XIU2CSSYS_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_adm_apb_d_cssys[] = {
	CLK_BLK_CPUCL0_UID_ADM_APB_D_CSSYS_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_aud[] = {
	GOUT_BLK_DPU_UID_AUD_IPCLKPORT_CCLK_ASB,
	GOUT_BLK_DPU_UID_AUD_IPCLKPORT_CCLK_DAP,
	GOUT_BLK_DPU_UID_AUD_IPCLKPORT_ACLK,
	CLK_BLK_DPU_UID_AUD_IPCLKPORT_BCLK_UAIF0,
	CLK_BLK_DPU_UID_AUD_IPCLKPORT_BCLK_UAIF1,
	GOUT_BLK_DPU_UID_AUD_IPCLKPORT_CCLK_CA32,
	CLK_BLK_DPU_UID_AUD_IPCLKPORT_BCLK_UAIF2,
	GOUT_BLK_DPU_UID_AUD_IPCLKPORT_ACLK_IRQ,
	GOUT_BLK_DPU_UID_AUD_IPCLKPORT_ACLK_IRQ,
	CLK_BLK_DPU_UID_AUD_IPCLKPORT_BCLK_CNT,
};
enum clk_id cmucal_vclk_ip_peri_axi_asb[] = {
	GOUT_BLK_DPU_UID_PERI_AXI_ASB_IPCLKPORT_ACLKM,
};
enum clk_id cmucal_vclk_ip_wdt_aud[] = {
	GOUT_BLK_DPU_UID_WDT_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dftmux_dpu[] = {
	GOUT_BLK_DPU_UID_DFTMUX_DPU_IPCLKPORT_AUD_CODEC_MCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_aud[] = {
	GOUT_BLK_DPU_UID_PPMU_AUD_IPCLKPORT_ACLK,
	GOUT_BLK_DPU_UID_PPMU_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_aud[] = {
	GOUT_BLK_DPU_UID_SYSMMU_AUD_IPCLKPORT_CLK_S1,
};
enum clk_id cmucal_vclk_ip_ppmu_dpu[] = {
	GOUT_BLK_DPU_UID_PPMU_DPU_IPCLKPORT_ACLK,
	GOUT_BLK_DPU_UID_PPMU_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_dpu[] = {
	GOUT_BLK_DPU_UID_SYSREG_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_dpu[] = {
	GOUT_BLK_DPU_UID_SLH_AXI_MI_P_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_d_dpu[] = {
	GOUT_BLK_DPU_UID_LH_AXI_SI_D_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_dpu[] = {
	GOUT_BLK_DPU_UID_SYSMMU_DPU_IPCLKPORT_CLK_S1,
};
enum clk_id cmucal_vclk_ip_dpu[] = {
	GOUT_BLK_DPU_UID_DPU_IPCLKPORT_ACLK_DECON,
	CLK_BLK_DPU_UID_DPU_IPCLKPORT_ACLK_DPP,
	CLK_BLK_DPU_UID_DPU_IPCLKPORT_ACLK_DMA,
};
enum clk_id cmucal_vclk_ip_gpio_dpu[] = {
	GOUT_BLK_DPU_UID_GPIO_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_decon0[] = {
	GOUT_BLK_DPU_UID_AD_APB_DECON0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_dmic[] = {
	GOUT_BLK_DPU_UID_DMIC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_dpu[] = {
	GOUT_BLK_DPU_UID_D_TZPC_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_smmu_aud_ns[] = {
	GOUT_BLK_DPU_UID_AD_APB_SMMU_AUD_NS_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_dpu_cmu_dpu[] = {
	CLK_BLK_DPU_UID_DPU_CMU_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_dpu[] = {
	GOUT_BLK_DPU_UID_QE_DPU_IPCLKPORT_ACLK,
	GOUT_BLK_DPU_UID_QE_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_aud[] = {
	GOUT_BLK_DPU_UID_QE_AUD_IPCLKPORT_ACLK,
	GOUT_BLK_DPU_UID_QE_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_dpu[] = {
	GOUT_BLK_DPU_UID_XIU_D_DPU_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_dpu_axi_asb[] = {
	GOUT_BLK_DPU_UID_DPU_AXI_ASB_IPCLKPORT_ACLKM,
	CLK_BLK_DPU_UID_DPU_AXI_ASB_IPCLKPORT_ACLKS,
};
enum clk_id cmucal_vclk_ip_gpio_fsys[] = {
	GOUT_BLK_FSYS_UID_GPIO_FSYS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_fsys[] = {
	GOUT_BLK_FSYS_UID_SLH_AXI_MI_P_FSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_d_fsys[] = {
	GOUT_BLK_FSYS_UID_SLH_AXI_SI_D_FSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_fsys[] = {
	GOUT_BLK_FSYS_UID_PPMU_FSYS_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS_UID_PPMU_FSYS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_fsys[] = {
	GOUT_BLK_FSYS_UID_SYSREG_FSYS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_fsys[] = {
	GOUT_BLK_FSYS_UID_XIU_D_FSYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_mmc_card[] = {
	GOUT_BLK_FSYS_UID_MMC_CARD_IPCLKPORT_I_ACLK,
	CLK_BLK_FSYS_UID_MMC_CARD_IPCLKPORT_SDCLKIN,
};
enum clk_id cmucal_vclk_ip_mmc_embd[] = {
	GOUT_BLK_FSYS_UID_MMC_EMBD_IPCLKPORT_I_ACLK,
	CLK_BLK_FSYS_UID_MMC_EMBD_IPCLKPORT_SDCLKIN,
};
enum clk_id cmucal_vclk_ip_d_tzpc_fsys[] = {
	GOUT_BLK_FSYS_UID_D_TZPC_FSYS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usb20drd_top[] = {
	GOUT_BLK_FSYS_UID_USB20DRD_TOP_IPCLKPORT_BUS_CLK_EARLY,
	CLK_BLK_FSYS_UID_USB20DRD_TOP_IPCLKPORT_I_USB20DRD_REF_CLK_50,
	GOUT_BLK_FSYS_UID_USB20DRD_TOP_IPCLKPORT_ACLK_PHYCTRL_20,
	CLK_BLK_FSYS_UID_USB20DRD_TOP_IPCLKPORT_I_USB20_PHY_REFCLK_26,
};
enum clk_id cmucal_vclk_ip_us_64to128_fsys[] = {
	GOUT_BLK_FSYS_UID_US_64TO128_FSYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_fsys[] = {
	GOUT_BLK_FSYS_UID_XIU_P_FSYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_fsys_cmu_fsys[] = {
	CLK_BLK_FSYS_UID_FSYS_CMU_FSYS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_g3d[] = {
	GOUT_BLK_G3D_UID_SLH_AXI_MI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_g3d[] = {
	GOUT_BLK_G3D_UID_SYSREG_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_gray2bin_g3d[] = {
	GOUT_BLK_G3D_UID_GRAY2BIN_G3D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_d_g3d[] = {
	GOUT_BLK_G3D_UID_LH_AXI_SI_D_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_g3d[] = {
	GOUT_BLK_G3D_UID_D_TZPC_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_g3d[] = {
	GOUT_BLK_G3D_UID_G3D_IPCLKPORT_CLK_STACKS,
	GOUT_BLK_G3D_UID_G3D_IPCLKPORT_CLK_STACKS,
	GOUT_BLK_G3D_UID_G3D_IPCLKPORT_CLK_COREGROUP,
	GOUT_BLK_G3D_UID_G3D_IPCLKPORT_CLK_COREGROUP,
	CLK_BLK_G3D_UID_G3D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_g3d[] = {
	GOUT_BLK_G3D_UID_PPMU_G3D_IPCLKPORT_ACLK,
	GOUT_BLK_G3D_UID_PPMU_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_g3d_cmu_g3d[] = {
	CLK_BLK_G3D_UID_G3D_CMU_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_si_ip_g3d[] = {
	GOUT_BLK_G3D_UID_LH_AXI_SI_IP_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_ip_g3d[] = {
	GOUT_BLK_G3D_UID_LH_AXI_MI_IP_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mfc[] = {
	GOUT_BLK_MFC_UID_MFC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmu_mfc[] = {
	GOUT_BLK_MFC_UID_PPMU_MFC_IPCLKPORT_PCLK,
	GOUT_BLK_MFC_UID_PPMU_MFC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_mfc[] = {
	GOUT_BLK_MFC_UID_SYSREG_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_mfc[] = {
	GOUT_BLK_MFC_UID_SLH_AXI_MI_P_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_mfc[] = {
	GOUT_BLK_MFC_UID_D_TZPC_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_d_mfc[] = {
	GOUT_BLK_MFC_UID_SLH_AXI_SI_D_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_as_apb_mfc[] = {
	GOUT_BLK_MFC_UID_AS_APB_MFC_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_xiu_d_mfc[] = {
	GOUT_BLK_MFC_UID_XIU_D_MFC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_mfc[] = {
	GOUT_BLK_MFC_UID_SYSMMU_MFC_IPCLKPORT_CLK_S1,
};
enum clk_id cmucal_vclk_ip_mfc_cmu_mfc[] = {
	CLK_BLK_MFC_UID_MFC_CMU_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_bl[] = {
	GOUT_BLK_MFC_UID_BL_IPCLKPORT_ACLK,
	GOUT_BLK_MFC_UID_BL_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_mif[] = {
	GOUT_BLK_MIF_UID_SYSREG_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmc[] = {
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK_PF,
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK_PF,
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK_SECURE,
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK_SECURE,
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK,
	CLK_BLK_MIF_UID_DMC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_mif[] = {
	GOUT_BLK_MIF_UID_D_TZPC_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_d0_mif[] = {
	GOUT_BLK_MIF_UID_LH_AXI_MI_D0_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_d1_mif[] = {
	GOUT_BLK_MIF_UID_LH_AXI_MI_D1_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_d2_mif[] = {
	GOUT_BLK_MIF_UID_LH_AXI_MI_D2_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lh_axi_mi_d3_mif[] = {
	GOUT_BLK_MIF_UID_LH_AXI_MI_D3_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_mif[] = {
	GOUT_BLK_MIF_UID_SLH_AXI_MI_P_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mif_cmu_mif[] = {
	CLK_BLK_MIF_UID_MIF_CMU_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ddr_phy[] = {
	CLK_BLK_MIF_UID_DDR_PHY_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_modem_cmu_modem[] = {
	CLK_BLK_MODEM_UID_MODEM_CMU_MODEM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_peri_cmu_peri[] = {
	CLK_BLK_PERI_UID_PERI_CMU_PERI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_peri[] = {
	CLK_BLK_PERI_UID_D_TZPC_PERI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_otp_con_top[] = {
	CLK_BLK_PERI_UID_OTP_CON_TOP_IPCLKPORT_PCLK,
	CLK_BLK_PERI_UID_OTP_CON_TOP_IPCLKPORT_I_OSCCLK,
};
enum clk_id cmucal_vclk_ip_busif_tmu[] = {
	CLK_BLK_PERI_UID_BUSIF_TMU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_gpio_peri[] = {
	CLK_BLK_PERI_UID_GPIO_PERI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_0[] = {
	CLK_BLK_PERI_UID_I2C_0_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_I2C_0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_1[] = {
	CLK_BLK_PERI_UID_I2C_1_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_I2C_1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_2[] = {
	CLK_BLK_PERI_UID_I2C_2_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_I2C_2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_3[] = {
	CLK_BLK_PERI_UID_I2C_3_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_I2C_3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mct[] = {
	CLK_BLK_PERI_UID_MCT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_pwm_motor[] = {
	CLK_BLK_PERI_UID_PWM_MOTOR_IPCLKPORT_I_PCLK_S0,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_peri[] = {
	CLK_BLK_PERI_UID_SLH_AXI_MI_P_PERI_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_peri[] = {
	CLK_BLK_PERI_UID_SYSREG_PERI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi00_i2c[] = {
	CLK_BLK_PERI_UID_USI00_I2C_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_USI00_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi00_usi[] = {
	CLK_BLK_PERI_UID_USI00_USI_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_USI00_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi_spi[] = {
	CLK_BLK_PERI_UID_USI_SPI_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_USI_SPI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi_uart[] = {
	CLK_BLK_PERI_UID_USI_UART_IPCLKPORT_IPCLK,
	CLK_BLK_PERI_UID_USI_UART_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt0[] = {
	CLK_BLK_PERI_UID_WDT0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt1[] = {
	CLK_BLK_PERI_UID_WDT1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_bis_s2d[] = {
	GOUT_BLK_S2D_UID_BIS_S2D_IPCLKPORT_CLK,
	GOUT_BLK_S2D_UID_BIS_S2D_IPCLKPORT_SCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_g_scan2dram[] = {
	GOUT_BLK_S2D_UID_SLH_AXI_MI_G_SCAN2DRAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_s2d_cmu_s2d[] = {
	CLK_BLK_S2D_UID_S2D_CMU_S2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmic_if[] = {
	CLK_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_IF_DIV2_CLK,
	CLK_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_IF_CLK,
	CLK_BLK_VTS_UID_DMIC_IF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vts_cmu_vts[] = {
	CLK_BLK_VTS_UID_VTS_CMU_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_p_vts[] = {
	CLK_BLK_VTS_UID_SLH_AXI_MI_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ahb_busmatrix_vts[] = {
	CLK_BLK_VTS_UID_AHB_BUSMATRIX_VTS_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_baaw_c_vts[] = {
	CLK_BLK_VTS_UID_BAAW_C_VTS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_baaw_d_vts[] = {
	CLK_BLK_VTS_UID_BAAW_D_VTS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_bps_axi_lp_vts[] = {
	CLK_BLK_VTS_UID_BPS_AXI_LP_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_bps_axi_p_vts[] = {
	CLK_BLK_VTS_UID_BPS_AXI_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_cortexm4integration[] = {
	CLK_BLK_VTS_UID_CORTEXM4INTEGRATION_IPCLKPORT_FCLK,
};
enum clk_id cmucal_vclk_ip_dmic_ahb0[] = {
	CLK_BLK_VTS_UID_DMIC_AHB0_IPCLKPORT_HCLK,
	CLK_BLK_VTS_UID_DMIC_AHB0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmic_ahb1[] = {
	CLK_BLK_VTS_UID_DMIC_AHB1_IPCLKPORT_HCLK,
	CLK_BLK_VTS_UID_DMIC_AHB1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_vts[] = {
	CLK_BLK_VTS_UID_D_TZPC_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_gpio_vts[] = {
	CLK_BLK_VTS_UID_GPIO_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hwacg_sys_dmic0[] = {
	CLK_BLK_VTS_UID_HWACG_SYS_DMIC0_IPCLKPORT_HCLK,
	CLK_BLK_VTS_UID_HWACG_SYS_DMIC0_IPCLKPORT_HCLK_BUS,
	CLK_BLK_VTS_UID_HWACG_SYS_DMIC0_IPCLKPORT_HCLK_BUS,
};
enum clk_id cmucal_vclk_ip_hwacg_sys_dmic1[] = {
	CLK_BLK_VTS_UID_HWACG_SYS_DMIC1_IPCLKPORT_HCLK,
	CLK_BLK_VTS_UID_HWACG_SYS_DMIC1_IPCLKPORT_HCLK_BUS,
	CLK_BLK_VTS_UID_HWACG_SYS_DMIC1_IPCLKPORT_HCLK_BUS,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_vts[] = {
	CLK_BLK_VTS_UID_MAILBOX_AP_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_aud_vts[] = {
	CLK_BLK_VTS_UID_MAILBOX_AUD_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_vts[] = {
	CLK_BLK_VTS_UID_PPMU_VTS_IPCLKPORT_ACLK,
	CLK_BLK_VTS_UID_PPMU_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_mi_lp_vts[] = {
	CLK_BLK_VTS_UID_SLH_AXI_MI_LP_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_c_vts[] = {
	CLK_BLK_VTS_UID_SLH_AXI_SI_C_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_slh_axi_si_d_vts[] = {
	CLK_BLK_VTS_UID_SLH_AXI_SI_D_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sweeper_c_vts[] = {
	CLK_BLK_VTS_UID_SWEEPER_C_VTS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sweeper_d_vts[] = {
	CLK_BLK_VTS_UID_SWEEPER_D_VTS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_vts[] = {
	CLK_BLK_VTS_UID_SYSREG_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_timer_vts[] = {
	CLK_BLK_VTS_UID_TIMER_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_vts[] = {
	CLK_BLK_VTS_UID_WDT_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_u_dmic_clk_mux[] = {
	CLK_BLK_VTS_UID_U_DMIC_CLK_MUX_IPCLKPORT_D0,
};

/* DVFS VCLK -> LUT List */
struct vclk_lut cmucal_vclk_vdd_alive_lut[] = {
	{300000, vdd_alive_ud_lut_params},
	{200000, vdd_alive_sud_lut_params},
	{100000, vdd_alive_nm_lut_params},
};
struct vclk_lut cmucal_vclk_vdd_int_lut[] = {
	{4264000, vdd_int_nm_lut_params},
	{3400000, vdd_int_ud_lut_params},
	{2000000, vdd_int_sud_lut_params},
};

/* SPECIAL VCLK -> LUT List */
struct vclk_lut cmucal_vclk_mux_clk_apm_i3c_pmic_lut[] = {
	{215250, mux_clk_apm_i3c_pmic_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_apm_bus_lut[] = {
	{400000, clkcmu_apm_bus_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_apm_timer_lut[] = {
	{60000, mux_clk_apm_timer_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_chub_timer_lut[] = {
	{30000, mux_clk_chub_timer_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clk_cmgp_adc_lut[] = {
	{30750, clk_cmgp_adc_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cmgp_peri_lut[] = {
	{430500, clkcmu_cmgp_peri_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_cmu_cmuref_lut[] = {
	{200000, mux_cmu_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_core_cmuref_lut[] = {
	{133333, mux_core_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cmu_boost_lut[] = {
	{133333, clkcmu_cmu_boost_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_cpucl0_cmuref_lut[] = {
	{200000, mux_cpucl0_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cpucl0_busp_lut[] = {
	{200000, clkcmu_cpucl0_busp_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_aud_uaif0_lut[] = {
	{150000, mux_clk_aud_uaif0_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_dpu_audif_lut[] = {
	{75000, clkcmu_dpu_audif_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_mif_cmuref_lut[] = {
	{200000, mux_mif_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_mif_busp_lut[] = {
	{200000, clkcmu_mif_busp_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_fsys_usb20drd_lut[] = {
	{50000, clkcmu_fsys_usb20drd_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_peri_uart_lut[] = {
	{200000, clkcmu_peri_uart_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_apm_uart_dbgcore_lut[] = {
	{215250, div_clk_apm_uart_dbgcore_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_chub_usi0_lut[] = {
	{430500, div_clk_chub_usi0_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_chub_bus_lut[] = {
	{430500, clkcmu_chub_bus_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_chub_i2c0_lut[] = {
	{215250, div_clk_chub_i2c0_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_chub_i2c1_lut[] = {
	{215250, div_clk_chub_i2c1_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_xx_lut[] = {
	{26000, div_clk_cmgp_26_lut_params},
	{13000, div_clk_cmgp_13_lut_params},
	{8666, div_clk_cmgp_8_lut_params},
	{6500, div_clk_cmgp_6_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_i2c0_lut[] = {
	{215250, div_clk_cmgp_i2c0_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_cmgp_usi0_lut[] = {
	{430500, mux_clk_cmgp_usi0_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_cmgp_usi1_lut[] = {
	{430500, mux_clk_cmgp_usi1_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_cmgp_usi2_lut[] = {
	{430500, mux_clk_cmgp_usi2_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_cmgp_usi3_lut[] = {
	{430500, mux_clk_cmgp_usi3_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_i2c1_lut[] = {
	{215250, div_clk_cmgp_i2c1_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_i2c2_lut[] = {
	{215250, div_clk_cmgp_i2c2_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_i2c3_lut[] = {
	{215250, div_clk_cmgp_i2c3_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_i2c4_lut[] = {
	{215250, div_clk_cmgp_i2c4_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_i2c5_lut[] = {
	{215250, div_clk_cmgp_i2c5_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cmgp_i2c6_lut[] = {
	{215250, div_clk_cmgp_i2c6_ud_lut_params},
};
struct vclk_lut cmucal_vclk_ap2cp_shared0_pll_clk_lut[] = {
	{800000, ap2cp_shared0_pll_clk_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cpucl0_shortstop_lut[] = {
	{600000, div_clk_cpucl0_shortstop_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cpucl0_cpu_lut[] = {
	{1200000, clkcmu_cpucl0_cpu_nm_lut_params},
	{667333, clkcmu_cpucl0_cpu_ud_lut_params},
	{400000, clkcmu_cpucl0_cpu_sud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cluster_atclk_lut[] = {
	{300000, div_clk_cluster_atclk_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_dpu_aud_cpu_lut[] = {
	{1200000, clkcmu_dpu_aud_cpu_nm_lut_params},
	{800000, clkcmu_dpu_aud_cpu_ud_lut_params},
	{400000, clkcmu_dpu_aud_cpu_sud_lut_params},
};
struct vclk_lut cmucal_vclk_clk_dpu_aud_dmic_lut[] = {
	{25000, clk_dpu_aud_dmic_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clk_dpu_aud_mclk_lut[] = {
	{25000, clk_dpu_aud_mclk_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peri_i2c_0_lut[] = {
	{200000, div_clk_peri_i2c_0_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_peri_ip_lut[] = {
	{400000, clkcmu_peri_ip_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peri_spi_lut[] = {
	{400000, div_clk_peri_spi_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peri_usi00_i2c_lut[] = {
	{200000, div_clk_peri_usi00_i2c_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peri_usi00_usi_lut[] = {
	{400000, div_clk_peri_usi00_usi_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peri_i2c_1_lut[] = {
	{200000, div_clk_peri_i2c_1_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peri_i2c_2_lut[] = {
	{200000, div_clk_peri_i2c_2_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peri_i2c_3_lut[] = {
	{200000, div_clk_peri_i2c_3_ud_lut_params},
};

/* COMMON VCLK -> LUT List */
struct vclk_lut cmucal_vclk_blk_cmu_lut[] = {
	{800000, blk_cmu_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_s2d_lut[] = {
	{100000, blk_s2d_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_apm_lut[] = {
	{430500, blk_apm_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_chub_lut[] = {
	{430500, blk_chub_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_core_lut[] = {
	{266667, blk_core_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_cpucl0_lut[] = {
	{1200000, blk_cpucl0_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_dpu_lut[] = {
	{600000, blk_dpu_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_vts_lut[] = {
	{430500, blk_vts_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_g3d_lut[] = {
	{222444, blk_g3d_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_mfc_lut[] = {
	{266667, blk_mfc_ud_lut_params},
};

/* Switch VCLK -> LUT Parameter List */
/*================================ SWPLL List =================================*/

/*================================ VCLK List =================================*/
unsigned int cmucal_vclk_size = 349;
struct vclk cmucal_vclk_list[] = {

/* DVFS VCLK*/
	CMUCAL_VCLK(VCLK_VDD_ALIVE, cmucal_vclk_vdd_alive_lut, cmucal_vclk_vdd_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_VDD_INT, cmucal_vclk_vdd_int_lut, cmucal_vclk_vdd_int, NULL, NULL),

/* SPECIAL VCLK*/
	CMUCAL_VCLK(VCLK_MUX_CLK_APM_I3C_PMIC, cmucal_vclk_mux_clk_apm_i3c_pmic_lut, cmucal_vclk_mux_clk_apm_i3c_pmic, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_APM_BUS, cmucal_vclk_clkcmu_apm_bus_lut, cmucal_vclk_clkcmu_apm_bus, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLK_APM_TIMER, cmucal_vclk_mux_clk_apm_timer_lut, cmucal_vclk_mux_clk_apm_timer, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLK_CHUB_TIMER, cmucal_vclk_mux_clk_chub_timer_lut, cmucal_vclk_mux_clk_chub_timer, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_CMGP_ADC, cmucal_vclk_clk_cmgp_adc_lut, cmucal_vclk_clk_cmgp_adc, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CMGP_PERI, cmucal_vclk_clkcmu_cmgp_peri_lut, cmucal_vclk_clkcmu_cmgp_peri, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CMU_CMUREF, cmucal_vclk_mux_cmu_cmuref_lut, cmucal_vclk_mux_cmu_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CORE_CMUREF, cmucal_vclk_mux_core_cmuref_lut, cmucal_vclk_mux_core_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CMU_BOOST, cmucal_vclk_clkcmu_cmu_boost_lut, cmucal_vclk_clkcmu_cmu_boost, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CPUCL0_CMUREF, cmucal_vclk_mux_cpucl0_cmuref_lut, cmucal_vclk_mux_cpucl0_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CPUCL0_BUSP, cmucal_vclk_clkcmu_cpucl0_busp_lut, cmucal_vclk_clkcmu_cpucl0_busp, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLK_AUD_UAIF0, cmucal_vclk_mux_clk_aud_uaif0_lut, cmucal_vclk_mux_clk_aud_uaif0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_DPU_AUDIF, cmucal_vclk_clkcmu_dpu_audif_lut, cmucal_vclk_clkcmu_dpu_audif, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_MIF_CMUREF, cmucal_vclk_mux_mif_cmuref_lut, cmucal_vclk_mux_mif_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_MIF_BUSP, cmucal_vclk_clkcmu_mif_busp_lut, cmucal_vclk_clkcmu_mif_busp, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_FSYS_USB20DRD, cmucal_vclk_clkcmu_fsys_usb20drd_lut, cmucal_vclk_clkcmu_fsys_usb20drd, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_PERI_UART, cmucal_vclk_clkcmu_peri_uart_lut, cmucal_vclk_clkcmu_peri_uart, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_APM_UART_DBGCORE, cmucal_vclk_div_clk_apm_uart_dbgcore_lut, cmucal_vclk_div_clk_apm_uart_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CHUB_USI0, cmucal_vclk_div_clk_chub_usi0_lut, cmucal_vclk_div_clk_chub_usi0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CHUB_BUS, cmucal_vclk_clkcmu_chub_bus_lut, cmucal_vclk_clkcmu_chub_bus, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CHUB_I2C0, cmucal_vclk_div_clk_chub_i2c0_lut, cmucal_vclk_div_clk_chub_i2c0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CHUB_I2C1, cmucal_vclk_div_clk_chub_i2c1_lut, cmucal_vclk_div_clk_chub_i2c1, NULL, NULL),
// for cmgp_xx
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_USI0, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_usi0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_USI1, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_usi1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_USI2, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_usi2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_USI3, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_usi3, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_I2C0, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_i2c0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_I2C1, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_i2c1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_I2C2, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_i2c2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_I2C3, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_i2c3, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_I2C4, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_i2c4, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_I2C5, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_i2c5, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CMGP_I2C6, cmucal_vclk_div_clk_cmgp_xx_lut, cmucal_vclk_div_clk_cmgp_i2c6, NULL, NULL),
// end cmgp_xx
	CMUCAL_VCLK(VCLK_AP2CP_SHARED0_PLL_CLK, cmucal_vclk_ap2cp_shared0_pll_clk_lut, cmucal_vclk_ap2cp_shared0_pll_clk, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CPUCL0_SHORTSTOP, cmucal_vclk_div_clk_cpucl0_shortstop_lut, cmucal_vclk_div_clk_cpucl0_shortstop, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CPUCL0_CPU, cmucal_vclk_clkcmu_cpucl0_cpu_lut, cmucal_vclk_clkcmu_cpucl0_cpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER_ATCLK, cmucal_vclk_div_clk_cluster_atclk_lut, cmucal_vclk_div_clk_cluster_atclk, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_DPU_AUD_CPU, cmucal_vclk_clkcmu_dpu_aud_cpu_lut, cmucal_vclk_clkcmu_dpu_aud_cpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_DPU_AUD_DMIC, cmucal_vclk_clk_dpu_aud_dmic_lut, cmucal_vclk_clk_dpu_aud_dmic, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_DPU_AUD_MCLK, cmucal_vclk_clk_dpu_aud_mclk_lut, cmucal_vclk_clk_dpu_aud_mclk, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERI_I2C_0, cmucal_vclk_div_clk_peri_i2c_0_lut, cmucal_vclk_div_clk_peri_i2c_0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_PERI_IP, cmucal_vclk_clkcmu_peri_ip_lut, cmucal_vclk_clkcmu_peri_ip, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERI_SPI, cmucal_vclk_div_clk_peri_spi_lut, cmucal_vclk_div_clk_peri_spi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERI_USI00_I2C, cmucal_vclk_div_clk_peri_usi00_i2c_lut, cmucal_vclk_div_clk_peri_usi00_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERI_USI00_USI, cmucal_vclk_div_clk_peri_usi00_usi_lut, cmucal_vclk_div_clk_peri_usi00_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERI_I2C_1, cmucal_vclk_div_clk_peri_i2c_1_lut, cmucal_vclk_div_clk_peri_i2c_1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERI_I2C_2, cmucal_vclk_div_clk_peri_i2c_2_lut, cmucal_vclk_div_clk_peri_i2c_2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERI_I2C_3, cmucal_vclk_div_clk_peri_i2c_3_lut, cmucal_vclk_div_clk_peri_i2c_3, NULL, NULL),

/* COMMON VCLK*/
	CMUCAL_VCLK(VCLK_BLK_CMU, cmucal_vclk_blk_cmu_lut, cmucal_vclk_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_S2D, cmucal_vclk_blk_s2d_lut, cmucal_vclk_blk_s2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_APM, cmucal_vclk_blk_apm_lut, cmucal_vclk_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CHUB, cmucal_vclk_blk_chub_lut, cmucal_vclk_blk_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CORE, cmucal_vclk_blk_core_lut, cmucal_vclk_blk_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CPUCL0, cmucal_vclk_blk_cpucl0_lut, cmucal_vclk_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DPU, cmucal_vclk_blk_dpu_lut, cmucal_vclk_blk_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_VTS, cmucal_vclk_blk_vts_lut, cmucal_vclk_blk_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_G3D, cmucal_vclk_blk_g3d_lut, cmucal_vclk_blk_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MFC, cmucal_vclk_blk_mfc_lut, cmucal_vclk_blk_mfc, NULL, NULL),

/* GATE VCLK*/
	CMUCAL_VCLK(VCLK_IP_APBIF_RTC_TOP, NULL, cmucal_vclk_ip_apbif_rtc_top, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APM_CMU_APM, NULL, cmucal_vclk_ip_apm_cmu_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_DP_APM, NULL, cmucal_vclk_ip_xiu_dp_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_CP_GNSS, NULL, cmucal_vclk_ip_mailbox_cp_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_CP_WPAN, NULL, cmucal_vclk_ip_mailbox_cp_wpan, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_WPAN, NULL, cmucal_vclk_ip_mailbox_ap_wpan, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_GPIO_ALIVE, NULL, cmucal_vclk_ip_apbif_gpio_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_PMU_ALIVE, NULL, cmucal_vclk_ip_apbif_pmu_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_RTC, NULL, cmucal_vclk_ip_apbif_rtc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_RTC_CHUB, NULL, cmucal_vclk_ip_apbif_rtc_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_VGPIO2AP, NULL, cmucal_vclk_ip_apbif_sysreg_vgpio2ap, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_VGPIO2APM, NULL, cmucal_vclk_ip_apbif_sysreg_vgpio2apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_VGPIO2PMU, NULL, cmucal_vclk_ip_apbif_sysreg_vgpio2pmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_ALIVE_0, NULL, cmucal_vclk_ip_d_tzpc_alive_0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GREBE_APM, NULL, cmucal_vclk_ip_grebe_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GREBE_DBGCORE, NULL, cmucal_vclk_ip_grebe_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_HW_SCANDUMP_CLKSTOP_CTRL, NULL, cmucal_vclk_ip_hw_scandump_clkstop_ctrl, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I3C_PMIC, NULL, cmucal_vclk_ip_i3c_pmic, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_INTMEM_ALIVE, NULL, cmucal_vclk_ip_intmem_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_APM_AP, NULL, cmucal_vclk_ip_mailbox_apm_ap, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_APM_CHUB, NULL, cmucal_vclk_ip_mailbox_apm_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_APM_CP, NULL, cmucal_vclk_ip_mailbox_apm_cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_APM_GNSS, NULL, cmucal_vclk_ip_mailbox_apm_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_APM_VTS, NULL, cmucal_vclk_ip_mailbox_apm_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_APM_WLBT, NULL, cmucal_vclk_ip_mailbox_apm_wlbt, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_CHUB, NULL, cmucal_vclk_ip_mailbox_ap_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_CP, NULL, cmucal_vclk_ip_mailbox_ap_cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_CP_S, NULL, cmucal_vclk_ip_mailbox_ap_cp_s, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_DBGCORE, NULL, cmucal_vclk_ip_mailbox_ap_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_GNSS, NULL, cmucal_vclk_ip_mailbox_ap_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_WLAN, NULL, cmucal_vclk_ip_mailbox_ap_wlan, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_CP_CHUB, NULL, cmucal_vclk_ip_mailbox_cp_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_CP_WLAN, NULL, cmucal_vclk_ip_mailbox_cp_wlan, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_GNSS_CHUB, NULL, cmucal_vclk_ip_mailbox_gnss_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_GNSS_WLBT, NULL, cmucal_vclk_ip_mailbox_gnss_wlbt, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_WLBT_ABOX, NULL, cmucal_vclk_ip_mailbox_wlbt_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_WLBT_CHUB0, NULL, cmucal_vclk_ip_mailbox_wlbt_chub0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_WLBT_CHUB1, NULL, cmucal_vclk_ip_mailbox_wlbt_chub1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PMU_INTR_GEN, NULL, cmucal_vclk_ip_pmu_intr_gen, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_ROM_CRC32_HOST, NULL, cmucal_vclk_ip_rom_crc32_host, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_C_CHUB, NULL, cmucal_vclk_ip_slh_axi_mi_c_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_C_GNSS, NULL, cmucal_vclk_ip_slh_axi_mi_c_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_C_MODEM, NULL, cmucal_vclk_ip_slh_axi_mi_c_modem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_C_VTS, NULL, cmucal_vclk_ip_slh_axi_mi_c_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_C_WLBT, NULL, cmucal_vclk_ip_slh_axi_mi_c_wlbt, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_ALIVE, NULL, cmucal_vclk_ip_slh_axi_mi_p_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_D_ALIVE, NULL, cmucal_vclk_ip_slh_axi_si_d_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_G_DBGCORE, NULL, cmucal_vclk_ip_slh_axi_si_g_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_G_SCAN2DRAM, NULL, cmucal_vclk_ip_slh_axi_si_g_scan2dram, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_LP_CHUB, NULL, cmucal_vclk_ip_slh_axi_si_lp_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_LP_VTS, NULL, cmucal_vclk_ip_slh_axi_si_lp_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_APM, NULL, cmucal_vclk_ip_sysreg_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_UART_DBGCORE, NULL, cmucal_vclk_ip_uart_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT_APM, NULL, cmucal_vclk_ip_wdt_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT_DBGCORE, NULL, cmucal_vclk_ip_wdt_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_C_CHUB, NULL, cmucal_vclk_ip_slh_axi_si_c_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CHUB_CMU_CHUB, NULL, cmucal_vclk_ip_chub_cmu_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT_CHUB1, NULL, cmucal_vclk_ip_wdt_chub1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AXI_DS_64TO32_CHUB, NULL, cmucal_vclk_ip_axi_ds_64to32_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_C_CHUB, NULL, cmucal_vclk_ip_baaw_c_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_D_CHUB, NULL, cmucal_vclk_ip_baaw_d_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BPS_AXI_LP_CHUB, NULL, cmucal_vclk_ip_bps_axi_lp_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BPS_AXI_P_CHUB, NULL, cmucal_vclk_ip_bps_axi_p_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CM4_CHUB, NULL, cmucal_vclk_ip_cm4_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DAPASYNC_CHUB, NULL, cmucal_vclk_ip_dapasync_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_CHUB, NULL, cmucal_vclk_ip_d_tzpc_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CHUB0, NULL, cmucal_vclk_ip_i2c_chub0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CHUB1, NULL, cmucal_vclk_ip_i2c_chub1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PDMA_CHUB, NULL, cmucal_vclk_ip_pdma_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_CHUB, NULL, cmucal_vclk_ip_ppmu_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PWM_CHUB, NULL, cmucal_vclk_ip_pwm_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_LP_CHUB, NULL, cmucal_vclk_ip_slh_axi_mi_lp_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_CHUB, NULL, cmucal_vclk_ip_slh_axi_mi_p_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_D_CHUB, NULL, cmucal_vclk_ip_slh_axi_si_d_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SWEEPER_C_CHUB, NULL, cmucal_vclk_ip_sweeper_c_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SWEEPER_D_CHUB, NULL, cmucal_vclk_ip_sweeper_d_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_CHUB, NULL, cmucal_vclk_ip_sysreg_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_TIMER_CHUB, NULL, cmucal_vclk_ip_timer_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI_CHUB, NULL, cmucal_vclk_ip_usi_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT_CHUB0, NULL, cmucal_vclk_ip_wdt_chub0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_DP_CHUB, NULL, cmucal_vclk_ip_xiu_dp_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_YAMIN_MCU_CHUB, NULL, cmucal_vclk_ip_yamin_mcu_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CMGP_CMU_CMGP, NULL, cmucal_vclk_ip_cmgp_cmu_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CMGP6, NULL, cmucal_vclk_ip_i2c_cmgp6, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CMGP4, NULL, cmucal_vclk_ip_i2c_cmgp4, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CMGP2, NULL, cmucal_vclk_ip_i2c_cmgp2, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CMGP1, NULL, cmucal_vclk_ip_i2c_cmgp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CMGP0, NULL, cmucal_vclk_ip_i2c_cmgp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CMGP3, NULL, cmucal_vclk_ip_i2c_cmgp3, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_CMGP5, NULL, cmucal_vclk_ip_i2c_cmgp5, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_ALIVE_1, NULL, cmucal_vclk_ip_d_tzpc_alive_1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_CMGP2WLBT, NULL, cmucal_vclk_ip_apbif_sysreg_cmgp2wlbt, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_CMGP2PMU_CHUB, NULL, cmucal_vclk_ip_apbif_sysreg_cmgp2pmu_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_CMGP2PMU_AP, NULL, cmucal_vclk_ip_apbif_sysreg_cmgp2pmu_ap, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_CMGP2GNSS, NULL, cmucal_vclk_ip_apbif_sysreg_cmgp2gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_CMGP2CP, NULL, cmucal_vclk_ip_apbif_sysreg_cmgp2cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_SYSREG_CMGP2CHUB, NULL, cmucal_vclk_ip_apbif_sysreg_cmgp2chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_ADC_CMGP, NULL, cmucal_vclk_ip_adc_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_CMGP, NULL, cmucal_vclk_ip_sysreg_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI_CMGP0, NULL, cmucal_vclk_ip_usi_cmgp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI_CMGP1, NULL, cmucal_vclk_ip_usi_cmgp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI_CMGP2, NULL, cmucal_vclk_ip_usi_cmgp2, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI_CMGP3, NULL, cmucal_vclk_ip_usi_cmgp3, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_APBIF_GPIO_CMGP, NULL, cmucal_vclk_ip_apbif_gpio_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AD_APB_PDMA0, NULL, cmucal_vclk_ip_ad_apb_pdma0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AD_APB_SPDMA, NULL, cmucal_vclk_ip_ad_apb_spdma, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_P_GNSS, NULL, cmucal_vclk_ip_baaw_p_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_P_CHUB, NULL, cmucal_vclk_ip_baaw_p_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_P_MODEM, NULL, cmucal_vclk_ip_baaw_p_modem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_P_WLBT, NULL, cmucal_vclk_ip_baaw_p_wlbt, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GIC400_AIHWACG, NULL, cmucal_vclk_ip_gic400_aihwacg, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_MFC, NULL, cmucal_vclk_ip_slh_axi_mi_d_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_VTS, NULL, cmucal_vclk_ip_slh_axi_mi_d_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_D_DPU, NULL, cmucal_vclk_ip_lh_axi_mi_d_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_FSYS, NULL, cmucal_vclk_ip_slh_axi_mi_d_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D0_MODEM, NULL, cmucal_vclk_ip_slh_axi_mi_d0_modem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D1_MODEM, NULL, cmucal_vclk_ip_slh_axi_mi_d1_modem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_ALIVE, NULL, cmucal_vclk_ip_slh_axi_mi_d_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_CHUB, NULL, cmucal_vclk_ip_slh_axi_mi_d_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_CSSYS, NULL, cmucal_vclk_ip_slh_axi_mi_d_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_GNSS, NULL, cmucal_vclk_ip_slh_axi_mi_d_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_D_WLBT, NULL, cmucal_vclk_ip_slh_axi_mi_d_wlbt, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_ALIVE, NULL, cmucal_vclk_ip_slh_axi_si_p_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_CHUB, NULL, cmucal_vclk_ip_slh_axi_si_p_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_CPUCL0, NULL, cmucal_vclk_ip_slh_axi_si_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_DPU, NULL, cmucal_vclk_ip_slh_axi_si_p_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_FSYS, NULL, cmucal_vclk_ip_slh_axi_si_p_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_GNSS, NULL, cmucal_vclk_ip_slh_axi_si_p_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_MFC, NULL, cmucal_vclk_ip_slh_axi_si_p_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_VTS, NULL, cmucal_vclk_ip_slh_axi_si_p_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_MODEM, NULL, cmucal_vclk_ip_slh_axi_si_p_modem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_PERI, NULL, cmucal_vclk_ip_slh_axi_si_p_peri, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_WLBT, NULL, cmucal_vclk_ip_slh_axi_si_p_wlbt, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PDMA_CORE, NULL, cmucal_vclk_ip_pdma_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SFR_APBIF_CMU_TOPC, NULL, cmucal_vclk_ip_sfr_apbif_cmu_topc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SIREX, NULL, cmucal_vclk_ip_sirex, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SPDMA_CORE, NULL, cmucal_vclk_ip_spdma_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_CORE, NULL, cmucal_vclk_ip_sysreg_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_D_CPUCL0, NULL, cmucal_vclk_ip_lh_axi_mi_d_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_RTIC, NULL, cmucal_vclk_ip_rtic, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SSS, NULL, cmucal_vclk_ip_sss, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_P_VTS, NULL, cmucal_vclk_ip_baaw_p_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_CORE, NULL, cmucal_vclk_ip_d_tzpc_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_D_G3D, NULL, cmucal_vclk_ip_lh_axi_mi_d_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_G3D, NULL, cmucal_vclk_ip_slh_axi_si_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AD_DAP_CSSYS_CPUCL0, NULL, cmucal_vclk_ip_ad_dap_cssys_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_P_MIF, NULL, cmucal_vclk_ip_slh_axi_si_p_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_D0_MIF, NULL, cmucal_vclk_ip_lh_axi_si_d0_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_D1_MIF, NULL, cmucal_vclk_ip_lh_axi_si_d1_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_D3_MIF, NULL, cmucal_vclk_ip_lh_axi_si_d3_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CORE_CMU_CORE, NULL, cmucal_vclk_ip_core_cmu_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_TREX_P_CORE, NULL, cmucal_vclk_ip_trex_p_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_D2_MIF, NULL, cmucal_vclk_ip_lh_axi_si_d2_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_TREX_D_CORE, NULL, cmucal_vclk_ip_trex_d_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CMU_CORE_CMUREF, NULL, cmucal_vclk_ip_cmu_core_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_CPUCL0, NULL, cmucal_vclk_ip_sysreg_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CLUSTER0, NULL, cmucal_vclk_ip_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_CPUCL0, NULL, cmucal_vclk_ip_slh_axi_mi_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_CPUCL0, NULL, cmucal_vclk_ip_d_tzpc_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_P_CPUCL0, NULL, cmucal_vclk_ip_xiu_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_CPUCL0, NULL, cmucal_vclk_ip_ppmu_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_G_DBGCORE, NULL, cmucal_vclk_ip_slh_axi_mi_g_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_IG_DBGCORE, NULL, cmucal_vclk_ip_lh_axi_si_ig_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_DP_DBGCORE, NULL, cmucal_vclk_ip_xiu_dp_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GRAY2BIN_ATCLK, NULL, cmucal_vclk_ip_gray2bin_atclk, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CPUCL0_CMU_CPUCL0, NULL, cmucal_vclk_ip_cpucl0_cmu_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CSSYS_DBG, NULL, cmucal_vclk_ip_cssys_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_D_CSSYS, NULL, cmucal_vclk_ip_slh_axi_si_d_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SECJTAG, NULL, cmucal_vclk_ip_secjtag, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_D_CPUCL0, NULL, cmucal_vclk_ip_lh_axi_si_d_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_IG_DBGCORE, NULL, cmucal_vclk_ip_lh_axi_mi_ig_dbgcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AD_APB_P_XIU2CSSYS, NULL, cmucal_vclk_ip_ad_apb_p_xiu2cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_ADM_APB_D_CSSYS, NULL, cmucal_vclk_ip_adm_apb_d_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AUD, NULL, cmucal_vclk_ip_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PERI_AXI_ASB, NULL, cmucal_vclk_ip_peri_axi_asb, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT_AUD, NULL, cmucal_vclk_ip_wdt_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DFTMUX_DPU, NULL, cmucal_vclk_ip_dftmux_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_AUD, NULL, cmucal_vclk_ip_ppmu_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSMMU_AUD, NULL, cmucal_vclk_ip_sysmmu_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_DPU, NULL, cmucal_vclk_ip_ppmu_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_DPU, NULL, cmucal_vclk_ip_sysreg_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_DPU, NULL, cmucal_vclk_ip_slh_axi_mi_p_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_D_DPU, NULL, cmucal_vclk_ip_lh_axi_si_d_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSMMU_DPU, NULL, cmucal_vclk_ip_sysmmu_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DPU, NULL, cmucal_vclk_ip_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GPIO_DPU, NULL, cmucal_vclk_ip_gpio_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AD_APB_DECON0, NULL, cmucal_vclk_ip_ad_apb_decon0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DMIC, NULL, cmucal_vclk_ip_dmic, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_DPU, NULL, cmucal_vclk_ip_d_tzpc_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AD_APB_SMMU_AUD_NS, NULL, cmucal_vclk_ip_ad_apb_smmu_aud_ns, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DPU_CMU_DPU, NULL, cmucal_vclk_ip_dpu_cmu_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_QE_DPU, NULL, cmucal_vclk_ip_qe_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_QE_AUD, NULL, cmucal_vclk_ip_qe_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_D_DPU, NULL, cmucal_vclk_ip_xiu_d_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DPU_AXI_ASB, NULL, cmucal_vclk_ip_dpu_axi_asb, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GPIO_FSYS, NULL, cmucal_vclk_ip_gpio_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_FSYS, NULL, cmucal_vclk_ip_slh_axi_mi_p_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_D_FSYS, NULL, cmucal_vclk_ip_slh_axi_si_d_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_FSYS, NULL, cmucal_vclk_ip_ppmu_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_FSYS, NULL, cmucal_vclk_ip_sysreg_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_D_FSYS, NULL, cmucal_vclk_ip_xiu_d_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MMC_CARD, NULL, cmucal_vclk_ip_mmc_card, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MMC_EMBD, NULL, cmucal_vclk_ip_mmc_embd, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_FSYS, NULL, cmucal_vclk_ip_d_tzpc_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USB20DRD_TOP, NULL, cmucal_vclk_ip_usb20drd_top, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_US_64TO128_FSYS, NULL, cmucal_vclk_ip_us_64to128_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_P_FSYS, NULL, cmucal_vclk_ip_xiu_p_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_FSYS_CMU_FSYS, NULL, cmucal_vclk_ip_fsys_cmu_fsys, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_G3D, NULL, cmucal_vclk_ip_slh_axi_mi_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_G3D, NULL, cmucal_vclk_ip_sysreg_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GRAY2BIN_G3D, NULL, cmucal_vclk_ip_gray2bin_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_D_G3D, NULL, cmucal_vclk_ip_lh_axi_si_d_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_G3D, NULL, cmucal_vclk_ip_d_tzpc_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_G3D, NULL, cmucal_vclk_ip_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_G3D, NULL, cmucal_vclk_ip_ppmu_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_G3D_CMU_G3D, NULL, cmucal_vclk_ip_g3d_cmu_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_SI_IP_G3D, NULL, cmucal_vclk_ip_lh_axi_si_ip_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_IP_G3D, NULL, cmucal_vclk_ip_lh_axi_mi_ip_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MFC, NULL, cmucal_vclk_ip_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_MFC, NULL, cmucal_vclk_ip_ppmu_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_MFC, NULL, cmucal_vclk_ip_sysreg_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_MFC, NULL, cmucal_vclk_ip_slh_axi_mi_p_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_MFC, NULL, cmucal_vclk_ip_d_tzpc_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_D_MFC, NULL, cmucal_vclk_ip_slh_axi_si_d_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AS_APB_MFC, NULL, cmucal_vclk_ip_as_apb_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_XIU_D_MFC, NULL, cmucal_vclk_ip_xiu_d_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSMMU_MFC, NULL, cmucal_vclk_ip_sysmmu_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MFC_CMU_MFC, NULL, cmucal_vclk_ip_mfc_cmu_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BL, NULL, cmucal_vclk_ip_bl, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_MIF, NULL, cmucal_vclk_ip_sysreg_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DMC, NULL, cmucal_vclk_ip_dmc, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_MIF, NULL, cmucal_vclk_ip_d_tzpc_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_D0_MIF, NULL, cmucal_vclk_ip_lh_axi_mi_d0_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_D1_MIF, NULL, cmucal_vclk_ip_lh_axi_mi_d1_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_D2_MIF, NULL, cmucal_vclk_ip_lh_axi_mi_d2_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_LH_AXI_MI_D3_MIF, NULL, cmucal_vclk_ip_lh_axi_mi_d3_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_MIF, NULL, cmucal_vclk_ip_slh_axi_mi_p_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MIF_CMU_MIF, NULL, cmucal_vclk_ip_mif_cmu_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DDR_PHY, NULL, cmucal_vclk_ip_ddr_phy, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MODEM_CMU_MODEM, NULL, cmucal_vclk_ip_modem_cmu_modem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PERI_CMU_PERI, NULL, cmucal_vclk_ip_peri_cmu_peri, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_PERI, NULL, cmucal_vclk_ip_d_tzpc_peri, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_OTP_CON_TOP, NULL, cmucal_vclk_ip_otp_con_top, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BUSIF_TMU, NULL, cmucal_vclk_ip_busif_tmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GPIO_PERI, NULL, cmucal_vclk_ip_gpio_peri, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_0, NULL, cmucal_vclk_ip_i2c_0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_1, NULL, cmucal_vclk_ip_i2c_1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_2, NULL, cmucal_vclk_ip_i2c_2, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_I2C_3, NULL, cmucal_vclk_ip_i2c_3, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MCT, NULL, cmucal_vclk_ip_mct, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PWM_MOTOR, NULL, cmucal_vclk_ip_pwm_motor, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_PERI, NULL, cmucal_vclk_ip_slh_axi_mi_p_peri, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_PERI, NULL, cmucal_vclk_ip_sysreg_peri, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI00_I2C, NULL, cmucal_vclk_ip_usi00_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI00_USI, NULL, cmucal_vclk_ip_usi00_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI_SPI, NULL, cmucal_vclk_ip_usi_spi, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_USI_UART, NULL, cmucal_vclk_ip_usi_uart, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT0, NULL, cmucal_vclk_ip_wdt0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT1, NULL, cmucal_vclk_ip_wdt1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BIS_S2D, NULL, cmucal_vclk_ip_bis_s2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_G_SCAN2DRAM, NULL, cmucal_vclk_ip_slh_axi_mi_g_scan2dram, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_S2D_CMU_S2D, NULL, cmucal_vclk_ip_s2d_cmu_s2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DMIC_IF, NULL, cmucal_vclk_ip_dmic_if, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_VTS_CMU_VTS, NULL, cmucal_vclk_ip_vts_cmu_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_P_VTS, NULL, cmucal_vclk_ip_slh_axi_mi_p_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_AHB_BUSMATRIX_VTS, NULL, cmucal_vclk_ip_ahb_busmatrix_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_C_VTS, NULL, cmucal_vclk_ip_baaw_c_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BAAW_D_VTS, NULL, cmucal_vclk_ip_baaw_d_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BPS_AXI_LP_VTS, NULL, cmucal_vclk_ip_bps_axi_lp_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_BPS_AXI_P_VTS, NULL, cmucal_vclk_ip_bps_axi_p_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_CORTEXM4INTEGRATION, NULL, cmucal_vclk_ip_cortexm4integration, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DMIC_AHB0, NULL, cmucal_vclk_ip_dmic_ahb0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_DMIC_AHB1, NULL, cmucal_vclk_ip_dmic_ahb1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_D_TZPC_VTS, NULL, cmucal_vclk_ip_d_tzpc_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_GPIO_VTS, NULL, cmucal_vclk_ip_gpio_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_HWACG_SYS_DMIC0, NULL, cmucal_vclk_ip_hwacg_sys_dmic0, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_HWACG_SYS_DMIC1, NULL, cmucal_vclk_ip_hwacg_sys_dmic1, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AP_VTS, NULL, cmucal_vclk_ip_mailbox_ap_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_MAILBOX_AUD_VTS, NULL, cmucal_vclk_ip_mailbox_aud_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_PPMU_VTS, NULL, cmucal_vclk_ip_ppmu_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_MI_LP_VTS, NULL, cmucal_vclk_ip_slh_axi_mi_lp_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_C_VTS, NULL, cmucal_vclk_ip_slh_axi_si_c_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SLH_AXI_SI_D_VTS, NULL, cmucal_vclk_ip_slh_axi_si_d_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SWEEPER_C_VTS, NULL, cmucal_vclk_ip_sweeper_c_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SWEEPER_D_VTS, NULL, cmucal_vclk_ip_sweeper_d_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_SYSREG_VTS, NULL, cmucal_vclk_ip_sysreg_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_TIMER_VTS, NULL, cmucal_vclk_ip_timer_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_WDT_VTS, NULL, cmucal_vclk_ip_wdt_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_IP_U_DMIC_CLK_MUX, NULL, cmucal_vclk_ip_u_dmic_clk_mux, NULL, NULL),
};

