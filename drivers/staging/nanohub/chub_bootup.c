// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CHUB IF Driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 * Authors:
 *      Boojin Kim <boojin.kim@samsung.com>
 *      Sukwon Ryu <sw.ryoo@samsung.com>
 *
 */

#include <soc/samsung/cal-if.h>
#include <linux/smc.h>
#ifdef CONFIG_EXYNOS_PD_EL3
#include <soc/samsung/exynos-el3_mon.h>
#endif
#ifdef CONFIG_EXYNOS_S2MPU
#include <soc/samsung/exynos-s2mpu.h>
#endif
#include <linux/firmware.h>
#include <linux/slab.h>
#include "chub.h"
#include "chub_exynos.h"
#include <soc/samsung/debug-snapshot.h>

#ifdef CONFIG_SENSORS_SSP
#include "../../sensorhub/slsi/ssp_platform.h"
enum fw_type {
	FW_TYPE_NONE,
	FW_TYPE_BIN,
	FW_TYPE_PUSH,
	FW_TYPE_SPU,
};
#endif

#undef ALIVE_WORK

#if defined(CONFIG_EXYNOS_IMGLOADER) && !defined(CONFIG_SENSORS_SSP_HRM_ADI)
#undef CONFIG_EXYNOS_IMGLOADER
#define CONFIG_EXYNOS_IMGLOADER_UNDEF
#endif

const char *os_image[SENSOR_VARIATION] = {
	"os.checked_0.bin",
	"os.checked_1.bin",
	"os.checked_2.bin",
	"os.checked_3.bin",
	"os.checked_4.bin",
	"os.checked_5.bin",
	"os.checked_6.bin",
	"os.checked_7.bin",
	"os.checked_8.bin",
};

static DEFINE_MUTEX(reset_mutex);
static DEFINE_MUTEX(pmu_shutdown_mutex);
static DEFINE_MUTEX(chub_err_mutex);

#define CHUB_BOOTUP_TIME_MS (3000)

extern int dbg_snapshot_expire_watchdog(void);

static void contexthub_reset_token(struct contexthub_ipc_info *chub)
{
	atomic_set(&chub->in_use_ipc, 0);
}

static inline void contexthub_set_in_reset(struct contexthub_ipc_info *chub,
					   bool i)
{
	atomic_set(&chub->in_reset, i);
}

static ssize_t chub_reset(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count);

#ifdef CONFIG_CONTEXTHUB_SENSOR_DEBUG
#define SENSORTASK_KICK_MS (5000)

void sensor_alive_check_func(struct work_struct *work)
{
	int ret;
	struct contexthub_ipc_info *ipc = container_of(to_delayed_work(work),
						       struct contexthub_ipc_info,
						       sensor_alive_work);

	nanohub_info("%s\n", __func__);
	ret = ipc_sensor_alive_check();
	if (ret < 0) {
		nanohub_info("%s: contexthub no reponce -> reset\n", __func__);
		contexthub_reset(ipc, true, 0xff);
	}
	schedule_delayed_work(&ipc->sensor_alive_work, msecs_to_jiffies(SENSORTASK_KICK_MS));
}
#endif
static int contexthub_alive_check(struct contexthub_ipc_info *chub)
{
	int trycnt = 0;
	int ret = 0;

	do {
		msleep(WAIT_CHUB_MS);
		contexthub_ipc_write_event(chub, MAILBOX_EVT_CHUB_ALIVE);
		if (++trycnt > WAIT_TRY_CNT)
			break;
	} while ((atomic_read(&chub->chub_status) != CHUB_ST_RUN));

	if (atomic_read(&chub->chub_status) == CHUB_ST_RUN) {
		nanohub_dev_info(chub->dev,
				 "%s done. contexthub status is %d\n", __func__,
				 atomic_read(&chub->chub_status));
	} else {
		nanohub_dev_warn(chub->dev,
				 "%s failed. contexthub status is %d\n",
				 __func__, atomic_read(&chub->chub_status));

		if (!atomic_read(&chub->in_reset)) {
			atomic_set(&chub->chub_status, CHUB_ST_NO_RESPONSE);
			contexthub_handle_debug(chub,
						CHUB_ERR_CHUB_NO_RESPONSE);
		} else {
			nanohub_dev_info(chub->dev,
					 "%s: skip to handle debug in reset\n",
					 __func__);
		}
		ret = -ETIMEDOUT;
	}
	return ret;
}

/* contexhub slient reset support */
void contexthub_handle_debug(struct contexthub_ipc_info *chub,
			     enum chub_err_type err)
{
	int thold = (err < CHUB_ERR_CRITICAL) ? 1 :
	    ((err <
	      CHUB_ERR_MAJER) ? CHUB_RESET_THOLD : CHUB_RESET_THOLD_MINOR);

	/* update error count */
	chub->err_cnt[err]++;

	nanohub_dev_info(chub->dev,
			 "%s: err:%d(cnt:%d), cur_err:%d, thold:%d, chub_status:%d\n",
			 __func__, err, chub->err_cnt[err], chub->cur_err,
			 thold, atomic_read(&chub->chub_status));

	/* set chub_status to CHUB_ST_ERR. request silent reset */
	if ((chub->err_cnt[err] >= thold) && (atomic_read(&chub->chub_status) != CHUB_ST_ERR)) {
		atomic_set(&chub->chub_status, CHUB_ST_ERR);
		nanohub_dev_info(chub->dev,
				 "%s: err:%d(cnt:%d), enter error status\n",
				 __func__, err, chub->err_cnt[err]);
		/* handle err */
		chub->cur_err = err;
		schedule_work(&chub->debug_work);
	}
}

static int contexthub_verify_symtable(struct contexthub_ipc_info *chub,
				      void *data, size_t size)
{
	u32 magic, symbol_size, offset = 0;

	if (!chub || (chub->os_type == OS_TYPE_MULTIOS && !chub->sel_os) || chub->symbol_table)
		return -EINVAL;

	magic = *(u32 *)(data + size - 4);
	if (magic != 0x626d7973) {
		offset = 528;
		magic = *(u32 *)(data + size - offset - 4);
	}

	nanohub_dev_info(chub->dev, "read magic : %x\n", magic);
	if (magic == 0x626d7973) {
		symbol_size = *(u32 *)(data + size - offset - 8);
		nanohub_dev_info(chub->dev, "symbol table size : %d\n",
				 symbol_size);

		if (symbol_size < size - offset) {
			chub->symbol_table = kmalloc(symbol_size, GFP_KERNEL);
			if (chub->symbol_table) {
				memcpy(chub->symbol_table,
				       data + size - symbol_size - offset - 8,
				       symbol_size);
				nanohub_dev_info(chub->dev,
						 "Load symbol table Done!!!\n");
			}
		}
	} else {
		chub->symbol_table = NULL;
	}

	return 0;
}

static int contexthub_download_image(struct contexthub_ipc_info *chub,
				     enum ipc_region reg)
{
	const struct firmware *entry;
	int ret = 0, firm = 0;
	void *chub_addr = NULL;

	if (chub->os_type == OS_TYPE_ONEBINARY) {
#ifdef CONFIG_SENSORS_SSP_HRM_ADI
		nanohub_dev_info(chub->dev, "%s: download one binary for DUAL bin\n", __func__);
		ret = request_firmware(&entry, OS_IMAGE_ONEBINARY_PUSH, chub->dev);
#else
		nanohub_dev_info(chub->dev, "%s: download one binary\n", __func__);
		ret = request_firmware(&entry, chub->os_name, chub->dev);
#endif
		chub_addr = chub->sram;
	} else {
		if (reg == IPC_REG_BL) {
			nanohub_dev_info(chub->dev, "%s: download bl\n", __func__);
			ret = request_firmware(&entry, "bl.unchecked.bin", chub->dev);
			chub_addr = chub->sram;
		} else if (reg == IPC_REG_OS) {
			nanohub_dev_info(chub->dev, "%s: download %s\n", __func__,
					 chub->os_name);
			ret = request_firmware(&entry, chub->os_name, chub->dev);
			chub_addr = ipc_get_base(reg);
		} else {
			nanohub_dev_err(chub->dev, "%s: invalid reg:%d\n", __func__,
					reg);
			return -EINVAL;
		}
	}
	
	firm = ssp_check_hrm_int(chub->ssp_data, chub->dev);

	if (ret) {
#ifdef CONFIG_SENSORS_SSP_HRM_ADI	
		if (firm == 1) { // ADI 
			strncpy(chub->os_name_new, OS_IMAGE_ONEBINARY_ADI, sizeof(OS_IMAGE_ONEBINARY_ADI));
			chub->chub_img_desc[0].fw_name = chub->os_name_new;
		} else if (0) { // SPU
			// TO DO
			ssp_set_firmware_type(chub->ssp_data, FW_TYPE_SPU);
		} else { // default
			chub->chub_img_desc[0].fw_name = chub->os_name;
			ssp_set_firmware_type(chub->ssp_data, FW_TYPE_BIN);
		}
		ret = imgloader_boot(&chub->chub_img_desc[0]);
		if (!ret) {  // success
			nanohub_dev_info(chub->dev, "%s: imgloader_boot success\n", __func__);
			return 0;	
		}
		
		nanohub_dev_info(chub->dev, "%s: imgloader_boot fail (%s)\n", chub->chub_img_desc[0].fw_name, __func__);
#endif

#ifdef CONFIG_SENSORS_SSP
		ret = ssp_download_firmware(chub->ssp_data, chub->dev, chub_addr);
		if (ret == 1) {
			nanohub_dev_info(chub->dev, "%s: spu firmware\n", __func__);
			ssp_set_firmware_type(chub->ssp_data, FW_TYPE_SPU);
		} else if (ret == 2) {
			nanohub_dev_info(chub->dev, "%s: default binary (0x%x:0x%x)\n", __func__, ipc_get_base(IPC_REG_BL), chub->sram);
			ssp_set_firmware_type(chub->ssp_data, FW_TYPE_BIN);
		}
		return 0;
#else
		nanohub_dev_err(chub->dev,
				"%s, bl(%d) request_firmware failed\n",
				reg == IPC_REG_BL, __func__);
		release_firmware(entry);
		return ret;
#endif
	}
#ifdef CONFIG_SENSORS_SSP
	ssp_set_firmware_type(chub->ssp_data, FW_TYPE_PUSH);
#endif

	memcpy_toio(chub_addr, entry->data, entry->size);
	nanohub_dev_info(chub->dev, "%s: bl:%d, bin(size:%d) on %lx\n",
			 __func__, reg == IPC_REG_BL, (int)entry->size,
			 (unsigned long)ipc_get_base(reg));
#ifdef CONFIG_EXYNOS_S2MPU
	if (reg == IPC_REG_BL)
		ret = exynos_verify_subsystem_fw("CHUB", 0,
						 chub->sram_phys, entry->size, SZ_4K);
	else if (reg == IPC_REG_OS)
		ret = exynos_verify_subsystem_fw("CHUB", 1,
						 chub->sram_phys + SZ_4K,
						 entry->size, SZ_1M - SZ_4K);
	else
		nanohub_dev_err(chub->dev, "%s: invalid reg:%d\n", __func__,
				reg);

	if (ret) {
		nanohub_dev_err(chub->dev, "%s: verify fail!:%d\n", __func__,
				ret);
		return -1;
	}
#endif
	if (chub->os_type == OS_TYPE_ONEBINARY || reg == IPC_REG_OS) {
		contexthub_verify_symtable(chub, (void *)entry->data, entry->size);
	}

	release_firmware(entry);

	return 0;
}

static int contexthub_download_and_check_image(struct contexthub_ipc_info *chub,
					       enum ipc_region reg)
{
	u32 *fw = vmalloc(ipc_get_size(reg));
	int ret = 0;

	if (!fw)
		return contexthub_download_image(chub, reg);

	memcpy_fromio(fw, ipc_get_base(reg), ipc_get_size(reg));
	ret = contexthub_download_image(chub, reg);
	if (ret) {
		nanohub_dev_err(chub->dev, "%s: download bl(%d) fails\n",
				__func__, reg == IPC_REG_BL);
		goto out;
	}

	ret = memcmp(fw, ipc_get_base(reg), ipc_get_size(reg));
	if (ret) {
		int i;
		u32 *fw_image = (u32 *)ipc_get_base(reg);

		nanohub_dev_err(chub->dev,
				"%s: fw(%lx) doens't match with size %d\n",
				__func__, (unsigned long)ipc_get_base(reg),
				ipc_get_size(reg));
		for (i = 0; i < ipc_get_size(reg) / 4; i++)
			if (fw[i] != fw_image[i]) {
				nanohub_dev_err(chub->dev,
						"fw[%d] %x -> wrong %x\n", i,
						fw_image[i], fw[i]);
				print_hex_dump(KERN_CONT, "before:",
					       DUMP_PREFIX_OFFSET, 16, 1,
					       &fw[i], 64, false);
				print_hex_dump(KERN_CONT, "after:",
					       DUMP_PREFIX_OFFSET, 16, 1,
					       &fw_image[i], 64, false);
				ret = -EINVAL;
				break;
			}
	}
out:
	nanohub_dev_info(chub->dev, "%s: download and checked bl(%d) ret:%d\n",
			 __func__, reg == IPC_REG_BL, ret);
	vfree(fw);
	return ret;
}

/* FIXME: test purpose */
int contexthub_download_cm55_image(struct contexthub_ipc_info *chub)
{
	int ret;
	const struct firmware *entry;

	nanohub_dev_info(chub->dev, "%s: download CM55\n", __func__);
	ret = request_firmware(&entry, "lpd.core.bin", chub->dev);
	if (ret) {
		nanohub_dev_info(chub->dev, "%s: download cm55 fail\n", __func__);
		release_firmware(entry);
		return ret;
	}

	memcpy_toio(chub->sram + SRAM_CM55_OFFSET, entry->data, entry->size);
	nanohub_dev_info(chub->dev, "%s: bin(size:%d) on 0x%lx\n",
			__func__, (int)entry->size, SRAM_CM55_OFFSET);
	release_firmware(entry);

	return ret;
}

static void contexthub_set_baaw(struct contexthub_ipc_info *chub)
{
	int ret, i;

	/* tzpc setting */
	ret = exynos_smc(SMC_CMD_CONN_IF,
			 (EXYNOS_CHUB << 32) | EXYNOS_SET_CONN_TZPC, 0, 0);
	if (ret) {
		nanohub_err("%s: TZPC setting fail\n", __func__);
		return;
	}

	/* BAAW-C-CHUB for CHUB to access MAILBOX, CMGP, etc. 1 window is used */
	if (!IS_ERR_OR_NULL(chub->baaw_c_chub)) {
		nanohub_info("%s BAAW_C_CHUB start 0x%lx end 0x%lx remap 0x%lx\n", __func__,
			     chub->baaw_info.baaw_c_chub_start,
			     chub->baaw_info.baaw_c_chub_end,
			     chub->baaw_info.baaw_c_chub_remap);
		IPC_HW_WRITE_BAAW_CHUB0(chub->baaw_c_chub,
					chub->baaw_info.baaw_c_chub_start);
		IPC_HW_WRITE_BAAW_CHUB1(chub->baaw_c_chub,
					chub->baaw_info.baaw_c_chub_end);
		IPC_HW_WRITE_BAAW_CHUB2(chub->baaw_c_chub,
					chub->baaw_info.baaw_c_chub_remap);
		IPC_HW_WRITE_BAAW_CHUB3(chub->baaw_c_chub, BAAW_RW_ACCESS_ENABLE);
	}
	/* BAAW-D-CHUB for CHUB to access SFRs. 1 window is used */
	if (!IS_ERR_OR_NULL(chub->baaw_d_chub)) {
		for (i = 0 ; i < sizeof(chub->baaw_info.baaw_d_chub_values)/sizeof(unsigned int) ; i++) {
			if (!chub->baaw_info.baaw_d_chub_values[i])
				break;
			nanohub_info("%s: BAAW_D_CHUB offset +%x %x\n", __func__,
				     i * 4, chub->baaw_info.baaw_d_chub_values[i]);
			__raw_writel(chub->baaw_info.baaw_d_chub_values[i], chub->baaw_d_chub + i * 4);
		}
	}
}

static int contexthub_reset_prepare(struct contexthub_ipc_info *chub)
{
	int ret;
	/* pmu call reset-release_config */
	ret = cal_chub_reset_release_config();
	if (ret) {
		nanohub_err("%s: reset release cfg fail\n", __func__);
		return ret;
	}

	/* baaw config */
	nanohub_dev_info(chub->dev, "%s: tzpc and baaw set\n", __func__);
	contexthub_set_baaw(chub);

	contexthub_chub_ipc_init(chub);

	return ret;
}

int contexthub_shutdown(struct contexthub_ipc_info *chub)
{
	int ret;
	int status = atomic_read(&chub->chub_status);

	if (atomic_read(&chub->chub_shutdown)) {
		nanohub_dev_info(chub->dev, "%s: chub already shutdown!\n", __func__);
		return 0;
	}
	/* mutex in ISR(itmon) is not allowed */
	if (status != CHUB_ST_ITMON)
		mutex_lock(&pmu_shutdown_mutex);
	/* pmu call assert */
	ret = cal_chub_reset_assert();
	if (ret) {
		nanohub_err("%s: reset assert fail\n", __func__);
		if (status != CHUB_ST_ITMON)
			mutex_unlock(&pmu_shutdown_mutex);
		return ret;
	}

	/* pmu call reset-release_config */
	ret = contexthub_reset_prepare(chub);
	if (ret) {
		nanohub_err("%s: reset prepare fail\n", __func__);
		if (status != CHUB_ST_ITMON)
			mutex_unlock(&pmu_shutdown_mutex);
		return ret;
	}
	atomic_set(&chub->chub_shutdown, 1);
	if (status != CHUB_ST_ITMON)
		mutex_unlock(&pmu_shutdown_mutex);

	return ret;
}

void __iomem *clk_con_div_clkcmu_chub_bus;
#define CHUB_RESET_WAIT_TIME_MS (300)
int contexthub_reset(struct contexthub_ipc_info *chub, bool force_load,
		     enum chub_err_type err)
{
	int ret = 0;
	int trycnt = 0;
	bool irq_disabled;
#ifdef CONFIG_SENSORS_SSP
	bool skip_panic = false;
	if (err == CHUB_ERR_SSP_RECOVERY) {
		skip_panic = true;
		err = CHUB_ERR_NONE;
	}
#endif
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
	if (chub->sysevent_dev) {
		if (chub->sysevent_status == NULL || chub->sysevent_status == PTR_ERR) {
			nanohub_dev_info(chub->dev,
				"sysevent_get doesn't called. skip this\n");
		} else {
			sysevent_put((void *)chub->sysevent_dev);
		}
	}
#if defined(CONFIG_EXYNOS_LPD)
	sysevent_put((void *)chub->lpd_sysevent_dev);
#endif
#endif
	mutex_lock(&reset_mutex);
	nanohub_dev_info(chub->dev,
			 "%s: force:%d, status:%d, in-reset:%d, err:%d, user:%d\n",
			 __func__, force_load, atomic_read(&chub->chub_status),
			 contexthub_read_in_reset(chub), err,
			 contexthub_read_token(chub));

	if (clk_con_div_clkcmu_chub_bus != NULL) {
		__raw_writel(0x2, clk_con_div_clkcmu_chub_bus);
		msleep(5);
		nanohub_dev_info(chub->dev, "%s: clk div: %x\n", __func__,
			__raw_readl(clk_con_div_clkcmu_chub_bus));
	}

	chub_dbg_dump_hw(chub, err);

	if (!force_load && (atomic_read(&chub->chub_status) == CHUB_ST_RUN)) {
		mutex_unlock(&reset_mutex);
		nanohub_dev_info(chub->dev, "%s: out status:%d\n", __func__,
				 atomic_read(&chub->chub_status));
		return 0;
	}

	/* disable mailbox interrupt to prevent sram access during chub reset */
	disable_irq(chub->irq_mailbox);
	if(chub->irq_wdt_disabled == 0)
		disable_irq(chub->irq_wdt);
	irq_disabled = true;

	atomic_inc(&chub->in_reset);
	chub_wake_lock(chub->ws_reset);
	contexthub_notifier_call(chub, CHUB_FW_ST_OFF);

	/* wait for ipc free */
	while (atomic_read(&chub->in_use_ipc)) {
		chub_wait_event(&chub->reset_lock, CHUB_RESET_WAIT_TIME_MS);
		if (++trycnt > RESET_WAIT_TRY_CNT) {
			nanohub_dev_info(chub->dev,
					 "%s: can't get lock. in_use_ipc: %d\n",
					 __func__, contexthub_read_token(chub));
			ret = -EINVAL;
			goto out;
		}
		nanohub_dev_info(chub->dev, "%s: wait for ipc user free: %d\n",
				 __func__, contexthub_read_token(chub));
	}
	if (!trycnt)
		msleep(CHUB_RESET_WAIT_TIME_MS);	/* wait for subsystem release */

	contexthub_set_in_reset(chub, true);

	nanohub_dev_info(chub->dev, "%s: start reset status:%d\n", __func__,
			 atomic_read(&chub->chub_status));

#if defined(CONFIG_SENSORS_SSP_DEBUG) && !defined(CONFIG_SEC_FACTORY)
	if (!skip_panic)
		dbg_snapshot_expire_watchdog();
		//panic("CHUB RESET!!");
#endif

	/* shutdown */
	ret = contexthub_shutdown(chub);

	if (ret) {
		nanohub_dev_err(chub->dev, "%s: shutdown fails, ret:%d\n",
				__func__, ret);
		goto out;
	}

	/* image download */
	if (force_load) {
#if defined(CONFIG_EXYNOS_IMGLOADER) && !defined(CONFIG_SENSORS_SSP_HRM_ADI)
		ssp_check_hrm_int(chub->ssp_data, chub->dev);
		ret = imgloader_boot(&chub->chub_img_desc[0]);
#else
		ret = contexthub_download_image(chub, IPC_REG_BL);
#endif
		if (ret) {
			nanohub_dev_err(chub->dev, "%s: download bl fails\n",
					__func__);
			ret = -EINVAL;
			goto out;
		}
		if (chub->os_type != OS_TYPE_ONEBINARY) {
			if (force_load) { /* can use new binary */
#ifdef CONFIG_EXYNOS_IMGLOADER
				ret = imgloader_boot(&chub->chub_img_desc[chub->num_os + 1]);
#else
				ret = contexthub_download_image(chub, IPC_REG_OS);
#endif
			} else { /* use previous binary */
				ret = contexthub_download_and_check_image(chub, IPC_REG_OS);
			}
		}
		if (ret) {
			nanohub_dev_err(chub->dev, "%s: download os fails\n", __func__);
			ret = -EINVAL;
			goto out;
		}
	}

	/* enable mailbox interrupt to get 'alive' event */
	enable_irq(chub->irq_mailbox);
	irq_disabled = false;

	if (clk_con_div_clkcmu_chub_bus != NULL) {
		__raw_writel(0x0, clk_con_div_clkcmu_chub_bus);
		msleep(5);
		nanohub_dev_info(chub->dev, "%s: clk div: %x\n", __func__,
			__raw_readl(clk_con_div_clkcmu_chub_bus));
	}

	/* reset release */
	if (atomic_read(&chub->chub_shutdown)) {
		contexthub_status_reset(chub);
		ret = cal_chub_reset_release();
		atomic_set(&chub->chub_shutdown, 0);
	} else {
		nanohub_dev_err(chub->dev,
				"contexthub status isn't shutdown. fails to reset: %d, %d\n",
				atomic_read(&chub->chub_shutdown),
				atomic_read(&chub->chub_status));
		ret = -EINVAL;
	}

	ret = contexthub_alive_check(chub);
	if (ret) {
		nanohub_dev_err(chub->dev, "%s: chub reset fail! (ret:%d)\n",
				__func__, ret);
	} else {
		nanohub_dev_info(chub->dev, "%s: chub reset done! (cnt:%d)\n",
				 __func__, chub->err_cnt[CHUB_ERR_RESET_CNT]);
		chub->err_cnt[CHUB_ERR_RESET_CNT]++;
		contexthub_reset_token(chub);
		contexthub_log_active_work(chub);
	}
#if defined(CONFIG_EXYNOS_LPD)
	contexthub_peri_gpio_addr();
#endif
out:
	if (ret) {
		atomic_set(&chub->chub_status, CHUB_ST_RESET_FAIL);
		if (irq_disabled)
			enable_irq(chub->irq_mailbox);
		nanohub_dev_err(chub->dev,
				"%s: chub reset fail! should retry to reset (ret:%d), irq_disabled:%d\n",
				__func__, ret, irq_disabled);
	}
	msleep(100);		/* wakeup delay */
	chub_wake_event(&chub->reset_lock);
	contexthub_set_in_reset(chub, false);
	contexthub_notifier_call(chub, CHUB_FW_ST_ON);
	enable_irq(chub->irq_wdt);
	chub->irq_wdt_disabled = 0;
	chub_wake_unlock(chub->ws_reset);
	mutex_unlock(&reset_mutex);
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
	if (chub->sysevent_dev) {
		chub->sysevent_status = sysevent_get("CHB");

		if(chub->sysevent_status == PTR_ERR || chub->sysevent_status == NULL)
			nanohub_warn("%s sysevent_get fail!", __func__);
	}
#if defined(CONFIG_EXYNOS_LPD)
	chub->lpd_sysevent_dev =  sysevent_get("LPD");
#endif
#endif
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
	if (chub->mlog.memlog_printf_chub)
		memlog_sync_to_file(chub->mlog.memlog_printf_chub);
#endif
#ifdef CONFIG_CONTEXTHUB_DEBUG
	if (err)
		panic("contexthub crash");
#endif

#ifdef CONFIG_SENSORS_SSP
	if (!ret) {
		nanohub_warn("[SSP] ssp_platform_start_refrsh_task!");
		ssp_platform_start_refrsh_task(chub->ssp_data);
    }
#endif

	return ret;
}

void contexthub_check_rtc(struct device *dev)
{
	void __iomem *rtc, *mct;
	s64 mct1, mct2, val;
	s32 rtc1, rtc2;

	rtc = ioremap(0x12a70000, 0x100);
	mct = ioremap(0x10040100, 0x300);
	
	__raw_writel(0xffffffff, rtc + 0x48);
	__raw_writel(__raw_readl(rtc + 0x3c) | (0x1 << 0), rtc + 0x3c);
	mdelay(10);
	nanohub_dev_info(dev, "mct 0x%x rtc 0x%x\n", __raw_readl(mct), __raw_readl(rtc + 0x94));
	mct1 = __raw_readl(mct + 0x4);
	mct1 = (mct1 << 32) | __raw_readl(mct);
	rtc1 = __raw_readl(rtc + 0x94);
	mdelay(100);
	mct2 = __raw_readl(mct);
	rtc2 = __raw_readl(rtc + 0x94);
	mct2 |= ((s64)__raw_readl(mct + 0x4) << 32);
	val = (mct2 - mct1) * 32768 / 26000000;
	nanohub_dev_info(dev, "mct 0x%x rtc 0x%x\n", mct2, rtc2);
	nanohub_dev_info(dev, "diff: mct %u\n", mct2 - mct1);
	nanohub_dev_info(dev, "should be: %u / rtc: %u (%d%%)\n", val, rtc2 - rtc1,
		(val - (rtc2 - rtc1)) * 100 / val);
	iounmap(rtc);
	iounmap(mct);
}

int contexthub_poweron(struct contexthub_ipc_info *chub)
{
	int ret = 0;
	struct device *dev = chub->dev;

	if (atomic_read(&chub->chub_status) == CHUB_ST_NO_POWER) {
		if (!chub->sram || !chub->sram_size) {
			nanohub_dev_err(dev, "%s sram information is not available!\n", __func__);
			return -1;
		} else
			nanohub_dev_info(dev, "%s 0x%lx, 0x%lx\n", __func__, chub->sram, chub->sram_size);
		memset_io(chub->sram, 0, chub->sram_size);
#if defined(CONFIG_EXYNOS_IMGLOADER) && !defined(CONFIG_SENSORS_SSP_HRM_ADI)
		ssp_check_hrm_int(chub->ssp_data, chub->dev);
		ret = imgloader_boot(&chub->chub_img_desc[0]);
#else
		ret = contexthub_download_image(chub, IPC_REG_BL);
#endif

#if 0//def CONFIG_SENSORS_SSP // serene_tmp
		if (ssp_get_firmware_type(chub->ssp_data) != FW_TYPE_PUSH) {
			nanohub_dev_err(dev, "%s temporarily blocked \n", __func__);
			return -1;
		}
#endif

		if (ret) {
			nanohub_dev_warn(dev, "fails to download bootloader\n");
			return ret;
		}

		ret = contexthub_ipc_drv_init(chub);
		if (ret) {
			dev_warn(dev, "fails to init ipc\n");
			return ret;
		}

		if (!strcmp(chub->os_name, os_image[0]) || chub->os_name[0] != 'o') {
			nanohub_dev_info(dev, "OS name not saved: %s\n",
					 chub->os_name);
			chub->sel_os = false;
		} else {
			nanohub_dev_info(dev, "OS name saved: %s\n",
					 chub->os_name);
			chub->sel_os = true;
		}
		if (chub->os_type != OS_TYPE_ONEBINARY) {
#ifdef CONFIG_EXYNOS_IMGLOADER
			ret = imgloader_boot(&chub->chub_img_desc[chub->num_os + 1]);
#else
			ret = contexthub_download_image(chub, IPC_REG_OS);
#endif
			if (ret) {
				nanohub_dev_warn(dev, "fails to download kernel\n");
				return ret;
			}
		} else
			chub->sel_os = true;

		if (chub->chub_dfs_gov) {
			nanohub_dev_info(dev, "%s: set dfs gov:%d\n", __func__, chub->chub_dfs_gov);
			ipc_set_dfs_gov(chub->chub_dfs_gov);
		}
		nanohub_dev_info(dev, "%s: get dfs gov:%d\n", __func__, ipc_get_dfs_gov());

		contexthub_status_reset(chub);

#ifdef NEED_TO_RTC_SYNC
		contexthub_check_time();
#endif
		atomic_set(&chub->chub_status, CHUB_ST_POWER_ON);

		/* enable Dump gpr */
		IPC_HW_WRITE_DUMPGPR_CTRL(chub->chub_dumpgpr, 0x1);

		// ioremap for control chub bus clk.
		clk_con_div_clkcmu_chub_bus = ioremap(0x12801800, 0x10);

		if (clk_con_div_clkcmu_chub_bus == NULL) {
			nanohub_info("chub_bus ioremap fail. skip changed frequency.\n");
		}

		contexthub_check_rtc(dev);
		ret = contexthub_soc_poweron(chub);
#if IS_ENABLED(CONFIG_EXYNOS_S2MPU)
		ret = exynos_request_fw_stage2_ap("CHUB");
		if (ret)
			nanohub_dev_err(chub->dev, "%s fw stage2 fail %d\n", __func__, ret);
#endif
		/* don't send alive with first poweron of multi-os */
		if (chub->sel_os)
			ret = contexthub_alive_check(chub);
		if (ret) {
			dev_warn(dev, "fails to poweron\n");
			return ret;
		}
		if (atomic_read(&chub->chub_status) == CHUB_ST_RUN) {
			/* chub driver directly gets alive event without multi-os */
			nanohub_dev_info(dev, "%s: contexthub power-on\n",
					 __func__);
		} else {
			if (chub->sel_os) {	/* without multi-os */
				nanohub_dev_warn(dev, "contexthub failed to power-on\n");
			} else {	/* with multi-os */
				nanohub_dev_info(dev, "%s: wait for multi-os poweron\n", __func__);
				ret =
				    chub_wait_event(&chub->poweron_lock,
						    WAIT_TIMEOUT_MS * 2);
				nanohub_dev_info(dev,
						 "%s: multi-os poweron %s, status:%d, ret:%d, flag:%d\n",
						 __func__,
						 (atomic_read(&chub->chub_status) == CHUB_ST_RUN) ?
						 "success" : "fails",
						 atomic_read(&chub->chub_status), ret,
						 chub->poweron_lock.flag);
			}
		}
	} else {
		/* CHUB already went through poweron sequence */
		return -EINVAL;
	}
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
	if (chub->sysevent_dev) {
		chub->sysevent_status = sysevent_get("CHB");
		if(chub->sysevent_status == PTR_ERR || chub->sysevent_status == NULL)
			nanohub_warn("%s sysevent_get fail!", __func__);
	}
#if defined(CONFIG_EXYNOS_LPD)
	/* notify LPD */
	chub->lpd_sysevent_dev =  sysevent_get("LPD");
#endif
#endif
	contexthub_notifier_call(chub, CHUB_FW_ST_POWERON);
#ifdef IPC_DEBUG
	ipc_test(chub);
#endif

#ifdef CONFIG_CONTEXTHUB_SENSOR_DEBUG
#ifdef ALIVE_WORK
	nanohub_dev_info(dev, "contexthub schedule sensor_alive_work\n");
	schedule_delayed_work(&chub->sensor_alive_work, msecs_to_jiffies(SENSORTASK_KICK_MS * 4));
	chub->sensor_alive_work_run = true;
#else
	nanohub_dev_info(dev, "skip sensor_alive_work\n");
#endif
#endif
#if defined(CONFIG_EXYNOS_LPD)
	contexthub_peri_gpio_addr();
#endif
	nanohub_info("%s done!\n", __func__);

#ifdef CONFIG_SENSORS_SSP
	//ssp_device_enable_ap_wakeup(chub->ssp_data, dev);
#endif

	return 0;
}

static void contexthub_select_os(struct contexthub_ipc_info *chub)
{
	int ret;
	u8 val = (u8)ipc_read_hw_value(IPC_VAL_HW_DEBUG);

	if (!val) {
		nanohub_dev_warn(chub->dev, "%s os number is invalid\n");
		val = 1;
	}
	chub->sel_os = true;
	chub->num_os = val;

	strcpy(chub->os_name, os_image[val]);
	nanohub_dev_info(chub->dev, "%s selected os_name = %s\n", __func__,
			 chub->os_name);
			 
#if defined(CONFIG_EXYNOS_IMGLOADER) && !defined(CONFIG_SENSORS_SSP_HRM_ADI)
	ssp_check_hrm_int(chub->ssp_data, chub->dev);
	ret = imgloader_boot(&chub->chub_img_desc[val + 1]);
#else
	ret = contexthub_download_image(chub, IPC_REG_OS);
#endif
	if (ret)
		nanohub_warn("%s: os download fail!");
	ipc_write_hw_value(IPC_VAL_HW_BOOTMODE, chub->os_load);
	ipc_write_hw_value(IPC_VAL_HW_DEBUG, READY_TO_GO);
	contexthub_alive_check(chub);
	nanohub_dev_info(chub->dev, "%s done: wakeup interrupt\n", __func__);
	chub_wake_event(&chub->poweron_lock);
}

static void handle_debug_work_func(struct work_struct *work)
{
	struct contexthub_ipc_info *chub =
	    container_of(work, struct contexthub_ipc_info, debug_work);

	/* 1st work: os select on booting */
	if ((atomic_read(&chub->chub_status) == CHUB_ST_POWER_ON) && (!chub->sel_os)) {
		contexthub_select_os(chub);
		return;
	}

	mutex_lock(&chub_err_mutex);
	/* 2nd work: slient reset */
	if (chub->cur_err) {
		int ret;
		int err = chub->cur_err;

		chub->cur_err = 0;
		nanohub_dev_info(chub->dev,
				 "%s: request silent reset. err:%d, status:%d, in-reset:%d\n",
				 __func__, err, __raw_readl(&chub->chub_status),
				 __raw_readl(&chub->in_reset));
		ret = chub_reset(chub->dev, NULL, NULL, 0);
		if (ret) {
			nanohub_dev_warn(chub->dev,
					 "%s: fails to reset:%d. status:%d\n",
					 __func__, ret,
					 __raw_readl(&chub->chub_status));
		} else {
			chub->cur_err = 0;
			nanohub_dev_info(chub->dev, "%s: done(chub reset)\n", __func__);
		}
	}
	mutex_unlock(&chub_err_mutex);
}

static ssize_t chub_poweron(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = dev_get_drvdata(dev);
	int ret = contexthub_poweron(chub);

	return ret < 0 ? ret : count;
}

static ssize_t chub_reset(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = dev_get_drvdata(dev);
	int ret;

#ifdef CONFIG_SENSOR_DRV
	ret = nanohub_hw_reset(chub->data);
#else
	ret = contexthub_reset(chub, 1, CHUB_ERR_NONE);
#endif

	return ret < 0 ? ret : count;
}

static struct device_attribute attributes[] = {
	__ATTR(poweron, 0220, NULL, chub_poweron),
	__ATTR(reset, 0220, NULL, chub_reset),
};

#define MAX_FIRMWARE_NUM 3
#if IS_ENABLED(CONFIG_EXYNOS_IMGLOADER)
static int contexthub_imgloader_mem_setup(struct imgloader_desc *desc, const u8 *metadata,
					  size_t size, phys_addr_t *fw_phys_base,
					  size_t *fw_bin_size, size_t *fw_mem_size)
{
	void *addr = NULL;
	struct contexthub_ipc_info *chub = (struct contexthub_ipc_info *)desc->data;

	if (chub->os_type == OS_TYPE_ONEBINARY) {
		*fw_phys_base = chub->sram_phys;
		*fw_bin_size = size;
		*fw_mem_size = CHUB_SRAM_SIZE;
		addr = chub->sram;
	} else {
		*fw_phys_base = chub->sram_phys + (desc->fw_id ? SRAM_OFFSET_OS : 0);
		*fw_bin_size = size;
		*fw_mem_size = desc->fw_id ? CHUB_SRAM_SIZE - SRAM_OFFSET_OS : SRAM_OFFSET_OS;
		addr = desc->fw_id ? ipc_get_base(IPC_REG_OS) : chub->sram;
	}
	memcpy_toio(addr, metadata, size);
	nanohub_info("%s chub image %s loaded by imgloader\n", __func__,
		     desc->fw_name);

	contexthub_verify_symtable(chub, (void *)metadata, size);

	return 0;
}

static int contexthub_imgloader_verify_fw(struct imgloader_desc *desc,
					  phys_addr_t fw_phys_base,
					  size_t fw_bin_size,
					  size_t fw_mem_size)
{
#ifdef CONFIG_EXYNOS_S2MPU
	u64 ret64 = exynos_verify_subsystem_fw(desc->name, desc->fw_id,
					   fw_phys_base, fw_bin_size,
					   ALIGN(fw_mem_size, SZ_4K));
	if (ret64) {
		nanohub_dev_warn(desc->dev,
				 "Failed F/W verification, ret=%llu\n", ret64);
		return -EIO;
	}

	ret64 = exynos_request_fw_stage2_ap(desc->name);
	if (ret64) {
		nanohub_dev_warn(desc->dev,
				 "Failed F/W verification to S2MPU, ret=%llu\n",
				 ret64);
		return -EIO;
	}
#endif
	return 0;
}

struct imgloader_ops contexthub_imgloader_ops = {
	.mem_setup = contexthub_imgloader_mem_setup,
	.verify_fw = contexthub_imgloader_verify_fw
};
#endif

static int contexthub_imgloader_init(struct contexthub_ipc_info *chub)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_EXYNOS_IMGLOADER)
	int i;

	if (chub->os_type == OS_TYPE_ONEBINARY) {
		chub->chub_img_desc[0].dev = chub->dev;
		chub->chub_img_desc[0].owner = THIS_MODULE;
		chub->chub_img_desc[0].ops = &contexthub_imgloader_ops;
		chub->chub_img_desc[0].data = (void *)chub;
		chub->chub_img_desc[0].name = "CHUB";
		chub->chub_img_desc[0].s2mpu_support = true;
		chub->chub_img_desc[0].fw_name = chub->os_name;
		chub->chub_img_desc[0].fw_id = 0;
		ret = imgloader_desc_init(&chub->chub_img_desc[0]);
	} else {
		for (i = 0; i < MAX_FIRMWARE_NUM; i++) {
			chub->chub_img_desc[i].dev = chub->dev;
			chub->chub_img_desc[i].owner = THIS_MODULE;
			chub->chub_img_desc[i].ops = &contexthub_imgloader_ops;
			chub->chub_img_desc[i].data = (void *)chub;
			chub->chub_img_desc[i].name = "CHUB";
			chub->chub_img_desc[i].s2mpu_support = true;
			chub->chub_img_desc[i].fw_name =
			    i ? os_image[i - 1] : "bl.unchecked.bin";
			chub->chub_img_desc[i].fw_id = i ? 1 : 0;
			ret |= imgloader_desc_init(&chub->chub_img_desc[i]);
		}
	}
#endif
	return ret;
}

#ifdef CONFIG_EXYNOS_S2MPU
static int contexthub_s2mpu_notifier(struct s2mpufd_notifier_block *nb,
				     struct s2mpufd_notifier_info *nb_data)
{
	struct contexthub_ipc_info *data =
	    container_of(nb, struct contexthub_ipc_info, s2mpu_nb);
	struct s2mpufd_notifier_info *s2mpu_data = nb_data;

	(void)data;
	(void)s2mpu_data;

	nanohub_info("%s called!\n", __func__);
	contexthub_handle_debug(data, CHUB_ERR_S2MPU);
	return S2MPUFD_NOTIFY_OK;
}
#endif

int contexthub_bootup_init(struct contexthub_ipc_info *chub)
{
	int i;
	int ret = 0;

	contexthub_set_baaw(chub);
#ifdef CONFIG_EXYNOS_S2MPU
	chub->s2mpu_nb.notifier_call = contexthub_s2mpu_notifier;
	chub->s2mpu_nb.subsystem = "CHUB";
	s2mpufd_notifier_call_register(&chub->s2mpu_nb);
#endif
	chub->ws_reset = chub_wake_lock_init(chub->dev, "chub_reboot");
	nanohub_dev_info(chub->dev, "%s with %s FW and %lu clk is done\n",
			 __func__, chub->os_name, chub->clkrate);
	init_waitqueue_head(&chub->poweron_lock.event);
	init_waitqueue_head(&chub->reset_lock.event);
	init_waitqueue_head(&chub->chub_alive_lock.event);
	atomic_set(&chub->poweron_lock.flag, 0);
	atomic_set(&chub->chub_alive_lock.flag, 0);
	contexthub_reset_token(chub);
	contexthub_set_in_reset(chub, 0);
	contexthub_imgloader_init(chub);
	INIT_WORK(&chub->debug_work, handle_debug_work_func);
#ifdef CONFIG_CONTEXTHUB_SENSOR_DEBUG
	INIT_DELAYED_WORK(&chub->sensor_alive_work, sensor_alive_check_func);
#endif

	for (i = 0, ret = 0; i < ARRAY_SIZE(attributes); i++) {
		ret = device_create_file(chub->dev, &attributes[i]);
		if (ret)
			nanohub_dev_warn(chub->dev,
					 "Failed to create file: %s\n",
					 attributes[i].attr.name);
	}
	return ret;
}

#ifdef CONFIG_EXYNOS_IMGLOADER_UNDEF
#define CONFIG_EXYNOS_IMGLOADER
#endif

