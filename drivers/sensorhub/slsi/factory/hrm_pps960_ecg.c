/*
 * Copyright (c)2013 Maxim Integrated Products, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "../ssp.h"
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define ECG_NAME_LENGTH			10
#define ECG_MODE_LENGTH			2
#define ECG_SELFTEST_LENGTH		15
#define ECG_LEADOFF_LENGTH		8
#define ECG_KEY_DET_LENGTH		1
#define ECG_NOISE_EXT_LENGTH	5
#define ECG_AC_TEST_5HZ_LENGTH	8
#define ECG_OTP_LENGTH			5
#define ECG_REG_READ_LENGTH		4
#define ECG_REG_WRITE_LENGTH	7

#define ECG_READ_CMD	1
#define ECG_WRITE_CMD	2

enum ecg_cmd_data {
	ECG_CMD_REG = 0,
	ECG_CMD_NAME,
	ECG_CMD_MODE,
	ECG_CMD_SELFTEST,
	ECG_CMD_LEADOFF,
	ECG_CMD_KEY_DET,
	ECG_CMD_NOISE_EXT,
	ECG_CMD_AC_TEST_5HZ,
	ECG_CMD_OTP = 9,
};

struct ecg_leadoff_data{
	uint8_t pass;
	uint8_t data;
	uint8_t mode[2];
	uint32_t debug;
} __attribute__ ((packed));

struct ecg_data{
	uint8_t pass;
	uint32_t data;
} __attribute__ ((packed));

struct ecg_selftest{
	uint8_t data[2];
	int32_t data_n;
	uint8_t otp;
	int32_t data_f;
	int32_t data_a;
} __attribute__ ((packed));

struct ecg_reg{
	uint8_t mode;
	uint8_t page;
	uint8_t address;
	int32_t value;
} __attribute__ ((packed));


static ssize_t ecg_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char name[ECG_NAME_LENGTH] = {0,};

	ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_NAME << 8) | ECG_READ_CMD, (char *)name, sizeof(name), 1000, ECG_NAME_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", name);
}

static ssize_t ecg_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint8_t mode[ECG_MODE_LENGTH] = {0,};
	uint8_t tmp[25] = {0,};
	char *strptr, *tkn;
	ssize_t len;

	struct ssp_data *data = dev_get_drvdata(dev);

	len = min(size, sizeof(tmp) - 1);

	memcpy(tmp, buf, len);
	pr_info("[SSP] %s - tmp[%d] %s\n", __func__, (int)size, tmp);

	tmp[len] = '\0';
	strptr = tmp;

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - command NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou8(tkn, 0, &mode[0])) {
		pr_err("[SSP] %s - command INVAL!\n", __func__);
		return -EINVAL;
	}

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - value NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou8(tkn, 0, &mode[1])) {
		pr_err("[SSP] %s - value INVAL!\n", __func__);
		return -EINVAL;
	}

	pr_info("mode: %u %u\n", mode[0], mode[1]);

	ssp_send_write_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_MODE << 8) | ECG_WRITE_CMD, (char *)mode, ECG_MODE_LENGTH, __func__);

	return size;
}

static ssize_t ecg_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint8_t mode[ECG_MODE_LENGTH] = {0,};
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_MODE << 8) | ECG_READ_CMD, (char *)mode, sizeof(mode), 1000, ECG_MODE_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %u\n", mode[0], mode[1]);
}

static ssize_t ecg_leadoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ecg_leadoff_data leadoff;
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_LEADOFF << 8) | ECG_READ_CMD, (char *)&leadoff, sizeof(leadoff), 1000, ECG_LEADOFF_LENGTH, __func__);
	if (ret != SUCCESS) {
		leadoff.data = 15;
	}
	return snprintf(buf, PAGE_SIZE, "%u %u %u %u %u\n", leadoff.pass, leadoff.data, leadoff.mode[0], leadoff.mode[1], leadoff.debug);
}

static ssize_t ecg_key_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint8_t key = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_KEY_DET << 8) | ECG_READ_CMD, (char *)&key, sizeof(key), 1000, ECG_KEY_DET_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u\n", key);
}

static ssize_t ecg_noise_external_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ecg_data noise;
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_NOISE_EXT << 8) | ECG_READ_CMD, (char *)&noise, sizeof(noise), 1000, ECG_NOISE_EXT_LENGTH, __func__);
	if (ret != SUCCESS) {
		noise.data = 0x7FFFFFFF;
	}

	return snprintf(buf, PAGE_SIZE, "%u %d.%03d\n", noise.pass, (noise.data/1000), (noise.data%1000));
}

static ssize_t ecg_ac_test_5hz_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int32_t test[2] = {0,};
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_AC_TEST_5HZ << 8) | ECG_READ_CMD, (char *)test, sizeof(test), 1000, ECG_AC_TEST_5HZ_LENGTH, __func__);
	if(ret != SUCCESS) {
		test[0] = 0x7FFFFFFF;
		test[1] = 0x7FFFFFFF;
	}
	return snprintf(buf, PAGE_SIZE, "%d.%03d %d.%03d\n", (test[0]/1000), (test[0]%1000), (test[1]/1000), (test[1]%1000));
}


static ssize_t ecg_otp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ecg_data opt;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_OTP << 8) | ECG_READ_CMD, (char *)&opt, sizeof(opt), 1000, ECG_OTP_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %u\n", opt.pass, opt.data);
}

static ssize_t ecg_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ecg_reg reg;
	uint8_t tmp[25] = {0,};
	char *strptr, *tkn;
	ssize_t len;

	struct ssp_data *data = dev_get_drvdata(dev);

	len = min(size, sizeof(tmp) - 1);

	memcpy(tmp, buf, len);
	pr_info("[SSP] %s - tmp[%d] %s\n", __func__, (int)size, tmp);

	tmp[len] = '\0';
	strptr = tmp;

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - mode NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou8(tkn, 0, &reg.mode)) {
		pr_err("[SSP] %s - mode INVAL!\n", __func__);
		return -EINVAL;
	}
	
	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - page NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou8(tkn, 0, &reg.page)) {
		pr_err("[SSP] %s - page INVAL!\n", __func__);
		return -EINVAL;
	}

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - address NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou8(tkn, 0, &reg.address)) {
		pr_err("[SSP] %s - address INVAL!\n", __func__);
		return -EINVAL;
	}

	tkn = strsep(&strptr, " ");
	if (!tkn) {
		pr_err("[SSP] %s - value NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou32(tkn, 0, &reg.value)) {
		pr_err("[SSP] %s - value INVAL!\n", __func__);
		return -EINVAL;
	}

	pr_info("[SSP] %s - mode: %d, page: %d, addr: %d, value: %d", __func__, reg.mode, reg.page, reg.address, reg.value);

	ssp_send_write_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_REG << 8) | ECG_WRITE_CMD, (char *)&reg, sizeof(reg), __func__);

	return size;
}

static ssize_t ecg_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t val = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_REG << 8) | ECG_READ_CMD, (char *)&val, sizeof(val), 1000, ECG_REG_READ_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u\n", val);
}

static ssize_t ecg_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	ssize_t count;

	mutex_lock(&data->sysfs_op_mtx);
	memcpy(data->hrm_ecg_lib, data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data,
	sizeof(data->hrm_ecg_lib));

	count = snprintf(buf, PAGE_SIZE,
	"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[0],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[1],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[2],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[3],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[4],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[5],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[6],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[7],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[8],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[9],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[10],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[11],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[12],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[13],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[14],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[15],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[16],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[17],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[18],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[19],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[20],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[21],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[22],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[23],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[24],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[25],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[26],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[27],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[28],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[29],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[30],
	data->buf[HRM_ECG_LIB_SENSOR].hrm_ecg_lib_data[31]);

	mutex_unlock(&data->sysfs_op_mtx);

	return count;
}

static ssize_t ecg_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ecg_selftest selftest;
	struct ssp_data *data = dev_get_drvdata(dev);	
	s32 float_data[3] = {0 , };
	int ret = 0;

	ret = ssp_send_read_factory_cmd(data, HRM_ECG_LIB_SENSOR, (ECG_CMD_SELFTEST<< 8) | ECG_READ_CMD, (char *)&selftest, sizeof(selftest), 5000, ECG_SELFTEST_LENGTH, __func__);
	if(ret != SUCCESS) {
		selftest.data[0] = 15;
		selftest.data[1] = 15;
	}
	float_data[0] = ((selftest.data_n%1000) > 0 ? (selftest.data_n%1000) : -(selftest.data_n%1000));
	float_data[1] = ((selftest.data_f%1000) > 0 ? (selftest.data_f%1000) : -(selftest.data_f%1000));
	float_data[2] = ((selftest.data_a%1000) > 0 ? (selftest.data_a%1000) : -(selftest.data_a%1000));

	return snprintf(buf, PAGE_SIZE, "%u %u %d.%03d %u %d.%03d %d.%03d\n",
		selftest.data[0], selftest.data[1], (selftest.data_n/1000), float_data[0], selftest.otp,
		(selftest.data_f/1000), float_data[1], (selftest.data_a/1000), float_data[2]);
}

static DEVICE_ATTR(reg, S_IRUGO | S_IWUSR | S_IWGRP, ecg_reg_show, ecg_reg_store);
static DEVICE_ATTR(name, S_IRUGO, ecg_name_show, NULL);
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR | S_IWGRP, ecg_mode_show, ecg_mode_store);
static DEVICE_ATTR(leadoff, S_IRUGO, ecg_leadoff_show, NULL);
static DEVICE_ATTR(key_det, S_IRUGO, ecg_key_det_show, NULL);
static DEVICE_ATTR(noise_external, S_IRUGO, ecg_noise_external_show, NULL);
static DEVICE_ATTR(ac_test_5hz, S_IRUGO, ecg_ac_test_5hz_show, NULL);
static DEVICE_ATTR(otp, S_IRUGO, ecg_otp_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, ecg_raw_data_show, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, ecg_selftest_show, NULL);


static struct device_attribute *ecg_attrs[] = {
	&dev_attr_reg,
	&dev_attr_name,
	&dev_attr_mode,
	&dev_attr_key_det,
	&dev_attr_leadoff,
	&dev_attr_noise_external,
	&dev_attr_ac_test_5hz,
	&dev_attr_otp,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	NULL,
};

void initialize_pps960_ecg_factorytest(struct ssp_data *data)
{
	sensors_register(data->hrm_ecg_device, data, ecg_attrs,
		"ecg_sensor");
	pr_err("[SSP]: %s\n", __func__);
}

void remove_pps960_ecg_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->hrm_ecg_device, ecg_attrs);
}

