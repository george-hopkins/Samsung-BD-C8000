/*
 * omap2_mcspi_seclcdtest.h
 *
 * Test Secondary LCD using McSPI driver
 *
 * Copyright (C) 2006 Texas Instruments, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */
	     

/*=================== MACROS=================================================*/

#define SET_COLUMN_ADDR                   0x15
#define SET_PAGE_ADDR                     0x75
#define SET_COM_OUTPUT_SCAN_DIR           0xBB
#define SET_DATA_OUTPUT_SCAN_DIR          0xBC
#define SET_COLOUR_LOOKUP_TABLE           0xCE
#define SET_DISPLAY_CONTROL               0xCA /*Select driver duty from 1/8 to 1/68*/
#define SET_AREA_SCROLL                   0xAA
#define SET_SCROLL_START                  0xAB
#define SET_POWER_CONTROL_REG             0x20
#define SET_CONTRAST_LVL_AND_RESI_RATIO   0x81
#define INCREMENT_CONTRAST_SET            0xD6
#define DECREMENT_CONTRAST_SET            0xD7
#define SET_NORMAL_DISPLAY                0xA6
#define SET_INVERSE_DISPLAY               0xA7
#define ENTER_PARTIAL_DISPLAY             0xA8
#define EXIT_PARTIAL_DISPLAY              0xA9
#define SET_DISPLAY_OFF                   0xAE
#define SET_DISPLAY_ON                    0xAF
#define ENTER_SLEEP_MODE                  0x95
#define EXIT_SLEEP_MODE                   0x94
#define ENABLE_INT_OSCILLATOR             0xD1
#define DISABLE_INT_OSCILLATOR            0xD2 /*NEED TO CHECK*/
#define SET_TEMP_COMPN_COEFFICIENT        0x82 
#define N0_OPERATION                      0x25
#define WRITE_DISPLAY_DATA                0x5C

/*GRAPHIC COMMANDS*/
#define DRAW_LINE                         0x83
#define FILL_ENABLE_DISABLE               0x92 /* send 1 EN, 0 for DIS as data*/
#define DRAW_RECTANGLE                    0x84
#define COPY                              0x8A
#define DIM_WINDOW                        0x8C
#define CLEAR_WINDOW                      0x8E

/*EXTENDED COMMANDS*/
#define SET_BIAS_RATIO                    0xFB
#define SET_FRAME_FRQ_NLINE_INVER         0xF2
#define SELECT_PWD_OR_FRC                 0xF7


/*FRAME FREQUENCY */
#define FRAME_FREQ_111MHZ     0xF
#define FRAME_FREQ_103_5MHZ   0xE
#define FRAME_FREQ_98MHZ      0xD
#define FRAME_FREQ_93MHZ      0xC
#define FRAME_FREQ_89MHZ      0xB
#define FRAME_FREQ_85MHZ      0xA
#define FRAME_FREQ_81_5MHZ    0x9
#define FRAME_FREQ_78MHZ      0x8
#define FRAME_FREQ_76_5MHZ    0x7
#define FRAME_FREQ_73_5MHZ    0x6
#define FRAME_FREQ_71MHZ      0x5
#define FRAME_FREQ_68MHZ      0x4
#define FRAME_FREQ_66_5MHZ    0x3
#define FRAME_FREQ_64_5MHZ    0x2
#define FRAME_FREQ_62_5MHZ    0x1
#define FRAME_FREQ_60_5MHZ    0x0

/*Booster Level For SPI LCD*/
#define BOOST_LEVEL_3X   0x0
#define BOOST_LEVEL_4X   0x4
#define BOOST_LEVEL_5X   0x8
#define BOOST_LEVEL_6X   0xC

/* COM output scan Direction*/
#define COM_SCAN_DIR1    0x0 /* direction is COM0->COM33 COM34->COM67*/
#define COM_SCAN_DIR2    0x1 /* direction is COM0->COM33 COM67<-COM34*/
#define COM_SCAN_DIR3    0x2 /* direction is COM33<-COM0 COM34->COM67*/
#define COM_SCAN_DIR4    0x3 /* direction is COM33<-COM0 COM67<-COM34*/

/* DATA output scan Direction*/
#define PAGE_ADDR_NORMAL       0x0 
#define PAGE_ADDR_INVERSE      0x1
#define COLUMN_ADDR_NORMAL     0x0 
#define COLUMN_ADDR_INVERSE    0x2
#define SCAN_DIR_TO_COLUMN     0x0
#define SCAN_DIR_TO_PAGE       0x4

#define COLUMN_SCAN_CN_PN    ( SCAN_DIR_TO_COLUMN | PAGE_ADDR_NORMAL | COLUMN_ADDR_NORMAL)
#define COLUMN_SCAN_CN_PI    ( SCAN_DIR_TO_COLUMN | PAGE_ADDR_INVERSE | COLUMN_ADDR_NORMAL) 
#define COLUMN_SCAN_CI_PN    ( SCAN_DIR_TO_COLUMN | PAGE_ADDR_NORMAL | COLUMN_ADDR_INVERSE)
#define COLUMN_SCAN_CI_PI    ( SCAN_DIR_TO_COLUMN | PAGE_ADDR_INVERSE | COLUMN_ADDR_INVERSE)
#define PAGE_SCAN_CN_PN      ( SCAN_DIR_TO_PAGE | PAGE_ADDR_NORMAL | COLUMN_ADDR_NORMAL)
#define PAGE_SCAN_CN_PI      ( SCAN_DIR_TO_PAGE | PAGE_ADDR_INVERSE | COLUMN_ADDR_NORMAL) 
#define PAGE_SCAN_CI_PN      ( SCAN_DIR_TO_PAGE | PAGE_ADDR_NORMAL | COLUMN_ADDR_INVERSE)
#define PAGE_SCAN_CI_PI      ( SCAN_DIR_TO_PAGE | PAGE_ADDR_INVERSE | COLUMN_ADDR_INVERSE)

/* DISPLAY MODES*/
#define DISPLAY_NORMAL       0x0 
#define DISPLAY_INVERSE      0x1

/* GRAY SCALE MODE*/
#define GRAY_SCALE_8_BIT_MODE       0x1
#define GRAY_SCALE_16_BIT_MODE      0x2

#define SSD1788_MAX_WIDTH       0x31
#define SSD1788_MAX_HEIGHT      0x22


/*Contrast Levels*/
#define CONTRAST_LEVEL_DEFAULT      0x4f
#define CONTRAST_LEVEL1             0x1
#define CONTRAST_LEVEL2             0x2
#define CONTRAST_LEVEL3             0x4
#define CONTRAST_LEVEL4             0x8
#define CONTRAST_LEVEL5             0xC
#define CONTRAST_LEVEL6             0x10
#define CONTRAST_LEVEL7             0x20

#define	 PADCONF_McSPI1_CLK          0x480021C8
#define  PADCONF_McSPI1_SIMO         0x480021CA
#define  PADCONF_McSPI1_SOMI         0x480021CC
#define  PADCONF_McSPI1_CS0          0x480021CE
#define  PADCONF_McSPI1_CS1          0x480021D0
#define  PADCONF_McSPI1_CS2          0x480021D2
#define  PADCONF_McSPI1_CS3          0x480021D4
#define  PADCONF_etk_clk	     0x48002A28
#define  PADCONF_sysboot5	     0x48002A14

