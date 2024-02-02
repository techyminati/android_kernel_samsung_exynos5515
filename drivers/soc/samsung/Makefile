# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_ARCH_EXYNOS9)       += exynos-el3_mon.o exynos-smc.o
obj-$(CONFIG_EXYNOS_PMU)	+= exynos-pmu.o

obj-$(CONFIG_EXYNOS_PMU_ARM_DRIVERS)	+= exynos3250-pmu.o exynos4-pmu.o \
					exynos5250-pmu.o exynos5420-pmu.o
obj-$(CONFIG_EXYNOS_PM_DOMAINS) += pm_domains.o
obj-$(CONFIG_EXYNOS_CHIPID)	+= exynos-chipid.o

# CAL
obj-$(CONFIG_CAL_IF)		+= cal-if/
obj-$(CONFIG_EXYNOS_CHIPID)	+= exynos-chipid.o
obj-$(CONFIG_EXYNOS_PD)        += exynos-pd.o exynos-pd-dbg.o
obj-$(CONFIG_EXYNOS_FLEXPMU_DBG)        += exynos-flexpmu-dbg.o

# ACPM
obj-$(CONFIG_EXYNOS_ACPM)       += acpm/

obj-$(CONFIG_ECT)		+= ect_parser.o
obj-$(CONFIG_EXYNOS_BCM)	+= exynos-bcm.o

#PM
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-powermode.o
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-pm.o
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-smc.o
obj-$(CONFIG_EXYNOS_PM_DVS)        += exynos-pm-dvs.o
obj-$(CONFIG_EXYNOS_RGT)        += exynos-rgt.o

# USI_V2
obj-$(CONFIG_USI_V2) += usi_v2.o
# CP
ifneq ($(CONFIG_CP_PMUCAL), y)
obj-$(CONFIG_EXYNOS_MODEM_IF) += pmu-cp.o
endif
#obj-$(CONFIG_SEC_MODEM_IF)  	+= pmu-cp.o
# CPIF
obj-$(CONFIG_EXYNOS_MODEM_IF) += cpif/
# GNSS interface driver
obj-$(CONFIG_EXYNOS_GNSS_IF) += gnssif/
#
# DVFS
obj-$(CONFIG_EXYNOS_DVFS_MANAGER)	+= exynos-dm.o

# DEBUG
obj-$(CONFIG_EXYNOS_DEBUG)      += debug/

# CPU HOTPLUG
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-cpu_hotplug.o

# CPU Power Management
obj-$(CONFIG_EXYNOS_CPUPM)     += exynos-cpupm.o

# Exynos Memlogger
obj-$(CONFIG_EXYNOS_MEMORY_LOGGER)      += memlogger.o

# Exynos Subsystem
obj-$(CONFIG_EXYNOS_SYSTEM_EVENT)       += sysevent.o sysevent_notif.o

# Exynos Image Loader
obj-$(CONFIG_EXYNOS_IMGLOADER)       += imgloader.o

# SDM
obj-$(CONFIG_EXYNOS_SDM)	+= exynos-sdm.o

# Exynos Secure Log
obj-$(CONFIG_EXYNOS_SECURE_LOG) += exynos-seclog.o

# Exynos TZASC fail detector
obj-$(CONFIG_EXYNOS_TZASC_FAIL_DETECTOR)	+= exynos-tzasc.o

# Exynos PPMU
 obj-$(CONFIG_EXYNOS_BCM_DBG)    += exynos-bcm_dbg.o exynos-bcm_dbg-dt.o
 obj-$(CONFIG_EXYNOS_BCM_DBG_DUMP)       += exynos-bcm_dbg-dump.o
