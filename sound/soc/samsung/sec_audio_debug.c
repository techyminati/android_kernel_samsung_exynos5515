/*
 *  sec_audio_debug.c
 *
 *  Copyright (c) 2021 Samsung Electronics
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include <linux/sched/clock.h>

#include <sound/soc.h>
#include <sound/samsung/sec_audio_debug.h>

#include <linux/sec_debug.h>
#include <sound/samsung/abox.h>
#include "abox/abox.h"
#include "abox/abox_core.h"

#define DBG_STR_BUFF_SZ 256

#define SEC_AUDIO_DEBUG_STRING_WQ_NAME "sec_audio_dbg_str_wq"

struct device *debug_dev;

struct sec_audio_debug_data {
	char *dbg_str_buf;
	unsigned long long mode_time;
	unsigned long mode_nanosec_t;
	struct mutex dbg_lock;
	struct workqueue_struct *debug_string_wq;
	struct work_struct debug_string_work;
	enum abox_debug_err_type debug_err_type;
	unsigned int *abox_dbg_addr;
};

static struct sec_audio_debug_data *p_debug_data;

static void abox_debug_string_update_workfunc(struct work_struct *wk)
{
	struct abox_data *data = abox_get_abox_data();
	int index;
	int gpr_id[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31}; /* R0~R14, PC */
	int gpr_count = sizeof(gpr_id) / sizeof(int);
	unsigned int len = 0;
/*
 *	struct sec_audio_debug_data *dbg_data = container_of(wk,
 *					   struct sec_audio_debug_data, debug_string_work);
*/
	if (!p_debug_data)
		return;

	mutex_lock(&p_debug_data->dbg_lock);

	p_debug_data->mode_time = data->audio_mode_time;
	p_debug_data->mode_nanosec_t = do_div(p_debug_data->mode_time, NSEC_PER_SEC);

	p_debug_data->dbg_str_buf = kzalloc(sizeof(char) * DBG_STR_BUFF_SZ, GFP_KERNEL);
	if (!p_debug_data->dbg_str_buf) {
		pr_err("%s: no memory\n", __func__);
		mutex_unlock(&p_debug_data->dbg_lock);
		return;
	}

	len += snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ - len,
			"ABOXERR%1x ", p_debug_data->debug_err_type);

	switch (p_debug_data->debug_err_type) {
	case TYPE_ABOX_DATAABORT:
	case TYPE_ABOX_PREFETCHABORT:
		if (!abox_is_on()) {
			len += snprintf(p_debug_data->dbg_str_buf + len,
					DBG_STR_BUFF_SZ - len, "Abox disabled");
		} else if (p_debug_data->abox_dbg_addr == NULL) {
			len += snprintf(p_debug_data->dbg_str_buf + len,
					DBG_STR_BUFF_SZ - len, "GPR NULL");
		} else {
			for (index = 0; index < gpr_count; index++) {
				len += snprintf(p_debug_data->dbg_str_buf + len,
						DBG_STR_BUFF_SZ - len, "%08x ",
						abox_core_read_gpr_dump(0, gpr_id[index], p_debug_data->abox_dbg_addr));
				if (len >= DBG_STR_BUFF_SZ - 1)
					goto buff_done;
			}
		}

		len += snprintf(p_debug_data->dbg_str_buf + len,
				DBG_STR_BUFF_SZ - len, "m%d:%05lu",
				data->audio_mode,
				(unsigned long) p_debug_data->mode_time);

		break;
	default:
		if (!abox_is_on()) {
			len += snprintf(p_debug_data->dbg_str_buf + len,
					DBG_STR_BUFF_SZ - len, "Abox disabled");
		} else {
			for (index = 0; index < gpr_count; index++) {
				len += snprintf(p_debug_data->dbg_str_buf + len,
						DBG_STR_BUFF_SZ - len, "%08x ",
						abox_core_read_gpr(0, gpr_id[index]));
				if (len >= DBG_STR_BUFF_SZ - 1)
					goto buff_done;
			}
		}
		len += snprintf(p_debug_data->dbg_str_buf + len,
				DBG_STR_BUFF_SZ - len, "m%d:%05lu",
				data->audio_mode,
				(unsigned long) p_debug_data->mode_time);
		break;
	}

buff_done:
	pr_info("%s: %s\n", __func__, p_debug_data->dbg_str_buf);
	/* not implemented yet */
	/* sec_debug_set_extra_info_aud(p_debug_data->dbg_str_buf); */
	secdbg_exin_set_aud(p_debug_data->dbg_str_buf);

	kfree(p_debug_data->dbg_str_buf);
	p_debug_data->dbg_str_buf = NULL;
	mutex_unlock(&p_debug_data->dbg_lock);
}

void abox_debug_string_update(enum abox_debug_err_type type, void *addr)
{
	p_debug_data->debug_err_type = type;
	p_debug_data->abox_dbg_addr = (unsigned int *) addr;

	queue_work(p_debug_data->debug_string_wq, &p_debug_data->debug_string_work);
}
EXPORT_SYMBOL_GPL(abox_debug_string_update);

static int sec_audio_debug_probe(struct platform_device *pdev)
{
	debug_dev = &pdev->dev;

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id sec_audio_debug_of_match[] = {
	{ .compatible = "samsung,audio-debug", },
	{},
};
MODULE_DEVICE_TABLE(of, sec_audio_debug_of_match);
#endif /* CONFIG_OF */

static struct platform_driver sec_audio_debug_driver = {
	.driver = {
		.name	= "sec-audio-debug",
		.owner	= THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = of_match_ptr(sec_audio_debug_of_match),
#endif /* CONFIG_OF */
	},

	.probe = sec_audio_debug_probe,
};
module_platform_driver(sec_audio_debug_driver);

static int __init sec_audio_debug_init(void)
{
	struct sec_audio_debug_data *data;
	int ret = 0;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("%s: failed to alloc data\n", __func__);
		return -ENOMEM;
	}

	p_debug_data = data;
	mutex_init(&p_debug_data->dbg_lock);

	p_debug_data->debug_string_wq = create_singlethread_workqueue(
						SEC_AUDIO_DEBUG_STRING_WQ_NAME);
	if (p_debug_data->debug_string_wq == NULL) {
		pr_err("%s: failed to creat debug_string_wq\n", __func__);
		ret = -ENOENT;
		goto err_aboxlog_data;
	}

	INIT_WORK(&p_debug_data->debug_string_work, abox_debug_string_update_workfunc);

	return 0;

err_aboxlog_data:
	mutex_destroy(&p_debug_data->dbg_lock);
	kfree(p_debug_data);
	p_debug_data = NULL;

	return ret;
}
early_initcall(sec_audio_debug_init);

static void __exit sec_audio_debug_exit(void)
{
	destroy_workqueue(p_debug_data->debug_string_wq);
	p_debug_data->debug_string_wq = NULL;

	mutex_destroy(&p_debug_data->dbg_lock);
	kfree(p_debug_data);
	p_debug_data = NULL;
}
module_exit(sec_audio_debug_exit);
MODULE_DESCRIPTION("Samsung Electronics Audio Debug driver");
MODULE_LICENSE("GPL");
