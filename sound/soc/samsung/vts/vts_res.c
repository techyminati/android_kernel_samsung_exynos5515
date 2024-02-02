// SPDX-License-Identifier: GPL-2.0-or-later
/* sound/soc/samsung/vts/vts-res.c
 *
 * ALSA SoC - Samsung VTS driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/pinctrl/consumer.h>
#include <linux/sched/clock.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
//#include <sound/vts.h>

#include "vts_res.h"
#include "vts_dbg.h"

#define VTS_PIN_TRACE

#ifdef VTS_PIN_TRACE
#define VTS_PIN_TRACE_MAX (100)
struct vts_pin_trace {
	unsigned int idx;
	char name[16];
	long long updated;
};
static unsigned int p_vts_pin_trace_idx;
static struct vts_pin_trace p_vts_pin_trace[VTS_PIN_TRACE_MAX];
#endif

struct vts_port_request {
	int id;
	int gpio_func;
	int clk;
	bool enable;
};

static struct vts_port_request vts_port_req[VTS_PORT_PAD_MAX][VTS_PORT_ID_MAX];

int vts_cfg_gpio(struct device *dev, const char *name)
{
	struct vts_data *data = dev_get_drvdata(dev);
	struct pinctrl_state *pin_state;
	int ret;
#ifdef VTS_PIN_TRACE
	unsigned int idx = p_vts_pin_trace_idx % VTS_PIN_TRACE_MAX;
	struct vts_pin_trace *pin_trace = &p_vts_pin_trace[idx];
#endif

	mutex_lock(&data->mutex_pin);
#ifdef VTS_PIN_TRACE
	strncpy(pin_trace->name, name, sizeof(pin_trace->name) - 1);
	pin_trace->idx = p_vts_pin_trace_idx;
	pin_trace->updated = local_clock();

	vts_dev_dbg(dev, "%s(%d)(%d) %s %lld\n", __func__,
			idx,
			pin_trace->idx,
			pin_trace->name,
			pin_trace->updated);

	vts_dev_info(dev, "%s(%d) %s\n", __func__,
			pin_trace->idx,
			pin_trace->name);

	p_vts_pin_trace_idx++;
#else
	vts_dev_info(dev, "%s(%s)\n", __func__, name);
#endif
	pin_state = pinctrl_lookup_state(data->pinctrl, name);
	if (IS_ERR(pin_state)) {
		vts_dev_err(dev, "Couldn't find pinctrl %s\n", name);
		ret = -EINVAL;
	} else {
		ret = pinctrl_select_state(data->pinctrl, pin_state);
		if (ret < 0)
			vts_dev_err(dev,
				"Unable to configure pinctrl %s\n", name);
	}
	mutex_unlock(&data->mutex_pin);

	return ret;
}

static void vts_port_print(struct device *dev)
{
	struct vts_port_request *port_req;
	int i, j = 0;

	for (i = 0; i < VTS_PORT_PAD_MAX; i++) {
		for (j = 0; j < VTS_PORT_ID_MAX; j++) {
			port_req = &vts_port_req[i][j];
			vts_dev_info(dev, "%s[%d %d] %s %s clk%dHz en%d\n",
					__func__,
					i, j,
					port_req->id ? "VTS": "SLIF",
					port_req->gpio_func ? "DPDM" : "APDM",
					port_req->clk,
					port_req->enable);
		}
	}
}

void vts_port_init(struct device *dev)
{
	struct vts_port_request *port_req;
	int i, j = 0;

	for (i = 0; i < VTS_PORT_PAD_MAX; i++) {
		for (j = 0; j < VTS_PORT_ID_MAX; j++) {
			port_req = &vts_port_req[i][j];
			port_req->id = j;
			port_req->gpio_func = 0;
			port_req->clk = 0;
			port_req->enable = 0;

			vts_dev_info(dev, "%s[%d %d] %s %s clk%dHz en%d\n",
					__func__,
					i, j,
					port_req->id ? "VTS": "SLIF",
					port_req->gpio_func ? "DPDM" : "APDM",
					port_req->clk,
					port_req->enable);
		}
	}
}

int vts_port_cfg(struct device *dev,
		enum vts_port_pad pad,
		enum vts_port_id id,
		enum vts_dmic_sel gpio_func,
		bool enable)
{
	static DEFINE_MUTEX(port_lock);
	char pin_name[32] = {0,};
	int ret = 0;

	struct vts_port_request *port_req = &vts_port_req[pad][id];

	if (port_req->enable == enable) {
		vts_dev_warn(dev, "%s [%d %d]already %s\n",
				__func__,
				pad, id,
				(enable ? "enabled" : "disabled"));
		vts_port_print(dev);

		return 0;
	}

	/* update port_req infomation */
	mutex_lock(&port_lock);
	port_req->gpio_func = (int)gpio_func;
	port_req->enable = enable;

	if (enable) {
		/* HACK: should change data structure like list */
		if (vts_port_req[pad][VTS_PORT_ID_VTS].enable ||
				vts_port_req[pad][VTS_PORT_ID_SLIF].enable) {
			/* dmic0_default */
			snprintf(pin_name, sizeof(pin_name),
					"dmic%d_default", pad);
			ret = vts_cfg_gpio(dev, pin_name);
		}
	} else {
		/* HACK: should change data structure like list */
		if (!vts_port_req[pad][VTS_PORT_ID_VTS].enable &&
				!vts_port_req[pad][VTS_PORT_ID_SLIF].enable) {
			/* dmic_default */
			snprintf(pin_name, sizeof(pin_name),
					"dmic%d_idle", pad);
			ret = vts_cfg_gpio(dev, pin_name);
		}
	}

	mutex_unlock(&port_lock);

	vts_dev_info(dev, "%s pad%d id%d f%d en%d\n",
			__func__,
			pad, id,
			port_req->gpio_func,
			port_req->enable);

	return ret;
}

int vts_set_clk_src(struct device *dev, enum vts_clk_src clk_src)
{
	struct vts_data *data = dev_get_drvdata(dev);
	int ret = 0;

	vts_dev_info(dev, "%s clk src=%d\n", __func__, clk_src);
	data->clk_path = clk_src;

	return ret;
}

int vts_clk_set_rate(struct device *dev, unsigned long combination)
{
	struct vts_data *data = dev_get_drvdata(dev);
	unsigned long dmic_rate, dmic_sync, dmic_if, mux_dmic_if;
	int result;

	vts_dev_info(dev, "%s(%lu)\n", __func__, combination);

	switch (combination) {
	case 2:
		mux_dmic_if = 49152000;
		dmic_rate = 384000;
		dmic_sync = 384000;
		dmic_if = 768000;
		break;
	case 0:
		mux_dmic_if = 49152000;
		dmic_rate = 512000;
		dmic_sync = 512000;
		dmic_if = 1024000;
		break;
	case 1:
		mux_dmic_if = 49152000;
		dmic_rate = 768000;
		dmic_sync = 768000;
		dmic_if = 1536000;
		break;
	case 3:
		mux_dmic_if = 49152000;
		dmic_rate = 4096000;
		dmic_sync = 2048000;
		dmic_if = 4096000;
		break;
	case 4:
	case 5:
		mux_dmic_if = 294912000;
		dmic_rate = 3072000;
		dmic_sync = 3072000;
		dmic_if = 6144000;
		break;
	default:
		result = -EINVAL;
		goto out;
	}

#if defined(CONFIG_SOC_EXYNOS2100)
	result = clk_set_rate(data->mux_clk_dmic_if, mux_dmic_if);
	if (result < 0)
		vts_dev_err(dev, "clk_set_rate: %d\n", result);
	vts_dev_info(dev, "DMIC IF MUX Clock : %ld\n",
		clk_get_rate(data->mux_clk_dmic_if));
#endif

	result = clk_set_rate(data->clk_dmic_if, dmic_if);
	if (result < 0) {
		vts_dev_err(dev,
			"Failed to set rate of the clock %s\n", "dmic_if");
		goto out;
	}
	vts_dev_info(dev, "DMIC IF clock rate: %lu\n",
		clk_get_rate(data->clk_dmic_if));

	result = clk_set_rate(data->clk_dmic_sync, dmic_sync);
	if (result < 0) {
		vts_dev_err(dev,
			"Failed to set rate of the clock %s\n", "dmic_sync");
		goto out;
	}
	vts_dev_info(dev, "DMIC SYNC clock rate: %lu\n",
		clk_get_rate(data->clk_dmic_sync));

#if !(IS_ENABLED(CONFIG_SOC_EXYNOS2100) || \
		IS_ENABLED(CONFIG_SOC_S5E9815) || \
		IS_ENABLED(CONFIG_SOC_S5E5515))
	result = clk_set_rate(data->clk_dmic, dmic_rate);
	if (result < 0) {
		vts_dev_err(dev,
			"Failed to set rate of the clock %s\n", "dmic");
		goto out;
	}
	vts_dev_info(dev, "DMIC clock rate: %lu\n",
		clk_get_rate(data->clk_dmic));
#endif

out:
	return result;
}

