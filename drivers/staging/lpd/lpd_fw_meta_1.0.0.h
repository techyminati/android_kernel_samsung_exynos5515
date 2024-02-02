#ifndef _EXYNOS_LPD_METADATA_H_
#define _EXYNOS_LPD_METADATA_H_
/* DO NOT CHANGE as need to sync up with kernel driver !!!!!!!!!!!!!!!!! */

#define _VERSION(major, minor, revision) ((__u32)(((major) << 16) | (minor << 8) | (revision & 0xFF)))
#define LPD_METADATA_API_VERSION		_VERSION(1, 0, 0)

enum lpd_clock_hand {
	LPD_CLOCK_HAND_HOUR 	= 0,
	LPD_CLOCK_HAND_MINUTE,
	LPD_CLOCK_HAND_SECOND,
	LPD_CLOCK_HAND_MAX,
}__attribute__((packed));

enum lpd_rsc_type {
	LPD_RSC_TYPE_BG = 0,
	LPD_RSC_TYPE_CANVAS,
	LPD_RSC_TYPE_HANDS,
	LPD_RSC_TYPE_DIGITAL,
	LPD_RSC_TYPE_FONT1,
	LPD_RSC_TYPE_FONT2,
	LDP_RSC_TYPE_ICON1,
};

enum lpd_img_fmt {
					/*      	|127       112|111        96|...|31         16|15          0| */
	ARGB8888	= 0x0,		/* 5'd0 	|  A3  |  R3  |  G3  |  B3  |...|  A0  |  R0  |  G0  |	B0  | */
	ARGB1555	= 0xc,		/* 5'd12	|A| R6| G6| B6|A| R5| G5| B5|...|A| R1| G1| B1|A| R0| G0| B0| */
	ARGB4444	= 0xd,		/* 5'd13	|A7|R7 |G7 |B7|A6|R6 |G6 |B6|...|A1|R1 |G1 |B1|A0|R0 |G0 |B0| */
};

enum img_buffer_type {
	LPD_RSC_BUF_TYPE_SRAM = 0,
	LPD_RSC_BUF_TYPE_DRAM,
};

#define MAX_RSC_AUX_INFO		8
enum
{
	HANDS_ID_HOUR = 0,
	HANDS_ID_MIN,
	HANDS_ID_SEC,
	HANDS_ID_INVALID = 3,
};

enum
{
	AUX_RSC_HANDS_NUMBER = 0,
	AUX_RSC_HANDS_WIDTH,
	AUX_RSC_HANDS_HEIGHT,
	AUX_RSC_HANDS_ID_0,
	AUX_RSC_HANDS_ID_1,
	AUX_RSC_HANDS_ID_2,
	AUX_RSC_HANDS_PIVOT_X,
	AUX_RSC_HANDS_PIVOT_Y,
	AUX_RSC_HANDS_MAX = 8,
};

enum
{
	AUX_LAYOUT_DIGIT_NOTATION = 0,
	AUX_LAYOUT_DIGIT_RATE,
	AUX_LAYOUT_FLASH_IND,
};

enum
{
	AUX_DIGIT_NOTATION_12 = 0,
	AUX_DIGIT_NOTATION_24 = 1,
};

enum
{
	DIGIT_POS_TENS_HOUR = 0,
	DIGIT_POS_UNITS_HOUR,
	DIGIT_POS_COLON,
	DIGIT_POS_TENS_MIN,
	DIGIT_POS_UNITS_MIN,
	DIGIT_POS_INVALID = 5,
};

/* resource info */
struct lpd_resource_info {
	enum lpd_rsc_type rsc_type;
	unsigned int width;
	unsigned int height;
	enum lpd_img_fmt img_fmt;
	unsigned int aux_info [MAX_RSC_AUX_INFO];
	enum img_buffer_type buf_type;
	unsigned int buf_size;
	unsigned int addr;
};

#define MAX_LAYOUT_AUX_INFO		4
#define MAX_POS_CNT			5

enum lpd_rot_type {
	LPD_ROTATION_0 = 0,
	LPD_ROTATION_90,
	LPD_ROTATION_180,
	LPD_ROTATION_270,
};

struct lpd_pos_info {
	unsigned int x;
	unsigned int y;
};

/* layout info */
struct lpd_layout_info {
	enum lpd_rsc_type rsc_type;
	unsigned int z_order;
	unsigned int visible;
	enum lpd_rot_type rotation;
	unsigned int aux_info[MAX_LAYOUT_AUX_INFO];
	struct lpd_pos_info pos[MAX_POS_CNT];
};

#define MAX_RESOURCE_CNT		5
#define MAX_LAYOUT_CNT			5

enum
{
	LPD_RSC_ID_BG = 0,
	LPD_RSC_ID_CANV  = 1,
};

/* meta info */
typedef struct lpd_meta_info {
   unsigned int lpd_metadata_version; /* meta_ver */
   unsigned int lpd_metadata_crc; 	  /* LSI added */
   unsigned int xres;
   unsigned int yres;
   unsigned int log_level;
   unsigned int lpd_activation;		  /* enable */
   unsigned int resource_cnt;         /* LSI added */
   unsigned int layout_cnt;           /* LSI added */
   struct lpd_resource_info resources[MAX_RESOURCE_CNT];
   struct lpd_layout_info layouts[MAX_LAYOUT_CNT];
} __attribute__((aligned(4),packed))lpd_meta_info;

struct lpd_panel_info {
	unsigned int id;

	unsigned int xres;
    unsigned int yres;
	unsigned int fps;

	unsigned int hs_clk;
	unsigned int p;
	unsigned int m;
	unsigned int s;
	unsigned int k;

	unsigned int cmd_lp_ref;
};

struct lpd_dpu_info {
	struct lpd_panel_info panel;		/* It should be same with panel size */
} __attribute__((aligned(4),packed));
#endif //_EXYNOS_LPD_METADATA_H
