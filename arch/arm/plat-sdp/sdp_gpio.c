/* sdp_gpio.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *  Ikjoon Jang <ij.jang@samsung.com>
 *
 * Simple GPIO support for SDPXX.
 * Do not declare platform devices for GPIO,
 * Use sdp_gpio_add_device to register GPIO platform device.
 */
 
/* TODOs 
 * kobj reference, sysfs interface
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <asm/arch/sdp_gpio.h>
#include <asm/arch/sdp_gpio_board.h>

#define pdev_to_gpio(pdev)	platform_get_drvdata(pdev)

#define SDP_GPIO_DEVNAME	"sdp_gpio"

#define SDP_GPIO_DEBUG
#define SDP_GPIO_DEBUG_RAW

#define printerr(fmt, arg...)	\
	do { printk (KERN_ERR "SDP_GPIO " fmt, ## arg); } while (0)

#define printinfo(fmt, arg...)	\
	do { printk (KERN_INFO "SDP_GPIO " fmt, ## arg); } while (0)
	
/* debug */
#if defined(SDP_GPIO_DEBUG)
#define printdbg(fmt, arg...)	\
	do { printk (KERN_DEBUG "SDP_GPIO " fmt, ## arg); } while (0)
#else
#define printdbg(fmt, arg...)
#endif

#if defined(SDP_GPIO_DEBUG_RAW)
/* debug more */
#define printraw(fmt, arg...)	\
	do { printk (KERN_DEBUG "SDP_GPIO " fmt, ## arg); } while (0)
#else
#define printraw(fmt, arg...)
#endif

/* driver data */
struct sdp_gpio {
	u32			reg_addr;	/* PA */
	size_t			reg_len;	/* mapping size */	
	int 			nports;
	
	spinlock_t		lock_pull;	/* lock for pull registers */	
	struct cdev		*cdev;
	
	void __iomem		*regbase;	/* ioremapped */
	struct sdp_gpio_port	ports[0];
};

static struct sdp_gpio *gpio_control;

static inline u32 sdp_gpio_readl(u32 __iomem *reg)
{
	u32 v = readl (reg);
	printraw ("read 0x%08x = 0x%08x\n", (u32)reg, v);
	return v;
}

static inline void sdp_gpio_writel(const u32 val, u32 __iomem *reg)
{
	printraw ("write 0x%08x = 0x%08x\n", (u32)reg, val);
	writel (val, reg);
}

static inline u32 sdp_gpio_writel_mask(const u32 val, u32 __iomem *reg,
				u32 mask, int shift)
{
	u32 tmp;
	
	tmp = sdp_gpio_readl (reg);
	
	tmp &= ~(mask << shift);
	tmp |= (val & mask) << shift;
	
	sdp_gpio_writel (tmp, reg);
	
	return tmp;
}

static void lock_port (struct sdp_gpio_port *port)
{
	spin_lock (&port->lock_port);
}

static void unlock_port (struct sdp_gpio_port *port)
{
	spin_unlock (&port->lock_port);
}

static void lock_pull (struct sdp_gpio *gpio)
{
	spin_lock (&gpio->lock_pull);
}

static void unlock_pull (struct sdp_gpio *gpio)
{
	spin_unlock (&gpio->lock_pull);
}
static struct sdp_gpio_port* find_port(struct sdp_gpio *gpio, int port)
{
	int i;
	for (i=0; i<gpio->nports; i++) {
		if (gpio->ports[i].portno == port)
			return &gpio->ports[i];
	}
	return NULL;
}

static int sdp_gpio_configure(struct sdp_gpio *gpio, struct gpio_ctl_param *param)
{
	struct sdp_gpio_port *port;
	
	/* no control value was setted */
	if ( !(param->val & (GPIO_FUNC_MASK | GPIO_PULL_MASK)) )
		return -EINVAL;

	port = find_port (gpio, param->port);
	if (!port || param->pin >= port->npins)
		return -ENODEV;
		
	/* function / in / out */
	if (param->val & GPIO_FUNC_MASK) {
		lock_port (port);
		
		/* TODO: check saved context */
		switch (param->val & GPIO_FUNC_MASK) {
		case GPIO_FUNC_IN:
			printdbg ("port%d pin%d set to input.\n", port->portno, param->pin);
			port->control =
				sdp_gpio_writel_mask (2, &port->reg->con, 3, param->pin * 4);			
			break;
		case GPIO_FUNC_OUT:
			printdbg ("port%d pin%d set to output.\n", port->portno, param->pin);
			port->control =
				sdp_gpio_writel_mask (3, &port->reg->con, 3, param->pin * 4);			
			break;
		case GPIO_FUNC_FN:
			printdbg ("port%d pin%d route to main function.\n", port->portno, param->pin);
			port->control =
				sdp_gpio_writel_mask (0, &port->reg->con, 3, param->pin * 4);
			break;
		default:
			/* XXX */
			break;
		}
		unlock_port (port);
	}
	
	/* pull-up / pull-down */
	if (param->val & GPIO_PULL_MASK) {
		struct sdp_gpio_pull *pull;
		u32 __iomem *pull_reg;
		
		if ( (port->control & (3 << (param->pin *4))) == 0) {
			/* XXX: Can PULL UP/DN be controlled when in FUNC mode? */
		}
		lock_pull (gpio);
		
		pull = &gpio->ports[param->port].pins[param->pin];
		pull_reg = gpio->regbase + pull->reg_offset;
		
		/* XXX: check saved context */
		
		switch (param->val & GPIO_PULL_MASK) {
		case GPIO_PULL_OFF:
			printdbg ("port%d pin%d pull control: off.\n", port->portno, param->pin);
			sdp_gpio_writel_mask (0, pull_reg, 3, pull->reg_shift);
			pull->state = SDP_GPIO_PULL_OFF;
			break;
		case GPIO_PULL_UP:
			printdbg ("port%d pin%d pull control: up.\n", port->portno, param->pin);
			sdp_gpio_writel_mask (3, pull_reg, 3, pull->reg_shift);
			pull->state = SDP_GPIO_PULL_UP;
			break;	
		case GPIO_PULL_DN:
			printdbg ("port%d pin%d pull control: down.\n", port->portno, param->pin);
			sdp_gpio_writel_mask (2, pull_reg, 3, pull->reg_shift);
			pull->state = SDP_GPIO_PULL_DN;
			break;
		default:
			break;
		}
		unlock_pull (gpio);
	}
	
	return 0;
}

static void sdp_gpio_set (struct sdp_gpio_port *port, int pin, int val)
{
	sdp_gpio_writel_mask (val, &port->reg->wdat, 1, pin);
}

static int sdp_gpio_get (struct sdp_gpio_port *port, int pin)
{
	u32 tmp = sdp_gpio_readl (&port->reg->rdat);
	if (tmp & (1<<pin))
		return 1;
	else
		return 0;
}

static int sdp_gpio_check_rw (struct sdp_gpio_port *port, int pin, int write)
{
	u32 val = (sdp_gpio_readl (&port->reg->con) >> (pin * 4)) & 3;
	
	if (write && (val != 3)) {
		printerr ("Write port%d pin%d: direction mismatch.\n", port->portno, pin);
		return -EBUSY;		
	}
	if (!write && (val != 2)) {
		printerr ("Read port%d pin%d: direction mismatch.\n", port->portno, pin);
		return -EBUSY;
	}
	return 0;
}

static int sdp_gpio_rw(struct sdp_gpio *gpio, struct gpio_ctl_param *param, int write)
{
	u32 val;
	int ret;
	struct sdp_gpio_port *port;
	
	/* no control value was setted */
	if (param->val & (GPIO_FUNC_MASK | GPIO_PULL_MASK)) {
		printdbg ("Control value was setted in request of io.\n");
		return -EINVAL;
	}

	port = find_port (gpio, param->port);
	if (!port || param->pin >= port->npins) {
		printerr ("Invalid port or pin number.\n");
		return -ENODEV;
	}
	
	/* check direction */
	ret = sdp_gpio_check_rw (port, param->pin, write);
	if (ret) {		
		return ret;
	}
	
	lock_port (port);
	
	if (write) {
		printdbg ("port%d pin%d set level to %d.\n", port->portno, param->pin, param->val);
		val = ((param->val & GPIO_LEVEL_MASK) == GPIO_LEVEL_HIGH) ? 1 : 0;
		sdp_gpio_set (port, param->pin, val);
		ret = 0;
	} else {
		val = sdp_gpio_get (port, param->pin);
		if (val) {
			param->val |= GPIO_LEVEL_HIGH;
		} else {
			param->val &= ~(GPIO_LEVEL_MASK);
			param->val |= GPIO_LEVEL_LOW;
		}
		printdbg ("port%d pin%d get level=%d.\n", port->portno, param->pin, param->val);
		ret = 0;
	}

	unlock_port(port);
	
	return ret;
}

static int sdp_gpio_ioctl (struct inode *inode, struct file *filp,
			   unsigned int cmd, unsigned long args)
{
	int ret;
	struct gpio_ctl_param param;
	struct sdp_gpio *gpio = (struct sdp_gpio *)filp->private_data;
	
	if (!gpio_control || gpio_control != gpio) {
		printerr ("Fatal.\n");
		return -ENODEV;
	}
	if (copy_from_user (&param, (void*)args, sizeof(param))) {
		printerr ("Failed to get ioctl argument.\n");
		return -EFAULT;
	}
	
	switch (cmd) {
	case GPIO_IOC_CONFIG:
		ret = sdp_gpio_configure (gpio, &param);
		break;
	case GPIO_IOC_READ:
		ret = sdp_gpio_rw (gpio, &param, 0);
		if (ret)
			goto exit_ioctl;
		if (copy_to_user ((void*)args, &param, sizeof(param))) {
			printerr ("Failed to give back ioctl argument.\n");
			ret = -EFAULT;
		}
		break;
	case GPIO_IOC_WRITE:
		ret = sdp_gpio_rw (gpio, &param, 1);
		break;
	default:
		ret = -EINVAL;
	};
exit_ioctl:
	return ret;
}

static int sdp_gpio_open (struct inode *inode, struct file *filp)
{
	if (gpio_control == NULL) {
		return -ENODEV;
	}
	filp->private_data = gpio_control;
	return 0;
}

static int sdp_gpio_release (struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static const struct file_operations sdp_gpio_fops = {
	.owner		= THIS_MODULE,
	.open		= sdp_gpio_open,
	.release	= sdp_gpio_release,
	.ioctl		= sdp_gpio_ioctl,
};

int __init sdp_gpio_add_device (u32 addr, size_t reg_len, struct sdp_gpio_port *ports, int nports)
{
	int ret;
	struct sdp_gpio *gpio = NULL;
	struct platform_device *pdev = NULL;
	
	/* only 1 GPIO controller on a system */
	if (gpio_control != NULL) {
		return -EBUSY;
	}

	/* create internal device data */
	gpio = kmalloc (sizeof(struct sdp_gpio_port) * nports + sizeof(*gpio), GFP_KERNEL);
	if (!gpio) {
		printerr ("Failed to allocate driver data.\n");		
		return -ENOMEM;
	}
	gpio->reg_addr = addr;
	gpio->reg_len = reg_len;
	gpio->nports = nports;	
	memcpy (gpio->ports, ports, sizeof(*ports)*nports);	
	
	/* platform device: only 1 GPIO controller on a bus */
	pdev = platform_device_alloc (SDP_GPIO_DEVNAME, 0);
	if (!pdev) {			
		printerr ("Failed to register device.\n");
		ret = -ENOMEM;
		goto err;
	}
	
	ret = platform_device_add (pdev);
	if (ret) {
		printerr ("Failed to register device to platform bus.\n");
		goto err;
	}

	gpio_control = gpio;

	printdbg ("Device is registered, %d ports.\n", nports);
	
	return 0;
err:
	if (pdev)
		platform_device_put (pdev);
	if (gpio)
		kfree (gpio);
	return ret;
}

static inline void sdp_gpio_port_init(void __iomem *reg, struct sdp_gpio_port *port)
{
	spin_lock_init (&port->lock_port);
	port->reg = reg + port->reg_offset;
	port->control = sdp_gpio_readl (&port->reg->con);
}

static int __devinit sdp_gpio_probe(struct platform_device *pdev)
{
	int ret, i;
	struct cdev *cdev = NULL;
	struct sdp_gpio *gpio = gpio_control;
	struct class *class = NULL;
	dev_t devid;

	/* only 1 GPIO controller on a system */
	if (!gpio) {
		printerr ("No GPIO devices are defined by machine.\n");
		return -ENODEV;
	}

	/* char device */
	ret = alloc_chrdev_region (&devid, 0, 1, SDP_GPIO_DEVNAME);
	if (ret) {
		printerr ("Failed to alloc char device id%s.\n", SDP_GPIO_DEVNAME);
		return ret;
	}	
	cdev = cdev_alloc ();
	if (!cdev) {
		ret = -EBUSY;
		goto err0;
	}
	cdev_init (cdev, &sdp_gpio_fops);
	cdev_add (cdev, devid, 1);
	printdbg ("char device registered major=%d, minor=%d.\n", MAJOR(cdev->dev), MINOR(cdev->dev));

	/* class */
	class = class_create (THIS_MODULE, SDP_GPIO_DEVNAME);
	if (!class) {
		printerr ("Failed to create sysfs gpio class.\n");
		ret = -EBUSY;
		goto err1;
	}
	printdbg ("sysfs class %s is registered.\n", SDP_GPIO_DEVNAME);
	
	/* class device : fake for registering a class device */
	/* XXX: class device is deprecated */
	class_device_create (class, NULL, devid, &pdev->dev, "%s%d", SDP_GPIO_DEVNAME, 0);
	
	gpio->regbase = ioremap_nocache (gpio->reg_addr, gpio->reg_len);
	if (!gpio->regbase) {
		printerr ("Failed to map io mem.\n");
		ret = -ENOMEM;
		goto err2;
	}
	printdbg ("io mem = 0x%08x, mapped to 0x%08x.\n", gpio->reg_addr, (u32)gpio->regbase);
	
	spin_lock_init (&gpio->lock_pull);
	for (i=0; i<gpio->nports; i++) {
		sdp_gpio_port_init (gpio->regbase, &gpio->ports[i]);
	}
	
	platform_set_drvdata (pdev, gpio);
	gpio->cdev = cdev;
	
	printinfo ("Probing completed. %d ports detected.\n", gpio->nports);
	return 0;
err2:
	iounmap (gpio->regbase);
err1:
	if (class)
		class_destroy (class);
	if (cdev)
		cdev_del (cdev);	
err0:
	unregister_chrdev_region (devid, 1);
	return ret;
}

/* this may not be called */
static int __devexit sdp_gpio_remove (struct platform_device *pdev)
{
	struct sdp_gpio *gpio = pdev_to_gpio (pdev);
	if (gpio) {
		iounmap (gpio->regbase);
		gpio->regbase = NULL;
		if (gpio->cdev) {
			cdev_del (gpio->cdev);
			gpio->cdev = NULL;
		}
	}
	return 0;
}

static struct platform_driver sdp_gpio_driver = {
	.probe		= sdp_gpio_probe,
	.remove		= sdp_gpio_remove,
	.driver		= {
		.name = SDP_GPIO_DEVNAME,
		.owner = THIS_MODULE,
	},
};

static int __init sdp_gpio_init(void)
{
	int ret;
	
	ret = platform_driver_register (&sdp_gpio_driver);
	if (ret < 0) {
		printerr ("Failed to register SDP GPIO platform driver.\n");
		return ret;
	} else {
		return 0;
	}
}
module_init (sdp_gpio_init);

static void __exit sdp_gpio_exit(void)
{
	platform_driver_unregister (&sdp_gpio_driver);
}
module_exit (sdp_gpio_exit);

MODULE_AUTHOR ("ij.jang@samsung.com");
MODULE_LICENSE ("GPL");

