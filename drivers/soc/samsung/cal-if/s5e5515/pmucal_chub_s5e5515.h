/* TODO: power sequence is not decided, ported from s5e9815 */

/* individual sequence descriptor for CHUB control - reset, config, release */

struct pmucal_seq chub_blk_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OUT",		0x12860000, 0x2120, (0x1 << 3), (0x0 << 3),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OUT",		0x12860000, 0x2120, (0x1 << 4), (0x0 << 4),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OUT",		0x12860000, 0x2120, (0x1 << 5), (0x1 << 5),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OUT",		0x12860000, 0x2120, (0x1 << 7), (0x1 << 7),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OUT",		0x12860000, 0x2120, (0x1 << 10), (0x1 << 10),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT,	"CHUB_IN",		0x12860000, 0x2124, (0x1 << 2), (0x1 << 2),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OUT",		0x12860000, 0x2120, (0x1 << 10), (0x0 << 10),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OUT",		0x12860000, 0x2120, (0x1 << 6), (0x1 << 6),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CRTC_CONTROL",		0x12860000, 0x73c, (0x1 << 0), (0x1 << 0),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CHUB_STATUS", 0x12860000, 0x2104, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq chub_reset_release_config[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_CONFIGURATION",	0x12860000, 0x2100, (0x1 << 0), (0x1 << 0),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT,	"CHUB_STATUS",		0x12860000, 0x2104, (0x1 << 0), (0x1 << 0),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OPTION",		0x12860000, 0x210c, (0x1 << 2), (0x0 << 2),0,0,0,0),
};

struct pmucal_seq chub_reset_assert[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_OPTION",	 	0x12860000, 0x210c, (0x1 << 2), (0x1 << 2),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_CONFIGURATION", 	0x12860000, 0x2100, (0x1 << 0), (0x0 << 0),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT,	"CHUB_STATUS", 		0x12860000, 0x2104, (0x1 << 0), (0x0 << 0),0,0,0,0),
};

struct pmucal_seq chub_reset_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_CPU_CONFIGURATION",	0x12860000, 0x3300, (0x1 << 0), (0x1 << 0),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT,	"CHUB_CPU_STATUS",		0x12860000, 0x3304, (0x1 << 0), (0x1 << 0),0,0,0,0),
};

struct pmucal_seq chub_cm55_reset_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,	"CHUB_CPU_CM55_CONFIGURATION",	0x12860000, 0x3500, (0x1 << 0), (0x1 << 0),0,0,0,0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT,	"CHUB_CPU_CM55_STATUS",		0x12860000, 0x3504, (0x1 << 0), (0x1 << 0),0,0,0,0),
};

struct pmucal_chub pmucal_chub_list = {
	.on = chub_blk_on,
	.off = chub_off,
	.reset_assert = chub_reset_assert,
	.reset_release_config = chub_reset_release_config,
	.reset_release = chub_reset_release,
	.cm55_reset_release = chub_cm55_reset_release,
	.num_on = ARRAY_SIZE(chub_blk_on),
	.num_off = ARRAY_SIZE(chub_off),
	.num_reset_assert = ARRAY_SIZE(chub_reset_assert),
	.num_reset_release_config = ARRAY_SIZE(chub_reset_release_config),
	.num_reset_release = ARRAY_SIZE(chub_reset_release),
	.num_cm55_reset_release = ARRAY_SIZE(chub_cm55_reset_release),
};

unsigned int pmucal_chub_list_size = 1;
