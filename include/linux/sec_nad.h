/* sec_nad.h
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

#ifndef SEC_NAD_H
#define SEC_NAD_H

#if defined(CONFIG_SEC_FACTORY)
#define NAD_PARAM_NAME "/dev/block/NAD_REFER"
#define NAD_OFFSET 8192
#define NAD_ENV_OFFSET (10240 * 2)

#define NAD_PARAM_READ	0
#define NAD_PARAM_WRITE	1
#define NAD_PARAM_EMPTY	2

#define NAD_DRAM_TEST_NONE	0
#define NAD_DRAM_TEST_PASS	1
#define NAD_DRAM_TEST_FAIL	2

#define NAD_BUFF_SIZE		10
#define NAD_CMD_LIST		3

#define NAD_ACAT_FAIL_SKIP_FLAG	1001
#define UNLIMITED_LOOP_FLAG		1002
#define NAD_SUPPORT_FLAG		1003
#define NAD_ACAT_FLAG			5001
#define NAD_SECOND_FLAG			6001
#define NAD_ACAT_SECOND_FLAG	5002

#define NAD_RETRY_COUNT			30
#define NAD_FAIL_COUNT			10

/* MAGIC CODE for NAD API Success */
#define MAGIC_NAD_API_SUCCESS	6057

struct nad_dram_test_info {
	u64 target_addr;	/* target address for test */
	u64 expected_val;	/* written val */
	u64 read_val;		/* read real value */

	unsigned int rank_id;		/* dram detail information*/
	unsigned int bank_id;
	unsigned int low_data;
	unsigned int col_data;

	unsigned int test_item;		/* mif or dram */
	unsigned int big_level;
	unsigned int lit_level;
	unsigned int mif_level;
	unsigned int int_level;
	unsigned int op_core_num;	/* operating CORE num */
};

struct nad_dram_information {
	char magic[8];
	unsigned int error_count;
	struct nad_dram_test_info nad_dram_fail_info[20];
};

struct nad_fail_information {
	unsigned int  das;
	char  das_string[10];
	unsigned int  block;
	char  block_string[10];
	unsigned int  level;
	unsigned int  vector;
	char  vector_string[10];
};

struct nad_fail_backup_data {
	unsigned int nad_inform1;
	unsigned int nad_inform2;

	struct nad_fail_information nad_fail_info;

};

#if defined(CONFIG_SEC_NAD_API)
static char nad_api_result_string[20*30] = {0,};

struct nad_api_results {
	char name[20];
	unsigned int result;
};
#endif

struct nad_env {
	char nad_factory[8];
	char nad_result[8];
	unsigned int nad_data;

	char nad_acat[8];
	char nad_acat_result[8];
	unsigned int nad_acat_data;
	unsigned int nad_acat_loop_count;
	unsigned int nad_acat_real_count;

	char nad_dram_test_need[8];
	unsigned int nad_dram_test_result;
	unsigned int nad_dram_fail_data;
	unsigned long long nad_dram_fail_address;

	int current_nad_status;
	unsigned int nad_acat_skip_fail;
	unsigned int unlimited_loop;
	unsigned int nad_support;
	unsigned int max_temperature;
	unsigned int nad_acat_max_temperature;

	unsigned int  nad_inform2_data;
	unsigned int  nad_inform3_data;
	unsigned int  nad_init_temperature;

	unsigned int  nad_acat_init_temperature;
	unsigned int  nad_acat_inform2_data;
	unsigned int  nad_acat_inform3_data;

	char nad_second[8];
	char nad_second_result[8];
	unsigned int  nad_second_data;
	unsigned int  nad_second_inform2_data;
	unsigned int  nad_second_inform3_data;
	unsigned int  nad_second_max_temperature;
	unsigned int  nad_second_init_temperature;

	char nad_acat_second[8];
	char nad_acat_second_result[8];
	unsigned int  nad_acat_second_data;
	unsigned int  nad_acat_second_inform2_data;
	unsigned int  nad_acat_second_inform3_data;
	unsigned int  nad_acat_second_max_temperature;
	unsigned int  nad_acat_second_init_temperature;

	struct nad_fail_information nad_fail_info;				/* 1st NAD */
	struct nad_fail_information nad_second_fail_info;		/* 2nd NAD */
	struct nad_fail_information nad_acat_fail_info;		/* ACAT 1st NAD */
	struct nad_fail_information nad_acat_second_fail_info;	/* ACAT 2nd NAD */

	struct nad_dram_information nad_dram_fail_information;
	struct nad_dram_information nad_second_dram_fail_information;
	struct nad_dram_information nad_acat_dram_fail_information;
	struct nad_dram_information nad_acat_second_dram_fail_information;

	unsigned int  nad_acat_second_running_count;
	int acat_fail_retry_count;
	struct nad_fail_backup_data acat_fail_backup[NAD_FAIL_COUNT];

#if defined(CONFIG_SEC_NAD_API)
	int nad_api_status;
	int nad_api_magic;
	int nad_api_total_count;
	struct nad_api_results nad_api_info[30];
#endif
	int nad_enter_status;

	char nad_extend[8];
	char nad_extend_result[8];
	unsigned int nadx_is_excuted;
	unsigned int nad_extend_data;
	unsigned int nad_extend_inform2_data;
	unsigned int nad_extend_inform3_data;
	unsigned int nad_extend_max_temperature;
	unsigned int nad_extend_init_temperature;
	unsigned int nad_extend_loop_count;
	unsigned int nad_extend_real_count;
	unsigned int nad_extend_skip_fail;
	struct nad_fail_information nad_extend_fail_info;
	struct nad_dram_information nad_extend_dram_fail_information;
	unsigned int nad_X_Pre_Domain_Level[7]; //{cl2_lv, cl1_lv, cl0_lv, g3d_lv, mif_lv, int_lv, disp_lv},
	unsigned int nad_X_Margin;

	unsigned int nadx_fail_count;
	struct nad_fail_backup_data nadx_fail_data_backup;

	char nad_extend_second[8];
	char nad_extend_second_result[8];
	unsigned int nad_extend_second_data;
	unsigned int nad_extend_second_inform2_data;
	unsigned int nad_extend_second_inform3_data;
	unsigned int nad_extend_second_max_temperature;
	unsigned int nad_extend_second_init_temperature;
	struct nad_fail_information nad_extend_second_fail_info;
	struct nad_dram_information nad_extend_second_dram_fail_information;
	unsigned int nad_X_second_Pre_Domain_Level[7]; //{cl2_lv, cl1_lv, cl0_lv, g3d_lv, mif_lv, int_lv, disp_lv},
	unsigned int nad_X_second_Margin;

	unsigned int nad_debug_enable;
	unsigned int nad_cp_disable;

	struct nad_fail_backup_data last_fail_data_backup;
	struct nad_dram_information nad_last_dram_fail_information;
	int last_nad_fail_status;
};

struct sec_nad_param {
	struct work_struct sec_nad_work;
	struct delayed_work sec_nad_delay_work;
	unsigned long offset;
	int state;
	int retry_cnt;
	int curr_cnt;
};

static struct sec_nad_param sec_nad_param_data;
static struct nad_env sec_nad_env;
extern unsigned int lpcharge;
static DEFINE_MUTEX(sec_nad_param_lock);
#endif

#endif
