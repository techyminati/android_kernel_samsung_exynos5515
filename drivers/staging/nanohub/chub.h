/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * CHUB IF Driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 * Authors:
 *      Boojin Kim <boojin.kim@samsung.com>
 *      Sukwon Ryu <sw.ryoo@samsung.com>
 *
 */

#ifndef __CONTEXTHUB_IPC_H_
#define __CONTEXTHUB_IPC_H_

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/pm_wakeup.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/platform_data/nanohub.h>
#include <linux/sched/clock.h>
#include <linux/sched/signal.h>
#include <linux/delay.h>
#ifdef CONFIG_EXYNOS_S2MPU
#include <soc/samsung/exynos-s2mpu.h>
#endif
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
#include <soc/samsung/sysevent.h>
#endif
#ifdef CONFIG_EXYNOS_IMGLOADER
#include <soc/samsung/imgloader.h>
#endif
#include "ipc_chub.h"
#include "chub_log.h"

#ifdef CONFIG_SENSORS_SSP
#include "../../sensorhub/slsi/ssp_platform.h"
#endif

#define WAIT_TRY_CNT (3)
#define RESET_WAIT_TRY_CNT (10)
#define WAIT_CHUB_MS (100)
#define WAIT_CHUB_DFS_SCAN_MS_MAX (120000)
#define WAIT_CHUB_DFS_SCAN_MS (2000)
#define WAIT_TIMEOUT_MS (1000)
#define MAX_USI_CNT (15)
#define BAAW_MAX_WINDOWS (8)

enum { CHUB_ON, CHUB_OFF };
enum { C2A_ON, C2A_OFF };

#define EXYNOS_CHUB 2ull

#ifdef CONFIG_SENSORS_SSP_HRM_ADI
#define OS_IMAGE_ONEBINARY "os.checked_org.bin"
#define OS_IMAGE_ONEBINARY_ADI "os.checked_ADI.bin"
#define OS_IMAGE_ONEBINARY_PUSH "os.checked.bin"
#else
#define OS_IMAGE_ONEBINARY "os.checked.bin"
#endif

#define OS_IMAGE_MULTIOS "os.checked_0.bin"
#define OS_IMAGE_DEFAULT "os.checked_1.bin"

enum { OS_TYPE_DEFAULT, OS_TYPE_MULTIOS, OS_TYPE_ONEBINARY };

/* utils for nanohub main */
#define wait_event_interruptible_timeout_locked(q, cond, tmo)		\
({									\
	long __ret = (tmo);						\
	DEFINE_WAIT(__wait);						\
	if (!(cond)) {							\
		for (;;) {						\
			__wait.flags &= ~WQ_FLAG_EXCLUSIVE;		\
			if (list_empty(&__wait.entry))			\
				__add_wait_queue_entry_tail(&(q), &__wait);	\
			set_current_state(TASK_INTERRUPTIBLE);		\
			if ((cond))					\
				break;					\
			if (signal_pending(current)) {			\
				__ret = -ERESTARTSYS;			\
				break;					\
			}						\
			spin_unlock_irq(&(q).lock);			\
			__ret = schedule_timeout(__ret);		\
			spin_lock_irq(&(q).lock);			\
			if (!__ret) {					\
				if ((cond))				\
					__ret = 1;			\
				break;					\
			}						\
		}							\
		__set_current_state(TASK_RUNNING);			\
		if (!list_empty(&__wait.entry))				\
			list_del_init(&__wait.entry);			\
		else if (__ret == -ERESTARTSYS &&			\
			 /*reimplementation of wait_abort_exclusive() */\
			 waitqueue_active(&(q)))			\
			__wake_up_locked_key(&(q), TASK_INTERRUPTIBLE,	\
			NULL);						\
	} else {							\
		__ret = 1;						\
	}								\
	__ret;								\
})

enum mailbox_event {
	MAILBOX_EVT_UTC_MAX = IPC_DEBUG_UTC_MAX,
	MAILBOX_EVT_DUMP_STATUS = IPC_DEBUG_DUMP_STATUS,
	MAILBOX_EVT_DFS_GOVERNOR = IPC_DEBUG_DFS_GOVERNOR,
	MAILBOX_EVT_WAKEUP,
	MAILBOX_EVT_WAKEUP_CLR,
	MAILBOX_EVT_ENABLE_IRQ,
	MAILBOX_EVT_DISABLE_IRQ,
	MAILBOX_EVT_CHUB_ALIVE,
	MAILBOX_EVT_RT_LOGLEVEL,
	MAILBOX_EVT_START_LPD,
	MAILBOX_EVT_STOP_LPD,
	MAILBOX_EVT_MAX,
};

enum chub_status {
	CHUB_ST_NO_POWER,
	CHUB_ST_POWER_ON,
	CHUB_ST_RUN,
	CHUB_ST_NO_RESPONSE,
	CHUB_ST_ERR,
	CHUB_ST_HANG,
	CHUB_ST_RESET_FAIL,
	CHUB_ST_ITMON,
};

struct read_wait {
	atomic_t cnt;
	atomic_t flag;
	wait_queue_head_t event;
};

struct chub_evt {
	atomic_t flag;
	wait_queue_head_t event;
};

#ifdef USE_EXYNOS_LOG
#define CHUB_DBG_DIR "/data/exynos/log/chub"
#else
#define CHUB_DBG_DIR "/data"
#endif

#define CHUB_RESET_THOLD (3)
#define CHUB_RESET_THOLD_MINOR (5)

enum chub_err_type {
	CHUB_ERR_NONE,
	CHUB_ERR_ITMON, /* ITMON by CHUB */
	CHUB_ERR_S2MPU, /* S2MPU by CHUB */
	CHUB_ERR_FW_FAULT, /* CSP_FAULT by CHUB */
	CHUB_ERR_FW_WDT, /* watchdog by CHUB */
	CHUB_ERR_FW_REBOOT,
	CHUB_ERR_ISR, /* 5 */
	CHUB_ERR_CHUB_NO_RESPONSE,
	CHUB_ERR_CRITICAL, /* critical errors */
	CHUB_ERR_EVTQ_ADD, /* write event error */
	CHUB_ERR_EVTQ_EMTPY, /* isr empty event */
	CHUB_ERR_EVTQ_WAKEUP, /* 10 */
	CHUB_ERR_EVTQ_WAKEUP_CLR,
	CHUB_ERR_WRITE_FAIL,
	CHUB_ERR_MAJER,	/* majer errors */
	CHUB_ERR_READ_FAIL,
	CHUB_ERR_CHUB_ST_ERR,	/* 15: chub_status ST_ERR */
	CHUB_ERR_COMMS_WAKE_ERR,
	CHUB_ERR_FW_ERROR,
	CHUB_ERR_NEED_RESET, /* reset errors */
	CHUB_ERR_NANOHUB_LOG,
	CHUB_ERR_COMMS_NACK, /* 20: ap comms error */
	CHUB_ERR_COMMS_BUSY,
	CHUB_ERR_COMMS_UNKNOWN,
	CHUB_ERR_COMMS,
	CHUB_ERR_RESET_CNT,
	CHUB_ERR_KERNEL_PANIC,
	CHUB_ERR_MAX,
#ifdef CONFIG_SENSORS_SSP
	CHUB_ERR_SSP_RECOVERY = CHUB_ERR_MAX + 1,
#endif
};

struct contexthub_baaw_info {
	unsigned int baaw_c_chub_start;
	unsigned int baaw_c_chub_end;
	unsigned int baaw_c_chub_remap;
	unsigned int baaw_d_chub_values[BAAW_MAX_WINDOWS * 4];
};

struct contexthub_symbol_addr {
	unsigned int base;
	unsigned int size;
	unsigned int offset;
	unsigned int length;
};

struct contexthub_symbol_table {
	unsigned int size;
	unsigned int count;
	unsigned int name_offset;
	unsigned int reserved;
	struct contexthub_symbol_addr symbol[0];
};

struct contexthub_notifi_info {
	char name[IPC_NAME_MAX];
	enum ipc_owner id;
	struct contexthub_notifier_block *nb;
};

#define CHUB_IRQ_PIN_MAX (5)
struct contexthub_ipc_info {
	u32 cur_err;
	int err_cnt[CHUB_ERR_MAX];
	struct device *dev;
	struct nanohub_data *data;
	struct nanohub_platform_data *pdata;
	struct ipc_info *chub_ipc;
	wait_queue_head_t wakeup_wait;
	struct work_struct debug_work;
	struct work_struct log_work;
	int log_work_reqcnt;
	spinlock_t logout_lock;
	struct read_wait read_lock;
	struct chub_evt chub_alive_lock;
	struct chub_evt poweron_lock;
	struct chub_evt reset_lock;
	u32 sram_size;
	void __iomem *sram;
	phys_addr_t sram_phys;
	void __iomem *mailbox;
	void __iomem *chub_dumpgpr;
	void __iomem *baaw_c_chub;
	void __iomem *baaw_d_chub;
	void __iomem *chub_dump_cmu;
	void __iomem *chub_dump_sys;
	void __iomem *chub_dump_wdt;
	void __iomem *chub_dump_timer;
	void __iomem *chub_dump_pwm;
	void __iomem *chub_dump_rtc;
	void __iomem *usi_array[MAX_USI_CNT];
	void __iomem *sysreg_chub;
	void __iomem *gpio_peri;
	int usi_cnt;
	struct contexthub_baaw_info baaw_info;
	struct log_buffer_info *log_info;
	struct log_buffer_info *dd_log;
	struct LOG_BUFFER *dd_log_buffer;
	struct runtimelog_buf chub_rt_log;
	unsigned long clkrate;
	atomic_t log_work_active;
	atomic_t irq1_apInt;
	atomic_t wakeup_chub;
	atomic_t in_use_ipc;
	int irq_mailbox;
	int irq_wdt;
	bool irq_wdt_disabled;
	int utc_run;
	int powermode;
	atomic_t chub_shutdown;
	atomic_t chub_status;
	atomic_t chub_sleep;
	u32 wakeup_by_chub_cnt;
	atomic_t in_reset;
	bool sel_os;
	bool os_load;
	u8 num_os;
	u8 os_type;
	char os_name[MAX_FILE_LEN];
	u32 chub_dfs_gov;
	/* wakeup control */
	struct wakeup_source *ws;
	struct wakeup_source *ws_reset;
	/* chub f/w callstack */
	struct contexthub_symbol_table *symbol_table;
	/* communicate with others */
	/* chub notifiers */
	struct contexthub_notifier_block chub_cipc_nb;
	struct contexthub_notifi_info cipc_noti[IPC_OWN_MAX];
	struct notifier_block itmon_nb;
	struct notifier_block panic_nb;
#if defined(CONFIG_EXYNOS_LPD)
	struct notifier_block lpd_nb;
	struct sysevent_device *lpd_sysevent_dev;
	wait_queue_head_t lpd_end_wait;
	bool lpd_flag;
	void __iomem *mailbox_apm2chub;
#endif
	u32 irq_pin_len;
	u32 irq_pins[CHUB_IRQ_PIN_MAX];
	struct delayed_work sensor_alive_work;
	bool sensor_alive_work_run;
#ifdef CONFIG_EXYNOS_IMGLOADER
	/* image loader */
	struct imgloader_desc chub_img_desc[3];
#endif
#ifdef CONFIG_EXYNOS_S2MPU
	struct s2mpufd_notifier_block s2mpu_nb;
#endif
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
	/* sysevent */
	struct sysevent_desc sysevent_desc;
	struct sysevent_device *sysevent_dev;
	void *sysevent_status;
#endif
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
	/* memlogger */
	struct memlogs mlog;
#endif
#ifdef CONFIG_SENSORS_SSP
	char os_name_new[MAX_FILE_LEN];
	struct ssp_data *ssp_data;
#endif
};

#define SENSOR_VARIATION 10

#define IPC_HW_WRITE_DUMPGPR_CTRL(base, val) \
	__raw_writel((val), (base) + REG_CHUB_DUMPGPR_CTRL)
#define IPC_HW_READ_DUMPGPR_PCR(base) \
	__raw_readl((base) + REG_CHUB_DUMPGPR_PCR)

#define READ_CHUB_USI_CONF(base) \
	__raw_readl((base) + USI_REG_USI_CONFIG)

/*	CHUB BAAW Registers : CHUB BASE + 0x100000 */
#define REG_BAAW_D_CHUB0 (0x0)
#define REG_BAAW_D_CHUB1 (0x4)
#define REG_BAAW_D_CHUB2 (0x8)
#define REG_BAAW_D_CHUB3 (0xc)
#define BAAW_VAL_MAX (4)
#define BAAW_RW_ACCESS_ENABLE 0x80000003

#define IPC_MAX_TIMEOUT (0xffffff)
#define INIT_CHUB_VAL (-1)

#define IPC_HW_WRITE_BAAW_CHUB0(base, val) \
	__raw_writel((val), (base) + REG_BAAW_D_CHUB0)
#define IPC_HW_WRITE_BAAW_CHUB1(base, val) \
	__raw_writel((val), (base) + REG_BAAW_D_CHUB1)
#define IPC_HW_WRITE_BAAW_CHUB2(base, val) \
	__raw_writel((val), (base) + REG_BAAW_D_CHUB2)
#define IPC_HW_WRITE_BAAW_CHUB3(base, val) \
	__raw_writel((val), (base) + REG_BAAW_D_CHUB3)

static inline struct wakeup_source *chub_wake_lock_init(struct device *dev, const char *name)
{
	struct wakeup_source *ws = NULL;

	ws = wakeup_source_register(dev, name);
	if (ws == NULL) {
		nanohub_err("%s: wakelock register fail\n", name);
		return NULL;
	}

	return ws;
}

static inline void chub_wake_lock_destroy(struct wakeup_source *ws)
{
	if (ws == NULL) {
		nanohub_err("wakelock unregister fail\n");
		return;
	}

	wakeup_source_unregister(ws);
}

static inline void chub_wake_lock(struct wakeup_source *ws)
{
	if (ws == NULL) {
		nanohub_err("wakelock fail\n");
		return;
	}

	__pm_stay_awake(ws);
}

static inline void chub_wake_lock_timeout(struct wakeup_source *ws, long timeout)
{
	if (ws == NULL) {
		nanohub_err("wakelock timeout fail\n");
		return;
	}

	__pm_wakeup_event(ws, jiffies_to_msecs(timeout));
}

static inline void chub_wake_unlock(struct wakeup_source *ws)
{
	if (ws == NULL) {
		nanohub_err("wake unlock fail\n");
		return;
	}

	__pm_relax(ws);
}

static inline int chub_wake_lock_active(struct wakeup_source *ws)
{
	if (ws == NULL) {
		nanohub_err("wake unlock fail\n");
		return 0;
	}

	return ws->active;
}

static inline int contexthub_read_in_reset(struct contexthub_ipc_info *chub)
{
	return atomic_read(&chub->in_reset);
}

static inline void chub_wake_event(struct chub_evt *event)
{
	atomic_set(&event->flag, 1);
	wake_up_interruptible_sync(&event->event);
}

static inline int chub_wait_event(struct chub_evt *event, int timeout)
{
	atomic_set(&event->flag, 0);
	return wait_event_interruptible_timeout(event->event,
						 atomic_read(&event->flag),
						 msecs_to_jiffies(timeout));
}

static inline int contexthub_read_token(struct contexthub_ipc_info *chub)
{
	if (contexthub_read_in_reset(chub))
		return -EINVAL;

	return atomic_read(&chub->in_use_ipc);
}

static inline int contexthub_get_token(struct contexthub_ipc_info *chub)
{
	if (contexthub_read_in_reset(chub))
		return -EINVAL;

	atomic_inc(&chub->in_use_ipc);
	return 0;
}

static inline void contexthub_put_token(struct contexthub_ipc_info *chub)
{
	atomic_dec(&chub->in_use_ipc);
}

static inline void contexthub_status_reset(struct contexthub_ipc_info *chub)
{
	int i;

	/* clear ipc value */
	atomic_set(&chub->wakeup_chub, CHUB_OFF);
	atomic_set(&chub->irq1_apInt, C2A_OFF);
	atomic_set(&chub->read_lock.cnt, 0);
	atomic_set(&chub->read_lock.flag, 0);
	atomic_set(&chub->log_work_active, 0);

	/* chub err init */
	for (i = 0; i < CHUB_ERR_COMMS; i++)
		chub->err_cnt[i] = 0;

	ipc_write_hw_value(IPC_VAL_HW_BOOTMODE, chub->os_load);
	ipc_set_chub_clk((u32)chub->clkrate);
	chub->chub_rt_log.loglevel = CHUB_RT_LOG_DUMP_PRT;
	ipc_set_chub_bootmode(BOOTMODE_COLD, chub->chub_rt_log.loglevel);
}

static inline void clear_err_cnt(struct contexthub_ipc_info *chub,
				 enum chub_err_type err)
{
	if (chub->err_cnt[err])
		chub->err_cnt[err] = 0;
}

/* CHUB Driver */
void contexthub_check_time(void);
int contexthub_notifier_call(struct contexthub_ipc_info *chub, enum CHUB_STATE state);

/* IPC IFs */
int contexthub_ipc_drv_init(struct contexthub_ipc_info *chub);
int contexthub_get_sensortype(struct contexthub_ipc_info *chub, char *buf);
int contexthub_ipc_if_init(struct contexthub_ipc_info *chub);
int contexthub_chub_ipc_init(struct contexthub_ipc_info *chub);
int contexthub_ipc_write_event(struct contexthub_ipc_info *chub, enum mailbox_event event);
int contexthub_ipc_read(struct contexthub_ipc_info *chub, uint8_t *rx, int max_length, int timeout);
int contexthub_ipc_write(struct contexthub_ipc_info *chub, uint8_t *tx, int length, int timeout);

/* Bootup */
int contexthub_download_cm55_image(struct contexthub_ipc_info *chub);
void contexthub_handle_debug(struct contexthub_ipc_info *chub, enum chub_err_type err);
int contexthub_bootup_init(struct contexthub_ipc_info *chub);
int contexthub_poweron(struct contexthub_ipc_info *chub);
int contexthub_shutdown(struct contexthub_ipc_info *chub);
int contexthub_reset(struct contexthub_ipc_info *chub, bool force_load, enum chub_err_type err);
#endif
