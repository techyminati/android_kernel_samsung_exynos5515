/*
 * linux/drivers/video/fbdev/exynos/panel/s6e36w5x01/s6e36w5x01_a2_s0_panel_dimming.h
 *
 * Header file for S6E36W5X01 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E36W5X01_A2_S0_PANEL_DIMMING_H___
#define __S6E36W5X01_A2_S0_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E36W5X01
 * PANEL : WISE_A3_S0
 */
#define S6E36W5X01_NR_STEP (256)
#define S6E36W5X01_HBM_STEP (1)
#define S6E36W5X01_TOTAL_STEP (S6E36W5X01_NR_STEP + S6E36W5X01_HBM_STEP)

static unsigned int s6e36w5x01_a2_s0_brt_tbl[S6E36W5X01_TOTAL_STEP] = {
	BRT(0), BRT(1), BRT(2), BRT(3), BRT(4), BRT(5), BRT(6), BRT(7), BRT(8), BRT(9), BRT(10),
	BRT(11), BRT(12), BRT(13), BRT(14), BRT(15), BRT(16), BRT(17), BRT(18), BRT(19), BRT(20),
	BRT(21), BRT(22), BRT(23), BRT(24), BRT(25), BRT(26), BRT(27), BRT(28), BRT(29), BRT(30),
	BRT(31), BRT(32), BRT(33), BRT(34), BRT(35), BRT(36), BRT(37), BRT(38), BRT(39), BRT(40),
	BRT(41), BRT(42), BRT(43), BRT(44), BRT(45), BRT(46), BRT(47), BRT(48), BRT(49), BRT(50),
	BRT(51), BRT(52), BRT(53), BRT(54), BRT(55), BRT(56), BRT(57), BRT(58), BRT(59), BRT(60),
	BRT(61), BRT(62), BRT(63), BRT(64), BRT(65), BRT(66), BRT(67), BRT(68), BRT(69), BRT(70),
	BRT(71), BRT(72), BRT(73), BRT(74), BRT(75), BRT(76), BRT(77), BRT(78), BRT(79), BRT(80),
	BRT(81), BRT(82), BRT(83), BRT(84), BRT(85), BRT(86), BRT(87), BRT(88), BRT(89), BRT(90),
	BRT(91), BRT(92), BRT(93), BRT(94), BRT(95), BRT(96), BRT(97), BRT(98), BRT(99), BRT(100),
	BRT(101), BRT(102), BRT(103), BRT(104), BRT(105), BRT(106), BRT(107), BRT(108), BRT(109), BRT(110),
	BRT(111), BRT(112), BRT(113), BRT(114), BRT(115), BRT(116), BRT(117), BRT(118), BRT(119), BRT(120),
	BRT(121), BRT(122), BRT(123), BRT(124), BRT(125), BRT(126), BRT(127), BRT(128), BRT(129), BRT(130),
	BRT(131), BRT(132), BRT(133), BRT(134), BRT(135), BRT(136), BRT(137), BRT(138), BRT(139), BRT(140),
	BRT(141), BRT(142), BRT(143), BRT(144), BRT(145), BRT(146), BRT(147), BRT(148), BRT(149), BRT(150),
	BRT(151), BRT(152), BRT(153), BRT(154), BRT(155), BRT(156), BRT(157), BRT(158), BRT(159), BRT(160),
	BRT(161), BRT(162), BRT(163), BRT(164), BRT(165), BRT(166), BRT(167), BRT(168), BRT(169), BRT(170),
	BRT(171), BRT(172), BRT(173), BRT(174), BRT(175), BRT(176), BRT(177), BRT(178), BRT(179), BRT(180),
	BRT(181), BRT(182), BRT(183), BRT(184), BRT(185), BRT(186), BRT(187), BRT(188), BRT(189), BRT(190),
	BRT(191), BRT(192), BRT(193), BRT(194), BRT(195), BRT(196), BRT(197), BRT(198), BRT(199), BRT(200),
	BRT(201), BRT(202), BRT(203), BRT(204), BRT(205), BRT(206), BRT(207), BRT(208), BRT(209), BRT(210),
	BRT(211), BRT(212), BRT(213), BRT(214), BRT(215), BRT(216), BRT(217), BRT(218), BRT(219), BRT(220),
	BRT(221), BRT(222), BRT(223), BRT(224), BRT(225), BRT(226), BRT(227), BRT(228), BRT(229), BRT(230),
	BRT(231), BRT(232), BRT(233), BRT(234), BRT(235), BRT(236), BRT(237), BRT(238), BRT(239), BRT(240),
	BRT(241), BRT(242), BRT(243), BRT(244), BRT(245), BRT(246), BRT(247), BRT(248), BRT(249), BRT(250),
	BRT(251), BRT(252), BRT(253), BRT(254), BRT(255),
	BRT(256),
};


static unsigned int s6e36w5x01_a2_s0_lum_tbl[S6E36W5X01_TOTAL_NR_LUMINANCE] = {
	/* normal 10x25+6 */
	5, 5, 5, 7, 10, 12, 14, 17, 19, 21,
	24, 26, 28, 30, 33, 35, 37, 40, 42, 44,
	47, 49, 51, 54, 56, 58, 61, 63, 65, 68,
	70, 72, 75, 77, 79, 81, 84, 86, 88, 91,
	93, 95, 98, 100, 102, 105, 107, 109, 112, 114,
	117, 119, 121, 124, 126, 128, 131, 133, 135, 138,
	140, 142, 145, 147, 150, 152, 154, 157, 159, 161,
	164, 166, 168, 171, 173, 175, 178, 180, 183, 185,
	187, 190, 192, 194, 197, 199, 201, 204, 206, 208,
	211, 213, 216, 218, 220, 223, 225, 227, 230, 232,
	234, 237, 239, 242, 244, 246, 249, 251, 253, 256,
	258, 260, 263, 265, 267, 270, 272, 275, 277, 279,
	282, 284, 286, 289, 291, 293, 296, 298, 300, 303,
	305, 308, 310, 312, 315, 317, 319, 322, 324, 326,
	329, 331, 333, 336, 338, 341, 343, 345, 348, 350,
	352, 355, 357, 359, 362, 364, 367, 369, 371, 374,
	376, 378, 381, 383, 385, 388, 390, 392, 395, 397,
	400, 402, 404, 407, 409, 411, 414, 416, 418, 421,
	423, 425, 428, 430, 433, 435, 437, 440, 442, 444,
	447, 449, 451, 454, 456, 458, 461, 463, 466, 468,
	470, 473, 475, 477, 480, 482, 484, 487, 489, 492,
	494, 496, 499, 501, 503, 506, 508, 510, 513, 515,
	517, 520, 522, 525, 527, 529, 532, 534, 536, 539,
	541, 543, 546, 548, 550, 553, 555, 558, 560, 562,
	565, 567, 569, 572, 574, 576, 579, 581, 583, 586,
	588, 591, 593, 595, 598, 600,
	/* hbm 1 */
	1000
};

static unsigned int s6e36w5x01_a2_s0_step_cnt_tbl[S6E36W5X01_TOTAL_STEP] = {
	[0 ... S6E36W5X01_TOTAL_STEP - 1] = 1,
};

struct brightness_table s6e36w5x01_a2_s0_panel_brightness_table = {
	.control_type = BRIGHTNESS_CONTROL_TYPE_GAMMA_MODE2,
	.brt = s6e36w5x01_a2_s0_brt_tbl,
	.sz_brt = ARRAY_SIZE(s6e36w5x01_a2_s0_brt_tbl),
	.sz_ui_brt = S6E36W5X01_NR_STEP,
	.sz_hbm_brt = S6E36W5X01_HBM_STEP,
	.lum = s6e36w5x01_a2_s0_lum_tbl,
	.sz_lum = S6E36W5X01_TOTAL_NR_LUMINANCE,
	.sz_ui_lum = S6E36W5X01_NR_LUMINANCE,
	.sz_hbm_lum = S6E36W5X01_NR_HBM_LUMINANCE,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = NULL,
	.sz_brt_to_step = 0,
	.step_cnt = s6e36w5x01_a2_s0_step_cnt_tbl,
	.sz_step_cnt = ARRAY_SIZE(s6e36w5x01_a2_s0_step_cnt_tbl),
	.vtotal = 3232,
};

static struct panel_dimming_info s6e36w5x01_a2_s0_panel_dimming_info = {
	.name = "s6e36w5x01_a2_s0",
	.dim_init_info = {
		NULL,
	},
	.target_luminance = -1,
	.nr_luminance = 0,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e36w5x01_a2_s0_panel_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = NULL,
	.dim_flash_on = false,
	.hbm_aor = NULL,
};

/*
 * PANEL INFORMATION
 * LDI : S6E36W5X01
 * PANEL : W5_WISE_A3_S0
 */
#define S6E36W5X01_W5_NR_STEP (256)
#define S6E36W5X01_W5_HBM_STEP (45)
#define S6E36W5X01_W5_TOTAL_STEP (S6E36W5X01_W5_NR_STEP + S6E36W5X01_W5_HBM_STEP)

static unsigned int s6e36w5x01_w5_a2_s0_brt_tbl[S6E36W5X01_W5_TOTAL_STEP] = {
	BRT(0), BRT(1), BRT(2), BRT(3), BRT(4), BRT(5), BRT(6), BRT(7), BRT(8), BRT(9), BRT(10),
	BRT(11), BRT(12), BRT(13), BRT(14), BRT(15), BRT(16), BRT(17), BRT(18), BRT(19), BRT(20),
	BRT(21), BRT(22), BRT(23), BRT(24), BRT(25), BRT(26), BRT(27), BRT(28), BRT(29), BRT(30),
	BRT(31), BRT(32), BRT(33), BRT(34), BRT(35), BRT(36), BRT(37), BRT(38), BRT(39), BRT(40),
	BRT(41), BRT(42), BRT(43), BRT(44), BRT(45), BRT(46), BRT(47), BRT(48), BRT(49), BRT(50),
	BRT(51), BRT(52), BRT(53), BRT(54), BRT(55), BRT(56), BRT(57), BRT(58), BRT(59), BRT(60),
	BRT(61), BRT(62), BRT(63), BRT(64), BRT(65), BRT(66), BRT(67), BRT(68), BRT(69), BRT(70),
	BRT(71), BRT(72), BRT(73), BRT(74), BRT(75), BRT(76), BRT(77), BRT(78), BRT(79), BRT(80),
	BRT(81), BRT(82), BRT(83), BRT(84), BRT(85), BRT(86), BRT(87), BRT(88), BRT(89), BRT(90),
	BRT(91), BRT(92), BRT(93), BRT(94), BRT(95), BRT(96), BRT(97), BRT(98), BRT(99), BRT(100),
	BRT(101), BRT(102), BRT(103), BRT(104), BRT(105), BRT(106), BRT(107), BRT(108), BRT(109), BRT(110),
	BRT(111), BRT(112), BRT(113), BRT(114), BRT(115), BRT(116), BRT(117), BRT(118), BRT(119), BRT(120),
	BRT(121), BRT(122), BRT(123), BRT(124), BRT(125), BRT(126), BRT(127), BRT(128), BRT(129), BRT(130),
	BRT(131), BRT(132), BRT(133), BRT(134), BRT(135), BRT(136), BRT(137), BRT(138), BRT(139), BRT(140),
	BRT(141), BRT(142), BRT(143), BRT(144), BRT(145), BRT(146), BRT(147), BRT(148), BRT(149), BRT(150),
	BRT(151), BRT(152), BRT(153), BRT(154), BRT(155), BRT(156), BRT(157), BRT(158), BRT(159), BRT(160),
	BRT(161), BRT(162), BRT(163), BRT(164), BRT(165), BRT(166), BRT(167), BRT(168), BRT(169), BRT(170),
	BRT(171), BRT(172), BRT(173), BRT(174), BRT(175), BRT(176), BRT(177), BRT(178), BRT(179), BRT(180),
	BRT(181), BRT(182), BRT(183), BRT(184), BRT(185), BRT(186), BRT(187), BRT(188), BRT(189), BRT(190),
	BRT(191), BRT(192), BRT(193), BRT(194), BRT(195), BRT(196), BRT(197), BRT(198), BRT(199), BRT(200),
	BRT(201), BRT(202), BRT(203), BRT(204), BRT(205), BRT(206), BRT(207), BRT(208), BRT(209), BRT(210),
	BRT(211), BRT(212), BRT(213), BRT(214), BRT(215), BRT(216), BRT(217), BRT(218), BRT(219), BRT(220),
	BRT(221), BRT(222), BRT(223), BRT(224), BRT(225), BRT(226), BRT(227), BRT(228), BRT(229), BRT(230),
	BRT(231), BRT(232), BRT(233), BRT(234), BRT(235), BRT(236), BRT(237), BRT(238), BRT(239), BRT(240),
	BRT(241), BRT(242), BRT(243), BRT(244), BRT(245), BRT(246), BRT(247), BRT(248), BRT(249), BRT(250),
	BRT(251), BRT(252), BRT(253), BRT(254), BRT(255),
	BRT(256), BRT(257), BRT(258), BRT(259), BRT(260), BRT(261), BRT(262), BRT(263), BRT(264), BRT(265),
	BRT(266), BRT(267), BRT(268), BRT(269), BRT(270), BRT(271), BRT(272), BRT(273), BRT(274), BRT(275),
	BRT(276), BRT(277), BRT(278), BRT(279), BRT(280), BRT(281), BRT(282), BRT(283), BRT(284), BRT(285),
	BRT(286), BRT(287), BRT(288), BRT(289), BRT(290), BRT(291), BRT(292), BRT(293), BRT(294), BRT(295),
	BRT(296), BRT(297), BRT(298), BRT(299), BRT(300)
};


static unsigned int s6e36w5x01_w5_a2_s0_lum_tbl[S6E36W5X01_W5_TOTAL_NR_LUMINANCE] = {
	/* normal 10x25+6 */
	5, 5, 5, 7, 10, 12, 14, 17, 19, 21,
	24, 26, 28, 30, 33, 35, 37, 40, 42, 44,
	47, 49, 51, 54, 56, 58, 61, 63, 65, 68,
	70, 72, 75, 77, 79, 81, 84, 86, 88, 91,
	93, 95, 98, 100, 102, 105, 107, 109, 112, 114,
	117, 119, 121, 124, 126, 128, 131, 133, 135, 138,
	140, 142, 145, 147, 150, 152, 154, 157, 159, 161,
	164, 166, 168, 171, 173, 175, 178, 180, 183, 185,
	187, 190, 192, 194, 197, 199, 201, 204, 206, 208,
	211, 213, 216, 218, 220, 223, 225, 227, 230, 232,
	234, 237, 239, 242, 244, 246, 249, 251, 253, 256,
	258, 260, 263, 265, 267, 270, 272, 275, 277, 279,
	282, 284, 286, 289, 291, 293, 296, 298, 300, 303,
	305, 308, 310, 312, 315, 317, 319, 322, 324, 326,
	329, 331, 333, 336, 338, 341, 343, 345, 348, 350,
	352, 355, 357, 359, 362, 364, 367, 369, 371, 374,
	376, 378, 381, 383, 385, 388, 390, 392, 395, 397,
	400, 402, 404, 407, 409, 411, 414, 416, 418, 421,
	423, 425, 428, 430, 433, 435, 437, 440, 442, 444,
	447, 449, 451, 454, 456, 458, 461, 463, 466, 468,
	470, 473, 475, 477, 480, 482, 484, 487, 489, 492,
	494, 496, 499, 501, 503, 506, 508, 510, 513, 515,
	517, 520, 522, 525, 527, 529, 532, 534, 536, 539,
	541, 543, 546, 548, 550, 553, 555, 558, 560, 562,
	565, 567, 569, 572, 574, 576, 579, 581, 583, 586,
	588, 591, 593, 595, 598, 600,
	/* hbm 45 */
	657, 665, 673, 679, 688, 694, 701, 797, 713, 723,
	729, 736, 744, 749, 759, 765, 772, 782, 788, 794,
	802, 809, 819, 826, 833, 842, 848, 854, 865, 873,
	881, 890, 897, 904, 912, 920, 930, 938, 945, 953,
	960, 967, 978, 987, 1000,
};

static unsigned int s6e36w5x01_w5_a2_s0_step_cnt_tbl[S6E36W5X01_W5_TOTAL_STEP] = {
	[0 ... S6E36W5X01_W5_TOTAL_STEP - 1] = 1,
};

struct brightness_table s6e36w5x01_w5_a2_s0_panel_brightness_table = {
	.control_type = BRIGHTNESS_CONTROL_TYPE_GAMMA_MODE2,
	.brt = s6e36w5x01_w5_a2_s0_brt_tbl,
	.sz_brt = ARRAY_SIZE(s6e36w5x01_w5_a2_s0_brt_tbl),
	.sz_ui_brt = S6E36W5X01_W5_NR_STEP,
	.sz_hbm_brt = S6E36W5X01_W5_HBM_STEP,
	.lum = s6e36w5x01_w5_a2_s0_lum_tbl,
	.sz_lum = S6E36W5X01_W5_TOTAL_NR_LUMINANCE,
	.sz_ui_lum = S6E36W5X01_W5_NR_LUMINANCE,
	.sz_hbm_lum = S6E36W5X01_W5_NR_HBM_LUMINANCE,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = NULL,
	.sz_brt_to_step = 0,
	.step_cnt = s6e36w5x01_w5_a2_s0_step_cnt_tbl,
	.sz_step_cnt = ARRAY_SIZE(s6e36w5x01_w5_a2_s0_step_cnt_tbl),
	.vtotal = 3232,
};

static struct panel_dimming_info s6e36w5x01_w5_a2_s0_panel_dimming_info = {
	.name = "s6e36w5x01_w5_a2_s0",
	.dim_init_info = {
		NULL,
	},
	.target_luminance = -1,
	.nr_luminance = 0,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e36w5x01_w5_a2_s0_panel_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = NULL,
	.dim_flash_on = false,
	.hbm_aor = NULL,
};

#endif /* __S6E36W5X01_A2_S0_PANEL_DIMMING_H___ */
