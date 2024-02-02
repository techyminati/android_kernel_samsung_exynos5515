#
# Makefile for nanohub
#

obj-$(CONFIG_NANOHUB) += nanohub.o
nanohub-$(CONFIG_SENSOR_DRV) := main.o comms.o
nanohub-$(CONFIG_CONTEXTHUB_DRV) += chub.o chub_bootup.o chub_dbg.o chub_log.o chub_exynos.o
nanohub-$(CONFIG_NANOHUB_MAILBOX) += chub_ipc_if.o ipc_chub.o ipc_chub_ap.o ipc_common.o ipc_hw.o
