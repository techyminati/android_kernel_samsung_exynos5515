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
#include <linux/ctype.h>

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"TI"
#define CHIP_ID		"PPS960"
#define MODULE_NAME	"AFE4920"

#define BIA_DATA_FILE_PATH "/efs/FactoryApp/bia_data"  // BIA Parasitic Component Calibration

#define BIA_NAME_R_LENGTH       (10)
#define BIA_MODEL_R_LENGTH       (10)
#define BIA_FREQ_W_LENGTH       (1)
#define BIA_FREQ_R_LENGTH       (1)
#define BIA_MODE_W_LENGTH       (1)
#define BIA_MODE_R_LENGTH       (1)
#define BIA_TRIM_R_LENGTH       (5)
#define BIA_CURRENT_R_LENGTH    (5)
#define BIA_NV_W_LENGTH         (128)
#define BIA_NV_R_LENGTH         (137)
#define BIA_IMP_R_LENGTH        (49)
#define BIA_KEY_STR_LENGTH      (10)
#define BIA_VENDOR_R_LENGTH     (10)

#define BIA_CMD_FREQ    (0)
#define BIA_CMD_MODE    (1)
#define BIA_CMD_TRIM    (2)
#define BIA_CMD_CURR    (3)
#define BIA_CMD_SIMP    (4)
#define BIA_CMD_NV_R    (5)
#define BIA_CMD_NV_W    (6)
#define BIA_CMD_LIMP    (7)
#define BIA_CMD_MIMP    (8)
#define BIA_CMD_HIMP    (9)
#define BIA_CMD_NAME    (10)
#define BIA_CMD_KEY_STR	(11)
#define BIA_CMD_MODEL   (12)
#define BIA_CMD_VENDOR  (13)

#define BIA_IMP_DATA_SIZE	8

#define BIA_READ_CMD	0x1C
#define BIA_WRITE_CMD	0x1D

struct bia_imp_data {
	u8 pass;
	u32 body_imp;
	u32 imp_len;
	u32 contact_imp[BIA_IMP_DATA_SIZE];
	u32 stdev;
	u32 cur_value;
} __attribute__ ((packed));

struct bia_nv_param_data{
	u8 pass;
	u32 len;
	u32 data[BIA_NV_W_LENGTH/4];
	s32 time;
} __attribute__ ((packed));

struct bia_data{
	u8 pass;
	u32 data;
} __attribute__ ((packed));

static ssize_t bia_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char name[10] = {0,};

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_NAME << 8) | BIA_READ_CMD, (char *)name, sizeof(name), 1000, BIA_NAME_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", name);
}

static ssize_t bia_model_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char model[10] = {0,};

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_MODEL << 8) | BIA_READ_CMD, (char *)model, sizeof(model), 1000, BIA_MODEL_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", model);
}

static ssize_t bia_frequency_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	uint8_t freq = 0;

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_FREQ << 8) | BIA_READ_CMD, (char *)&freq, sizeof(freq), 1000, BIA_FREQ_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", (int)freq);
}

static ssize_t bia_frequency_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char buffer[BIA_FREQ_W_LENGTH] = {0,};
	int64_t freq;
	int iRet = 0;

	iRet = kstrtoll(buf, 10, &freq);
	if (iRet < 0)
		return iRet;

	buffer[0] = (char)freq;

	ssp_send_write_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_FREQ << 8) | BIA_WRITE_CMD, buffer, BIA_FREQ_W_LENGTH, __func__);

	return size;
}

static ssize_t bia_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char buffer[BIA_MODE_W_LENGTH] = {0,};
	int64_t dMode;
	int iRet = 0;

	iRet = kstrtoll(buf, 10, &dMode);
	if (iRet < 0)
		return iRet;

	buffer[0] = (char)dMode;

	ssp_send_write_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_MODE << 8) | BIA_WRITE_CMD, buffer, BIA_MODE_W_LENGTH, __func__);

	return size;
}

// AT+BIASENSO=0,0,0
static ssize_t bia_trim_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct bia_data trim;

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_TRIM << 8) | BIA_READ_CMD, (char *)&trim, sizeof(trim), 1000, BIA_TRIM_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %d\n", trim.pass, (s32)trim.data);
}

// AT+BIASENSO=0,0,1
static ssize_t bia_current_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct bia_data cur;

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_CURR << 8) | BIA_READ_CMD, (char *)&cur, sizeof(cur), 1000, BIA_CURRENT_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %x\n", cur.pass, cur.data);
}

#if 0 // test_code
static int save_bia_nv_param(struct ssp_data *data)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(BIA_DATA_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open osc_reg_value file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return -EIO;
	}

	//iRet = vfs_write(cal_filp, (char *)&data->buf[HRM_BIA_FAC_SENSOR], BIA_CAL_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);
	iRet = vfs_write(cal_filp, (char *)&data->biacal, BIA_CAL_DATA_SIZE * sizeof(u32), &cal_filp->f_pos);

	if (iRet != BIA_CAL_DATA_SIZE * sizeof(u32)) {
		pr_err("[SSP]: %s - Can't write hrm osc_reg_value to file\n",
			__func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return iRet;
}
#endif

int read_bia_nv_param(struct ssp_data *data)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	struct bia_nv_param_data nv_param;
	uint8_t tmp[350] = {0, };
	long fsize;
	char *strptr, *tkn;
	int i;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(BIA_DATA_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		pr_err("[SSP] %s : file open fail %d\n", __func__, iRet);
		return iRet;
	}

	fsize = cal_filp->f_path.dentry->d_inode->i_size;

	if(fsize > 350) {
		pr_err("[SSP]: %s - fsize : %d\n",__func__, fsize);
		fsize = 350;
	}

	iRet = vfs_read(cal_filp, (char *)&tmp, fsize, &cal_filp->f_pos);

	pr_info("[SSP] %s: %d %d\n", __func__, iRet, fsize);
	if (iRet !=  fsize) {
		pr_err("[SSP]: %s - Can't read data from file\n",__func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	if (iRet == -EIO){
		return iRet;
	}

	if(fsize < 350){
		tmp[fsize] = '\0';
	} else {
		tmp[349] = '\0';
	}

	strptr = tmp;

	tkn = strsep(&strptr, " ");
	if (tkn == NULL) {
		pr_err("[SSP] %s - pass NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtou8(tkn, 16, &nv_param.pass)) {
		pr_err("[SSP] %s - pass INVAL!\n", __func__);
		return -EINVAL;
	}

	tkn = strsep(&strptr, " ");
	if (tkn == NULL) {
		pr_err("[SSP] %s - len NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtouint(tkn, 16, &nv_param.len)) {
		pr_err("[SSP] %s - len INVAL!\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < BIA_CAL_DATA_SIZE; i++) {
		tkn = strsep(&strptr, " ");
		if (tkn == NULL) {
			pr_err("[SSP] %s : too few arguments (%d needed)", __func__, BIA_CAL_DATA_SIZE);
			return -EINVAL;
		}
		if(kstrtouint(tkn, 16, &nv_param.data[i])) {
			pr_err("[SSP] %s : kstros16 error %d", __func__);
			return -EINVAL;
		}
	}

	tkn = strsep(&strptr, " ");
	if (tkn == NULL) {
		pr_err("[SSP] %s - time NULL!\n", __func__);
		return -EINVAL;
	}
	if (kstrtoint(tkn, 16, &nv_param.time)) {
			pr_err("[SSP] %s - time INVAL!\n", __func__);
			return -EINVAL;
	}

	pr_info("[SSP] %s: [%u %u %d]\n", __func__,
		nv_param.pass, nv_param.len, nv_param.time);

	pr_info("[SSP] %s: [%x %x %x %x %x %x %x %x]\n", __func__,
		nv_param.data[0], nv_param.data[1], nv_param.data[2], nv_param.data[3],
		nv_param.data[4], nv_param.data[5], nv_param.data[6], nv_param.data[7]);
	pr_info("[SSP] %s: [%x %x %x %x %x %x %x %x]\n", __func__,
		 nv_param.data[8], nv_param.data[9], nv_param.data[10], nv_param.data[11],
		 nv_param.data[12], nv_param.data[13], nv_param.data[14], nv_param.data[15]);
	pr_info("[SSP] %s: [%x %x %x %x %x %x %x %x]\n", __func__,
		 nv_param.data[16], nv_param.data[17], nv_param.data[18], nv_param.data[19],
		 nv_param.data[20], nv_param.data[21], nv_param.data[22], nv_param.data[23]);
	pr_info("[SSP] %s: [%x %x %x %x %x %x %x %x]\n", __func__,
		 nv_param.data[24], nv_param.data[25], nv_param.data[26], nv_param.data[27],
		 nv_param.data[28], nv_param.data[29], nv_param.data[30], nv_param.data[31]);

	memcpy(data->biacal, nv_param.data, sizeof(data->biacal));

	return iRet;
}

int pps960_set_bia_calibration(struct ssp_data *data)
{
	int ret;
	s32 buffer[BIA_NV_W_LENGTH/4] = {0,};

	memcpy(&buffer[0], data->biacal, BIA_NV_W_LENGTH);
	ret = ssp_send_write_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_NV_W << 8) | BIA_WRITE_CMD, (char *)buffer, BIA_NV_W_LENGTH, __func__);

	if (ret != SUCCESS) {
		ssp_errf("ssp_send_command Fail %d", ret);
		return ret;
	}

	return ret;
}


// AT+BIASENSO=0,2,0
// read BIA Parasitic Component Calibration from SensorHub and return
static ssize_t bia_nv_param_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct bia_nv_param_data param;
	ssize_t count;

	// read NV Param from Sensorhub
	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_NV_R << 8) | BIA_READ_CMD, (char *)&param, sizeof(param), 1000, BIA_NV_R_LENGTH, __func__);

	// copy NV Param
	memcpy(data->biacal, param.data, sizeof(data->biacal));

	count = snprintf(buf, PAGE_SIZE, "%u %u %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %d\n",
		param.pass, param.len,
		data->biacal[0], data->biacal[1], data->biacal[2], data->biacal[3], data->biacal[4], data->biacal[5], data->biacal[6], data->biacal[7],
		data->biacal[8], data->biacal[9], data->biacal[10], data->biacal[11], data->biacal[12], data->biacal[13], data->biacal[14], data->biacal[15],
		data->biacal[16], data->biacal[17], data->biacal[18], data->biacal[19], data->biacal[20], data->biacal[21], data->biacal[22], data->biacal[23],
		data->biacal[24], data->biacal[25], data->biacal[26], data->biacal[27], data->biacal[28], data->biacal[29], data->biacal[30], data->biacal[31],
		param.time);

	return count;
}

// read BIA Parasitic Component Calibration from NV and send it to Sensorhub
static ssize_t bia_nv_param_write_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 buffer[BIA_NV_W_LENGTH/4] = {0,};
	ssize_t count;

	mutex_lock(&data->sysfs_op_mtx);
	read_bia_nv_param(data);
	memcpy(&buffer[0], data->biacal, BIA_NV_W_LENGTH);

	ssp_send_write_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_NV_W << 8) | BIA_WRITE_CMD, (char *)buffer, BIA_NV_W_LENGTH, __func__);

	count = snprintf(buf, PAGE_SIZE, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
		data->biacal[0], data->biacal[1], data->biacal[2], data->biacal[3], data->biacal[4], data->biacal[5], data->biacal[6], data->biacal[7],
		data->biacal[8], data->biacal[9], data->biacal[10], data->biacal[11], data->biacal[12], data->biacal[13], data->biacal[14], data->biacal[15],
		data->biacal[16], data->biacal[17], data->biacal[18], data->biacal[19], data->biacal[20], data->biacal[21], data->biacal[22], data->biacal[23],
		data->biacal[24], data->biacal[25], data->biacal[26], data->biacal[27], data->biacal[28], data->biacal[29], data->biacal[30], data->biacal[31]);

	mutex_unlock(&data->sysfs_op_mtx);

	return count;
}

// read buffer from user and send it to Sensorhub
static ssize_t bia_nv_param_write_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 buffer[BIA_NV_W_LENGTH/4] = {0, };
	int64_t result;
	char tmp[10] = {0, };
	int iRet = 0;
	int i, j = 0, cnt = 0;

	for (i = 0 ; i < size ; i++) {
		if (buf[i] != ' ' && buf[i] != '\n') {
			if (j >= 8) {
				pr_err("[SSP]: param size error (current limit - max 8 xdigit)\n");
				return size;
			}
			if (!isxdigit(buf[i])) {
				pr_err("[SSP]: wrong character(%c)\n", buf[i]);
				return size;
			}
			tmp[j++] = buf[i];
		} else {
			iRet = kstrtoll(tmp, 16, &result);
			buffer[cnt++] = (s32)result;
			//pr_err("[SSP]: (%d) %d - 0x%x (%d)\n", iRet, buffer[cnt-1], buffer[cnt-1] , cnt-1);
			if (iRet < 0)
				return iRet;

			j = 0;
			memset(tmp, 0, sizeof(tmp));

			if(cnt >= BIA_NV_W_LENGTH/4) {
				break;
			}
		}
	}

	if (cnt != (BIA_NV_W_LENGTH/4)) {
		pr_err("[SSP]: total param length error (%d)\n", cnt);
		return size;
	} else {
		pr_err("[SSP]%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
				buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7],
				buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15],
				buffer[16], buffer[17], buffer[18], buffer[19], buffer[20], buffer[21], buffer[22], buffer[23],
				buffer[24], buffer[25], buffer[26], buffer[27], buffer[28], buffer[29], buffer[30], buffer[31]);
	}

	ssp_send_write_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_NV_W << 8) | BIA_WRITE_CMD, (char *)buffer, BIA_NV_W_LENGTH, __func__);

	return size;
}

// TBD
static ssize_t bia_sub_imp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct bia_imp_data imp;

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_SIMP << 8) | BIA_READ_CMD, (char *)&imp, sizeof(imp), 1000, BIA_IMP_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %x %u %x %x %x %x %x %x %x %x %x %x\n",
		imp.pass, imp.body_imp, imp.imp_len, imp.contact_imp[0], imp.contact_imp[1], imp.contact_imp[2], imp.contact_imp[3],
		imp.contact_imp[4], imp.contact_imp[5], imp.contact_imp[6], imp.contact_imp[7], imp.stdev, imp.cur_value);
}

// AT+BIASENSO=0,1,0
static ssize_t bia_low_imp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct bia_imp_data imp;

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_LIMP << 8) | BIA_READ_CMD, (char *)&imp, sizeof(imp), 1000, BIA_IMP_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %x %u %x %x %x %x %x %x %x %x %x %x\n",
		imp.pass, imp.body_imp, imp.imp_len, imp.contact_imp[0], imp.contact_imp[1], imp.contact_imp[2], imp.contact_imp[3],
		imp.contact_imp[4], imp.contact_imp[5], imp.contact_imp[6], imp.contact_imp[7], imp.stdev, imp.cur_value);
}

// AT+BIASENSO=0,1,1
static ssize_t bia_mid_imp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct bia_imp_data imp;

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_MIMP << 8) | BIA_READ_CMD, (char *)&imp, sizeof(imp), 1000, BIA_IMP_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %x %u %x %x %x %x %x %x %x %x %x %x\n",
		imp.pass, imp.body_imp, imp.imp_len, imp.contact_imp[0], imp.contact_imp[1], imp.contact_imp[2], imp.contact_imp[3],
		imp.contact_imp[4], imp.contact_imp[5], imp.contact_imp[6], imp.contact_imp[7], imp.stdev, imp.cur_value);
}

// AT+BIASENSO=0,1,2
static ssize_t bia_high_imp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct bia_imp_data imp;

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_HIMP << 8) | BIA_READ_CMD, (char *)&imp, sizeof(imp), 1000, BIA_IMP_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%u %x %u %x %x %x %x %x %x %x %x %x %x\n",
		imp.pass, imp.body_imp, imp.imp_len, imp.contact_imp[0], imp.contact_imp[1], imp.contact_imp[2], imp.contact_imp[3],
		imp.contact_imp[4], imp.contact_imp[5], imp.contact_imp[6], imp.contact_imp[7], imp.stdev, imp.cur_value);
}

static ssize_t bia_raw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	ssize_t count;

	mutex_lock(&data->sysfs_op_mtx);
	memcpy(data->hrm_bia_raw, data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data,
	sizeof(data->hrm_bia_raw));

	count = snprintf(buf, PAGE_SIZE,
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[0],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[1],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[2],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[3],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[4],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[5],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[6],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[7],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[8],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[9],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[10],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[11],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[12],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[13],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[14],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[15],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[16],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[17],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[18],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[19],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[20],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[21],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[22],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[23],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[24],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[25],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[26],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[27],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[28],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[29],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[30],
			data->buf[HRM_BIA_RAW_SENSOR].hrm_bia_raw_data[31]);

	mutex_unlock(&data->sysfs_op_mtx);

	return count;

}

static ssize_t bia_key_string_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	uint8_t str[BIA_KEY_STR_LENGTH] = {0, };

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_KEY_STR << 8) | BIA_READ_CMD, (char *)str, sizeof(str), 1000, BIA_KEY_STR_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d %d %d %d %d %d %d\n",
		str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8], str[9]);
}

static ssize_t bia_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char vendor[10] = {0,};

	ssp_send_read_factory_cmd(data, HRM_BIA_RAW_SENSOR, (BIA_CMD_VENDOR << 8) | BIA_READ_CMD, (char *)vendor, sizeof(vendor), 1000, BIA_VENDOR_R_LENGTH, __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", vendor);
}


static DEVICE_ATTR(name, S_IRUGO, bia_name_show, NULL);
static DEVICE_ATTR(model, S_IRUGO, bia_model_show, NULL);
static DEVICE_ATTR(frequency, S_IRUGO | S_IWUSR | S_IWGRP, bia_frequency_show, bia_frequency_store);
static DEVICE_ATTR(mode, /*S_IRUGO | */S_IWUSR | S_IWGRP, NULL, bia_mode_store);
static DEVICE_ATTR(trim, S_IRUGO, bia_trim_show, NULL);
static DEVICE_ATTR(curr, S_IRUGO, bia_current_show, NULL);
static DEVICE_ATTR(nv_param_read, S_IRUGO, bia_nv_param_read_show, NULL);
static DEVICE_ATTR(nv_param_write, S_IRUGO | S_IWUSR | S_IWGRP, bia_nv_param_write_show, bia_nv_param_write_store);
static DEVICE_ATTR(sub_imp, S_IRUGO, bia_sub_imp_show, NULL);
static DEVICE_ATTR(low_imp, S_IRUGO, bia_low_imp_show, NULL);
static DEVICE_ATTR(mid_imp, S_IRUGO, bia_mid_imp_show, NULL);
static DEVICE_ATTR(high_imp, S_IRUGO, bia_high_imp_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, bia_raw_show, NULL);
static DEVICE_ATTR(key_string, S_IRUGO, bia_key_string_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, bia_vendor_show, NULL);

static struct device_attribute *bia_attrs[] = {
	&dev_attr_name,
	&dev_attr_model,
	&dev_attr_frequency,
	&dev_attr_mode,
	&dev_attr_trim,
	&dev_attr_curr,
	&dev_attr_nv_param_read,
	&dev_attr_nv_param_write,
	&dev_attr_sub_imp,
	&dev_attr_low_imp,
	&dev_attr_mid_imp,
	&dev_attr_high_imp,
	&dev_attr_raw_data,
	&dev_attr_key_string,
	&dev_attr_vendor,
	NULL,
};

void initialize_pps960_bia_factorytest(struct ssp_data *data)
{
	struct device *ret; 	//int ret;

	ret = sensors_register(data->bia_device, data, bia_attrs,
		"bia_sensor");

	pr_err("[SSP_BIA] %s [0x%x] \n", __func__, ret);
}

void remove_pps960_bia_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->bia_device, bia_attrs);

	pr_err("[SSP_BIA] %s\n", __func__);
}
