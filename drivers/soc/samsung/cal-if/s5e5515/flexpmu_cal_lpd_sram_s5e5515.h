struct pmucal_seq lpd_sram_on[] = {
	// PGEN clear
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEMORY_CTRL_CHUB_OUT", 0x12860000, 0x3fa0, (0x10 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	// PGEN__FEEDBACK
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "MEMORY_CTRL_CHUB_IN", 0x12860000, 0x3fa4, (0x10 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	// RET:16 clear
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEMORY_CTRL_CHUB_OUT", 0x12860000, 0x3fa0, (0x10 << 16), (0x0 << 16), 0, 0, 0xffffffff, 0),
	// CEN:8 clear
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEMORY_CTRL_CHUB_OUT", 0x12860000, 0x3fa0, (0x10 << 8), (0x0 << 8), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq lpd_sram_off[] = {
	// PGEN set
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEMORY_CTRL_CHUB_OUT", 0x12860000, 0x3fa0, (0xf0 << 0), (0xf0 << 0), 0, 0, 0xffffffff, 0),
	// PGEN__FEEDBACK
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "MEMORY_CTRL_CHUB_IN", 0x12860000, 0x3fa4, (0xf0 << 0), (0xf0 << 0), 0, 0, 0xffffffff, 0),
};

unsigned int lpd_sram_on_size = ARRAY_SIZE(lpd_sram_on);
unsigned int lpd_sram_off_size = ARRAY_SIZE(lpd_sram_off);
