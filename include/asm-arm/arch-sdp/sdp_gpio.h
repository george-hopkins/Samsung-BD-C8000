/* sdp_gpio.H
 *
 * Copyright (c) 2009 Samsung Electronics
 *  Ikjoon Jang <ij.jang@samsung.com>
 *
 * User level interface to SDP GPIO driver
 */
 
#ifndef _SDP_GPIO_IO_H_
#define _SDP_GPIO_IO_H_

#define SDP_GPIO_DEVNAME	"sdp_gpio"

/* user level command via ioctl */
#define GPIO_IOC_CONFIG		'J'
#define GPIO_IOC_WRITE		'K'
#define GPIO_IOC_READ		'L'

/* user level argument via ioctl */
struct gpio_ctl_param {
	unsigned int	port;
	unsigned int	pin;
	unsigned int	val;

/* available values for 'val' member */
/* When ioctl cmd is GPIO_IOC_[READ|WRITE] */
#define GPIO_LEVEL_MASK		0x001
#define GPIO_LEVEL_LOW		0x000
#define GPIO_LEVEL_HIGH		0x001
	
/* When ioctl cmd is GPIO_IOC_CONFIG */
/* control, not same as hw definitions */
#define GPIO_FUNC_MASK		0x030
#define GPIO_FUNC_FN		0x010
#define GPIO_FUNC_IN		0x020
#define GPIO_FUNC_OUT		0x030

/* pull, not same as hw definitions */
#define GPIO_PULL_MASK		0x300
#define GPIO_PULL_OFF		0x100
#define GPIO_PULL_DN		0x200
#define GPIO_PULL_UP		0x300
};

#endif
