#ifndef __PMUCAL_SHUB_H__
#define __PMUCAL_SHUB_H__
#include "pmucal_common.h"

struct pmucal_chub {
	struct pmucal_seq *init;
	struct pmucal_seq *status;
	struct pmucal_seq *cpu_status;
	struct pmucal_seq *on;
	struct pmucal_seq *off;
	struct pmucal_seq *reset_assert;
	struct pmucal_seq *reset_release_config;
	struct pmucal_seq *reset_release;
	struct pmucal_seq *cm55_reset_release;
	u32 num_init;
	u32 num_status;
	u32 num_cpu_status;
	u32 num_on;
	u32 num_off;
	u32 num_reset_assert;
	u32 num_reset_release_config;
	u32 num_reset_release;
	u32 num_cm55_reset_release;
};

/* APIs to be supported to PWRCAL interface */
extern int pmucal_chub_initialize(void);

extern int pmucal_chub_init(void);
extern int pmucal_chub_status(void);
extern int pmucal_chub_cpu_status(void);
extern int pmucal_chub_on(void);
extern int pmucal_chub_reset_assert(void);
extern int pmucal_chub_reset_release_config(void);
extern int pmucal_chub_reset_release(void);
extern int pmucal_chub_cm55_reset_release(void);

extern struct pmucal_chub pmucal_chub_list;
extern unsigned int pmucal_chub_list_size;
#endif
