/*
 * include/linux/knox_gearpay.h
 *
 * Include file for the KNOX protection for Watch Samsung Pay.
 */
#ifndef _LINUX_KNOX_GEARPAY_H
#define _LINUX_KNOX_GEARPAY_H

#include <linux/types.h>

/* For Button press/release event */
typedef enum gearpay_button_event_s {
	BUTTON_EVENT_RELEASED  = (u32)0,
	BUTTON_EVENT_PRESSED
} gearpay_button_event_t;

typedef enum gearpay_strap_status_s {
	GEARPAY_STATUS_UNSTRAP = (u32)0,
	GEARPAY_STATUS_STRAP
} gearpay_strap_status_t;

/**
 * @brief knox_gearpay_hard_button_event
 * Called by Button driver code when button is pressed or released.
 */
void knox_gearpay_hard_button_event(gearpay_button_event_t button_event,
									u32 timestamp);

/**
 * @brief knox_gearpay_strap_status
 * Called by Sensor Hub driver code up on strap and unstrap event.
 */
void knox_gearpay_strap_status(gearpay_strap_status_t status);

#endif /* _LINUX_KNOX_GEARPAY_H */

