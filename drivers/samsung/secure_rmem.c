/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>

static int __init exynos_secure_rmem_setup(struct reserved_mem *remem)
{

	pr_err("%s: Reserved memory: secure_rmem\n", __func__);

	return 0;
}
RESERVEDMEM_OF_DECLARE(secure_rmem, "exynos,secure", exynos_secure_rmem_setup);
#endif /* CONFIG_OF_RESERVED_MEM */

