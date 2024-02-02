/* individual sequence descriptor for SHUB control - init, reset, release, shub_active_clear, shub_reset_req_clear */
struct pmucal_seq shub_init[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CTRL_REFCLK_PMU",                    0x11860000, 0x0b1c, (0x1 <<  4), (0x0 <<  4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CTRL_REFCLK_CHUB_VTS",               0x11860000, 0x0b24, (0x1 <<  4), (0x0 <<  4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CHUB_CTRL",                          0x11860000, 0x007c, (0x1 <<  9), (0x1 <<  9), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT,       "RESET_SEQUENCER_STATUS",             0x11860000, 0x0504, (0x7 << 12), (0x5 << 12), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CTRL_REFCLK_PMU",                    0x11860000, 0x0b1c, (0x1 <<  4), (0x1 <<  4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CTRL_REFCLK_CHUB_VTS",               0x11860000, 0x0b24, (0x1 <<  4), (0x1 <<  4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CHUB_CPU_OPTION",                    0x11860000, 0x8908, (0x1 << 15), (0x1 << 15), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CHUB_CPU_OPTION",                    0x11860000, 0x8908, (0x1 << 29), (0x1 << 29), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq shub_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ,       "RESET_SEQUENCER_STATUS",             0x11860000, 0x0504, (0x7 << 12),           0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq shub_cpu_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ,       "CHUB_CPU_STATUS",                    0x11860000, 0x8904, (0x1 << 0),           0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq shub_reset_release_config[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "RESET_ISOLATION_CHUB_CONFIGURATION", 0x11860000, 0x8be0, (0x1 <<  0), (0x1 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "OSCCLK_GATE_CHUB_CONFIGURATION",     0x11860000, 0x8a00, (0x1 <<  0), (0x1 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "RESET_CMU_CHUB_CONFIGURATION",       0x11860000, 0x8aa0, (0x3 <<  0), (0x3 <<  0), 0, 0, 0xffffffff, 0),
//PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "RESET_RETENTION_CMU_CHUB_CONFIGURATION",   0x11860000, 0x8aa0, (0x3 <<  0), (0x2 <<  0), 0, 0, 0xffffffff, 0),
//PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "RESET_RETENTION_CMU_CHUB_CONFIGURATION",   0x11860000, 0x8aa0, (0x3 <<  0), (0x3 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "LOGIC_RESET_CHUB_CONFIGURATION",     0x11860000, 0x8a40, (0x3 <<  0), (0x3 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "TOP_BUS_CHUB_CONFIGURATION",         0x11860000, 0x8940, (0x7 <<  0), (0x7 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "PMU_EVENT_INTERRUPT_ENABLE_GRP3",    0x11860000, 0x7f0c, (0x1 << 22), (0x1 << 22), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "GRP3_INTR_BID_ENABLE",		 0x11870000, 0x0300, (0x1 << 22), (0x1 << 22), 0, 0, 0xffffffff, 0),
};
//code loading after release_config
struct pmucal_seq shub_reset_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "CHUB_CPU_OPTION",                    0x11860000, 0x8908, (0x1 << 15), (0x1 << 15), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CHUB_CPU_CONFIGURATION",             0x11860000, 0x8900, (0x1 <<  0), (0x1 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "TOP_BUS_CHUB_OPTION",                0x11860000, 0x8948, (0x1 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq shub_reset_assert[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "GRP3_INTR_BID_ENABLE",		 0x11870000, 0x0300, (0x1 << 22), (0x0 << 22), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "PMU_EVENT_INTERRUPT_ENABLE_GRP3",    0x11860000, 0x7f0c, (0x1 << 22), (0x0 << 22), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "GRP3_INTR_BID_CLEAR",		 0x11870000, 0x030c, (0x1 << 22), (0x1 << 22), 0, 0, 0xffffffff, 0),
	//PMUCAL_SEQ_DESC(PMUCAL_DELAY,      "DELAY",                              0x11860000,    0x0, (0x0 <<  0),        1000, 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE,      "TOP_BUS_CHUB_OPTION",                0x11860000, 0x8948, (0x1 <<  0), (0x1 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "SWEEPER_CLEAN_CHUB_CONFIGURATION",   0x11860000, 0x8ba0, (0x1 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
//wait about 1ms
	PMUCAL_SEQ_DESC(PMUCAL_DELAY,      "DELAY",                              0x11860000,    0x0, (0x0 <<  0),        1000, 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "TOP_BUS_CHUB_CONFIGURATION",         0x11860000, 0x8940, (0x7 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "OSCCLK_GATE_CHUB_CONFIGURATION",     0x11860000, 0x8a00, (0x1 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "RESET_ISOLATION_CHUB_CONFIGURATION", 0x11860000, 0x8be0, (0x1 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CHUB_CPU_CONFIGURATION",             0x11860000, 0x8900, (0x1 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "LOGIC_RESET_CHUB_CONFIGURATION",     0x11860000, 0x8a40, (0x3 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "RESET_CMU_CHUB_CONFIGURATION",       0x11860000, 0x8aa0, (0x3 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
//PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "RESET_RETENTION_CMU_CHUB_CONFIGURATION",   0x11860000, 0x8a80, (0x3 <<  0), (0x0 <<  0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "SWEEPER_CLEAN_CHUB_CONFIGURATION",   0x11860000, 0x8ba0, (0x1 <<  0), (0x1 <<  0), 0, 0, 0xffffffff, 0),
#if 0
#endif
};
struct pmucal_shub pmucal_shub_list = {
		.init = shub_init,
		.status = shub_status,
		.cpu_status = shub_cpu_status,
		.reset_assert = shub_reset_assert,
		.reset_release_config = shub_reset_release_config,
		.reset_release = shub_reset_release,
		.num_init = ARRAY_SIZE(shub_init),
		.num_status = ARRAY_SIZE(shub_status),
		.num_cpu_status = ARRAY_SIZE(shub_cpu_status),
		.num_reset_assert = ARRAY_SIZE(shub_reset_assert),
		.num_reset_release_config = ARRAY_SIZE(shub_reset_release_config),
		.num_reset_release = ARRAY_SIZE(shub_reset_release),
};
unsigned int pmucal_shub_list_size = 1;
