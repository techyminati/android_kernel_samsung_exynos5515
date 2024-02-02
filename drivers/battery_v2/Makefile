obj-$(CONFIG_SB_CORE)			+= core/
obj-$(CONFIG_SB_STEP)			+= step/

obj-$(CONFIG_SB_ADC)			+= sb_adc.o
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_battery.o sec_battery_vote.o sec_battery_thermal.o
obj-$(CONFIG_CALC_TIME_TO_FULL)	+= sec_battery_ttf.o
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sysfs_core.o sysfs_battery.o sysfs_device.o sysfs_event.o
obj-$(CONFIG_ENG_BATTERY_CONCEPT) += sysfs_eng.o
obj-$(CONFIG_BATTERY_CISD)	+= sec_cisd.o
obj-$(CONFIG_UPDATE_BATTERY_DATA)	+= sec_battery_data.o
obj-$(CONFIG_BATTERY_NOTIFIER)	+= battery_notifier.o
obj-$(CONFIG_JIG_NOTIFIER)	+= jig_notifier.o
obj-$(CONFIG_FUELGAUGE_S2MPW03)	+= s2mpw03_fuelgauge.o
obj-$(CONFIG_CHARGER_S2MPW03)   += s2mpw03_charger.o
obj-$(CONFIG_S2MPW03_RID_DETECT) += s2mpw03_muic.o

ifeq ($(CONFIG_WIRELESS_CHARGER_CPS4019), y)
	obj-$(CONFIG_WIRELESS_FIRMWARE_UPDATE)	+= cps4019_firmware.o
	obj-y									+= cps4019_charger.o
else
	obj-$(CONFIG_WIRELESS_CHARGER_P9222)	+= p9222_charger.o
endif

subdir-ccflags-y := -Wformat
