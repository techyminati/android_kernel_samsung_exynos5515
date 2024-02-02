#include "pmucal_chub.h"
#include "pmucal_rae.h"
#include <soc/samsung/exynos-pmu.h>

/**
 *  pmucal_chub_init - init chub.
 *		        exposed to PWRCAL interface.

 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_chub_init(void)
{
	int ret;

	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	if (!pmucal_chub_list.init) {
		pr_err("%s there is no sequence element for %s.\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	ret = pmucal_rae_handle_seq(pmucal_chub_list.init,
				pmucal_chub_list.num_init);
	if (ret) {
		pr_err("%s %s: error on handling sequences.\n",
				PMUCAL_PREFIX, __func__);
		return ret;
	}

	return 0;
}

/**
 *  pmucal_chub_cpu_status - get chub cpu status.
 *		        exposed to PWRCAL interface.

 *  Returns 1 when chub is run, 0 when chub is reset.
 *  Otherwise, negative error code.
 */
int pmucal_chub_cpu_status(void)
{
	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	if (!pmucal_chub_list.cpu_status) {
		pr_err("%s there is no sequence element for %s.\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}
	pmucal_rae_handle_seq(pmucal_chub_list.cpu_status,
				pmucal_chub_list.num_cpu_status);
	pr_err("chub_cpu_status: 0x%X\n", pmucal_chub_list.cpu_status->value);
	return pmucal_chub_list.cpu_status->value;
}

/**
 *  pmucal_chub_status - get chub status.
 *		        exposed to PWRCAL interface.

 *  Returns 1 when the chub is initialized, 0 when not.
 *  Otherwise, negative error code.
 */
int pmucal_chub_status(void)
{
	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	if (!pmucal_chub_list.status) {
		pr_err("%s there is no sequence element for %s.\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	pmucal_rae_handle_seq(pmucal_chub_list.status,
				pmucal_chub_list.num_status);

	pr_err("chub_status: 0x%X\n", pmucal_chub_list.status->value);

	if (pmucal_chub_list.status->value == 0x5)
		return 1;
	else
		return 0;
}

int pmucal_chub_on(void)
{
	int ret;

	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);
	ret = pmucal_rae_handle_seq(pmucal_chub_list.on, pmucal_chub_list.num_on);

	return ret;
}

/**
 *  pmucal_chub_reset_assert - reset assert chub.
 *		        exposed to PWRCAL interface.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_chub_reset_assert(void)
{
	int ret;

	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	pr_info("%s: CONFIG 0x%lx STATUS 0x%lx STATES 0x%lx", __func__,
		__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2100),
		__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2104),
		__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2108));

	if (!pmucal_chub_list.reset_assert) {
		pr_err("%s there is no sequence element for %s.\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	ret = pmucal_rae_handle_seq(pmucal_chub_list.reset_assert,
				pmucal_chub_list.num_reset_assert);
	if (ret) {
		pr_err("%s %s: error on handling sequences.\n",
				PMUCAL_PREFIX, __func__);
		pr_info("%s: CONFIG 0x%lx STATUS 0x%lx STATES 0x%lx", __func__,
			__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2100),
			__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2104),
			__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2108));
		return ret;
	}

	return 0;
}

/**
 *  pmucal_chub_reset_release - reset_release chub.
 *		        exposed to PWRCAL interface.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_chub_reset_release_config(void)
{
	int ret;

	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	if (!pmucal_chub_list.reset_release_config) {
		pr_err("%s there is no sequence element for %s.\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	ret = pmucal_rae_handle_seq(pmucal_chub_list.reset_release_config,
				pmucal_chub_list.num_reset_release_config);
	if (ret) {
		pr_err("%s %s: error on handling sequences.\n",
				PMUCAL_PREFIX, __func__);
		pr_info("%s: CONFIG 0x%lx STATUS 0x%lx STATES 0x%lx", __func__,
			__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2100),
			__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2104),
			__raw_readl(pmucal_chub_list.reset_assert->base_va + 0x2108));
		return ret;
	}

	return 0;
}

/**
 *  pmucal_chub_reset_release - reset_release chub.
 *		        exposed to PMUCAL interface.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_chub_reset_release(void)
{
	int ret;

	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	if (!pmucal_chub_list.reset_release) {
		pr_err("%s there is no sequence element for %s.\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	ret = pmucal_rae_handle_seq(pmucal_chub_list.reset_release,
				pmucal_chub_list.num_reset_release);
	if (ret) {
		pr_err("%s %s: error on handling sequences.\n",
				PMUCAL_PREFIX, __func__);
		return ret;
	}

	return 0;
}

/**
 *  pmucal_chub_reset_release - reset_release chub.
 *		        exposed to PMUCAL interface.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_chub_cm55_reset_release(void)
{
	int ret;

	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	if (!pmucal_chub_list.cm55_reset_release) {
		pr_err("%s there is no sequence element for %s.\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	ret = pmucal_rae_handle_seq(pmucal_chub_list.cm55_reset_release,
				pmucal_chub_list.num_cm55_reset_release);
	if (ret) {
		pr_err("%s %s: error on handling sequences.\n",
				PMUCAL_PREFIX, __func__);
		return ret;
	}

	return 0;
}

int pmucal_is_chub_regs(int reg)
{
	int i;
	int is_chub_regs = 0;

	for (i = 0; i < pmucal_chub_list.num_init; i++) {
		if (reg == pmucal_chub_list.init[i].base_pa + pmucal_chub_list.init[i].offset) {
			is_chub_regs = 1;
			goto out;
		}
	}

	for (i = 0; i < pmucal_chub_list.num_reset_assert; i++) {
		if (reg == pmucal_chub_list.reset_assert[i].base_pa + pmucal_chub_list.reset_assert[i].offset) {
			is_chub_regs = 1;
			goto out;
		}
	}

	for (i = 0; i < pmucal_chub_list.num_reset_release; i++) {
		if (reg == pmucal_chub_list.reset_release[i].base_pa + pmucal_chub_list.reset_release[i].offset) {
			is_chub_regs = 1;
			goto out;
		}
	}

out:
	return is_chub_regs;
}

/**
 *  pmucal_chub_initialize - Initialize function of PMUCAL SHUB common logic.
 *		            exposed to PWRCAL interface.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int __init pmucal_chub_initialize(void)
{
	int ret = 0;

	pr_info("%s%s()\n", PMUCAL_PREFIX, __func__);

	if (!pmucal_chub_list_size) {
		pr_err("%s %s: there is no chub list. aborting init...\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	ret = pmucal_rae_phy2virt(pmucal_chub_list.on,
				pmucal_chub_list.num_on);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting init...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}

	/* convert physical base address to virtual addr */
	ret = pmucal_rae_phy2virt(pmucal_chub_list.init,
				pmucal_chub_list.num_init);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting init...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}

	ret = pmucal_rae_phy2virt(pmucal_chub_list.status,
				pmucal_chub_list.num_status);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting status...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}

	ret = pmucal_rae_phy2virt(pmucal_chub_list.cpu_status,
				pmucal_chub_list.num_cpu_status);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting status...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}

	ret = pmucal_rae_phy2virt(pmucal_chub_list.reset_assert,
				pmucal_chub_list.num_reset_assert);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting reset_assert...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}

	ret = pmucal_rae_phy2virt(pmucal_chub_list.reset_release_config,
				pmucal_chub_list.num_reset_release_config);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting reset_release_config...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}

	ret = pmucal_rae_phy2virt(pmucal_chub_list.reset_release,
				pmucal_chub_list.num_reset_release);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting reset_release...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}
#if defined(CONFIG_SOC_S5E5515)
	ret = pmucal_rae_phy2virt(pmucal_chub_list.cm55_reset_release,
				pmucal_chub_list.num_cm55_reset_release);
	if (ret) {
		pr_err("%s %s: error on PA2VA conversion. aborting cm55_reset_release...\n",
				PMUCAL_PREFIX, __func__);
		goto out;
	}
#endif
out:
	return ret;
}
