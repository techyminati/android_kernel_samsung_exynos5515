/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/gpio.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/input.h>
#endif
#include <linux/sec_ext.h>
#include "./debug/sec_debug_internal.h"
#include "../battery_v2/include/sec_charging_common.h"
#include <linux/sec_batt.h>

#include <asm/cacheflush.h>
#include <asm/system_misc.h>

#include <linux/reset/exynos-reset.h>
#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/acpm_ipc_ctrl.h>
#include <linux/battery/sec_charging_common.h>
#include <linux/notifier.h>
#include <linux/string.h>
//#include <soc/samsung/exynos-sci.h>

#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif
#if IS_ENABLED(CONFIG_SEC_PARAM)
extern unsigned int lpcharge;
#endif

extern void hard_reset_delay(void); 

/* MULTICMD
 * reserve 8bit | dumpsink_sel 1bit | clk_change 1bit | dumpsink 2bit | param 1bit | dram_test 1bit | cp_debugmem 2bit | debuglevel 2bit | forceupload 2bit
 */
#define FORCEUPLOAD_ON                          (0x5)
#define FORCEUPLOAD_OFF                         (0x0)
#define DEBUGLEVEL_LOW                          (0x4f4c)
#define DEBUGLEVEL_MID                          (0x494d)
#define DEBUGLEVEL_HIGH                         (0x4948)
#define DUMPSINK_USB                            (0x0)
#define DUMPSINK_BOOTDEV                        (0x42544456)
#define DUMPSINK_SDCARD                         (0x73646364)
#define DUMPSINK_SELECT                         (0x65254478)
#define MULTICMD_CNT_MAX                        10
#define MULTICMD_LEN_MAX                        50
#define MULTICMD_FORCEUPLOAD_SHIFT              0
#define MULTICMD_FORCEUPLOAD_ON                 (0x1)
#define MULTICMD_FORCEUPLOAD_OFF                (0x2)
#define MULTICMD_DEBUGLEVEL_SHIFT               (MULTICMD_FORCEUPLOAD_SHIFT + 2)
#define MULTICMD_DEBUGLEVEL_LOW                 (0x1)
#define MULTICMD_DEBUGLEVEL_MID                 (0x2)
#define MULTICMD_DEBUGLEVEL_HIGH                (0x3)
#define MULTICMD_CPMEM_SHIFT                    (MULTICMD_DEBUGLEVEL_SHIFT + 2)
#define MULTICMD_CPMEM_ON                       (0x1)
#define MULTICMD_CPMEM_OFF                      (0x2)
#define MULTICMD_DRAMTEST_SHIFT                 (MULTICMD_CPMEM_SHIFT + 2)
#define MULTICMD_DRAMTEST_ON                    (0x1)
#define MULTICMD_PARAM_SHIFT                    (MULTICMD_DRAMTEST_SHIFT + 1)
#define MULTICMD_PARAM_ON                       (0x1)
#define MULTICMD_DUMPSINK_SHIFT                 (MULTICMD_PARAM_SHIFT + 1)
#define MULTICMD_DUMPSINK_USB                   (0x1)
#define MULTICMD_DUMPSINK_BOOT                  (0x2)
#define MULTICMD_DUMPSINK_SD                    (0x3)
#define MULTICMD_CLKCHANGE_SHIFT                (MULTICMD_DUMPSINK_SHIFT + 2)
#define MULTICMD_CLKCHANGE_ON                   (0x1)
#define MULTICMD_DUMPSINK_SEL_SHIFT             (MULTICMD_CLKCHANGE_SHIFT + 1)
#define MULTICMD_DUMPSINK_SEL                   (0x1)

extern void cache_flush_all(void);
extern void exynos_mach_restart(const char *cmd);
extern struct atomic_notifier_head panic_notifier_list;
extern struct exynos_reboot_helper_ops exynos_reboot_ops;
extern int exynos_reboot_pwrkey_status(void);

/* MINFORM */
#define SEC_REBOOT_START_OFFSET		(24)
#define SEC_REBOOT_END_OFFSET		(16)

enum sec_power_flags {
	SEC_REBOOT_DEFAULT = 0x30,
	SEC_REBOOT_NORMAL = 0x4E,
	SEC_REBOOT_LPM = 0x70,
};

#define SEC_DUMPSINK_MASK 0x0000FFFF

/* PANIC INFORM */
#define SEC_RESET_REASON_PREFIX         0x12345600
#define SEC_RESET_SET_PREFIX            0xabc00000
#define SEC_RESET_MULTICMD_PREFIX       0xa5600000
enum sec_reset_reason {
	SEC_RESET_REASON_UNKNOWN   = (SEC_RESET_REASON_PREFIX | 0x00),
	SEC_RESET_REASON_DOWNLOAD  = (SEC_RESET_REASON_PREFIX | 0x01),
	SEC_RESET_REASON_UPLOAD    = (SEC_RESET_REASON_PREFIX | 0x02),
	SEC_RESET_REASON_CHARGING  = (SEC_RESET_REASON_PREFIX | 0x03),
	SEC_RESET_REASON_RECOVERY  = (SEC_RESET_REASON_PREFIX | 0x04),
	SEC_RESET_REASON_FOTA      = (SEC_RESET_REASON_PREFIX | 0x05),
	SEC_RESET_REASON_FOTA_BL   = (SEC_RESET_REASON_PREFIX | 0x06), /* update bootloader */
	SEC_RESET_REASON_SECURE    = (SEC_RESET_REASON_PREFIX | 0x07), /* image secure check fail */
	SEC_RESET_REASON_FWUP      = (SEC_RESET_REASON_PREFIX | 0x09), /* emergency firmware update */
	SEC_RESET_REASON_EM_FUSE   = (SEC_RESET_REASON_PREFIX | 0x0a), /* EMC market fuse */
	SEC_RESET_REASON_FACTORY   = (SEC_RESET_REASON_PREFIX | 0x0c), /* go to factory mode */
	SEC_RESET_REASON_BOOTLOADER	= (SEC_RESET_REASON_PREFIX | 0x0d), /* go to download mode */
	SEC_RESET_REASON_WIRELESSD_BL	= (SEC_RESET_REASON_PREFIX | 0x0e), /* go to wireless download BOTA mode */
	SEC_RESET_REASON_RECOVERY_WD	= (SEC_RESET_REASON_PREFIX | 0x0f), /* go to wireless download mode */
	SEC_RESET_REASON_PKEY_HOLD	= (SEC_RESET_REASON_PREFIX | 0x12), /* Power Key HOLD during shutdown */
	SEC_RESET_REASON_EMERGENCY = 0x0,

	SEC_RESET_SET_DPRM         = (SEC_RESET_SET_PREFIX | 0x20000),
	SEC_RESET_SET_FORCE_UPLOAD = (SEC_RESET_SET_PREFIX | 0x40000),
	SEC_RESET_SET_DEBUG        = (SEC_RESET_SET_PREFIX | 0xd0000),
	SEC_RESET_SET_SWSEL        = (SEC_RESET_SET_PREFIX | 0xe0000),
	SEC_RESET_SET_SUD          = (SEC_RESET_SET_PREFIX | 0xf0000),
	SEC_RESET_CP_DBGMEM        = (SEC_RESET_SET_PREFIX | 0x50000), /* cpmem_on: CP RAM logging */
	SEC_RESET_SET_POWEROFF_WATCH   = (SEC_RESET_SET_PREFIX | 0x90000), /* Power off Watch mode */
#if defined(CONFIG_SEC_ABC)
	SEC_RESET_USER_DRAM_TEST   = (SEC_RESET_SET_PREFIX | 0x60000), /* USER DRAM TEST */
#endif
#if defined(CONFIG_SEC_SYSUP)
	SEC_RESET_SET_PARAM   = (SEC_RESET_SET_PREFIX | 0x70000),
#endif
	SEC_RESET_SET_DUMPSINK	   = (SEC_RESET_SET_PREFIX | 0x80000),
	SEC_RESET_SET_MULTICMD     = SEC_RESET_MULTICMD_PREFIX,
};

static int sec_reboot_on_panic;
static char panic_str[10] = "panic";

ATOMIC_NOTIFIER_HEAD(sec_power_off_notifier_list);
EXPORT_SYMBOL(sec_power_off_notifier_list);

static char * sec_strtok(char *s1, const char *delimit)
{
	static char *lastToken = NULL;
	char *tmp;

	if (s1 == NULL) {
		s1 = lastToken;

		if (s1 == NULL)
			return NULL;
	} else {
		s1 += strspn(s1, delimit);
	}

	tmp = strpbrk(s1, delimit);
	if (tmp) {
		*tmp = '\0';
		lastToken = tmp + 1;
	} else {
		lastToken = NULL;
	}

	return s1;
}

static void sec_multicmd(const char *cmd)
{
	unsigned long value = 0;
	char *multicmd_ptr;
	char *multicmd_cmd[MULTICMD_CNT_MAX];
	char copy_cmd[100] = {0,};
	unsigned long multicmd_value = 0;
	int i, cnt = 0;

	strcpy(copy_cmd, cmd);
	multicmd_ptr = sec_strtok(copy_cmd, ":");
	while (multicmd_ptr != NULL) {
		if (cnt >= MULTICMD_CNT_MAX)
			break;

		multicmd_cmd[cnt++] = multicmd_ptr;
		multicmd_ptr = sec_strtok(NULL, ":");
	}

	for (i = 1; i < cnt; i++) {
		if (strlen(multicmd_cmd[i]) < MULTICMD_LEN_MAX) {
			if (!strncmp(multicmd_cmd[i], "forceupload", 11) && !kstrtoul(multicmd_cmd[i] + 11, 0, &value)) {
				if (value == FORCEUPLOAD_ON)
					multicmd_value |= (MULTICMD_FORCEUPLOAD_ON << MULTICMD_FORCEUPLOAD_SHIFT);
				else if (value == FORCEUPLOAD_OFF)
					multicmd_value |= (MULTICMD_FORCEUPLOAD_OFF << MULTICMD_FORCEUPLOAD_SHIFT);
			}
			else if (!strncmp(multicmd_cmd[i], "debug", 5) && !kstrtoul(multicmd_cmd[i] + 5, 0, &value)) {
				if (value == DEBUGLEVEL_HIGH)
					multicmd_value |= (MULTICMD_DEBUGLEVEL_HIGH << MULTICMD_DEBUGLEVEL_SHIFT);
				else if (value == DEBUGLEVEL_MID)
					multicmd_value |= (MULTICMD_DEBUGLEVEL_MID << MULTICMD_DEBUGLEVEL_SHIFT);
				else if (value == DEBUGLEVEL_LOW)
					multicmd_value |= (MULTICMD_DEBUGLEVEL_LOW << MULTICMD_DEBUGLEVEL_SHIFT);
			}
			else if (!strncmp(multicmd_cmd[i], "cpmem_on", 8))
				multicmd_value |= (MULTICMD_CPMEM_ON << MULTICMD_CPMEM_SHIFT);
			else if (!strncmp(multicmd_cmd[i], "cpmem_off", 9))
				multicmd_value |= (MULTICMD_CPMEM_OFF << MULTICMD_CPMEM_SHIFT);
#if defined(CONFIG_SEC_ABC)
			else if (!strncmp(multicmd_cmd[i], "user_dram_test", 14) && sec_abc_get_enabled())
				multicmd_value |= (MULTICMD_DRAMTEST_ON << MULTICMD_DRAMTEST_SHIFT);
#endif
#if defined(CONFIG_SEC_SYSUP)
			else if (!strncmp(multicmd_cmd[i], "param", 5))
				multicmd_value |= (MULTICMD_PARAM_ON << MULTICMD_PARAM_SHIFT);
#endif
			else if (!strncmp(multicmd_cmd[i], "dump_sink", 9) && !kstrtoul(multicmd_cmd[i] + 9, 0, &value)) {
				if (value == DUMPSINK_USB)
					multicmd_value |= (MULTICMD_DUMPSINK_USB << MULTICMD_DUMPSINK_SHIFT);
				else if (value == DUMPSINK_BOOTDEV)
					multicmd_value |= (MULTICMD_DUMPSINK_BOOT << MULTICMD_DUMPSINK_SHIFT);
				else if (value == DUMPSINK_SDCARD)
					multicmd_value |= (MULTICMD_DUMPSINK_SD << MULTICMD_DUMPSINK_SHIFT);
				else if (value == DUMPSINK_SELECT)
					multicmd_value |= (MULTICMD_DUMPSINK_SEL << MULTICMD_DUMPSINK_SEL_SHIFT);
			}
#if defined(CONFIG_ARM_EXYNOS_ACME_DISABLE_BOOT_LOCK) && defined(CONFIG_ARM_EXYNOS_DEVFREQ_DISABLE_BOOT_LOCK)
			else if (!strncmp(multicmd_cmd[i], "clkchange_test", 14))
				multicmd_value |= (MULTICMD_CLKCHANGE_ON << MULTICMD_CLKCHANGE_SHIFT);
#endif
		}
	}
	pr_emerg("%s: multicmd_value: %lu\n", __func__, multicmd_value);
	exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_MULTICMD | multicmd_value);
}

void sec_set_reboot_magic(int magic, int offset, int mask)
{
	u32 tmp = 0;

	exynos_pmu_read(SEC_DEBUG_MAGIC_INFORM, &tmp);
	pr_info("%s: prev: %x\n", __func__, tmp);
	mask <<= offset;
	tmp &= (~mask);
	tmp |= magic << offset;
	pr_info("%s: set as: %x\n", __func__, tmp);
	exynos_pmu_write(SEC_DEBUG_MAGIC_INFORM, tmp);
}
EXPORT_SYMBOL(sec_set_reboot_magic);

static void sec_power_off(void)
{
	u32 poweroff_try = 0;
	union power_supply_propval ac_val = {0, };
	union power_supply_propval usb_val = {0, };
	union power_supply_propval wpc_val = {0, };
	u32 reboot_charging = 0;

	sec_set_reboot_magic(SEC_REBOOT_LPM, SEC_REBOOT_END_OFFSET, 0xFF);
	psy_do_property("ac", get, POWER_SUPPLY_PROP_ONLINE, ac_val);
	psy_do_property("usb", get, POWER_SUPPLY_PROP_ONLINE, usb_val);
	psy_do_property("wireless", get, POWER_SUPPLY_PROP_ONLINE, wpc_val);
	reboot_charging = ac_val.intval || usb_val.intval || wpc_val.intval;
	pr_info("[%s] reboot_charging(%d), AC[%d], USB[%d], WPC[%d]\n",
			__func__, reboot_charging, ac_val.intval, usb_val.intval, wpc_val.intval);

	pr_info("Exynos reboot, PWR Key(%d)\n", exynos_reboot_pwrkey_status());

	flush_cache_all();

	/* before power off */
	pr_crit("%s: call pre-power_off notifiers\n", __func__);
	atomic_notifier_call_chain(&sec_power_off_notifier_list, 0, NULL);

	while (1) {
#ifdef CONFIG_SEC_FACTORY
		/* for WATCH4/5 reboot issue in leakage current test */
		if (poweroff_try >= 5) {
			sec_set_reboot_magic(SEC_REBOOT_NORMAL, SEC_REBOOT_END_OFFSET, 0xFF);
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_PKEY_HOLD);
			pr_emerg("%s: POWER KEY HOLD, reboot in factory binary\n", __func__);

			exynos_mach_restart("sw reset");

			pr_emerg("%s: waiting for reboot\n", __func__);
			while (1);
		}
#else
		/* Check reboot charging */
#if IS_ENABLED(CONFIG_SEC_PARAM)
		if ((reboot_charging || (poweroff_try >= 5)) && !lpcharge) {
#else
		if (reboot_charging || (poweroff_try >= 5)) {
#endif
			/* if reboot_charging is true, to enter LP charging.
			 * else Power Key HOLD
			 */
			if (reboot_charging) {
				exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UNKNOWN);
				pr_emerg("%s: charger connected or power off failed(%d), reboot!\n", __func__, poweroff_try);
			} else {
				sec_set_reboot_magic(SEC_REBOOT_NORMAL, SEC_REBOOT_END_OFFSET, 0xFF);
				exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_PKEY_HOLD);
				pr_emerg("%s: POWER KEY HOLD, reboot!\n", __func__);
			}

			exynos_mach_restart("sw reset");

			pr_emerg("%s: waiting for reboot\n", __func__);
			while (1);
		}
#endif
		/* wait for power button release.
		 * but after exynos_acpm_reboot is called
		 * power on status cannot be read */
		if (exynos_reboot_pwrkey_status()) 
			pr_info("PWR Key is not released (%d)(poweroff_try:%d)\n", exynos_reboot_pwrkey_status(), poweroff_try);
		else {
			if (exynos_reboot_ops.acpm_reboot)
				exynos_reboot_ops.acpm_reboot();
			else
				pr_err("Exynos reboot, acpm_reboot not registered\n");
			pr_emerg("Set PS_HOLD Low.\n");
			exynos_pmu_update(EXYNOS_PMU_PS_HOLD_CONTROL, 0x1<<8, 0x0);
			pr_emerg("Should not reach here! Device will be restarted after 950 msec.\n");
			mdelay(950);
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UNKNOWN);;
			exynos_mach_restart("sw reset");
			pr_emerg("%s: waiting for reboot\n", __func__);
			while (1);
		}
		++poweroff_try;
		mdelay(1000);
	}

}

static int sec_reboot(struct notifier_block *this,
				unsigned long mode, void *cmd)
{
	local_irq_disable();

	hard_reset_delay();

	if (sec_reboot_on_panic && !cmd)
		cmd = panic_str;

	pr_emerg("%s (%d, %s)\n", __func__, reboot_mode, cmd ? cmd : "(null)");

	/* LPM mode prevention */
	sec_set_reboot_magic(SEC_REBOOT_NORMAL, SEC_REBOOT_END_OFFSET, 0xFF);

	if (cmd) {
		unsigned long value = 0;
		if (!strcmp(cmd, "fota"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_FOTA);
		else if (!strcmp(cmd, "fota_bl"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_FOTA_BL);
		else if (!strcmp(cmd, "recovery"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_RECOVERY);
		else if (!strcmp(cmd, "download"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_DOWNLOAD);
		else if (!strcmp(cmd, "bootloader"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_BOOTLOADER);
		else if (!strcmp(cmd, "upload"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UPLOAD);
		else if (!strcmp(cmd, "secure"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_SECURE);
		else if (!strcmp(cmd, "wdownload"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_RECOVERY_WD);
		else if (!strcmp(cmd, "wirelessd"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_WIRELESSD_BL);
		else if (!strcmp(cmd, "factory"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_FACTORY);
		else if (!strcmp(cmd, "fwup"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_FWUP);
		else if (!strcmp(cmd, "em_mode_force_user"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_EM_FUSE);
#if defined(CONFIG_SEC_ABC)
		else if (!strcmp(cmd, "user_dram_test") && sec_abc_get_enabled())
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_USER_DRAM_TEST);
#endif
		else if (!strncmp(cmd, "emergency", 9))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_EMERGENCY);
		else if (!strncmp(cmd, "debug", 5) && !kstrtoul(cmd + 5, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_DEBUG | value);
		else if (!strncmp(cmd, "dump_sink", 9) && !kstrtoul(cmd + 9, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_DUMPSINK | (SEC_DUMPSINK_MASK & value));
		else if (!strncmp(cmd, "forceupload", 11) && !kstrtoul(cmd + 11, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_FORCE_UPLOAD | value);
		else if (!strncmp(cmd, "dprm", 4))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_DPRM);
		else if (!strncmp(cmd, "swsel", 5) && !kstrtoul(cmd + 5, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_SWSEL | value);
		else if (!strncmp(cmd, "sud", 3) && !kstrtoul(cmd + 3, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_SUD | value);
		else if (!strncmp(cmd, "multicmd:", 9))
			sec_multicmd(cmd);
#if defined(CONFIG_SEC_SYSUP)
		else if (!strncmp(cmd, "param", 5))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_PARAM);
#endif
		else if (!strncmp(cmd, "cpmem_on", 8))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x1);
		else if (!strncmp(cmd, "cpmem_off", 9))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x2);
		else if (!strncmp(cmd, "mbsmem_on", 9))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x1);
		else if (!strncmp(cmd, "mbsmem_off", 10))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x2);
		else if (!strncmp(cmd, "watchonly", 9)){
			int wcoff = 10;

			if (!strncmp(cmd + wcoff, "exercise", 8))
				wcoff += 10;
			else
				wcoff += 1;

			if (((char*)cmd)[wcoff] == '0')
				kstrtoul(cmd + (wcoff + 1), 0, &value);
			else
				kstrtoul(cmd + wcoff, 0, &value);

			if (((char*)cmd)[wcoff - 1] == '+')
				value |= 1 << 0xf;

			if (((char*)cmd)[wcoff - 2] == '1')
				value |= 1 << 0xe;

			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_POWEROFF_WATCH | value);
		} else if (!strncmp(cmd, "panic", 5)) {
			/*
			 * This line is intentionally blanked because the PANIC INFORM is used for upload cause
			 * in sec_debug_set_upload_cause() only in case of  panic() .
			 */
		} else
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UNKNOWN);
	} else {
		exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UNKNOWN);
	}

	flush_cache_all();

	return NOTIFY_DONE;
}

static int sec_reboot_panic_handler(struct notifier_block *nb,
				unsigned long l, void *buf)
{
	pr_emerg("sec_reboot: %s\n", __func__);

	sec_reboot_on_panic = 1;

	return NOTIFY_DONE;
}

static struct notifier_block nb_panic_block = {
	.notifier_call = sec_reboot_panic_handler,
	.priority = 128,
};

static struct notifier_block sec_restart_nb = {
	.notifier_call = sec_reboot,
	.priority = 130,
};

static int __init sec_reboot_init(void)
{
	int err;

	err = atomic_notifier_chain_register(&panic_notifier_list, &nb_panic_block);
	if (err) {
		pr_err("cannot register panic handler (err=%d)\n", err);
	}

	err = register_restart_handler(&sec_restart_nb);
	if (err) {
		pr_err("cannot register restart handler (err=%d)\n", err);
	}
	pm_power_off = sec_power_off;

	pr_info("register restart handler successfully\n");

	return err;
}

subsys_initcall(sec_reboot_init);
