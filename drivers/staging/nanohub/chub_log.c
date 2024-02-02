// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CHUB IF Driver Log
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 * Authors:
 *      Boojin Kim <boojin.kim@samsung.com>
 *      Sukwon Ryu <sw.ryoo@samsung.com>
 *
 */

#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/iommu.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "chub_log.h"
#include "chub_dbg.h"
#include "chub_exynos.h"
#include "ipc_chub.h"

/* cat /d/nanohub/log */
#define SIZE_OF_BUFFER (SZ_2M)

#define S_IRWUG (0660)
#define DEFAULT_FLUSH_MS (1000)

static struct dentry *dbg_root_dir __read_mostly;
static LIST_HEAD(log_list_head);
static struct log_buffer_info *print_info;
u32 auto_log_flush_ms;

static DEFINE_MUTEX(log_mutex);

static void handle_log_work_func(struct work_struct *work)
{
	struct contexthub_ipc_info *chub =
	    container_of(work, struct contexthub_ipc_info, log_work);
	int retrycnt = 0;

retry:
	if (contexthub_get_token(chub)) {
		chub_wait_event(&chub->reset_lock, WAIT_TIMEOUT_MS * 2);
		if (!retrycnt) {
			retrycnt++;
			goto retry;
		}
		atomic_set(&chub->log_work_active, 0);
		return;
	}
	ipc_logbuf_flush_on(1);
	mutex_lock(&log_mutex);
	if (ipc_logbuf_outprint())
		chub->err_cnt[CHUB_ERR_NANOHUB_LOG]++;
	mutex_unlock(&log_mutex);
	ipc_logbuf_flush_on(0);
	contexthub_put_token(chub);
	atomic_set(&chub->log_work_active, 0);
}

static int contexthub_memlog_init(struct contexthub_ipc_info *chub)
{
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
	int ret;

	ret = memlog_register("CHB", chub->dev, &chub->mlog.memlog_chub);
	if (!chub->mlog.memlog_chub) {
		dev_err(chub->dev, "memlog chub desc registration fail ret:%d\n", ret);
		return -1;
	}

	chub->mlog.memlog_printf_file_chub = memlog_alloc_file(chub->mlog.memlog_chub,
							       "log-fil", SZ_256K, SZ_1M, 1000, 3);
	dev_info(chub->dev, "memlog printf file chub %s\n",
		 chub->mlog.memlog_printf_file_chub ? "pass" : "fail");
	if (chub->mlog.memlog_printf_file_chub) {
		memlog_obj_set_sysfs_mode(chub->mlog.memlog_printf_file_chub, true);
		chub->mlog.memlog_printf_chub =
		    memlog_alloc_printf(chub->mlog.memlog_chub, SZ_256K,
					chub->mlog.memlog_printf_file_chub, "log-mem", 0);
	}
	dev_info(chub->dev, "memlog printf chub %s\n",
		 chub->mlog.memlog_printf_chub ? "pass" : "fail");

#endif
	return 0;
}

int contexthub_sync_memlogger(void *chub_p)
{
	int ret = 0;
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
	struct contexthub_ipc_info *chub = chub_p;

	if (chub->mlog.memlog_printf_chub)
		ret |= memlog_sync_to_file(chub->mlog.memlog_printf_chub);
#endif
	return ret;
}

void contexthub_log_active_work(void *chub_p)
{
	struct contexthub_ipc_info *chub = chub_p;

	if (ipc_logbuf_filled() && !atomic_read(&chub->log_work_active)) {
		chub->log_work_reqcnt++;	/* debug */
		atomic_set(&chub->log_work_active, 1);
		schedule_work(&chub->log_work);
	}
}

static int log_file_open(struct inode *inode, struct file *file)
{
	struct log_buffer_info *info = inode->i_private;

	dev_info(info->dev, "%s\n", __func__);

	file->private_data = inode->i_private;
	info->log_file_index = -1;

	return 0;
}

static ssize_t log_file_read(struct file *file, char __user *buf, size_t count,
			     loff_t *ppos)
{
	struct log_buffer_info *info = file->private_data;
	struct runtimelog_buf *rt_buf = info->rt_log;
	size_t end, size;
	bool first = (info->log_file_index < 0);
	int result;

	dev_dbg(info->dev, "%s(%zu, %lld)\n", __func__, count, *ppos);

	mutex_lock(&info->lock);
	if (info->log_file_index < 0) {
		info->log_file_index =
		    likely(rt_buf->wrap) ? rt_buf->write_index : 0;
	}

	do {
		end = ((info->log_file_index < rt_buf->write_index) ||
		       ((info->log_file_index == rt_buf->write_index) && !first)) ?
			rt_buf->write_index : rt_buf->buffer_size;
		size = min(end - info->log_file_index, count);
		if (size == 0) {
			mutex_unlock(&info->lock);
			if (file->f_flags & O_NONBLOCK) {
				dev_dbg(info->dev, "non block\n");
				return -EAGAIN;
			}
			rt_buf->updated = false;

			result = wait_event_interruptible(rt_buf->wq, rt_buf->updated);
			if (result != 0) {
				dev_dbg(info->dev, "interrupted\n");
				return result;
			}
			mutex_lock(&info->lock);
		}
	} while (size == 0);

	dev_dbg(info->dev, "start=%zd, end=%zd size=%zd\n",
		info->log_file_index, end, size);
	if (info->log_file_index >= 0) {
		if (copy_to_user
		    (buf, rt_buf->buffer + info->log_file_index, size)) {
			mutex_unlock(&info->lock);
			return -EFAULT;
		}
	} else
		dev_err(info->dev, "%s: log_file_index initialized again!", __func__);

	info->log_file_index += size;
	if (info->log_file_index >= SIZE_OF_BUFFER)
		info->log_file_index = 0;

	mutex_unlock(&info->lock);
	dev_dbg(info->dev, "%s: size = %zd\n", __func__, size);

	return size;
}

static unsigned int log_file_poll(struct file *file, poll_table *wait)
{
	struct log_buffer_info *info = file->private_data;

	dev_dbg(info->dev, "%s\n", __func__);

	poll_wait(file, &info->rt_log->wq, wait);
	return POLLIN | POLLRDNORM;
}

static const struct file_operations log_fops = {
	.open = log_file_open,
	.read = log_file_read,
	.poll = log_file_poll,
	.llseek = generic_file_llseek,
	.owner = THIS_MODULE,
};

static struct dentry *chub_dbg_get_root_dir(void)
{
	if (!dbg_root_dir)
		dbg_root_dir = debugfs_create_dir("nanohub", NULL);

	return dbg_root_dir;
}

static struct log_buffer_info *log_register_buffer
	(struct device *dev, int id, struct runtimelog_buf *rt_log, char *name, bool sram)
{
	struct log_buffer_info *info;

	if (!rt_log) {
		dev_warn(dev, "%s NULL rt_log", __func__);
		return NULL;
	}

	info = vmalloc(sizeof(struct log_buffer_info));
	if (!info) {
		dev_warn(dev, "%s info malloc fail", __func__);
		return NULL;
	}

	mutex_init(&info->lock);
	info->id = id;
	info->file_created = false;
	init_waitqueue_head(&rt_log->wq);
	info->dev = dev;
	info->rt_log = rt_log;

	/* HACK: clang make error
	buffer->index_reader = 0;
	buffer->index_writer = 0;
	*/
	info->save_file_name[0] = '\0';
	info->filp = NULL;

	dev_info(dev, "%s with %p buffer size %d.\n",
		 __func__, rt_log->buffer, rt_log->buffer_size);

	debugfs_create_file(name, S_IRWUG, chub_dbg_get_root_dir(), info,
			    &log_fops);

	list_add_tail(&info->list, &log_list_head);

	print_info = info;
	info->sram_log_buffer = false;
	info->support_log_save = false;

	return info;
}

static struct contexthub_ipc_info *chub_info;
#define SIZE_OF_LOG		(1024)//(240)

void chub_printf(struct device *dev, int level, int fw_idx, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	struct logbuf_output log;
	int n;
	struct contexthub_ipc_info *chub = chub_info;
	char buf[SIZE_OF_LOG];

	memset(&log, 0, sizeof(struct logbuf_output));
	log.timestamp = ktime_get_boot_ns() / 1000;
	log.level = level;
	log.size = fw_idx;
	log.buf = buf;
	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	switch (level) {
	case 'W':
		if (dev)
			dev_warn(dev, "%pV", &vaf);
		else
			pr_warn("nanohub: %pV", &vaf);
		break;
	case 'E':
		if (dev)
			dev_err(dev, "%pV", &vaf);
		else
			pr_err("nanohub: %pV", &vaf);
		break;
	case 'D':
		if (dev)
			dev_dbg(dev, "%pV", &vaf);
		else
			pr_debug("nanohub: %pV", &vaf);
		break;
	default:
		if (dev)
			dev_info(dev, "%pV", &vaf);
		else
			pr_info("nanohub: %pV", &vaf);
		break;
	}
	if (chub) {
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
		if (chub->mlog.memlog_printf_chub) {
			if (dev)
				memlog_write_printf(chub->mlog.memlog_printf_chub,
						    MEMLOG_LEVEL_ERR, "%s: %pV",
						    dev_driver_string(dev), &vaf);
			else
				memlog_write_printf(chub->mlog.memlog_printf_chub,
						    MEMLOG_LEVEL_ERR, "%pV", &vaf);
		}
#endif
		if (dev)
			n = snprintf(log.buf, SIZE_OF_LOG - 1, "%s: %pV", dev_driver_string(dev),  &vaf);
		else
			n = snprintf(log.buf, SIZE_OF_LOG - 1, "%pV", &vaf);
		ipc_logbuf_printf(chub, &log, n - 1);
	}
	va_end(args);
}

int contexthub_log_init(void *chub_p)
{
	int ret;
	struct contexthub_ipc_info *chub = chub_p;

	/* register memlog at 1st to log chub */
	ret = contexthub_memlog_init(chub);
	if (ret) {
		dev_err(chub->dev, "memlog not registered...\n");
		return ret;
	}

	chub->log_info = log_register_buffer(chub->dev, 0,
					     &chub->chub_rt_log,
					     "log", 0);
	chub->chub_rt_log.loglevel = 0;
	INIT_WORK(&chub->log_work, handle_log_work_func);
	chub->log_work_reqcnt = 0;

	/* init fw runtime log */
	chub->chub_rt_log.buffer = vzalloc(SIZE_OF_BUFFER);
	if (!chub->chub_rt_log.buffer)
		return -ENOMEM;

	chub->chub_rt_log.buffer_size = SIZE_OF_BUFFER;
	chub->chub_rt_log.write_index = 0;
	chub->chub_rt_log.wrap = false;
	chub_info = chub;

	return ret;
}
