/* sec_nad.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/sec_class.h>
#include <linux/sec_nad.h>
#include <linux/fs.h>

#if defined(CONFIG_SEC_FACTORY)
static void sec_nad_param_update(struct work_struct *work)
{
	int ret = -1;
	struct file *fp;
	struct sec_nad_param *param_data =
		container_of(work, struct sec_nad_param, sec_nad_work);

	pr_info("%s: set param at %s\n", __func__, param_data->state ? "write" : "read");

	fp = filp_open(NAD_PARAM_NAME, O_RDWR | O_SYNC, 0);
	if (IS_ERR(fp)) {
		pr_err("%s: filp_open error %ld\n", __func__, PTR_ERR(fp));

		/* Rerun workqueue when nad_refer read fail */
		if (param_data->retry_cnt > param_data->curr_cnt) {
			pr_info("retry count : %d\n", param_data->curr_cnt++);
			schedule_delayed_work(&sec_nad_param_data.sec_nad_delay_work, HZ * 1);
		}
		return;
	}
	ret = fp->f_op->llseek(fp, -param_data->offset, SEEK_END);
	if (ret < 0) {
		pr_err("%s: llseek error %d!\n", __func__, ret);
		goto close_fp_out;
	}

	switch (param_data->state) {
	case NAD_PARAM_WRITE:
		ret = vfs_write(fp, (char *)&sec_nad_env, sizeof(struct nad_env), &(fp->f_pos));

		if (ret < 0) {
			pr_err("%s: write error! %d\n", __func__, ret);
			goto close_fp_out;
		}

		pr_info(
			"NAD factory : %s\n"
			"NAD result : %s\n"
			"NAD data : 0x%x\n"
			"NAD ACAT : %s\n"
			"NAD ACAT result : %s\n"
			"NAD ACAT data : 0x%x\n"
			"NAD ACAT loop count : %d\n"
			"NAD ACAT real count : %d\n"
			"NAD DRAM test need : %s\n"
			"NAD DRAM test result : 0x%x\n"
			"NAD DRAM test fail data : 0x%x\n"
			"NAD DRAM test fail address : 0x%08lx\n"
			"NAD status : %d\n"
			"NAD ACAT skip fail : %d\n"
			"NAD unlimited loop : %d\n", sec_nad_env.nad_factory,
			sec_nad_env.nad_result,
			sec_nad_env.nad_data,
			sec_nad_env.nad_acat,
			sec_nad_env.nad_acat_result,
			sec_nad_env.nad_acat_data,
			sec_nad_env.nad_acat_loop_count,
			sec_nad_env.nad_acat_real_count,
			sec_nad_env.nad_dram_test_need,
			sec_nad_env.nad_dram_test_result,
			sec_nad_env.nad_dram_fail_data,
			sec_nad_env.nad_dram_fail_address,
			sec_nad_env.current_nad_status,
			sec_nad_env.nad_acat_skip_fail,
			sec_nad_env.unlimited_loop);
			pr_info("NAD X : %s\n", sec_nad_env.nad_extend);
			pr_info("sec_nad_env total size : %lu\n", sizeof(sec_nad_env));
			pr_info(
			"NADX : %s\n"
			"NADX result : %s\n"
			"NADX data : 0x%x\n"
			"NADX inform2_data : 0x%x\n"
			"NADX inform3_data : 0x%x\n"
			"NADX nadx_is_excuted : %d\n", sec_nad_env.nad_extend,
			sec_nad_env.nad_extend_result,
			sec_nad_env.nad_extend_data,
			sec_nad_env.nad_extend_inform2_data,
			sec_nad_env.nad_extend_inform3_data,
			sec_nad_env.nadx_is_excuted);
			pr_info(
			"NADX SECOND: %s\n"
			"NADX SECOND result : %s\n"
			"NADX SECOND data : 0x%x\n"
			"NADX SECOND inform2_data : 0x%x\n"
			"NADX SECOND inform3_data : 0x%x\n",
			sec_nad_env.nad_extend_second,
			sec_nad_env.nad_extend_second_result,
			sec_nad_env.nad_extend_second_data,
			sec_nad_env.nad_extend_second_inform2_data,
			sec_nad_env.nad_extend_second_inform3_data);
		break;
	case NAD_PARAM_READ:
		ret = vfs_read(fp, (char *)&sec_nad_env, sizeof(struct nad_env), &(fp->f_pos));

		if (ret < 0) {
			pr_err("%s: read error! %d\n", __func__, ret);
			goto close_fp_out;
		}

		pr_info(
			"NAD factory : %s\n"
			"NAD result : %s\n"
			"NAD data : 0x%x\n"
			"NAD ACAT : %s\n"
			"NAD ACAT result : %s\n"
			"NAD ACAT data : 0x%x\n"
			"NAD ACAT loop count : %d\n"
			"NAD ACAT real count : %d\n"
			"NAD DRAM test need : %s\n"
			"NAD DRAM test result : 0x%x\n"
			"NAD DRAM test fail data : 0x%x\n"
			"NAD DRAM test fail address : 0x%08lx\n"
			"NAD status : %d\n"
			"NAD ACAT skip fail : %d\n"
			"NAD unlimited loop : %d\n", sec_nad_env.nad_factory,
			sec_nad_env.nad_result,
			sec_nad_env.nad_data,
			sec_nad_env.nad_acat,
			sec_nad_env.nad_acat_result,
			sec_nad_env.nad_acat_data,
			sec_nad_env.nad_acat_loop_count,
			sec_nad_env.nad_acat_real_count,
			sec_nad_env.nad_dram_test_need,
			sec_nad_env.nad_dram_test_result,
			sec_nad_env.nad_dram_fail_data,
			sec_nad_env.nad_dram_fail_address,
			sec_nad_env.current_nad_status,
			sec_nad_env.nad_acat_skip_fail,
			sec_nad_env.unlimited_loop);
			pr_info("NAD X : %s\n", sec_nad_env.nad_extend);
			pr_info("sec_nad_env total size : %lu\n", sizeof(sec_nad_env));
			pr_info(
			"NADX : %s\n"
			"NADX result : %s\n"
			"NADX data : 0x%x\n"
			"NADX DAS : %s BLOCK : %s LEVEL : %d VECTOR : %s\n"
			"NADX nadx_is_excuted : %d\n", sec_nad_env.nad_extend,
			sec_nad_env.nad_extend_result,
			sec_nad_env.nad_extend_data,
			sec_nad_env.nad_extend_fail_info.das_string,
			sec_nad_env.nad_extend_fail_info.block_string,
			sec_nad_env.nad_extend_fail_info.level,
			sec_nad_env.nad_extend_fail_info.vector_string,
			sec_nad_env.nadx_is_excuted);
		break;
	}
close_fp_out:
	if (fp) {
		pr_info("%s: close\n", __func__);
		filp_close(fp, NULL);
	}

	pr_info("%s: exit %d\n", __func__, ret);
	return;
}

int sec_set_nad_param(int val)
{
	int ret = -1;

	mutex_lock(&sec_nad_param_lock);

	switch (val) {
	case NAD_PARAM_WRITE:
	case NAD_PARAM_READ:
		goto set_param;
	default:
		goto unlock_out;
	}

set_param:
	sec_nad_param_data.state = val;
	schedule_work(&sec_nad_param_data.sec_nad_work);

	/* how to determine to return success or fail ? */
	ret = 0;
unlock_out:
	mutex_unlock(&sec_nad_param_lock);
	return ret;
}

static void sec_nad_init_update(struct work_struct *work)
{
	int ret = -1;

	pr_info("%s\n", __func__);

	ret = sec_set_nad_param(NAD_PARAM_READ);
	if (ret < 0)
	pr_err("%s: read error! %d\n", __func__, ret);

}

static ssize_t show_nad_stat(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	pr_info("%s\n", __func__);

	if (!strncasecmp(sec_nad_env.nad_result, "FAIL", 4) && !strncasecmp(sec_nad_env.nad_second_result, "FAIL", 4)) {
		/* Both of NAD were failed. */
		return sprintf(buf,
			       "NG_3.1_L_0x%x_0x%x_0x%x_0x%x,IT(%d),MT(%d),DAS(%s),BLOCK(%s),"
				"FN(%s_%s_%d_%s),FD(0x%08llx_0x%08llx_0x%08llx),"
				"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx)\n",
				sec_nad_env.nad_data,
				sec_nad_env.nad_inform2_data,
				sec_nad_env.nad_second_data,
				sec_nad_env.nad_second_inform2_data,
				sec_nad_env.nad_init_temperature,
				sec_nad_env.max_temperature,
				sec_nad_env.nad_second_fail_info.das_string,
				sec_nad_env.nad_second_fail_info.block_string,
				sec_nad_env.nad_fail_info.das_string,
				sec_nad_env.nad_fail_info.block_string,
				sec_nad_env.nad_fail_info.level,
				sec_nad_env.nad_fail_info.vector_string,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].target_addr,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].read_val,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].expected_val,
				sec_nad_env.nad_second_fail_info.das_string,
				sec_nad_env.nad_second_fail_info.block_string,
				sec_nad_env.nad_second_fail_info.level,
				sec_nad_env.nad_second_fail_info.vector_string,
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].target_addr,
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].read_val,
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].expected_val);
	} else {
		return sprintf(buf, "OK_3.1_L_0x%x_0x%x_0x%x_0x%x,IT(%d),MT(%d),OT(0x%x),"
				"TN(%d)\n",
				sec_nad_env.nad_data,
				sec_nad_env.nad_inform2_data,
				sec_nad_env.nad_second_data,
				sec_nad_env.nad_second_inform2_data,
				sec_nad_env.nad_init_temperature,
				sec_nad_env.max_temperature,
				sec_nad_env.nad_inform3_data,
				sec_nad_env.nad_inform3_data & 0xFF);
	}
}
static DEVICE_ATTR(nad_stat, S_IRUGO, show_nad_stat, NULL);

static ssize_t store_nad_erase(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = -1;

	pr_info("%s\n", __func__);

	if (!strncmp(buf, "erase", 5)) {
		strncpy(sec_nad_env.nad_factory, "GAP", 3);
		strncpy(sec_nad_env.nad_second, "DUMM", 4);
#if defined(CONFIG_SEC_NAD_API)
		sec_nad_env.nad_api_status = 0;
#endif
		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0)
			pr_err("%s: write error! %d\n", __func__, ret);

		return count;
	} else
		return count;
}

static ssize_t show_nad_erase(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	pr_info("%s\n", __func__);

	if (!strncmp(sec_nad_env.nad_factory, "GAP", 3))
		return sprintf(buf, "OK\n");
	else
		return sprintf(buf, "NG\n");
}

static DEVICE_ATTR(nad_erase, S_IRUGO | S_IWUSR, show_nad_erase, store_nad_erase);

static ssize_t show_nad_acat(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	pr_info("%s\n", __func__);

	/* Check status if ACAT NAD was excuted */
	if (sec_nad_env.current_nad_status >= NAD_ACAT_FLAG) {
		if (!strncasecmp(sec_nad_env.nad_acat_result, "FAIL", 4) && !strncasecmp(sec_nad_env.nad_acat_second_result, "FAIL", 4)) {
			return sprintf(buf, "NG_3.0_L_0x%x_0x%x_0x%x_0x%x,%d,%d,HPM(NULL),%s,%s,"
					"FN(%s_%s_%d_%s),FD(0x%08llx_0x%08llx_0x%08llx),"
					"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx)\n",
					sec_nad_env.nad_acat_data,
					sec_nad_env.nad_acat_inform2_data,
					sec_nad_env.nad_acat_second_data,
					sec_nad_env.nad_acat_second_inform2_data,
					sec_nad_env.nad_acat_init_temperature,
					sec_nad_env.nad_acat_max_temperature,
					sec_nad_env.nad_acat_second_fail_info.das_string,
					sec_nad_env.nad_acat_second_fail_info.block_string,
					sec_nad_env.nad_acat_fail_info.das_string,
					sec_nad_env.nad_acat_fail_info.block_string,
					sec_nad_env.nad_acat_fail_info.level,
					sec_nad_env.nad_acat_fail_info.vector_string,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].target_addr,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].read_val,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].expected_val,
					sec_nad_env.nad_acat_second_fail_info.das_string,
					sec_nad_env.nad_acat_second_fail_info.block_string,
					sec_nad_env.nad_acat_second_fail_info.level,
					sec_nad_env.nad_acat_second_fail_info.vector_string,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].target_addr,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].read_val,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].expected_val);
		} else {
			return sprintf(buf, "OK_3.0_L_0x%x_0x%x_0x%x_0x%x,%d,%d,HPM(NULL),OT(0x%x)\n",
					sec_nad_env.nad_acat_data,
					sec_nad_env.nad_acat_inform2_data,
					sec_nad_env.nad_acat_second_data,
					sec_nad_env.nad_acat_second_inform2_data,
					sec_nad_env.nad_acat_init_temperature,
					sec_nad_env.nad_acat_max_temperature,
					sec_nad_env.nad_acat_inform3_data);
		}
	} else
		return sprintf(buf, "NO_3.0_L_NADTEST\n");
}

static ssize_t store_nad_acat(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = -1;
    int idx = 0;
    char temp[NAD_BUFF_SIZE*3];
	char nad_cmd[NAD_CMD_LIST][NAD_BUFF_SIZE];
	char *nad_ptr, *string;
	unsigned int len = 0;

	pr_info("buf : %s count : %d\n", buf, (int)count);

	if ((int)count < NAD_BUFF_SIZE)
		return -EINVAL;

	/* Copy buf to nad temp */
	len = (unsigned int)min(count, sizeof(temp) - 1);
	strncpy(temp, buf, len);
	temp[len] = '\0';
	string = temp;

	while (idx < NAD_CMD_LIST) {
		nad_ptr = strsep(&string, ",");
		if (nad_ptr ==  NULL || strlen(nad_ptr) >= NAD_BUFF_SIZE) {
			pr_info(" %s: invalid input\n", __func__);
			return -EINVAL;
		}
		strcpy(nad_cmd[idx++], nad_ptr);
	}

    /*
* ACAT NAD Write Data format
*
* nad_acat,20,0 : ACAT Loop test (20 cycles)
* nad_acat,20,1 : ACAT Loop & DRAM test (20 cycles & 1 DRAM test)
* nad_acat,0,1  : ACAT DRAM test (1 DRAM test)
*
*/
	if (!strncmp(buf, "nad_acat", 8)) {
		/* Get NAD loop count */
		ret = sscanf(nad_cmd[1], "%d\n", &sec_nad_env.nad_acat_loop_count);
		if (ret != 1)
			return -EINVAL;

		/* case 1 : ACAT NAD */
		if (sec_nad_env.nad_acat_loop_count > 0) {
			pr_info("ACAT NAD test command.\n");

			strncpy(sec_nad_env.nad_acat, "ACAT", 4);
			strncpy(sec_nad_env.nad_acat_second, "DUMM", 4);
			sec_nad_env.nad_acat_second_running_count = 0;
			sec_nad_env.acat_fail_retry_count = 0;
			sec_nad_env.nad_acat_real_count = 0;
			sec_nad_env.nad_acat_data = 0;
			sec_nad_env.nad_acat_real_count = 0;
			sec_nad_env.current_nad_status = 0;
			sec_nad_env.nad_acat_skip_fail = 0;
			sec_nad_env.unlimited_loop = 0;
			sec_nad_env.max_temperature = 0;
			sec_nad_env.nad_acat_max_temperature = 0;
		}

		if (sec_nad_env.nad_acat_loop_count ==  444) {
			pr_info("NADX test command.\n");
			strncpy(sec_nad_env.nad_extend, "NADX", 4);
			strncpy(sec_nad_env.nad_extend_result, "NULL", 4);
			sec_nad_env.nad_extend_init_temperature = 0;
			sec_nad_env.nad_extend_max_temperature = 0;
			sec_nad_env.nad_extend_data = 0;
			sec_nad_env.nad_extend_inform2_data = 0;
			sec_nad_env.nad_extend_inform3_data = 0;
			sec_nad_env.current_nad_status = 0;
			sec_nad_env.nad_extend_real_count = 0;
			sec_nad_env.nad_extend_loop_count = 1;
			sec_nad_env.nad_extend_skip_fail = 0;
			sec_nad_env.nad_acat_loop_count = 0;
			strncpy(sec_nad_env.nad_acat, "NONE", 4);
			strncpy(sec_nad_env.nad_acat_second, "DUMM", 4);
			sec_nad_env.nad_acat_second_running_count = 0;
			sec_nad_env.acat_fail_retry_count = 0;
			sec_nad_env.nad_acat_real_count = 0;
			sec_nad_env.nad_acat_data = 0;
			sec_nad_env.nad_acat_real_count = 0;
			sec_nad_env.current_nad_status = 0;
			sec_nad_env.nad_acat_skip_fail = 0;
			sec_nad_env.unlimited_loop = 0;
			sec_nad_env.max_temperature = 0;
			sec_nad_env.nad_acat_max_temperature = 0;
		}

		/* case 2 : DRAM test */
		if (!strncmp(nad_cmd[2], "1", 1)) {
			pr_info("DRAM test command.\n");

			strncpy(sec_nad_env.nad_dram_test_need, "DRAM", 4);
			sec_nad_env.nad_dram_test_result = 0;
			sec_nad_env.nad_dram_fail_data = 0;
			sec_nad_env.nad_dram_fail_address = 0;
		} else
			strncpy(sec_nad_env.nad_dram_test_need, "DONE", 4);

		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0) {
			pr_err("%s: write error! %d\n", __func__, ret);
			goto err_out;
		}

		return count;
	} else
		return count;
err_out:
	return ret;
}

static DEVICE_ATTR(nad_acat, S_IRUGO | S_IWUSR,  show_nad_acat, store_nad_acat);

static ssize_t show_nad_dram(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	pr_info("%s\n", __func__);

	if (sec_nad_env.nad_dram_test_result == NAD_DRAM_TEST_FAIL)
		return sprintf(buf, "NG_DRAM_0x%08lx,0x%x\n",
				sec_nad_env.nad_dram_fail_address,
				sec_nad_env.nad_dram_fail_data);
	else if (sec_nad_env.nad_dram_test_result == NAD_DRAM_TEST_PASS)
		return sprintf(buf, "OK_DRAM\n");
	else
		return sprintf(buf, "NO_DRAMTEST\n");
}
static DEVICE_ATTR(nad_dram, S_IRUGO, show_nad_dram, NULL);

static ssize_t show_nad_all(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf,
			"NAD factory : %s\n"
			"NAD result : %s\n"
			"NAD data : 0x%x\n"
			"NAD ACAT : %s\n"
			"NAD ACAT result : %s\n"
			"NAD ACAT data : 0x%x\n"
			"NAD ACAT loop count : %d\n"
			"NAD ACAT real count : %d\n"
			"NAD DRAM test need : %s\n"
			"NAD DRAM test result : 0x%x\n"
			"NAD DRAM test fail data : 0x%x\n"
			"NAD DRAM test fail address : 0x%08lx\n"
			"NAD status : %d\n"
			"NAD ACAT skip fail : %d\n"
			"NAD unlimited loop : %d\n", sec_nad_env.nad_factory,
			sec_nad_env.nad_result,
			sec_nad_env.nad_data,
			sec_nad_env.nad_acat,
			sec_nad_env.nad_acat_result,
			sec_nad_env.nad_acat_data,
			sec_nad_env.nad_acat_loop_count,
			sec_nad_env.nad_acat_real_count,
			sec_nad_env.nad_dram_test_need,
			sec_nad_env.nad_dram_test_result,
			sec_nad_env.nad_dram_fail_data,
			sec_nad_env.nad_dram_fail_address,
			sec_nad_env.current_nad_status,
			sec_nad_env.nad_acat_skip_fail,
			sec_nad_env.unlimited_loop);
}
static DEVICE_ATTR(nad_all, S_IRUGO, show_nad_all, NULL);

static ssize_t nad_x_run_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	int ret;

	if (sec_nad_env.nadx_is_excuted == 1) {
		sec_nad_env.nadx_is_excuted = 0;

		// clear nadx executed
		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0) {
			pr_err("%s: write error! %d\n", __func__, ret);
			return sprintf(buf, "%s\n", "RUN");
		}
		return sprintf(buf, "%s\n", "RUN");
	} else
		return sprintf(buf, "%s\n", "NORUN");

}
static DEVICE_ATTR(nad_x_run, S_IRUGO, nad_x_run_show, NULL);

static ssize_t nad_fac_result_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	if (!strncmp(sec_nad_env.nad_extend_result, "PASS", 4))
		return sprintf(buf, "%s\n", sec_nad_env.nad_extend_result);
	else if (!strncmp(sec_nad_env.nad_extend_result, "MAIN", 4))
		return sprintf(buf, "%s\n", "FAIL,MAIN_NAD");
	else if (!strncmp(sec_nad_env.nad_extend_result, "FAIL", 4) &&
			!strncasecmp(sec_nad_env.nad_extend_second_result, "FAIL", 4))
		return sprintf(buf, "FAIL,NG_NADX_DAS(%s),BLOCK(%s),LEVEL(%d),VECTOR(%s),PRE(%d,%d,%d,%d,%d,%d)\n",
					sec_nad_env.nad_extend_fail_info.das_string,
					sec_nad_env.nad_extend_fail_info.block_string,
					sec_nad_env.nad_extend_fail_info.level,
					sec_nad_env.nad_extend_fail_info.vector_string,
					sec_nad_env.nad_X_Pre_Domain_Level[0],
					sec_nad_env.nad_X_Pre_Domain_Level[1],
					sec_nad_env.nad_X_Pre_Domain_Level[2],
					sec_nad_env.nad_X_Pre_Domain_Level[3],
					sec_nad_env.nad_X_Pre_Domain_Level[4],
					sec_nad_env.nad_X_Pre_Domain_Level[5]);
	else if (!strncmp(sec_nad_env.nad_extend_result, "FAIL", 4) &&
			!strncasecmp(sec_nad_env.nad_extend_second_result, "PASS", 4))
		return sprintf(buf, "%s\n", sec_nad_env.nad_extend_second_result);
	else
		return sprintf(buf, "%s\n", "NG");
}

static ssize_t nad_fac_result_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int param_update = 0;
	int ret;

	pr_info("buf : %s count = %d\n", buf, (int)count);

	/* check cmd */
	if (!strncmp(buf, "nadx", 4)) {
		pr_info("run NADX command.\n");
		strncpy(sec_nad_env.nad_extend, "NADX", 4);
		strncpy(sec_nad_env.nad_extend_result, "NULL", 4);
		sec_nad_env.nad_extend_init_temperature = 0;
		sec_nad_env.nad_extend_max_temperature = 0;
		sec_nad_env.nad_extend_data = 0;
		sec_nad_env.nad_extend_inform2_data = 0;
		sec_nad_env.nad_extend_inform3_data = 0;
		sec_nad_env.current_nad_status = 0;
		sec_nad_env.nad_extend_real_count = 0;
		sec_nad_env.nad_extend_loop_count = 1;
		sec_nad_env.nad_extend_skip_fail = 0;
		param_update = 1;
	} else if (!strncmp(buf, "mainfail", 8)) {
		pr_info("update failed main nad result\n");
		strncpy(sec_nad_env.nad_extend_result, "MAIN", 4);
		param_update = 1;
	}

	if (param_update == 1) {
		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0) {
			pr_err("%s: write error! %d\n", __func__, ret);
			return -1;
		}
	}
	return count;
}
static DEVICE_ATTR(nad_fac_result, S_IRUGO | S_IWUSR, nad_fac_result_show, nad_fac_result_store);

static ssize_t show_nad_support(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	pr_info("%s\n", __func__);

	if (sec_nad_env.nad_support == NAD_SUPPORT_FLAG)
		return sprintf(buf, "SUPPORT_NADX\n");
	else
		return sprintf(buf, "NOT_SUPPORT\n");
}
static DEVICE_ATTR(nad_support, S_IRUGO, show_nad_support, NULL);

#if defined(CONFIG_SEC_NAD_API)
static void make_result_data_to_string(void)
{
	int i = 0;

	pr_info("%s : api total count(%d)\n", __func__, sec_nad_env.nad_api_total_count);

	/* Make result string if array is empty */
	if (!strlen(nad_api_result_string)) {
		for (i = 0; i < sec_nad_env.nad_api_total_count; i++) {
			pr_info("%s : name(%s) result(%d)\n", __func__,
				sec_nad_env.nad_api_info[i].name, sec_nad_env.nad_api_info[i].result);
			/* Failed gpio test */
			if (sec_nad_env.nad_api_info[i].result) {
				strcat(nad_api_result_string, sec_nad_env.nad_api_info[i].name);
				strcat(nad_api_result_string, ",");
			}
		}
		nad_api_result_string[strlen(nad_api_result_string)-1] = '\0';
	}
}

static ssize_t show_nad_api(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	pr_info("%s\n", __func__);

	/* Check nad api running status */
	if (sec_nad_env.nad_api_status == MAGIC_NAD_API_SUCCESS) {
		if (sec_nad_env.nad_api_magic == MAGIC_NAD_API_SUCCESS)
			return sprintf(buf, "OK_%d\n", sec_nad_env.nad_api_total_count);
		else {
			make_result_data_to_string();
			return sprintf(buf, "NG_%d,%s\n", sec_nad_env.nad_api_total_count, nad_api_result_string);
		}
	} else
	return sprintf(buf, "NONE\n");
}
static DEVICE_ATTR(nad_api, S_IRUGO, show_nad_api, NULL);
#endif
#endif

static int __init sec_nad_init(void)
{
	int ret = 0;
#if defined(CONFIG_SEC_FACTORY)
	struct device *sec_nad;

	pr_info("%s\n", __func__);

	/* Skip nad init when device goes to lp charging */
	if (lpcharge)
		return ret;

	sec_nad = sec_device_create(NULL, "sec_nad");

	if (IS_ERR(sec_nad)) {
		pr_err("%s Failed to create device(sec_nad)!\n", __func__);
		return PTR_ERR(sec_nad);
    }

	ret = device_create_file(sec_nad, &dev_attr_nad_stat);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_erase);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_acat);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_dram);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_all);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_support);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

#if defined(CONFIG_SEC_NAD_API)
	ret = device_create_file(sec_nad, &dev_attr_nad_api);
	if (ret) {
	pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}
#endif

	ret = device_create_file(sec_nad, &dev_attr_nad_fac_result);
	if (ret) {
		pr_err("%s: Failed to create fac_result file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_x_run);
	if (ret) {
		pr_err("%s: Failed to create fac_result file\n", __func__);
		goto err_create_nad_sysfs;
	}

	/* Initialize nad param struct */
	sec_nad_param_data.offset = NAD_ENV_OFFSET;
	sec_nad_param_data.state = NAD_PARAM_EMPTY;
	sec_nad_param_data.retry_cnt = NAD_RETRY_COUNT;
	sec_nad_param_data.curr_cnt = 0;

	INIT_WORK(&sec_nad_param_data.sec_nad_work, sec_nad_param_update);
	INIT_DELAYED_WORK(&sec_nad_param_data.sec_nad_delay_work, sec_nad_init_update);

#if defined(CONFIG_SEC_NAD_MANUAL_PARAM_READTIME)
	schedule_delayed_work(&sec_nad_param_data.sec_nad_delay_work, HZ * CONFIG_SEC_NAD_MANUAL_PARAM_READTIME);
#else
	schedule_delayed_work(&sec_nad_param_data.sec_nad_delay_work, HZ * 10);
#endif

	return 0;
err_create_nad_sysfs:
	sec_device_destroy(sec_nad->devt);
	return ret;
#else
	return ret;
#endif
}

static void __exit sec_nad_exit(void)
{
#if defined(CONFIG_SEC_FACTORY)
	cancel_work_sync(&sec_nad_param_data.sec_nad_work);
	cancel_delayed_work(&sec_nad_param_data.sec_nad_delay_work);
	pr_info("%s: exit\n", __func__);
#endif
}

module_init(sec_nad_init);
module_exit(sec_nad_exit);
