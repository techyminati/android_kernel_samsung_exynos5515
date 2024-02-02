/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos9810 devfreq
 */

#ifndef _DT_BINDINGS_S5E5515_DEVFREQ_H
#define _DT_BINDINGS_S5E5515_DEVFREQ_H
/* DEVFREQ TYPE LIST */
#define DEVFREQ_MIF			0
#define DEVFREQ_INT			1
#define DEVFREQ_DISP			2
#define DEVFREQ_AUD			3
#define DEVFREQ_TYPE_END		4

/* ESS FLAG LIST */
#define DSS_FLAG_INT	3
#define DSS_FLAG_MIF	4
#define DSS_FLAG_DISP	6
#define DSS_FLAG_AUD	8
#define DSS_FLAG_TNR	13

/* DEVFREQ GOV TYPE */
#define SIMPLE_INTERACTIVE 0

#endif
