/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Debug interface between LPD fw and LPD driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "lpd.h"

#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

/*log level for lpd driver, default: 6*/
int lpd_log_level = 6;

/*log level for cm55,  default: 2(Info)*/
u8 lpd_fwlog_level = 2;
static char fwlog_names[] = {
	'E',
	'W',
	'I',
	'D',
};

static int lpd_logbuf_regdump(struct lpd_device *lpd)
{
	struct lpd_fw_dump *dump;
	struct lpd_fw_dump_content *log;
	u32 index = 0;
	u32 len;
	int ret;

	dump = kzalloc(sizeof(struct lpd_fw_dump), GFP_KERNEL);
	ret = lpd_sram_read(lpd, dump, sizeof(struct lpd_fw_dump), lpd->sram.off.dump);
	if (ret < 0)
		goto end;

	if (strncmp(dump->magic, LPD_FW_DUMP_MAGIC, sizeof(dump->magic)) != 0) {
		lpd_err("%s: invalid magic %s\n", __func__, dump->magic);
		goto end;
	}

	while (index < dump->lines) {
		log = &dump->log[index];
		len = strlen((char *)log);

		if (len > 0 && len <= DUMP_SIZE)
			lpd_info("%s",(char *)log->buf);
		else
			lpd_err("%s: size err:%d, idx:%d\n", __func__, len, index);

		index = (index + 1) % DUMP_NUM;
	}
end:
	kfree(dump);
	
	return 0;
}

enum lpd_logbuf_wr {
	LOGBUF_WR_ENQUEUE 	= 1 << 0,
	LOGBUF_WR_DEQUE 	= 1 << 1,
	LOGBUF_WR_FULL		= 1 << 2,
	LOGBUF_WR_LOGLEVEL	= 1 << 3,
	LOGBUF_WR_REGDUMP	= 1 << 4,
};


int lpd_init_logbuf(struct lpd_device *lpd)
{
	int ret;
	size_t size = 0;
	void *buf = NULL;
	unsigned int offset = 0;
	struct lpd_fw_logbuf_header header;
	unsigned int magic_len = 0;

	ret = lpd_sram_read(lpd, &header, sizeof(struct lpd_fw_logbuf_header), lpd->sram.off.logbuf);
	if (ret < 0) {
		lpd_err("ERR:%s sram read fail\n", __func__);
		return ret;
	}

	magic_len = strlen(LPD_FW_LOGBUF_MAGIC);
	if (magic_len > LPD_FW_MAGIC_SIZE) {
		lpd_err("ERR:%s: exceed log buf magic size: %d\n magic_len\n", __func__, magic_len);
		magic_len = LPD_FW_MAGIC_SIZE;
	}

	if (strncmp(header.magic, LPD_FW_LOGBUF_MAGIC, magic_len) != 0) {
		lpd_info("%s: init logbuf header: loglevel: %d->%d\n", __func__, header.loglevel, lpd_fwlog_level);

		buf = &header;
		offset = lpd->sram.off.logbuf;
		size = sizeof(struct lpd_fw_logbuf_header);

		memset(&header, 0, size);
		memcpy(header.magic, LPD_FW_LOGBUF_MAGIC, magic_len);
		header.loglevel = lpd_fwlog_level;
	} else {
		buf = &header.seq_num;
		size = sizeof(header.seq_num);
		offset = lpd->sram.off.logbuf + offsetof(struct lpd_fw_logbuf_header, seq_num);

		header.seq_num = header.seq_num + 1;
	}

	ret = lpd_sram_write(lpd, buf, size, offset);
	if (ret != size) {
		lpd_err("ERR: %s: failed write logbuf data to sram: %d\n", __func__, ret);
		return ret;
	}

	lpd_info("%s: seq_num %d\n", __func__, header.seq_num);
	return 0;
}

int lpd_read_cm55_fault_status(struct lpd_device *lpd)
{
	int ret;
	struct lpd_fw_logbuf_header header;
	unsigned int magic_len;

	if (lpd == NULL) {
		lpd_err("ERR: %s: invalid argument\n", __func__);
		return -EINVAL;
	}

	ret = lpd_sram_read(lpd, &header, sizeof(struct lpd_fw_logbuf_header), lpd->sram.off.logbuf);
	if (ret != sizeof(struct lpd_fw_logbuf_header)) {
		lpd_err("ERR:%s sram read fail\n", __func__);
		return ret;
	}

	magic_len = strlen(LPD_FW_LOGBUF_MAGIC);
	if (magic_len > LPD_FW_MAGIC_SIZE) {
		lpd_err("ERR:%s exceed magic code len: %d\n", __func__, magic_len);
		magic_len = LPD_FW_MAGIC_SIZE;
	}

	if (strncmp(header.magic, LPD_FW_LOGBUF_MAGIC, magic_len) != 0) {
		lpd_err("ERR:%s: invalid magic code\n", __func__);
		return -EINVAL;
	}

	lpd_info("%s: lpd fault status: %x\n", __func__, header.lpd_fault_status);

	if (header.lpd_fault_status != 0) {
		lpd_err("ERR: %s found cm55 fault: %x\n", __func__, header.lpd_fault_status);
		BUG();
	}

	return 0;
}



int lpd_logbuf_outprint(struct lpd_device *lpd)
{
	struct lpd_fw_logbuf_content *log;
	struct lpd_fw_logbuf_header *header;
	u32 eq;
	int ret;
	u32 len;
	u32 changed = 0;
	u32 off;
	struct lpd_fw_logbuf *logbuf;
	unsigned int magic_len;

	NULL_CHECK(lpd);
	NULL_CHECK(lpd->sram_logbuf);

	logbuf = lpd->sram_logbuf;

	mutex_lock(&lpd->d.logbuf_lock);

	ret = lpd_sram_read(lpd, logbuf, sizeof(struct lpd_fw_logbuf), lpd->sram.off.logbuf);
	if (ret < 0)
		goto done;

	magic_len = strlen(LPD_FW_LOGBUF_MAGIC);
	if (magic_len > LPD_FW_MAGIC_SIZE) {
		lpd_err("ERR:%s exceed magic code len: %d\n", __func__, magic_len);
		magic_len = LPD_FW_MAGIC_SIZE;
	}


	header = &logbuf->header;
	if (strncmp(header->magic, LPD_FW_LOGBUF_MAGIC, magic_len) != 0) {
		lpd_err("%s: invalid magic %s eq %d dq %d\n", __func__, header->magic,header->eq,header->dq);
		goto done;
	}

	eq = header->eq;
	if (eq >= LPD_LOGBUF_NUM || header->dq >= LPD_LOGBUF_NUM) {
		lpd_err("%s: index  eq:%d, dq:%d\n", __func__, eq, header->dq);
		header->eq = 0;
		header->dq = 0;
		changed |= LOGBUF_WR_ENQUEUE;
		changed |= LOGBUF_WR_DEQUE;

		goto done;
	}

	if (header->full) {
		header->full = 0;
		header->dq = (eq + 1) % LPD_LOGBUF_NUM;
		changed |= LOGBUF_WR_FULL;
		changed |= LOGBUF_WR_DEQUE;
	}

	if (header->loglevel != lpd_fwlog_level) {
		header->loglevel = lpd_fwlog_level;
		changed |= LOGBUF_WR_LOGLEVEL;
	}

	while (eq != header->dq) {
		log = &logbuf->log[header->dq];
		len = strlen((char *)log);

		if (len > 0 && len <= LOGBUF_BUF_SIZE) {
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
			if (lpd->d.mlog.memlog_printf) {
				memlog_write_printf(lpd->d.mlog.memlog_printf,
							MEMLOG_LEVEL_ERR, "%s", (char *)log->buf);
			}
#endif
			if (log->level || log->timestamp) {
				lpd_info("FW: %02d [%6llu.%06llu]%c %s",
						log->size%100,
					    (log->timestamp) / 1000000,
					    (log->timestamp) % 1000000,
					    fwlog_names[log->level], (char *)log->buf);
			} else {
				lpd_info("FW: %d                 %s",
					    header->dq, (char *)log->buf);
			}
		} else {
			lpd_err("%s: size err:%d, eq:%d, dq:%d\n",
					 __func__, len, eq,
					 header->dq);
		}
		header->dq = (header->dq + 1) % LPD_LOGBUF_NUM;
		changed |= LOGBUF_WR_DEQUE;
	}

	if (header->regdump > 0) {
		lpd_logbuf_regdump(lpd);
		header->regdump = 0;
		changed |= LOGBUF_WR_REGDUMP;
	}

done:
	if (changed && logbuf) {
		if (changed & LOGBUF_WR_ENQUEUE) {
			off = lpd->sram.off.logbuf+offsetof(struct lpd_fw_logbuf_header, eq);
			ret = lpd_sram_write(lpd, &header->eq, sizeof(header->eq), off);
		}

		if (changed & LOGBUF_WR_DEQUE) {
			off = lpd->sram.off.logbuf+offsetof(struct lpd_fw_logbuf_header, dq);
			ret = lpd_sram_write(lpd, &header->dq, sizeof(header->dq), off);
		}

		if (changed & LOGBUF_WR_FULL) {
			off = lpd->sram.off.logbuf+offsetof(struct lpd_fw_logbuf_header, full);
			ret = lpd_sram_write(lpd, &header->full, sizeof(header->full), off);
		}

		if (changed & LOGBUF_WR_LOGLEVEL) {
			off = lpd->sram.off.logbuf+offsetof(struct lpd_fw_logbuf_header, loglevel);
			ret = lpd_sram_write(lpd, &header->loglevel, sizeof(header->loglevel), off);
		}

		if (changed & LOGBUF_WR_REGDUMP) {
			off = lpd->sram.off.logbuf+offsetof(struct lpd_fw_logbuf_header, regdump);
			ret = lpd_sram_write(lpd, &header->regdump, sizeof(header->regdump), off);
		}
		//no return on error
	}

	mutex_unlock(&lpd->d.logbuf_lock);

	return 0;
}

#if TEST_M55_NOTI_AP_STATE
int lpd_logbuf_noti_apstate(struct lpd_device *lpd, u8 apState)
{
	u32 off;
	int ret;

	off = lpd->sram.off.logbuf + offsetof(struct lpd_fw_logbuf_header, apstate);
	ret = lpd_sram_write(lpd, &apState, sizeof(apState), off);

	return ret;
}
#endif

static int lpd_logbuf_thread(void *arg)
{
	struct lpd_device *lpd =(struct lpd_device *)arg;
	NULL_CHECK(lpd);

	lpd_info("%s started sram base %x size %x\n", __func__, lpd->sram.base, lpd->sram.size);
	while (!kthread_should_stop()) {
		lpd_logbuf_outprint(lpd);
		msleep(500);
	}

	return 0;
}

static int lpd_logbuf_start(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);

	lpd_info("%s\n", __func__);

	if (lpd->d.logbuf_thread == NULL)
		lpd->d.logbuf_thread = kthread_run(lpd_logbuf_thread, lpd, "lpd_logbuf_thread");

	return 0;
}

int lpd_logbuf_stop(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);

	lpd_info("%s\n", __func__);

	if (lpd->d.logbuf_thread)
		kthread_stop(lpd->d.logbuf_thread);

	lpd->d.logbuf_thread = NULL;
	return 0;
}

static int lpd_logbuf_enable_show(struct seq_file *s, void *unused)
{
	if (lpd_fwlog_level < sizeof(fwlog_names) / sizeof(fwlog_names[0]))
		seq_printf(s, "log level %c %d\n", fwlog_names[lpd_fwlog_level]);
	else
		seq_printf(s, "log level INVALID\n");

	return 0;
}

static int lpd_logbuf_enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, lpd_logbuf_enable_show, inode->i_private);
}

static ssize_t lpd_logbuf_enable_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	struct seq_file *m = file->private_data;
	struct lpd_device *lpd = m->private;
	char *buf_data;
	int ret;
	int enable;

	NULL_CHECK(lpd);
	if (!count)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &enable);
	if (ret < 0)
		goto out;

	if (enable) {
		lpd_fwlog_level = enable;
		lpd_logbuf_start(lpd);
	} else {
		lpd_logbuf_stop(lpd);
	}

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations lpd_logbuf_enable_fops = {
	.owner = THIS_MODULE,
	.open = lpd_logbuf_enable_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = lpd_logbuf_enable_write,
};

static int lpd_dbg_level_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%u\n", lpd_log_level);

	return 0;
}

static int lpd_dbg_level_open(struct inode *inode, struct file *file)
{
	return single_open(file, lpd_dbg_level_show, inode->i_private);
}

static ssize_t lpd_dbg_level_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;

	if (!count)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &lpd_log_level);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations lpd_dbg_level_fops = {
	.open = lpd_dbg_level_open,
	.write = lpd_dbg_level_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int lpd_dbg_create_debugfs(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);

	lpd->d.debug_root = debugfs_create_dir("lpd", NULL);

	lpd->d.logbuf_enable = debugfs_create_file("logbuf_enable", 0444,
		lpd->d.debug_root, lpd, &lpd_logbuf_enable_fops);

	lpd->d.dbg_level = debugfs_create_file("dbg_level", 0444,
		lpd->d.debug_root, lpd, &lpd_dbg_level_fops);

	return 0;
}

int lpd_dbg_destroy_debugfs(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);

	if (lpd->d.logbuf_enable)
		debugfs_remove(lpd->d.logbuf_enable);

	if (lpd->d.dbg_level)
		debugfs_remove(lpd->d.dbg_level);

	if (lpd->d.debug_root)
		debugfs_remove(lpd->d.debug_root);

	return 0;
}

static int lpd_memlog_init(struct lpd_device *lpd)
{
#ifdef CONFIG_EXYNOS_MEMORY_LOGGER
	int ret;

	NULL_CHECK(lpd);

	ret = memlog_register("LPD", lpd->dev, &lpd->d.mlog.memlog_lpd);
	if (!lpd->d.mlog.memlog_lpd) {
		lpd_err("memlog lpd registration fail ret:%d\n", ret);
		return -1;
	}

	lpd->d.mlog.memlog_printf_file = memlog_alloc_file(lpd->d.mlog.memlog_lpd,
		"log-fil", SZ_512K, SZ_1M, 1000, 3);
	if (lpd->d.mlog.memlog_printf_file) {
		//memlog_obj_set_sysfs_mode(lpd->d.mlog.memlog_printf_file, true);
		lpd->d.mlog.memlog_printf = memlog_alloc_printf(lpd->d.mlog.memlog_lpd, SZ_512K,
			lpd->d.mlog.memlog_printf_file, "log-mem", 0);
	}

	lpd->d.mlog.memlog_sram_file = memlog_alloc_file(lpd->d.mlog.memlog_lpd,
		"srm-fil", lpd->sram.size, lpd->sram.size, 1000, 3);
	if (lpd->d.mlog.memlog_sram_file) {
		lpd->d.mlog.memlog_sram = memlog_alloc_dump(lpd->d.mlog.memlog_lpd, lpd->sram.size,
			lpd->sram.base, false, lpd->d.mlog.memlog_sram_file, "srm-dmp");
	}

	lpd_info("memlog printf %s sram %s\n", lpd->d.mlog.memlog_printf ? "pass" : "fail",
		lpd->d.mlog.memlog_sram ? "pass" : "fail");
#endif
	return 0;
}

int lpd_dbg_init(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);
	mutex_init(&lpd->d.logbuf_lock);
	lpd_dbg_create_debugfs(lpd);
	lpd_memlog_init(lpd);
	lpd->d.pm_refcnt = 0;
	return 0;
}

int lpd_dbg_uninit(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);
	lpd_dbg_destroy_debugfs(lpd);
	mutex_destroy(&lpd->d.logbuf_lock);
	return 0;
}

