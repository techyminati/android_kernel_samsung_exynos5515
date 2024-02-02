#
# Makefile for the sensor drivers.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_SENSORS_SSP)		+= ssp_dev.o ssp_data.o ssp_sysfs.o ssp_iio.o ssp_scontext.o\
						ssp_firmware.o ssp_debug.o ssp_dump.o
obj-$(CONFIG_SENSORS_SYSFS)		+= sensors_core.o
obj-$(CONFIG_SENSORS_SSP_INPUT_DEV)	+= ssp_input.o

obj-$(CONFIG_SENSORS_SSP_FACTORY)	+= factory/ssp_factory.o factory/ factory/mcu_chub.o
obj-$(CONFIG_SENSORS_SSP_CHUB)	+= ssp_comm.o ssp_platform.o
obj-$(CONFIG_SENSORS_SSP_LPM_MOTION)	+= ssp_lpm.o
obj-$(CONFIG_SENSORS_SSP_VIB_NOTIFY)	+= ssp_motor.o