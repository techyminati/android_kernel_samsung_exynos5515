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

#ifndef ___SAMSUNG_LPD_MEM_H__
#define ___SAMSUNG_LPD_MEM_H__
#include <linux/types.h>
#include <asm/io.h>

struct lpd_reserved_memory {
	bool 			reserved;
	phys_addr_t 	base;
	size_t 			size;

#if (TEST_DRAM_ACCESS==1) /* Uses DMA */
	char __iomem 	*io_base;
	dma_addr_t		dma;
#endif
};

enum lpd_sram_state {
	LPD_SRAM_INVALID 	= 0,
	LPD_SRAM_LPD_ON		= 1<<0, // LPD region power on
	LPD_SRAM_CHUB_ON 	= 1<<1, // CHUB boot-up
	LPD_SRAM_CHUB_PG	= 1<<2, // CHUB power-gating
};

struct lpd_sram_memory {
	u8	state;
	phys_addr_t base;
	size_t 		size;
	void __iomem *io_base;
	struct lpd_sram_offset { // from 0
		u32	code;
		u32	ram;
		u32	ipc;
		u32	logbuf;
		u32	desc;
		u32	img;
		u32	dump;
	} off;	
};

#endif
