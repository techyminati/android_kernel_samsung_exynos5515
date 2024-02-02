/*
 *  sb_adc.c
 *  Samsung Mobile Battery ADC Driver
 *
 *  Copyright (C) 2022 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/iio/consumer.h>
#include <linux/platform_device.h>

#include "include/sb_adc.h"
#include "include/sec_sysfs.h"

#define SB_ADC_NAME			"sb-adc"
#define adc_log(str, ...)	pr_info("[sb-adc]:%s: "str, __func__, ##__VA_ARGS__)

typedef int (*sb_adc_order_it)(int *arr, int len, int adc);
typedef ssize_t (*show_it)(struct device *dev, struct device_attribute *attr, char *buf);

enum {
	SYSFS_IDX_ADC	= 0,
	SYSFS_IDX_VALUE,

	SYSFS_IDX_MAX
};

struct sb_adc_sysfs {
	struct sb_adc *adc;

	struct device_attribute attr[SYSFS_IDX_MAX];
	struct sec_sysfs sysfs;
};

struct sb_adc {
	struct list_head list;

	const char *name;
	int *arr_value;
	int *arr_adc;
	int arr_len;

	int value;
	int adc;
	int forced_adc;
	int forced_value;

	void *pdata;
	sb_adc_it adc_it;
	sb_adc_order_it order_it;

	int *adc_buf;
	union {
		unsigned int flags;

		struct {
			unsigned	total_cnt : 8,
						min_max : 7,
						order : 1,
						no_table : 1;

			unsigned	forced : 1;
		} flag;
	};

	struct sb_adc_sysfs *sysfs;
};

static LIST_HEAD(adc_list);
static DEFINE_MUTEX(adc_lock);

#define MAX_ADC_SYSFS	5
struct sb_adc_sysfs adc_sysfs[MAX_ADC_SYSFS];
unsigned int adc_sysfs_count;

struct sb_adc_sysfs *alloc_sysfs(void)
{
	if (adc_sysfs_count >= MAX_ADC_SYSFS)
		return NULL;

	return &adc_sysfs[adc_sysfs_count++];
}

static ssize_t show_adc_attrs(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sb_adc_sysfs *sysfs =
		container_of(attr, struct sb_adc_sysfs, attr[SYSFS_IDX_ADC]);
	int adc_now = 0;

	sb_adc_get_adc_now(sysfs->adc, &adc_now);
	return scnprintf(buf, PAGE_SIZE, "%d\n", adc_now);
}

static ssize_t show_attrs(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sb_adc_sysfs *sysfs =
		container_of(attr, struct sb_adc_sysfs, attr[SYSFS_IDX_VALUE]);
	int value_now = 0;

	sb_adc_get_value_now(sysfs->adc, &value_now);
	return scnprintf(buf, PAGE_SIZE, "%d\n", value_now);
}

static ssize_t store_attrs(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static void set_attr(struct device_attribute *attr, const char *name, show_it show)
{
	attr->attr.name = name;
	attr->attr.mode = 0664;
	attr->show = show;
	attr->store = store_attrs;
}

static struct sb_adc_sysfs *create_sysfs(struct sb_adc *adc)
{
	struct sb_adc_sysfs *sysfs;
	char *adc_name;
	int size, idx = 1;

	sysfs = alloc_sysfs();
	if (!sysfs)
		return NULL;

	size = strlen(adc->name) + 5;
	adc_name = kzalloc(size, GFP_KERNEL);
	if (!adc_name)
		return NULL;
	snprintf(adc_name, size, "%s_adc", adc->name);

	set_attr(&sysfs->attr[SYSFS_IDX_ADC], adc_name, show_adc_attrs);

	if (adc->flag.no_table)
		goto finish_sysfs;

	set_attr(&sysfs->attr[SYSFS_IDX_VALUE], adc->name, show_attrs);

	idx = SYSFS_IDX_MAX;
finish_sysfs:
	/* add sysfs node */
	sysfs->adc = adc;
	sysfs->sysfs.attr = sysfs->attr;
	sysfs->sysfs.size = idx;
	add_sec_sysfs(&sysfs->sysfs.list);

	return sysfs;
}

static int ascending_order(int *arr, int len, int adc)
{
	int low = 0, high = len - 1, mid;

	if (arr[low] >= adc)
		return -1;
	if (arr[high] <= adc)
		return len;

	while (low <= high) {
		mid = (high + low) / 2;
		if (arr[mid] > adc)
			high = mid - 1;
		else if (arr[mid] < adc)
			low = mid + 1;
		else
			return mid;
	}

	return high;
}

static int descending_order(int *arr, int len, int adc)
{
	int low = 0, high = len - 1, mid;

	if (arr[low] <= adc)
		return -1;
	if (arr[high] >= adc)
		return len;

	while (low <= high) {
		mid = (high + low) / 2;
		if (arr[mid] < adc)
			high = mid - 1;
		else if (arr[mid] > adc)
			low = mid + 1;
		else
			return mid;
	}

	return high;
}

static void qsort(int *arr, int start, int end)
{
	int pivot, temp;
	int i, j;

	if (start < end) {
		pivot = start;
		i = start;
		j = end;

		while (i < j) {
			while (arr[i] <= arr[pivot] && i < end)
				i++;
			while (arr[j] > arr[pivot])
				j--;

			if (i < j) {
				temp = arr[i];
				arr[i] = arr[j];
				arr[j] = temp;
			}
		}

		temp = arr[pivot];
		arr[pivot] = arr[j];
		arr[j] = temp;

		qsort(arr, start, j - 1);
		qsort(arr, j + 1, end);
	}
}

static int read_adc(struct sb_adc *adc)
{
	int i = 0, temp, total = 0;

	for (i = 0; i < adc->flag.total_cnt; i++) {
		if (adc->adc_it(adc->pdata, &temp))
			temp = adc->adc;

		adc->adc_buf[i] = temp;
		total += temp;
	}

	qsort(adc->adc_buf, 0, adc->flag.total_cnt - 1);
	for (i = 0; i < adc->flag.min_max; i++) {
		total -=
			(adc->adc_buf[i] + adc->adc_buf[adc->flag.total_cnt - 1 - i]);
	}
	temp = total / (adc->flag.total_cnt - (2 * adc->flag.min_max));
	adc_log("%s - temp = %d, total = %d\n", adc->name, temp, total);
	return temp;
}

static int convert_adc(struct sb_adc *adc, int temp)
{
	int idx, ret;

	if (adc->flag.no_table)
		return 0;

	idx = adc->order_it(adc->arr_adc, adc->arr_len, temp);
	if (idx <= -1) {
		ret = adc->arr_value[0];
		goto finish_convert;
	} else if (idx >= adc->arr_len) {
		ret = adc->arr_value[adc->arr_len - 1];
		goto finish_convert;
	}
	ret = adc->arr_value[idx];
	ret +=
		((adc->arr_value[idx + 1] - adc->arr_value[idx]) * (temp - adc->arr_adc[idx])) /
		(adc->arr_adc[idx + 1] - adc->arr_adc[idx]);

finish_convert:
	adc_log("%s - idx = %d, ret = %d\n", adc->name, idx, ret);
	return ret;
}

static struct device_node *find_adc_node(const char *name)
{
	struct device_node *np, *child;

	np = of_find_node_by_name(NULL, SB_ADC_NAME);
	if (!np)
		return NULL;

	for_each_child_of_node(np, child) {
		if (!strcmp(child->name, name))
			return child;
	}

	return NULL;
}

static int parse_adc_dt(struct device_node *node, struct sb_adc *adc)
{
	int adc_len, value_len;
	int ret = 0;

	ret = of_property_read_u32(node, "flag", (u32 *)&adc->flags);
	if (ret) {
		adc_log("%s - failed to read flag(%d)\n", node->name, ret);
		return ret;
	}

	adc->adc_buf = kcalloc(adc->flag.total_cnt, sizeof(int), GFP_KERNEL);
	if (!adc->adc_buf) {
		adc_log("%s - failed to alloc adc_BUF\n", node->name);
		return -ENOMEM;
	}

	if (adc->flag.no_table)
		goto finish_parse_adc;

	adc_len = of_property_count_u32_elems(node, "adc");
	value_len = of_property_count_u32_elems(node, "value");

	if ((adc_len <= 0) || (value_len <= 0) || (adc_len != value_len)) {
		adc_log("%s - invalid adc or value (%d, %d)\n", node->name, adc_len, value_len);
		goto invalid_size;
	}
	adc->arr_len = adc_len;

	adc->arr_adc = kcalloc(adc_len, sizeof(int), GFP_KERNEL);
	if (!adc->arr_adc) {
		adc_log("%s - failed to alloc adc\n", node->name);
		goto failed_to_alloc_adc;
	}

	ret = of_property_read_u32_array(node, "adc", (u32 *)adc->arr_adc, adc_len);
	if (ret) {
		adc_log("%s - failed to read adc(%d)\n", node->name, ret);
		goto failed_to_read_adc;
	}

	adc->arr_value = kcalloc(value_len, sizeof(int), GFP_KERNEL);
	if (!adc->arr_value) {
		adc_log("%s - failed to alloc value\n", node->name);
		goto failed_to_alloc_value;
	}

	ret = of_property_read_u32_array(node, "value", (u32 *)adc->arr_value, value_len);
	if (ret) {
		adc_log("%s - failed to read value(%d)\n", node->name, ret);
		goto failed_to_read_value;
	}

finish_parse_adc:
	return 0;

failed_to_read_value:
	kfree(adc->arr_value);
failed_to_alloc_value:
failed_to_read_adc:
	kfree(adc->arr_adc);
failed_to_alloc_adc:
invalid_size:
	kfree(adc->adc_buf);
	adc->arr_len = 0;
	return ret;
}

static struct sb_adc *find_adc(const char *name)
{
	struct sb_adc *adc;

	list_for_each_entry(adc, &adc_list, list) {
		if (!strcmp(adc->name, name))
			return adc;
	}

	return NULL;
}

static void del_adc(struct sb_adc *adc)
{
	adc_log("delete %s node.\n", adc->name);

	if (adc->sysfs != NULL)
		adc->sysfs->adc = NULL;

	kfree(adc->adc_buf);
	kfree(adc->arr_adc);
	kfree(adc->arr_value);
	list_del(&adc->list);
	kfree(adc);
}

static int get_adc(struct sb_adc *adc)
{
	return (adc->flag.forced) ?
		adc->forced_adc : adc->adc;
}

static int get_value(struct sb_adc *adc)
{
	return (adc->flag.forced) ?
		adc->forced_value : adc->value;
}

static void update_adc(struct sb_adc *adc)
{
	adc->adc = read_adc(adc);
	adc->value = convert_adc(adc, adc->adc);
}

struct sb_adc *sb_adc_add(const char *name, void *pdata, sb_adc_it adc_it)
{
	struct device_node *node;
	struct sb_adc *adc;
	int ret;

	if ((name == NULL) || (pdata == NULL) || (adc_it == NULL))
		return ERR_PTR(-EINVAL);

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc) {
		adc_log("%s was already added.\n", name);
		goto finish_add;
	}

	node = find_adc_node(name);
	if (node == NULL) {
		adc_log("%s - failed to find node\n", name);
		return ERR_PTR(-ENODEV);
	}

	adc = kzalloc(sizeof(struct sb_adc), GFP_KERNEL);
	if (!adc) {
		adc_log("%s - failed to alloc sb_adc\n", name);
		return ERR_PTR(-ENOMEM);
	}

	ret = parse_adc_dt(node, adc);
	if (ret) {
		adc_log("%s - failed to parse sb_adc(%d)\n", name, ret);
		kfree(adc);
		return ERR_PTR(ret);
	}

	adc->name = name;
	adc->pdata = pdata;
	adc->adc_it = adc_it;
	adc->order_it = (adc->flag.order) ?	descending_order : ascending_order;
	ret = adc->adc_it(pdata, &adc->adc);
	adc_log("Added %s - read adc(%d, %d)\n", name, adc->adc, ret);

	list_add(&adc->list, &adc_list);

	adc->sysfs = create_sysfs(adc);
finish_add:
	mutex_unlock(&adc_lock);
	return adc;
}
EXPORT_SYMBOL(sb_adc_add);

void sb_adc_del(struct sb_adc *adc)
{
	if (adc == NULL)
		return;

	mutex_lock(&adc_lock);
	del_adc(adc);
	mutex_unlock(&adc_lock);
}
EXPORT_SYMBOL(sb_adc_del);

int sb_adc_get_adc(struct sb_adc *adc, int *data)
{
	if ((adc == NULL) || (data == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	*data = get_adc(adc);
	mutex_unlock(&adc_lock);

	return 0;
}
EXPORT_SYMBOL(sb_adc_get_adc);

int sb_adc_get_value(struct sb_adc *adc, int *value)
{
	if ((adc == NULL) || (value == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	*value = get_value(adc);
	mutex_unlock(&adc_lock);

	return 0;
}
EXPORT_SYMBOL(sb_adc_get_value);

int sb_adc_get_adc_now(struct sb_adc *adc, int *data)
{
	if ((adc == NULL) || (data == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	*data = read_adc(adc);
	mutex_unlock(&adc_lock);

	return 0;
}
EXPORT_SYMBOL(sb_adc_get_adc_now);

int sb_adc_get_value_now(struct sb_adc *adc, int *value)
{
	if ((adc == NULL) || (value == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	*value = convert_adc(adc, read_adc(adc));
	mutex_unlock(&adc_lock);

	return 0;
}
EXPORT_SYMBOL(sb_adc_get_value_now);

int sb_adc_update(struct sb_adc *adc)
{
	if (adc == NULL)
		return -EINVAL;

	mutex_lock(&adc_lock);
	update_adc(adc);
	mutex_unlock(&adc_lock);

	return 0;
}
EXPORT_SYMBOL(sb_adc_update);

void sb_adc_delf(const char *name)
{
	struct sb_adc *adc;

	if (name == NULL)
		return;

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc)
		del_adc(adc);
	mutex_unlock(&adc_lock);
}
EXPORT_SYMBOL(sb_adc_delf);

int sb_adc_get_adcf(const char *name, int *data)
{
	struct sb_adc *adc;

	if ((name == NULL) || (data == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc)
		*data = get_adc(adc);
	mutex_unlock(&adc_lock);

	return (!adc) ? (-ENODEV) : 0;
}
EXPORT_SYMBOL(sb_adc_get_adcf);

int sb_adc_get_valuef(const char *name, int *value)
{
	struct sb_adc *adc;

	if ((name == NULL) || (value == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc)
		*value = get_value(adc);
	mutex_unlock(&adc_lock);

	return (!adc) ? (-ENODEV) : 0;
}
EXPORT_SYMBOL(sb_adc_get_valuef);

int sb_adc_get_adc_nowf(const char *name, int *data)
{
	struct sb_adc *adc;

	if ((name == NULL) || (data == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc)
		*data = read_adc(adc);
	mutex_unlock(&adc_lock);

	return (!adc) ? (-ENODEV) : 0;
}
EXPORT_SYMBOL(sb_adc_get_adc_nowf);

int sb_adc_get_value_nowf(const char *name, int *value)
{
	struct sb_adc *adc;

	if ((name == NULL) || (value == NULL))
		return -EINVAL;

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc)
		*value = convert_adc(adc, read_adc(adc));
	mutex_unlock(&adc_lock);

	return (!adc) ? (-ENODEV) : 0;
}
EXPORT_SYMBOL(sb_adc_get_value_nowf);

int sb_adc_updatef(const char *name)
{
	struct sb_adc *adc;

	if (name == NULL)
		return -EINVAL;

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc)
		update_adc(adc);
	mutex_unlock(&adc_lock);

	return (!adc) ? (-ENODEV) : 0;
}
EXPORT_SYMBOL(sb_adc_updatef);

struct sb_adc *sb_adc_get(const char *name)
{
	struct sb_adc *adc;

	if (name == NULL)
		return ERR_PTR(-EINVAL);

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	mutex_unlock(&adc_lock);

	return (!adc) ? ERR_PTR(-ENODEV) : adc;
}
EXPORT_SYMBOL(sb_adc_get);

int sb_adc_set(const char *name, bool forced, int data, int value)
{
	struct sb_adc *adc;

	if (name == NULL)
		return -EINVAL;

	mutex_lock(&adc_lock);
	adc = find_adc(name);
	if (adc) {
		adc->flag.forced = forced;
		adc->forced_adc = data;
		adc->forced_value = value;
	}
	mutex_unlock(&adc_lock);

	return (!adc) ? (-ENODEV) : 0;
}
EXPORT_SYMBOL(sb_adc_set);

void sb_adc_all_update(void)
{
	struct sb_adc *adc;

	mutex_lock(&adc_lock);
	list_for_each_entry(adc, &adc_list, list) {
		update_adc(adc);
	}
	mutex_unlock(&adc_lock);

}
EXPORT_SYMBOL(sb_adc_all_update);

static int iio_read(void *pdata, int *data)
{
	int ret = iio_read_channel_raw(pdata, data);

	return (ret >= 0) ? 0 : ret;
}

static int parse_dt(struct device *dev)
{
	const struct property *prop;
	struct iio_channel *iio;
	const char *p, *end;
	size_t l;

	prop = of_find_property(dev->of_node, "io-channel-names", NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	p = prop->value;
	end = p + prop->length;

	for (; p < end; p += l) {
		struct sb_adc *adc = NULL;

		l = strnlen(p, end - p) + 1;
		if (p + l > end)
			return -EILSEQ;

		iio = iio_channel_get(dev, p);
		if (IS_ERR_OR_NULL(iio))
			continue;

		adc = sb_adc_add(p, iio, iio_read);
		if (IS_ERR_VALUE(adc))
			iio_channel_release(iio);
	}

	return 0;
}

static void del_all(struct device *dev)
{
	const struct property *prop;
	const char *p, *end;
	size_t l;

	mutex_lock(&adc_lock);

	prop = of_find_property(dev->of_node, "io-channel-names", NULL);
	if (!prop)
		goto end_del;
	if (!prop->value)
		goto end_del;

	p = prop->value;
	end = p + prop->length;

	for (; p < end; p += l) {
		struct sb_adc *adc = NULL;

		l = strnlen(p, end - p) + 1;
		if (p + l > end)
			goto end_del;

		adc = find_adc(p);
		if (IS_ERR_VALUE(adc))
			continue;

		iio_channel_release((struct iio_channel *)adc->pdata);
		del_adc(adc);
	}

end_del:
	mutex_unlock(&adc_lock);
}

static int sb_adc_probe(struct platform_device *pdev)
{
	int ret = 0;

	adc_log("Start\n");
	ret = parse_dt(&pdev->dev);
	adc_log("Finish(%d)\n", ret);

	return 0;
}

static int sb_adc_remove(struct platform_device *pdev)
{
	adc_log("\n");
	del_all(&pdev->dev);
	return 0;
}

static void sb_adc_shutdown(struct platform_device *pdev)
{
	adc_log("\n");
	del_all(&pdev->dev);
}

static const struct of_device_id sb_adc_dt_ids[] = {
	{ .compatible = "samsung,sb-adc" },
	{ }
};
MODULE_DEVICE_TABLE(of, sb_adc_dt_ids);

static struct platform_driver sb_adc_driver = {
	.driver = {
		.name = SB_ADC_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sb_adc_dt_ids,
	},
	.probe = sb_adc_probe,
	.remove = sb_adc_remove,
	.shutdown = sb_adc_shutdown,
};

static int __init sb_adc_init(void)
{
	return platform_driver_register(&sb_adc_driver);
}

static void __exit sb_adc_exit(void)
{
	platform_driver_unregister(&sb_adc_driver);
}
late_initcall(sb_adc_init);
module_exit(sb_adc_exit);

MODULE_DESCRIPTION("Samsung Battery ADC Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
