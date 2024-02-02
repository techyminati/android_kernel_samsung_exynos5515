/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * notifer and sysevent interface to communicate with other drivers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "lpd.h"
#ifdef CONFIG_EXYNOS_ITMON
#include <soc/samsung/exynos-itmon.h>
#endif
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
#include <soc/samsung/memlogger.h>
#endif
#include <soc/samsung/exynos-lpd.h>

#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
static int lpd_sysevt_chub_on(const struct sysevent_desc *sysevent)

{
	struct lpd_device *lpd = container_of(sysevent, struct lpd_device, nb.chub_desc);

	NULL_CHECK(lpd);

	lpd->sram.state |= LPD_SRAM_CHUB_ON;
	
	lpd_info("%s called!\n", __func__);
	return 0;
}

static int lpd_sysevt_chub_off(const struct sysevent_desc *sysevent, bool force_stop)

{
	struct lpd_device *lpd = container_of(sysevent, struct lpd_device, nb.chub_desc);

	NULL_CHECK(lpd);

	lpd->sram.state &= ~LPD_SRAM_CHUB_ON;
	
	lpd_info("%s called!\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_EXYNOS_ITMON
static int lpd_notifier_itmon(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct lpd_device *lpd = container_of(nb, struct lpd_device, nb.itmon);
	struct itmon_notifier *itmon_data = nb_data;

	if(lpd && itmon_data) {
		if (itmon_data->master &&
			((!strncmp("CM4_SHUB_CD", itmon_data->master, sizeof("CM4_SHUB_CD") - 1)) ||
			(!strncmp("CM4_SHUB_P", itmon_data->master, sizeof("CM4_SHUB_P") - 1)) ||
			(!strncmp("PDMA_SHUB", itmon_data->master, sizeof("PDMA_SHUB") - 1)) ||
			(!strncmp("CM4_CHUB", itmon_data->master, sizeof("CM4_CHUB") - 1)) ||
			(!strncmp("PDMA_CHUB", itmon_data->master, sizeof("PDMA_CHUB") - 1)) ||
			(!strncmp("CHUB", itmon_data->master, sizeof("CHUB") - 1))
			)) {
			lpd_info("%s: chub(%s) itmon detected: action:%lu!!\n",
				__func__, itmon_data->master, action);
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
			if (IS_LPD_SRAM_AVAILABLE(lpd))
				memlog_do_dump(lpd->d.mlog.memlog_sram, MEMLOG_LEVEL_EMERG);
#endif
		}

		if ((itmon_data->port && 
				(strncmp("DPU", itmon_data->port, sizeof("DPU") - 1) == 0)) ||
			(itmon_data->dest && 
				(strncmp("DPU", itmon_data->dest, sizeof("DPU") - 1) == 0))) {
			lpd_info("%s: port: %s, dest: %s, action: %lu!!\n", 
				__func__, itmon_data->port, itmon_data->dest, action);
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
			if (IS_LPD_SRAM_AVAILABLE(lpd))
				memlog_do_dump(lpd->d.mlog.memlog_sram, MEMLOG_LEVEL_EMERG);
#endif
		}

		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}
#endif

static int lpd_notifier_panic(struct notifier_block *nb,
				    unsigned long action, void *data)
{
	struct lpd_device *lpd = container_of(nb, struct lpd_device, nb.panic);
	lpd_err("==============LPD LOG on PANIC==============\n");
	lpd_logbuf_outprint(lpd);
	lpd_err("==============LPD LOG END===================\n");

#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
	if (IS_LPD_SRAM_AVAILABLE(lpd))
		memlog_do_dump(lpd->d.mlog.memlog_sram, MEMLOG_LEVEL_EMERG);
#endif
	return 0;
}


static void update_dpu_state(struct lpd_device *lpd, int *state)
{
	if (state == NULL)
		return;

	if ((*state < DPU_STATE_ON) || (*state > DPU_STATE_DOZE_SUSPEND)) {
		lpd_err("%s: invalid state: %d\n", __func__, *state);
		return;
	}

	if (lpd->dpu_state != *state) {
		lpd_info("%s change dpu state: %d -> %d\n", __func__, lpd->dpu_state, *state);
		lpd->dpu_state = *state;
	}
}


static int lpd_notifier_begin(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{	
	struct lpd_device *lpd = container_of(nb, struct lpd_device, nb.begin);	
	int noti = NOTIFY_DONE;
	int ret = 0;
	struct lpd_dpu_info *dpu_info;

	if (IS_ERR_OR_NULL(lpd)) {
		lpd_err("ERR: %s invalid parameter: lpd is null\n", __func__);
		return NOTIFY_BAD;
	}

	switch (action) {
		case LPD_NOTIFIER_CONFIG:
			lpd_dbg("%s: LPD_NOTIFIER_CONFIG: enable: %d\n", __func__, lpd->enable);
			if (lpd->enable) {
				dpu_info = (struct lpd_dpu_info *)nb_data;
				if (dpu_info == NULL) {
					lpd_err("ERR: %s: nb data is null\n", __func__);
					return NOTIFY_STOP;
				}
#if defined(LPD_SRAM_POWER_GATING)
				memcpy(&lpd->img_desc.dpu, dpu_info, sizeof(struct lpd_dpu_info));
#else
				lpd_update_dpu_data(lpd, dpu_info);
				lpd_init_logbuf(lpd);
#endif
				lpd_print_img_desc(lpd);
			}
			noti = NOTIFY_STOP; // OK but no need to go further
			break;
		case LPD_NOTIFIER_START_PRE:
			update_dpu_state(lpd, (int *)nb_data);
			ret = lpd_pm_acquire(lpd);
			if (ret < 0)
				noti = NOTIFY_BAD;
			else
				noti = NOTIFY_STOP;  // OK but no need to go further
			break;
		case LPD_NOTIFIER_START:
			update_dpu_state(lpd, (int *)nb_data);
			ret = lpd_start(lpd, LPD_NOTIFIER_STEPS_BEGIN);
			if (ret < 0)
				noti = NOTIFY_BAD;
			else
				noti = NOTIFY_OK;
			break;
		case LPD_NOTIFIER_STOP:
			update_dpu_state(lpd, (int *)nb_data);
			ret = lpd_stop(lpd, LPD_NOTIFIER_STEPS_BEGIN);
			if (ret <0)
				noti = NOTIFY_BAD;
			else
				noti = NOTIFY_OK;
			break;
		case LPD_NOTIFIER_STOP_POST:
			update_dpu_state(lpd, (int *)nb_data);
			ret = lpd_pm_release(lpd);
			if (ret <0)
				noti = NOTIFY_BAD;
			else
				noti = NOTIFY_STOP;  // OK but no need to go further
			break;
		default:
			lpd_err("no operation %d\n",action);
			noti = NOTIFY_BAD; // no need to go further
			break;
	}

	return noti;
}

static int lpd_notifier_end(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct lpd_device *lpd = container_of(nb, struct lpd_device, nb.end);
	int noti = NOTIFY_DONE;
	int ret = 0;

	if (IS_ERR_OR_NULL(lpd)) {
		lpd_err("ERR: %s invalid parameter: lpd is null\n", __func__);
		return NOTIFY_BAD;
	}

	switch (action) {
		case LPD_NOTIFIER_START:
		update_dpu_state(lpd, (int *)nb_data);
		ret = lpd_start(lpd, LPD_NOTIFIER_STEPS_END);
		if (ret < 0)
			noti = NOTIFY_BAD;
		else
			noti = NOTIFY_OK;
		break;
	case LPD_NOTIFIER_STOP:
		update_dpu_state(lpd, (int *)nb_data);
		ret = lpd_stop(lpd, LPD_NOTIFIER_STEPS_END);
		if (ret < 0)
			noti = NOTIFY_BAD;
		else
			noti = NOTIFY_OK;
		break;
	default:
		lpd_err("no operation %d\n", action);
		noti = NOTIFY_BAD; // no need to go further
		break;
	}

	return noti;
}


// communication path: DPU <-> LPD <-> CHUB
static BLOCKING_NOTIFIER_HEAD(lpd_notifier_list);
int lpd_notifier_register(struct notifier_block *nb)
{
	if (nb == NULL) {
		lpd_err("ERR:%s: nb is null\n", __func__);
		return -1;
	}

	if (nb->priority > LPD_NOTIFIER_PRIORITY_MAX) {
		lpd_err("LPD notifier priority %d is not allowed", nb->priority);
		return -1;
	}

	return blocking_notifier_chain_register(&lpd_notifier_list, nb);
}
EXPORT_SYMBOL(lpd_notifier_register);

int lpd_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&lpd_notifier_list, nb);
}
EXPORT_SYMBOL(lpd_notifier_unregister);

/*
 * 0: init (not allowed)
 * 1: config (set panel size)
 * 2: prepare (not allowed)
 * 3: start
 * 4: stop
 */
int lpd_notifier_call(u32 action, void *data)
{
	return blocking_notifier_call_chain(&lpd_notifier_list, action, data);
}
EXPORT_SYMBOL(lpd_notifier_call);

int lpd_notifier_init(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);
#ifdef CONFIG_EXYNOS_ITMON
	/* itmon notifier */
	lpd->nb.itmon.notifier_call = lpd_notifier_itmon;
	itmon_notifier_chain_register(&lpd->nb.itmon);
#endif
	/* panic notifier */
	lpd->nb.panic.notifier_call = lpd_notifier_panic;
	atomic_notifier_chain_register(&panic_notifier_list, &lpd->nb.panic);

	/* dpu notifier */
	lpd->nb.begin.notifier_call = lpd_notifier_begin;
	lpd->nb.begin.priority = LPD_NOTIFIER_PRIORITY_LPD_BEGIN;
	lpd_notifier_register(&lpd->nb.begin);

	lpd->nb.end.notifier_call = lpd_notifier_end;
	lpd->nb.end.priority = LPD_NOTIFIER_PRIORITY_LPD_END;
	lpd_notifier_register(&lpd->nb.end);

#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
	/* chub power on/off sysevent */
	lpd->nb.chub_desc.name = "LPD";
	lpd->nb.chub_desc.owner = THIS_MODULE;
	lpd->nb.chub_desc.shutdown = lpd_sysevt_chub_off;
	lpd->nb.chub_desc.powerup = lpd_sysevt_chub_on;
	lpd->nb.chub_desc.ramdump = NULL;
	lpd->nb.chub_desc.crash_shutdown = NULL;
	lpd->nb.chub_desc.dev = lpd->dev;
	lpd->nb.chub_dev = sysevent_register(&lpd->nb.chub_desc);
	NULL_CHECK(lpd->nb.chub_dev);
#endif

	return 0;	
}

int lpd_notifier_uninit(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);
#ifdef CONFIG_EXYNOS_ITMON
	/* nothing for itmon notifier */
#endif
	/* panic notifier */
	atomic_notifier_chain_unregister(&panic_notifier_list, &lpd->nb.panic);

	/* dpu notifier */
	lpd_notifier_unregister(&lpd->nb.begin);
	lpd_notifier_unregister(&lpd->nb.end);

#if 0 // unregister failed as CHUB does not call sysevent_put before shoutdown
#ifdef CONFIG_EXYNOS_SYSTEM_EVENT
	/* chub power on/off sysevent */
	sysevent_unregister(lpd->nb.chub_dev);
#endif
#endif

	return 0;
}
