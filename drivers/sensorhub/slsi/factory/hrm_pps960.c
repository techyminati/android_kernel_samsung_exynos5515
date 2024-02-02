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

#if 0
#define VENDOR		"TI"
#define CHIP_ID		"PPS960"
#define MODULE_NAME	"AFE4920"
#endif

#define EOL_DATA_FILE_PATH "/efs/FactoryApp/hrm_eol_data"
#define XTALK2_DATA_FILE_PATH "/efs/FactoryApp/hrm_xtalk2_data"

#define HRM_CMD_SELFTEST	0
#define HRM_CMD_LED_G_IR	1
#define HRM_CMD_NOISE		2
#define HRM_CMD_NAME		0
#define HRM_CMD_VENDOR		1
#define HRM_CMD_MODULE		2
#define HRM_CMD_IOCTL		0
#define HRM_CMD_EOL_SPEC	16
#define HRM_CMD_XTALK_SPEC	17
#define HRM_CMD_TRIM_CHECK	18

#define HRM_SELFTEST_LENGTH	24
#define HRM_LED_G_IR_LENGTH	12
#define HRM_NOISE_LENGTH	16
#define HRM_NAME_LENGTH		64
#define HRM_VENDOR_LENGTH	64
#define HRM_MODULE_LENGTH	64
#define HRM_IOCTL_LENGTH	64
#define HRM_SPEC_LENGTH		49
#define HRM_TRIM_LENGTH		24

struct hrm_led {
	int32_t g_led;
	int32_t ir;
	int32_t r_led;
};

static ssize_t hrm_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char name[HRM_NAME_LENGTH] = {0,};

	ssp_send_read_cmd(data, HRM_SENSOR_INFO, HRM_CMD_NAME, (char *)name, sizeof(name), 1000, HRM_NAME_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", name);
}

static ssize_t hrm_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char vendor[HRM_VENDOR_LENGTH] = {0,};

	ssp_send_read_cmd(data, HRM_SENSOR_INFO, HRM_CMD_VENDOR, (char *)vendor, sizeof(vendor), 1000, HRM_VENDOR_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", vendor);
}

static ssize_t hrm_module_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char module[HRM_MODULE_LENGTH] = {0,};

	ssp_send_read_cmd(data, HRM_SENSOR_INFO, HRM_CMD_MODULE, (char *)module, sizeof(module), 1000, HRM_MODULE_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", module);
}

static ssize_t hrm_led_g_ir_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iRet = 0;
	struct hrm_led led_g_ir;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_RAW_FAC_SENSOR, HRM_CMD_LED_G_IR, (char *)&led_g_ir, sizeof(led_g_ir), 1000, HRM_LED_G_IR_LENGTH, __func__);

	if ((led_g_ir.g_led > 2050000) && (led_g_ir.ir > 2050000) && (led_g_ir.r_led > 2050000))
		iRet = 1;
	else
		iRet = 0;

	ssp_dbg("[SSP] : %s, %d, %d, %d Ret:%d\n", __func__, led_g_ir.g_led, led_g_ir.ir, led_g_ir.r_led, iRet);

	return snprintf(buf, PAGE_SIZE, "%d\n", iRet);
}

static ssize_t hrm_noise_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iRet = 0;
	int32_t pd[4] = {0,};
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_RAW_FAC_SENSOR, HRM_CMD_NOISE, (char *)pd, sizeof(pd), 1000, HRM_NOISE_LENGTH, __func__);

	if ((pd[0] > 2050000) && (pd[1] > 2050000) && (pd[2] > 2050000) && (pd[3] > 2050000))
		iRet = 1;
	else
		iRet = 0;

	ssp_dbg("[SSP] : %s, %d, %d, %d, %d Ret:%d\n", __func__, pd[0], pd[1], pd[2], pd[3], iRet);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d %d\n", iRet, pd[0], pd[1], pd[2], pd[3]);
}

static int hrm_open_eol_data(struct ssp_data *data)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;
	u32 eol_data[HRM_EOL_DATA_SIZE];

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(EOL_DATA_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		pr_info("[SSP] %s : file open fail %d\n", __func__, iRet);
		memset(data->hrmcal, 0x00, sizeof(data->hrmcal));

		return iRet;
	}

	iRet = vfs_read(cal_filp, (char *)&eol_data, HRM_EOL_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);

	if (iRet != HRM_EOL_DATA_SIZE  * sizeof(u32))
		iRet = -EIO;

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	pr_info("[SSP] %s: [%d %d %d %d %d %d %d %d]\n", __func__,
		eol_data[0], eol_data[1], eol_data[2], eol_data[3],
		eol_data[4], eol_data[5], eol_data[6], eol_data[7]);
	pr_info("[SSP] %s: [%d %d %d %d %d %d %d %d]\n", __func__,
		eol_data[8], eol_data[9], eol_data[10], eol_data[11],
		eol_data[12], eol_data[13], eol_data[14], eol_data[15]);
	pr_info("[SSP] %s: [%d %d %d %d %d %d %d %d]\n", __func__,
		eol_data[16], eol_data[17], eol_data[18], eol_data[19],
		eol_data[20], eol_data[21], eol_data[22], eol_data[23]);

	memcpy(data->hrmcal, eol_data, sizeof(eol_data));

	return iRet;
}

static int save_hrm_eol_data(struct ssp_data *data)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(EOL_DATA_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open osc_reg_value file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return -EIO;
	}

	iRet = vfs_write(cal_filp, (char *)&data->buf[HRM_RAW_FAC_SENSOR], HRM_EOL_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);

	if (iRet != HRM_EOL_DATA_SIZE * sizeof(u32)) {
		pr_err("[SSP]: %s - Can't write hrm osc_reg_value to file\n",
			__func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return iRet;
}

static int hrm_open_xtalk2_data(struct ssp_data *data)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;
	u32 xtalk2_data[HRM_XTALK2_DATA_SIZE];

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(XTALK2_DATA_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		pr_info("[SSP] %s : file open fail %d\n", __func__, iRet);
		memset(data->hrmcal2, 0x00, sizeof(data->hrmcal2));

		return iRet;
	}

	iRet = vfs_read(cal_filp, (char *)&xtalk2_data, HRM_XTALK2_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);

	if (iRet != HRM_XTALK2_DATA_SIZE  * sizeof(u32))
		iRet = -EIO;

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	pr_info("[SSP] %s: [%d %d %d %d %d %d %d %d]\n", __func__,
		xtalk2_data[0], xtalk2_data[1], xtalk2_data[2], xtalk2_data[3],
		xtalk2_data[4], xtalk2_data[5], xtalk2_data[6], xtalk2_data[7]);
	pr_info("[SSP] %s: [%d %d %d %d %d %d %d %d]\n", __func__,
		xtalk2_data[8], xtalk2_data[9], xtalk2_data[10], xtalk2_data[11],
		xtalk2_data[12], xtalk2_data[13], xtalk2_data[14], xtalk2_data[15]);
	pr_info("[SSP] %s: [%d %d %d %d %d %d %d %d]\n", __func__,
		xtalk2_data[16], xtalk2_data[17], xtalk2_data[18], xtalk2_data[19],
		xtalk2_data[20], xtalk2_data[21], xtalk2_data[22], xtalk2_data[23]);

	memcpy(data->hrmcal2, xtalk2_data, sizeof(xtalk2_data));

	return iRet;
}

static int save_hrm_xtalk2_data(struct ssp_data *data)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(XTALK2_DATA_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open osc_reg_value file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return -EIO;
	}

	iRet = vfs_write(cal_filp, (char *)&data->buf[HRM_RAW_FAC2_SENSOR], HRM_XTALK2_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);

	if (iRet != HRM_XTALK2_DATA_SIZE * sizeof(u32)) {
		pr_err("[SSP]: %s - Can't write hrm osc_reg_value to file\n",
			__func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return iRet;
}

int pps960_set_hrm_calibration(struct ssp_data *data)
{
	int i, iRet = 1;

	if (!(data->uSensorState & (1ULL << HRM_RAW_SENSOR))) {
		for (i = 0; i < HRM_EOL_DATA_SIZE; i++)
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[i] = 0;

		for (i = 0; i < HRM_XTALK2_DATA_SIZE; i++)
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[i] = 0;

		pr_info("[SSP]: %s - Skip this function!!!"\
			", hrm sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);

		return 0;
	}

	pr_info("[SSP] %s: Set hrm eol cal data %d, %d, %d, %d, %d, %d, %d, %d",
		__func__, data->hrmcal[0], data->hrmcal[1], data->hrmcal[2],
		data->hrmcal[3], data->hrmcal[4], data->hrmcal[5],
		data->hrmcal[6], data->hrmcal[7]);
	pr_info("[SSP] %s: %d, %d, %d, %d, %d, %d, %d, %d\n", __func__,
		data->hrmcal[8], data->hrmcal[9], data->hrmcal[10],
		data->hrmcal[11], data->hrmcal[12], data->hrmcal[13],
		data->hrmcal[14], data->hrmcal[15]);
	pr_info("[SSP] %s: %d, %d, %d, %d, %d, %d, %d ,%d\n", __func__,
		data->hrmcal[16], data->hrmcal[17], data->hrmcal[18],
		data->hrmcal[19], data->hrmcal[20], data->hrmcal[21],
		data->hrmcal[22], data->hrmcal[23]);

	pr_info("[SSP] %s: Set hrm xtalk2 cal data %d, %d, %d, %d, %d, %d, %d, %d",
		__func__, data->hrmcal2[0], data->hrmcal2[1], data->hrmcal2[2],
		data->hrmcal2[3], data->hrmcal2[4], data->hrmcal2[5],
		data->hrmcal2[6], data->hrmcal2[7]);
	pr_info("[SSP] %s: %d, %d, %d, %d, %d, %d, %d, %d\n", __func__,
		data->hrmcal2[8], data->hrmcal2[9], data->hrmcal2[10],
		data->hrmcal2[11], data->hrmcal2[12], data->hrmcal2[13],
		data->hrmcal2[14], data->hrmcal2[15]);
	pr_info("[SSP] %s: %d, %d, %d, %d, %d, %d, %d ,%d\n", __func__,
		data->hrmcal2[16], data->hrmcal2[17], data->hrmcal2[18],
		data->hrmcal2[19], data->hrmcal2[20], data->hrmcal2[21],
		data->hrmcal2[22], data->hrmcal2[23]);

	return iRet;
}

int pps960_hrm_open_calibration(struct ssp_data *data)
{
	int iRet1, iRet2;

	iRet1 = hrm_open_eol_data(data);
	iRet2 = hrm_open_xtalk2_data(data);

	if (iRet1 == -EIO || iRet2 == -EIO) {
		return -EIO;
	} else {
		return iRet1 + iRet2;
	}
}

static ssize_t hrm_lib_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char version[HRM_LIB_VERSION_INFO_LENGTH]  = {0, };
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_cmd(data, HRM_LIB_VERSION_INFO, 0, (char *)version, sizeof(version), 1000, HRM_LIB_VERSION_INFO_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			version[0], version[1], version[2], version[3], version[4],
			version[5], version[6], version[7], version[8], version[9]);
}

static ssize_t hrm_eol_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	ssize_t count;

	mutex_lock(&data->sysfs_op_mtx);
	memcpy(data->hrmcal, data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data,
		sizeof(data->hrmcal));

	save_hrm_eol_data(data);
	set_hrm_calibration(data);

	count = snprintf(buf, PAGE_SIZE,
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[0],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[1],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[2],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[3],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[4],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[5],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[6],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[7],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[8],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[9],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[10],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[11],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[12],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[13],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[14],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[15],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[16],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[17],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[18],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[19],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[20],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[21],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[22],
			data->buf[HRM_RAW_FAC_SENSOR].hrm_eol_data[23]);
	mutex_unlock(&data->sysfs_op_mtx);

	return count;
}

static ssize_t hrm_eol_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet;
	int64_t dEnable;
	struct ssp_data *data = dev_get_drvdata(dev);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	mutex_lock(&data->sysfs_op_mtx);
	if (dEnable)
		atomic_set(&data->eol_enable, 1);
	else
		atomic_set(&data->eol_enable, 0);
	mutex_unlock(&data->sysfs_op_mtx);

	return size;
}

static ssize_t hrm_raw_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->buf[HRM_RAW_SENSOR].hrm_raw_value1,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value2,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value3,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value4,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value5,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value6,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value7,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value8,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value9,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value10,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value11,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value12,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value13,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value14,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value15,
		data->buf[HRM_RAW_SENSOR].hrm_raw_value16);
}

static ssize_t hrm_lib_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[HRM_LIB_SENSOR].hr,
		data->buf[HRM_LIB_SENSOR].rri,
		data->buf[HRM_LIB_SENSOR].snr);
}

static ssize_t hrm_eol_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;
	u32 eol_data[HRM_EOL_DATA_SIZE] = { 0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(EOL_DATA_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		memset(eol_data, 0, sizeof(eol_data));
		goto exit;
	}

	iRet = vfs_read(cal_filp, (char *)&eol_data, HRM_EOL_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);

	if (iRet != HRM_EOL_DATA_SIZE * sizeof(u32))
		iRet = -EIO;

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

exit:
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d "\
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		eol_data[0], eol_data[1], eol_data[2], eol_data[3],
		eol_data[4], eol_data[5], eol_data[6], eol_data[7],
		eol_data[8], eol_data[9], eol_data[10], eol_data[11],
		eol_data[12], eol_data[13], eol_data[14], eol_data[15],
		eol_data[16], eol_data[17], eol_data[18], eol_data[19],
		eol_data[20], eol_data[21], eol_data[22], eol_data[23]);
}

static ssize_t hrm_ir_threshold_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int32_t thres = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_cmd(data, HRM_IR_LEVEL_THRESHOLD, 0, (char *)&thres, sizeof(thres), 1000, HRM_IR_LEVEL_THRESHOLD_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", thres);
}

static ssize_t hrm_dqa_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int32_t dqa[HRM_DQA_LENGTH]  = {0, };
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_cmd(data, HRM_DQA, 0, (char *)dqa, sizeof(dqa), 1000, (HRM_DQA_LENGTH * 4), __func__);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			dqa[0], dqa[1], dqa[2], dqa[3], dqa[4],
			dqa[5], dqa[6], dqa[7], dqa[8], dqa[9],
			dqa[10], dqa[11], dqa[12], dqa[13], dqa[14],
			dqa[15], dqa[16], dqa[17], dqa[18], dqa[19], dqa[20], dqa[21]);
}

static ssize_t hrm_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iRet = 0;
	int32_t selftest[6]  = {0, };
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_RAW_FAC_SENSOR, HRM_CMD_SELFTEST, (char *)selftest, sizeof(selftest), 1000, HRM_SELFTEST_LENGTH, __func__);

	pr_info("[SSP] %s- gain=0x%x ambgain=0x%x ir=%d red=%d green=%d amb=%d\n",
		__func__, selftest[0], selftest[1], selftest[2],
		selftest[3], selftest[4], selftest[5]);

	if ((selftest[2] > 0 && selftest[2] < 4194304)
		&& (selftest[3] > 0 && selftest[3] < 4194304)
		&& (selftest[4] > 0 && selftest[4] < 4194304)
		&& (selftest[5] > 0 && selftest[5] < 4194304)) {
		if (selftest[2] == 0 && selftest[3] == 0 &&
			selftest[4] == 0 && selftest[5] == 0)
			iRet = 0;
		else
			iRet = 1;
	} else
		iRet = 0;

	return snprintf(buf, PAGE_SIZE, "%d\n", iRet);
}

static ssize_t hrm_xtalk2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	ssize_t count;

	mutex_lock(&data->sysfs_op_mtx);
	memcpy(data->hrmcal2, data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data,
		sizeof(data->hrmcal2));

	save_hrm_xtalk2_data(data);
//	set_hrm_calibration(data);

	count = snprintf(buf, PAGE_SIZE,
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[0],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[1],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[2],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[3],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[4],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[5],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[6],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[7],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[8],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[9],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[10],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[11],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[12],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[13],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[14],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[15],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[16],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[17],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[18],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[19],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[20],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[21],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[22],
			data->buf[HRM_RAW_FAC2_SENSOR].hrm_xtalk2_data[23]);
	mutex_unlock(&data->sysfs_op_mtx);

	return count;
}

static ssize_t hrm_xtalk2_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet;
	int64_t dEnable;
	struct ssp_data *data = dev_get_drvdata(dev);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	mutex_lock(&data->sysfs_op_mtx);
	if (dEnable)
		atomic_set(&data->xtalk2_enable, 1);
	else
		atomic_set(&data->xtalk2_enable, 0);
	mutex_unlock(&data->sysfs_op_mtx);

	return size;
}


static ssize_t hrm_xtalk2_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;
	u32 xtalk2_data[HRM_XTALK2_DATA_SIZE] = { 0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(XTALK2_DATA_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		memset(xtalk2_data, 0, sizeof(xtalk2_data));
		goto exit;
	}

	iRet = vfs_read(cal_filp, (char *)&xtalk2_data, HRM_XTALK2_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);

	if (iRet != HRM_XTALK2_DATA_SIZE * sizeof(u32))
		iRet = -EIO;

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

exit:
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d "\
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		xtalk2_data[0], xtalk2_data[1], xtalk2_data[2], xtalk2_data[3],
		xtalk2_data[4], xtalk2_data[5], xtalk2_data[6], xtalk2_data[7],
		xtalk2_data[8], xtalk2_data[9], xtalk2_data[10], xtalk2_data[11],
		xtalk2_data[12], xtalk2_data[13], xtalk2_data[14], xtalk2_data[15],
		xtalk2_data[16], xtalk2_data[17], xtalk2_data[18], xtalk2_data[19],
		xtalk2_data[20], xtalk2_data[21], xtalk2_data[22], xtalk2_data[23]);
}

static ssize_t hrm_ioctl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint8_t sbuf[HRM_IOCTL_LENGTH] = {0,};
	uint8_t tmp[300] = {0,};
	char *strptr, *tkn;
	int i;
	int iRet = 0;
	ssize_t len;

	struct ssp_data *data = dev_get_drvdata(dev);

	len = min(size, sizeof(tmp) - 1);

	memcpy(tmp, buf, len);
	pr_info("[SSP] %s - tmp[%d] %s\n", __func__, (int)size, tmp);

	tmp[len] = '\0';
	strptr = tmp;

	for (i = 0; i < HRM_IOCTL_LENGTH; i++) {
		tkn = strsep(&strptr, " ");
		if (!tkn)
			break;
		if (kstrtou8(tkn, 0, &sbuf[i])) {
			iRet = -EINVAL;
			break;
		}
	}

	if (iRet == -EINVAL) {
		pr_err("[SSP] %s : -EINVAL!\n", __func__);
		return iRet;
	}

	ssp_send_write_cmd(data, HRM_IOCTL, HRM_CMD_IOCTL, (char *)sbuf, HRM_IOCTL_LENGTH, __func__);

	pr_info("[SSP] %s - %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "\
			"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", __func__,
			sbuf[0], sbuf[1], sbuf[2], sbuf[3], sbuf[4], sbuf[5], sbuf[6], sbuf[7], sbuf[8], sbuf[9], sbuf[10], sbuf[11], sbuf[12], sbuf[13], sbuf[14], sbuf[15],
			sbuf[16], sbuf[17], sbuf[18], sbuf[19], sbuf[20], sbuf[21], sbuf[22], sbuf[23], sbuf[24], sbuf[25], sbuf[26], sbuf[27], sbuf[28], sbuf[29], sbuf[30], sbuf[31],
			sbuf[32], sbuf[33], sbuf[34], sbuf[35], sbuf[36], sbuf[37], sbuf[38], sbuf[39], sbuf[40], sbuf[41], sbuf[42], sbuf[43], sbuf[44], sbuf[45], sbuf[46], sbuf[47],
			sbuf[48], sbuf[49], sbuf[50], sbuf[51], sbuf[52], sbuf[53], sbuf[54], sbuf[55], sbuf[56], sbuf[57], sbuf[58], sbuf[59], sbuf[60], sbuf[61], sbuf[62], sbuf[63]);

	return size;
}

static ssize_t hrm_ioctl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint8_t sbuf[HRM_IOCTL_LENGTH] = {0,};
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_cmd(data, HRM_IOCTL, HRM_CMD_IOCTL, (char *)sbuf, sizeof(sbuf), 1000, HRM_IOCTL_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "\
							"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
							sbuf[0], sbuf[1], sbuf[2], sbuf[3], sbuf[4], sbuf[5], sbuf[6], sbuf[7], sbuf[8], sbuf[9], sbuf[10], sbuf[11], sbuf[12], sbuf[13], sbuf[14], sbuf[15],
							sbuf[16], sbuf[17], sbuf[18], sbuf[19], sbuf[20], sbuf[21], sbuf[22], sbuf[23], sbuf[24], sbuf[25], sbuf[26], sbuf[27], sbuf[28], sbuf[29], sbuf[30], sbuf[31],
							sbuf[32], sbuf[33], sbuf[34], sbuf[35], sbuf[36], sbuf[37], sbuf[38], sbuf[39], sbuf[40], sbuf[41], sbuf[42], sbuf[43], sbuf[44], sbuf[45], sbuf[46], sbuf[47],
							sbuf[48], sbuf[49], sbuf[50], sbuf[51], sbuf[52], sbuf[53], sbuf[54], sbuf[55], sbuf[56], sbuf[57], sbuf[58], sbuf[59], sbuf[60], sbuf[61], sbuf[62], sbuf[63]);
}

static ssize_t hrm_eol_spec_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t sbuf[HRM_SPEC_LENGTH] = {0,};
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_RAW_FAC_SENSOR, HRM_CMD_EOL_SPEC, (char *)sbuf, sizeof(sbuf), 1000, (HRM_SPEC_LENGTH * 4), __func__);

	return snprintf(buf, PAGE_SIZE, "%d,"\
							"IR Low DC, %d, %d, Green1 Low DC, %d, %d, Green2 Low DC, %d, %d, Red Low DC, %d, %d, "\
							"IR High DC, %d, %d, Green1 High DC, %d, %d, Green2 High DC, %d, %d, Red High DC, %d, %d, "\
							"System Noise, %d, %d, IR Frequency, %d, %d, PD1, %d, %d, PD2, %d, %d, PD3, %d, %d, PD4, %d, %d, "\
							"IR High Noise, %d, %d, Green1 High Noise, %d, %d, Green2 High Noise, %d, %d, Red High Noise, %d, %d, "\
							"IR Trim Check, %d, %d, G1 Trim Check, %d, %d, G2 Trim Check, %d, %d, "\
							"Red Trim Check, %d, %d, Trim Check, %d, %d, Design ID, %d, %d\n",
							sbuf[0], sbuf[1], sbuf[2], sbuf[3], sbuf[4], sbuf[5], sbuf[6], sbuf[7], sbuf[8], sbuf[9], sbuf[10], sbuf[11],
							sbuf[12], sbuf[13], sbuf[14], sbuf[15], sbuf[16], sbuf[17], sbuf[18], sbuf[19], sbuf[20], sbuf[21], sbuf[22],
							sbuf[23], sbuf[24], sbuf[25], sbuf[26], sbuf[27], sbuf[28], sbuf[29], sbuf[30], sbuf[31], sbuf[32], sbuf[33],
							sbuf[34], sbuf[35], sbuf[36], sbuf[37], sbuf[38], sbuf[39], sbuf[40], sbuf[41], sbuf[42], sbuf[43], sbuf[44],
							sbuf[45], sbuf[46], sbuf[47], sbuf[48]);
}
static ssize_t hrm_xtalk_spec_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t sbuf[HRM_SPEC_LENGTH] = {0,};
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_RAW_FAC_SENSOR, HRM_CMD_XTALK_SPEC, (char *)sbuf, sizeof(sbuf), 1000, (HRM_SPEC_LENGTH * 4), __func__);

	return snprintf(buf, PAGE_SIZE, "%d,"\
							"IR Low DC, %d, %d, Green1 Low DC, %d, %d, Green2 Low DC, %d, %d, Red Low DC, %d, %d, "\
							"IR High DC, %d, %d, Green1 High DC, %d, %d, Green2 High DC, %d, %d, Red High DC, %d, %d, "\
							"System Noise, %d, %d, IR Frequency, %d, %d, PD1, %d, %d, PD2, %d, %d, PD3, %d, %d, PD4, %d, %d, "\
							"IR High Noise, %d, %d, Green1 High Noise, %d, %d, Green2 High Noise, %d, %d, Red High Noise, %d, %d, "\
							"IR Trim Check, %d, %d, G1 Trim Check, %d, %d, G2 Trim Check, %d, %d, "\
							"Red Trim Check, %d, %d, Trim Check, %d, %d, Design ID, %d, %d\n",
							sbuf[0], sbuf[1], sbuf[2], sbuf[3], sbuf[4], sbuf[5], sbuf[6], sbuf[7], sbuf[8], sbuf[9], sbuf[10], sbuf[11],
							sbuf[12], sbuf[13], sbuf[14], sbuf[15], sbuf[16], sbuf[17], sbuf[18], sbuf[19], sbuf[20], sbuf[21], sbuf[22],
							sbuf[23], sbuf[24], sbuf[25], sbuf[26], sbuf[27], sbuf[28], sbuf[29], sbuf[30], sbuf[31], sbuf[32], sbuf[33],
							sbuf[34], sbuf[35], sbuf[36], sbuf[37], sbuf[38], sbuf[39], sbuf[40], sbuf[41], sbuf[42], sbuf[43], sbuf[44],
							sbuf[45], sbuf[46], sbuf[47], sbuf[48]);
}

static ssize_t hrm_trim_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t trim[HRM_SPEC_LENGTH] = {0,};
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_send_read_factory_cmd(data, HRM_RAW_FAC_SENSOR, HRM_CMD_TRIM_CHECK, (char *)trim, sizeof(trim), 1000, (HRM_SPEC_LENGTH * 4), __func__);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d %d %d\n", trim[0], trim[1], trim[2], trim[3], trim[4], trim[5]);
}

static ssize_t hrm_con_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	// 0: OK, 1: NG
	char con_det;
	struct ssp_data *data = dev_get_drvdata(dev);

	char *tmpbuf = NULL;
	int return_length = 0;
	int iRet = 0;

	struct ssp_msg *msg;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
		con_det = 1;
		return snprintf(buf, PAGE_SIZE, "%d\n", con_det);
	}

	msg->cmd = MSG2SSP_AP_READ_HRM_CON_DET_PIN;
	msg->length = sizeof(con_det);
	msg->options = AP2HUB_READ;
	msg->buffer = (char *)&con_det;
	msg->free_buffer = 0;
	msg->data = 0;

	pr_err("[SSP] %s(f) 0x%x - %d %d\n", __func__, msg->cmd, msg->data, msg->buffer[0]);

	iRet = ssp_send_command(data, msg->cmd, msg->options, 1000,
			msg->buffer, msg->length, &tmpbuf, &return_length, msg->data);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - transfer fail %d\n", __func__, iRet);
		con_det = 1;
	} else if (return_length != sizeof(con_det)) {
		pr_err("[SSP] %s - length is wrong (%d - %d)", __func__, return_length, sizeof(con_det));
		con_det = 1;
	} else {
		memcpy(&con_det, tmpbuf, return_length);
		ssp_dbg("[SSP] %s - %d", __func__, con_det);
	}

	kfree(tmpbuf);
	kfree(msg);

	return snprintf(buf, PAGE_SIZE, "%d\n", con_det);
}

static DEVICE_ATTR(name, S_IRUGO, hrm_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, hrm_vendor_show, NULL);
static DEVICE_ATTR(hrm_lib_ver, S_IRUGO, hrm_lib_version_show, NULL);
static DEVICE_ATTR(hrm_eol, S_IRUGO | S_IWUSR | S_IWGRP, hrm_eol_show, hrm_eol_store);
static DEVICE_ATTR(hrm_raw, S_IRUGO, hrm_raw_data_read, NULL);
static DEVICE_ATTR(hrm_lib, S_IRUGO, hrm_lib_data_read, NULL);
static DEVICE_ATTR(hrm_eol_data, S_IRUGO, hrm_eol_data_show, NULL);
static DEVICE_ATTR(hrm_ir_threshold, S_IRUGO, hrm_ir_threshold_show, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, hrm_selftest_show, NULL);
static DEVICE_ATTR(module, S_IRUGO, hrm_module_show, NULL);
static DEVICE_ATTR(hrm_led_g_ir, S_IRUGO, hrm_led_g_ir_show, NULL);
static DEVICE_ATTR(hrm_noise, S_IRUGO, hrm_noise_show, NULL);
static DEVICE_ATTR(hrm_dqa, S_IRUGO, hrm_dqa_show, NULL);
static DEVICE_ATTR(hrm_xtalk2, S_IRUGO | S_IWUSR | S_IWGRP, hrm_xtalk2_show, hrm_xtalk2_store);
static DEVICE_ATTR(hrm_xtalk2_data, S_IRUGO, hrm_xtalk2_data_show, NULL);
static DEVICE_ATTR(ioctl, S_IRUGO | S_IWUSR | S_IWGRP, hrm_ioctl_show, hrm_ioctl_store);
static DEVICE_ATTR(eol_spec, S_IRUGO, hrm_eol_spec_show, NULL);
static DEVICE_ATTR(xtalk_spec, S_IRUGO, hrm_xtalk_spec_show, NULL);
static DEVICE_ATTR(trim_check, S_IRUGO, hrm_trim_check_show, NULL);
static DEVICE_ATTR(hrm_con_det, S_IRUGO, hrm_con_det_show, NULL);


static struct device_attribute *hrm_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_hrm_lib_ver,
	&dev_attr_hrm_eol,
	&dev_attr_hrm_raw,
	&dev_attr_hrm_lib,
	&dev_attr_hrm_eol_data,
	&dev_attr_hrm_ir_threshold,
	&dev_attr_selftest,
	&dev_attr_module,
	&dev_attr_hrm_led_g_ir,
	&dev_attr_hrm_noise,
	&dev_attr_hrm_dqa,
	&dev_attr_ioctl,
	&dev_attr_hrm_xtalk2,
	&dev_attr_hrm_xtalk2_data,
	&dev_attr_eol_spec,
	&dev_attr_xtalk_spec,
	&dev_attr_trim_check,
	&dev_attr_hrm_con_det,
	NULL,
};

void initialize_pps960_hrm_factorytest(struct ssp_data *data)
{
	sensors_register(data->hrm_device, data, hrm_attrs,
		"hrm_sensor");
	pr_err("[SSP]: %s\n", __func__);
}

void remove_pps960_hrm_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->hrm_device, hrm_attrs);
}
