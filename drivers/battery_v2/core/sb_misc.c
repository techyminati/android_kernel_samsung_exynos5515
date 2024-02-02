/*
 *  sb_misc.c
 *  Samsung Mobile Battery MISC
 *
 *  Copyright (C) 2022 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/miscdevice.h>

#include "sb_misc.h"

#define misc_log(misc, str, ...)	pr_info("[sb-misc-%s]:%s: "str, misc->md.name, __func__, ##__VA_ARGS__)
#define misc_err(str, ...)			pr_info("[sb-misc-error]:%s: "str, __func__, ##__VA_ARGS__)

struct sb_misc {
	struct miscdevice md;

	atomic_t open_lock;
	atomic_t ioctl_lock;

	misc_func	cb_ioctl;
};

struct sb_misc *get_sb_misc(struct miscdevice *md)
{
	return container_of(md, struct sb_misc, md);
}

static bool is_lock(atomic_t *lock)
{
	return (atomic_read(lock) >= 1);
}

static int misc_open(struct inode *inode, struct file *file)
{
	struct sb_misc *misc;
	int ret = 0;

	if (!file->private_data) {
		misc_err("can not get the miscdevice data by private_data\n");
		return -EINVAL;
	}
	misc = get_sb_misc(file->private_data);

	if (is_lock(&misc->open_lock)) {
		misc_err("busy\n");
		return -EBUSY;
	}
	atomic_inc(&misc->open_lock);

	return ret;
}

static int misc_close(struct inode *inode, struct file *file)
{
	struct sb_misc *misc;

	if (!file->private_data) {
		misc_err("can not get the miscdevice data by private_data\n");
		return -EINVAL;
	}
	misc = get_sb_misc(file->private_data);

	if (is_lock(&misc->open_lock))
		atomic_dec(&misc->open_lock);

	return 0;
}

static long misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct sb_misc *misc;
	int ret = 0;

	if (!file->private_data) {
		misc_err("can not get the miscdevice data by private_data\n");
		return -EINVAL;
	}
	misc = get_sb_misc(file->private_data);

	if (is_lock(&misc->ioctl_lock)) {
		misc_err("busy\n");
		return -EBUSY;
	}

	misc_log(misc, "cmd = 0x%x\n", cmd);
	if (misc->cb_ioctl) {
		atomic_inc(&misc->ioctl_lock);
		ret = misc->cb_ioctl(misc->md.parent, cmd, arg);
		atomic_dec(&misc->ioctl_lock);
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long
misc_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	ret = misc_ioctl(file, cmd, arg);

	return ret;
}
#endif

static const struct file_operations misc_fops = {
	.owner		= THIS_MODULE,
	.open		= misc_open,
	.release	= misc_close,
	.llseek		= no_llseek,
	.unlocked_ioctl = misc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = misc_compat_ioctl,
#endif
};

struct sb_misc *sb_misc_init(struct device *dev, const char *path, misc_func cb_ioctl)
{
	struct sb_misc *misc;
	int ret = 0;

	if ((dev == NULL) || (path == NULL))
		return ERR_PTR(-EINVAL);

	misc = kzalloc(sizeof(struct sb_misc), GFP_KERNEL);
	if (!misc)
		return ERR_PTR(-ENOMEM);

	/* set miscdevice */
	misc->md.minor = MISC_DYNAMIC_MINOR;
	misc->md.name = path;
	misc->md.fops = &misc_fops;
	misc->md.parent = dev;

	ret = misc_register(&misc->md);
	if (ret) {
		misc_err("register fail (ret = %d)\n", ret);
		goto reg_fail;
	}

	/* set values */
	misc->cb_ioctl = cb_ioctl;

	/* set atomic */
	atomic_set(&misc->open_lock, 0);
	atomic_set(&misc->ioctl_lock, 0);

	return misc;

reg_fail:
	kfree(misc);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(sb_misc_init);

int sb_misc_exit(struct sb_misc *misc)
{
	if (misc == NULL)
		return -EINVAL;

	misc_deregister(&misc->md);
	kfree(misc);

	return 0;
}
EXPORT_SYMBOL(sb_misc_exit);
