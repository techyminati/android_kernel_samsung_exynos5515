/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * IPs Traffic Monitor(ITMON) Driver for Samsung S5E5515 SOC
 * By Hosung Kim (hosung0.kim@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/pm_domain.h>
#include <linux/printk.h>
#include <soc/samsung/exynos-itmon.h>
#include <soc/samsung/debug-snapshot.h>
#include <linux/sec_debug.h>
/* #define MULTI_IRQ_SUPPORT_ITMON */

#define OFFSET_TMOUT_REG		(0x2000)
#define OFFSET_REQ_R			(0x0)
#define OFFSET_REQ_W			(0x20)
#define OFFSET_RESP_R			(0x40)
#define OFFSET_RESP_W			(0x60)
#define OFFSET_ERR_REPT			(0x20)
#define OFFSET_PROT_CHK			(0x100)
#define OFFSET_NUM			(0x4)

#define REG_INT_MASK			(0x0)
#define REG_INT_CLR			(0x4)
#define REG_INT_INFO			(0x8)
#define REG_EXT_INFO_0			(0x10)
#define REG_EXT_INFO_1			(0x14)
#define REG_EXT_INFO_2			(0x18)
#define REG_EXT_USER			(0x80)

#define REG_DBG_CTL			(0x10)
#define REG_TMOUT_INIT_VAL		(0x14)
#define REG_TMOUT_FRZ_EN		(0x18)
#define REG_TMOUT_FRZ_STATUS		(0x1C)
#define REG_TMOUT_BUF_WR_OFFSET		(0x20)

#define REG_TMOUT_BUF_POINT_ADDR	(0x20)
#define REG_TMOUT_BUF_ID		(0x24)

#define REG_TMOUT_BUF_PAYLOAD_0		(0x28)
#define REG_TMOUT_BUF_PAYLOAD_1		(0x30)
#define REG_TMOUT_BUF_PAYLOAD_2		(0x34)
#define REG_TMOUT_BUF_PAYLOAD_3		(0x38)
#define REG_TMOUT_BUF_PAYLOAD_4		(0x3C)

#define REG_PROT_CHK_CTL		(0x4)
#define REG_PROT_CHK_INT		(0x8)
#define REG_PROT_CHK_INT_ID		(0xC)
#define REG_PROT_CHK_START_ADDR_LOW	(0x10)
#define REG_PROT_CHK_END_ADDR_LOW	(0x14)
#define REG_PROT_CHK_START_END_ADDR_UPPER	(0x18)

#define RD_RESP_INT_ENABLE		(1 << 0)
#define WR_RESP_INT_ENABLE		(1 << 1)
#define ARLEN_RLAST_INT_ENABLE		(1 << 2)
#define AWLEN_WLAST_INT_ENABLE		(1 << 3)
#define INTEND_ACCESS_INT_ENABLE	(1 << 4)

#define BIT_PROT_CHK_ERR_OCCURRED(x)	(((x) & (0x1 << 0)) >> 0)
#define BIT_PROT_CHK_ERR_CODE(x)	(((x) & (0x7 << 1)) >> 1)

#define BIT_ERR_CODE(x)			(((x) & (0xF << 28)) >> 28)
#define BIT_ERR_OCCURRED(x)		(((x) & (0x1 << 27)) >> 27)
#define BIT_ERR_VALID(x)		(((x) & (0x1 << 26)) >> 26)
#define BIT_AXID(x)			(((x) & (0xFFFF)))
#define BIT_AXUSER(x)			(((x) & (0xFFFFFFFF)))
#define BIT_AXUSER_PERI(x)		(((x) & (0xFFFF << 16)) >> 16)
#define BIT_AXBURST(x)			(((x) & (0x3)))
#define BIT_AXPROT(x)			(((x) & (0x3 << 2)) >> 2)
#define BIT_AXLEN(x)			(((x) & (0xF << 16)) >> 16)
#define BIT_AXSIZE(x)			(((x) & (0x7 << 28)) >> 28)

#define ERRCODE_SLVERR			(0)
#define ERRCODE_DECERR			(1)
#define ERRCODE_UNSUPPORTED		(2)
#define ERRCODE_POWER_DOWN		(3)
#define ERRCODE_UNKNOWN_4		(4)
#define ERRCODE_UNKNOWN_5		(5)
#define ERRCODE_TMOUT			(6)

#define DATA				(0)
#define PERI				(1)
#define BUS_PATH_TYPE			(2)

#define TRANS_TYPE_WRITE		(0)
#define TRANS_TYPE_READ			(1)
#define TRANS_TYPE_NUM			(2)

#define FROM_CP				(0)
#define FROM_CPU			(2)

#define CP_COMMON_STR			"CP_"
#define NOT_AVAILABLE_STR		"N/A"

#define TMOUT				(0xFFFFF)
#define TMOUT_TEST			(0x1)

#define PANIC_THRESHOLD			(10)
#define ERR_THRESHOLD			(5)

/* This value will be fixed */
#define INTEND_ADDR_START		(0)
#define INTEND_ADDR_END			(0)

enum err_type {
	FATAL = 0,
	DREX_TMOUT,
	CPU,
	CP,
	CHUB,
	IP,
	TYPE_MAX,
};

struct itmon_policy {
	char *name;
	int policy;
	bool error;
};

struct itmon_rpathinfo {
	unsigned int id;
	char *port_name;
	char *dest_name;
	unsigned int bits;
	unsigned int shift_bits;
};

struct itmon_masterinfo {
	char *port_name;
	unsigned int user;
	char *master_name;
	unsigned int bits;
};

struct itmon_nodegroup;

struct itmon_traceinfo {
	char *port;
	char *master;
	char *dest;
	unsigned long target_addr;
	unsigned int errcode;
	bool read;
	bool onoff;
	bool path_dirty;
	bool dirty;
	unsigned long from;
	int path_type;
	char buf[SZ_32];
	unsigned int axsize;
	unsigned int axlen;
	unsigned int axburst;
	unsigned int axprot;
	bool baaw_prot;
	struct list_head list;
};

struct itmon_tracedata {
	unsigned int int_info;
	unsigned int ext_info_0;
	unsigned int ext_info_1;
	unsigned int ext_info_2;
	unsigned int ext_user;
	unsigned int dbg_mo_cnt;
	unsigned int prot_chk_ctl;
	unsigned int prot_chk_info;
	unsigned int prot_chk_int_id;
	unsigned int offset;
	struct itmon_traceinfo *ref_info;
	bool logging;
	bool read;
};

struct itmon_nodeinfo {
	unsigned int type;
	char *name;
	unsigned int phy_regs;
	bool err_enabled;
	bool prot_chk_enabled;
	bool addr_detect_enabled;
	bool retention;
	unsigned int time_val;
	bool tmout_enabled;
	bool tmout_frz_enabled;
	void __iomem *regs;
	struct itmon_tracedata tracedata;
	struct itmon_nodegroup *group;
	struct list_head list;
};

struct itmon_nodegroup {
	char *name;
	unsigned int phy_regs;
	bool ex_table;
	struct itmon_nodeinfo *nodeinfo;
	unsigned int nodesize;
	unsigned int path_type;
	void __iomem *regs;
	int irq;
};

struct itmon_platdata {
	const struct itmon_rpathinfo *rpathinfo;
	const struct itmon_masterinfo *masterinfo;
	struct itmon_nodegroup *nodegroup;
	struct list_head infolist[TRANS_TYPE_NUM];
	struct list_head datalist[TRANS_TYPE_NUM];
	ktime_t last_time;
	bool cp_crash_in_progress;
	unsigned int sysfs_tmout_val;

	struct itmon_policy *policy;
	unsigned int err_cnt_node;
	unsigned int err_cnt;
	unsigned int err_cnt_by_cpu;
	unsigned int panic_threshold;
	bool probed;
};

struct itmon_dev {
	struct device *dev;
	struct itmon_platdata *pdata;
	struct of_device_id *match;
	int irq;
	int id;
	void __iomem *regs;
	spinlock_t ctrl_lock;
	struct itmon_notifier notifier_info;
};

struct itmon_panic_block {
	struct notifier_block nb_panic_block;
	struct itmon_dev *pdev;
};

static struct itmon_policy err_policy[] = {
	[FATAL]		= {"err_fatal",		0, false},
	[DREX_TMOUT]	= {"err_drex_tmout",	0, false},
	[CPU]		= {"err_cpu",		0, false},
	[CP]		= {"err_cp",		0, false},
	[CHUB]		= {"err_chub",		0, false},
	[IP]		= {"err_ip",		0, false},
};

const static struct itmon_rpathinfo rpathinfo[] = {
	/* 0x2000_0000 - 0xf_ffff_ffff */
	{0,	"DPU",		"CP_MEM",	0x7, 0},
	{1,	"CP_0",		"CP_MEM",	0x7, 0},
	{2,	"CP_1",		"CP_MEM",	0x7, 0},
	{3,	"WLBT",		"CP_MEM",	0x7, 0},

	{0,	"DPU",		"RT_MEM",	0x7, 0},
	{1,	"GNSS",		"RT_MEM",	0x7, 0},
	{2,	"CP_1",		"RT_MEM",	0x7, 0},
	{3,	"WLBT",		"RT_MEM",	0x7, 0},

	{0,	"ALIVE",	"NRT_MEM",	0xF, 0},
	{1,	"CHUB",		"NRT_MEM",	0xF, 0},
	{2,	"CSSYS",	"NRT_MEM",	0xF, 0},
	{3,	"FSYS",		"NRT_MEM",	0xF, 0},
	{4,	"GNSS",		"NRT_MEM",	0xF, 0},
	{5,	"G3D",		"NRT_MEM",	0xF, 0},
	{6,	"MFC",		"NRT_MEM",	0xF, 0},
	{7,	"CP_1",		"NRT_MEM",	0xF, 0},
	{8,	"PDMA",		"NRT_MEM",	0xF, 0},
	{9,	"RTIC",		"NRT_MEM",	0xF, 0},
	{10,	"SPDMA",	"NRT_MEM",	0xF, 0},
	{11,	"SSS",		"NRT_MEM",	0xF, 0},
	{12,	"SIREX_S",	"NRT_MEM",	0xF, 0},
	{13,	"VTS",		"NRT_MEM",	0xF, 0},
	{14,	"WLBT",		"NRT_MEM",	0xF, 0},

	{0,	"ALIVE",	"CORE_DP",	0x1F, 0},
	{1,	"CHUB",		"CORE_DP",	0x1F, 0},
	{2,	"CPUCL0",	"CORE_DP",	0x1F, 0},
	{3,	"CSSYS",	"CORE_DP",	0x1F, 0},
	{4,	"DPU",		"CORE_DP",	0x1F, 0},
	{5,	"FSYS",		"CORE_DP",	0x1F, 0},
	{6,	"GNSS",		"CORE_DP",	0x1F, 0},
	{7,	"G3D",		"CORE_DP",	0x1F, 0},
	{8,	"MFC",		"CORE_DP",	0x1F, 0},
	{9,	"CP_0",		"CORE_DP",	0x1F, 0},
	{10,	"CP_1",		"CORE_DP",	0x1F, 0},
	{11,	"PDMA",		"CORE_DP",	0x1F, 0},
	{12,	"RTIC",		"CORE_DP",	0x1F, 0},
	{13,	"SPDMA",	"CORE_DP",	0x1F, 0},
	{14,	"SSS",		"CORE_DP",	0x1F, 0},
	{15,	"VTS",		"CORE_DP",	0x1F, 0},
	{16,	"WLBT",		"CORE_DP",	0x1F, 0},

	{0,	"ALIVE",	"SIREX_MEM",	0x1F, 0},
	{1,	"CHUB",		"SIREX_MEM",	0x1F, 0},
	{2,	"CPUCL0",	"SIREX_MEM",	0x1F, 0},
	{3,	"CSSYS",	"SIREX_MEM",	0x1F, 0},
	{4,	"DPU",		"SIREX_MEM",	0x1F, 0},
	{5,	"FSYS",		"SIREX_MEM",	0x1F, 0},
	{6,	"GNSS",		"SIREX_MEM",	0x1F, 0},
	{7,	"G3D",		"SIREX_MEM",	0x1F, 0},
	{8,	"MFC",		"SIREX_MEM",	0x1F, 0},
	{9,	"CP_0",		"SIREX_MEM",	0x1F, 0},
	{10,	"CP_1",		"SIREX_MEM",	0x1F, 0},
	{11,	"PDMA",		"SIREX_MEM",	0x1F, 0},
	{12,	"RTIC",		"SIREX_MEM",	0x1F, 0},
	{13,	"SPDMA",	"SIREX_MEM",	0x1F, 0},
	{14,	"SSS",		"SIREX_MEM",	0x1F, 0},
	{15,	"VTS",		"SIREX_MEM",	0x1F, 0},
	{16,	"WLBT",		"SIREX_MEM",	0x1F, 0},

	{0,	"CPUCL0",	"CPU_MEM",	0, 0},
};

const static struct itmon_masterinfo masterinfo[] = {
	{"ALIVE",	0x0, /*XXXXX0*/ "APM",			0x1},

	{"CHUB",	0x0, /*XXXX0X*/ "CM4_CHUB",		0x2},
	{"CHUB",	0x2, /*XXXX10*/ "PDMA_CHUB",		0x2},
	{"CHUB",	0x3, /*XXXX11*/ "YAMIN_MCU_CHUB",	0x3},

	{"CSSYS",	0x0, /*XXXXX0*/ "CSSYS_DBG",		0x1},
	{"CSSYS",	0x1, /*XXXXX1*/ "DBGC",			0x1},

	{"DPU",		0x0, /*XXXX00*/ "DPU_DMA",		0x3},
	{"DPU",		0x2, /*XXXX10*/ "SYSMMU_DPU",		0x3},
	{"DPU",		0x1, /*XXX001*/ "ABOX_SPUS/SPUM",	0x7},
	{"DPU",		0x5, /*XXX101*/ "ABOX_CORTEXA32",	0x7},
	{"DPU",		0x3, /*XXXX11*/ "ABOX_SYSMMU",		0x3},

	{"FSYS",	0x0, /*XXXX00*/ "EMMC_EMBD",		0x3},
	{"FSYS",	0x1, /*XXXX01*/ "EMMC_CARD",		0x3},
	{"FSYS",	0x2, /*XXXX10*/ "USB20DRD_TOP",		0x3},

	{"MFC",		0x0, /*XXXX00*/ "MFC",			0x3},
	{"MFC",		0x2, /*XXXX10*/ "BL",			0x3},
	{"MFC",		0x1, /*XXXXX1*/ "SYSMMU",		0x1},

	{"GNSS",	0x2, /*X00010*/ "CM7_SXXCM7",		0x1F},
	{"GNSS",	0x0, /*X00000*/ "XDMAC_0",		0x1F},
	{"GNSS",	0x1, /*X00001*/ "XDMAC_1",		0x1F},

	{"CP_",		0x0, /*XXX000*/ "UCPUM",		0x7},
	{"CP_",		0x1, /*XXX001*/ "DMAtoL2",		0x7},
	{"CP_",		0x5, /*XXX101*/ "LCPUMtoL2",		0x7},
	{"CP_",		0x4, /*00X100*/ "LMACtoL2",		0x37},
	{"CP_",		0x2, /*000010*/ "CSXAP",		0x3F},
	{"CP_",		0x3, /*0XX011*/ "MDMtoL2",		0x27},
	{"CP_",		0x6, /*000110*/ "HMtoL2",		0x3F},

	{"G3D",		0x0,/*XXXXXX*/	"",			0x0},
	{"PDMA",	0x0,/*XXXXXX*/	"",			0x0},
	{"RTIC",	0x0,/*XXXXXX*/	"",			0x0},
	{"SIREX_S",	0x0,/*XXXXXX*/	"",			0x0},
	{"SSS",		0x0,/*XXXXXX*/	"",			0x0},
	{"VTS",		0x0,/*XXXXXX*/	"",			0x0},
	{"WLBT",	0x0,/*XXXXXX*/	"",			0x0},
	{"CPUCL0",	0x0,/*XXXXXX*/	"",			0x0},
	{"SPDMA",	0x0,/*XXXXXX*/	"",			0x0},
};

static struct itmon_nodeinfo vec_d[] = {
	{M_NODE, "ALIVE",	0x15003000, 1, 1, 0, 0},
	{M_NODE, "CHUB",	0x15013000, 1, 1, 0, 0},
	{M_NODE, "CPUCL0",	0x15023000, 1, 1, 0, 0},
	{M_NODE, "CSSYS",	0x15033000, 1, 1, 0, 0},
	{M_NODE, "DPU",		0x15043000, 1, 1, 0, 0},
	{M_NODE, "FSYS",	0x15053000, 1, 1, 0, 0},
	{M_NODE, "G3D",		0x15073000, 1, 1, 0, 0},
	{M_NODE, "GNSS",	0x15063000, 1, 1, 0, 0},
	{M_NODE, "MFC",		0x15083000, 1, 1, 0, 0},
	{M_NODE, "CP_0",	0x15093000, 1, 1, 0, 0},
	{M_NODE, "CP_1",	0x150A3000, 1, 1, 0, 0},
	{M_NODE, "PDMA",	0x150B3000, 1, 1, 0, 0},
	{M_NODE, "RTIC",	0x150C3000, 1, 1, 0, 0},
	{M_NODE, "SIREX_S",	0x150D3000, 1, 1, 0, 0},
	{M_NODE, "SPDMA",	0x150E3000, 1, 1, 0, 0},
	{M_NODE, "SSS",		0x150F3000, 1, 1, 0, 0},
	{M_NODE, "VTS",		0x15103000, 1, 1, 0, 0},
	{M_NODE, "WLBT",	0x15113000, 1, 1, 0, 0},
	{S_NODE, "CORE_DP",	0x15163000, 1, 1, 0, 0},
	{S_NODE, "CPU_MEM",	0x15123000, 1, 1, 0, 0},
	{S_NODE, "CP_MEM",	0x15133000, 1, 1, 0, 0},
	{S_NODE, "NRT_MEM",	0x15153000, 1, 1, 0, 0},
	{S_NODE, "RT_MEM",	0x15143000, 1, 1, 0, 0},
	{S_NODE, "SIREX_MEM",	0x15173000, 1, 1, 0, 0},
};

static struct itmon_nodeinfo vec_p[] = {
	{M_NODE, "CORE_DP",	0x15203000, 1, 1, 0, 0},
	{S_NODE, "ALIVE",	0x15233000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "CHUB",	0x15243000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "CORE_SFR",	0x15223000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "CORE_TREX",	0x15213000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "CPUCL0",	0x15253000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "DPU",		0x15263000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "FSYS",	0x15273000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "G3D",		0x152A3000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "GIC",		0x15283000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "GNSS",	0x15293000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "MFC",		0x152B3000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "MIF",		0x152C3000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "CP",		0x152D3000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "PERI",	0x152E3000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "VTS",		0x152F3000, 1, 1, 0, 0, TMOUT, 1},
	{S_NODE, "WLBT",	0x15303000, 1, 1, 0, 0, TMOUT, 1},
};

static struct itmon_nodegroup nodegroup[] = {
	{"BUS_DATA", 0x151B3000, 0, vec_d, ARRAY_SIZE(vec_d), DATA},
	{"BUS_PERI", 0x15333000, 0, vec_p, ARRAY_SIZE(vec_p), PERI},
};

const static char *itmon_pathtype[] = {
	"DATA Path transaction",
	"Configuration(SFR) Path transaction",
};

/* Error Code Description */
const static char *itmon_errcode[] = {
	"Error Detect by the Slave(SLVERR)",
	"Decode error(DECERR)",
	"Unsupported transaction error",
	"Power Down access error",
	"Unsupported transaction",
	"Unsupported transaction",
	"Timeout error - response timeout in timeout value",
	"Invalid errorcode",
};

const static char *itmon_node_string[] = {
	"M_NODE",
	"TAXI_S_NODE",
	"TAXI_M_NODE",
	"S_NODE",
};

const static char *itmon_cpu_node_string[] = {
	"CPUCL0",
	"M_CPU",
	"SCI_IRPM",
	"SCI_CCM",
	"CCI",
};

const static char *itmon_drex_node_string[] = {
	"RT_MEM",
	"NRT_MEM",
	"CPU_MEM",
	"SIREX_MEM",
	"DREX_IRPS",
	"BUS0_DP",
};

const static unsigned int itmon_invalid_addr[] = {
	0x03000000,
	0x04000000,
};

static struct itmon_dev *g_itmon;

/* declare notifier_list */
ATOMIC_NOTIFIER_HEAD(itmon_notifier_list);

static const struct of_device_id itmon_dt_match[] = {
	{.compatible = "samsung,s5e5515-itmon",
	 .data = NULL,},
	{},
};
MODULE_DEVICE_TABLE(of, itmon_dt_match);

static struct itmon_rpathinfo *itmon_get_rpathinfo
					(struct itmon_dev *itmon,
					 unsigned int id,
					 char *dest_name)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_rpathinfo *rpath = NULL;
	int i;

	if (!dest_name)
		return NULL;

	for (i = 0; i < (int)ARRAY_SIZE(rpathinfo); i++) {
		if (pdata->rpathinfo[i].id == (id & pdata->rpathinfo[i].bits)) {
			if (dest_name && !strncmp(pdata->rpathinfo[i].dest_name,
						  dest_name,
						  strlen(pdata->rpathinfo[i].dest_name))) {
				rpath = (struct itmon_rpathinfo *)&pdata->rpathinfo[i];
				break;
			}
		}
	}
	return rpath;
}

static struct itmon_masterinfo *itmon_get_masterinfo
				(struct itmon_dev *itmon,
				 char *port_name,
				 unsigned int user)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_masterinfo *master = NULL;
	unsigned int val;
	int i;

	if (!port_name)
		return NULL;

	for (i = 0; i < (int)ARRAY_SIZE(masterinfo); i++) {
		if (!strncmp(pdata->masterinfo[i].port_name,
				port_name, strlen(port_name))) {
			val = user & pdata->masterinfo[i].bits;
			if (val == pdata->masterinfo[i].user) {
				master = (struct itmon_masterinfo *)&pdata->masterinfo[i];
				break;
			}
		}
	}
	return master;
}

static void itmon_enable_addr_detect(struct itmon_dev *itmon,
			             struct itmon_nodeinfo *node,
			             bool enabled)
{
	/* This feature is only for M_NODE */
	unsigned int tmp, val;
	unsigned int offset = OFFSET_PROT_CHK;

	val = __raw_readl(node->regs + offset + REG_PROT_CHK_CTL);
	val |= INTEND_ACCESS_INT_ENABLE;
	__raw_writel(val, node->regs + offset + REG_PROT_CHK_CTL);

	val = ((unsigned int)INTEND_ADDR_START & U32_MAX);
	__raw_writel(val, node->regs + offset + REG_PROT_CHK_START_ADDR_LOW);

	val = (unsigned int)(((unsigned long)INTEND_ADDR_START >> 32) & U16_MAX);
	__raw_writel(val, node->regs + offset
				+ REG_PROT_CHK_START_END_ADDR_UPPER);

	val = ((unsigned int)INTEND_ADDR_END & 0xFFFFFFFF);
	__raw_writel(val, node->regs + offset
			+ REG_PROT_CHK_END_ADDR_LOW);
	val = ((unsigned int)(((unsigned long)INTEND_ADDR_END >> 32)
							& 0XFFFF0000) << 16);
	tmp = readl(node->regs + offset + REG_PROT_CHK_START_END_ADDR_UPPER);
	__raw_writel(tmp | val, node->regs + offset
				+ REG_PROT_CHK_START_END_ADDR_UPPER);

	dev_dbg(itmon->dev, "ITMON - %s addr detect  enabled\n", node->name);
}

static void itmon_enable_prot_chk(struct itmon_dev *itmon,
			       struct itmon_nodeinfo *node,
			       bool enabled)
{
	unsigned int offset = OFFSET_PROT_CHK;
	unsigned int val = 0;

	if (enabled)
		val = RD_RESP_INT_ENABLE | WR_RESP_INT_ENABLE |
		     ARLEN_RLAST_INT_ENABLE | AWLEN_WLAST_INT_ENABLE;

	__raw_writel(val, node->regs + offset + REG_PROT_CHK_CTL);

	dev_dbg(itmon->dev, "ITMON - %s hw_assert enabled\n", node->name);
}

static void itmon_enable_err_report(struct itmon_dev *itmon,
			       struct itmon_nodeinfo *node,
			       bool enabled)
{
	struct itmon_platdata *pdata = itmon->pdata;
	unsigned int offset = OFFSET_REQ_R;

	if (!pdata->probed || !node->retention)
		__raw_writel(1, node->regs + offset + REG_INT_CLR);

	/* enable interrupt */
	__raw_writel(enabled, node->regs + offset + REG_INT_MASK);

	/* clear previous interrupt of req_write */
	offset = OFFSET_REQ_W;
	if (pdata->probed || !node->retention)
		__raw_writel(1, node->regs + offset + REG_INT_CLR);
	/* enable interrupt */
	__raw_writel(enabled, node->regs + offset + REG_INT_MASK);

	/* clear previous interrupt of response_read */
	offset = OFFSET_RESP_R;
	if (!pdata->probed || !node->retention)
		__raw_writel(1, node->regs + offset + REG_INT_CLR);
	/* enable interrupt */
	__raw_writel(enabled, node->regs + offset + REG_INT_MASK);

	/* clear previous interrupt of response_write */
	offset = OFFSET_RESP_W;
	if (!pdata->probed || !node->retention)
		__raw_writel(1, node->regs + offset + REG_INT_CLR);
	/* enable interrupt */
	__raw_writel(enabled, node->regs + offset + REG_INT_MASK);

	dev_dbg(itmon->dev,
		"ITMON - %s error reporting enabled\n", node->name);
}

static void itmon_enable_timeout(struct itmon_dev *itmon,
			       struct itmon_nodeinfo *node,
			       bool enabled)
{
	unsigned int offset = OFFSET_TMOUT_REG;

	/* Enable Timeout setting */
	__raw_writel(enabled, node->regs + offset + REG_DBG_CTL);

	/* set tmout interval value */
	__raw_writel(node->time_val,
		     node->regs + offset + REG_TMOUT_INIT_VAL);

	if (node->tmout_frz_enabled) {
		/* Enable freezing */
		__raw_writel(enabled,
			     node->regs + offset + REG_TMOUT_FRZ_EN);
	}
	dev_dbg(itmon->dev, "ITMON - %s timeout enabled\n", node->name);
}

static void itmon_init(struct itmon_dev *itmon, bool enabled)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node;
	int i, j;

	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		node = pdata->nodegroup[i].nodeinfo;
		for (j = 0; j < pdata->nodegroup[i].nodesize; j++) {
			if (node[j].type == S_NODE && node[j].tmout_enabled)
				itmon_enable_timeout(itmon, &node[j], true);

			if (node[j].err_enabled)
				itmon_enable_err_report(itmon, &node[j], true);

			if (node[j].prot_chk_enabled)
				itmon_enable_prot_chk(itmon, &node[j], true);

			if (node[j].addr_detect_enabled)
				itmon_enable_addr_detect(itmon, &node[j], true);
		}
	}
}

void itmon_wa_enable(const char *node_name, int node_type, bool enabled)
{
	struct itmon_dev *itmon = g_itmon;
	struct itmon_platdata *pdata;
	struct itmon_nodeinfo *node;
	int i, j;

	if (!g_itmon)
		return;

	pdata = g_itmon->pdata;

	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		node = pdata->nodegroup[i].nodeinfo;
		for (j = 0; j < pdata->nodegroup[i].nodesize; j++) {
			if (node[j].err_enabled &&
				node[j].type == node_type &&
				!strncmp(node[j].name, node_name, strlen(node_name))) {
				itmon_enable_err_report(itmon, &node[j], enabled);
			}
		}
	}
}
EXPORT_SYMBOL(itmon_wa_enable);

void itmon_enable(bool enabled)
{
	if (g_itmon)
		itmon_init(g_itmon, enabled);
}
EXPORT_SYMBOL(itmon_enable);

static void itmon_post_handler_apply_policy(struct itmon_dev *itmon,
					    int ret_value)
{
	struct itmon_platdata *pdata = itmon->pdata;

	switch (ret_value) {
	case NOTIFY_OK:
		dev_err(itmon->dev, "all notify calls response NOTIFY_OK\n");
		break;
	case NOTIFY_STOP:
		dev_err(itmon->dev, "notify calls response NOTIFY_STOP, refer to notifier log\n");
		pdata->policy[IP].error = true;
		break;
	case NOTIFY_BAD:
		dev_err(itmon->dev, "notify calls response NOTIFY_BAD\n");
		pdata->policy[FATAL].error = true;
		break;
	}
}

static void itmon_post_handler_to_notifier(struct itmon_dev *itmon,
					   struct itmon_traceinfo *info,
					   unsigned int trans_type)
{
	int ret = 0;

	/* After treatment by port */
	if (!info->port || strlen(info->port) < 1)
		return;

	itmon->notifier_info.port = info->port;
	itmon->notifier_info.master = info->master;
	itmon->notifier_info.dest = info->dest;
	itmon->notifier_info.read = info->read;
	itmon->notifier_info.target_addr = info->target_addr;
	itmon->notifier_info.errcode = info->errcode;
	itmon->notifier_info.onoff = info->onoff;

	dev_err(itmon->dev, "     +ITMON Notifier Call Information\n\n");

	/* call notifier_call_chain of itmon */
	ret = atomic_notifier_call_chain(&itmon_notifier_list,
						0, &itmon->notifier_info);
	itmon_post_handler_apply_policy(itmon, ret);

	dev_err(itmon->dev, "\n     -ITMON Notifier Call Information\n"
		"-------------------------------------"
		"-------------------------------------\n");
}

static void itmon_post_handler_by_dest(struct itmon_dev *itmon,
				       struct itmon_traceinfo *info,
				       unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;

	if (!info->dest || strlen(info->dest) < 1)
		return;

	if (info->errcode == ERRCODE_TMOUT) {
		int i;

		for (i = 0; i < (int)ARRAY_SIZE(itmon_drex_node_string); i++) {
			if (!strncmp(info->dest, itmon_drex_node_string[i],
				strlen(itmon_drex_node_string[i]) - 1)) {
				pdata->policy[DREX_TMOUT].error = true;
				break;
			}

		}
	}
}

static void itmon_post_handler_by_master(struct itmon_dev *itmon,
					 struct itmon_traceinfo *info,
					 unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;

	/* After treatment by port */
	if (!info->port || strlen(info->port) < 1)
		return;

	if (test_bit(FROM_CPU, &info->from)) {
		ktime_t now, interval;

		now = ktime_get();
		interval = ktime_sub(now, pdata->last_time);
		pdata->last_time = now;
		pdata->err_cnt_by_cpu++;
		if (pdata->err_cnt_by_cpu > pdata->panic_threshold)
			pdata->policy[CPU].error = true;

		if (info->errcode == ERRCODE_TMOUT) {
			pdata->policy[FATAL].error = true;
			dev_err(itmon->dev,
				"Try to handle error, even CPU transaction detected - %s",
				itmon_errcode[info->errcode]);
		} else {
			dev_err(itmon->dev, "Skips CPU transaction detected - "
				"err_cnt_by_cpu: %u, interval: %lluns\n",
				pdata->err_cnt_by_cpu,
				(unsigned long long)ktime_to_ns(interval));
		}
	} else if (!strncmp(info->port, CP_COMMON_STR, strlen(CP_COMMON_STR))) {
		pdata->policy[CP].error = true;
	} else {
		pdata->err_cnt++;
	}
}

static void itmon_report_timeout(struct itmon_dev *itmon,
				struct itmon_nodeinfo *node,
				unsigned int trans_type)
{
	unsigned int id, payload0, payload1 = 0, payload2, payload3, payload4;
	unsigned int axid, user, valid, timeout, info;
	unsigned long addr;
	char *master_name, *port_name;
	struct itmon_rpathinfo *port;
	struct itmon_masterinfo *master;
	struct itmon_nodegroup *group = NULL;
	int i, num = (trans_type == TRANS_TYPE_READ ? SZ_128 : SZ_64);
	int rw_offset = (trans_type == TRANS_TYPE_READ ?
				0 : REG_TMOUT_BUF_WR_OFFSET);
	int path_offset = 0;

	if (!node)
		return;

	group = node->group;
	if (group && group->path_type == DATA)
		path_offset = SZ_4;

	pr_auto(ASL3,
		"-----------------------------------------"
		"-----------------------------------------\n");
	pr_auto(ASL3,
		"      ITMON Report (%s)\n", trans_type == TRANS_TYPE_READ ? "READ" : "WRITE");
	pr_auto(ASL3,
		"-----------------------------------------"
		"-----------------------------------------\n");
	pr_auto(ASL3,
		"      Timeout Error Occurred : Master --> %s\n", node->name);
	pr_auto(ASL3,
		"      TIMEOUT_BUFFER Information(NODE: %s)\n", node->name);
	pr_auto(ASL3,
		"	> NUM|   BLOCK|  MASTER|VALID|TIMEOUT|"
		"      ID| PAYLOAD0|         ADDRESS| PAYLOAD4|\n");

	for (i = 0; i < num; i++) {
		writel(i, node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_POINT_ADDR + rw_offset);
		id = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_ID + rw_offset);
		payload0 = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_0);
		if (path_offset == SZ_4)
			payload1 = readl(node->regs + OFFSET_TMOUT_REG +
					REG_TMOUT_BUF_PAYLOAD_1 + rw_offset);
		payload2 = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_2 + rw_offset);
		payload3 = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_3 + rw_offset);
		payload4 = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_4 + rw_offset);

		if (path_offset == SZ_4) {
			timeout = (payload0 & (unsigned int)(GENMASK(7, 4))) >> 4;
			user = payload1;
		} else {
			timeout = (payload0 & (unsigned int)(GENMASK(19, 16))) >> 16;
			user = (payload0 & (unsigned int)(GENMASK(15, 8))) >> 8;
		}

		addr = (((unsigned long)payload2 & GENMASK(15, 0)) << 32ULL);
		addr |= payload3;

		info = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_3 + rw_offset + path_offset);

		/* ID[5:0] 6bit : R-PATH */
		axid = id & GENMASK(5, 0);
		/* PAYLOAD[0] : Valid or Not valid */
		valid = payload0 & BIT(0);

		port = (struct itmon_rpathinfo *)
				itmon_get_rpathinfo(itmon, axid, node->name);
		if (port) {
			port_name = port->port_name;
			if (user) {
				master = (struct itmon_masterinfo *)
					itmon_get_masterinfo(itmon, port_name, user);
				if (master)
					master_name = master->master_name;
				else
					master_name = NOT_AVAILABLE_STR;
			} else {
				master_name = NOT_AVAILABLE_STR;
			}
		} else {
			port_name = NOT_AVAILABLE_STR;
			master_name = NOT_AVAILABLE_STR;
		}
		pr_auto(ASL3,
			"      > %03d|%8s|%8s|%5u|%7x|%08x|%08X|%016zx|%08x\n",
				i, port_name, master_name, valid, timeout,
				id, payload0, addr, payload4);
	}
	pr_auto(ASL3,
		"-----------------------------------------"
		"-----------------------------------------\n");
}

static unsigned int power(unsigned int param, unsigned int num)
{
	if (num == 0)
		return 1;
	return param * (power(param, num - 1));
}

static void itmon_report_prot_chk_rawdata(struct itmon_dev *itmon,
				     struct itmon_nodeinfo *node)
{
	unsigned int dbg_mo_cnt, prot_chk_ctl, prot_chk_info, prot_chk_int_id;
#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	char temp_buf[SZ_128];
#endif

	dbg_mo_cnt = __raw_readl(node->regs + OFFSET_PROT_CHK);
	prot_chk_ctl = __raw_readl(node->regs +
				OFFSET_PROT_CHK + REG_PROT_CHK_CTL);
	prot_chk_info = __raw_readl(node->regs +
				OFFSET_PROT_CHK + REG_PROT_CHK_INT);
	prot_chk_int_id = __raw_readl(node->regs +
				OFFSET_PROT_CHK + REG_PROT_CHK_INT_ID);

	/* Output Raw register information */
	pr_auto(ASL3,
		"-----------------------------------------"
		"-----------------------------------------\n");
	pr_auto(ASL3,
		"      Protocol Checker Raw Register Information"
		"(ITMON information)\n");
	pr_auto(ASL3,
		"      > %s(%s, 0x%08X)\n",
		node->name, itmon_node_string[node->type],
		node->phy_regs);
	pr_auto(ASL3,
		"      > REG(0x100~0x10C)      : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		dbg_mo_cnt,
		prot_chk_ctl,
		prot_chk_info,
		prot_chk_int_id);

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	snprintf(temp_buf, SZ_128, "%s/ %s/ 0x%08X/ %s/ 0x%08X, 0x%08X, 0x%08X, 0x%08X",
		"Protocol Error", node->name, node->phy_regs,
		itmon_node_string[node->type],
		dbg_mo_cnt,
		prot_chk_ctl,
		prot_chk_info,
		prot_chk_int_id);
	secdbg_exin_set_busmon(temp_buf);
#endif
}

static void itmon_report_rawdata(struct itmon_dev *itmon,
				 struct itmon_nodeinfo *node,
				 unsigned int trans_type)
{
	struct itmon_tracedata *data = &node->tracedata;

	/* Output Raw register information */
	dev_err(itmon->dev,
		"Raw Register Information -----------------------------\n"
		"      > %s(%s, 0x%08X)\n"
		"      > REG(0x08~0x18,0x80)   : 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n"
		"      > REG(0x100~0x10C)      : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		node->name, itmon_node_string[node->type],
		node->phy_regs + data->offset,
		data->int_info,
		data->ext_info_0,
		data->ext_info_1,
		data->ext_info_2,
		data->ext_user,
		data->dbg_mo_cnt,
		data->prot_chk_ctl,
		data->prot_chk_info,
		data->prot_chk_int_id);
}

#define DSS_OFFSET_CUSTOM	(0xF000)
static void itmon_report_traceinfo(struct itmon_dev *itmon,
				   struct itmon_traceinfo *info,
				   unsigned int trans_type)
{
#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	char temp_buf[SZ_128];
#endif

	if (!info->dirty)
		return;

	pr_auto(ASL3,
		"-----------------------------------------"
		"-----------------------------------------\n");
	pr_auto(ASL3,
		"      Transaction Information\n");
	pr_auto(ASL3,
		"      > Master         : %s %s\n",
		info->port, info->master ? info->master : "");
	pr_auto(ASL3,
		"      > Target         : %s\n",
		info->dest ? info->dest : NOT_AVAILABLE_STR);
	pr_auto(ASL3,
		"      > Target Address : 0x%lX %s\n",
		info->target_addr,
		info->baaw_prot == true ? "(BAAW Remapped address)" : "");
	pr_auto(ASL3,
		"      > Type           : %s\n",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE");
	pr_auto(ASL3,
		"      > Error code     : %s\n",
		itmon_errcode[info->errcode]);
	pr_auto(ASL3,
		"-----------------------------------------"
		"-----------------------------------------\n");

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	snprintf(temp_buf, SZ_128, "%s %s/ %s/ 0x%zx %s/ %s/ %s",
		info->port, info->master ? info->master : "",
		info->dest ? info->dest : NOT_AVAILABLE_STR,
		info->target_addr,
		info->baaw_prot == true ? "(BAAW Remapped address)" : "",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE",
		itmon_errcode[info->errcode]);
	secdbg_exin_set_busmon(temp_buf);
#endif

	dev_err(itmon->dev,
		"\n-----------------------------------------"
		"-----------------------------------------\n"
		"      > Size           : %u bytes x %u burst => %u bytes\n"
		"      > Burst Type     : %u (0:FIXED, 1:INCR, 2:WRAP)\n"
		"      > Level          : %s\n"
		"      > Protection     : %s\n"
		"      > Path Type      : %s\n\n",
		power(2, info->axsize), info->axlen + 1,
		power(2, info->axsize) * (info->axlen + 1),
		info->axburst,
		info->axprot & BIT(0) ? "Privileged" : "Unprivileged",
		info->axprot & BIT(1) ? "Non-secure" : "Secure",
		itmon_pathtype[info->path_type]);

	dbg_snapshot_set_str_offset(DSS_OFFSET_CUSTOM, SZ_128, "%s %s/ %s/ 0x%zx %s/ %s/ %s",
		info->port, info->master ? info->master : "",
		info->dest ? info->dest : NOT_AVAILABLE_STR,
		info->target_addr,
		info->baaw_prot == true ? "(BAAW Remapped address)" : "",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE",
		itmon_errcode[info->errcode]);
}

static void itmon_report_pathinfo(struct itmon_dev *itmon,
				  struct itmon_nodeinfo *node,
				  struct itmon_traceinfo *info,
				  unsigned int trans_type)

{
	struct itmon_tracedata *data = &node->tracedata;
	if (!info->path_dirty) {
		pr_auto(ASL3,
			"-----------------------------------------"
			"-----------------------------------------\n");
		pr_auto(ASL3,
			"      ITMON Report (%s)\n",
			trans_type == TRANS_TYPE_READ ? "READ" : "WRITE");
		pr_auto(ASL3,
			"-----------------------------------------"
			"-----------------------------------------\n");
		pr_auto(ASL3,
			"      PATH Information\n");
		info->path_dirty = true;
	}
	switch (node->type) {
	case M_NODE:
		pr_auto(ASL3,
			"      > %14s, %8s(0x%08X)\n",
			node->name, "M_NODE", node->phy_regs + data->offset);
		break;
	case T_S_NODE:
		pr_auto(ASL3,
			"      > %14s, %8s(0x%08X)\n",
			node->name, "T_S_NODE", node->phy_regs + data->offset);
		break;
	case T_M_NODE:
		pr_auto(ASL3,
			"      > %14s, %8s(0x%08X)\n",
			node->name, "T_M_NODE", node->phy_regs + data->offset);
		break;
	case S_NODE:
		pr_auto(ASL3,
			"      > %14s, %8s(0x%08X)\n",
			node->name, "S_NODE", node->phy_regs + data->offset);
		break;
	}
}

static int itmon_parse_cpuinfo(struct itmon_dev *itmon,
			       struct itmon_nodeinfo *node,
			       struct itmon_traceinfo *info,
			       unsigned int userbit)
{
	struct itmon_tracedata *find_data = NULL;
	int cluster_num, core_num, i;
	int ret = -1;

	for (i = 0; i < (int)ARRAY_SIZE(itmon_cpu_node_string); i++) {
		if (!strncmp(node->name, itmon_cpu_node_string[i],
			strlen(itmon_cpu_node_string[i]) - 1)) {
				core_num = (userbit & (GENMASK(2, 0)));
				cluster_num = 0;
				snprintf(info->buf, SZ_32 - 1,
						"CPU%d Cluster%d", core_num, cluster_num);
				find_data = &node->tracedata;
				find_data->ref_info = info;
				info->port = info->buf;
				set_bit(FROM_CPU, &info->from);
				ret = 0;
				break;
			}
	};

	return ret;
}

static void itmon_parse_traceinfo(struct itmon_dev *itmon,
				   struct itmon_nodeinfo *node,
				   unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_tracedata *data = &node->tracedata;
	struct itmon_traceinfo *new_info = NULL;
	struct itmon_masterinfo *master = NULL;
	struct itmon_rpathinfo *port = NULL;
	struct itmon_nodeinfo *find_node = NULL;
	struct itmon_tracedata *find_data = NULL;
	struct itmon_nodegroup *group = node->group;
	unsigned int errcode, axid;
	unsigned int userbit;
	int i;

	errcode = BIT_ERR_CODE(data->int_info);

	if (node->type == M_NODE) {
		if (pdata->err_cnt_node > 1) {
			if (!BIT_ERR_VALID(data->int_info))
				return;
			if (errcode != ERRCODE_DECERR)
				return;
		}
	} else {
		if (!BIT_ERR_VALID(data->int_info))
			return;
	}

	new_info = kzalloc(sizeof(struct itmon_traceinfo), GFP_ATOMIC);
	if (!new_info) {
		dev_err(itmon->dev, "failed to kmalloc for %s node\n",
			node->name);
		return;
	}
	axid = (unsigned int)BIT_AXID(data->int_info);
	if (group->path_type == DATA)
		userbit = BIT_AXUSER(data->ext_user);
	else
		userbit = BIT_AXUSER_PERI(data->ext_info_2);

	new_info->port = NULL;
	new_info->master = NULL;

	switch (node->type) {
	case S_NODE:
		new_info->dest = node->name;
		port = (struct itmon_rpathinfo *)
				itmon_get_rpathinfo(itmon, axid, node->name);
		list_for_each_entry(find_node, &pdata->datalist[trans_type], list) {
			if (find_node->type != M_NODE)
				continue;
			if (!itmon_parse_cpuinfo(itmon, find_node, new_info, userbit)) {
				break;
			} else if (port && !strncmp(find_node->name, port->port_name,
							strlen(port->port_name) - 1)) {
				new_info->port = port->port_name;
				master = (struct itmon_masterinfo *)
						itmon_get_masterinfo(itmon,
							new_info->port, userbit);
				if (master)
					new_info->master = master->master_name;

				find_data = &find_node->tracedata;
				find_data->ref_info = new_info;
				break;
			} else {
				if (!port) {
					for (i = 0; i < (int)ARRAY_SIZE(rpathinfo); i++) {
						if (!strncmp(find_node->name,
								pdata->rpathinfo[i].port_name,
								strlen(find_node->name))) {
								new_info->port = find_node->name;
								new_info->master = " ";
								find_data = &find_node->tracedata;
								find_data->ref_info = new_info;
								break;
						}
					}
				}
			}
		}
		if (!new_info->port) {
			new_info->port = "Failed to parsing";
			new_info->master = "Refer to Raw Node information";
		}
		break;
	case M_NODE:
		new_info->dest = "Refer to address";

		if (!itmon_parse_cpuinfo(itmon, node, new_info, userbit)) {
			break;
		} else {
			new_info->port = node->name;
			if (BIT_ERR_VALID(data->int_info)) {
				master = (struct itmon_masterinfo *)
					itmon_get_masterinfo(itmon,
							node->name, userbit);
				if (master)
					new_info->master = master->master_name;
				else
					new_info->master = " ";
			} else {
				new_info->master = " ";
			}
		}
		break;
	default:
		dev_err(itmon->dev,
			"Unknown Error - offset:%u\n", data->offset);
		kfree(new_info);
		return;
	}

	/* Last Information */
	new_info->path_type = group->path_type;
	new_info->target_addr =
		(((unsigned long)node->tracedata.ext_info_1
		& GENMASK(15, 0)) << 32ULL);
	new_info->target_addr |= node->tracedata.ext_info_0;
	new_info->errcode = errcode;
	new_info->dirty = true;
	new_info->axsize = BIT_AXSIZE(data->ext_info_1);
	new_info->axlen = BIT_AXLEN(data->ext_info_1);
	new_info->axburst = BIT_AXBURST(data->ext_info_2);
	new_info->axprot = BIT_AXPROT(data->ext_info_2);
	new_info->baaw_prot = false;

	for (i = 0; i < (int)ARRAY_SIZE(itmon_invalid_addr); i++) {
		if (new_info->target_addr == itmon_invalid_addr[i]) {
			new_info->baaw_prot = true;
			break;
		}
	}
	data->ref_info = new_info;
	list_add(&new_info->list, &pdata->infolist[trans_type]);
}

static void itmon_analyze_errnode(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *info, *next_info;
	struct itmon_tracedata *data;
	struct itmon_nodeinfo *node, *next_node;
	unsigned int trans_type;
	int i;

	/* Parse */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		list_for_each_entry(node, &pdata->datalist[trans_type], list) {
			if (node->type == S_NODE || node->type == M_NODE)
				itmon_parse_traceinfo(itmon, node, trans_type);
		}
	}

	/* Report */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		list_for_each_entry(info, &pdata->infolist[trans_type], list) {
			info->path_dirty = false;
			list_for_each_entry(node, &pdata->datalist[trans_type], list) {
				if (node) {
					data = &node->tracedata;
					if (data->ref_info == info)
						itmon_report_pathinfo(itmon, node, info, trans_type);
				}
			}
			itmon_report_traceinfo(itmon, info, trans_type);
		}
	}

	/* Report Raw all tracedata and Clean-up tracedata and node */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		for (i = M_NODE; i < NODE_TYPE; i++) {
			list_for_each_entry_safe(node, next_node,
				&pdata->datalist[trans_type], list) {
				if (i == node->type) {
					itmon_report_rawdata(itmon, node, trans_type);
					list_del(&node->list);
					kfree(node);
				}
			}
		}
	}

	/* Rest works and Clean-up traceinfo */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		list_for_each_entry_safe(info, next_info,
				&pdata->infolist[trans_type], list) {
			itmon_post_handler_to_notifier(itmon, info, trans_type);
			itmon_post_handler_by_dest(itmon, info, trans_type);
			itmon_post_handler_by_master(itmon, info, trans_type);
			list_del(&info->list);
			kfree(info);
		}
	}
}

static void itmon_collect_errnode(struct itmon_dev *itmon,
			    struct itmon_nodegroup *group,
			    struct itmon_nodeinfo *node,
			    unsigned int offset)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *new_node = NULL;
	unsigned int int_info, info0, info1, info2, user = 0;
	unsigned int prot_chk_ctl, prot_chk_info, prot_chk_int_id, dbg_mo_cnt;
	bool read = TRANS_TYPE_WRITE;
	bool req = false;

	int_info = __raw_readl(node->regs + offset + REG_INT_INFO);
	info0 = __raw_readl(node->regs + offset + REG_EXT_INFO_0);
	info1 = __raw_readl(node->regs + offset + REG_EXT_INFO_1);
	info2 = __raw_readl(node->regs + offset + REG_EXT_INFO_2);
	if (group->path_type == DATA)
		user = __raw_readl(node->regs + offset + REG_EXT_USER);

	dbg_mo_cnt = __raw_readl(node->regs + OFFSET_PROT_CHK);
	prot_chk_ctl = __raw_readl(node->regs +
				OFFSET_PROT_CHK + REG_PROT_CHK_CTL);
	prot_chk_info = __raw_readl(node->regs +
				OFFSET_PROT_CHK + REG_PROT_CHK_INT);
	prot_chk_int_id = __raw_readl(node->regs +
				OFFSET_PROT_CHK + REG_PROT_CHK_INT_ID);
	switch (offset) {
	case OFFSET_REQ_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_REQ_W:
		req = true;
		/* Only S-Node is able to make log to registers */
		break;
	case OFFSET_RESP_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_RESP_W:
		req = false;
		/* Only NOT S-Node is able to make log to registers */
		break;
	default:
		pr_auto(ASL3,
			"Unknown Error - node:%s offset:%u\n", node->name, offset);
		break;
	}

	new_node = kzalloc(sizeof(struct itmon_nodeinfo), GFP_ATOMIC);
	if (new_node) {
		/* Fill detected node information to tracedata's list */
		memcpy(new_node, node, sizeof(struct itmon_nodeinfo));
		new_node->tracedata.int_info = int_info;
		new_node->tracedata.ext_info_0 = info0;
		new_node->tracedata.ext_info_1 = info1;
		new_node->tracedata.ext_info_2 = info2;
		new_node->tracedata.ext_user = user;
		new_node->tracedata.dbg_mo_cnt = dbg_mo_cnt;
		new_node->tracedata.prot_chk_ctl = prot_chk_ctl;
		new_node->tracedata.prot_chk_info = prot_chk_info;
		new_node->tracedata.prot_chk_int_id = prot_chk_int_id;

		new_node->tracedata.offset = offset;
		new_node->tracedata.read = read;
		new_node->tracedata.ref_info = NULL;
		new_node->group = group;
		pdata->err_cnt_node++;
		if (BIT_ERR_VALID(int_info))
			node->tracedata.logging = true;
		else
			node->tracedata.logging = false;

		list_add(&new_node->list, &pdata->datalist[read]);
	} else {
		pr_auto(ASL3,
			"failed to kmalloc for %s node %x offset\n",
			node->name, offset);
	}
}

static int __itmon_search_node(struct itmon_dev *itmon,
			       struct itmon_nodegroup *group,
			       bool clear)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node = NULL;
	unsigned int val, offset, freeze;
	unsigned long vec, bit = 0;
	int i, ret = 0;

	if (group->phy_regs) {
		if (group->ex_table)
			vec = (unsigned long)__raw_readq(group->regs);
		else
			vec = (unsigned long)__raw_readl(group->regs);
	} else {
		vec = GENMASK(group->nodesize, 0);
	}

	if (!vec)
		goto exit;

	node = group->nodeinfo;

	for_each_set_bit(bit, &vec, group->nodesize) {
		/* exist array */
		for (i = 0; i < OFFSET_NUM; i++) {
			offset = i * OFFSET_ERR_REPT;
			/* Check Request information */
			val = __raw_readl(node[bit].regs + offset + REG_INT_INFO);
			if (BIT_ERR_OCCURRED(val)) {
				/* This node occurs the error */
				itmon_collect_errnode(itmon, group, &node[bit], offset);
				if (clear)
					__raw_writel(1, node[bit].regs
							+ offset + REG_INT_CLR);
				ret = true;
			}
		}
		/* Check H/W assertion */
		if (node[bit].prot_chk_enabled) {
			val = __raw_readl(node[bit].regs + OFFSET_PROT_CHK +
						REG_PROT_CHK_INT);
			/* if timeout_freeze is enable,
			 * PROT_CHK interrupt is able to assert without any information */
			if (BIT_PROT_CHK_ERR_OCCURRED(val) && (val & GENMASK(8, 1))) {
				itmon_report_prot_chk_rawdata(itmon, &node[bit]);
				pdata->policy[FATAL].error = true;
				ret = true;
			}
		}
		/* Check freeze enable node */
		if (node[bit].type == S_NODE && node[bit].tmout_frz_enabled) {
			val = __raw_readl(node[bit].regs + OFFSET_TMOUT_REG  +
						REG_TMOUT_FRZ_STATUS );
			freeze = val & (unsigned int)GENMASK(1, 0);
			if (freeze) {
				if (freeze & BIT(0))
					itmon_report_timeout(itmon, &node[bit], TRANS_TYPE_WRITE);
				if (freeze & BIT(1))
					itmon_report_timeout(itmon, &node[bit], TRANS_TYPE_READ);
				pdata->policy[FATAL].error = true;
				ret = true;
			}
		}
	}
exit:
	return ret;
}

static int itmon_search_node(struct itmon_dev *itmon,
			     struct itmon_nodegroup *group,
			     bool clear)
{
	int i, ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&itmon->ctrl_lock, flags);

	if (group) {
		ret = __itmon_search_node(itmon, group, clear);
	} else {
		/* Processing all group & nodes */
		for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
			group = &nodegroup[i];
			ret |= __itmon_search_node(itmon, group, clear);
		}
	}
	itmon_analyze_errnode(itmon);

	spin_unlock_irqrestore(&itmon->ctrl_lock, flags);
	return ret;
}

static void itmon_do_dpm_policy(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;
	int i;

	for (i = 0; i < TYPE_MAX; i++) {
		if (pdata->policy[i].error) {
			dev_err(itmon->dev, "%s: %s\n", __func__,
					pdata->policy[i].name);

			if (pdata->policy[i].policy == GO_S2D_ID)
				printk_safe_flush_on_panic();

			dbg_snapshot_do_dpm_policy(pdata->policy[i].policy);
			pdata->policy[i].error = false;
		}
	}
}

static void itmon_post_handler(struct itmon_dev *itmon)
{
	static unsigned long last_time;
	static unsigned long last_errcnt;
	unsigned long ts;
	unsigned long rem_nsec;
	unsigned long cur_time;
	unsigned long delta;

	cur_time = local_clock();
	delta = (last_time == 0) ? 0 : (cur_time - last_time);
	ts = last_time;
	rem_nsec = do_div(ts, 1000000000);

	dev_err(itmon->dev, "Before ITMON: [%5lu.%06lu], delta: %lu, last_errcnt: %d\n",
					(unsigned long)ts, rem_nsec / 1000, last_errcnt);

	/* delta < 1s */
	if (delta > 0 && delta < 1000000000UL) {
		last_errcnt++;
		if (last_errcnt > ERR_THRESHOLD)
			dbg_snapshot_do_dpm_policy(GO_S2D_ID);
	} else {
		last_errcnt = 0;
	}

	last_time = cur_time;
}

static irqreturn_t itmon_irq_handler(int irq, void *data)
{
	struct itmon_dev *itmon = (struct itmon_dev *)data;
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodegroup *group = NULL;
	bool ret;
	int i;

	/* Search itmon group */
	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		group = &pdata->nodegroup[i];
		dev_err(itmon->dev,
				"%s: %d irq, %s group, 0x%x vec",
				__func__, irq, group->name,
				group->phy_regs == 0 ? 0 : __raw_readl(group->regs));
	}

	ret = itmon_search_node(itmon, NULL, true);
	if (!ret) {
		dev_err(itmon->dev, "No errors found %s\n", __func__);
	} else {
		pr_auto(ASL3,
			"Error detected: err_cnt_node:%u, err_cnt:%u, err_cnt_by_cpu:%u\n",
			pdata->err_cnt_node, pdata->err_cnt, pdata->err_cnt_by_cpu);

		itmon_do_dpm_policy(itmon);
	}

	pdata->err_cnt_node = 0;

	itmon_post_handler(itmon);

	return IRQ_HANDLED;
}

void itmon_notifier_chain_register(struct notifier_block *block)
{
	atomic_notifier_chain_register(&itmon_notifier_list, block);
}
EXPORT_SYMBOL(itmon_notifier_chain_register);

static int itmon_logging_panic_handler(struct notifier_block *nb,
				     unsigned long l, void *buf)
{
	struct itmon_panic_block *itmon_panic = (struct itmon_panic_block *)nb;
	struct itmon_dev *itmon = itmon_panic->pdev;
	struct itmon_platdata *pdata = itmon->pdata;
	int ret;

	if (!IS_ERR_OR_NULL(itmon)) {
		/* Check error has been logged */
		ret = itmon_search_node(itmon, NULL, false);
		if (!ret) {
			dev_info(itmon->dev, "No errors found %s\n", __func__);
		} else {
			dev_err(itmon->dev,
				"Error detected: err_cnt:%u, err_cnt_by_cpu:%u\n",
				pdata->err_cnt, pdata->err_cnt_by_cpu);

			itmon_do_dpm_policy(itmon);
		}
	}
	return 0;
}

static void itmon_parse_dt(struct itmon_dev *itmon)
{
	struct device_node *np = itmon->dev->of_node;
	struct itmon_platdata *pdata = itmon->pdata;
	unsigned int val;
	int i;

	if (!of_property_read_u32(np, "panic_count", &val))
		pdata->panic_threshold = val;
	else
		pdata->panic_threshold = PANIC_THRESHOLD;

	dev_info(itmon->dev, "panic threshold: %d\n", pdata->panic_threshold);
	for (i = 0; i < TYPE_MAX; i++) {
		if (!of_property_read_u32(np, pdata->policy[i].name, &val))
			pdata->policy[i].policy = val;

		dev_info(itmon->dev, "policy: %s: [%d]\n",
				pdata->policy[i].name, pdata->policy[i].policy);
	}
}

static int itmon_probe(struct platform_device *pdev)
{
	struct itmon_dev *itmon;
	struct itmon_panic_block *itmon_panic = NULL;
	struct itmon_platdata *pdata;
	struct itmon_nodeinfo *node;
	unsigned int irq_option = 0, irq;
	char *dev_name;
	int ret, i, j;

	itmon = devm_kzalloc(&pdev->dev,
				sizeof(struct itmon_dev), GFP_KERNEL);
	if (!itmon)
		return -ENOMEM;

	itmon->dev = &pdev->dev;

	spin_lock_init(&itmon->ctrl_lock);

	pdata = devm_kzalloc(&pdev->dev,
				sizeof(struct itmon_platdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	itmon->pdata = pdata;
	itmon->pdata->masterinfo = masterinfo;
	itmon->pdata->rpathinfo = rpathinfo;
	itmon->pdata->nodegroup = nodegroup;
	itmon->pdata->policy = err_policy;

	itmon_parse_dt(itmon);

	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		dev_name = nodegroup[i].name;
		node = nodegroup[i].nodeinfo;

		if (nodegroup[i].phy_regs) {
			nodegroup[i].regs =
				devm_ioremap_nocache(&pdev->dev,
						 nodegroup[i].phy_regs, SZ_16K);
			if (nodegroup[i].regs == NULL) {
				dev_err(&pdev->dev,
					"failed to claim register region - %s\n",
					dev_name);
				return -ENOENT;
			}
		}
#ifdef MULTI_IRQ_SUPPORT_ITMON
		irq_option = IRQF_GIC_MULTI_TARGET;
#endif
		irq = irq_of_parse_and_map(pdev->dev.of_node, i);
		nodegroup[i].irq = irq;

		ret = devm_request_irq(&pdev->dev, irq,
				       itmon_irq_handler, irq_option, dev_name, itmon);
		if (ret == 0) {
			dev_info(&pdev->dev,
				 "success to register request irq%u - %s\n",
				 irq, dev_name);
		} else {
			dev_err(&pdev->dev, "failed to request irq - %s\n",
				dev_name);
			return -ENOENT;
		}

		for (j = 0; j < nodegroup[i].nodesize; j++) {
			node[j].regs = devm_ioremap_nocache(&pdev->dev,
						node[j].phy_regs, SZ_16K);
			if (node[j].regs == NULL) {
				dev_err(&pdev->dev,
					"failed to claim register region - %s\n",
					dev_name);
				return -ENOENT;
			}
		}
	}

	itmon_panic = devm_kzalloc(&pdev->dev, sizeof(struct itmon_panic_block),
				 GFP_KERNEL);

	if (!itmon_panic) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				    "panic handler data\n");
	} else {
		itmon_panic->nb_panic_block.notifier_call = itmon_logging_panic_handler;
		itmon_panic->pdev = itmon;
		atomic_notifier_chain_register(&panic_notifier_list,
					       &itmon_panic->nb_panic_block);
	}

	platform_set_drvdata(pdev, itmon);

	pdata->cp_crash_in_progress = false;

	itmon_init(itmon, true);

	g_itmon = itmon;
	pdata->probed = true;

	for (i = 0; i < TRANS_TYPE_NUM; i++) {
		INIT_LIST_HEAD(&pdata->datalist[i]);
		INIT_LIST_HEAD(&pdata->infolist[i]);
	}

	dev_info(&pdev->dev, "success to probe s5e5515 ITMON driver\n");

	return 0;
}

static int itmon_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int itmon_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int itmon_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct itmon_dev *itmon = platform_get_drvdata(pdev);
	struct itmon_platdata *pdata = itmon->pdata;

	/* re-enable ITMON if cp-crash progress is not starting */
	if (!pdata->cp_crash_in_progress)
		itmon_init(itmon, true);

	dev_info(dev, "%s\n", __func__);
	return 0;
}

static SIMPLE_DEV_PM_OPS(itmon_pm_ops, itmon_suspend, itmon_resume);
#define ITMON_PM	(itmon_pm_ops)
#else
#define ITM_ONPM	NULL
#endif

static struct platform_driver s5e5515_itmon_driver = {
	.probe = itmon_probe,
	.remove = itmon_remove,
	.driver = {
		   .name = "s5e5515-itmon",
		   .of_match_table = itmon_dt_match,
		   .pm = &itmon_pm_ops,
		   },
};
module_platform_driver(s5e5515_itmon_driver);

MODULE_DESCRIPTION("Samsung s5e5515 ITMON DRIVER");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:s5e5515-itmon");
