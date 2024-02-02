/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * character device driver for User Application
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "lpd.h"

#include <linux/mm.h>
#include <linux/uaccess.h>
#include <soc/samsung/exynos-lpd.h>


static int check_meta_version(struct lpd_meta_info *meta_data)
{
	int ret = 0;

	NULL_CHECK(meta_data);

	if (meta_data->lpd_metadata_version != LPD_METADATA_API_VERSION) {
		lpd_warn("LPD meta version mismatch %d.%d.%d vs %d.%d.%d\n",
			META_VERSION_MAJOR(meta_data->lpd_metadata_version),
			META_VERSION_MINOR(meta_data->lpd_metadata_version),
			META_VERSION_REVISION(meta_data->lpd_metadata_version),
			META_VERSION_MAJOR(LPD_METADATA_API_VERSION),
			META_VERSION_MINOR(LPD_METADATA_API_VERSION),
			META_VERSION_REVISION(LPD_METADATA_API_VERSION));
	} else {
		lpd_info("LPD meta version %d.%d.%d\n",
			META_VERSION_MAJOR(meta_data->lpd_metadata_version),
			META_VERSION_MINOR(meta_data->lpd_metadata_version),
			META_VERSION_REVISION(meta_data->lpd_metadata_version));
	}
	return ret;
}

static int request_lpd_stop(struct lpd_device *lpd)
{
	int ret = 0;

	NULL_CHECK(lpd);

	mutex_lock(&lpd->op_lock);

	if (lpd->dpu_state != DPU_STATE_DOZE_SUSPEND) {
		lpd_err("%s wrong dpu state: %d\n", __func__, lpd->dpu_state);
		ret = -EINVAL;
		goto err_stop;
	}

	if (lpd->state != LPD_STATE_START_END) {
		lpd_info("%s wrong lpd state: %d\n", __func__, lpd->state);
		ret = -EINVAL;
		goto err_stop;
	}

	lpd_notifier_call(LPD_NOTIFIER_STOP, NULL);
	lpd_notifier_call(LPD_NOTIFIER_STOP_POST, NULL);

err_stop:
	mutex_unlock(&lpd->op_lock);

	return ret;
}


static int request_lpd_start(struct lpd_device *lpd)
{
	int ret = 0;

	NULL_CHECK(lpd);

	mutex_lock(&lpd->op_lock);

	if (lpd->dpu_state != DPU_STATE_DOZE_SUSPEND) {
		lpd_err("%s wrong dpu state: %d\n", __func__, lpd->dpu_state);
		ret = -EINVAL;
		goto err_start;
	}

	if (lpd->state != LPD_STATE_PM_RELEASE) {
		lpd_info("%s wrong lpd state: %d\n", __func__, lpd->state);
		ret = -EINVAL;
		goto err_start;
	}

	//lpd_notifier_call(LPD_NOTIFIER_CONFIG, &decon->lpd_dpu);
	lpd_notifier_call(LPD_NOTIFIER_START_PRE, NULL);
	lpd_notifier_call(LPD_NOTIFIER_START, NULL);

err_start:
	mutex_unlock(&lpd->op_lock);

	return ret;
}


static int ioctl_set_meta_data(struct lpd_device *lpd, void __user *argp)
{
	int ret = 0;
	struct lpd_meta_info meta_data;

	NULL_CHECK(lpd);
	NULL_CHECK(argp);

	if (IS_LPD_START_STATE(lpd)) {
		lpd_err("%s not allowed. should stop first\n", __func__);
		return -1;
	}

	ret = copy_from_user(&meta_data, argp, sizeof(struct lpd_meta_info));
	if (ret) {
		lpd_err("ERR:%s: failed to copy_from_user\n", __func__);
		goto set_err;
	}

	check_meta_version(&meta_data);

	lpd->enable = meta_data.enable;
	lpd_info("%s: meta_data->enable: %d\n", __func__, meta_data.enable);

	if (meta_data.enable == false) {
		lpd_info("%s: lpd disabled\n", __func__);
		lpd_set_state(lpd, LPD_STATE_INIT);
		return 0;
	}

#if defined(LPD_SRAM_POWER_GATING)
	memcpy(&lpd->img_desc.meta, &meta_data, sizeof(struct lpd_meta_info));
#else
	ret = lpd_update_meta_data(lpd, &meta_data);
	if (ret != sizeof(lpd_meta_info)) {
		lpd_err("ERR:%s: failed to update meta data: %d\n", __func__, ret);
		goto set_err;
	}
#endif
	lpd_set_state(lpd, LPD_STATE_PREPARE);

	return ret;

set_err:
	lpd_set_state(lpd, LPD_STATE_INIT);

	return ret;

}

static int ioctl_set_sensor_data(struct lpd_device *lpd, void __user *argp)
{
	int ret = 0;
	struct lpd_sensor_data sensor_data;

	NULL_CHECK(lpd);
	NULL_CHECK(argp);

	if (IS_LPD_START_STATE(lpd)) {
		lpd_err("%s not allowed. should stop first\n", __func__);
		return -1;
	}

	ret = copy_from_user(&sensor_data, argp, sizeof(struct lpd_sensor_data));
	if (ret) {
		lpd_err("ERR:%s: failed to copy_from_user\n", __func__);
		return ret;
	}
#if defined(LPD_SRAM_POWER_GATING)
	memcpy(&lpd->img_desc.sensor, &sensor_data, sizeof(struct lpd_sensor_data));
#else
	ret = lpd_update_sensor_data(lpd, &sensor_data);
	if (ret != sizeof(struct lpd_sensor_data)) {
		lpd_err("ERR:%s: failed to update sensor data: %d\n", ret);
		return -EINVAL;
	}
#endif

	return 0;
}

static int ioctl_prepare_update_meta(struct lpd_device *lpd)
{
	int ret = 0;

	NULL_CHECK(lpd);

	lpd_info("%s: state: %d, req: %d\n", __func__, lpd->state, lpd->meta_update_req);
	ret = request_lpd_stop(lpd);
	if (ret) {
		lpd_err("%s: faield to request stop lpd\n", __func__);
		return ret;
	}

	lpd->meta_update_req = UPDATE_REQ_PREPARED;
	return ret;
}

static int ioctl_update_meta(struct lpd_device *lpd, void __user *argp)
{
	int ret = 0;
	struct lpd_meta_info meta_data;

	NULL_CHECK(lpd);
	NULL_CHECK(argp);

	if (lpd->meta_update_req != UPDATE_REQ_PREPARED) {
		lpd_err("%s req_meta_update was not set\n", __func__);
		return -EINVAL;
	}

	if (IS_LPD_START_STATE(lpd)) {
		lpd_err("%s already lpd start status\n", __func__);
		return ret;
	}

	ret = copy_from_user(&meta_data, argp, sizeof(struct lpd_meta_info));
	if (ret) {
		lpd_err("ERR:%s: failed to copy_from_user\n", __func__);
		goto update_err;
	}

	check_meta_version(&meta_data);

	lpd->enable = meta_data.enable;
	lpd_info("%s: meta_data->enable: %d\n", __func__, meta_data.enable);

	if (meta_data.enable == false) {
		lpd_info("%s: lpd disabled current state: %d\n", __func__, lpd->state);
		if (lpd->state == LPD_STATE_PREPARE)
			lpd_set_state(lpd, LPD_STATE_INIT);

		return 0;
	}
#if defined(LPD_SRAM_POWER_GATING)
	memcpy(&lpd->img_desc.meta, &meta_data, sizeof(struct lpd_meta_info));
#else
	ret = lpd_update_meta_data(lpd, &meta_data);
	if (ret != sizeof(lpd_meta_info)) {
		lpd_err("ERR:%s: failed to update meta data: %d\n", __func__, ret);
		goto update_err;
	}
#endif
	ret = request_lpd_start(lpd);
	if (ret < 0) {
		lpd_err("ERR:%s : failed to request lpd start\n", __func__, ret);
		goto update_err;
	}

	lpd->meta_update_req = UPDATE_REQ_DONE;

	return ret;

update_err:
	lpd_set_state(lpd, LPD_STATE_INIT);

	return ret;

}


static long lpd_dev_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg_)
{
	int ret = 0;
	void __user *argp = (void __user *)arg_;
	struct lpd_device *lpd;

	NULL_CHECK(file);
	lpd = file->private_data;
	NULL_CHECK(lpd);

	switch (cmd) {

		case LPD_IOCTL_SET_META_DATA:
			lpd_dbg("%s: SET_LPD_META_DATA\n", __func__);
			ret = ioctl_set_meta_data(lpd, argp);
			if (ret) {
				lpd_err("ERR:%s:failed to ioctl_set_meta_data\n", __func__);
				goto exit_ioctl;
			}
			break;

		case LPD_IOCTL_SET_SENSOR_DATA:
			lpd_dbg("%s: SET_LPD_SENSOR_DATA\n", __func__);
			ret = ioctl_set_sensor_data(lpd, argp);
			if (ret) {
				lpd_err("ERR:%s:failed to ioctl_set_sensor_data\n", __func__);
				goto exit_ioctl;
			}
			break;

		case LPD_IOCTL_PREPARE_UPDATE_META_DATA:
			lpd_info("%s: LPD_IOCTL_PREPARE_UPDATE_META_DATA\n", __func__);
			ret = ioctl_prepare_update_meta(lpd);
			if (ret) {
				lpd_err("ERR:%s:failed to ioctl_prepare_update_meta\n", __func__);
				goto exit_ioctl;
			}
			break;

		case LPD_IOCTL_UPDATE_META_DATA:
			lpd_info("%s: LPD_IOCTL_UPDATE_META_DATA\n", __func__);
			ret = ioctl_update_meta(lpd, argp);
			if (ret) {
				lpd_err("ERR:%s:failed to ioctl_update_meta\n", __func__);
				goto exit_ioctl;
			}
			break;

		default:
			lpd_err("undefined IOCTL %d\n",cmd);
			break;
	}
exit_ioctl:

	return ret;
}

#ifdef CONFIG_COMPAT
static long lpd_dev_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	arg = (unsigned long) compat_ptr(arg);
	return lpd_dev_ioctl(file, cmd, arg);
}
#endif

static int lpd_dev_open(struct inode *inode, struct file *file)
{
    struct lpd_device *lpd;

	NULL_CHECK(inode);
	NULL_CHECK(file);

    lpd = container_of(inode->i_cdev, struct lpd_device, chardev.cdev);

    file->private_data = lpd;

	return 0;
}

static ssize_t lpd_dev_read(struct file *file, char *buffer,
								size_t length, loff_t *offset)
{
	return 0;
}

static ssize_t lpd_dev_write(struct file *file, const char *buffer,
			     size_t length, loff_t *offset)
{
	int8_t num_os;
	int ret;

	/* read int8_t num_os */
	ret = copy_from_user(&num_os, buffer, sizeof(num_os));

	return 0;
}

static int lpd_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;
    struct lpd_device *lpd;
#if (TEST_DRAM_ACCESS==1) /* Uses DMA */
	int ret_pfn;
#else
	unsigned long off, user_size, kernel_size;
#endif

	NULL_CHECK(file);
	lpd = file->private_data;
	NULL_CHECK(lpd);
#if (TEST_DRAM_ACCESS==1)  /* Uses DMA */
	ret = dma_mmap_from_dev_coherent(lpd->dev, vma, lpd->rmem.io_base,
			lpd->rmem.size, &ret_pfn);
	if(ret == 1 && ret_pfn >=0)
		ret = 0;
	else {
		lpd_err("no dev coherent memory ret %d ret_pfn %d\n", ret, ret_pfn);
		ret = -1;
	}
#else
	off = vma->vm_pgoff << PAGE_SHIFT;
	user_size = vma->vm_end - vma->vm_start;
	kernel_size = lpd->rmem.size;

	if (off >= kernel_size || user_size > (kernel_size - off)){
		lpd_err("## off %x user size %x kernel size %x\n",
			off, user_size, kernel_size);
		return -ENXIO;
	}

	ret = remap_pfn_range(vma, vma->vm_start,
			__phys_to_pfn(lpd->rmem.base) + vma->vm_pgoff,
			user_size,
			pgprot_writecombine(vma->vm_page_prot));
#endif
	return ret;
}

static const struct file_operations lpd_dev_fileops = {
	.owner = THIS_MODULE,
	.open = lpd_dev_open,
	.read = lpd_dev_read,
	.write = lpd_dev_write,
	.unlocked_ioctl = lpd_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= lpd_dev_compat_ioctl,
#endif
	.mmap	= lpd_dev_mmap,
};

int lpd_dev_init(struct lpd_device *lpd)
{
	int ret = 0;
	struct lpd_chardev_info *chardev;

	NULL_CHECK(lpd);
	chardev = &lpd->chardev;

	ret = alloc_chrdev_region(&chardev->devid, 0, 1, CDEV_NAME);
	if (ret < 0) {
		lpd_err("failed to alloc lpd dev: %d\n", ret);
		goto err;
	}
	cdev_init(&lpd->chardev.cdev, &lpd_dev_fileops);
	cdev_add(&lpd->chardev.cdev, chardev->devid, 1);

	chardev->class = class_create( THIS_MODULE, CDEV_NAME );
	if (IS_ERR(chardev->class)) {
		ret = PTR_ERR(chardev->class);
		lpd_err("failed to create lpd class: %d\n", ret);
		goto err;
	}

	chardev->csdev = device_create(chardev->class, NULL, chardev->devid, NULL, CDEV_NAME);
	if (IS_ERR(chardev->csdev)) {
		ret = PTR_ERR(chardev->csdev);
		lpd_err("failed to create lpd device: %d\n", ret);
		goto err;
	}

	return ret;

err:
	if (chardev->class)
		class_destroy(chardev->class);

	if (chardev->devid) {
		cdev_del(&lpd->chardev.cdev);
		unregister_chrdev_region(chardev->devid, 1);
	}

	return ret;
}

int lpd_dev_uninit(struct lpd_device *lpd)
{
	struct lpd_chardev_info *chardev;

	NULL_CHECK(lpd);
	chardev = &lpd->chardev;

	device_destroy(chardev->class, chardev->devid);
	class_destroy(chardev->class);
	cdev_del(&lpd->chardev.cdev);
	unregister_chrdev_region(chardev->devid, 1);
	return 0;
}

