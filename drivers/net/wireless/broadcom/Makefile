
ifneq ($(filter wise% fresh%, $(TARGET_PRODUCT)), )
ifeq ($(SEC_BUILD_OPTION_HW_REVISION),01)
CONFIG_BCM43013=n
endif
endif

obj-$(CONFIG_B43)		+= b43/
obj-$(CONFIG_B43LEGACY)		+= b43legacy/

obj-$(CONFIG_BRCMFMAC)	+= brcm80211/
obj-$(CONFIG_BRCMSMAC)	+= brcm80211/
obj-$(CONFIG_BCM43013)  += bcmdhd_101_16/
obj-$(CONFIG_BCM43013)  += bcmdhd_101_16/dhd_custom_exynos.o
obj-$(CONFIG_BCM43013)  += bcmdhd_101_16/dhd_custom_memprealloc.o