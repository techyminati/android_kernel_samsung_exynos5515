#ifndef __ASV_EXYNOS9810_H__
#define __ASV_EXYNOS9810_H__

#include <linux/io.h>

#define ASV_TABLE_BASE	(0x10009000)
#define ID_TABLE_BASE	(0x10000000)
#define HPM_ASV_BASE (0x1000a030)
#define ALIVE_DVS_BASE (0x1000f078)

static struct dentry *rootdir;
struct __attribute__ ((packed)) asv_tbl_info {
	unsigned cpu_asv_group:4;
	unsigned g3d_asv_group:4;
	unsigned int_asv_group:4;
	unsigned mif_asv_group:4;
	unsigned aud_asv_group:4;
	unsigned disp_asv_group:4;
	unsigned cp_asv_group:4;

	unsigned asv_reserved1:20;

	signed cpu_modify_group:4;
	signed g3d_modify_group:4;
	signed int_modify_group:4;
	signed mif_modify_group:4;
	signed aud_modify_group:4;
	signed disp_modify_group:4;
	signed cp_modify_group:4;

	unsigned asv_reserved2:20;

	unsigned asv_table_version:7;
	unsigned asv_method:1;
	unsigned asb_version:7;
	unsigned asv_reserved3:1;
	unsigned asvg_version:4;
	unsigned asv_reserved4:12;
};

struct id_tbl_info {
	unsigned reserved_0;

	unsigned chip_id_2;

	unsigned chip_id_1:12;
	unsigned reserved_2_1:4;
	unsigned char reserved_ids_int:8;
	unsigned char ids_reserved:8;

	unsigned char ids_int_int_m:8;
	unsigned asb_version:8;
	unsigned char ids_cp:8;
	unsigned reserved_1:8;

	unsigned reserved_2:16;
	unsigned short sub_rev:4;
	unsigned short main_rev:4;
};

struct __attribute__ ((packed)) hpm_asv_info {
	unsigned reserved:16;
	unsigned alive_sum:8;
	unsigned small_large:8;
};

struct __attribute__ ((packed)) alive_dvs_info {
	unsigned reserved1:18;
	unsigned alive_dvs:4;
	unsigned reserved2:10;
};

#define ASV_INFO_ADDR_CNT	(sizeof(struct asv_tbl_info) / 4)
#define ID_INFO_ADDR_CNT	(sizeof(struct id_tbl_info) / 4)
#define HPM_ASV_INFO_ADDR_CNT	(sizeof(struct hpm_asv_info) / 4)
#define ALIVE_DVS_INFO_ADDR_CNT	(sizeof(struct alive_dvs_info) / 4)

static struct asv_tbl_info asv_tbl;
static struct id_tbl_info id_tbl;
static struct hpm_asv_info hpm_asv;
static struct alive_dvs_info alive_dvs;

int asv_get_grp(unsigned int id)
{
	int grp = -1;

	switch (id) {
	case MIF:
		grp = asv_tbl.mif_asv_group + asv_tbl.mif_modify_group;
		break;
	case INT:
		grp = asv_tbl.int_asv_group + asv_tbl.int_modify_group;
		break;
	case CPUCL0:
		grp = asv_tbl.cpu_asv_group + asv_tbl.cpu_modify_group;
		break;
	case G3D:
		grp = asv_tbl.g3d_asv_group + asv_tbl.g3d_modify_group;
		break;
	case DISP:
		grp = asv_tbl.disp_asv_group + asv_tbl.disp_modify_group;
		break;
	case AUD:
		grp = asv_tbl.aud_asv_group + asv_tbl.aud_modify_group;
		break;
	case CP:
		grp = asv_tbl.cp_asv_group + asv_tbl.cp_modify_group;
		break;
	default:
		pr_info("Un-support asv grp %d\n", id);
	}

	return grp;
}

int asv_get_ids_info(unsigned int id)
{
	int ids = 0;

	switch (id) {
	case G3D:
	case CPUCL0:
	case MIF:
	case DISP:
	case AUD:
	case INT:
		ids = id_tbl.ids_int_int_m;
		break;
	case CP:
		ids = id_tbl.ids_cp;
		break;
	case DVS:
		ids = alive_dvs.alive_dvs;
		break;
	default:
		pr_info("Un-support ids info %d\n", id);
	}

	return ids;
}

int asv_get_table_ver(void)
{
	return asv_tbl.asv_table_version;
}

void id_get_rev(unsigned int *main_rev, unsigned int *sub_rev)
{
	*main_rev = id_tbl.main_rev;
	*sub_rev =  id_tbl.sub_rev;
}

int id_get_product_line(void)
{
	return id_tbl.chip_id_1 >> 10;
}

int id_get_asb_ver(void)
{
	return id_tbl.asb_version;
}

int print_asv_table(char *buf)
{
	int r;

	r = sprintf(buf, "%s  cpu grp : %d, ids : %d\n", buf,
			                asv_tbl.cpu_asv_group + asv_tbl.cpu_modify_group,
			                                id_tbl.ids_int_int_m);
	r = sprintf(buf, "%s  g3d grp : %d, ids : %d\n", buf,
			                asv_tbl.g3d_asv_group + asv_tbl.g3d_modify_group,
			                                id_tbl.ids_int_int_m);
	r = sprintf(buf, "%s  mif grp : %d, ids : %d\n", buf,
			                asv_tbl.mif_asv_group + asv_tbl.mif_modify_group,
			                                id_tbl.ids_int_int_m);
	r = sprintf(buf, "%s  int grp : %d, ids : %d\n", buf,
			                asv_tbl.int_asv_group + asv_tbl.int_modify_group,
			                                id_tbl.ids_int_int_m);
	r = sprintf(buf, "%s  disp grp : %d, ids : %d\n", buf,
			                asv_tbl.disp_asv_group + asv_tbl.disp_modify_group,
			                                id_tbl.ids_int_int_m);
	r = sprintf(buf, "%s  cp grp : %d, ids : %d\n", buf,
			                asv_tbl.cp_asv_group + asv_tbl.cp_modify_group,
			                                id_tbl.ids_cp);

	r = sprintf(buf, "%s\n", buf);
	r = sprintf(buf, "%s  asv_table_version : %d\n", buf, asv_tbl.asv_table_version);
	r = sprintf(buf, "%s  asb_version : %d\n", buf, id_tbl.asb_version);
	r = sprintf(buf, "%s  chipid : 0x%03x%08x\n", buf, id_tbl.chip_id_1, id_tbl.chip_id_2);
	r = sprintf(buf, "%s  main revision : %d\n", buf, id_tbl.main_rev);
	r = sprintf(buf, "%s  sub revision : %d\n", buf, id_tbl.sub_rev);

	r = sprintf(buf, "%s  Alive+Alive_M+CHUB = %d.%02d\n", buf,
			(hpm_asv.alive_sum * 100) / 20 / 100, (hpm_asv.alive_sum * 100) / 20 % 100);
	r = sprintf(buf, "%s  dvs info : %d\n", buf, alive_dvs.alive_dvs);

	return r;
}
static struct kobject *asv_kobj;

static ssize_t
show_asv_info(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int r;

	r = print_asv_table(buf);

	return r;
}

static ssize_t
show_alive_sum_info(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int r;

	r = sprintf(buf, "%s  Alive+Alive_M+CHUB = %d.%02d\n", buf,
			(hpm_asv.alive_sum * 100) / 20 / 100, (hpm_asv.alive_sum * 100) / 20 % 100);

	return r;
}

static ssize_t
show_small_large_info(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int r;

	r = sprintf(buf, "%s Small_Large = %d\n", buf, hpm_asv.small_large);

	return r;
}
int asv_table_init(void)
{
	int i;
	unsigned int *p_table;
	unsigned int *regs;
	unsigned long tmp;
#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	char buf[1023] = {0,};
#endif

	p_table = (unsigned int *)&asv_tbl;

	for (i = 0; i < ASV_INFO_ADDR_CNT; i++) {
		exynos_smc_readsfr((unsigned long)(ASV_TABLE_BASE + 0x4 * i), &tmp);
		*(p_table + i) = (unsigned int)tmp;
	}

	p_table = (unsigned int *)&id_tbl;

	regs = (unsigned int *)ioremap(ID_TABLE_BASE, ID_INFO_ADDR_CNT * sizeof(int));
	for (i = 0; i < ID_INFO_ADDR_CNT; i++)
		*(p_table + i) = (unsigned int)regs[i];

	p_table = (unsigned int *)&hpm_asv;

	for (i = 0; i < HPM_ASV_INFO_ADDR_CNT; i++) {
		exynos_smc_readsfr((unsigned long)(HPM_ASV_BASE + 0x4 * i), &tmp);
		*(p_table + i) = (unsigned int)tmp;
	}

	p_table = (unsigned int *)&alive_dvs;

	regs = (unsigned int *)ioremap(ALIVE_DVS_BASE, ALIVE_DVS_INFO_ADDR_CNT * sizeof(int));
	for (i = 0; i < ALIVE_DVS_INFO_ADDR_CNT; i++)
		*(p_table + i) = (unsigned int)regs[i];

#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	print_asv_table(buf);
	pr_info("%s\n",buf);
#endif

	return asv_tbl.asv_table_version;
}

static struct kobj_attribute asv_info_attr = __ATTR(asv_info, 0444, show_asv_info, NULL);
static struct kobj_attribute alive_sum_info_attr = __ATTR(alive_alivem_chub, 0444, show_alive_sum_info, NULL);
static struct kobj_attribute small_large_attr = __ATTR(small_large_info, 0444, show_small_large_info, NULL);

int asv_debug_init(void)
{
	int ret = 0;

	asv_kobj = kobject_create_and_add("asv", kernel_kobj);

	if (!asv_kobj) {
		pr_info("%s: failed to create asv_kobj\n", __FILE__);
		return -EINVAL;
	}
	ret = sysfs_create_file(asv_kobj, &asv_info_attr.attr);
	if (ret) {
		pr_info("%s: failed to create sysfs\n", __FILE__);
		sysfs_remove_file(asv_kobj, &asv_info_attr.attr);
		return -EINVAL;
	}
	ret = sysfs_create_file(asv_kobj, &alive_sum_info_attr.attr);
	if (ret) {
		pr_info("%s: failed to create sysfs\n", __FILE__);
		sysfs_remove_file(asv_kobj, &asv_info_attr.attr);
		return -EINVAL;
	}
	ret = sysfs_create_file(asv_kobj, &small_large_attr.attr);
	if (ret) {
		pr_info("%s: failed to create sysfs\n", __FILE__);
		sysfs_remove_file(asv_kobj, &asv_info_attr.attr);
		return -EINVAL;
	}
	return 0;
}
subsys_initcall(asv_debug_init);
#endif
