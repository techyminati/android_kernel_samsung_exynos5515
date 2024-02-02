/*****************************************************************************
 *
 * Copyright (c) 2021 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/
#include "dev.h"
#include "debug.h"
#include "slsi_tracepoint_debug.h"

static DEFINE_MUTEX(slsi_tracepoint_cb_mutex);

#define TRACEPOINT_HASH_BITS 5
#define TRACEPOINT_TABLE_SIZE BIT(TRACEPOINT_HASH_BITS)
static struct hlist_head tracepoint_table[TRACEPOINT_TABLE_SIZE];

struct tracepoint_entry {
	struct tracepoint *tp;
	struct list_head probes;
	struct hlist_node hlist;
	char name[0];
};

struct tracepoint_cb {
	struct tracepoint_func tp_func;
	struct list_head list;
};

static struct tracepoint_entry *slsi_get_tracepoint(const char *tracepoint_name)
{
	struct tracepoint_entry *e;
	u32 hash = jhash(tracepoint_name, strlen(tracepoint_name), 0);
	struct hlist_head *head = &tracepoint_table[hash & (TRACEPOINT_TABLE_SIZE - 1)];

	hlist_for_each_entry(e, head, hlist) {
		if (!strcmp(tracepoint_name, e->name))
			return e;
	}
	return NULL;
}

static void slsi_add_tracepoint(struct tracepoint *tp, void *data)
{
	struct hlist_head *head;
	struct tracepoint_entry *e;
	char *tp_name;
	u32 hash;

	if (!data)
		return;

	tp_name = (char *)data;
	if (strcmp(tp->name, tp_name))
		return;

	e = kmalloc(sizeof(struct tracepoint_entry *), GFP_KERNEL);
	if (!e)
		return;

	hash = jhash(tp->name, strlen(tp->name), 0);
	head = &tracepoint_table[hash & (TRACEPOINT_TABLE_SIZE - 1)];
	memcpy(e->name, tp_name, strlen(tp_name) + 1);
	e->tp = tp;
	INIT_LIST_HEAD(&e->probes);
	hlist_add_head(&e->hlist, head);
}

static struct tracepoint_entry *slsi_add_new_tracepoint(const char *tp_name)
{
	for_each_kernel_tracepoint(slsi_add_tracepoint, (void *)tp_name);
	return slsi_get_tracepoint(tp_name);
}

int slsi_register_tracepoint_callback(const char *tp_name, void *func, void *data)
{
	struct tracepoint_entry *e;
	struct tracepoint_cb *tp_cb;
	int ret = 0;

	mutex_lock(&slsi_tracepoint_cb_mutex);
	e = slsi_get_tracepoint(tp_name);
	if (!e) {
		e = slsi_add_new_tracepoint(tp_name);
		if (!e) {
			SLSI_ERR_NODEV(" Tracepoint[%s] : Name is not correct.\n", tp_name);
			ret = -EINVAL;
			goto end;
		}
	}
	if (!list_empty(&e->probes)) {
		list_for_each_entry(tp_cb, &e->probes, list) {
			if (tp_cb->tp_func.func == func) {
				SLSI_INFO_NODEV(" Tracepoint[%s] : Function already exists.\n", tp_name);
				goto end;
			}
		}
	}
	tp_cb = kmalloc(sizeof(struct tracepoint_cb *), GFP_KERNEL);
	if (!tp_cb) {
		if (list_empty(&e->probes)) {
			hlist_del(&e->hlist);
			kfree(e);
		}
		ret = -ENOMEM;
		goto end;
	}
	tp_cb->tp_func.func = func;
	tp_cb->tp_func.data = data;
	list_add(&tp_cb->list, &e->probes);

	ret = tracepoint_probe_register(e->tp, tp_cb->tp_func.func, tp_cb->tp_func.data);
	SLSI_INFO_NODEV(" Tracepoint[%s] : Registration callback function is success.\n", tp_name);
end:
	mutex_unlock(&slsi_tracepoint_cb_mutex);
	return ret;
}

int slsi_unregister_tracepoint_callback(const char *tp_name, void *func)
{
	struct tracepoint_entry *e;
	struct tracepoint_cb *tp_cb;
	int ret = -EINVAL;

	mutex_lock(&slsi_tracepoint_cb_mutex);
	e = slsi_get_tracepoint(tp_name);
	if (!e)
		goto err;

	if (!list_empty(&e->probes)) {
		list_for_each_entry(tp_cb, &e->probes, list) {
			if (tp_cb->tp_func.func == func) {
				ret = tracepoint_probe_unregister(e->tp, tp_cb->tp_func.func, NULL);
				list_del(&tp_cb->list);
				kfree(tp_cb);
				goto end;
			}
		}
	}
end:
	if (list_empty(&e->probes)) {
		hlist_del(&e->hlist);
		kfree(e);
	}
	SLSI_INFO_NODEV(" Tracepoint[%s] : Callback function is unregistered\n", tp_name);
err:
	mutex_unlock(&slsi_tracepoint_cb_mutex);
	return ret;
}

int slsi_unregister_tracepoint_callback_all(const char *tp_name)
{
	struct tracepoint_entry *e;
	struct tracepoint_cb *tp_cb;
	struct tracepoint_cb *backup_tp_cb;
	int ret = -EINVAL;

	mutex_lock(&slsi_tracepoint_cb_mutex);
	e = slsi_get_tracepoint(tp_name);
	if (!e)
		goto end;

	if (!list_empty(&e->probes)) {
		list_for_each_entry_safe(tp_cb, backup_tp_cb, &e->probes, list) {
			ret = tracepoint_probe_unregister(e->tp, tp_cb->tp_func.func, NULL);
			if (ret < 0)
				goto end;
			list_del(&tp_cb->list);
			kfree(tp_cb);
		}
	}
	hlist_del(&e->hlist);
	kfree(e);
	SLSI_INFO_NODEV(" Tracepoint[%s] : All callback functions are unregisters.\n", tp_name);
end:
	mutex_unlock(&slsi_tracepoint_cb_mutex);
	return ret;
}

static void remove_tracepoint(struct tracepoint *tp, void *ignore)
{
	slsi_unregister_tracepoint_callback_all(tp->name);
}

void slsi_unregister_all_tracepoints(void)
{
	for_each_kernel_tracepoint(remove_tracepoint, NULL);
}
