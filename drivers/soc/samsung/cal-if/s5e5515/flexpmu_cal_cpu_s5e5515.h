struct cpu_inform pmucal_cpuinform_list[] = {
	PMUCAL_CPU_INFORM(0, 0x12860000, 0x860),
	PMUCAL_CPU_INFORM(1, 0x12860000, 0x864),
};
unsigned int cpu_inform_list_size = ARRAY_SIZE(pmucal_cpuinform_list);
#ifndef ACPM_FRAMEWORK
/* individual sequence descriptor for each core - on, off, status */
struct pmucal_seq core00_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x12870000, 0x0200, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x12870000, 0x020c, (0x1 << 0), (0x1 << 0), 0x12870000, 0x0208, (0x1 << 0), (0x1 << 0) | (0x1 << 0)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU0_INT_EN", 0x12860000, 0x1044, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x12870000, 0x0200, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x12870000, 0x010c, (0x1 << 0), (0x1 << 0), 0x12870000, 0x0108, (0x1 << 0), (0x1 << 0) | (0x1 << 0)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x12870000, 0x010c, (0x1 << 8), (0x1 << 8), 0x12870000, 0x0108, (0x1 << 8), (0x1 << 8) | (0x1 << 8)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU0_INT_EN", 0x12860000, 0x1044, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x12860000, 0x1004, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core01_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x12870000, 0x0200, (0x1 << 1), (0x0 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x12870000, 0x020c, (0x1 << 1), (0x1 << 1), 0x12870000, 0x0208, (0x1 << 1), (0x1 << 1) | (0x1 << 1)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU1_INT_EN", 0x12860000, 0x10c4, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x12870000, 0x0200, (0x1 << 1), (0x1 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x12870000, 0x010c, (0x1 << 1), (0x1 << 1), 0x12870000, 0x0108, (0x1 << 1), (0x1 << 1) | (0x1 << 1)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x12870000, 0x010c, (0x1 << 9), (0x1 << 9), 0x12870000, 0x0108, (0x1 << 9), (0x1 << 9) | (0x1 << 9)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU1_INT_EN", 0x12860000, 0x10c4, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x12860000, 0x1084, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_on[] = {
};

struct pmucal_seq cluster0_off[] = {
};

struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x12860000, 0x1404, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	PMUCAL_NUM_CORES,
};

struct pmucal_cpu pmucal_cpu_list[PMUCAL_NUM_CORES] = {
	[CPU_CORE00] = {
		.id = CPU_CORE00,
		.release = 0,
		.on = core00_on,
		.off = core00_off,
		.status = core00_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core00_on),
		.num_off = ARRAY_SIZE(core00_off),
		.num_status = ARRAY_SIZE(core00_status),
	},
	[CPU_CORE01] = {
		.id = CPU_CORE01,
		.release = 0,
		.on = core01_on,
		.off = core01_off,
		.status = core01_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core01_on),
		.num_off = ARRAY_SIZE(core01_off),
		.num_status = ARRAY_SIZE(core01_status),
	},
};

unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	PMUCAL_NUM_CLUSTERS,
};

struct pmucal_cpu pmucal_cluster_list[PMUCAL_NUM_CLUSTERS] = {
	[CPU_CLUSTER0] = {
		.id = CPU_CLUSTER0,
		.on = cluster0_on,
		.off = cluster0_off,
		.status = cluster0_status,
		.num_on = ARRAY_SIZE(cluster0_on),
		.num_off = ARRAY_SIZE(cluster0_off),
		.num_status = ARRAY_SIZE(cluster0_status),
	},
};

unsigned int pmucal_cluster_list_size = ARRAY_SIZE(pmucal_cluster_list);

enum pmucal_opsnum {
	NUM_PMUCAL_OPTIONS,
};

struct pmucal_cpu pmucal_pmu_ops_list[] = {};

unsigned int pmucal_option_list_size = 0;

#else

struct pmucal_seq core00_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core00_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x12860000, 0x1004, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core01_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core01_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x12860000, 0x1084, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX_CPU", 0x15c00000, 0x0854, (0x3f << 0), (0x13 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_up", 0, 0, 0x28, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX_CPU", 0x15c00000, 0x0854, (0x3f << 0), (0x13 << 0), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX_CPU", 0x15c00000, 0x0854, (0x3f << 0), (0x13 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_down", 0, 0, 0x29, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX_CPU", 0x15c00000, 0x0854, (0x3f << 0), (0x3 << 0), 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x12860000, 0x1404, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	PMUCAL_NUM_CORES,
};

struct pmucal_cpu pmucal_cpu_list[PMUCAL_NUM_CORES] = {
	[CPU_CORE00] = {
		.id = CPU_CORE00,
		.release = core00_release,
		.on = core00_on,
		.off = core00_off,
		.status = core00_status,
		.num_release = ARRAY_SIZE(core00_release),
		.num_on = ARRAY_SIZE(core00_on),
		.num_off = ARRAY_SIZE(core00_off),
		.num_status = ARRAY_SIZE(core00_status),
	},
	[CPU_CORE01] = {
		.id = CPU_CORE01,
		.release = core01_release,
		.on = core01_on,
		.off = core01_off,
		.status = core01_status,
		.num_release = ARRAY_SIZE(core01_release),
		.num_on = ARRAY_SIZE(core01_on),
		.num_off = ARRAY_SIZE(core01_off),
		.num_status = ARRAY_SIZE(core01_status),
	},
};

unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	PMUCAL_NUM_CLUSTERS,
};

struct pmucal_cpu pmucal_cluster_list[PMUCAL_NUM_CLUSTERS] = {
	[CPU_CLUSTER0] = {
		.id = CPU_CLUSTER0,
		.on = cluster0_on,
		.off = cluster0_off,
		.status = cluster0_status,
		.num_on = ARRAY_SIZE(cluster0_on),
		.num_off = ARRAY_SIZE(cluster0_off),
		.num_status = ARRAY_SIZE(cluster0_status),
	},
};

unsigned int pmucal_cluster_list_size = ARRAY_SIZE(pmucal_cluster_list);

enum pmucal_opsnum {
	NUM_PMUCAL_OPTIONS,
};

struct pmucal_cpu pmucal_pmu_ops_list[NUM_PMUCAL_OPTIONS] = {
};
unsigned int pmucal_option_list_size = ARRAY_SIZE(pmucal_pmu_ops_list);

struct cpu_info cpuinfo[] = {
	[0] = {
		.min = 0,
		.max = 1,
		.total = 2,
	},
};
#endif