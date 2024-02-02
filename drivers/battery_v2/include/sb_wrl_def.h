/*
 * sb_wrl_def.h
 * Samsung Mobile Wireless Define Header
 *
 * Copyright (C) 2022 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SB_WRL_DEF_H
#define __SB_WRL_DEF_H __FILE__

/* Sec Battery Wireless Charger - charging mode */
enum sb_wrl_charge_mode {
	SB_WRL_CHG_MODE_CC = 0, /* DEFAULT */
	SB_WRL_CHG_MODE_CV,
	SB_WRL_CHG_MODE_HALF_BRIDGE,

	SB_WRL_CHG_MODE_MAX,
};

static inline const char *get_chg_mode_str(int chg_mode)
{
	switch (chg_mode) {
	case SB_WRL_CHG_MODE_CC:
		return "CC";
	case SB_WRL_CHG_MODE_CV:
		return "CV";
	case SB_WRL_CHG_MODE_HALF_BRIDGE:
		return "HB";
	}

	return "Unknown";
}

enum sb_wrl_pad_type {
	SB_WRL_PAD_TYPE_NORMAL	= 0,
	SB_WRL_PAD_TYPE_D2D,
	SB_WRL_PAD_TYPE_MULTI,
	SB_WRL_PAD_TYPE_ILLEGAL,
	SB_WRL_PAD_TYPE_FAST_MULTI,
	SB_WRL_PAD_TYPE_FACTORY
};

#define SB_WRL_FACTORY_MODE_TX_ID	0x5F

static inline bool is_normal_pad_type(int pad_type)
{
	return (pad_type == SB_WRL_PAD_TYPE_NORMAL);
}

static inline bool is_d2d_pad_type(int pad_type)
{
	return (pad_type == SB_WRL_PAD_TYPE_D2D);
}

static inline bool is_multi_pad_type(int pad_type)
{
	return (pad_type == SB_WRL_PAD_TYPE_MULTI) ||
		(pad_type == SB_WRL_PAD_TYPE_FAST_MULTI);
}

static inline bool is_illegal_pad_type(int pad_type)
{
	return (pad_type == SB_WRL_PAD_TYPE_ILLEGAL);
}

/* Sec Battery Wireless Charger - TX structure */
#define sb_wrl_tx_type	unsigned long long
struct sb_wrl_tx {
	union {
		/* max size = 8 bytes */
		sb_wrl_tx_type info_data;

		struct {
			unsigned	id : 8,
						recv_id : 1,
						pad_type : 4,
						phm : 1,
						phm_state : 1,
						phm_time : 16,
						auth : 1,
						auth_state : 4,
						rx_pwr_state : 1,
						fw_state : 1,
						tx_pwr_budg : 8; /* 45b */
		} info;
	};

	sb_wrl_tx_type chg_mode[SB_WRL_CHG_MODE_MAX];
};

static inline void sb_wrl_tx_init(struct sb_wrl_tx *tx)
{
	int i = SB_WRL_CHG_MODE_MAX - 1;

	if (tx == NULL)
		return;

	tx->info_data = 0;
	for (; i >= 0; i--)
		tx->chg_mode[i] = 0;
}

static inline void sb_wrl_tx_copy(struct sb_wrl_tx *dst, struct sb_wrl_tx *src)
{
	int i = SB_WRL_CHG_MODE_MAX - 1;

	if (dst == NULL)
		return;

	if (src == NULL) {
		sb_wrl_tx_init(dst);
		return;
	}

	dst->info_data = src->info_data;
	for (; i >= 0; i--)
		dst->chg_mode[i] = src->chg_mode[i];
}

static inline struct sb_wrl_tx *
sb_wrl_tx_find(struct sb_wrl_tx *tx_table, int size, int tx_id)
{
	int start, end, mid;

	if ((tx_id < 0) || (tx_id > 0xFF) ||
		(tx_table == NULL) || (size <= 0))
		return NULL;

	start = 0; end = size;
	while (start <= end) {
		mid = (start + end) / 2;

		if (tx_table[mid].info.id > tx_id)
			end = mid - 1;
		else if (tx_table[mid].info.id < tx_id)
			start = mid + 1;
		else
			return &tx_table[mid];
	}

	return NULL;
}

/* EN Pin */
#define VN_WPC_EN	"WPC-EN"

/* PHM */
#define VN_WPC_PHM	"WPC-PHM"

enum WPC_PHM_SET {
	WPC_PHM_OFF	= 0,
	WPC_PHM_ON,
	WPC_PHM_EPT_ON,
};

/* LDO */
#define VN_WPC_LDO	"WPC-LDO"

enum WPC_LDO_SET {
	WPC_LDO_ON = 0,
	WPC_LDO_OFF,
};

/* AP Mode */
#define VN_WPC_APM	"WPC-APM"

enum WPC_APM_SET {
	WPC_APM_NONE = 0,
	WPC_APM_NORMAL,
	WPC_APM_STORE,
	WPC_APM_BOOT,
	WPC_APM_WAKE_UP,
	WPC_APM_VOUT_ON,
	WPC_APM_VOUT_OFF,
	WPC_APM_TEST,
};

/* AP MODE	VOUT		VRECT
 * 0x00	:	5V			5.5V
 * 0x01	:	5V			5.35V
 * 0x02	:	VBAT + 0.3V	VOUT + 0.1V
 * 0x03	:	VBAT + 0.3V	VOUT + 0.1V
 * 0x04	:	VBAT + 0.3V	VOUT + 0.1V
 * 0x05	:	VBAT + 0.3V	VOUT + 0.1V
 * 0x06	:	4.8V		5.16V
 * 0x07	:	4.8V		5.28V
 * 0x08	:	4.9V		5.26V
 * 0x09	:	4.9V		5.38V
 * 0x0A	:	4.9V		5.46V
 * 0x0B	:	4.9V		5.55V
 * 0x0C	:	5.1V		5.5V
 * 0x0D	:	5.1V		5.6V
 * 0x0E	:	5.1V		5.7V
 * 0x0F	:	5.1V		5.8V
 * 0x10	:	4.6V		4.7V
 * 0x11	:	4.7V		4.8V
 * 0x12	:	WAKE UP
 * 0x13	:	VOUT OFF
 * 0xF0	:	5V
 */
enum sb_wrl_ap_mode {
	SB_WRL_AP_MODE_HOLD		= 0,
	SB_WRL_AP_MODE_BOOT,

	SB_WRL_AP_MODE_BATTERY,

	SB_WRL_AP_MODE_CC_CV	= 0x6,
	SB_WRL_AP_MODE_INCOMP_CC_CV,

	SB_WRL_AP_MODE_CC		= 0x10,
	SB_WRL_AP_MODE_INCOMP_CC,
	SB_WRL_AP_MODE_WAKEUP,
	SB_WRL_AP_MODE_INCOMP_PHP,

	SB_WRL_AP_MODE_MEASURE	= 0xF0
};

/* Vout Mode */
#define VN_WPC_VM		"WPC-VM"		/* VOUT MODE */

/* For DC */
#define VN_WPC_DC_HDR		"WPC-DC-HDR"	/* DC VOUT HEADROOM */
#define VN_WPC_ICHG			"WPC-ICHG"		/* DC ICHG */
#define VN_WPC_DC_HIGH_VOUT	"WPC-DC-HIGH-VOUT"	/* DC HIGH VOUT */

/* For Battery Tracking */
#define VN_WPC_LOW_VOUT		"WPC-LOW-VOUT"		/* BT LOW VOUT */
#define VN_WPC_BT_VBAT_HDR	"WPC-BT-VBAT-HDR"	/* BT VBAT HEADROOM */
#define VN_WPC_BT_HDR		"WPC-BT-HDR"		/* BT VOUT HEADROOM */

/* For Fixed Vout */
#define VN_WPC_VOUT		"WPC-VOUT"		/* FX VOUT */
#define VN_WPC_FX_HDR	"WPC-FX-HDR"	/* FX VOUT HEADROOM */

/*
 * Vout Mode Table
 * Value(Bit)	|		Vout		| 		Vrect
 * b000					5V					5.5V
 * b001			Vsys + ICHG * Ron_chg		Vout + DC Vout Headroom
 * b010			Vsys + VBAT Headroom		Vout + BT Vout Headroom
 * b011					vout				vout + FV Vout Headroom
 * b100				Vout Off(LDO off)		-
 * b101				5V -> Prev				5.5V -> Prev
 * b110					5V					5.5V
 * b111						AP Mode Table
 */
enum sb_wrl_vm_type {
	SB_WRL_VM_INIT	= 0,
	SB_WRL_VM_DC,		/* Direct Charging */
	SB_WRL_VM_BT,		/* Battery Tracking */
	SB_WRL_VM_FX,		/* Fixed Vout */
	SB_WRL_VM_OFF,		/* Vout Off */
	SB_WRL_VM_WAKE,		/* Wake Up */
	SB_WRL_VM_CM,		/* Current Measure */
	SB_WRL_VM_APM,		/* AP Mode */
	SB_WRL_VM_MAX
};

static inline const char *get_vout_mode_str(int mode)
{
	switch (mode) {
	case SB_WRL_VM_INIT:
		return "Init";
	case SB_WRL_VM_DC:
		return "DC";
	case SB_WRL_VM_BT:
		return "BT";
	case SB_WRL_VM_FX:
		return "FX";
	case SB_WRL_VM_OFF:
		return "Off";
	case SB_WRL_VM_WAKE:
		return "Wake";
	}

	return "Reserved";
}

#define SB_WRL_HDR_STEP		10
#define SB_WRL_VOUT_STEP	1
#define SB_WRL_ICHG_STEP	4

union sb_wrl_vm {
	/* max size = 8 bytes */
	sb_wrl_tx_type		value;

	struct {
		unsigned	type : 8,
					resv : 24;
	} base;

	struct {
		unsigned	type : 8,
					resv : 24,
					hdr : 8,
					ichg : 8,
					high_vout : 16;
	} dc;

	struct {
		unsigned	type : 8,
					resv : 24,
					low_vout : 16,
					vbat_hdr : 8,
					hdr : 8;
	} bt;

	struct {
		unsigned	type : 8,
					resv : 24,
					vout : 16,
					hdr : 8;
	} fx;
};

#if defined(CONFIG_OF)
#define sb_of_parse_wrl_vm(np, pdata, value) \
	({ \
		int ret = 0; \
		ret = of_property_read_u32_array(np, #value, \
			(unsigned int *)&pdata->value, 2); \
		ret; \
	})
#endif

#endif /* __SB_WRL_DEF_H */

