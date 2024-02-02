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
#include <linux/power/s2mpw03_charger.h>
#include <linux/version.h>
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#if 0
#include <linux/wakelock.h>
#endif

#define ENABLE_MIVR 1
#define MINVAL(a, b) ((a <= b) ? a : b)
#define HEALTH_DEBOUNCE_CNT 3

#ifndef EN_TEST_READ
#define EN_TEST_READ 1
#endif

struct s2mpw03_charger_data {
	struct s2mpw03_dev	*iodev;
	struct i2c_client       *client;
	struct device *dev;
	struct s2mpw03_platform_data *s2mpw03_pdata;
	struct delayed_work	charger_work;
	struct delayed_work init_work;
	struct delayed_work usb_work;
	struct delayed_work rid_work;
	struct delayed_work acok_work;

	struct workqueue_struct *charger_wqueue;
	struct power_supply	*psy_chg;
	struct power_supply_desc	psy_chg_desc;
	s2mpw03_charger_platform_data_t *pdata;
	int dev_id;
	int charging_current;
	int topoff_current;
	int charge_mode;
	int cable_type;
	bool is_charging;
	bool is_usb_ready;
	struct mutex io_lock;

	/* register programming */
	int reg_addr;
	int reg_data;

	bool full_charged;
	bool ovp;
	bool factory_mode;

	int unhealth_cnt;
	int status;
	int onoff;

	/* s2mpw03 */
	int irq_det_bat;
	int irq_chg;
	int irq_tmrout;

	int irq_uart_off;
	int irq_uart_on;
	int irq_usb_off;
	int irq_usb_on;
	int irq_uart_cable;
	int irq_fact_leakage;
	int irq_jigon;
	int irq_acokf;
	int irq_acokr;
	int irq_rid_attach;
#if defined(CONFIG_MUIC_NOTIFIER)
	muic_attached_dev_t	muic_dev;
#endif
};

static enum power_supply_property s2mpw03_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
};

static int s2mpw03_get_charging_health(struct s2mpw03_charger_data *charger);
static void s2mpw03_topoff_status_setting(struct s2mpw03_charger_data *charger);
static void s2mpw03_muic_detect_handler(struct s2mpw03_charger_data *charger, bool is_on);

static char *s2mpw03_supplied_to[] = {
	"s2mpw03-charger",
};

static void s2mpw03_test_read(struct i2c_client *i2c)
{
	u8 data;
	char str[1016] = {0,};
	int i;

	for (i = S2MPW03_CHG_REG_INT1M; i <= S2MPW03_CHG_REG_CTRL12; i++) {
		s2mpw03_read_reg(i2c, i, &data);

		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
	}

	pr_err("[CHG]%s: %s\n", __func__, str);
}

static void s2mpw03_enable_charger_switch(struct s2mpw03_charger_data *charger,
		int onoff)
{

	if (onoff > 0) {
		pr_err("[DEBUG]%s: turn on charger\n", __func__);
		s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL1, EN_CHG_MASK, EN_CHG_MASK);
		s2mpw03_write_reg(charger->client, S2MPW03_CHG_REG_CHG_OTP12, 0x8A);
	} else {
		charger->full_charged = false;
		pr_err("[DEBUG] %s: turn off charger\n", __func__);
		s2mpw03_write_reg(charger->client, S2MPW03_CHG_REG_CHG_OTP12, 0x8B);
		s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL1, 0, EN_CHG_MASK);
	}
}

static void s2mpw03_set_regulation_voltage(struct s2mpw03_charger_data *charger,
		int float_voltage)
{
	unsigned int data;

	pr_err("[DEBUG]%s: float_voltage %d\n", __func__, float_voltage);

	if (float_voltage <= 4050)
		data = 0x0;
	else if (float_voltage > 4050 && float_voltage <= 4450)
		data = (int)((float_voltage - 4050) * 10 / 125);
	else
		data = 0x1F;

	s2mpw03_update_reg(charger->client,
			S2MPW03_CHG_REG_CTRL2, data << CV_SEL_SHIFT, CV_SEL_MASK);
}

static void s2mpw03_set_fast_charging_current(struct i2c_client *i2c,
		int charging_current)
{
	int data;

	pr_err("[DEBUG]%s: fast charge current  %d\n", __func__, charging_current);

	if (charging_current < 75)
		data = 0x0;
	else if (charging_current < 150)
		data = 0x1;
	else if (charging_current < 175)
		data = 0x2;
	else if (charging_current < 200)
		data = 0x3;
	else if (charging_current < 250)
		data = 0x4;
	else if (charging_current < 300)
		data = 0x5;
	else if (charging_current < 350)
		data = 0x6;
	else
		data = 0x7;

	s2mpw03_update_reg(i2c, S2MPW03_CHG_REG_CTRL8, data << CC_SEL_SHIFT,
			CC_SEL_MASK);
#if EN_TEST_READ
	s2mpw03_test_read(i2c);
#endif
}

int fast_charging_current[] = {30, 75, 150, 175, 200, 250, 300, 350};

static int s2mpw03_get_fast_charging_current(struct i2c_client *i2c)
{
	int ret;
	u8 data;

	ret = s2mpw03_read_reg(i2c, S2MPW03_CHG_REG_CTRL8, &data);
	if (ret < 0)
		return ret;

	data = (data & CC_SEL_MASK) >> CC_SEL_SHIFT;

	return fast_charging_current[data];
}

int eoc_current[16] = {
	10, 12, 15, 17, 20, 22, 25, 27, 30, 32, 35, 40, 45, 50, 55, 60};

static int s2mpw03_get_current_eoc_setting(struct s2mpw03_charger_data *charger)
{
	int ret;
	u8 data;

	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_CTRL5, &data);
	if (ret < 0)
		return ret;

	data = (data & EOC_I_SEL_MASK) >> EOC_I_SEL_SHIFT;

	if (data > 0xf)
		data = 0xf;

	pr_err("[DEBUG]%s: data(0x%x), top-off current	%d\n", __func__, data, eoc_current[data]);

	return eoc_current[data];
}

static void s2mpw03_set_topoff_current(struct i2c_client *i2c, int current_limit)
{
	int data;
	int i;
	int len = sizeof(eoc_current)/sizeof(int);

	for (i = 0; i < len; i++) {
		if (current_limit <= eoc_current[i]) {
			data = i;
			break;
		}
	}

	if (i >= len)
		data = len - 1;

	pr_err("[DEBUG]%s: top-off current	%d, data=0x%x\n", __func__, current_limit, data);

	s2mpw03_update_reg(i2c, S2MPW03_CHG_REG_CTRL5, data << EOC_I_SEL_SHIFT,
			EOC_I_SEL_MASK);
}

static void s2mpw03_set_additional(struct s2mpw03_charger_data *charger, int n, int onoff)
{
	pr_err("[DEBUG]%s: n (%d), onoff (%d)\n", __func__, n, onoff);

	if (onoff == 1) {
		/* Apply additional charging current */
		s2mpw03_update_reg(charger->client,
		S2MPW03_CHG_REG_CTRL4, n << T_ADD_SHIFT, T_ADD_MASK);

		/* Additional charging path On */
		s2mpw03_update_reg(charger->client,
		S2MPW03_CHG_REG_CTRL3, FORCED_ADD_ON_MASK, FORCED_ADD_ON_MASK);

	} else if (onoff == 0) {
		/* Additional charging path Off */
		s2mpw03_update_reg(charger->client,
		S2MPW03_CHG_REG_CTRL3, 0 << FORCED_ADD_ON_SHIFT, FORCED_ADD_ON_MASK);

		/* Restore addition charging current */
		s2mpw03_update_reg(charger->client,
		S2MPW03_CHG_REG_CTRL4, n << T_ADD_SHIFT, T_ADD_MASK);
	}
}

enum {
	S2MPW03_MIVR_4200MV = 0,
	S2MPW03_MIVR_4400MV,
	S2MPW03_MIVR_4600MV,
	S2MPW03_MIVR_4800MV,
};

#if ENABLE_MIVR
/* charger input regulation voltage setting */
static void s2mpw03_set_mivr_level(struct s2mpw03_charger_data *charger)
{
	int mivr = S2MPW03_MIVR_4600MV;

	s2mpw03_update_reg(charger->client,
			S2MPW03_CHG_REG_CTRL5, mivr << IVR_V_SEL_SHIFT, IVR_V_SEL_MASK);
}
#endif /*ENABLE_MIVR*/

/* here is set init charger data */
static bool s2mpw03_chg_init(struct s2mpw03_charger_data *charger)
{
	dev_info(&charger->client->dev, "%s : DEV ID : 0x%x\n", __func__,
			charger->dev_id);

	/* Factory_mode initialization */
	charger->factory_mode = false;

	/* change Top-off detection debounce time to max 0x25[3:2]=11 */
	s2mpw03_write_reg(charger->client, S2MPW03_CHG_REG_CHG_OTP4, 0x3D);

	/* Top-off Timer Disable 0x16[6]=1 */
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL11,
		NO_TIMEOUT_30M_TM_MASK, NO_TIMEOUT_30M_TM_MASK);

	/* Watchdog timer disable */
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL8, 0x00, WDT_EN_MASK);

	/* Manual reset enable */
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL7, MRST_EN_MASK, MRST_EN_MASK);

	/* RID detect debounce time : 200msec -> 100msec */
	s2mpw03_update_reg(charger->client, S2MPW03_CHG_REG_CTRL9, 0x00, TDB_RID_MASK);

	return true;
}

static int s2mpw03_get_charging_status(struct s2mpw03_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	int ret;
	u8 chg_sts, pmic_sts;

	s2mpw03_topoff_status_setting(charger);

	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &chg_sts);
	if (ret < 0)
		return status;
	ret = s2mpw03_read_reg(charger->iodev->pmic, S2MPW03_PMIC_REG_STATUS1, &pmic_sts);
	dev_info(charger->dev, "%s : charger status : 0x%x, pmic status : 0x%x\n",
			__func__, chg_sts, pmic_sts);

	if (charger->full_charged) {
			dev_info(charger->dev, "%s : POWER_SUPPLY_STATUS_FULL : 0x%x\n", __func__, chg_sts);
			return POWER_SUPPLY_STATUS_FULL;
	}

	if (!(pmic_sts & ACOKB_STATUS_MASK) && (chg_sts & CIN2BAT_STATUS_MASK) &&
		(chg_sts & CHG_ACOK_STATUS_MASK) && !(chg_sts & CHG_DONE_STATUS_MASK))
		status = POWER_SUPPLY_STATUS_CHARGING;
	else if (!(pmic_sts & ACOKB_STATUS_MASK) && (chg_sts & CIN2BAT_STATUS_MASK) &&
		!(chg_sts & CHG_ACOK_STATUS_MASK) && !(chg_sts & CHG_DONE_STATUS_MASK))
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if ((chg_sts & CHG_ACOK_STATUS_MASK) && (chg_sts & CHG_DONE_STATUS_MASK))
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;

#if EN_TEST_READ
	s2mpw03_test_read(charger->client);
#endif
	return status;
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
		status = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
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

	pr_info("[%s] \n " , __func__);
	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &chg_sts1);

	pr_info("[%s] chg_status1: 0x%x\n " , __func__, chg_sts1);
	if (ret < 0)
		return POWER_SUPPLY_HEALTH_UNKNOWN;

	if (chg_sts1 & CHG_ACOK_STATUS_MASK) {
		charger->ovp = false;
		charger->unhealth_cnt = 0;
		pr_info("[%s] POWER_SUPPLY_HEALTH_GOOD\n", __func__);
		return POWER_SUPPLY_HEALTH_GOOD;
	}

	if((chg_sts1 & CHGVINOVP_STATUS_MASK) && (chg_sts1 & CIN2BAT_STATUS_MASK)) {
		pr_info("[%s] POWER_SUPPLY_HEALTH_OVERVOLTAGE, unhealth_cnt %d\n " ,
			__func__, charger->unhealth_cnt);
		if (charger->unhealth_cnt < HEALTH_DEBOUNCE_CNT)
			return POWER_SUPPLY_HEALTH_GOOD;
		charger->unhealth_cnt++;
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}
	return POWER_SUPPLY_HEALTH_GOOD;
}

static int s2mpw03_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{

	struct s2mpw03_charger_data *charger =
		power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->charging_current ? 1 : 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s2mpw03_get_charging_status(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s2mpw03_get_charging_health(charger);
#if EN_TEST_READ
		s2mpw03_test_read(charger->client);
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = 2000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			val->intval = s2mpw03_get_fast_charging_current(charger->client);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = s2mpw03_get_fast_charging_current(charger->client);
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
	default:
		return -EINVAL;
	}

	return 0;
}

static int s2mpw03_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct s2mpw03_charger_data *charger =
		power_supply_get_drvdata(psy);

	struct device *dev = charger->dev;
/*	int previous_cable_type = charger->cable_type; */

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;
		/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY ||
				charger->cable_type == POWER_SUPPLY_TYPE_UNKNOWN) {
			dev_info(dev, "%s() [BATT] Type Battery\n", __func__);
			if (!charger->pdata->charging_current_table)
				return -EINVAL;
		} else if (charger->cable_type == POWER_SUPPLY_TYPE_OTG) {
			dev_info(dev, "%s() OTG mode not supported\n", __func__);
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
		s2mpw03_set_fast_charging_current(charger->client, charger->charging_current);
#if EN_TEST_READ
		s2mpw03_test_read(charger->client);
#endif
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
		pr_info("[DEBUG]%s: CHARGING_ENABLE(%d)\n", __func__, charger->charge_mode);
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
	default:
		return -EINVAL;
	}

	return 0;
}

static void s2mpw03_factory_mode_setting(struct s2mpw03_charger_data *charger)
{
	charger->factory_mode = true;
	pr_err("%s, factory mode\n", __func__);
}

static void s2mpw03_topoff_status_setting(struct s2mpw03_charger_data *charger)
{
	u8 chg_sts1, chg_sts3;

	s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &chg_sts1);
	s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS3, &chg_sts3);

	if ((chg_sts1 & TOP_OFF_STATUS_MASK) && (chg_sts3 & CV_OK_STATUS_MASK)) {
		pr_info("%s : top_off status!\n", __func__);
		charger->full_charged = true;
	} else {
		charger->full_charged = false;
	}
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

#if defined(CONFIG_S2MPW03_RID_DETECT)
static void s2mpw03_muic_init_detect(struct work_struct *work)
{
	struct s2mpw03_charger_data *charger =
		container_of(work, struct s2mpw03_charger_data, init_work.work);

	int ret;
	unsigned char pmic_sts1, chg_sts1, chg_sts4;

	/* check when booting after USB connected */
	ret = s2mpw03_read_reg(charger->iodev->pmic, S2MPW03_PMIC_REG_STATUS1, &pmic_sts1);
	if (ret < 0) {
		pr_err("{DEBUG] %s : pmic status1 read fail\n", __func__);
	}
	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &chg_sts1);
	if (ret < 0) {
		pr_err("{DEBUG] %s : chg status1 read fail\n", __func__);
	}
	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);
	if (ret < 0) {
		pr_err("{DEBUG] %s : chg status4 read fail\n", __func__);
	}

	pr_info("%s: pmic status1, chg status1, chg status4: 0x%x, 0x%x, 0x%x\n",
		__func__, pmic_sts1, chg_sts1, chg_sts4);

	if (!(pmic_sts1 & ACOKB_STATUS_MASK)) {
		charger->onoff = 1;
		if (chg_sts4 & UART_CABLE_MASK) {
#if defined(CONFIG_MUIC_NOTIFIER)
			charger->muic_dev = ATTACHED_DEV_USB_MUIC;
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			pr_info("%s: USB connected\n", __func__);
		} else if (chg_sts4 & UART_BOOT_OFF_MASK) {
#if defined(CONFIG_MUIC_NOTIFIER)
			charger->muic_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			pr_info("%s: JIG_ID UART OFF ( 523K ) connected\n", __func__);
		} else if (chg_sts4 & UART_BOOT_ON_MASK) {
#if defined(CONFIG_MUIC_NOTIFIER)
			charger->muic_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			pr_info("%s: JIG_ID UART ON ( 619K ) connected\n", __func__);
		} else if (chg_sts4 & USB_BOOT_OFF_MASK) {
#if defined(CONFIG_MUIC_NOTIFIER)
			charger->muic_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			pr_info("%s: JIG_ID USB OFF ( 255K ) connected\n", __func__);
		} else if (chg_sts4 & USB_BOOT_ON_MASK) {
#if defined(CONFIG_MUIC_NOTIFIER)
			charger->muic_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			pr_info("%s: JIG_ID USB OFF ( 301K ) connected\n", __func__);
		} else if (chg_sts1 & CHG_ACOK_STATUS_MASK) {
			pr_info("%s: Wired TA connected\n", __func__);
#if defined(CONFIG_MUIC_NOTIFIER)
#if defined(CONFIG_S2MPW03_TA_DETECT)
			charger->muic_dev = ATTACHED_DEV_USB_MUIC;
#else
			charger->muic_dev = ATTACHED_DEV_TA_MUIC;
#endif
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
		} else {
			pr_info("%s: Wireless TA connected\n", __func__);
			//charger->muic_dev = ATTACHED_DEV_WIRELESS_TA_MUIC;
		}
	}
}

static void s2mpw03_muic_usb_detect(struct work_struct *work)
{
	struct s2mpw03_charger_data *charger =
		container_of(work, struct s2mpw03_charger_data, usb_work.work);

	int ret;
	unsigned char pmic_sts1, chg_sts4;

	charger->is_usb_ready = true;
	/* check when booting after USB connected */
	ret = s2mpw03_read_reg(charger->iodev->pmic, S2MPW03_PMIC_REG_STATUS1, &pmic_sts1);
	if (ret < 0) {
		pr_err("{DEBUG] %s : pmic status1 read fail\n", __func__);
	}
	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);
	if (ret < 0) {
		pr_err("{DEBUG] %s : chg status4 read fail\n", __func__);
	}

	pr_info("%s: pmic status1 chg status4: 0x%x,0x%x\n", __func__, pmic_sts1, chg_sts4);
	if (!(pmic_sts1 & ACOKB_STATUS_MASK)) {
		charger->onoff = 1;
		if(chg_sts4 & UART_CABLE_MASK) {
			if (charger->is_usb_ready) {
#if defined(CONFIG_MUIC_NOTIFIER)
				charger->muic_dev = ATTACHED_DEV_USB_MUIC;
				muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			}
			pr_info("%s: USB connected\n", __func__);
		} else if (chg_sts4 & USB_BOOT_ON_MASK) {
			if (charger->is_usb_ready) {
#if defined(CONFIG_MUIC_NOTIFIER)
				charger->muic_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
				muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			}
			pr_info("%s: JIG_ID USB ON ( 301K ) connected\n", __func__);
		} else if (chg_sts4 & USB_BOOT_OFF_MASK) {
			if (charger->is_usb_ready) {
#if defined(CONFIG_MUIC_NOTIFIER)
				charger->muic_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
				muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			}
			pr_info("%s: JIG_ID USB OFF ( 255K ) connected\n", __func__);
		}
	}
}

static void s2mpw03_muic_rid_check(struct work_struct *work)
{
	struct s2mpw03_charger_data *charger =
		container_of(work, struct s2mpw03_charger_data, rid_work.work);
	unsigned char chg_sts4;

	s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);
	pr_info("%s: acokr irq stat4: 0x%x\n", __func__, chg_sts4);

	if (!((chg_sts4 & USB_BOOT_ON_MASK) || (chg_sts4 & USB_BOOT_OFF_MASK) ||
		(chg_sts4 & UART_BOOT_ON_MASK) || (chg_sts4 & UART_BOOT_OFF_MASK) ||
		(chg_sts4 & UART_CABLE_MASK) || (chg_sts4 & FACT_LEAKAGE_MASK))) {
		charger->factory_mode = false;
		pr_err("%s: factory mode[%d]\n", __func__, charger->factory_mode);
	}
}

static void s2mpw03_muic_attach_detect(struct work_struct *work)
{
	struct s2mpw03_charger_data *charger =
		container_of(work, struct s2mpw03_charger_data, acok_work.work);

	s2mpw03_muic_detect_handler(charger, true);
}

static void s2mpw03_muic_detect_handler(struct s2mpw03_charger_data *charger, bool is_on)
{
	unsigned char chg_sts1, chg_sts2, chg_sts4;

	if(is_on) {
		charger->onoff = 1;

		s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS1, &chg_sts1);
		s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS2, &chg_sts2);
		s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);
		pr_info("%s:sts1 : 0x%x, sts2 : 0x%x,  rid irq stat4: 0x%x\n", __func__,
			chg_sts1, chg_sts2, chg_sts4);

		if (chg_sts4 & UART_CABLE_MASK) {
#if defined(CONFIG_MUIC_NOTIFIER)
			if (charger->is_usb_ready) {
				charger->muic_dev = ATTACHED_DEV_USB_MUIC;
				muic_notifier_attach_attached_dev(charger->muic_dev);
			}
#endif
			pr_info("%s: USB connected. status4: 0x%x\n", __func__, chg_sts4);
		} else {
			if (!(chg_sts4 & JIGON_MASK) && !(chg_sts4 & USB_BOOT_ON_MASK) &&
				!(chg_sts4 & USB_BOOT_OFF_MASK) && !(chg_sts4 & UART_BOOT_ON_MASK)
				&& !(chg_sts4 & UART_BOOT_OFF_MASK)) {
#if defined(CONFIG_MUIC_NOTIFIER)
				if (chg_sts1 & CHG_ACOK_STATUS_MASK) {
					pr_info("%s: Wired TA connected\n", __func__);
#if defined(CONFIG_S2MPW03_TA_DETECT)
					charger->muic_dev = ATTACHED_DEV_USB_MUIC;
#else
					charger->muic_dev = ATTACHED_DEV_TA_MUIC;
#endif
				} else {
					pr_info("%s: Wireless TA connected\n", __func__);
					//charger->muic_dev = ATTACHED_DEV_WIRELESS_TA_MUIC;
				}

				muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			} else {
				if ((chg_sts4 & USB_BOOT_ON_MASK) || (chg_sts4 & USB_BOOT_OFF_MASK) ||
					(chg_sts4 & UART_BOOT_ON_MASK) || (chg_sts4 & UART_BOOT_OFF_MASK)) {
#if defined(CONFIG_MUIC_NOTIFIER)
					charger->muic_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
					muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
					pr_info(" USB_BOOT_ON_STATUS_MASK USB_BOOT_OFF_STATUS_MASK.\n");
					pr_info(" UART_BOOT_ON_STATUS_MASK UART_BOOT_OFF_STATUS_MASK.\n");
					pr_info("%s: JIG_ID UART OFF ( 523K ) connected \n", __func__);
				}
				if ((chg_sts4 & USB_BOOT_ON_MASK) || (chg_sts4 & USB_BOOT_OFF_MASK)) {
#if defined(CONFIG_MUIC_NOTIFIER)
					if (charger->is_usb_ready) {
						charger->muic_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
						muic_notifier_attach_attached_dev(charger->muic_dev);
					}
#endif
					pr_info(" USB_BOOT_ON_STATUS_MASK USB_BOOT_OFF_STATUS_MASK.\n");
					pr_info("%s: JIG_ID USB ON ( 301K ) connected \n", __func__);
				}
				pr_info("%s: JIG_ID connected.\n", __func__);
			}
		}
	} else {
		charger->onoff = 0;

		s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);
		pr_info("%s: rid irq : sts4[0x%x]\n", __func__, chg_sts4);

#if defined(CONFIG_MUIC_NOTIFIER)
		muic_notifier_detach_attached_dev(charger->muic_dev);
#endif
	}
}

static irqreturn_t s2mpw03_muic_isr(int irq, void *data)
{
	struct s2mpw03_charger_data *charger = data;

	pr_info("%s: irq:%d\n", __func__, irq);

	if (irq == charger->irq_acokf)
		schedule_delayed_work(&charger->acok_work, msecs_to_jiffies(150));
	else if (irq == charger->irq_acokr)
		s2mpw03_muic_detect_handler(charger, false);
	else if (irq == charger->irq_usb_on) {
		/* usb boot on */
		pr_info("%s: JIG_ID USB ON ( 301K ) connected \n", __func__);
		if (charger->is_usb_ready) {
#if defined(CONFIG_MUIC_NOTIFIER)
			charger->muic_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
			}
		s2mpw03_factory_mode_setting(charger);
	} else if (irq == charger->irq_uart_off) {
		pr_info("%s: JIG_ID UART OFF ( 523K ) connected \n", __func__);
#if defined(CONFIG_MUIC_NOTIFIER)
		charger->muic_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
		muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
		s2mpw03_factory_mode_setting(charger);
	} else if (irq == charger->irq_uart_on) {
		pr_info("%s: JIG_ID UART ON ( 619K ) connected \n", __func__);
#if defined(CONFIG_MUIC_NOTIFIER)
		charger->muic_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
		muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
		s2mpw03_factory_mode_setting(charger);
	} else if (irq == charger->irq_usb_off) {
		pr_info("%s: JIG_ID USB OFF ( 255K ) connected \n", __func__);
		if (charger->is_usb_ready) {
#if defined(CONFIG_MUIC_NOTIFIER)
			charger->muic_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
			muic_notifier_attach_attached_dev(charger->muic_dev);
#endif
		}
		s2mpw03_factory_mode_setting(charger);
	}
	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _dev_id, _name)				\
do {									\
	ret = request_threaded_irq(_irq, NULL, s2mpw03_muic_isr,	\
				0, _name, _dev_id);	\
	if (ret < 0) {							\
		pr_err("%s: Failed to request s2mpw03 muic IRQ #%d: %d\n",		\
				__func__, _irq, ret);	\
		_irq = 0;						\
	}								\
} while (0)

static int s2mpw03_muic_irq_init(struct s2mpw03_charger_data *charger)
{
	int ret = 0;

	if (charger->iodev && (charger->iodev->irq_base > 0)) {
		int irq_base = charger->iodev->irq_base;

		/* request MUIC IRQ */
#if 0
		charger->irq_fact_leakage = irq_base + S2MPW03_CHG_IRQ_FACT_LEAKAGE_INT3;
		REQUEST_IRQ(charger->irq_fact_leakage, charger, "muic-fact_leakage");
#endif
		charger->irq_uart_cable = irq_base + S2MPW03_CHG_IRQ_UART_CABLE_INT4;
		REQUEST_IRQ(charger->irq_uart_cable, charger, "muic-uart_cable");

		charger->irq_usb_off = irq_base + S2MPW03_CHG_IRQ_USB_BOOT_OFF_INT4;
		REQUEST_IRQ(charger->irq_usb_off, charger, "muic-usb_off");

		charger->irq_usb_on = irq_base + S2MPW03_CHG_IRQ_USB_BOOT_ON_INT4;
		REQUEST_IRQ(charger->irq_usb_on, charger, "muic-usb_on");

		charger->irq_uart_off = irq_base + S2MPW03_CHG_IRQ_UART_BOOT_OFF_INT4;
		REQUEST_IRQ(charger->irq_uart_off, charger, "muic-uart_off");

		charger->irq_uart_on = irq_base + S2MPW03_CHG_IRQ_UART_BOOT_ON_INT4;
		REQUEST_IRQ(charger->irq_uart_on, charger, "muic-uart_on");

		charger->irq_acokf = irq_base + S2MPW03_PMIC_IRQ_ACOKBF_INT1;
		REQUEST_IRQ(charger->irq_acokf, charger, "muic-acokf");

		charger->irq_acokr = irq_base + S2MPW03_PMIC_IRQ_ACOKBR_INT1;
		REQUEST_IRQ(charger->irq_acokr, charger, "muic-acokr");
	}

	pr_err("%s:usb_off(%d), usb_on(%d), uart_off(%d), uart_on(%d), jig_on(%d), muic-acokf(%d), muic-acokr(%d)\n",
		__func__, charger->irq_usb_off, charger->irq_usb_on, charger->irq_uart_off, charger->irq_uart_on,
		charger->irq_jigon, charger->irq_acokf, charger->irq_acokr);

	return ret;
}

#define FREE_IRQ(_irq, _dev_id, _name)					\
do {									\
	if (_irq) {							\
		free_irq(_irq, _dev_id);				\
		pr_info("%s: IRQ(%d):%s free done\n",	\
				__func__, _irq, _name);			\
	}								\
} while (0)

static void s2mpw03_muic_free_irqs(struct s2mpw03_charger_data *charger)
{
	pr_info("%s\n", __func__);

	/* free MUIC IRQ */
	FREE_IRQ(charger->irq_uart_off, charger, "muic-uart_off");
	FREE_IRQ(charger->irq_uart_on, charger, "muic-uart_on");
	FREE_IRQ(charger->irq_usb_off, charger, "muic-usb_off");
	FREE_IRQ(charger->irq_usb_on, charger, "muic-usb_on");
	FREE_IRQ(charger->irq_uart_cable, charger, "muic-uart_cable");
	FREE_IRQ(charger->irq_fact_leakage, charger, "muic-fact_leakage");
	FREE_IRQ(charger->irq_jigon, charger, "muic-jigon");
}
#endif

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
	u8 chg_sts4;

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

#if defined(CONFIG_MUIC_NOTIFIER)
	charger->muic_dev = ATTACHED_DEV_NONE_MUIC;
#endif
	charger->onoff = 0;

	s2mpw03_chg_init(charger);
	s2mpw03_set_additional(charger, 0, 0);

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
	s2mpw03_test_read(charger->client);
#endif

#if defined(CONFIG_S2MPW03_RID_DETECT)
	INIT_DELAYED_WORK(&charger->rid_work, s2mpw03_muic_rid_check);
	INIT_DELAYED_WORK(&charger->acok_work, s2mpw03_muic_attach_detect);

	ret = s2mpw03_muic_irq_init(charger);
	if (ret) {
		pr_err( "[muic] %s: failed to init muic irq(%d)\n", __func__, ret);
		goto fail_init_irq;
	}

	INIT_DELAYED_WORK(&charger->init_work, s2mpw03_muic_init_detect);
	schedule_delayed_work(&charger->init_work, msecs_to_jiffies(3000));

	charger->is_usb_ready = false;
	INIT_DELAYED_WORK(&charger->usb_work, s2mpw03_muic_usb_detect);
	schedule_delayed_work(&charger->usb_work, msecs_to_jiffies(13000));

#endif

	/* factory_mode setting */
	ret = s2mpw03_read_reg(charger->client, S2MPW03_CHG_REG_STATUS4, &chg_sts4);
	if (ret < 0)
		pr_err("{DEBUG] %s : chg status4 read fail\n", __func__);

	if ((chg_sts4 & USB_BOOT_ON_MASK) || (chg_sts4 & USB_BOOT_OFF_MASK) ||
		(chg_sts4 & UART_BOOT_ON_MASK) || (chg_sts4 & UART_BOOT_OFF_MASK) ||
		(chg_sts4 & UART_CABLE_MASK) || (chg_sts4 & FACT_LEAKAGE_MASK))
		s2mpw03_factory_mode_setting(charger);

	pr_info("%s:[BATT] S2MPW03 charger driver loaded OK\n", __func__);

	return 0;

#if defined(CONFIG_S2MPW03_RID_DETECT)
fail_init_irq:
	s2mpw03_muic_free_irqs(charger);
#endif
err_power_supply_register:
	destroy_workqueue(charger->charger_wqueue);
	power_supply_unregister(charger->psy_chg);
//	power_supply_unregister(&charger->psy_battery);
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
