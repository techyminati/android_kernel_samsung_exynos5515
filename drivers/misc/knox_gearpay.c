/*
 * knox_gearpay.c - Driver for providing API and
 * ioctl interface for KNOX protection for Samsung
 * pay on Samsung Gear/Watch.
 *
 * Copyright (C) 2021, Samsung Electronics
 * Santosh Patil <santosh.kp@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#include <linux/knox_gearpay.h>
#include <tzdev/tee_client_api.h>


#define LOG(FMT, args...) \
printk(KERN_DEBUG "%s:%s " FMT "\n", KBUILD_MODNAME, __func__, ##args)

#define LOG_INFO(FMT, args...) \
printk(KERN_INFO "%s:%s " FMT "\n", KBUILD_MODNAME, __func__, ##args)

#define LOG_ERR(FMT, args...) \
printk(KERN_ERR "%s:%s " FMT "\n", KBUILD_MODNAME, __func__, ##args)

#define LOG_ENTRY \
printk(KERN_DEBUG "%s:%s - Entry\n", KBUILD_MODNAME, __func__)

#define LOG_EXIT \
printk(KERN_DEBUG "%s:%s - Exit\n", KBUILD_MODNAME, __func__)

/* ********************* Start of TZ Calls ********************* */

/* Structures / defines to avoid headers */
#define CMD_SPAY_AUTHW_SET_HARDWARE_BUTTON			0x00000001
#define SPAY_AUTHW_MAX_ERROR_STR_LEN 				256

typedef struct tz_set_hardware_button_status_msg_cmd {
	uint32_t button_status;
} __attribute__ ((packed)) tz_set_hardware_button_status_msg_cmd_t;

typedef struct tz_set_hardware_button_status_msg_resp {
	uint32_t return_code;
	uint8_t error_msg[SPAY_AUTHW_MAX_ERROR_STR_LEN];
} __attribute__ ((packed)) tz_set_hardware_button_status_msg_resp_t;

typedef struct tz_set_hardware_button_status_msg_payload {
	union {
		tz_set_hardware_button_status_msg_cmd_t cmd;
		tz_set_hardware_button_status_msg_resp_t resp;
	} __attribute__ ((packed)) payload;
} __attribute__ ((packed)) tz_set_hardware_button_status_msg_payload_t;

#define CMD_SPAY_INIT					0x7FFFFFF1
#define TZ_SPAY_TIMA_MSR_MAX_SIZE		1024

typedef struct {
	uint32_t len;
	uint8_t blob[TZ_SPAY_TIMA_MSR_MAX_SIZE];
} __attribute__ ((packed)) secure_boot_measurement_t;

typedef struct tz_spay_init_msg_cmd {
	secure_boot_measurement_t measurement;
} __attribute__ ((packed)) tz_spay_init_msg_cmd_t;

typedef struct tz_spay_init_msg_resp {
	uint32_t return_code;
	struct {
		uint32_t len;
		uint8_t data[SPAY_AUTHW_MAX_ERROR_STR_LEN];
	} __attribute__ ((packed)) error_msg;
} __attribute__ ((packed)) tz_spay_init_msg_resp_t;

typedef struct tz_spay_init_msg_payload {
	union {
		tz_spay_init_msg_cmd_t cmd;
		tz_spay_init_msg_resp_t resp;
	} __attribute__ ((packed)) payload;
} __attribute__ ((packed)) tz_spay_init_msg_payload_t;


// Global variables
static const TEEC_UUID watchAuthTAuuid = {
	.timeLow = 0x0,
	.timeMid = 0x0,
	.timeHiAndVersion = 0x0,
	.clockSeqAndNode = {0x0, 0x0, 0x53, 0x70, 0x79, 0x61, 0x74, 0x57},
};


static TEEC_Context context;
static TEEC_Session session;
static bool isTALoaded;
static bool isTAInitialized;
static TEEC_Operation operation;

static bool loadAuthTA(void)
{

	uint32_t returnOrigin;
	TEEC_Result res;

	LOG_ENTRY;

	if (isTALoaded) {
		LOG_INFO("TA already loaded");
		return true;
	}

	res = TEEC_InitializeContext(NULL, &context);
	if (res != TEEC_SUCCESS) {
		LOG_ERR("InitializeContext failed: %x", res);
		return (res != TEEC_SUCCESS);
	}

	res = TEEC_OpenSession(&context,
							&session,
							&watchAuthTAuuid,
							0,
							NULL,
							NULL,
							&returnOrigin);

	if (res != TEEC_SUCCESS) {
		LOG_ERR("Loading of AUTH TA failed, %#x from %#x", res, returnOrigin);
		goto finalize;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
										TEEC_MEMREF_TEMP_OUTPUT,
										TEEC_NONE,
										TEEC_NONE);
	isTALoaded = true;
	LOG_INFO("Watch Auth TA loaded successfullly");
	return true;

finalize:
	TEEC_FinalizeContext(&context);
	isTAInitialized = false;
	isTALoaded = false;
	return false;
}


static void unLoadTA(void)
{
	LOG_ENTRY;
	TEEC_CloseSession(&session);
	TEEC_FinalizeContext(&context);
	isTAInitialized = false;
	isTALoaded = false;
}


static bool initAuthTA(void)
{
	uint32_t returnOrigin;
	TEEC_Result res;
	tz_spay_init_msg_payload_t req;
	tz_spay_init_msg_payload_t resp;
	LOG_ENTRY;

	memset(&req, 0x0, sizeof(tz_spay_init_msg_payload_t));
	memset(&resp, 0x0, sizeof(tz_spay_init_msg_payload_t));
	operation.params[0].tmpref.buffer = &req;
	operation.params[0].tmpref.size = sizeof(tz_spay_init_msg_payload_t);
	operation.params[1].tmpref.buffer = &resp;
	operation.params[1].tmpref.size = sizeof(tz_spay_init_msg_payload_t);

	res = TEEC_InvokeCommand(&session, CMD_SPAY_INIT,
							 &operation, &returnOrigin);
	if (res != TEEC_SUCCESS) {
		LOG_ERR("Init returned %#x from %#x", res, returnOrigin);
		return false;
	}
	isTAInitialized = true;
	return true;
}


static void spay_auth_watch_set_hardware_button_status(void)
{
	uint32_t returnOrigin;
	TEEC_Result res;
	tz_set_hardware_button_status_msg_payload_t req;
	tz_set_hardware_button_status_msg_payload_t resp;
	LOG_ENTRY;

	if (!isTALoaded) {
		LOG("Load Watch AUTH TA");
		if (!loadAuthTA()) {
			LOG_ERR("Failed to load TA");
			return;
		}
	}

	if (!isTAInitialized) {

		LOG("Initialize Watch Auth TA");
		if (!initAuthTA()) {
			LOG_ERR("Failed to init Auth TA");
			unLoadTA();
			return;
		}
	}

	LOG("Set hardware button");
	memset(&req, 0x0, sizeof(tz_set_hardware_button_status_msg_payload_t));
	memset(&resp, 0x0, sizeof(tz_set_hardware_button_status_msg_payload_t));
	operation.params[0].tmpref.buffer = &req;
	operation.params[0].tmpref.size =
	sizeof(tz_set_hardware_button_status_msg_payload_t);
	operation.params[1].tmpref.buffer = &resp;
	operation.params[1].tmpref.size =
	sizeof(tz_set_hardware_button_status_msg_payload_t);

	res = TEEC_InvokeCommand(&session,
							CMD_SPAY_AUTHW_SET_HARDWARE_BUTTON,
							&operation, &returnOrigin);
	if (res != TEEC_SUCCESS) {
		LOG_ERR("Failed set hardware button status: \
		%#x from %#x", res, returnOrigin);
	}
}

/* *********************** End of TZ Calls *********************** */

#define BUTTON_TIMEOUT		500	// milliseconds
#define DISABLE_LOCK
#define PREVIOUS_BUTTON_EVENT_IS_PRESSED	1
#define PREVIOUS_BUTTON_EVENT_IS_RELEASED	2

struct primary_button_event_data_t {
	struct timer_list button_timer;
	struct work_struct button_work;
#ifndef DISABLE_LOCK
	struct mutex lock;
	struct mutex lock_ta;
#endif
	u32 timer_started;
	u32 pressed_timestamp;
	u32 previous_button_event;
};

static struct primary_button_event_data_t button_event_data;

static void process_button_work(struct work_struct *work)
{
	LOG_INFO("Back Key longpress detected");
#ifndef DISABLE_LOCK
	mutex_lock(&button_event_data.lock_ta);
	spay_auth_watch_set_hardware_button_status();
	mutex_unlock(&button_event_data.lock_ta);
#else
	LOG_INFO("Calling TA API");
	spay_auth_watch_set_hardware_button_status();
#endif
}

static void primary_button_reset_values(void)
{
	LOG_ENTRY;
#ifndef DISABLE_LOCK
	mutex_lock(&button_event_data.lock);
#endif
	button_event_data.timer_started = 0;
	button_event_data.previous_button_event = 0;
	button_event_data.pressed_timestamp = 0;
#ifndef DISABLE_LOCK
	mutex_unlock(&button_event_data.lock);
#endif
	LOG_EXIT;
}

static void primary_button_timer_callback(struct timer_list *unused)
{
	LOG_ENTRY;
	LOG_INFO("Timer expired");
	if (button_event_data.previous_button_event == PREVIOUS_BUTTON_EVENT_IS_PRESSED) {
		LOG_INFO("Button pressed for more than 500ms");
		schedule_work(&button_event_data.button_work);
	} else {
		LOG_INFO("Control shouldn't reach here!");
	}
	primary_button_reset_values();
	LOG_EXIT;
}

/**
 * @brief knox_gearpay_hard_button_event
 * Called by Button driver code when button is pressed or released.
 */
void knox_gearpay_hard_button_event(gearpay_button_event_t button_event,
									u32 timestamp)
{
	LOG_ENTRY;
	LOG_INFO("Event: %d, Timestamp: %ld", button_event, timestamp);

#ifndef DISABLE_LOCK
	mutex_lock(&button_event_data.lock);
#endif
	switch (button_event) {
	case BUTTON_EVENT_PRESSED:
		if (button_event_data.previous_button_event != PREVIOUS_BUTTON_EVENT_IS_PRESSED) {
			button_event_data.pressed_timestamp = timestamp;
			button_event_data.previous_button_event = PREVIOUS_BUTTON_EVENT_IS_PRESSED;
			
			LOG_INFO("Button timer_started: %d", button_event_data.timer_started);
			if (button_event_data.timer_started == 0) {
				// Start timer
				LOG_INFO("Starting button timer");
				button_event_data.timer_started = 1;
				mod_timer(&button_event_data.button_timer,\
						jiffies + msecs_to_jiffies(BUTTON_TIMEOUT));

			}
		} else {
			LOG_INFO("Previous event was button pressed event, so ignore!");
		}
		break;

	case BUTTON_EVENT_RELEASED:
		if (button_event_data.previous_button_event == PREVIOUS_BUTTON_EVENT_IS_PRESSED) {
			if (timestamp - button_event_data.pressed_timestamp >= BUTTON_TIMEOUT) {
				LOG_INFO("Button pressed for more than 500ms");
				primary_button_reset_values();
				schedule_work(&button_event_data.button_work);
			} else {
				if (button_event_data.timer_started) {
					// Stop timer
					LOG_INFO("Stopping button timer");
					primary_button_reset_values();
					del_timer(&button_event_data.button_timer);
				}
			}
			button_event_data.previous_button_event = PREVIOUS_BUTTON_EVENT_IS_RELEASED;
		} else {
			LOG_INFO("Previous event was not button pressed event, so ignore!");
		}
		break;
	}

	LOG("button_event: %u", button_event);
	
#ifndef DISABLE_LOCK
	mutex_unlock(&button_event_data.lock);
#endif
}

/**
 * @brief knox_gearpay_strap_status
 * Called by Sensor Hub driver code up on strap and unstrap event.
 */
void knox_gearpay_strap_status(gearpay_strap_status_t status)
{
	LOG_INFO("knox_gearpay_strap_status called");
}


static int __init knox_gearpay_init(void)
{
	LOG_ENTRY;

	/* Initialize variables */
	button_event_data.timer_started = 0;
	button_event_data.pressed_timestamp = 0;

	timer_setup(&button_event_data.button_timer,
			primary_button_timer_callback, 0);

	INIT_WORK(&button_event_data.button_work, process_button_work);

#ifndef DISABLE_LOCK
	mutex_init(&button_event_data.lock);
	mutex_init(&button_event_data.lock_ta);
#endif

	LOG_INFO("knox_gearpay_init: completed...");

	return 0;
}
module_init(knox_gearpay_init);

static void __exit knox_gearpay_exit(void)
{
	cancel_work_sync(&button_event_data.button_work);
#ifndef DISABLE_LOCK
	mutex_destroy(&button_event_data.lock);
	mutex_destroy(&button_event_data.lock_ta);
#endif
}
module_exit(knox_gearpay_exit);

EXPORT_SYMBOL(knox_gearpay_hard_button_event);
EXPORT_SYMBOL(knox_gearpay_strap_status);

MODULE_DESCRIPTION("KNOX Samsung Pay protection driver");
MODULE_AUTHOR("Santosh Patil <santosh.kp@samsung.com>");
MODULE_LICENSE("GPL v2");
