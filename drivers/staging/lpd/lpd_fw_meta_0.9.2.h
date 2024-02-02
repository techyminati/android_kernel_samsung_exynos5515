#ifndef ___SAMSUNG_LPD_METADATA_H__
#define ___SAMSUNG_LPD_METADATA_H__
/* DO NOT CHANGE as need to sync up with kernel driver !!!!!!!!!!!!!!!!! */

#define _VERSION(major, minor, revision) ((__u32)(((major) << 16) | (minor << 8) | (revision & 0xFF)))
#define LPD_METADATA_API_VERSION		_VERSION(0, 9, 2)

#define HAND_COUNT				(2)
#define ICON_COUNT				(3)
#define DIGIT_DATA_SET				(6)
#define FONT_IMAGE_SET				(10)
#define TOTAL_IMAGE_DATA_SET 		((COLCK_DATA_SET) + (ICON_IMAGE_SET))

/* ¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é BASIC DATA STRUCTURE ¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é¡é*/
enum lpd_image_format {
					/* Little endian */
					/*      			|127       112|111        96|...|31         16|15          0| */
	ARGB8888	= 0x0,		/* 5'd0 	|  A3  |  R3  |  G3  |  B3  |...|  A0  |  R0  |  G0  |	B0  | */
	ARGB1555	= 0xc,		/* 5'd12	|A| R6| G6| B6|A| R5| G5| B5|...|A| R1| G1| B1|A| R0| G0| B0| */
	ARGB4444	= 0xd,		/* 5'd13	|A7|R7 |G7 |B7|A6|R6 |G6 |B6|...|A1|R1 |G1 |B1|A0|R0 |G0 |B0| */
};

enum lpd_image_attribute {
	LPD_LOCATE 		= (0x1 << 0),
	LPD_ROTATE 		= (0x1 << 1),
	LPD_SELECT	 	= (0x1 << 2),
};

struct lpd_dimension {
	unsigned int width;		/* in pixels */
	unsigned int height; 	/* in pixels */
};

struct lpd_coordinate {
	unsigned int x;			/* It should be represented the distance from (0,0) of panel in pixels */
	unsigned int y;			/* It should be represented the distance from (0,0) of panel in pixels */
};

struct lpd_rotation_info {
	// Settable values
	unsigned int rotation_factor; 		/* This is the value that will be multiplied by rotation_unit_angle. */
	// Fixed vlaues
	unsigned int rotation_unit_angle; 	/* To be rotated by unit angle, when rotation_factor is ticking. The values are normalized to 10000 */
};

#define LPD_IMG_DESCRIPTION_SIZE	30
#define LPD_IMG_AVAILABLE_COLOR_CNT	10
struct lpd_image {
	// Settable values
	bool draw_enable;
	unsigned int lpd_imgage_buffer;

	// Fixed vlaues
	unsigned int z_order;								/* It should be zero, if it is backgound image */
	unsigned int lpd_buffer_info_index;					/* It is buffer index of "struct lpd_buffer_info img_info[TOTAL_IMAGE_SET];" */
	unsigned char lpd_image_description[LPD_IMG_DESCRIPTION_SIZE]; 	/* identifier ex) "watch face background" */
	enum lpd_image_attribute 	image_attr;							/* It is composed by bit wise of enum lpd_image_attribute */
	enum lpd_image_format		img_format;

	// For Location
	struct lpd_dimension		dimension;				/* It is image size */
	struct lpd_coordinate		placement; 				/* This is the position of the image that will be placed over the background image. */

	// For Selection
	unsigned int *lpd_font_buf_array;					/* It is same width "unsigned int *lpd_imgage_buffer;"" */
	unsigned int lpd_font_value;						/* It is buffer index of "struct lpd_buffer_info font_info[TOTAL_FONT_SET][10];" */

	// For Rotation
	struct lpd_coordinate		rotation_coordinate;	/* It is used for rotation each clock hand */
	struct lpd_rotation_info	hand_info;
};

struct lpd_data {
	__u32 data_index;
	/* watch time value : sensor_data => [0], hh => [0], mm => [1], ss => [2]*/
	__u32 data_value[3];
};

struct clock_hands {
	struct lpd_data 		data;
	struct lpd_image 		hour;
	struct lpd_image 		minute;
	struct lpd_image 		second; 	/* Normally this value is not used */
};

struct digit {
	struct lpd_data 		data;
	struct lpd_image 		digit;
};

struct lpd_buffer_info {
	__u32 buffer_address;
	__u32 buffer_size;
};

/* ¡é¡é¡é¡é¡é Top layer DATA STRUCTURE ¡é¡é¡é¡é¡é */
struct lpd_image_info {
	__u32 lpd_metadata_producer;		/* Application or producer name, ex) "watch face" or "health" */
	__u32 img_info_cnt;					/* TOTAL_IMAGE_DATA_SET */ 
	__u32 digit_info_cnt;				/* DIGIT_DATA_SET */

	struct lpd_buffer_info image_info[HAND_COUNT + ICON_COUNT];
	struct lpd_buffer_info digits_info[DIGIT_DATA_SET][FONT_IMAGE_SET];
};

typedef struct lpd_metadata {
	__u32 lpd_metadata_version;			/* This should contain API version information.	LPD_METADATA_API_VERSION */
	__u32 lpd_metadata_producer;		/* Application or producer name, ex) "watch face", "health" or "Other" */
	bool lpd_activation;
	bool dram_eanble;					/* This is a factor that will differentiate the case of using DRAM or SRAM only. */

	struct clock_hands			hands[HAND_COUNT];
	struct lpd_image			icon[ICON_COUNT];
	struct digit				digits[DIGIT_DATA_SET];

	__u32 lpd_metadata_crc;				/* 32bit CRC For image data validation */
} __attribute__((aligned(4),packed))lpd_metadata; /* It is for 4type aligned, It can be changed by architecture */

/* This is for display HW */
struct lpd_dpu_info {
	unsigned long long int frame_counter;
	__u32 lpd_dma_address[2]; 					/* lpd_dma_address[0] is layer0 for dma0/dpp0/window1 */
												/* lpd_dma_address[1] is layer1 for dma1/dpp1/window0 */
	struct lpd_dimension		dimension;		/* It should be same with panel size */
	struct lpd_coordinate		placement; 		/* IT should be always (0,0) of panel */
};
#endif //_EXYNOS_LPD_METADATA_H
