/*
 * drivers/media/video/omap/gci/pseudoi2c.h
 *
 * Header file for I2C slave driver for TI's OMAP3430 Camera ISP
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
 
#ifndef PSEUDOI2C_H
#define PSEUDOI2C_H

void pseudoi2c_exit(void);
int pseudoi2c_init(u8 i2c_num);

void p_i2c_set_reglength(unsigned int reg_length);
void p_i2c_set_datalength(unsigned int data_length);

int p_i2c_write_slave(short slave_id, u8 *value,int num_bytes, u8 data_width);
int p_i2c_read_slave(short slave_id, u8 *value, u8 num_bytes, u8 data_width);

int p_i2c_attach_slave(u8 slaveid);
int p_i2c_detach_slave(u8 slaveid);
#endif 	/* ifndef PSEUDOI2C_H */
