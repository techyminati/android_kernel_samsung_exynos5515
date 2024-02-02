/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/of_reserved_mem.h>
#include <linux/sched/clock.h>
#include <linux/of.h>
#include <linux/arm-smccc.h>
#include <linux/smc.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/pstore_ram.h>

#include "debug-snapshot-local.h"
#include <linux/sec_debug.h>

#ifdef CONFIG_SEC_PM_DEBUG
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

static bool sec_log_full;
#endif /* CONFIG_SEC_PM_DEBUG */

#define DSS_MAGIC	0x1234ABCD

struct dbg_snapshot_bl {
	unsigned int magic;
	unsigned int item_count;
	struct {
		char name[SZ_16];
		unsigned long paddr;
		unsigned long size;
	} item[DSS_MAX_BL_SIZE];
};

struct dbg_snapshot_interface {
	struct dbg_snapshot_log *info_event;
};

struct dbg_snapshot_bl *dss_bl;
struct dbg_snapshot_item dss_items[] = {
	[DSS_ITEM_HEADER_ID]	= {DSS_ITEM_HEADER,	{0, 0, 0, true}, true, NULL ,NULL,},
	[DSS_ITEM_KERNEL_ID]	= {DSS_ITEM_KERNEL,	{0, 0, 0, false}, true, NULL ,NULL,},
	[DSS_ITEM_PLATFORM_ID]	= {DSS_ITEM_PLATFORM,	{0, 0, 0, false}, false, NULL ,NULL,},
	[DSS_ITEM_FATAL_ID]	= {DSS_ITEM_FATAL,	{0, 0, 0, false}, false, NULL ,NULL,},
	[DSS_ITEM_KEVENTS_ID]	= {DSS_ITEM_KEVENTS,	{0, 0, 0, false}, false, NULL ,NULL,},
	[DSS_ITEM_S2D_ID]	= {DSS_ITEM_S2D,	{0, 0, 0, true}, false, NULL, NULL,},
	[DSS_ITEM_ARRDUMP_RESET_ID] = {DSS_ITEM_ARRDUMP_RESET,{0, 0, 0, false}, false, NULL, NULL,},
	[DSS_ITEM_ARRDUMP_PANIC_ID] = {DSS_ITEM_ARRDUMP_PANIC,{0, 0, 0, false}, false, NULL, NULL,},
	[DSS_ITEM_FIRST_ID] = {DSS_ITEM_FIRST,		{0, 0, 0, false}, false, NULL, NULL,},
	[DSS_ITEM_INIT_TASK_ID] = {DSS_ITEM_INIT_TASK,	{0, 0, 0, false}, false, NULL, NULL,},
};
EXPORT_SYMBOL(dss_items);

/*  External interface variable for trace debugging */
static struct dbg_snapshot_interface dss_info __attribute__ ((used));
static struct dbg_snapshot_interface *ess_info __attribute__ ((used));
#ifdef CONFIG_SEC_BOOTSTAT
static void (*hook_bootstat)(const char *buf);
#endif
char *last_kmsg;
EXPORT_SYMBOL(last_kmsg);
int last_kmsg_size;
EXPORT_SYMBOL(last_kmsg_size);

struct dbg_snapshot_base *dss_base;
struct dbg_snapshot_base *ess_base;
struct dbg_snapshot_log *dss_log = NULL;
struct dbg_snapshot_desc dss_desc;

#ifdef CONFIG_SEC_DEBUG
void secdbg_base_get_kevent_info(struct ess_info_offset *p, int type)
{
	unsigned long kevent_base_va = (unsigned long)(dss_log->task);
	unsigned long kevent_base_pa = dss_items[DSS_ITEM_KEVENTS_ID].entry.paddr;

	switch (type) {
	case DSS_KEVENT_TASK:
		p->base = kevent_base_pa + (unsigned long)(dss_log->task) - kevent_base_va;
		p->nr = ARRAY_SIZE(dss_log->task[0]);
		p->size = sizeof(struct task_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_WORK:
		p->base = kevent_base_pa + (unsigned long)(dss_log->work) - kevent_base_va;
		p->nr = ARRAY_SIZE(dss_log->work[0]);
		p->size = sizeof(struct work_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_IRQ:
		p->base = kevent_base_pa + (unsigned long)(dss_log->irq) - kevent_base_va;
		p->nr = ARRAY_SIZE(dss_log->irq[0]);
		p->size = sizeof(struct irq_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_FREQ:
		p->base = kevent_base_pa + (unsigned long)(dss_log->freq) - kevent_base_va;
		p->nr = ARRAY_SIZE(dss_log->freq[0]);
		p->size = sizeof(struct freq_log);
		p->per_core = 0;
		break;

	case DSS_KEVENT_IDLE:
		p->base = kevent_base_pa + (unsigned long)(dss_log->cpuidle) - kevent_base_va;
		p->nr = ARRAY_SIZE(dss_log->cpuidle[0]);
		p->size = sizeof(struct cpuidle_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_THRM:
		p->base = kevent_base_pa + (unsigned long)(dss_log->thermal) - kevent_base_va;
		p->nr = ARRAY_SIZE(dss_log->thermal);
		p->size = sizeof(struct thermal_log);
		p->per_core = 0;
		break;

	case DSS_KEVENT_ACPM:
		p->base = kevent_base_pa + (unsigned long)(dss_log->acpm) - kevent_base_va;
		p->nr = ARRAY_SIZE(dss_log->acpm);
		p->size = sizeof(struct acpm_log);
		p->per_core = 0;
		break;

/*	case DSS_KEVENT_MFRQ:
		p->base = kevent_base_pa + (unsigned long)(dss_log->freq_misc) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct freq_misc_log);
		p->per_core = 0;
		break;
*/
	default:
		p->base = 0;
		p->nr = 0;
		p->size = 0;
		p->per_core = 0;
		break;
	}

	p->last = secdbg_base_get_kevent_index_addr(type);
}
#endif

int dbg_snapshot_add_bl_item_info(const char *name,
			unsigned long paddr, unsigned long size)
{
	unsigned int i;

	if (!paddr || !size) {
		dev_err(dss_desc.dev, "Invalid address(%s) or size(%s)\n",
				paddr, size);
		return -EINVAL;
	}

	if (!dss_bl || dss_bl->item_count >= DSS_MAX_BL_SIZE) {
		dev_err(dss_desc.dev, "Item list full or null!\n");
		return -ENOMEM;
	}

	for (i = 0; i < dss_bl->item_count; i++) {
		if (!strncmp(dss_bl->item[i].name, name,
					strlen(dss_bl->item[i].name))) {
			dev_err(dss_desc.dev, "Already has %s item\n", name);
			return -EEXIST;
		}
	}

	memcpy(dss_bl->item[dss_bl->item_count].name, name, strlen(name) + 1);
	dss_bl->item[dss_bl->item_count].paddr = paddr;
	dss_bl->item[dss_bl->item_count].size = size;
	dss_bl->item_count++;

	return 0;
}
EXPORT_SYMBOL(dbg_snapshot_add_bl_item_info);

void __iomem *dbg_snapshot_get_header_vaddr(void)
{
	if (dbg_snapshot_get_item_enable(DSS_ITEM_HEADER))
		return (void __iomem *)(dss_items[DSS_ITEM_HEADER_ID].entry.vaddr);
	else
		return NULL;
}
EXPORT_SYMBOL(dbg_snapshot_get_header_vaddr);

unsigned long dbg_snapshot_get_header_paddr(void)
{
	if (dbg_snapshot_get_item_enable(DSS_ITEM_HEADER))
		return (unsigned long)dss_items[DSS_ITEM_HEADER_ID].entry.paddr;
	else
		return 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_header_paddr);

unsigned int dbg_snapshot_get_val_offset(unsigned int offset)
{
	if (dbg_snapshot_get_item_enable(DSS_ITEM_HEADER))
		return __raw_readl(dbg_snapshot_get_header_vaddr() + offset);
	else
		return 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_val_offset);

void dbg_snapshot_set_val_offset(unsigned int val, unsigned int offset)
{
	if (dbg_snapshot_get_item_enable(DSS_ITEM_HEADER))
		__raw_writel(val, dbg_snapshot_get_header_vaddr() + offset);
}
EXPORT_SYMBOL(dbg_snapshot_set_val_offset);

void dbg_snapshot_set_str_offset(unsigned int offset,
				 size_t len, const char *fmt,  ...)
{
	char *buf = (char *)(dbg_snapshot_get_header_vaddr() + offset);
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, len, fmt, args);
	va_end(args);
}
EXPORT_SYMBOL(dbg_snapshot_set_str_offset);

static void dbg_snapshot_set_sjtag_status(void)
{
#ifdef SMC_CMD_GET_SJTAG_STATUS
	struct arm_smccc_res res;
	arm_smccc_smc(SMC_CMD_GET_SJTAG_STATUS, 0x3, 0, 0, 0, 0, 0, 0, &res);
	dss_desc.sjtag_status = res.a0;
	dev_info(dss_desc.dev, "SJTAG is %sabled\n",
			dss_desc.sjtag_status ? "en" : "dis");
#endif
}

int dbg_snapshot_get_sjtag_status(void)
{
	return dss_desc.sjtag_status;
}
EXPORT_SYMBOL(dbg_snapshot_get_sjtag_status);

void dbg_snapshot_scratch_reg(unsigned int val)
{
	dbg_snapshot_set_val_offset(val, DSS_OFFSET_SCRATCH);
}
EXPORT_SYMBOL(dbg_snapshot_scratch_reg);

void dbg_snapshot_scratch_clear(void)
{
	dbg_snapshot_scratch_reg(DSS_SIGN_RESET);
}
EXPORT_SYMBOL(dbg_snapshot_scratch_clear);

bool dbg_snapshot_is_scratch(void)
{
	return dbg_snapshot_get_val_offset(DSS_OFFSET_SCRATCH) == DSS_SIGN_SCRATCH;
}
EXPORT_SYMBOL(dbg_snapshot_is_scratch);

void dbg_snapshot_set_debug_test_buffer_addr(u64 paddr, unsigned int cpu)
{
	void __iomem *header = dbg_snapshot_get_header_vaddr();

	if (header)
		__raw_writeq(paddr, header + DSS_OFFSET_DEBUG_TEST_BUFFER(cpu));
}
EXPORT_SYMBOL(dbg_snapshot_set_debug_test_buffer_addr);

unsigned int dbg_snapshot_get_debug_test_buffer_addr(unsigned int cpu)
{
	void __iomem *header = dbg_snapshot_get_header_vaddr();

	return header ? __raw_readq(header + DSS_OFFSET_DEBUG_TEST_BUFFER(cpu)) : 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_debug_test_buffer_addr);

unsigned long dbg_snapshot_get_last_pc_paddr(void)
{
	/* We want to save the pc value to NC region only if DSS is enabled. */
	unsigned long header = dbg_snapshot_get_header_paddr();

	return header ? header + DSS_OFFSET_CORE_LAST_PC : 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_last_pc_paddr);

unsigned long dbg_snapshot_get_last_pc(unsigned int cpu)
{
	void __iomem *header = dbg_snapshot_get_header_vaddr();

	if (header)
		return __raw_readq(header + DSS_OFFSET_CORE_LAST_PC + cpu * 8);
	else
		return 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_last_pc);

int dbg_snapshot_get_dpm_none_dump_mode(void)
{
	unsigned int val;
	void __iomem *header = dbg_snapshot_get_header_vaddr();

	if (header) {
		val = __raw_readl(header + DSS_OFFSET_NONE_DPM_DUMP_MODE);
		if ((val & GENMASK(31, 16)) == DSS_SIGN_MAGIC)
			return (val & GENMASK(15, 0));
	}

	return -1;
}
EXPORT_SYMBOL(dbg_snapshot_get_dpm_none_dump_mode);

void dbg_snapshot_set_dpm_none_dump_mode(unsigned int mode)
{
	void __iomem *header = dbg_snapshot_get_header_vaddr();

	if (header) {
		if (mode)
			mode |= DSS_SIGN_MAGIC;
		else
			mode = 0;

		__raw_writel(mode, header + DSS_OFFSET_NONE_DPM_DUMP_MODE);
	}
}
EXPORT_SYMBOL(dbg_snapshot_set_dpm_none_dump_mode);

void dbg_snapshot_set_qd_entry(unsigned long address)
{
	void __iomem *header = dbg_snapshot_get_header_vaddr();

	if (header)
		__raw_writeq((unsigned long)(virt_to_phys)((void *)address),
				header + DSS_OFFSET_QD_ENTRY);
}
EXPORT_SYMBOL(dbg_snapshot_set_qd_entry);

static struct dbg_snapshot_item *dbg_snapshot_get_item(const char *name)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		if (dss_items[i].name &&
				!strncmp(dss_items[i].name, name, strlen(name)))
			return &dss_items[i];
	}

	return NULL;
}

unsigned int dbg_snapshot_get_item_size(const char* name)
{
	struct dbg_snapshot_item *item = dbg_snapshot_get_item(name);

	return item && item->entry.enabled ? item->entry.size : 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_item_size);

unsigned long dbg_snapshot_get_item_vaddr(const char *name)
{
	struct dbg_snapshot_item *item = dbg_snapshot_get_item(name);

	return item && item->entry.enabled ? item->entry.vaddr : 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_item_vaddr);

unsigned int dbg_snapshot_get_item_paddr(const char* name)
{
	struct dbg_snapshot_item *item = dbg_snapshot_get_item(name);

	return item && item->entry.enabled ? item->entry.paddr : 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_item_paddr);

int dbg_snapshot_get_item_enable(const char *name)
{
	struct dbg_snapshot_item *item = dbg_snapshot_get_item(name);

	return item && item->entry.enabled ? item->entry.enabled : 0;
}
EXPORT_SYMBOL(dbg_snapshot_get_item_enable);

void dbg_snapshot_set_item_enable(const char *name, int en)
{
	struct dbg_snapshot_item *item = NULL;

	if (!name || !dss_dpm.enabled_debug || dss_dpm.dump_mode == NONE_DUMP)
		return;

	/* This is default for debug-mode */
	item = dbg_snapshot_get_item(name);
	if (item) {
		item->entry.enabled = en;
		pr_info("item - %s is %sabled\n", name, en ? "en" : "dis");
	}
}
EXPORT_SYMBOL(dbg_snapshot_set_item_enable);

static void dbg_snapshot_set_enable(int en)
{
	dss_base->enabled = en;
	dev_info(dss_desc.dev, "%sabled\n", en ? "en" : "dis");
}

int dbg_snapshot_get_enable(void)
{
	return dss_base && dss_base->enabled;
}
EXPORT_SYMBOL(dbg_snapshot_get_enable);

void *dbg_snapshot_get_item_by_index(int index)
{
	if (index < 0 || index > ARRAY_SIZE(dss_items))
		return NULL;

	return &dss_items[index];
}
EXPORT_SYMBOL(dbg_snapshot_get_item_by_index);

int dbg_snapshot_get_num_items(void)
{
	return ARRAY_SIZE(dss_items);
}
EXPORT_SYMBOL(dbg_snapshot_get_num_items);

static inline int dbg_snapshot_check_eob(struct dbg_snapshot_item *item,
						size_t size)
{
	size_t max = (size_t)(item->head_ptr + item->entry.size);
	size_t cur = (size_t)(item->curr_ptr + size);

	return (unlikely(cur > max)) ? -1 : 0;
}

static void dbg_snapshot_hook_logger(const char *buf, size_t size,
				    unsigned int id)
{
	struct dbg_snapshot_item *item = &dss_items[id];
	size_t last_buf;
	u32 offset = 0;
#ifdef CONFIG_SEC_BOOTSTAT
	static char *logger_buffer = NULL;
#endif

	if (!dbg_snapshot_get_enable() || !item->entry.enabled)
		return;

	if (id == DSS_ITEM_KERNEL_ID) {
		offset = DSS_OFFSET_LAST_LOGBUF;
	} else if (id == DSS_ITEM_PLATFORM_ID) {
		offset = DSS_OFFSET_LAST_PLATFORM_LOGBUF;
	} else if (id == DSS_ITEM_FIRST_ID) {
		if (dbg_snapshot_check_eob(item, size)) {
			item->entry.enabled = false;
			return;
		}
	}
	if (dbg_snapshot_check_eob(item, size)) {
		if (id == DSS_ITEM_KERNEL_ID)
			dbg_snapshot_set_val_offset(1, DSS_OFFSET_CHECK_EOB);
		item->curr_ptr = item->head_ptr;
#ifdef CONFIG_SEC_PM_DEBUG
		sec_log_full = true;
#endif /* CONFIG_SEC_PM_DEBUG */
	}

	memcpy(item->curr_ptr, buf, size);
	item->curr_ptr += size;
	/*  save the address of last_buf to physical address */
	last_buf = (size_t)item->curr_ptr;

	if (offset)
		dbg_snapshot_set_val_offset(item->entry.paddr +
					(last_buf - item->entry.vaddr),
					offset);

#ifdef CONFIG_SEC_BOOTSTAT
	/* SEC_BOOTSTAT */
	if (id != DSS_ITEM_PLATFORM_ID || logger_buffer == buf)
		return;

	if (size > 2 && strncmp(buf, "!@", 2) == 0) {
		char _buf[SZ_128];
		size_t count = size < SZ_128 ? size : SZ_128 - 1;
		memcpy(_buf, buf, count);
		_buf[count] = '\0';
		pr_info("%s\n", _buf);

		if ((hook_bootstat) && size > 6 &&
				strncmp(_buf, "!@Boot", 6) == 0)
			hook_bootstat(_buf);
	}
}

void register_hook_bootstat(void (*func)(const char *buf))
{
	if (!func)
		return;

	hook_bootstat = func;
	pr_info("%s done!\n", __func__);
}
EXPORT_SYMBOL(register_hook_bootstat);
#endif

void dbg_snapshot_output(void)
{
	unsigned long i, size = 0;

	pr_info("debug-snapshot physical / virtual memory layout:\n");
	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		if (!dss_items[i].entry.enabled)
			continue;
		pr_info("%-16s: phys:0x%zx / virt:0x%zx / size:0x%zx / en:%d\n",
				dss_items[i].name,
				dss_items[i].entry.paddr,
				dss_items[i].entry.vaddr,
				dss_items[i].entry.size,
				dss_items[i].entry.enabled);
		size += dss_items[i].entry.size;
	}

	pr_info("total_item_size: %ldKB, dbg_snapshot_log struct size: %ldKB\n",
			size / SZ_1K, sizeof(struct dbg_snapshot_log) / SZ_1K);
}
EXPORT_SYMBOL(dbg_snapshot_output);

static void dbg_snapshot_init_desc(struct device *dev)
{
	u32 val;

	/* initialize dss_desc */
	memset((void *)&dss_desc, 0, sizeof(struct dbg_snapshot_desc));
	raw_spin_lock_init(&dss_desc.ctrl_lock);
	dbg_snapshot_set_sjtag_status();
	dss_desc.dev = dev;

	if (!of_property_read_u32(dev->of_node, "panic_to_wdt", &val))
		dss_desc.panic_to_wdt = val;

	if (!of_property_read_u32(dev->of_node, "hold-key", &val))
		dss_desc.hold_key = val;

	if (!of_property_read_u32(dev->of_node, "trigger-key", &val))
		dss_desc.trigger_key = val;
}

static void dbg_snapshot_fixmap(void)
{
	size_t last_buf;
	size_t vaddr, paddr, size, offset;
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		if (!dss_items[i].entry.enabled)
			continue;

		/*  assign dss_item information */
		paddr = dss_items[i].entry.paddr;
		vaddr = dss_items[i].entry.vaddr;
		size = dss_items[i].entry.size;

		if (i == DSS_ITEM_HEADER_ID) {
			/*  initialize kernel event to 0 except only header */
			memset((size_t *)(vaddr + DSS_KEEP_HEADER_SZ), 0,
					size - DSS_KEEP_HEADER_SZ);

			dss_bl = dbg_snapshot_get_header_vaddr() + DSS_OFFSET_ITEM_INFO;
			memset(dss_bl, 0, sizeof(struct dbg_snapshot_bl));
			dss_bl->magic = DSS_MAGIC;
			dss_bl->item_count = 0;
		} else if (i == DSS_ITEM_KERNEL_ID || i == DSS_ITEM_PLATFORM_ID) {
			/*  load last_buf address value(phy) by virt address */
			if (i == DSS_ITEM_KERNEL_ID)
				offset = DSS_OFFSET_LAST_LOGBUF;
			else if (i == DSS_ITEM_PLATFORM_ID)
				offset = DSS_OFFSET_LAST_PLATFORM_LOGBUF;

			last_buf = (size_t)dbg_snapshot_get_val_offset(offset);
			/*  check physical address offset of logbuf */
			if (last_buf >= paddr && (last_buf < paddr + size)) {
				/*  assumed valid address, conversion to virt */
				dss_items[i].curr_ptr =
					(unsigned char *)(vaddr + (last_buf - paddr));
			} else {
				/*  invalid address, set to first line */
				dss_items[i].curr_ptr = (unsigned char *)vaddr;
				/*  initialize logbuf to 0 */
				memset((size_t *)vaddr, 0, size);
			}
		} else if (i == DSS_ITEM_KEVENTS_ID) {
			if (!dss_items[i].persist)
				memset((size_t *)vaddr, 0, size);

			if (size < sizeof(struct dbg_snapshot_log)) {
				dbg_snapshot_set_val_offset(0x5F5F5F5F,
						DSS_OFFSET_KEVENTS_OVERFLOW);
				pr_err("dbg_snapshot_log is larger than kevents!\n");
				dbg_snapshot_set_item_enable(DSS_ITEM_KEVENTS, false);
			}
		} else {
			/*  initialized log to 0 if persist == false */
			if (!dss_items[i].persist)
				memset((size_t *)vaddr, 0, size);
		}

		dbg_snapshot_add_bl_item_info(dss_items[i].name, paddr, size);
	}

	dss_log = (struct dbg_snapshot_log *)(dss_items[DSS_ITEM_KEVENTS_ID].entry.vaddr);
	/*  set fake translation to virtual address to debug trace */
	dss_info.info_event = dss_log;
	ess_info = &dss_info;

	/* output the information of debug-snapshot */
	dbg_snapshot_output();
}

static void dbg_snapshot_boot_cnt(void)
{
	struct dbg_snapshot_item *item = &dss_items[DSS_ITEM_PLATFORM_ID];
	unsigned int reg;

	reg = dbg_snapshot_get_val_offset(DSS_OFFSET_KERNEL_BOOT_CNT_MAGIC);
	if (reg != DSS_BOOT_CNT_MAGIC) {
		dbg_snapshot_set_val_offset(DSS_BOOT_CNT_MAGIC,
				DSS_OFFSET_KERNEL_BOOT_CNT_MAGIC);
		reg = 0;
	} else {
		reg = dbg_snapshot_get_val_offset(DSS_OFFSET_KERNEL_BOOT_CNT);
	}
	dbg_snapshot_set_val_offset(++reg, DSS_OFFSET_KERNEL_BOOT_CNT);

	dev_info(dss_desc.dev, "Kernel Booting SEQ #%u\n", reg);
	if (dbg_snapshot_get_enable() && item->entry.enabled) {
		char _buf[SZ_128];

		snprintf(_buf, SZ_128 - 1, "\nBooting SEQ #%u\n", reg);
		dbg_snapshot_hook_logger((const char *)_buf, strlen(_buf),
				DSS_ITEM_PLATFORM_ID);
	}
}

static int dbg_snapshot_rmem_setup(struct device_node *root)
{
	struct reserved_mem *rmem;
	struct device_node *rmem_np;
	struct dbg_snapshot_item *item;
	bool en;
	unsigned long i, j;
	unsigned long flags = VM_NO_GUARD | VM_MAP;
	pgprot_t prot = __pgprot(PROT_NORMAL_NC);
	unsigned int cacheable;
	int page_size, mem_count = 0;
	struct page *page;
	struct page **pages;
	void *vaddr;

	mem_count = of_count_phandle_with_args(root, "memory-region", NULL);
	if (mem_count <= 0) {
		pr_err("debug-snapshot: no such memory-region\n");
		return -ENOMEM;
	}

	for (i = 0; i < mem_count; i++) {
		rmem_np = of_parse_phandle(root, "memory-region", i);
		if (!rmem_np) {
			pr_err("debug-snapshot: no such memory-region of index %d\n", i);
			continue;
		}

		en = of_device_is_available(rmem_np);
		if (!en) {
			pr_err("debug-snapshot: %s item is disabled, Skip alloc reserved memory\n",
					rmem_np->name);
			continue;
		}

		rmem = of_reserved_mem_lookup(rmem_np);
		if (!rmem) {
			pr_err("debug-snapshot: no such reserved mem of node name %s\n",
					rmem_np->name);
			continue;
		}

		dbg_snapshot_set_item_enable(rmem->name, en);
		item = dbg_snapshot_get_item(rmem->name);
		if (!rmem->base || !rmem->size) {
			pr_err("debug-snapshot: %s item wrong base(0x%x) or size(0x%x)\n",
					item->name, rmem->base, rmem->size);
			item->entry.enabled = false;
			continue;
		}

		if (!of_property_read_u32(rmem_np, "cacheable", &cacheable)) {
			if (cacheable) {
				prot = __pgprot(PROT_NORMAL);
				pr_info("%s item is changed to cacheable\n", item->name);
			}
		}

		page_size = rmem->size / PAGE_SIZE;
		pages = kzalloc(sizeof(struct page *) * page_size, GFP_KERNEL);
		page = phys_to_page(rmem->base);

		for (j = 0; j < page_size; j++)
			pages[j] = page++;

		vaddr = vmap(pages, page_size, flags, prot);
		kfree(pages);
		if (!vaddr) {
			pr_err("debug-snapshot: %s: paddr:%x page_size:%x failed to vmap\n",
					item->name, rmem->base, page_size);
			item->entry.enabled = false;
			continue;
		}

		item->entry.paddr = rmem->base;
		item->entry.size = rmem->size;
		item->entry.vaddr = (size_t)vaddr;
		item->head_ptr = (unsigned char *)vaddr;
		item->curr_ptr = (unsigned char *)vaddr;

		if (item == &dss_items[DSS_ITEM_HEADER_ID]) {
			memset(dbg_snapshot_get_header_vaddr(), 0,
					sizeof(struct dbg_snapshot_base));
			dss_base = (struct dbg_snapshot_base *)dbg_snapshot_get_header_vaddr();
			dss_base->enabled = false;
			dss_base->vaddr = (size_t)vaddr;
			dss_base->paddr = rmem->base;
			ess_base = dss_base;
		}
		dss_base->size += rmem->size;
	}

	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		item = &dss_items[i];
		if (!item->entry.paddr || !item->entry.size)
			item->entry.enabled = false;
	}

	return 0;
}

static ssize_t dbg_snapshot_last_kmsg_read(struct file *file, char __user *buf,
					 size_t count, loff_t *ppos)
{
	int ret = 0;
	unsigned int size = last_kmsg_size;

	if (!last_kmsg || !size)
		goto out;

	if (*ppos > size)
		goto out;

	if (*ppos + count > size)
		count = size - *ppos;

	ret = copy_to_user(buf, last_kmsg + (*ppos), count);
	if (ret)
		return -EFAULT;

	*ppos += count;
	ret = count;
out:
	return ret;
}

static ssize_t dbg_snapshot_first_kmsg_read(struct file *file, char __user *buf,
					 size_t count, loff_t *ppos)
{
	int ret = 0;
	long first_vaddr = dbg_snapshot_get_item_vaddr(DSS_ITEM_FIRST);
	long log_size = dbg_snapshot_get_item_size(DSS_ITEM_FIRST);

	if (!first_vaddr || !log_size)
		goto out;

	if (*ppos > log_size)
		goto out;

	if (*ppos + count > log_size)
		count = log_size - *ppos;

	ret = copy_to_user(buf, (char *)first_vaddr + (*ppos), count);
	if (ret)
		return -EFAULT;

	*ppos += count;
	ret = count;
out:
	return ret;
}

static const struct file_operations proc_last_kmsg_op = {
	.owner	= THIS_MODULE,
	.read 	= dbg_snapshot_last_kmsg_read,
};

static const struct file_operations proc_first_kmsg_op = {
	.owner	= THIS_MODULE,
	.read 	= dbg_snapshot_first_kmsg_read,
};

static void dbg_snapshot_init_proc(void)
{
	struct dbg_snapshot_item *item = &dss_items[DSS_ITEM_KERNEL_ID];
	size_t start, max, cur;

	if (!item)
		return;

	if (!item->entry.enabled || !item->entry.size)
		return;

	start = (size_t)item->head_ptr;
	max = (size_t)(item->head_ptr + item->entry.size);
	cur = (size_t)item->curr_ptr;

	last_kmsg_size = item->entry.size;
	if (dbg_snapshot_get_item_enable(DSS_ITEM_FIRST))
		proc_create("first_kmsg", 0, NULL, &proc_first_kmsg_op);

	last_kmsg = (char *)vzalloc(last_kmsg_size);
	if (!last_kmsg)
		return;

	memcpy(last_kmsg, item->curr_ptr, max - cur);
	memcpy(last_kmsg + max - cur, item->head_ptr, cur - start);

	proc_create("last_kmsg", 0, NULL, &proc_last_kmsg_op);
}

static ssize_t dss_panic_to_wdt_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%sabled\n",
			dss_desc.panic_to_wdt ? "En" : "Dis");
}

static ssize_t dss_panic_to_wdt_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long mode;

	if (!kstrtoul(buf, 10, &mode))
		dss_desc.panic_to_wdt = !!mode;

	return count;
}

static ssize_t dss_dpm_none_dump_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "currnet DPM dump_mode: %x, "
			"DPM none dump_mode: %x\n",
			dss_dpm.dump_mode, dss_dpm.dump_mode_none);
}

static ssize_t dss_dpm_none_dump_mode_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int mode;

	if (kstrtoint(buf, 10, &mode))
		return count;

	dbg_snapshot_set_dpm_none_dump_mode(mode);
	if (mode && dss_dpm.version) {
		dss_dpm.dump_mode_none = 1;
		dbg_snapshot_scratch_clear();
	} else {
		dss_dpm.dump_mode_none = 0;
		dbg_snapshot_scratch_reg(DSS_SIGN_SCRATCH);
	}

	dev_info(dss_desc.dev, "DPM: success to switch %sNONE_DUMP mode\n",
			dss_dpm.dump_mode_none ? "" : "NOT ");
	return count;
}

static ssize_t dss_logging_item_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int i;
	ssize_t n = 0;

	if (!dbg_snapshot_get_enable())
		goto out;

	n += scnprintf(buf + n, SZ_128, "item_count : %d\n", dss_bl->item_count);

	for (i = 0; i < dss_bl->item_count; i++) {
		n += scnprintf(buf + n, SZ_128, "%s paddr : 0x%X / size : 0x%X\n",
				dss_bl->item[i].name,
				dss_bl->item[i].paddr,
				dss_bl->item[i].size);
	}
out:
	return n;
}

DEVICE_ATTR_RW(dss_panic_to_wdt);
DEVICE_ATTR_RW(dss_dpm_none_dump_mode);
DEVICE_ATTR_RO(dss_logging_item);

static struct attribute *dss_sysfs_attrs[] = {
	&dev_attr_dss_dpm_none_dump_mode.attr,
	&dev_attr_dss_logging_item.attr,
	&dev_attr_dss_panic_to_wdt.attr,
	NULL,
};

static struct attribute_group dss_sysfs_group = {
	.attrs = dss_sysfs_attrs,
};

static const struct attribute_group *dss_sysfs_groups[] = {
	&dss_sysfs_group,
	NULL,
};

#ifdef CONFIG_SEC_PM_DEBUG
static ssize_t sec_log_read_all(struct file *file, char __user *buf,
				size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	size_t size;
	struct dbg_snapshot_item *item = &dss_items[DSS_ITEM_KERNEL_ID];

	if (sec_log_full)
		size = item->entry.size;
	else
		size = (size_t)(item->curr_ptr - item->head_ptr);

	if (pos >= size)
		return 0;

	count = min(len, size);

	if ((pos + count) > size)
		count = size - pos;

	if (copy_to_user(buf, item->head_ptr + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations sec_log_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_log_read_all,
};

static void sec_log_init(void)
{
	struct proc_dir_entry *entry;
	struct dbg_snapshot_item *item = &dss_items[DSS_ITEM_KERNEL_ID];

	if (!item->head_ptr)
		return;

	entry = proc_create("sec_log", 0440, NULL, &sec_log_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return;
	}

	proc_set_size(entry, item->entry.size);
}
#else
static inline void sec_log_init(void) {}
#endif /* CONFIG_SEC_PM_DEBUG */

static int dbg_snapshot_probe(struct platform_device *pdev)
{
	dbg_snapshot_init_desc(&pdev->dev);
	dbg_snapshot_init_proc();
	dbg_snapshot_init_utils();

	dbg_snapshot_set_enable(true);
	dbg_snapshot_boot_cnt();

	register_hook_logbuf(dbg_snapshot_hook_logger);
	register_hook_logger(dbg_snapshot_hook_logger);

	if (sysfs_create_groups(&pdev->dev.kobj, dss_sysfs_groups))
		dev_err(dss_desc.dev, "fail to register debug-snapshot sysfs\n");

	sec_log_init();

	dev_info(&pdev->dev, "%s successful.\n", __func__);
	return 0;
}

static const struct of_device_id dss_of_match[] = {
	{ .compatible	= "samsung,debug-snapshot" },
	{},
};
MODULE_DEVICE_TABLE(of, dss_of_match);

static struct platform_driver dbg_snapshot_driver = {
	.probe = dbg_snapshot_probe,
	.driver  = {
		.name  = "debug-snapshot",
		.of_match_table = of_match_ptr(dss_of_match),
	},
};

static __init int dbg_snapshot_init(void)
{
	struct device_node *root;

	if (dbg_snapshot_dt_scan_dpm()) {
		pr_err("%s: no such dpm node\n", __func__);
		return -ENODEV;
	}

	root = of_find_node_by_name(NULL, "dss");
	if (!root)
		return -ENODEV;

	dss_desc.root = root;
	if (dbg_snapshot_rmem_setup(root)) {
		pr_err("%s failed\n", __func__);
		return -ENODEV;
	}

	dbg_snapshot_fixmap();
	dbg_snapshot_init_dpm();
	dbg_snapshot_init_log();

	return 0;
}
early_initcall(dbg_snapshot_init);

static __init int dbg_snapshot_post_init(void)
{
	return platform_driver_register(&dbg_snapshot_driver);
}
core_initcall(dbg_snapshot_post_init);

static void __exit dbg_snapshot_exit(void)
{
	platform_driver_unregister(&dbg_snapshot_driver);
}
module_exit(dbg_snapshot_exit);

MODULE_DESCRIPTION("Debug-Snapshot");
MODULE_LICENSE("GPL v2");
