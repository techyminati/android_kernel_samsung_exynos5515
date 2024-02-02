/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * memory interface for SRAM and DRAM reserved area
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "lpd.h"

#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/firmware.h>

int lpd_sram_acquire(struct lpd_device *lpd, struct platform_device *pdev)
{
	struct resource *res;

	NULL_CHECK(lpd);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sram");
	if (IS_ERR_OR_NULL(res)) {
		lpd_err("Failed to get sram\n");
		return -EINVAL;
	}

	/* unable to ioremap via devm_ioremap as ContextHub already initialized the SRAM */
	lpd->sram.io_base = ioremap(res->start, resource_size(res));
	if (IS_ERR(lpd->sram.io_base)) {
		lpd_err("Failed to ioremap of sram\n");
		return -EINVAL;
	}

	lpd->sram.base = res->start;
	lpd->sram.size = resource_size(res);
	lpd->sram.state = LPD_SRAM_INVALID;
	//lpd_sram = lpd->sram.io_base;
	lpd_info("sram: start(0x%x), size(%d KB 0x%x)\n",
						(u32)res->start, resource_size(res)/1024, resource_size(res));
	return 0;
}

int lpd_sram_release(struct lpd_device *lpd)
{
	NULL_CHECK(lpd);
	if(lpd->sram.io_base)
		iounmap(lpd->sram.io_base);
	return 0;
}


int lpd_sram_read(struct lpd_device *lpd, void *buffer, size_t size, int offset)
{
	void __iomem *sram;

	if (IS_ERR_OR_NULL(lpd)) {
		lpd_err("%s %d invalid argument\n", __func__, __LINE__);
		return -1;
	}

	if (buffer == NULL) {
		lpd_err("ERR: %s: invalid parameter 0x%x 0x%x\n", __func__, buffer, offset);
		return -1;
	}

	if (!IS_LPD_SRAM_AVAILABLE(lpd)) {
		lpd_err("ERR: %s: unable to access SRAM as unavailable (%x)\n", __func__, lpd->sram.state);
		return -1;
	}

	if (lpd->sram.io_base == NULL) {
		lpd_err("ERR: %s: unable to access SRAM as ioremap failed\n", __func__);
		return -1;
	}

	if (offset < 0) {
		lpd_err("ERR: %s: invalid parameter: negative offset 0x%x\n", __func__, offset);
		return -1;
	}

	if (offset + size > lpd->sram.size) {
		lpd_err("ERR: %s: unable to get sram by 0x%x + 0x%x > 0x%x\n", __func__, offset, size, lpd->sram.size);
		return -1;
	}

	sram = lpd->sram.io_base + offset;

	memcpy_fromio(buffer, sram, size);

	return size;
}

int lpd_sram_write(struct lpd_device *lpd, void *buffer, size_t size, int offset)
{
	void __iomem *sram;

	if(IS_ERR_OR_NULL(lpd)) {
		lpd_err("ERR: %s:%d invalid argument\n", __func__, __LINE__);
		return -1;
	}

	if (buffer == NULL) {
		lpd_err("ERR: %s: invalid parameter 0x%x 0x%x\n", __func__, buffer, offset);
		return -1;
	}

	if (!IS_LPD_SRAM_AVAILABLE(lpd)) {
		lpd_err("ERR: %s unable to access SRAM as unavailable (%x)\n", __func__, lpd->sram.state);
		return -1;
	}

	if (lpd->sram.io_base == NULL) {
		lpd_err("ERR: %s unable to access SRAM as ioremap failed\n", __func__);
		return -1;
	}

	if (offset < 0) {
		lpd_err("ERR: %s invalid parameter negative offset 0x%x\n", __func__, offset);
		return -1;
	}

	if (offset + size > lpd->sram.size) {
		lpd_err("ERR: %s unable to get sram by 0x%x + 0x%x > 0x%x\n", __func__, offset, size, lpd->sram.size);
		return -1;
	}

	sram = lpd->sram.io_base + offset;

	memcpy_toio(sram, buffer, size);

	return size;
}

#define VERIFY_SRAM 1
int lpd_sram_verify(struct lpd_device *lpd, void *buf, int off_d, int off_s, int size)
{
	char *buf_d;
	char *buf_s;
	int ret = -1;

	NULL_CHECK(lpd);

#if (VERIFY_SRAM ==0)
	return 0;
#endif

	if(buf == NULL) { // if buf is null, get data from rmem
		ret = lpd_rmem_read(lpd, &buf_d, size, off_d);
		ERR_CHECK(ret);
	}
	else
		buf_d = (char *)buf;

	buf_s = vmalloc(size);
	if(buf_s == NULL)
		goto done;
	ret = lpd_sram_read(lpd, buf_s, size, off_s);
	if(ret < 0)
		goto done;

	if(buf_d && buf_s){
		if(memcmp(buf_d, buf_s, size) == 0)
			lpd_dbg(" => [VERIFY OK] 0x%x 0x%x\n", buf_s[0], buf_s[1]);
		else
			lpd_err(" => [VERIFY NG] 0x%x 0x%x\n", buf_s[0], buf_s[1]);
	}
	else
		lpd_err(" => [Failed ]invalid parameter ret %d buf %x %x\n",ret, buf_d, buf_s);
done:
	if(buf_s)
		vfree(buf_s);

	return ret;
}

/*CM55 Reset-release would be done by CM4, this is just for test purpose*/

#if (TEST_M55_RESET_RELEASE)
#include <linux/delay.h>
#define SYSREG_CHUB_BASE 0x10C40000
#define USER_REG0 (0x0000)
#define USER_REG1 (0x0004)
#define USER_REG2 (0x0008)
#define USER_REG3 (0x000C)
#define USER_REG4 (0x0010)
#define BOOTADDR_S (0x1000)
#define BOOTADDR_NS (0x1004)
#define YAMIN_INST_CTRL (0x1008)
#define YAMIN_INST_FAULT0 (0x100C)
#define YAMIN_INST_FAULT1 (0x1010)
#define INTREQ_CM4_TO (0x1014)
#define INTREQ_YAMIN_TO (0x1018)
#define YAMIN_INST_CUR (0x1200)

#define PMU_BASE 0x12860000
#define CM55_CFG (0x3500)
#define CM55_STATUS (0x3504)
#define CM55_STATES (0x3508)
#define CM55_OPTION (0x350C)
#define CM55_CTRL (0x3510)
#endif

int lpd_sram_fw_on(struct lpd_device *lpd)
{
#if TEST_M55_RESET_RELEASE
	void __iomem *m55_pmu = NULL;
	void __iomem *m55_sysreg = NULL;

	NULL_CHECK(lpd);

	m55_pmu = ioremap(PMU_BASE, 0x4000);
	if (IS_ERR(m55_pmu)) {
		return -EINVAL;
	}

	m55_sysreg = ioremap(SYSREG_CHUB_BASE, 0x4000);
	if (IS_ERR(m55_sysreg)) {
		return -EINVAL;
	}

	lpd_info(">>> lpd fw PMU status %x states %x option %x ctrl %x \n",
						readl(m55_pmu+CM55_STATUS),readl(m55_pmu+CM55_STATES),
						readl(m55_pmu+CM55_OPTION),readl(m55_pmu+CM55_CTRL));
	lpd_info(">>> lpd fw SYSREG ctrl %x fault0 %x fault1 %x cur %x \n",
						readl(m55_sysreg+YAMIN_INST_CTRL),readl(m55_sysreg+YAMIN_INST_FAULT0),
						readl(m55_sysreg+YAMIN_INST_FAULT1),readl(m55_sysreg+YAMIN_INST_CUR));
	lpd_info(">>> lpd fw boot addr %x %x \n",
						readl(m55_sysreg+BOOTADDR_S), readl(m55_sysreg+BOOTADDR_NS));
	lpd_info(">>> lpd fw user %x %x %x %x %x \n",
						readl(m55_sysreg+USER_REG0),readl(m55_sysreg+USER_REG1),
						readl(m55_sysreg+USER_REG2),readl(m55_sysreg+USER_REG3),
						readl(m55_sysreg+USER_REG4));

	writel(LPD_FW_SRAM_AP_TO_FW(lpd->sram.base)>>7, m55_sysreg+BOOTADDR_S);
	writel(LPD_FW_SRAM_AP_TO_FW(lpd->sram.base)>>7, m55_sysreg+BOOTADDR_NS);

	lpd_info("###### LPD fw reset #####\n");
	writel(0x1, m55_pmu+CM55_CFG);

	lpd_info("<<< lpd fw status %x states %x option %x ctrl %x \n",
						readl(m55_pmu+CM55_STATUS),readl(m55_pmu+CM55_STATES),
						readl(m55_pmu+CM55_OPTION),readl(m55_pmu+CM55_CTRL));
	lpd_info("<<< lpd fw SYSREG ctrl %x fault0 %x fault1 %x cur %x \n",
						readl(m55_sysreg+YAMIN_INST_CTRL),readl(m55_sysreg+YAMIN_INST_FAULT0),
						readl(m55_sysreg+YAMIN_INST_FAULT1),readl(m55_sysreg+YAMIN_INST_CUR));
	lpd_info("<<< lpd fw boot addr %x %x \n",
						readl(m55_sysreg+BOOTADDR_S), readl(m55_sysreg+BOOTADDR_NS));
	lpd_info("<<< lpd fw user %x %x %x %x %x \n",
						readl(m55_sysreg+USER_REG0),readl(m55_sysreg+USER_REG1),
						readl(m55_sysreg+USER_REG2),readl(m55_sysreg+USER_REG3),
						readl(m55_sysreg+USER_REG4));

	iounmap(m55_sysreg);
	iounmap(m55_pmu);
#endif
	return 0;
}

int lpd_sram_fw_download(struct lpd_device *lpd)
{
	const struct firmware *entry;
	int ret;

	NULL_CHECK(lpd);

	lpd_dbg("%s +\n", __func__);
	ret = request_firmware(&entry, "lpd.core.bin", lpd->dev);
	if (ret) {
		lpd_err("cannot find lpd firmware\n");
		return -1;
	}

	ret = lpd_sram_write(lpd, (void *)entry->data, entry->size, 0);
	if (ret < 0) {
		lpd_err("ERR:%s: failed to write firmware\n", __func__);
		goto err_download;
	}

	return 0;
err_download:
	release_firmware(entry);
	return ret;
}

/*
[Memory MAP by M55 FW]

LPD: map: RMEM base           0x7a000000, size  16384 KB 0x01000000
LPD: map: SRAM CODE           0x00100000, size    128 KB 0x00020000
LPD: map: SRAM RAM            0x00120000, size     64 KB 0x00010000
LPD: map: SRAM IPC            0x00130000, size      0 KB 0x0000002c
LPD: map: SRAM LOGBUF         0x0013002c, size      7 KB 0x00001fd4
LPD: map: SRAM IMG DESC       0x00132000, size      3 KB 0x00000c00
LPD: map: SRAM IMG DATA       0x00132c00, size    821 KB 0x000cd400
LPD: map: SRAM DUMP           0x00132c00, size    821 KB 0x000cd400
*/

int lpd_sram_fw_info(struct lpd_device *lpd)
{
	int ret = 0;
	struct lpd_fw_info fwinfo;
	struct lpd_fw_memory_map *fwmap;

	NULL_CHECK(lpd);

	// need to wait for CHUB boot up
	ret = lpd_sram_read(lpd, &fwinfo, sizeof(fwinfo), LPD_FW_SRAM_INFO_OFFSET);
	ERR_CHECK(ret);

	if(strncmp(fwinfo.magic, LPD_FW_MAGIC, sizeof(fwinfo.magic)) != 0) {
		lpd_err("invalid magic (%s) !!!!\n", fwinfo.magic);
		return -1;
	}
	else {
		lpd_dbg("Exynos LPD Firmware\n");
		lpd_dbg("======================================================\n");
		lpd_dbg("## LPD SW info ##\n");
		lpd_dbg(" %-12s: %s\n", "MAGIC", fwinfo.magic);
		lpd_dbg(" %-12s: 0x%06x\n", "VERSION", fwinfo.version);
		lpd_dbg(" %-12s: 0x%06x\n", "META VERSION", fwinfo.meta_ver);
		lpd_dbg(" %-12s: %s\n", "SOC", fwinfo.soc);
		lpd_dbg(" %-12s: %s\n", "BUILD Date", fwinfo.time);
		lpd_dbg(" %-12s: %s\n", "BUILD User", fwinfo.name);
		lpd_dbg(" %-12s: %s\n", "BUILD Commit", fwinfo.commit);

		fwmap = &fwinfo.map;

		if(LPD_FW_DRAM_AP_TO_FW(lpd->rmem.base) != fwmap->rmem.base ||
										lpd->rmem.size!= fwmap->rmem.size){
			lpd_warn("mistmatch of rmem address %x vs %x, so update!\n",
						fwmap->rmem.base, LPD_FW_DRAM_AP_TO_FW(lpd->rmem.base));
			fwmap->rmem.base = LPD_FW_DRAM_AP_TO_FW(lpd->rmem.base);
			fwmap->rmem.size = lpd->rmem.size;
			ret = lpd_sram_write(lpd, &fwinfo, sizeof(fwinfo), LPD_FW_SRAM_INFO_OFFSET);
			ERR_CHECK(ret);
		}
	}

	#define GET_SIZE(field) (field ## _end - field ## _start)
	lpd_dbg("## Memory info ##\n");
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"RMEM base",
						fwmap->rmem.base,
						fwmap->rmem.size/1024,fwmap->rmem.size);
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"SRAM CODE",
						fwmap->sram.code_start,
						GET_SIZE(fwmap->sram.code)/1024, GET_SIZE(fwmap->sram.code));
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"SRAM RAM",
						fwmap->sram.ram_start,
						GET_SIZE(fwmap->sram.ram)/1024, GET_SIZE(fwmap->sram.ram));
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"SRAM IPC",
						fwmap->sram.ipc_start,
						GET_SIZE(fwmap->sram.ipc)/1024, GET_SIZE(fwmap->sram.ipc));
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"SRAM LOGBUF",
						fwmap->sram.logbuf_start,
						GET_SIZE(fwmap->sram.logbuf)/1024, GET_SIZE(fwmap->sram.logbuf));
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"SRAM IMG DESC",
						fwmap->sram.desc_start,
						GET_SIZE(fwmap->sram.desc)/1024, GET_SIZE(fwmap->sram.desc));
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"SRAM IMG DATA",
						fwmap->sram.img_start,
						GET_SIZE(fwmap->sram.img)/1024, GET_SIZE(fwmap->sram.img));
	lpd_dbg(" map: %-14s: 0x%08x, size %6d KB 0x%08x\n",
						"SRAM DUMP",
						fwmap->sram.dump_start,
						GET_SIZE(fwmap->sram.dump)/1024, GET_SIZE(fwmap->sram.dump));

	lpd_dbg("## Check Struct Size ##\n");
	lpd_dbg(" %-8s: %8d Byte\n", "Total", TOTAL_STRUCT_SIZE);
	lpd_dbg(" %-8s: %8d Byte\n", "INFO", sizeof(struct lpd_fw_info));
	lpd_dbg(" %-8s: %8d Byte\n", "INTF",  sizeof(struct lpd_fw_interface));
	lpd_dbg(" %-8s: %8d Byte\n", "IMG DESC", sizeof(struct lpd_fw_img_desc));
	lpd_dbg(" %-8s: %8d Byte\n", "DUMP", sizeof(struct lpd_fw_dump));
	lpd_dbg("======================================================\n\n");

	lpd->sram.off.code = 0;
	lpd->sram.off.ram = fwmap->sram.ram_start - fwmap->sram.code_start;
	lpd->sram.off.ipc = fwmap->sram.ipc_start - fwmap->sram.code_start;
	lpd->sram.off.logbuf = fwmap->sram.logbuf_start - fwmap->sram.code_start;
	lpd->sram.off.desc = fwmap->sram.desc_start - fwmap->sram.code_start;
	lpd->sram.off.img = fwmap->sram.img_start - fwmap->sram.code_start;
	lpd->sram.off.dump = fwmap->sram.dump_start - fwmap->sram.code_start;

	return ret;
}

int lpd_rmem_acquire(struct lpd_device *lpd, struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	struct device_node *node;
	struct device_node *np;
	static struct reserved_mem *lpd_rmem;

	NULL_CHECK(lpd);

	dev = lpd->dev;
	NULL_CHECK(dev);

	node = dev->of_node;
	NULL_CHECK(node);

	lpd->rmem.reserved = false;
	lpd->rmem.base = 0;

	ret = of_reserved_mem_device_init_by_idx(dev, node, 0);
	if (ret < 0) {
		lpd_err( "No reserved LPD memory in DT\n");
		return -1;
	}

	np = of_parse_phandle(node, "memory-region", 0);
	NULL_CHECK(np);

	lpd_rmem = of_reserved_mem_lookup(np);
	NULL_CHECK(lpd_rmem);

	lpd->rmem.reserved = true;
	lpd->rmem.base = lpd_rmem->base;
	lpd->rmem.size = lpd_rmem->size;

	lpd_info("LPD DRAM is reserved at addr %x size %x\n",
					lpd->rmem.base, lpd->rmem.size);
#if (TEST_DRAM_ACCESS==1) /* Uses DMA */
	ret = dma_alloc_from_dev_coherent(dev, lpd->rmem.size,
				&lpd->rmem.dma, &lpd->rmem.io_base);

	if(ret == 1 && lpd->rmem.io_base)
		ret = 0;
	else {
		lpd_err("no dev coherent memory, Try to adjust lpd_rmem size\n");
		ret = -1;
	}
#endif
	return ret;
}

int lpd_rmem_release(struct lpd_device *lpd)
{
	struct device *dev;
	NULL_CHECK(lpd);
	dev = lpd->dev;
	NULL_CHECK(dev);
#if (TEST_DRAM_ACCESS==1) /* Uses DMA */
	dma_release_from_dev_coherent(dev, get_order(lpd->rmem.size),
					&lpd->rmem.io_base);
#endif
	/* unreserve memory */
	of_reserved_mem_device_release(dev);

	/* update state */
	lpd->rmem.reserved = false;
	lpd->rmem.base = 0xdead;
	lpd->rmem.size = 0;

	return 0;
}

int lpd_rmem_read(struct lpd_device *lpd, char **buffer,
							size_t size, int offset)
{
	if(IS_ERR_OR_NULL(lpd)) {
		pr_err("%s %d invalid argument\n",__func__,__LINE__);
		return -1;
	}

	if(lpd->rmem.reserved == false || lpd->rmem.base == 0){
		lpd_err("unable to access RMEM as failed to reserve\n");
		return -1;
	}

	if (offset < 0) {
		lpd_err("ERR: %s: invalid parameter: negative offset 0x%x\n", __func__, offset);
		return -1;
	}

	if(offset + size > lpd->rmem.size){
		lpd_err("unable to get reserved dram by 0x%x + 0x%x > 0x%x\n",
											offset, size, lpd->rmem.size);
		return -1;
	}

	*buffer = (char *)phys_to_virt(lpd->rmem.base)+offset;
	if(*buffer == NULL){
		lpd_err("unable to get virtual address 0x%x\n",offset);
		return -1;
	}

	return size;
}

int lpd_rmem_to_sram(struct lpd_device *lpd, int off_d, int off_s, int size)
{
	int ret;
	char *buf;

	ret = lpd_rmem_read(lpd, &buf, size, off_d);
	ERR_CHECK(ret);

	ret = lpd_sram_write(lpd, buf, size, off_s);
	ERR_CHECK(ret);

	lpd_sram_verify(lpd, NULL, off_d, off_s, size);

	return ret;
}

static int lpd_rmem_dev_init(struct reserved_mem *rmem, struct device *dev)
{
	struct lpd_device *lpd = dev_get_drvdata(dev);

	lpd_info("%s +\n", __func__);
	if(lpd && rmem){
		lpd->rmem.base = rmem->base;
		lpd->rmem.size = rmem->size;
	}
	else
		lpd_info("lpd device is not yet initialized\n");
	lpd_info("%s -\n", __func__);

	return 0;
}

/*
 * rmem_device_release is called in of_reserved_mem_device_release function
 * when reserved memory is no longer required.
 */
static void lpd_rmem_dev_release(struct reserved_mem *rmem, struct device *dev)
{
	struct page *first = phys_to_page(PAGE_ALIGN(rmem->base));
	struct page *last = phys_to_page((rmem->base + rmem->size) & PAGE_MASK);
	struct page *page;

	lpd_info("%s: base=%pa, size=%pa, first=%pa, last=%pa\n",
			__func__, &rmem->base, &rmem->size, first, last);

	for (page = first; page != last; page++) {
		__ClearPageReserved(page);
		set_page_count(page, 1);
		__free_pages(page, 0);
		adjust_managed_page_count(page, 1);
	}
}

static const struct reserved_mem_ops lpd_rmem_ops = {
	.device_init	= lpd_rmem_dev_init,
	.device_release = lpd_rmem_dev_release,
};

static int __init lpd_rmem_dev_setup(struct reserved_mem *rmem)
{
	lpd_info("%s: base=%pa, size=%pa\n", __func__, &rmem->base, &rmem->size);

	rmem->ops = &lpd_rmem_ops;
	return 0;
}

RESERVEDMEM_OF_DECLARE(lpd_rmem, "exynos,lpd_rmem", lpd_rmem_dev_setup);

