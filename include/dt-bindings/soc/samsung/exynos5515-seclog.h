/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos Secure Log
 */

#ifndef EXYNOS5515_SECLOG_H
#define EXYNOS5515_SECLOG_H

/*
 * NOTE
 *
 * SECLOG_NUM_OF_CPU would be changed
 * at each SoC
 */
#define SECLOG_NUM_OF_CPU                       (2)

/* Secure log buffer information */
#define SECLOG_LOG_BUF_BASE                     (0x96800000)
#define SECLOG_LOG_BUF_SIZE                     (0x10000)
#define SECLOG_LOG_BUF_TOTAL_SIZE               (SECLOG_LOG_BUF_SIZE * SECLOG_NUM_OF_CPU)

#endif  /* EXYNOS5515_SECLOG_H */
