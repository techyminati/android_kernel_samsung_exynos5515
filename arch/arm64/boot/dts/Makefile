# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos/s5e5515_nocp.dtb
dtbo-y += samsung/exynos5515-wisebl_na_open_00.dtbo
dtbo-y += samsung/exynos5515-wisebl_na_open_01.dtbo
dtbo-y += samsung/exynos5515-wisebl_na_open_02.dtbo
dtbo-y += samsung/exynos5515-wisebl_na_open_03.dtbo
dtbo-y += samsung/exynos5515-wisebl_na_open_10.dtbo
dtbo-y += samsung/exynos5515-wisebl_na_open_11.dtbo
dtbo-y += samsung/exynos5515-wisebl_na_open_15.dtbo

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
