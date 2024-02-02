/* 
 * Copyright (C) 2020 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/arm-smccc.h>
#include <asm/cacheflush.h>
#include "svsmc.h"

#define SD_DEV_NAME "svsmc"
#define RBCNTINFO_SIZE	16
#define SIGNATURE_SIZE	512
#define SIGNFIELD_SIZE  RBCNTINFO_SIZE + SIGNATURE_SIZE
#define TOTAL_BUF_SIZE	60 * 1024 * 1024
 
static DEFINE_MUTEX(svsmc_mutex);
static void *sv_virt;
static struct reserved_mem *sv_rmem;
static unsigned int wd_enabled = 0;
atomic_t sv_mapped;

// SSS
uint64_t h_ctx_buf; /* phys addr of h_ctx */
uint64_t s_ctx_buf; /* phys addr of s_ctx */
uint64_t sig_buf; /* phys addr of sig */
uint64_t input_buf; /* phys addr of data chunk which is shared with wd */
uint64_t sha_buf; /* phys addr of sha data */
uint64_t final_buf; /* phys addr of final sha data for sig check */

sha_ctx *h_ctx;
sb_ctx *s_ctx;

static int exynos_cm_smc(uint64_t *arg0, uint64_t *arg1,
                         uint64_t *arg2, uint64_t *arg3)
{
	struct arm_smccc_res res;

	arm_smccc_smc(*arg0, *arg1, *arg2, *arg3, 0, 0, 0, 0, &res);

	*arg0 = res.a0;
	*arg1 = res.a1;
	*arg2 = res.a2;
	*arg3 = res.a3;

        return *arg0;
}

static int32_t sv_sha_init(unsigned int offset, unsigned int data_len)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	int ret = 0;

	memset(h_ctx, 0, sizeof(sha_ctx));

	h_ctx->func = SHA_INIT;
	h_ctx->alg = ALG_SHA512;

	__flush_dcache_area((void *)h_ctx, sizeof(sha_ctx));

	reg0 = SMC_CM_SSS_HASH;
	reg1 = h_ctx_buf;
	reg2 = CMD_HASH_NWD_IMG;
	reg3 = 0;

	ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);

	__inval_dcache_area((void *)h_ctx, sizeof(sha_ctx));

	memset(phys_to_virt((phys_addr_t)sig_buf), 0, SIGNFIELD_SIZE);
	memcpy(phys_to_virt((phys_addr_t)sig_buf), sv_virt + offset, data_len);
	__flush_dcache_area(phys_to_virt((phys_addr_t)sig_buf), SIGNFIELD_SIZE);

	return ret;
}

static int sv_sha_update(unsigned int offset, unsigned int data_len)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	uint64_t data_buf = input_buf;
	int ret = 0;

	data_buf += offset;

	h_ctx->func = SHA_UPDATE;
	h_ctx->input = data_buf;
	h_ctx->input_len = data_len;

	__flush_dcache_area((void *)h_ctx, sizeof(sha_ctx));
	__flush_dcache_area(phys_to_virt((phys_addr_t)data_buf), data_len);

	reg0 = SMC_CM_SSS_HASH;
	reg1 = h_ctx_buf;
	reg2 = CMD_HASH_NWD_IMG;
	reg3 = 0;

	ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);

	__inval_dcache_area((void *)h_ctx, sizeof(sha_ctx));

	return ret;
}

static int sv_sha_final(void)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	int ret = 0;

	memset(phys_to_virt((phys_addr_t)sha_buf), 0, SHA_DIGEST_LEN + RBCNTINFO_SIZE);

	h_ctx->func = SHA_FINAL;
	h_ctx->output = sha_buf;
	h_ctx->output_len = SHA_DIGEST_LEN;

	__flush_dcache_area((void *)h_ctx, sizeof(sha_ctx));
	__flush_dcache_area(phys_to_virt((phys_addr_t)sha_buf), SHA_DIGEST_LEN + RBCNTINFO_SIZE);

	reg0 = SMC_CM_SSS_HASH;
	reg1 = h_ctx_buf;
	reg2 = CMD_HASH_NWD_IMG;
	reg3 = 0;

	ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);
	
	__inval_dcache_area(phys_to_virt((phys_addr_t)sha_buf), SHA_DIGEST_LEN + RBCNTINFO_SIZE);

	return ret;
}

static int sv_sig_check_with_hash(void)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	int ret = 0;

	// calculate final hash
	memcpy(phys_to_virt((phys_addr_t)sha_buf) + SHA_DIGEST_LEN, 
					phys_to_virt(sig_buf), RBCNTINFO_SIZE);
	memset(h_ctx, 0, sizeof(sha_ctx));

	h_ctx->func = SHA_DIGEST;
	h_ctx->alg = ALG_SHA512;
	h_ctx->input = sha_buf;
	h_ctx->input_len = SHA_DIGEST_LEN + RBCNTINFO_SIZE;
	h_ctx->output = final_buf;
	h_ctx->output_len = SHA_DIGEST_LEN;

	__flush_dcache_area((void *)h_ctx, sizeof(sha_ctx));
	__flush_dcache_area(phys_to_virt((phys_addr_t)sha_buf), SHA_DIGEST_LEN + RBCNTINFO_SIZE);

	reg0 = SMC_CM_SSS_HASH;
	reg1 = h_ctx_buf;
	reg2 = CMD_HASH_NWD_IMG;
	reg3 = 0;

	ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);
	if (ret > 0) {
		pr_err("%s: final hash calculation error %d\n", __func__, ret);
		return ret;
	}

	__inval_dcache_area(phys_to_virt((phys_addr_t)final_buf), SHA_DIGEST_LEN);

	// finally, check sig
	memset(s_ctx, 0, sizeof(sb_ctx));
	memcpy(s_ctx->hash, phys_to_virt((phys_addr_t)final_buf), SHA_DIGEST_LEN);

	s_ctx->sign_field_ptr = sig_buf;
	s_ctx->sign_field_len = SIGNFIELD_SIZE;

	__flush_dcache_area((void *)s_ctx, sizeof(sb_ctx));

	reg0 = SMC_CM_SECURE_BOOT;
	reg1 = CMD_CHECK_SIGN_NWD_WITH_HASH;
	reg2 = s_ctx_buf;
	reg3 = 0;

	ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3); 

	return ret;
}

static int sv_sig_check(uint32_t data_len)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	int ret = 0;

	memset(s_ctx, 0, sizeof(sb_ctx));

	s_ctx->signed_img_ptr = input_buf;
	s_ctx->signed_img_len = data_len;

	__flush_dcache_area((void *)s_ctx, sizeof(sb_ctx));
	__flush_dcache_area(phys_to_virt((phys_addr_t)input_buf), data_len);

	reg0 = SMC_CM_SECURE_BOOT;
	reg1 = CMD_CHECK_SIGN_NWD;
	reg2 = s_ctx_buf;
	reg3 = 0;

	ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3); 

	return ret;
}

static int __init svsmc_get_wd_mode(char *arg)
{
	get_option(&arg, &wd_enabled);

	pr_info("wireless download mode is %sabled\n", wd_enabled ? "En" : "Dis");

	return 0;
}
early_param("androidboot.wirelessd", svsmc_get_wd_mode);

static void svsmc_free_rmem(void)
{
	int ret = 0;
	unsigned long f_pages = 0;

	if (!sv_rmem->base) {
		pr_err("reserved addr is null\n");
		goto out;
	}

	if (!sv_rmem->size) {
		pr_err("reserved size is zero\n");
		goto out;
	}

	ret = memblock_free(sv_rmem->base, sv_rmem->size);
	if (ret) {
		pr_err("%s: memblock_free failed (ret : %d)\n", __func__, ret);
		goto out;
	}

	f_pages = free_reserved_area((void *)(phys_to_virt(sv_rmem->base)),
				(void *)(phys_to_virt(sv_rmem->base) + sv_rmem->size),
				0, "svsmc");
	pr_info("%s: reserved mem %lu pages are freed\n", __func__, f_pages);

out:
	pr_info("%s: completed %d\n", __func__, ret);
}

static int __init svsmc_rmem_setup(struct reserved_mem *rmem)
{
        pr_info("%s: base=%pa, size=%pa\n", __func__, &rmem->base, &rmem->size);
        sv_rmem = rmem;
        return 0;
}
RESERVEDMEM_OF_DECLARE(sv_rmem, "exynos,svsmc", svsmc_rmem_setup);

static int svsmc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long pfn;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long len = vma->vm_end - vma->vm_start;

	if (offset >= sv_rmem->size)
		return -EINVAL;

	if (len > (sv_rmem->size - offset))
		return -EINVAL;

	pr_info("%s: mapping %ld bytes at offset %ld\n", __func__, len, offset);

	pfn = __phys_to_pfn(sv_rmem->base + offset);

	/**
	 * remap_pfn_range parameters
	 * @vma: user vma to map to
	 * @vma->vm_start: target user address to start at
	 * @pfn: physical address of kernel memory
	 * @len: size of map area
	 * @vma->vm_page_prot: page protection flags for this mapping
	 */
	if (remap_pfn_range(vma, vma->vm_start, pfn, vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		pr_err("%s: userspace vma mapping error\n", __func__);
		return -EAGAIN;
	}

	return 0;
}

static long svsmc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = TOTAL_BUF_SIZE;
	smc_ioctl_info i_info;
	void __user *ioargp = (void __user *)arg;

	copy_from_user(&i_info, ioargp, sizeof(smc_ioctl_info));

	if (i_info.data_len > TOTAL_BUF_SIZE) {
		pr_err("%s: data len is lager than buffer size\n", __func__);
		goto out;
	}

	switch (cmd) {
	case SHA_INIT:
		ret = sv_sha_init(i_info.offset, i_info.data_len);
		break;
	case SHA_UPDATE:
		ret = sv_sha_update(i_info.offset, i_info.data_len);
		break;
	case SHA_FINAL:
		ret = sv_sha_final();
		if (ret > 0) {
			pr_err("%s: sha update is failed\n", __func__);
			break;
		}

		ret = sv_sig_check_with_hash();
		break;
	case SHA_DIGEST:
		ret = sv_sig_check(i_info.data_len);
		break;
	default:
		pr_err("not implemented yet\n");
		break;
	}

out:
	pr_info("%s: cmd %u result is %x\n", __func__, cmd, ret);

	return ret;
}

static int svsmc_open(struct inode *inode, struct file *file)
{
	atomic_inc(&sv_mapped);

	return 0;
}

static int svsmc_release(struct inode *inode, struct file *file)
{
	if (atomic_dec_and_test(&sv_mapped))
		pr_info("map is released\n");

	return 0;
}

static const struct file_operations sv_ops = {
	.owner = THIS_MODULE,
	.mmap = svsmc_mmap,
	.unlocked_ioctl = svsmc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = svsmc_ioctl,
#endif
	.open = svsmc_open,
	.release = svsmc_release,
};

struct miscdevice svsmc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SD_DEV_NAME,
	.fops = &sv_ops,
};

static int __init svsmc_init(void)
{
	int ret;


	if (!wd_enabled) {
		pr_info("this is not wd mode. just free the memory and do not init\n");
		svsmc_free_rmem();
		return -EINVAL;	
	}
		
	ret = misc_register(&svsmc);
	if (ret) {
		pr_err("svsmc can't register misc on minor%d\n",
			MISC_DYNAMIC_MINOR);
		svsmc_free_rmem();
		return ret;
	}

	sv_virt = phys_to_virt(sv_rmem->base);

	// paddr
	input_buf = (uint64_t)(sv_rmem->base);
	h_ctx_buf = (uint64_t)(sv_rmem->base + 0x3C10000);
	sig_buf = (uint64_t)(sv_rmem->base + 0x3C20000);
	sha_buf = (uint64_t)(sv_rmem->base + 0x3C30000);
	s_ctx_buf = (uint64_t)(sv_rmem->base + 0x3C40000);
	final_buf = (uint64_t)(sv_rmem->base + 0x3C50000);
	
	h_ctx = (sha_ctx *)(phys_to_virt(sv_rmem->base + 0x3C10000));
	s_ctx = (sb_ctx *)(phys_to_virt(sv_rmem->base + 0x3C40000));

	ret = 0;

	return ret;
}

static void __exit svsmc_exit(void)
{
	svsmc_free_rmem();
	misc_deregister(&svsmc);
}

module_init(svsmc_init);
module_exit(svsmc_exit);

MODULE_AUTHOR("SEC");
MODULE_DESCRIPTION("Signature Verifier");
