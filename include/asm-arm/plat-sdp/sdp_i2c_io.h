/*********************************************************************************************
 *
 *	sdp_i2c_io.h (Samsung Soc i2c device driver ioctl arguement)
 *
 *	author : tukho.kim@samsung.com
 *	
 ********************************************************************************************/
/*********************************************************************************************
 * Description 
// Dec,23,2008 	tukho.kim	increase data size for one packet to 2MB
// Oct,08,2009  tukho.kim	add scl monitoring on/off function "1.2"
// Oct,13,2009  tukho.kim	add byte delay function "1.21"
// Oct,14,2009  tukho.kim	add non-stop write and combined read command "1.30"
// Nov,01,2009  tukho.kim	remove "1.30", add function exp ddc read sequence "1.22"
 ********************************************************************************************/

#ifndef __SDP_I2C_IO_H
#define __SDP_I2C_IO_H

#define I2C_CMD_WRITE		0x01
#define I2C_CMD_READ		0x02

#define I2C_CMD_COMBINED_READ	0x11
#define I2C_CMD_WRITE_N_READ	0x12

/* DDC EDID READ */
#define I2C_CMD_EDID_READ	0x40	// 1.22

/* define for debugging -> not check acknowledge*/
#define M_DEBUG			reserve[0]
#define DEBUG_ALWAYS_NOT_ACK 	(1 << 0)  // 1.2
#define MODE_SCL_MON_OFF	(1 << 1)  // 1.2
#define BYTE_DELAY_ON		(1 << 2)  // 1.21
#define BYTE_DELAY_10US(a)	(a << 4)  // 1.21

/* DDC EDID READ */
#define I2C_EDID_SEG_DEV	0x60    // 1.22, EDID read spec
#define I2C_EDID_SEGMENT	reserve[1]    // 1.22, EDID read spec
/* DDC EDID READ END */



struct sdp_i2c_packet_t {
	unsigned char	slaveAddr;
	unsigned char	subAddrSize;
	unsigned short	udelay;
	unsigned short 	speedKhz;
// Dec,23,2008 	tukho.kim	increase data size for one packet to 2MB
// unsigned short -> unsigned int 
	unsigned int	dataSize;
	unsigned char 	*pSubAddr;
	unsigned char	*pDataBuffer;
	unsigned char	reserve[4];
};


#ifdef __KERNEL__
extern int 
sdp_i2c_request(int port, unsigned int cmd, struct sdp_i2c_packet_t * pKernelPacket);
#endif

#endif // __SDP_I2C_IO_H

#if 0
// 1.3
#define I2C_CMD_COMB_W1_COM_R	0x14
#define I2C_CMD_COMB_W2_COM_R	0x15
#define I2C_CMD_COMB_W3_COM_R	0x16

// 1.3 : device address is not same, when write packet 
#define I2C_CMD_COMB_DW1_COM_R	0x18  
#define I2C_CMD_COMB_DW2_COM_R	0x19
#define I2C_CMD_COMB_DW		(1 << 4)

// 1.3
#define COMB_W1		reserve[1]
#define COMB_W2		reserve[2]
#define COMB_W3		reserve[3]
#define COMB_DW_DEV	reserve[3]
#endif


