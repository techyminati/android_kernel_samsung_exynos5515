#
# Makefile for the CMUCAL drivers.
#

# Object files in subdirectories

obj-y					+= cal-if.o
obj-$(CONFIG_PMUCAL)			+= pmucal_system.o pmucal_local.o pmucal_cpu.o pmucal_rae.o
obj-$(CONFIG_PMUCAL_DBG)		+= pmucal_dbg.o
obj-$(CONFIG_FLEXPMU)			+= pmucal_powermode.o
obj-$(CONFIG_CP_PMUCAL)			+= pmucal_cp.o
obj-$(CONFIG_GNSS_PMUCAL)		+= pmucal_gnss.o
obj-$(CONFIG_CHUB_PMUCAL)		+= pmucal_chub.o
obj-$(CONFIG_CMUCAL)			+= cmucal.o ra.o vclk.o pll_spec.o
obj-$(CONFIG_CMUCAL_DEBUG)		+= cmucal-debug.o
obj-$(CONFIG_ACPM_DVFS)			+= acpm_dvfs.o fvmap.o
obj-$(CONFIG_CMU_EWF)			+= cmu_ewf.o

obj-$(CONFIG_SOC_EXYNOS9110)            += exynos9110/cal_data.o
obj-$(CONFIG_SOC_S5E5515)            += s5e5515/cal_data.o
