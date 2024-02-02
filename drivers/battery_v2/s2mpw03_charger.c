/* drivers/battery_2/s2mpw03_charger.c
 * S2MPW03 Charger Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/mfd/samsung/s2mpw03.h>
#include <linux/mfd/samsung/s2mpw03-regulator.h>
#include <linux/version.h>
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/sec_batt.h>
#include "include/charger/s2mpw03_charger.h"
#include "include/sec_battery_vote.h"
#include "include/sec_vote_event.h"

#define ENABLE_MIVR 1
#define MINVAL(a, b) ((a <= b) ? a : b)
#define HEALTH_DEBOUNCE_CNT 3
#define EN_WRITE_REG 0

#ifndef EN_TEST_READ
#define EN_TEST_READ 1
#endif

struct s2mpw03_additional_curr {
	u8 fast_chg_reg_val;
	u8 add_chg_reg_val;
	u8 additional_chg_set;
	int chg_current;
};

struct s2mpw03_charger_log_data {
	u8	reg_data[32];
	int chg_status;
	int health;
};

struct s2mpw03_charger_data {
	struct s2mpw03_dev	*iodev;
	struct i2c_client       *client;
	struct device *dev;
	struct s2mpw03_platform_data *s2mpw03_pdata;
	struct delayed_work	charger_work;
	struct s2mpw03_muic *muic;

	struct workqueue_struct *charger_wqueue;
	struct power_supply	*psy_chg;
	struct power_supply_desc	psy_chg_desc;
	s2mpw03_charger_platform_data_t *pdata;

	struct sec_vote *in2bat_vote;

	int dev_id;
	int charging_current;
	int topoff_current;
	int charge_mode;
	int cable_type;
	bool is_charging;
	struct mutex io_lock;

	/* register programming */
	int reg_addr;
	int reg_data;

	bool ovp;
	bool factory_mode;

	int unhealth_cnt;
	int status;
	int onoff;

	/* s2mpw03 */
	int irq_det_bat;
	int irq_chg;
	int irq_tmrout;

	struct s2mpw03_additional_curr *chg_curr_table;
	int table_size;
	struct s2mpw03_charger_log_data log_data;

	bool top_off_event;
};

#if EN_WRITE_REG
/* write only area where mutex is required due to interference with the otp area */
#define is_w_only_reg(reg) ( \
			((reg >= 0x1C && reg <= 0x48) || (reg >= 0x4D && reg <= 0x51) || \
			(reg >= 0x59 && reg <= 0x5A)))
#endif

typedef struct _current_table_ {
	unsigned	cur_val : 12;
	unsigned	reg_val : 8;
	unsigned	add_sts : 4;
	unsigned	add_val : 8;
} current_table;

current_table gcur_table[72];
unsigned char gcur_table_size;

extern int factory_mode;

static enum power_supply_property s2mpw03_charger_props[] = {
};

static int s2mpw03_get_charging_health(struct s2mpw03_charger_data *charger);

static char *s2mpw03_supplied_to[] = {
	"s2mpw03-charger",
};

#if EN_WRITE_REG
/* due to the interference between the write only area and the otp area, */
/* the following function needs to be used when using the write function. */
int s2mpw03_chg_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	int ret;

	if (is_w_only_reg(reg))
		s2mpw03_ext_bus_set(true);

	ret = s2mpw03_write_reg(i2c, reg, value);

	if (is_w_only_reg(reg))
		s2mpw03_ext_bus_set(false);
	return ret;
}
#endif

static void s2mpw03_make_current_table(void)
{
	int cc_table[8] = { 35, 75, 150, 175, 200, 250, 300, 350 };
	current_table temp_table[72];
	int div_cur, i;
	u8 idx_table[702] = { 0 };
	u8 x, y;

	for (x = 0; x < 72; x += 9) {
		temp_table[x].cur_val = cc_table[x / 9];
		temp_table[x].reg_val = x / 9;
		temp_table[x].add_sts = 0;
		temp_table[x].add_val = 0;
		idx_table[temp_table[x].cur_val] = x + 1;

		for (y = 1; y <= 8; y++) {
			div_cur = (temp_table[x].cur_val * 1000 * y) / 8000;
			temp_table[x + y].cur_val = temp_table[x].cur_val + div_cur;
			temp_table[x + y].reg_val = x / 9;
			temp_table[x + y].add_sts = 1;
			temp_table[x + y].add_val = y - 1;
			idx_table[temp_table[x + y].cur_val] = x + y + 1;
		}
	}

	gcur_table_size = 0;
	for (i = 0; i < 702; i++) {
		if (idx_table[i] != 0)
			gcur_table[gcur_table_size++] = temp_table[idx_table[i] - 1];
	}
}

static current_table *s2mpw03_find_current_data(unsigned int chg_cur)
{
	current_table *pret = &gcur_table[0];
	int start, end, mid = 0;

	start = 0; end = gcur_table_size - 1;

	if (gcur_table[start].cur_val >= chg_cur)
		return &gcur_table[start];
	if (gcur_table[end].cur_val <= chg_cur)
		return &gcur_table[end];

	start += 1; end -= 1;
	while (start <= end) {
		mid = (start + end) / 2;
		pret = &gcur_table[mid];

		if (pret->cur_val < chg_cur)
			start = mid + 1;
		else if (pret->cur_val > chg_cur)
			end = mid - 1;
		else
			return pret;
	}

	return &gcur_table[end];
}

static void s2mpw03_test_read(struct s2mpw03_charger_data *charger)
{
	static int cnt = 0;
	u8 data;
	char str[256] = {0,};
	int i;
	bool print_log = false;

	for (i = S2MPW03_CHG_REG_INT1M; i <= S2MPW03_CHG_REG_CTRL12; i++) {
		s2mpw03_read_reg(charger->client, i, &data);
		if (data != charger->log_data.reg_data[i])
			print_log = true;
		charger->log_data.reg_data[i] = data;
		sprintf(str+strlen(str), "%x:%02x ", i, data);
	}

	/* print in2bat value */
	s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_CHG_SET4, &data);
	sprintf(str+strlen(str), "%x:%02x ", S2MPW03_CHG_REG_CHG_SET4, data);

	if (!print_log) {
		if (cnt++ > 10)
			print_log = true;
	}

	if (print_log) {
		pr_info("%s: %s\n", __func__, str);
		cnt = 0;
	}
}

static void s2mpw03_enable_charger_switch(struct s2mpw03_charger_data *charger,
		int onoff)
{
	if (onoff > 0) {
		pr_err("[DEBUG]%s: turn on charger\n", __func__);
		s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL1, EN_CHG_MASK, EN_CHG_MASK);
	} else {
		if (!charger->factory_mode) {
			pr_info("%s: turn off charger\n", __func__);

			/* if set S2MPW02_CHG_REG_CHG_OTP12 to 0x8B Q4 FET use small & big.
			 * if S2MPW02_CHG_REG_CHG_OTP12 is 0x8A while discharging,
			 * Vsys can drop when heavy load is occured.
			 */
			s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL1, 0, EN_CHG_MASK);
		}
	}
}

static void s2mpw03_set_regulation_voltage(struct s2mpw03_charger_data *charger,
		int float_voltage)
{
	unsigned int data;

	pr_err("[DEBUG]%s: float_voltage %d & top off event [%d]\n",
		__func__, float_voltage, (charger->top_off_event ? 1 : 0));

	if (float_voltage <= 4050)
		data = 0x0;
	else if (float_voltage > 4050 && float_voltage <= 4450)
		data = (int)((float_voltage - 4050) * 10 / 125);
	else
		data = 0x1F;

	s2mpw03_update_reg(charger->client,
			S2MPW03_CHG_REG_CTRL2, data << CV_SEL_SHIFT, CV_SEL_MASK);
}

static void s2mpw03_set_fast_charging_current(struct s2mpw03_charger_data *charger,
		int charging_current)
{
	current_table *pret = NULL;

	/* Additional charging path Off */
	s2mpw03_update_reg(charger->client,
		S2MPW03_CHG_REG_CTRL3, 0 << FORCED_ADD_ON_SHIFT, FORCED_ADD_ON_MASK);

	pret = s2mpw03_find_current_data(charging_current);
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL8,
		pret->reg_val << CC_SEL_SHIFT, CC_SEL_MASK);

	if (pret->add_sts) {
		/* Apply additional charging current */
		s2mpw03_update_reg(charger->client,
			S2MPW03_CHG_REG_CTRL4, (pret->add_val) << T_ADD_SHIFT, T_ADD_MASK);

		/* Additional charging path On */
		s2mpw03_update_reg(charger->client,
			S2MPW03_CHG_REG_CTRL3, FORCED_ADD_ON_MASK, FORCED_ADD_ON_MASK);
	}

	pr_err("[DEBUG]%s: chg_curr = %d, data = %d, add_sts = %d, add_val = %d\n",
		__func__, charging_current, pret->reg_val, pret->add_sts, pret->add_val);
}

static int s2mpw03_get_fast_charging_current(struct s2mpw03_charger_data *charger)
{
	int fast_charging_current[] = { 35, 75, 150, 175, 200, 250, 300, 350 };
	int ret, fcc = 0;
	u8 data;

	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_CTRL8, &data);
	if (ret < 0)
		return ret;

	data = (data & CC_SEL_MASK) >> CC_SEL_SHIFT;
	fcc = fast_charging_current[data];

	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_CTRL3, &data);
	if (ret < 0)
		return ret;

	if (data & FORCED_ADD_ON_MASK) {
		ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_CTRL4, &data);
		if (ret < 0)
			return ret;

		data = ((data & T_ADD_MASK) >> T_ADD_SHIFT) + 1;
		fcc = fcc + ((fcc * data * 1000) / 8000);
	}

	return fcc;
}

static int s2mpw03_get_current_eoc_setting(struct s2mpw03_charger_data *charger)
{
	int eoc_current[12] = {10, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60};
	int ret;
	u8 data;

	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_CTRL5, &data);
	if (ret < 0)
		return ret;

	data = (data & EOC_I_SEL_MASK) >> EOC_I_SEL_SHIFT;

	if (data > 0xb)
		data = 0xb;

	pr_err("[DEBUG]%s: data(0x%x), top-off current	%d\n", __func__, data, eoc_current[data]);

	return eoc_current[data];
}

static void s2mpw03_set_topoff_current(struct i2c_client *i2c, int current_limit)
{
	u8 data;

	if (current_limit < 10)
		data = 0x00;
	else if (current_limit > 60)
		data = 0x0B;
	else
		data = (current_limit / 5) - 1;

	pr_err("[DEBUG]%s: top-off current	%d, data=0x%x\n", __func__, current_limit, data);
	s2mpw03_update_reg(i2c, S2MPW03_CHG_REG_CTRL5, data << EOC_I_SEL_SHIFT,
			EOC_I_SEL_MASK);
}

#if ENABLE_MIVR
enum {
	S2MPW03_MIVR_4200MV = 0,
	S2MPW03_MIVR_4400MV,
	S2MPW03_MIVR_4600MV,
	S2MPW03_MIVR_4800MV,
};

/* charger input regulation voltage setting */
static void s2mpw03_set_mivr_level(struct s2mpw03_charger_data *charger)
{
	int mivr = S2MPW03_MIVR_4600MV;

	s2mpw03_update_reg(charger->client,
			S2MPW03_CHG_REG_CTRL5, mivr << IVR_V_SEL_SHIFT, IVR_V_SEL_MASK);
}
#endif /*ENABLE_MIVR*/

static int s2mpw03_set_in2bat_vote(void *data, int v)
{
	struct s2mpw03_charger_data *charger = data;

	/* Enable : 0, Disable : 1 */
	return s2mpw03_update_reg(charger->client,
		S2MPW03_CHG_REG_CHG_SET4,
		((!!v) ? (0) : (CHG_IN2BAT_MASK)), CHG_IN2BAT_MASK);
}

/* here is set init charger data */
static bool s2mpw03_chg_init(struct s2mpw03_charger_data *charger)
{
	dev_info(&charger->client->dev, "%s : DEV ID : 0x%x\n", __func__,
			charger->dev_id);

	/* Factory_mode initialization */
	charger->factory_mode = factory_mode;

	/* Top-off Timer Disable 0x16[6]=1 */
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL11,
		NO_TIMEOUT_30M_TM_MASK, NO_TIMEOUT_30M_TM_MASK);

	/* Watchdog timer disable */
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL8, 0x00, WDT_EN_MASK);

	/* Manual reset enable */
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL7, MRST_EN_MASK, MRST_EN_MASK);

	return true;
}

static bool s2mpw03_check_topoff_status(u8 chg_sts1, u8 chg_sts3, int topoff)
{
	union power_supply_propval value;

	if ((chg_sts1 & TOP_OFF_STATUS_MASK) && (chg_sts3 & CV_OK_STATUS_MASK))
		return true;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property("s2mpw03-fuelgauge", get,
		POWER_SUPPLY_PROP_CURRENT_AVG, value);

	/* topoff w/a*/
	return ((value.intval > 0) && (value.intval <= (topoff / 2)));
}

static int s2mpw03_get_charging_status(struct s2mpw03_charger_data *charger)
{
	u8 chg_sts1, chg_sts3, pmic_sts;
	int ret;

	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &chg_sts1);
	if (ret < 0) goto failed_to_read_reg;
	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS3, &chg_sts3);
	if (ret < 0) goto failed_to_read_reg;
	ret = s2mpw03_read_reg(charger->iodev->pmic, S2MPW03_PMIC_REG_STATUS1, &pmic_sts);
	if (ret < 0) goto failed_to_read_reg;

	dev_info(charger->dev, "%s: chg_sts1 = 0x%x, chg_sts3 = 0x%x, pmic_sts = 0x%x\n",
		__func__, chg_sts1, chg_sts3, pmic_sts);

	if (s2mpw03_check_topoff_status(chg_sts1, chg_sts3, charger->topoff_current))
		return POWER_SUPPLY_STATUS_FULL;

	if (!(pmic_sts & ACOKB_STATUS_MASK) && (chg_sts1 & CIN2BAT_STATUS_MASK) &&
		(chg_sts1 & CHG_ACOK_STATUS_MASK) && !(chg_sts1 & CHG_DONE_STATUS_MASK))
		return POWER_SUPPLY_STATUS_CHARGING;
	else if (!(pmic_sts & ACOKB_STATUS_MASK) && (chg_sts1 & CIN2BAT_STATUS_MASK) &&
		!(chg_sts1 & CHG_ACOK_STATUS_MASK) && !(chg_sts1 & CHG_DONE_STATUS_MASK))
		return POWER_SUPPLY_STATUS_DISCHARGING;
	else if ((chg_sts1 & CHG_ACOK_STATUS_MASK) && (chg_sts1 & CHG_DONE_STATUS_MASK))
		return POWER_SUPPLY_STATUS_NOT_CHARGING;

	return POWER_SUPPLY_STATUS_UNKNOWN;

failed_to_read_reg:
	dev_info(charger->dev, "%s: failed to read reg(0x%x, 0x%x, 0x%x), ret(%d)\n",
		__func__, chg_sts1, chg_sts3, pmic_sts, ret);
	return POWER_SUPPLY_STATUS_UNKNOWN;
}

static int s2mpw03_get_charge_type(struct i2c_client *iic)
{
	int status = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	int ret;
	u8 data;

	ret = s2mpw03_read_reg(iic, S2MPW03_CHG_REG_STATUS1, &data);
	if (ret < 0) {
		dev_err(&iic->dev, "%s fail\n", __func__);
		return ret;
	}

	switch (data & CHG_ACOK_STATUS_MASK) {
	case CHG_ACOK_STATUS_MASK:
		status = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	default:
		status = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;
	}

	return status;
}

static bool s2mpw03_get_batt_present(struct i2c_client *iic)
{
	int ret;
	u8 data;

	ret = s2mpw03_read_reg(iic, S2MPW03_CHG_REG_STATUS2, &data);
	if (ret < 0)
		return false;

	return (data & DET_BAT_STATUS_MASK) ? true : false;
}

static int s2mpw03_get_charging_health(struct s2mpw03_charger_data *charger)
{
	int ret;
	u8 chg_sts1;

	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &chg_sts1);
	pr_info("%s: chg_status1 = 0x%x, unhealth_cnt = %d\n" ,
		__func__, chg_sts1, charger->unhealth_cnt);
	if (ret < 0)
		return POWER_SUPPLY_HEALTH_UNKNOWN;

	if (is_wireless_type(charger->cable_type)) {
		union power_supply_propval value;

		psy_do_property(charger->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_HEALTH, value);
		pr_info("%s: check wpc health = %d\n", __func__, value.intval);

		if (value.intval == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)
			return POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
	}

	if (chg_sts1 & CHG_ACOK_STATUS_MASK) {
		charger->ovp = false;
		charger->unhealth_cnt = 0;
		return POWER_SUPPLY_HEALTH_GOOD;
	}

	if((chg_sts1 & CHGVINOVP_STATUS_MASK) && (chg_sts1 & CIN2BAT_STATUS_MASK)) {
		if (charger->unhealth_cnt < HEALTH_DEBOUNCE_CNT)
			return POWER_SUPPLY_HEALTH_GOOD;
		charger->unhealth_cnt++;
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}

	return POWER_SUPPLY_HEALTH_GOOD;
}

static int s2mpw03_monitor_work_debug_info(struct s2mpw03_charger_data *charger)
{
	struct s2mpw03_charger_data *c = charger;

	pr_info("%s charging(%d) chg_current(%d) chg_sts(%d) eoc(%d) health(%d) "
		"float(%d) top_off(%d) cv_status(%d) in2bat(%d)\n",
		__func__,
		c->is_charging,
		c->charging_current,
		c->log_data.chg_status,
		c->topoff_current,
		c->log_data.health,
		charger->pdata->chg_float_voltage,
		!!(c->log_data.reg_data[S2MPW03_CHG_REG_STATUS1] & TOP_OFF_STATUS_MASK),
		!!(c->log_data.reg_data[S2MPW03_CHG_REG_STATUS3] & CV_OK_STATUS_MASK),
		!!(c->log_data.reg_data[S2MPW03_CHG_REG_STATUS1] & CIN2BAT_STATUS_MASK)
		);
#if EN_TEST_READ
	s2mpw03_test_read(charger);
#endif
	return 0;
}

static int s2mpw03_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct s2mpw03_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
	unsigned char status;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->charging_current ? 1 : 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s2mpw03_get_charging_status(charger);
		charger->log_data.chg_status = val->intval;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s2mpw03_get_charging_health(charger);
		charger->log_data.health = val->intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = 2000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			val->intval = s2mpw03_get_fast_charging_current(charger);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = s2mpw03_get_fast_charging_current(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		val->intval = s2mpw03_get_current_eoc_setting(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = s2mpw03_get_charge_type(charger->client);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = charger->pdata->chg_float_voltage;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s2mpw03_get_batt_present(charger->client);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = charger->is_charging;
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_POWER_NOW:
		s2mpw03_read_reg(charger->iodev->pmic, S2MPW03_PMIC_REG_STATUS1, &status);
		pr_info("%s: pm status : 0x%x\n", __func__, status);
		if (status & S2MPW03_STATUS1_ACOKB_M)
			val->intval = ACOK_NO_INPUT;
		else
			val->intval = ACOK_INPUT;
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
			case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
				val->intval = 0;
				s2mpw03_monitor_work_debug_info(charger);
				break;
			case POWER_SUPPLY_EXT_PROP_CHG_IN_OK:
				s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS2, &status);
				val->intval = (status & A2D_CHGINOK_STATUS_MASK) ? 1 : 0;
				break;
			default:
				break;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int s2mpw03_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct s2mpw03_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	struct device *dev = charger->dev;
/*	int previous_cable_type = charger->cable_type; */

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;
		/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		if (is_nocharge_type(charger->cable_type)) {
			dev_info(dev, "%s() [BATT] Type Battery\n", __func__);
		} else {
			dev_info(dev, "%s()  Set charging, Cable type = %d\n",
				 __func__, charger->cable_type);
#if ENABLE_MIVR
			s2mpw03_set_mivr_level(charger);
#endif /*ENABLE_MIVR*/
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		dev_info(dev, "%s() set current[%d]\n", __func__, val->intval);
		charger->charging_current = val->intval;
		s2mpw03_set_fast_charging_current(charger, charger->charging_current);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		dev_info(dev, "%s() float voltage(%d)\n", __func__, val->intval);
		charger->pdata->chg_float_voltage = val->intval;
		s2mpw03_set_regulation_voltage(charger,
				charger->pdata->chg_float_voltage);
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		charger->topoff_current = val->intval;
		s2mpw03_set_topoff_current(charger->client, charger->topoff_current);
		dev_info(dev, "%s() chg eoc current = %d mA, is_charging %d\n",
			__func__, val->intval, charger->is_charging);
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		dev_err(dev, "%s() OTG mode not supported\n", __func__);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->charge_mode = val->intval;
		pr_info("[DEBUG]%s: %s\n", __func__,
			CHARGE_MODE_STRING[charger->charge_mode]);
		switch (charger->charge_mode) {
		case SEC_BAT_CHG_MODE_BUCK_OFF:
		case SEC_BAT_CHG_MODE_CHARGING_OFF:
			charger->is_charging = false;
			break;
		case SEC_BAT_CHG_MODE_CHARGING:
			charger->is_charging = true;
			break;
		}
		s2mpw03_enable_charger_switch(charger, charger->is_charging);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		break;
	case POWER_SUPPLY_PROP_AUTHENTIC:
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_FORCED_JIG_MODE:
			break;
		case POWER_SUPPLY_EXT_PROP_CHECK_DROP_CV:
			break;
		case POWER_SUPPLY_EXT_PROP_FACTORY_MODE:
			charger->factory_mode = val->intval;
			pr_err("%s, factory mode\n", __func__);
			break;
		default:
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#if 0
static irqreturn_t s2mpw03_chg_isr(int irq, void *data)
{
	struct s2mpw03_charger_data *charger = data;
	u8 val, valm;

	s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &val);
	pr_info("[DEBUG]%s, %02x\n" , __func__, val);
	s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_INT1M, &valm);
	pr_info("%s : CHG_INT1 ---> 0x%x\n", __func__, valm);

	if ((val & TOP_OFF_STATUS_MASK) && (val & CHG_ACOK_STATUS_MASK)) {
		pr_info("%s : top_off status~!\n", __func__);
		charger->full_charged = true;
	}

	return IRQ_HANDLED;
}
#endif

static irqreturn_t s2mpw03_tmrout_isr(int irq, void *data)
{
	struct s2mpw03_charger_data *charger = data;
	u8 val;

	s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS2, &val);
	if (val & TMROUT_STATUS_MASK) {
		/* Timer out status */
		pr_err("%s, fast-charging timeout, timer clear\n", __func__);
		s2mpw03_enable_charger_switch(charger, false);
		msleep(100);
		s2mpw03_enable_charger_switch(charger, true);
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static int s2mpw03_charger_parse_dt(struct device *dev,
		struct s2mpw03_charger_platform_data *pdata)
{
	struct device_node *np = of_find_node_by_name(NULL, "s2mpw03-charger");
	int ret;

	/* SC_CTRL8 , SET_VF_VBAT , Battery regulation voltage setting */
	ret = of_property_read_u32(np, "battery,chg_float_voltage",
				&pdata->chg_float_voltage);

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_string(np,
			"battery,charger_name", (char const **)&pdata->charger_name);

		ret = of_property_read_string(np,
			"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);

		ret = of_property_read_u32(np, "battery,full_check_type",
				&pdata->full_check_type);

		ret = of_property_read_u32(np, "battery,full_check_type_2nd",
				&pdata->full_check_type_2nd);
		if (ret)
			pr_info("%s : Full check type 2nd is Empty\n",
						__func__);
	}
	dev_info(dev, "s2mpw03 charger parse dt retval = %d\n", ret);
	return ret;
}
#else
static int s2mpw03_charger_parse_dt(struct device *dev,
		struct s2mpw03_charger_platform_data *pdata)
{
	return -ENOSYS;
}
#endif
/* if need to set s2mpw03 pdata */
static struct of_device_id s2mpw03_charger_match_table[] = {
	{ .compatible = "samsung,s2mpw03-charger",},
	{},
};

static int s2mpw03_charger_probe(struct platform_device *pdev)
{
	struct s2mpw03_dev *s2mpw03 = dev_get_drvdata(pdev->dev.parent);
	struct s2mpw03_platform_data *pdata = dev_get_platdata(s2mpw03->dev);
	struct s2mpw03_charger_data *charger;
	struct power_supply_config psy_cfg = {};
	int ret = 0;

	pr_info("%s:[BATT] S2MPW03 Charger driver probe\n", __func__);
	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	mutex_init(&charger->io_lock);

	charger->iodev = s2mpw03;
	charger->dev = &pdev->dev;
	charger->client = s2mpw03->charger;

	charger->pdata = devm_kzalloc(&pdev->dev, sizeof(*(charger->pdata)),
			GFP_KERNEL);
	if (!charger->pdata) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_parse_dt_nomem;
	}
	ret = s2mpw03_charger_parse_dt(&pdev->dev, charger->pdata);
	if (ret < 0)
		goto err_parse_dt;

	platform_set_drvdata(pdev, charger);

	if (charger->pdata->charger_name == NULL)
		charger->pdata->charger_name = "s2mpw03-charger";

	charger->psy_chg_desc.name           = charger->pdata->charger_name;
	charger->psy_chg_desc.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg_desc.get_property   = s2mpw03_chg_get_property;
	charger->psy_chg_desc.set_property   = s2mpw03_chg_set_property;
	charger->psy_chg_desc.properties     = s2mpw03_charger_props;
	charger->psy_chg_desc.num_properties = ARRAY_SIZE(s2mpw03_charger_props);

	charger->dev_id = s2mpw03->pmic_rev;
	charger->onoff = 0;

	s2mpw03_chg_init(charger);
	s2mpw03_make_current_table();

	charger->in2bat_vote = sec_vote_init(VN_IN2BAT, SEC_VOTE_MIN, VOTER_MAX,
		true, sec_voter_name, s2mpw03_set_in2bat_vote, charger);
	if (!charger->in2bat_vote) {
		dev_err(s2mpw03->dev, "%s: failed to init in2bat_vote\n", __func__);
		ret = -ENOMEM;
		goto err_in2bat_vote;
	}
	change_sec_voter_pri(charger->in2bat_vote, VOTER_S2MPW03_CONTROL, VOTE_PRI_10);
	sec_vote(charger->in2bat_vote, VOTER_CABLE, true, 1);

	psy_cfg.drv_data = charger;
	psy_cfg.supplied_to = s2mpw03_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(s2mpw03_supplied_to);
	charger->psy_chg = power_supply_register(&pdev->dev, &charger->psy_chg_desc, &psy_cfg);
	if (IS_ERR(charger->psy_chg)) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		ret = PTR_ERR(charger->psy_chg);
		goto err_power_supply_register;
	}

	charger->irq_chg = pdata->irq_base + S2MPW03_CHG_IRQ_TOPOFF_INT1;
#if 0
	ret = request_threaded_irq(charger->irq_chg, NULL,
			s2mpw03_chg_isr, 0, "chg-irq", charger);
	if (ret < 0) {
		dev_err(s2mpw03->dev, "%s: Fail to request charger irq in IRQ: %d: %d\n",
					__func__, charger->irq_chg, ret);
		goto err_power_supply_register;
	}
#endif
	charger->irq_tmrout = charger->iodev->irq_base + S2MPW03_CHG_IRQ_TMROUT_INT2;
	ret = request_threaded_irq(charger->irq_tmrout, NULL,
			s2mpw03_tmrout_isr, 0, "tmrout-irq", charger);
	if (ret < 0) {
		dev_err(s2mpw03->dev, "%s: Fail to request charger irq in IRQ: %d: %d\n",
					__func__, charger->irq_tmrout, ret);
		goto err_power_supply_register;
	}
#if EN_TEST_READ
	s2mpw03_test_read(charger);
#endif

	charger->muic = s2mpw03_init_muic(pdev);
	if (IS_ERR_OR_NULL(charger->muic))
		goto err_power_supply_register;

	pr_info("%s:[BATT] S2MPW03 charger driver loaded OK\n", __func__);

	return 0;

err_power_supply_register:
	destroy_workqueue(charger->charger_wqueue);
	power_supply_unregister(charger->psy_chg);
//	power_supply_unregister(&charger->psy_battery);
	sec_vote_destroy(charger->in2bat_vote);
err_in2bat_vote:
err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&charger->io_lock);
	kfree(charger);
	return ret;
}

static int s2mpw03_charger_remove(struct platform_device *pdev)
{
	struct s2mpw03_charger_data *charger =
		platform_get_drvdata(pdev);

	power_supply_unregister(charger->psy_chg);
	s2mpw03_muic_unregister(charger->muic);
	mutex_destroy(&charger->io_lock);
	kfree(charger);
	return 0;
}

#if defined CONFIG_PM
static int s2mpw03_charger_suspend(struct device *dev)
{
	return 0;
}

static int s2mpw03_charger_resume(struct device *dev)
{

	return 0;
}
#else
#define s2mpw03_charger_suspend NULL
#define s2mpw03_charger_resume NULL
#endif

static void s2mpw03_charger_shutdown(struct device *dev)
{
	struct s2mpw03_charger_data *charger = dev_get_drvdata(dev);

	s2mpw03_enable_charger_switch(charger, true);
	sec_vote(charger->in2bat_vote, VOTER_S2MPW03_CONTROL, true, 1);

	pr_info("%s: S2MPW03 Charger driver shutdown\n", __func__);
}

static SIMPLE_DEV_PM_OPS(s2mpw03_charger_pm_ops, s2mpw03_charger_suspend,
		s2mpw03_charger_resume);

static struct platform_driver s2mpw03_charger_driver = {
	.driver         = {
		.name   = "s2mpw03-charger",
		.owner  = THIS_MODULE,
		.of_match_table = s2mpw03_charger_match_table,
		.pm     = &s2mpw03_charger_pm_ops,
		.shutdown = s2mpw03_charger_shutdown,
	},
	.probe          = s2mpw03_charger_probe,
	.remove		= s2mpw03_charger_remove,
};

static int __init s2mpw03_charger_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&s2mpw03_charger_driver);

	return ret;
}
device_initcall(s2mpw03_charger_init);

static void __exit s2mpw03_charger_exit(void)
{
	platform_driver_unregister(&s2mpw03_charger_driver);
}
module_exit(s2mpw03_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Charger driver for S2MPW03");
