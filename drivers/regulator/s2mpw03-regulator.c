/*
 * s2mpw03-regulator.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/samsung/s2mpw03-regulator.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/reset/exynos-reset.h>
#ifdef CONFIG_SEC_PM_DEBUG
#include <linux/sec_pm_debug.h>
#endif /* CONFIG_SEC_PM_DEBUG */

static struct s2mpw03_info *static_info;
static struct regulator_desc regulators[S2MPW03_REGULATOR_MAX];

struct s2mpw03_info {
	struct regulator_dev *rdev[S2MPW03_REGULATOR_MAX];
	u8 opmode[S2MPW03_REGULATOR_MAX];
	int num_regulators;
	struct i2c_client *i2c;
	struct s2mpw03_dev *iodev;
	unsigned int vsel_value[S2MPW03_REGULATOR_MAX];
	bool cache_data;
};

static unsigned int s2mpw03_of_map_mode(unsigned int val)
{
	switch (val) {
	case SEC_OPMODE_SUSPEND:    /* ON in Standby Mode */
		return 0x1;
	case SEC_OPMODE_VGPIO:        /* ON in VGPIO Mode */
		return 0x2;
	case SEC_OPMODE_ON:        /* ON in Normal Mode */
		return 0x3;
	default:
		return REGULATOR_MODE_INVALID;
	}
}

/* Some LDOs supports [LPM/Normal]ON mode during suspend state */
static int s2m_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);
	unsigned int val;
	int id = rdev_get_id(rdev);

	val = mode << S2MPW03_ENABLE_SHIFT;

	s2mpw03->opmode[id] = val;

	return 0;
}

 /* set initial mode of LDO15 */
/*
int s2m_ldo15_set_mode(unsigned int val)
{
	int ret;

	ret = s2mpw03_update_reg(static_info->i2c, S2MPW03_PMIC_REG_L15CTRL, val, 0xC0);
	if (ret)
		return ret;

	pr_debug("%s : LDO15 mode [%d]\n", __func__, val);

	return 0;
}
EXPORT_SYMBOL_GPL(s2m_ldo15_set_mode);
*/

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, val, mask;

	reg = rdev->desc->enable_reg;
	val = s2mpw03->opmode[id];
	mask = rdev->desc->enable_mask;

	return s2mpw03_update_reg(s2mpw03->i2c, reg, val, mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);
	u8 reg, val = 0, mask;

	reg = rdev->desc->enable_reg;
	val = rdev->desc->enable_is_inverted ?
		rdev->desc->enable_mask : 0;
	mask = rdev->desc->enable_mask;

	return s2mpw03_update_reg(s2mpw03->i2c, reg, val, mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);
	int ret;
	u8 reg, val, mask;

	reg = rdev->desc->enable_reg;
	mask = rdev->desc->enable_mask;

	ret = s2mpw03_read_reg(s2mpw03->i2c, reg, &val);
	if (ret)
		return ret;

	return rdev->desc->enable_is_inverted ?
		!(val & mask) : !!(val & mask);
}

static int get_ramp_delay(int ramp_delay)
{
	unsigned char cnt = 0;

	ramp_delay /= 6;

	while (true) {
		ramp_delay = ramp_delay >> 1;
		if (ramp_delay == 0)
			break;
		cnt++;
	}
	return cnt;
}

static int s2m_set_ramp_delay(struct regulator_dev *rdev, int ramp_delay)
{
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);
	int ramp_shift, reg_id = rdev_get_id(rdev);
	u8 ramp_mask = 0x03;
	u8 ramp_value = 0;

	ramp_value = get_ramp_delay(ramp_delay/1000);
	if (ramp_value > 4) {
		pr_warn("%s: ramp_delay: %d not supported\n",
			rdev->desc->name, ramp_delay);
	}

	ramp_value = 0x00;	// 6.25mv/us fixed

	switch (reg_id) {
	case S2MPW03_BUCK1:
		ramp_shift = 0;
		break;
	case S2MPW03_BUCK4:
		ramp_shift = 2;
		break;
	default:
		return 0;
	}

	return s2mpw03_update_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_BUCK_RAMP,
			  ramp_value << ramp_shift, ramp_mask << ramp_shift);
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);
	int ret;
	u8 val;
	ret = s2mpw03_read_reg(s2mpw03->i2c, rdev->desc->vsel_reg, &val);
	if (ret)
		return ret;
	val &= rdev->desc->vsel_mask;
	return val;
}

static int s2m_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);
	int ret;

	ret = s2mpw03_update_reg(s2mpw03->i2c, rdev->desc->vsel_reg,
				  sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mpw03_update_reg(s2mpw03->i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);

	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

static int s2m_set_voltage_sel_regmap_buck(struct regulator_dev *rdev,
					unsigned sel)
{
	int ret;
	struct s2mpw03_info *s2mpw03 = rdev_get_drvdata(rdev);

	ret = s2mpw03_write_reg(s2mpw03->i2c, rdev->desc->vsel_reg, sel);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mpw03_update_reg(s2mpw03->i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);

	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

static int s2m_set_voltage_time_sel(struct regulator_dev *rdev,
				   unsigned int old_selector,
				   unsigned int new_selector)
{
	unsigned int ramp_delay = 0;
	int old_volt, new_volt;

	if (rdev->constraints->ramp_delay)
		ramp_delay = rdev->constraints->ramp_delay;
	else if (rdev->desc->ramp_delay)
		ramp_delay = rdev->desc->ramp_delay;

	if (ramp_delay == 0) {
		pr_warn("%s: ramp_delay not set\n", rdev->desc->name);
		return -EINVAL;
	}

	/* sanity check */
	if (!rdev->desc->ops->list_voltage)
		return -EINVAL;

	old_volt = rdev->desc->ops->list_voltage(rdev, old_selector);
	new_volt = rdev->desc->ops->list_voltage(rdev, new_selector);

	if (old_selector < new_selector)
		return DIV_ROUND_UP(new_volt - old_volt, ramp_delay);

	return 0;
}

u32 pmic_rev_get(void)
{
	return SEC_PMIC_REV(static_info->iodev);
}

static int s2mpw03_read_pwron_status(void)
{
	u8 val;
	struct s2mpw03_info *s2mpw03 = static_info;

	s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_STATUS1, &val);
	pr_info("%s: 0x%02hhx\n", __func__, val);

	return (val & S2MPW03_STATUS1_PWRON_M);
}

static struct regulator_ops s2mpw03_bst_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
	.set_mode		= s2m_set_mode,
};

static struct regulator_ops s2mpw03_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
	.set_mode		= s2m_set_mode,
};

static struct regulator_ops s2mpw03_buck_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap_buck,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
	.set_mode		= s2m_set_mode,
	.set_ramp_delay		= s2m_set_ramp_delay,
};

#define _BUCK(macro)	S2MPW03_BUCK##macro
#define _buck_ops(num)	s2mpw03_buck_ops##num

#define _LDO(macro)	S2MPW03_LDO##macro
#define _REG(ctrl)	S2MPW03_PMIC_REG##ctrl
#define _ldo_ops(num)	s2mpw03_ldo_ops##num
#define _TIME(macro)	S2MPW03_ENABLE_TIME##macro

#define _BST(macro)	S2MPW03_BST##macro
#define _bst_ops(num)	s2mpw03_bst_ops##num

#define _RANGE_BUCK(num)	S2MPW03_BUCK_RANGE##num
#define _RANGE_LDO(num)		S2MPW03_LDO_RANGE##num
#define _RANGE_BST(num)		S2MPW03_BST_RANGE##num

#define _MIN(range)	range##_MIN
#define _STEP(range)	range##_STEP

#define BUCK_DESC(_name, _id, _ops, r, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _MIN(r),				\
	.uV_step	= _STEP(r),				\
	.n_voltages	= S2MPW03_BUCK_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPW03_BUCK_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPW03_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode 	= s2mpw03_of_map_mode			\
}

#define LDO_DESC(_name, _id, _ops, r, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _MIN(r),				\
	.uV_step	= _STEP(r),				\
	.n_voltages	= S2MPW03_LDO_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPW03_LDO_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPW03_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode 	= s2mpw03_of_map_mode			\
}

#define BST_DESC(_name, _id, _ops, r, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _MIN(r),				\
	.uV_step	= _STEP(r),				\
	.n_voltages	= S2MPW03_BST_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPW03_BST_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPW03_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode 	= s2mpw03_of_map_mode			\
}

static struct regulator_desc regulators[S2MPW03_REGULATOR_MAX] = {
	/* name, id, ops, min_uv, uV_step, vsel_reg, enable_reg */
	LDO_DESC("LDO1", _LDO(1), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L1CTRL), _REG(_L1CTRL), _TIME(_LDO)),
	LDO_DESC("LDO2", _LDO(2), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L2CTRL), _REG(_L2CTRL), _TIME(_LDO)),
	LDO_DESC("LDO3", _LDO(3), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L3CTRL), _REG(_L3CTRL), _TIME(_LDO)),
	LDO_DESC("LDO4", _LDO(4), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L4CTRL), _REG(_L4CTRL), _TIME(_LDO)),
	LDO_DESC("LDO5", _LDO(5), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L5CTRL), _REG(_L5CTRL), _TIME(_LDO)),
	LDO_DESC("LDO6", _LDO(6), &_ldo_ops(), _RANGE_LDO(5),
			_REG(_L6CTRL), _REG(_L6CTRL), _TIME(_LDO)),
	LDO_DESC("LDO7", _LDO(7), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L7CTRL), _REG(_L7CTRL), _TIME(_LDO)),
	LDO_DESC("LDO8", _LDO(8), &_ldo_ops(), _RANGE_LDO(4),
			_REG(_L8CTRL), _REG(_L8CTRL), _TIME(_LDO)),
	LDO_DESC("LDO9", _LDO(9), &_ldo_ops(), _RANGE_LDO(4),
			_REG(_L9CTRL), _REG(_L9CTRL), _TIME(_LDO)),
	LDO_DESC("LDO10", _LDO(10), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L10CTRL), _REG(_L10CTRL), _TIME(_LDO)),
	LDO_DESC("LDO11", _LDO(11), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L11CTRL), _REG(_L11CTRL), _TIME(_LDO)),
	LDO_DESC("LDO12", _LDO(12), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L12CTRL), _REG(_L12CTRL), _TIME(_LDO)),
	LDO_DESC("LDO13", _LDO(13), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L13CTRL), _REG(_L13CTRL), _TIME(_LDO)),
	LDO_DESC("LDO14", _LDO(14), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L14CTRL), _REG(_L14CTRL), _TIME(_LDO)),
	LDO_DESC("LDO15", _LDO(15), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L15CTRL), _REG(_L15CTRL), _TIME(_LDO)),
	LDO_DESC("LDO16", _LDO(16), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L16CTRL), _REG(_L16CTRL), _TIME(_LDO)),
	LDO_DESC("LDO17", _LDO(17), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L17CTRL), _REG(_L17CTRL), _TIME(_LDO)),
	LDO_DESC("LDO18", _LDO(18), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L18CTRL), _REG(_L18CTRL), _TIME(_LDO)),
	LDO_DESC("LDO19", _LDO(19), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L19CTRL), _REG(_L19CTRL), _TIME(_LDO)),
	LDO_DESC("LDO20", _LDO(20), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L20CTRL), _REG(_L20CTRL), _TIME(_LDO)),
	LDO_DESC("LDO21", _LDO(21), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L21CTRL), _REG(_L21CTRL), _TIME(_LDO)),
	LDO_DESC("LDO22", _LDO(22), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L22CTRL), _REG(_L22CTRL), _TIME(_LDO)),
	LDO_DESC("LDO23", _LDO(23), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L23CTRL), _REG(_L23CTRL), _TIME(_LDO)),
	LDO_DESC("LDO24", _LDO(24), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L24CTRL), _REG(_L24CTRL), _TIME(_LDO)),
	LDO_DESC("LDO25", _LDO(25), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L25CTRL), _REG(_L25CTRL), _TIME(_LDO)),
	LDO_DESC("LDO26", _LDO(26), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L26CTRL), _REG(_L26CTRL), _TIME(_LDO)),
	LDO_DESC("LDO27", _LDO(27), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L27CTRL), _REG(_L27CTRL), _TIME(_LDO)),
	LDO_DESC("LDO28", _LDO(28), &_ldo_ops(), _RANGE_LDO(4),
			_REG(_L28CTRL2), _REG(_L28CTRL1), _TIME(_LDO)),
	LDO_DESC("LDO29", _LDO(29), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L29CTRL), _REG(_L29CTRL), _TIME(_LDO)),
	LDO_DESC("LDO30", _LDO(30), &_ldo_ops(), _RANGE_LDO(3),
			_REG(_L30CTRL), _REG(_L30CTRL), _TIME(_LDO)),
	LDO_DESC("LDO31", _LDO(31), &_ldo_ops(), _RANGE_LDO(1),
			_REG(_L31CTRL), _REG(_L31CTRL), _TIME(_LDO)),
	LDO_DESC("LDO32", _LDO(32), &_ldo_ops(), _RANGE_LDO(2),
			_REG(_L32CTRL), _REG(_L32CTRL), _TIME(_LDO)),
	BUCK_DESC("BUCK1", _BUCK(1), &_buck_ops(), _RANGE_BUCK(1),
			_REG(_B1OUT), _REG(_B1CTRL), _TIME(_BUCK1)),
	BUCK_DESC("BUCK2", _BUCK(2), &_buck_ops(), _RANGE_BUCK(2),
			_REG(_B2OUT1), _REG(_B2CTRL), _TIME(_BUCK2)),
	BUCK_DESC("BUCK3", _BUCK(3), &_buck_ops(), _RANGE_BUCK(2),
			_REG(_B3OUT1), _REG(_B3CTRL), _TIME(_BUCK3)),
	BUCK_DESC("BUCK4", _BUCK(4), &_buck_ops(), _RANGE_BUCK(1),
			_REG(_B4OUT1), _REG(_B4CTRL), _TIME(_BUCK4)),
	BST_DESC("BST1", _BST(1), &_bst_ops(), _RANGE_BST(1),
			_REG(_BST1CTRL), _REG(_BST1CTRL), _TIME(_BST)),
	BST_DESC("BST2", _BST(2), &_bst_ops(), _RANGE_BST(1),
			_REG(_BST2CTRL), _REG(_BST2CTRL), _TIME(_BST)),
};

#ifdef CONFIG_OF
static struct of_device_id s2mpw03_of_match[] = {
	{
		.compatible = "s2mpw03-regulator",
	},
	{},
};

static int s2mpw03_pmic_dt_parse_pdata(struct s2mpw03_dev *iodev,
					struct s2mpw03_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2mpw03_regulator_data *rdata;
	unsigned int i;

	pmic_np = iodev->dev->of_node;
	if (!pmic_np) {
		dev_err(iodev->dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(iodev->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		pdata->num_regulators++;
	}

	rdata = devm_kzalloc(iodev->dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		dev_err(iodev->dev,
			"could not allocate memory for regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;
	pdata->num_rdata = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < S2MPW03_REGULATOR_MAX; i++)
			if (!of_node_cmp(reg_np->name,
					regulators[i].name))
				break;
		if (i == ARRAY_SIZE(regulators)) {
			dev_warn(iodev->dev,
			"don't know how to configure regulator %s\n",
			reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(
						iodev->dev, reg_np,
						&regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
		pdata->num_rdata++;
	}

	return 0;
}
#else
static int s2mpw03_pmic_dt_parse_pdata(struct s2mpw03_dev *iodev,
					struct s2mpw03_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

static int s2mpw03_pmic_probe(struct platform_device *pdev)
{
	struct s2mpw03_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct s2mpw03_platform_data *pdata = iodev->pdata;
	struct regulator_config config = { };
	struct s2mpw03_info *s2mpw03;
	int i, ret;
	u8 val;

	pr_info("[PMIC] %s: start\n", __func__);

	s2mpw03 = devm_kzalloc(&pdev->dev, sizeof(struct s2mpw03_info),
				GFP_KERNEL);
	if (!s2mpw03)
		return -ENOMEM;

	s2mpw03->iodev = iodev;
	s2mpw03->i2c = iodev->pmic;

	pr_info("[PMIC] %s: iodev->phase: %d \n", __func__, iodev->phase);

	if (iodev->phase >= 2) {
		ret = s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_L1CTRL, &val);
		val &= 0x3F;
		pr_info("[PMIC] %s: L1CTRL:%02x \n", __func__, val);
		if (val >= 0x00 && val <= 0x30) {
			val += 20;
			/* set buck2_out2 default voltage to ldo1 voltage + 150mv in phase2 */
			ret = s2mpw03_write_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_B2OUT2, val);
			ret = s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_B2OUT2, &val);
			pr_info("[PMIC] %s: B2OUT2:%02x \n", __func__, val);
		}
	}

	pr_info("[PMIC] %s: pmic_ldo6_reg: %d \n", __func__, iodev->ldo6_reg);

	if (iodev->ldo6_reg >= 0) {
		/* enable LDO6's discharge registance option for phase2 */
		s2mpw03_update_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_LDO_EXT_DSCH2,
			0x01 << S2MPW03_PMIC_REG_LDO_EXT_DSCH_EN_SHIFT, 0x80);
		/* set LDO6's discharge registance to 50ohm after phase2 */
		s2mpw03_update_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_LDO_EXT_DSCH2,
			iodev->ldo6_reg << S2MPW03_PMIC_REG_LDO_EXT_DSCH_OPT_SHIFT, 0x60);
	}

	static_info = s2mpw03;

	if (iodev->dev->of_node) {
		ret = s2mpw03_pmic_dt_parse_pdata(iodev, pdata);
		if (ret)
			return ret;
	}

	if (!pdata) {
		dev_err(pdev->dev.parent, "Platform data not supplied\n");
		return -ENODEV;
	}

	platform_set_drvdata(pdev, s2mpw03);

#ifdef CONFIG_SEC_PM_DEBUG
	ret = s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_PWRONSRC,
			&pmic_onsrc);
	if (ret)
		dev_err(&pdev->dev, "failed to read PWRONSRC\n");

	ret = s2mpw03_read_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_OFFSRC,
			&pmic_offsrc);
	if (ret)
		dev_err(&pdev->dev, "failed to read OFFSRC\n");

	/* Clear OFFSRC register */
	ret = s2mpw03_write_reg(s2mpw03->i2c, S2MPW03_PMIC_REG_OFFSRC, 0);
	if (ret)
		dev_err(&pdev->dev, "failed to write OFFSRC\n");
#endif /* CONFIG_SEC_PM_DEBUG */

	for (i = 0; i < pdata->num_rdata; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &pdev->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2mpw03;
		config.of_node = pdata->regulators[i].reg_node;
		s2mpw03->opmode[id] = regulators[id].enable_mask;
		s2mpw03->rdev[i] = devm_regulator_register(&pdev->dev,
							&regulators[id], &config);
		if (IS_ERR(s2mpw03->rdev[i])) {
			ret = PTR_ERR(s2mpw03->rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n",
				i);
			s2mpw03->rdev[i] = NULL;
			return ret;
		}
	}

	s2mpw03->num_regulators = pdata->num_rdata;

	exynos_reboot_register_pmic_ops(NULL, NULL, NULL, s2mpw03_read_pwron_status);

	pr_info("[PMIC] %s: end\n", __func__);

	return 0;
}

static int s2mpw03_pmic_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct platform_device_id s2mpw03_pmic_id[] = {
	{ "s2mpw03-regulator", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, s2mpw03_pmic_id);

static int s2mpw03_pmic_suspend(struct device *dev)
{
	pr_info("%s\n", __func__);

	return 0;
}

static int s2mpw03_pmic_resume(struct device *dev)
{
	pr_info("%s\n", __func__);

	return 0;
}

static const struct dev_pm_ops s2mpw03_pmic_pm_ops = {
	.suspend_late = s2mpw03_pmic_suspend,
	.resume_early = s2mpw03_pmic_resume,
};

static struct platform_driver s2mpw03_pmic_driver = {
	.driver = {
		.name = "s2mpw03-regulator",
		.owner = THIS_MODULE,
		.pm = &s2mpw03_pmic_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(s2mpw03_of_match),
#endif
	},
	.probe = s2mpw03_pmic_probe,
	.remove = s2mpw03_pmic_remove,
	.id_table = s2mpw03_pmic_id,
};

static int __init s2mpw03_pmic_init(void)
{
	return platform_driver_register(&s2mpw03_pmic_driver);
}
subsys_initcall(s2mpw03_pmic_init);

static void __exit s2mpw03_pmic_exit(void)
{
	platform_driver_unregister(&s2mpw03_pmic_driver);
}
module_exit(s2mpw03_pmic_exit);

/* Module information */
MODULE_AUTHOR("Samsung LSI");
MODULE_DESCRIPTION("SAMSUNG S2MPW03 Regulator Driver");
MODULE_LICENSE("GPL");
