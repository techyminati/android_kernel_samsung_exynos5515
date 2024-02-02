/*
 * sec vibrator driver for common code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "[VIB] " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/vibrator/sec_vibrator.h>
#include <linux/vibrator/sec_vibrator_notifier.h>

enum {
	VIB_NO_COMPEMSATION = 0,
	VIB_COMPENSATION_WAY1,
	VIB_COMPENSATION_WAY2,
};

static int sec_vibrator_set_frequency(struct sec_vibrator_drvdata *ddata,
	int frequency)
{
	int ret = 0;

	if (!ddata->vib_ops->set_frequency)
		return -ENOSYS;

	ret = ddata->vib_ops->set_frequency(ddata->dev, frequency);

	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

static int sec_vibrator_set_intensity(struct sec_vibrator_drvdata *ddata,
	int intensity)
{
	int ret = 0;

	if (!ddata->vib_ops->set_intensity)
		return -ENOSYS;

	if (intensity < -(MAX_INTENSITY) || MAX_INTENSITY < intensity) {
		pr_err("%s out of range(%d)\n", __func__, intensity);
		return -EINVAL;
	}

	ret = ddata->vib_ops->set_intensity(ddata->dev, intensity);

	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

static int sec_vibrator_set_enable(struct sec_vibrator_drvdata *ddata, bool en)
{
	int ret = 0;

	if (!ddata->vib_ops->enable) {
		pr_err("%s cannot supported\n", __func__);
		return -ENOSYS;
	}

	ret = ddata->vib_ops->enable(ddata->dev, en);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	sec_vib_notifier_notify(en, ddata);

	return ret;
}

static int sec_vibrator_set_overdrive(struct sec_vibrator_drvdata *ddata,
		bool en)
{
	int ret = 0;
	pr_info("%s\n", __func__);
	if (!ddata->vib_ops->set_overdrive)
		return -ENOSYS;

	ret = ddata->vib_ops->set_overdrive(ddata->dev, en);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

static void sec_vibrator_haptic_enable(struct sec_vibrator_drvdata *ddata)
{
	pr_info("%s\n", __func__);
	sec_vibrator_set_enable(ddata, true);

	if (ddata->vib_ops->set_frequency)
		pr_info("freq:%d, intensity:%d, %dms\n", ddata->frequency,
				ddata->intensity, ddata->timeout);
	else if (ddata->vib_ops->set_intensity)
		pr_info("intensity:%d, %dms\n", ddata->intensity,
				ddata->timeout);
	else
		pr_info("%dms\n", ddata->timeout);
}

static void sec_vibrator_haptic_disable(struct sec_vibrator_drvdata *ddata)
{
	pr_info("%s\n", __func__);
	/* clear haptic engine variables */
	ddata->f_packet_en = false;
	ddata->packet_cnt = 0;
	ddata->packet_size = 0;

	sec_vibrator_set_enable(ddata, false);

	if (ddata->timeout > 0)
		pr_info("timeout, off\n");
	else
		pr_info("off\n");
	__pm_relax(ddata->vib_ws);
}

static void sec_vibrator_engine_run_packet(struct sec_vibrator_drvdata *ddata,
		struct vib_packet packet)
{
	int frequency = packet.freq;
	int intensity = packet.intensity;
	int overdrive = packet.overdrive;
	int fifo =  packet.fifo_flag;

	if (fifo == 0)
		pr_info("%s [%d] freq:%d, intensity:%d, time:%d overdrive: %d\n",
		__func__, ddata->packet_cnt, frequency, intensity,
		ddata->timeout, overdrive);
	else
		pr_info("%s [%d] fifo num:%d, intensity:%d, time:%d overdrive: %d\n",
		__func__, ddata->packet_cnt, frequency, intensity,
		ddata->timeout, overdrive);

	if (!ddata->f_packet_en) {
		pr_err("haptic packet is empty\n");
		return;
	}

	sec_vibrator_set_overdrive(ddata, overdrive);
		
	if (intensity) {
		if (fifo == 0) {
			if (frequency == 0)
				frequency = ddata->alert_freq;
			if (ddata->frequency != frequency) {
				ddata->frequency = frequency;
				sec_vibrator_set_frequency(ddata, frequency);
			}
			sec_vibrator_set_intensity(ddata, intensity);
		} else
			ddata->vib_ops->set_fifo_intensity(ddata->dev, intensity);

		pr_info("[haptic engine] motor run\n");
		if (fifo == 0)
			sec_vibrator_set_enable(ddata, true);
		else {
			sec_vibrator_set_enable(ddata, false);
			ddata->vib_ops->enable_fifo(ddata->dev, frequency);
		}
	} else {
		pr_info("[haptic engine] motor stop\n");
		sec_vibrator_set_enable(ddata, false);
		sec_vibrator_set_intensity(ddata, intensity);
	}
	pr_info("%s end\n", __func__);
}

static void timed_output_enable(struct sec_vibrator_drvdata *ddata, unsigned int value)
{
	struct hrtimer *timer = &ddata->timer;
	int ret = 0;

	pr_info("%s\n", __func__);
	kthread_flush_worker(&ddata->kworker);
	ret = hrtimer_cancel(timer);
	mutex_lock(&ddata->vib_mutex);

	value = min_t(int, value, MAX_TIMEOUT);
	ddata->timeout = value;
	if (value) {
		__pm_stay_awake(ddata->vib_ws);
		if (ddata->f_packet_en) {
			hrtimer_start(timer, 0,	HRTIMER_MODE_REL);
		} else {
			sec_vibrator_haptic_enable(ddata);
			hrtimer_start(timer, ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC), HRTIMER_MODE_REL);
		}
	} else {
		sec_vibrator_haptic_disable(ddata);
	}
	mutex_unlock(&ddata->vib_mutex);
}

static enum hrtimer_restart haptic_timer_func(struct hrtimer *timer)
{
	struct sec_vibrator_drvdata *ddata
		= container_of(timer, struct sec_vibrator_drvdata, timer);

	pr_info("%s\n", __func__);
	kthread_queue_work(&ddata->kworker, &ddata->kwork);
	return HRTIMER_NORESTART;
}

static void sec_vibrator_work(struct kthread_work *work)
{
	struct sec_vibrator_drvdata *ddata
		= container_of(work, struct sec_vibrator_drvdata, kwork);
	struct hrtimer *timer = &ddata->timer;
	int fifo = 0;

	pr_info("%s\n", __func__);
	mutex_lock(&ddata->vib_mutex);
	if (ddata->f_packet_en) {
		if (ddata->packet_cnt < ddata->packet_size) {
			ddata->timeout = ddata->vib_pac[ddata->packet_cnt].time;
			fifo = ddata->vib_pac[ddata->packet_cnt].fifo_flag;
			if (fifo == 0) {
				sec_vibrator_engine_run_packet(ddata,
						ddata->vib_pac[ddata->packet_cnt]);
				hrtimer_start(timer, ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC),
						HRTIMER_MODE_REL);
			} else {
				hrtimer_start(timer, ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC),
						HRTIMER_MODE_REL);
				sec_vibrator_engine_run_packet(ddata,
						ddata->vib_pac[ddata->packet_cnt]);
			}
			ddata->packet_cnt++;
			goto unlock_without_vib_off;
		} else {
			ddata->f_packet_en = false;
			ddata->packet_cnt = 0;
			ddata->packet_size = 0;
		}
	}
	sec_vibrator_haptic_disable(ddata);

unlock_without_vib_off:
	mutex_unlock(&ddata->vib_mutex);
}

static ssize_t intensity_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int intensity = 0, ret = 0;

	ret = kstrtoint(buf, 0, &intensity);
	if (ret) {
		pr_err("fail to get intensity\n");
		return -EINVAL;
	}

	if ((intensity < 0) || (intensity > MAX_INTENSITY)) {
		pr_err("[VIB]: %s out of range\n", __func__);
		return -EINVAL;
	}

	ddata->intensity = intensity;
	sec_vibrator_set_intensity(ddata, ddata->intensity);

	return count;
}

static ssize_t intensity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"intensity: %u\n", ddata->intensity);
}

static ssize_t fifo_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int ret;
	int file_num;

	pr_info("%s +\n", __func__);

	ret = kstrtoint(buf, 0, &file_num);
	if (ret) {
		pr_err("fail to get file_num\n");
		return -EINVAL;
	}

	ret = ddata->vib_ops->set_fifo_intensity(ddata->dev, 10000);
	ret = ddata->vib_ops->enable_fifo(ddata->dev, file_num);

	pr_info("%s -\n", __func__);
	return count;
}

static ssize_t fifo_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int ret = 0;

	if (!ddata->vib_ops->get_fifo_filepath)
		return snprintf(buf, VIB_BUFSIZE, "NONE\n");

	ret = ddata->vib_ops->get_fifo_filepath(ddata->dev, buf);

	return ret;
}

static ssize_t multi_freq_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int num, ret;

	ret = kstrtoint(buf, 0, &num);
	if (ret) {
		pr_err("fail to get frequency\n");
		return -EINVAL;
	}

	if (num == 0)
		ddata->frequency = ddata->alert_freq;
	else
		ddata->frequency = num;
	sec_vibrator_set_frequency(ddata, ddata->frequency);

	return count;
}

static ssize_t multi_freq_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"frequency: %d\n", ddata->frequency);
}

// TODO: need to update
static ssize_t haptic_engine_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int i = 0, _data = 0, tmp = 0;

	if (sscanf(buf, "%6d", &_data) != 1)
		return count;

	if (_data > PACKET_MAX_SIZE * VIB_PACKET_MAX)
		pr_info("%s, [%d] packet size over\n", __func__, _data);
	else {
		ddata->packet_size = _data / VIB_PACKET_MAX;
		ddata->packet_cnt = 0;
		ddata->f_packet_en = true;

		buf = strstr(buf, " ");

		for (i = 0; i < ddata->packet_size; i++) {
			for (tmp = 0; tmp < VIB_PACKET_MAX; tmp++) {
				if (buf == NULL) {
					pr_err("%s, buf is NULL, Please check packet data again\n",
							__func__);
					ddata->f_packet_en = false;
					return count;
				}

				if (sscanf(buf++, "%6d", &_data) != 1) {
					pr_err("%s, packet data error, Please check packet data again\n",
							__func__);
					ddata->f_packet_en = false;
					return count;
				}

				switch (tmp) {
				case VIB_PACKET_TIME:
					ddata->vib_pac[i].time = _data;
					break;
				case VIB_PACKET_INTENSITY:
					ddata->vib_pac[i].intensity = _data;
					break;
				case VIB_PACKET_FREQUENCY:
					ddata->vib_pac[i].freq = _data;
					break;
				case VIB_PACKET_OVERDRIVE:
					ddata->vib_pac[i].overdrive = _data;
					break;
				}
				buf = strstr(buf, " ");
			}
		}
	}

	return count;
}

static ssize_t haptic_engine_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int i = 0;
	size_t size = 0;

	for (i = 0; i < ddata->packet_size && ddata->f_packet_en &&
			((4 * VIB_BUFSIZE + size) < PAGE_SIZE); i++) {
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].time);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].intensity);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].freq);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].overdrive);
	}

	return size;
}

static ssize_t hybrid_haptic_engine_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int i = 0, _data = 0, tmp = 0, check_fifo = 0, origin_time = 0;
	int way1_cmp_sum = 0, way2_cmp_sum = 0;
	int normal_pac_time_sum = 0;
	int compensation_way = VIB_NO_COMPEMSATION;

	pr_info("%s +\n", __func__);
	if (sscanf(buf, "%6d", &_data) != 1)
		return count;

	if (_data > PACKET_MAX_SIZE * VIB_PACKET_MAX)
		pr_info("%s, [%d] packet size over\n", __func__, _data);
	else {
		ddata->packet_size = _data / VIB_PACKET_MAX;
		ddata->packet_cnt = 0;
		ddata->f_packet_en = true;

		buf = strstr(buf, " ");

		for (i = 0; i < ddata->packet_size; i++) {
			check_fifo = 0;
			for (tmp = 0; tmp < VIB_PACKET_MAX; tmp++) {
				if (buf == NULL) {
					pr_err("%s, buf is NULL, Please check packet data again\n",
							__func__);
					ddata->f_packet_en = false;
					return count;
				}

				if (*(buf+1) == '#'){
					buf=buf+2;
					if (sscanf(buf++, "%6d", &_data) == 1) {
						check_fifo = 1;
					}
				}

				else if (sscanf(buf++, "%6d", &_data) != 1) {
					pr_err("%s, packet data error, Please check packet data again\n",
							__func__);
					ddata->f_packet_en = false;
					return count;
				}

				switch (tmp) {
				case VIB_PACKET_TIME:
					ddata->vib_pac[i].time = _data;
					break;
				case VIB_PACKET_INTENSITY:
					ddata->vib_pac[i].intensity = _data;
					break;
				case VIB_PACKET_FREQUENCY:
					ddata->vib_pac[i].freq = _data;
					if (check_fifo == 0) {
						ddata->vib_pac[i].fifo_flag = 0;
						if (ddata->time_compensation && ddata->max_delay_ms) {
							origin_time = ddata->vib_pac[i].time;
							normal_pac_time_sum += origin_time;
							way1_cmp_sum += (origin_time*ddata->time_compensation)/100;
							way2_cmp_sum += ddata->max_delay_ms;
						}
					} else
						ddata->vib_pac[i].fifo_flag = 1;
					break;
				case VIB_PACKET_OVERDRIVE:
					ddata->vib_pac[i].overdrive = _data;
					break;
				}
				buf = strstr(buf, " ");
			}
		}

		if (ddata->time_compensation == 0 || way2_cmp_sum <= ddata->max_delay_ms) {
			compensation_way = VIB_NO_COMPEMSATION;
		} else if (way1_cmp_sum < way2_cmp_sum) {
			compensation_way = VIB_COMPENSATION_WAY1;
			pr_info("%s compensation way1:way1_cmp_sum=%d\n",
				__func__, way1_cmp_sum);
		} else {
			compensation_way = VIB_COMPENSATION_WAY2;
			pr_info("%s compensation way2:way2_cmp_sum=%d,normal_pac_time_sum=%d\n",
				__func__, way2_cmp_sum, normal_pac_time_sum);
		}
		
		for (i = 0; i < ddata->packet_size; i++) {
			if (ddata->vib_pac[i].fifo_flag == 1) {
				pr_info("%s i=%d, file=%d time=%d intensity=%d\n", __func__, i,
					ddata->vib_pac[i].freq,
					ddata->vib_pac[i].time,
					ddata->vib_pac[i].intensity);
				continue;
			}
			origin_time = ddata->vib_pac[i].time;
			if (compensation_way == VIB_COMPENSATION_WAY1)
				ddata->vib_pac[i].time -= (origin_time*ddata->time_compensation)/100;
			else if (compensation_way == VIB_COMPENSATION_WAY2)
				ddata->vib_pac[i].time -= (way2_cmp_sum*origin_time)/normal_pac_time_sum;

			pr_info("%s i=%d, origin_time=%d->time=%d intensity=%d freq=%d\n",
				__func__, i, origin_time,
				ddata->vib_pac[i].time,
				ddata->vib_pac[i].intensity,
				ddata->vib_pac[i].freq);
		}
	}
	pr_info("%s -\n", __func__);
	return count;
}

static ssize_t hybrid_haptic_engine_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int i = 0;
	size_t size = 0;

	for (i = 0; i < ddata->packet_size && ddata->f_packet_en &&
			((4 * VIB_BUFSIZE + size) < PAGE_SIZE); i++) {
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].time);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].intensity);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].freq);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].overdrive);
	}

	return size;
}

static ssize_t enable_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	struct hrtimer *timer = &ddata->timer;
	int remaining = 0;

	if (hrtimer_active(timer)) {
		ktime_t remain = hrtimer_get_remaining(timer);
		struct timeval t = ktime_to_timeval(remain);

		remaining = t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	return sprintf(buf, "%d\n", remaining);
}

static ssize_t enable_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	
	int value;
	int ret;

	ret = kstrtoint(buf, 0, &value);
	if (ret != 0)
		return -EINVAL;

	timed_output_enable(ddata, value);
	return size;
}

static ssize_t motor_type_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct sec_vibrator_drvdata *ddata = (struct sec_vibrator_drvdata *)
		dev_get_drvdata(dev);
	int ret = 0;

	if (!ddata->vib_ops->get_motor_type)
		return snprintf(buf, VIB_BUFSIZE, "NONE\n");

	ret = ddata->vib_ops->get_motor_type(ddata->dev, buf);

	return ret;
}

static ssize_t motor_type_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
	/* do nothing */
	return 0;
}

static DEVICE_ATTR_RW(hybrid_haptic_engine);
static DEVICE_ATTR_RW(haptic_engine);
static DEVICE_ATTR_RW(multi_freq);
static DEVICE_ATTR_RW(intensity);
static DEVICE_ATTR_RW(enable);
static DEVICE_ATTR_RW(motor_type);
static DEVICE_ATTR_RW(fifo);


static struct attribute *sec_vibrator_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_motor_type.attr,
	&dev_attr_fifo.attr,
	&dev_attr_hybrid_haptic_engine.attr,
	NULL,
};

static struct attribute_group sec_vibrator_attr_group = {
	.attrs = sec_vibrator_attributes,
};

static struct attribute *multi_freq_attributes[] = {
	&dev_attr_haptic_engine.attr,
	&dev_attr_multi_freq.attr,
	NULL,
};

static struct attribute_group multi_freq_attr_group = {
	.attrs = multi_freq_attributes,
};

int sec_vibrator_register(struct sec_vibrator_drvdata *ddata)
{
	struct task_struct *kworker_task;
	int ret = 0;

	if (!ddata) {
		pr_err("%s no ddata\n", __func__);
		return  -ENODEV;
	}

	mutex_init(&ddata->vib_mutex);
	kthread_init_worker(&ddata->kworker);
	kworker_task = kthread_run(kthread_worker_fn,
		   &ddata->kworker, "sec_vibrator");
	if (IS_ERR(kworker_task)) {
		pr_err("Failed to create message pump task\n");
		ret = -ENOMEM;
		goto err_kthread;
	}

	kthread_init_work(&ddata->kwork, sec_vibrator_work);
	hrtimer_init(&ddata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->timer.function = haptic_timer_func;
	ddata->vib_ws= wakeup_source_register(NULL, "vibrator_wlock");

	/* create /sys/class/timed_output/vibrator */
	ddata->to_class = class_create(THIS_MODULE, "timed_output");
	if (IS_ERR(ddata->to_class)) {
		ret = PTR_ERR(ddata->to_class);
		goto err_sysfs3;
	}
	ddata->to_dev = device_create(ddata->to_class, NULL,
		MKDEV(0, 0), ddata, "vibrator");
	if (IS_ERR(ddata->to_dev)) {
		ret = PTR_ERR(ddata->to_dev);
		goto err_device_create;
	}

	dev_set_drvdata(ddata->to_dev, ddata);

	ret = sysfs_create_group(&ddata->to_dev->kobj,
			&sec_vibrator_attr_group);
	if (ret) {
		ret = -ENODEV;
		pr_err("Failed to create sysfs1 %d\n", ret);
		goto err_sysfs1;
	}

	if (ddata->vib_ops->set_intensity) {
		ret = sysfs_create_file(&ddata->to_dev->kobj,
				&dev_attr_intensity.attr);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs2 %d\n", ret);
			goto err_sysfs2;
		}

		ddata->intensity = MAX_INTENSITY;
	}


	if (ddata->vib_ops->set_frequency) {
		ret = sysfs_create_group(&ddata->to_dev->kobj,
				&multi_freq_attr_group);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs4 %d\n", ret);
			goto err_sysfs3;
		}
	}
	pr_info("%s done\n", __func__);

	return ret;

err_sysfs3:
	if (ddata->vib_ops->set_intensity) {
		sysfs_remove_file(&ddata->to_dev->kobj,
				&dev_attr_intensity.attr);
	}
err_sysfs2:
	sysfs_remove_group(&ddata->to_dev->kobj, &sec_vibrator_attr_group);
err_sysfs1:
	device_destroy(ddata->to_class, MKDEV(0, 0));
err_device_create:
	class_destroy(ddata->to_class);
err_kthread:
	mutex_destroy(&ddata->vib_mutex);
	return ret;
}

int sec_vibrator_unregister(struct sec_vibrator_drvdata *ddata)
{
	pr_info("%s\n", __func__);
	sec_vibrator_haptic_disable(ddata);

	if (ddata->vib_ops->set_frequency) {
		sysfs_remove_group(&ddata->to_dev->kobj,
				&multi_freq_attr_group);
	}
	if (ddata->vib_ops->set_intensity) {
		sysfs_remove_file(&ddata->to_dev->kobj,
				&dev_attr_intensity.attr);
	}
	sysfs_remove_group(&ddata->to_dev->kobj, &sec_vibrator_attr_group);
	device_destroy(ddata->to_class, MKDEV(0, 0));
	class_destroy(ddata->to_class);
	mutex_destroy(&ddata->vib_mutex);
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sec vibrator driver");
