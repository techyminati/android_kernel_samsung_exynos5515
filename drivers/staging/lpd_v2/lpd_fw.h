#ifndef ___SAMSUNG_LPD_FW_H__
#define ___SAMSUNG_LPD_FW_H__
/* DO NOT CHANGE as need to sync up with kernel driver !!!!!!!!!!!!!!!!! */

#ifndef _VERSION
#define _VERSION(major, minor, revision) \
				((__u32)(((major) << 16) | (minor << 8) | (revision & 0xFF)))
#endif
#define LPD_FW_API_VERSION		_VERSION(2, 0, 1)

/* Default mem info */
#define LPD_FW_DRAM_AP_BASE			0x9A000000
#define LPD_FW_DRAM_FW_BASE			0xDA000000
#define LPD_FW_DRAM_SIZE			0x00600000
#define LPD_FW_DRAM_AP_TO_FW(addr)	(LPD_FW_DRAM_FW_BASE+addr-LPD_FW_DRAM_AP_BASE)
#define LPD_FW_DRAM_FW_TO_AP(addr)	(LPD_FW_DRAM_AP_BASE+addr-LPD_FW_DRAM_FW_BASE)

#define LPD_FW_SRAM_AP_BASE			0x10E00000
#define LPD_FW_SRAM_FW_BASE			0x00100000
#define LPD_FW_SRAM_AP_TO_FW(addr)	(LPD_FW_SRAM_FW_BASE+addr-LPD_FW_SRAM_AP_BASE)
#define LPD_FW_SRAM_FW_TO_AP(addr)	(LPD_FW_SRAM_AP_BASE+addr-LPD_FW_SRAM_FW_BASE)

#define LPD_FW_DPU_AP_BASE			0x10000000
#define LPD_FW_DPU_FW_BASE			0x50000000
#define LPD_FW_DPU_AP_TO_FW(addr)	(LPD_FW_DPU_FW_BASE+addr-LPD_FW_DPU_AP_BASE)
#define LPD_FW_DPU_FW_TO_AP(addr)	(LPD_FW_DPU_AP_BASE+addr-LPD_FW_DPU_FW_BASE)

#define LPD_FW_SRAM_INFO_OFFSET 	0x200

#define LPD_FW_MAGIC_SIZE 16

// DUMP
#define LPD_FW_DUMP_MAGIC "LPD FW DUMP"

#define DUMP_SIZE (90)
#define DUMP_NUM (250)

struct lpd_fw_dump {
	char magic[LPD_FW_MAGIC_SIZE];
	__u32 lines;
	struct lpd_fw_dump_content {
		char buf[DUMP_SIZE];
		__u16 size;
	} log[DUMP_NUM];
};

// LOGBUF
#define LPD_FW_LOGBUF_MAGIC "LPD FW LOGBUF"

#define LOGBUF_SIZE (80) /* expected buffer size of one lpd_fw_logbuf_content */
#define LPD_LOGBUF_NUM (79)
#define LOGBUF_BUF_SIZE \
	(LOGBUF_SIZE - sizeof(__u64) - sizeof(__u32) - sizeof(__u16) - 3 * sizeof(__u8))

struct lpd_fw_logbuf {
	struct lpd_fw_logbuf_header {
		char magic[LPD_FW_MAGIC_SIZE];
		__u32 seq_num;
		__u32 eq; 		/* write owner chub (index_writer) */
		__u32 dq; 		/* read onwer ap (index_reader) */
		__u8 full;
		__u8 loglevel;	/*0: E, 1: W, 2: I, 3: D*/
		__u8 regdump;	/*0: nothing, 1: received register dump */
		__u8 apstate;	/*0: AP alive, 1: AP sleep */
		/* for debug */
		__u32 full_cnt;
		__u32 fw_num;
		__u32 lpd_fault_status; /* fault status for cm55*/
	} header;

	struct lpd_fw_logbuf_content {
		char buf[LOGBUF_BUF_SIZE];
		__u64 timestamp;
		__u32 nextaddr;
		__u16 size;		/* save current buf size & save fw_num at the end for debug*/
		__u8 lines;
		__u8 level;
	} log[LPD_LOGBUF_NUM];
};

// IPC
#define LPD_FW_IPC_MAGIC "LPD FW IPC"
enum lpd_ipc_command_dir {
	IPC_COMMAND_DIR_FROM, 	// from external
	IPC_COMMAND_DIR_TO, 	// to external
	IPC_COMMAND_DIR_MAX
}__attribute__((packed));

enum lpd_ipc_command {
	IPC_COMMAND_INVALID	 = 1,	// CM55 <-> CM4
	IPC_COMMAND_BOOT_DONE,		// CM55 -> CM4
	IPC_COMMAND_START_DRAW, 	// CM4 -> CM55
	IPC_COMMAND_START_DRAW_ACK, // CM55 -> CM4
	IPC_COMMAND_STOP_DRAW, 		// CM4 -> CM55
	IPC_COMMAND_STOP_DRAW_ACK, 	// CM55 -> CM4
	IPC_COMMAND_END_DRAW, 		// CM55 -> CM4
	IPC_COMMAND_MAX
}__attribute__((packed));

struct lpd_fw_ipc_data{
	union lpd_fw_ipc_time {
		struct {
			__u64 sec  : 6;		// 0-61
			__u64 min  : 6;		// 0-59
			__u64 hour : 5;		// 0-23
			__u64 mday : 5;		// 1-31
			__u64 mon  : 4;		// 0-11
			__u64 year : 8;		// since 1900
			__u64 wday : 3;		// 0-6
			__u64 yday : 9;		// 0-365
		};
		__u64 time64;
	} time;
	__u8 res[9];
};

struct lpd_fw_ipc{
	char magic[LPD_FW_MAGIC_SIZE];
	__u32 cmd[IPC_COMMAND_DIR_MAX];
	__u8 busy;
	struct lpd_fw_ipc_data data;
} __attribute__((aligned(4)));

struct lpd_fw_interface{
	struct lpd_fw_ipc ipc;
	struct lpd_fw_logbuf logbuf;
};

// INFO & MAP
#define LPD_FW_MAGIC "LPD Firmware"

struct lpd_fw_memory_map {
	struct lpd_fw_dram_map {
		__u32	base;
		__u32	size;
	} rmem; // reserved DRAM
	struct lpd_fw_sram_map {
		__u32	code_start;
		__u32	code_end;
		__u32	ipc_start;
		__u32	ipc_end;
		__u32	logbuf_start;
		__u32	logbuf_end;
		__u32	ram_start;
		__u32	ram_end;
		__u32	desc_start;
		__u32	desc_end;
		__u32	img_start;
		__u32	img_end;
		__u32	dump_start;
		__u32	dump_end;
	} sram;
};

struct lpd_fw_info {
	char	magic[LPD_FW_MAGIC_SIZE];
	__u8	soc[8];
	__u32	version;
	__u32	meta_ver;
	__u8	time[21];
	__u8	name[19];
	__u8	commit[200];
	struct lpd_fw_memory_map map;
};

// Image Descriptor
#include "lpd_fw_meta_1.1.2.h"
struct lpd_fw_img_desc {
	struct lpd_sensor_data sensor;		/* sensor data */
	struct lpd_meta_info meta;
	struct lpd_dpu_info dpu;
};

#define TOTAL_STRUCT_SIZE (sizeof(struct lpd_fw_img_desc) + sizeof(struct lpd_fw_info)+ \
				sizeof(struct lpd_fw_interface) + sizeof(struct lpd_fw_dump))

#endif
