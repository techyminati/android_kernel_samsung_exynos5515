#include "../cmucal.h"
#include "cmucal-vclklut.h"


/* DVFS VCLK -> LUT Parameter List */
unsigned int vdd_alive_ud_lut_params[] = {
	0, 1, 0, 0, 0, 0, 
};
unsigned int vdd_alive_sud_lut_params[] = {
	3, 1, 1, 1, 1, 1, 
};
unsigned int vdd_alive_nm_lut_params[] = {
	0, 0, 0, 0, 0, 0, 
};
unsigned int vdd_int_nm_lut_params[] = {
	1, 1, 2, 0, 0, 0, 0, 2, 0, 0, 0, 1, 2, 0, 0, 1, 0, 1, 4264000, 0, 1, 
};
unsigned int vdd_int_ud_lut_params[] = {
	0, 3, 3, 0, 0, 3, 1, 1, 2, 1, 1, 0, 1, 1, 2, 1, 1, 0, 3400000, 1, 0, 
};
unsigned int vdd_int_sud_lut_params[] = {
	2, 3, 7, 1, 1, 7, 1, 3, 6, 1, 3, 0, 1, 3, 1, 3, 2, 0, 2000000, 0, 1, 
};

/* SPECIAL VCLK -> LUT Parameter List */
unsigned int mux_clk_apm_i3c_pmic_ud_lut_params[] = {
	0, 1, 
};
unsigned int clkcmu_apm_bus_ud_lut_params[] = {
	1, 0, 
};
unsigned int mux_clk_apm_timer_ud_lut_params[] = {
	0, 
};
unsigned int mux_clk_chub_timer_ud_lut_params[] = {
	0, 
};
unsigned int clk_cmgp_adc_ud_lut_params[] = {
	1, 13, 
};
unsigned int clkcmu_cmgp_peri_ud_lut_params[] = {
	0, 1, 
};
unsigned int mux_cmu_cmuref_ud_lut_params[] = {
	1, 1, 
};
unsigned int mux_core_cmuref_ud_lut_params[] = {
	1, 
};
unsigned int clkcmu_cmu_boost_ud_lut_params[] = {
	2, 
};
unsigned int mux_cpucl0_cmuref_ud_lut_params[] = {
	1, 
};
unsigned int clkcmu_cpucl0_busp_ud_lut_params[] = {
	1, 1, 
};
unsigned int mux_clk_aud_uaif0_ud_lut_params[] = {
	0, 2, 0, 2, 0, 2, 7, 2, 
};
unsigned int clkcmu_dpu_audif_ud_lut_params[] = {
	7, 0, 
};
unsigned int mux_mif_cmuref_ud_lut_params[] = {
	1, 
};
unsigned int clkcmu_mif_busp_ud_lut_params[] = {
	1, 1, 
};
unsigned int clkcmu_fsys_usb20drd_ud_lut_params[] = {
	7, 1, 
};
unsigned int clkcmu_peri_uart_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_apm_uart_dbgcore_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_chub_usi0_ud_lut_params[] = {
	0, 1, 
};
unsigned int clkcmu_chub_bus_ud_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_chub_i2c0_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_chub_i2c1_ud_lut_params[] = {
	1, 1, 
};

unsigned int div_clk_cmgp_400_lut_params[] = {
	0, 1,
};
unsigned int div_clk_cmgp_200_lut_params[] = {
	1, 1,
};
unsigned int div_clk_cmgp_133_lut_params[] = {
	2, 1,
};
unsigned int div_clk_cmgp_100_lut_params[] = {
	3, 1,
};
unsigned int div_clk_cmgp_80_lut_params[] = {
	4, 1,
};
unsigned int div_clk_cmgp_66_lut_params[] = {
	5, 1,
};
unsigned int div_clk_cmgp_57_lut_params[] = {
	6, 1,
};
unsigned int div_clk_cmgp_50_lut_params[] = {
	7, 1,
};
unsigned int div_clk_cmgp_44_lut_params[] = {
	8, 1,
};
unsigned int div_clk_cmgp_40_lut_params[] = {
	9, 1,
};
unsigned int div_clk_cmgp_36_lut_params[] = {
	10, 1,
};
unsigned int div_clk_cmgp_33_lut_params[] = {
	11, 1,
};
unsigned int div_clk_cmgp_30_lut_params[] = {
	12, 1,
};
unsigned int div_clk_cmgp_28_lut_params[] = {
	13, 1,
};
unsigned int div_clk_cmgp_20_lut_params[] = {
	19, 1,
};
unsigned int div_clk_cmgp_16_lut_params[] = {
	24, 1,
};

unsigned int div_clk_cmgp_26_lut_params[] = {
	0, 0,
};
unsigned int div_clk_cmgp_13_lut_params[] = {
	1, 0,
};
unsigned int div_clk_cmgp_8_lut_params[] = {
	2, 0,
};
unsigned int div_clk_cmgp_6_lut_params[] = {
	3, 0,
};

unsigned int div_clk_cmgp_i2c0_ud_lut_params[] = {
	1, 1, 
};
unsigned int mux_clk_cmgp_usi0_ud_lut_params[] = {
	1, 
};
unsigned int mux_clk_cmgp_usi1_ud_lut_params[] = {
	1, 
};
unsigned int mux_clk_cmgp_usi2_ud_lut_params[] = {
	1, 
};
unsigned int mux_clk_cmgp_usi3_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_cmgp_i2c1_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_cmgp_i2c2_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_cmgp_i2c3_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_cmgp_i2c4_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_cmgp_i2c5_ud_lut_params[] = {
	1, 1, 
};
unsigned int div_clk_cmgp_i2c6_ud_lut_params[] = {
	1, 1, 
};
unsigned int ap2cp_shared0_pll_clk_ud_lut_params[] = {
	0, 
};
unsigned int div_clk_cpucl0_shortstop_ud_lut_params[] = {
	1, 
};
unsigned int clkcmu_cpucl0_cpu_nm_lut_params[] = {
	0, 
};
unsigned int clkcmu_cpucl0_cpu_ud_lut_params[] = {
	0, 
};
unsigned int clkcmu_cpucl0_cpu_sud_lut_params[] = {
	0, 
};
unsigned int div_clk_cluster_atclk_ud_lut_params[] = {
	3, 
};
unsigned int clkcmu_dpu_aud_cpu_nm_lut_params[] = {
	0, 
};
unsigned int clkcmu_dpu_aud_cpu_ud_lut_params[] = {
	0, 
};
unsigned int clkcmu_dpu_aud_cpu_sud_lut_params[] = {
	0, 
};
unsigned int clk_dpu_aud_dmic_ud_lut_params[] = {
	2, 
};
unsigned int clk_dpu_aud_mclk_ud_lut_params[] = {
	2, 
};
unsigned int div_clk_peri_i2c_0_ud_lut_params[] = {
	1, 
};
unsigned int clkcmu_peri_ip_ud_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_peri_spi_ud_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_peri_usi00_i2c_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_peri_usi00_usi_ud_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_peri_i2c_1_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_peri_i2c_2_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_peri_i2c_3_ud_lut_params[] = {
	1, 
};

/* COMMON VCLK -> LUT Parameter List */
unsigned int blk_cmu_ud_lut_params[] = {
	1600000, 2002000, 1200000, 1, 1, 1, 1, 2, 1, 0, 1, 0, 
};
unsigned int blk_s2d_ud_lut_params[] = {
	800000, 1, 
};
unsigned int blk_apm_ud_lut_params[] = {
	2, 1, 
};
unsigned int blk_chub_ud_lut_params[] = {
	1, 0, 
};
unsigned int blk_core_ud_lut_params[] = {
	0, 1, 
};
unsigned int blk_cpucl0_ud_lut_params[] = {
	0, 0, 1, 3, 0, 
};
unsigned int blk_dpu_ud_lut_params[] = {
	0, 1, 
};
unsigned int blk_vts_ud_lut_params[] = {
	1, 0, 1, 1, 0, 
};
unsigned int blk_g3d_ud_lut_params[] = {
	2, 
};
unsigned int blk_mfc_ud_lut_params[] = {
	1, 
};
