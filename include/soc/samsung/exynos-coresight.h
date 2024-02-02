/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef EXYNOS_CORESIGHT_H
#define EXYNOS_CORESIGHT_H

#if IS_ENABLED(CONFIG_EXYNOS_CORESIGHT)
int exynos_cs_stop_cpus(void);
#else /* CONFIG_EXYNOS_CORESIGHT */
#define exynos_cs_stop_cpus()		(-1)
#endif /* CONFIG_EXYNOS_CORESIGHT */
#endif
