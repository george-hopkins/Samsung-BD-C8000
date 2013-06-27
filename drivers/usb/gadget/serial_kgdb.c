/*
 * gadget/serial_kgdb.c -- USB gadget serial driver for KGDB over USB
 *
 * Author: John Mehaffey
 *         MontaVista Software, Inc.
 *         mehaf@mvista.com
 *         source@mvista.com
 *
 *  (C) Copyright 2007 MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2, as published
 *  by the Free Software Foundation.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Revision History (most recent first)
 * 2007Aug25 jfm - Original Version
 *
 * This code is based on the gadget serial driver, which is
 * Copyright 2003 (C) Al Borchers (alborchers@steinerpoint.com),
 * which was based in part on the Gadget Zero driver, which
 * is Copyright (C) 2003 by David Brownell, all rights reserved.
 *
 * The gadget serial driver also borrows from usbserial.c, which is
 * Copyright (C) 1999 - 2002 Greg Kroah-Hartman (greg@kroah.com)
 * Copyright (C) 2000 Peter Berger (pberger@brimson.com)
 * Copyright (C) 2000 Al Borchers (alborchers@steinerpoint.com)
 *
 * This code also borrows ideas and code snippets from kgdb_eth.c
 * and 8250_kgdb.c.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/utsname.h>
#include <linux/device.h>
#include <linux/irqreturn.h>
#include <linux/kgdb.h>
#include <linux/list.h>

#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/atomic.h>

#include <linux/usb/ch9.h>
#include <linux/usb/cdc.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"


/* Defines */

#define GS_DEBUG 1

/* Note that the version for KGDB is bumped one minor rev from that
 * of the original g_serial driver
 */
#define GS_VERSION_STR			"v2.3"
#define GS_VERSION_NUM			0x0203

#define GS_LONG_NAME			"KGDB Serial Gadget"
#define GS_SHORT_NAME			"kgdb_gs"

#define GS_NUM_PORTS			1

#define GS_DEFAULT_READ_Q_SIZE		3
#define GS_DEFAULT_WRITE_Q_SIZE		3

#define GS_DEFAULT_READ_BUF_SIZE	512
#define GS_DEFAULT_WRITE_BUF_SIZE	32

#define GS_DEFAULT_USE_ACM		1

#include "gadget_serial.h"

static struct gs_dev *gs_device;
static int kgdbou_registered = 0;

/* un-variables */
static const char *EP_IN_NAME;
static const char *EP_OUT_NAME;
static const char *EP_NOTIFY_NAME;

static unsigned int read_q_size = GS_DEFAULT_READ_Q_SIZE;
static unsigned int write_q_size = GS_DEFAULT_WRITE_Q_SIZE;

static unsigned int read_buf_size = GS_DEFAULT_READ_BUF_SIZE;
static unsigned int write_buf_size = GS_DEFAULT_WRITE_BUF_SIZE;

static unsigned int use_acm = GS_DEFAULT_USE_ACM;

/* external functions */
extern int net2280_set_fifo_mode(struct usb_gadget *gadget, int mode);
extern irqreturn_t usb_controller_poll(void);

/* Gadget Driver */

/*
 * gs_alloc_ports
 *
 * Allocate all ports and set the gs_dev struct to point to them.
 * Return 0 if successful, or a negative error number.
 *
 * The device lock is normally held when calling this function.
 */
static int gs_alloc_ports(struct gs_dev *dev, gfp_t kmalloc_flags)
{
	int i;
	struct gs_port *port;

	if (dev == NULL)
		return -EIO;

	for (i=0; i<GS_NUM_PORTS; i++) {
		if ((port=kzalloc(sizeof(struct gs_port), kmalloc_flags)) == NULL)
			return -ENOMEM;

		port->port_dev = dev;
		port->port_num = i;
		port->port_line_coding.dwDTERate = cpu_to_le32(GS_DEFAULT_DTE_RATE);
		port->port_line_coding.bCharFormat = GS_DEFAULT_CHAR_FORMAT;
		port->port_line_coding.bParityType = GS_DEFAULT_PARITY;
		port->port_line_coding.bDataBits = GS_DEFAULT_DATA_BITS;
		spin_lock_init(&port->port_lock);
		init_waitqueue_head(&port->port_write_wait);

		dev->dev_port[i] = port;

	/* allocate in/out buffers */
		port->in_buf.buffer = (char *)kmalloc(read_buf_size,
						      kmalloc_flags);

		if ((port->in_buf.buffer) == NULL) {
			printk(KERN_ERR "kgdbgs_alloc_ports: "
			       "cannot allocate port read buffer\n");
			kfree(port);
			return -ENOMEM;
		}
		port->in_buf.size = read_buf_size;
		port->in_buf.head = port->in_buf.tail = 0;
		atomic_set(&port->in_buf.count, 0);

		port->out_buf.buffer = (char *)kmalloc(write_buf_size,
						       kmalloc_flags);

		if ((port->out_buf.buffer) == NULL) {
			printk(KERN_ERR "kgdbgs_alloc_ports: "
			       "cannot allocate port write buffer\n");
			kfree(port->in_buf.buffer);
			kfree(port);
			return -ENOMEM;
		}
		port->out_buf.size = write_buf_size;
		port->out_buf.head = port->out_buf.tail = 0;
		atomic_set(&port->out_buf.count, 0);

	}

	return 0;
}

/* gs_free_port
 * Free a port and associated buffers.  Open ports are disconnected by
 * freeing their write buffers, setting their device pointers
 * and the pointers to them in the device to NULL.  These
 * ports will be freed when closed.
 */

static void gs_free_port(struct gs_port *port)
{
#if defined (CONFIG_KGDBOU) || defined (CONFIG_KGDBOU_MODULE)
	if (port->port_num == 0) {

		kfree(port->in_buf.buffer);
		kfree(port->out_buf.buffer);
		kfree(port);
		return;
	}
#else
	unsigned long flags;

	spin_lock_irqsave(&port->port_lock, flags);

	if (port->port_write_buf != NULL) {
		kfree(port->port_write_buf->buf_buf);
		kfree(port->port_write_buf);
		port->port_write_buf = NULL;
	}

	if (port->port_open_count > 0 || port->port_in_use) {
		port->port_dev = NULL;
		wake_up_interruptible(&port->port_write_wait);
		if (port->port_tty) {
			wake_up_interruptible(&port->port_tty->read_wait);
			wake_up_interruptible(&port->port_tty->write_wait);
		}
		spin_unlock_irqrestore(&port->port_lock, flags);
	} else {
		spin_unlock_irqrestore(&port->port_lock, flags);
		kfree(port);
	}
#endif
}

/*
 * gs_free_ports
 *
 * Free all closed ports, and disconnect open ports.
 *
 * The device lock is normally held when calling this function.
 */
static void gs_free_ports(struct gs_dev *dev)
{
	int i;
	struct gs_port *port;

	if (dev == NULL)
		return;

	for (i=0; i<GS_NUM_PORTS; i++) {
		if ((port=dev->dev_port[i]) != NULL) {
			dev->dev_port[i] = NULL;
			gs_free_port(port);
		}
	}
}

/*
 * gs_alloc_req
 *
 * Allocate a usb_request and its buffer.  Returns a pointer to the
 * usb_request or NULL if there is an error.
 */
static struct usb_request *
gs_alloc_req(struct usb_ep *ep, unsigned int len, gfp_t kmalloc_flags)
{
	struct usb_request *req;

	if (ep == NULL)
		return NULL;

	req = usb_ep_alloc_request(ep, kmalloc_flags);

	if (req != NULL) {
		req->length = len;
		req->buf = kmalloc(len, kmalloc_flags);
		if (req->buf == NULL) {
			usb_ep_free_request(ep, req);
			return NULL;
		}
	}

	return req;
}

/*
 * gs_free_req
 *
 * Free a usb_request and its buffer.
 */
static void gs_free_req(struct usb_ep *ep, struct usb_request *req)
{
	if (ep != NULL && req != NULL) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

/*
 * kgdbgs_recv_packet
 *
 * Called for each USB packet received.  Reads the packet
 * header and stuffs the data in the input buffer
 *
 * Called from USB completion routine, on interrupt time.
 */
static void kgdbgs_recv_packet(struct gs_dev *dev, char *packet, unsigned int size)
{
	int i;
	struct gs_port *port = dev->dev_port[0];   /* only support port 0 */

	gs_debug_level(3, "kgdbgs_recv_packet, size=%d\n", size);
	/*
	 * This could be GDB trying to attach.  But it could also be GDB
	 * finishing up a session, with kgdb_connected=0 but GDB sending
	 * an ACK for the final packet.  To make sure we don't try and
	 * make a breakpoint when GDB is leaving, make sure that if
	 * !kgdb_connected the only len == 1 packet we wake up on is ^C.
	 */
	if (!kgdb_connected && size != 1 &&
	    !atomic_read(&kgdb_setting_breakpoint)) {
		tasklet_schedule(&kgdb_tasklet_breakpoint);
	}

	for (i = 0; i < size; i++) {
		if (packet[i] == 3)
			tasklet_schedule(&kgdb_tasklet_breakpoint);

		if (atomic_read(&port->in_buf.count) >= port->in_buf.size) {
			/* buffer overflow, clear it */
			gs_debug("input buffer overflow, clearing all data "
				 "count=%d, size=%d\n",
				 atomic_read(&port->in_buf.count),
				 port->in_buf.size);
			port->in_buf.head = port->in_buf.tail = 0;
			atomic_set(&port->in_buf.count, 0);
			break; /* discard rest of packet as well */
		}
		port->in_buf.buffer[port->in_buf.head++] = packet[i];
		if (port->in_buf.head == port->in_buf.size)
			port->in_buf.head = 0; /* wrap */

		atomic_inc(&port->in_buf.count);
	}

}

/*
* gs_read_complete
*/
static void gs_read_complete(struct usb_ep *ep, struct usb_request *req)
{
	int ret;
	struct gs_dev *dev = ep->driver_data;

	if (dev == NULL) {
		printk(KERN_ERR "gs_read_complete: NULL device pointer\n");
		return;
	}

	gs_debug_level(3, "gs_read_complete: status %d\n", req->status);


	switch(req->status) {
	case 0:
 		/* normal completion */
		kgdbgs_recv_packet(dev, req->buf, req->actual);
requeue:
		req->length = ep->maxpacket;
		if ((ret=usb_ep_queue(ep, req, GFP_ATOMIC))) {
			printk(KERN_ERR
			"gs_read_complete: cannot requeue read request, ret=%d\n",
				ret);
		}
		break;

	case -ESHUTDOWN:
		/* disconnect */
		gs_debug("gs_read_complete: shutdown\n");
		gs_free_req(ep, req);
		break;

	default:
		/* unexpected */
		printk(KERN_ERR
		"gs_read_complete: unexpected status error, status=%d\n",
			req->status);
		goto requeue;
		break;
	}
}

/*
* gs_write_complete
*/
static void gs_write_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gs_dev *dev = ep->driver_data;
	struct gs_req_entry *gs_req = req->context;

	if (dev == NULL) {
		printk(KERN_ERR "gs_write_complete: NULL device pointer\n");
		return;
	}

	gs_debug_level(3, "gs_write_complete: status %d\n", req->status);

	switch(req->status) {
	case 0:
		/* normal completion */
requeue:
		if (gs_req == NULL) {
			printk(KERN_ERR
				"gs_write_complete: NULL request pointer\n");
			return;
		}

#if defined (CONFIG_KGDBOU) || defined (CONFIG_KGDBOU_MODULE)
		gs_req->in_use = 0;
#else
		spin_lock(&dev->dev_lock);
		list_add(&gs_req->re_entry, &dev->dev_req_list);
		spin_unlock(&dev->dev_lock);

		gs_send(dev);
#endif
		break;

	case -ESHUTDOWN:
		/* disconnect */
		gs_debug("gs_write_complete: shutdown\n");
		gs_free_req(ep, req);
		break;

	default:
		printk(KERN_ERR
		"gs_write_complete: unexpected status error, status=%d\n",
			req->status);
		goto requeue;
		break;
	}
}

/*
 * gs_alloc_req_entry
 *
 * Allocates a request and its buffer, using the given
 * endpoint, buffer len, and kmalloc flags.
 */
static struct gs_req_entry *
gs_alloc_req_entry(struct usb_ep *ep, unsigned len, gfp_t kmalloc_flags)
{
	struct gs_req_entry	*req;

	req = kmalloc(sizeof(struct gs_req_entry), kmalloc_flags);
	if (req == NULL)
		return NULL;

	req->re_req = gs_alloc_req(ep, len, kmalloc_flags);
	if (req->re_req == NULL) {
		kfree(req);
		return NULL;
	}

	req->re_req->context = req;

#if defined (CONFIG_KGDBOU) || defined (CONFIG_KGDBOU_MODULE)
	req->in_use = 0;
#endif

	return req;
}

/*
 * gs_free_req_entry
 *
 * Frees a request and its buffer.
 */
static void gs_free_req_entry(struct usb_ep *ep, struct gs_req_entry *req)
{
	if (ep != NULL && req != NULL) {
		if (req->re_req != NULL)
			gs_free_req(ep, req->re_req);
		kfree(req);
	}
}

/*
 * gs_reset_config
 *
 * Mark the device as not configured, disable all endpoints,
 * which forces completion of pending I/O and frees queued
 * requests, and free the remaining write requests on the
 * free list.
 *
 * The device lock must be held when calling this function.
 */
static void gs_reset_config(struct gs_dev *dev)
{
	struct gs_req_entry *req_entry;

	if (dev == NULL) {
		printk(KERN_ERR "gs_reset_config: NULL device pointer\n");
		return;
	}

	if (dev->dev_config == GS_NO_CONFIG_ID)
		return;

	dev->dev_config = GS_NO_CONFIG_ID;

	/* free write requests on the free list */
	/* TODO <<<<handle kgdb free/inuse >>>> */
	while(!list_empty(&dev->dev_req_list)) {
		req_entry = list_entry(dev->dev_req_list.next,
			struct gs_req_entry, re_entry);
		list_del(&req_entry->re_entry);
		gs_free_req_entry(dev->dev_in_ep, req_entry);
	}

	/* disable endpoints, forcing completion of pending i/o; */
	/* completion handlers free their requests in this case */
	if (dev->dev_notify_ep) {
		usb_ep_disable(dev->dev_notify_ep);
		dev->dev_notify_ep = NULL;
	}
	if (dev->dev_in_ep) {
		usb_ep_disable(dev->dev_in_ep);
		dev->dev_in_ep = NULL;
	}
	if (dev->dev_out_ep) {
		usb_ep_disable(dev->dev_out_ep);
		dev->dev_out_ep = NULL;
	}
}

/*
 * gs_set_config
 *
 * Configures the device by enabling device specific
 * optimizations, setting up the endpoints, allocating
 * read and write requests and queuing read requests.
 *
 * The device lock must be held when calling this function.
 */
static int gs_set_config(struct gs_dev *dev, unsigned config)
{
	int i;
	int ret = 0;
	struct usb_gadget *gadget = dev->dev_gadget;
	struct usb_ep *ep;
	struct usb_endpoint_descriptor *ep_desc;
	struct usb_request *req;
	struct gs_req_entry *req_entry;

	if (dev == NULL) {
		printk(KERN_ERR "gs_set_config: NULL device pointer\n");
		return 0;
	}

	if (config == dev->dev_config)
		return 0;

	gs_reset_config(dev);

	switch (config) {
	case GS_NO_CONFIG_ID:
		return 0;
	case GS_BULK_CONFIG_ID:
		if (use_acm)
			return -EINVAL;
		/* device specific optimizations */
		if (gadget_is_net2280(gadget))
			net2280_set_fifo_mode(gadget, 1);
		break;
	case GS_ACM_CONFIG_ID:
		if (!use_acm)
			return -EINVAL;
		/* device specific optimizations */
		if (gadget_is_net2280(gadget))
			net2280_set_fifo_mode(gadget, 1);
		break;
	default:
		return -EINVAL;
	}

	dev->dev_config = config;

	gadget_for_each_ep(ep, gadget) {

		if (EP_NOTIFY_NAME
		&& strcmp(ep->name, EP_NOTIFY_NAME) == 0) {
			ep_desc = GS_SPEED_SELECT(
				gadget->speed == USB_SPEED_HIGH,
				&gs_highspeed_notify_desc,
				&gs_fullspeed_notify_desc);
			ret = usb_ep_enable(ep,ep_desc);
			if (ret == 0) {
				ep->driver_data = dev;
				dev->dev_notify_ep = ep;
				dev->dev_notify_ep_desc = ep_desc;
			} else {
				printk(KERN_ERR "gs_set_config: cannot enable notify endpoint %s, ret=%d\n",
					ep->name, ret);
				goto exit_reset_config;
			}
		}

		else if (strcmp(ep->name, EP_IN_NAME) == 0) {
			ep_desc = GS_SPEED_SELECT(
				gadget->speed == USB_SPEED_HIGH,
 				&gs_highspeed_in_desc,
				&gs_fullspeed_in_desc);
			ret = usb_ep_enable(ep,ep_desc);
			if (ret == 0) {
				ep->driver_data = dev;
				dev->dev_in_ep = ep;
				dev->dev_in_ep_desc = ep_desc;
			} else {
				printk(KERN_ERR "gs_set_config: cannot enable in endpoint %s, ret=%d\n",
					ep->name, ret);
				goto exit_reset_config;
			}
		}

		else if (strcmp(ep->name, EP_OUT_NAME) == 0) {
			ep_desc = GS_SPEED_SELECT(
				gadget->speed == USB_SPEED_HIGH,
				&gs_highspeed_out_desc,
				&gs_fullspeed_out_desc);
			ret = usb_ep_enable(ep,ep_desc);
			if (ret == 0) {
				ep->driver_data = dev;
				dev->dev_out_ep = ep;
				dev->dev_out_ep_desc = ep_desc;
			} else {
				printk(KERN_ERR "gs_set_config: cannot enable out endpoint %s, ret=%d\n",
					ep->name, ret);
				goto exit_reset_config;
			}
		}

	}

	if (dev->dev_in_ep == NULL || dev->dev_out_ep == NULL
	|| (config != GS_BULK_CONFIG_ID && dev->dev_notify_ep == NULL)) {
		printk(KERN_ERR "gs_set_config: cannot find endpoints\n");
		ret = -ENODEV;
		goto exit_reset_config;
	}

	/* allocate and queue read requests */
	ep = dev->dev_out_ep;
	for (i=0; i<read_q_size && ret == 0; i++) {
		if ((req=gs_alloc_req(ep, ep->maxpacket, GFP_ATOMIC))) {
			req->complete = gs_read_complete;
			if ((ret=usb_ep_queue(ep, req, GFP_ATOMIC))) {
				printk(KERN_ERR "gs_set_config: cannot queue read request, ret=%d\n",
					ret);
			}
		} else {
			printk(KERN_ERR "gs_set_config: cannot allocate read requests\n");
			ret = -ENOMEM;
			goto exit_reset_config;
		}
	}

	/* allocate write requests, and put on free list */
	ep = dev->dev_in_ep;
	for (i=0; i<write_q_size; i++) {
		if ((req_entry=gs_alloc_req_entry(ep, ep->maxpacket, GFP_ATOMIC))) {
			req_entry->re_req->complete = gs_write_complete;
			list_add(&req_entry->re_entry, &dev->dev_req_list);
		} else {
			printk(KERN_ERR "gs_set_config: cannot allocate write requests\n");
			ret = -ENOMEM;
			goto exit_reset_config;
		}
	}

	printk(KERN_INFO "gs_set_config: %s configured, %s speed %s config\n",
		GS_LONG_NAME,
		gadget->speed == USB_SPEED_HIGH ? "high" : "full",
		config == GS_BULK_CONFIG_ID ? "BULK" : "CDC-ACM");

	return 0;

exit_reset_config:
	gs_reset_config(dev);
	return ret;
}

/*
 * gs_build_config_buf
 *
 * Builds the config descriptors in the given buffer and returns the
 * length, or a negative error number.
 */
static int gs_build_config_buf(u8 *buf, enum usb_device_speed speed,
	u8 type, unsigned int index, int is_otg)
{
	int len;
	int high_speed;
	const struct usb_config_descriptor *config_desc;
	const struct usb_descriptor_header **function;

	if (index >= gs_device_desc.bNumConfigurations)
		return -EINVAL;

	/* other speed switches high and full speed */
	high_speed = (speed == USB_SPEED_HIGH);
	if (type == USB_DT_OTHER_SPEED_CONFIG)
		high_speed = !high_speed;

	if (use_acm) {
		config_desc = &gs_acm_config_desc;
		function = GS_SPEED_SELECT(high_speed,
			gs_acm_highspeed_function,
			gs_acm_fullspeed_function);
	} else {
		config_desc = &gs_bulk_config_desc;
		function = GS_SPEED_SELECT(high_speed,
			gs_bulk_highspeed_function,
			gs_bulk_fullspeed_function);
	}

	/* for now, don't advertise srp-only devices */
	if (!is_otg)
		function++;

	len = usb_gadget_config_buf(config_desc, buf, GS_MAX_DESC_LEN, function);
	if (len < 0)
		return len;

	((struct usb_config_descriptor *)buf)->bDescriptorType = type;

	return len;
}

/*
 * gs_setup_standard
 * called by gs_setup
 * implements USB_TYPE_STANDARD control request
 */
static int gs_setup_standard(struct usb_gadget *gadget,
	const struct usb_ctrlrequest *ctrl)
{
	int ret = -EOPNOTSUPP;
	struct gs_dev *dev = get_gadget_data(gadget);
	struct usb_request *req = dev->dev_ctrl_req;
	u16 wIndex = le16_to_cpu(ctrl->wIndex);
	u16 wValue = le16_to_cpu(ctrl->wValue);
	u16 wLength = le16_to_cpu(ctrl->wLength);

	switch (ctrl->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			break;

		switch (wValue >> 8) {
		case USB_DT_DEVICE:
			ret = min(wLength,
				(u16)sizeof(struct usb_device_descriptor));
			memcpy(req->buf, &gs_device_desc, ret);
			break;

#ifdef CONFIG_USB_GADGET_DUALSPEED
		case USB_DT_DEVICE_QUALIFIER:
			if (!gadget->is_dualspeed)
				break;
			ret = min(wLength,
				(u16)sizeof(struct usb_qualifier_descriptor));
			memcpy(req->buf, &gs_qualifier_desc, ret);
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
			if (!gadget->is_dualspeed)
				break;
			/* fall through */
#endif /* CONFIG_USB_GADGET_DUALSPEED */
		case USB_DT_CONFIG:
			ret = gs_build_config_buf(req->buf, gadget->speed,
				wValue >> 8, wValue & 0xff,
				gadget->is_otg);
			if (ret >= 0)
				ret = min(wLength, (u16)ret);
			break;

		case USB_DT_STRING:
			/* wIndex == language code. */
			ret = usb_gadget_get_string(&gs_string_table,
				wValue & 0xff, req->buf);
			if (ret >= 0)
				ret = min(wLength, (u16)ret);
			break;
		}
		break;

	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != 0)
			break;
		spin_lock(&dev->dev_lock);
		ret = gs_set_config(dev, wValue);
		spin_unlock(&dev->dev_lock);
		break;

	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != USB_DIR_IN)
			break;
		*(u8 *)req->buf = dev->dev_config;
		ret = min(wLength, (u16)1);
		break;

	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != USB_RECIP_INTERFACE
				|| !dev->dev_config
				|| wIndex >= GS_MAX_NUM_INTERFACES)
			break;
		if (dev->dev_config == GS_BULK_CONFIG_ID
				&& wIndex != GS_BULK_INTERFACE_ID)
			break;
		/* no alternate interface settings */
		if (wValue != 0)
			break;
		spin_lock(&dev->dev_lock);
		/* PXA hardware partially handles SET_INTERFACE;
		 * we need to kluge around that interference.  */
		if (gadget_is_pxa(gadget)) {
			ret = gs_set_config(dev, use_acm ?
				GS_ACM_CONFIG_ID : GS_BULK_CONFIG_ID);
			goto set_interface_done;
		}
		if (dev->dev_config != GS_BULK_CONFIG_ID
				&& wIndex == GS_CONTROL_INTERFACE_ID) {
			if (dev->dev_notify_ep) {
				usb_ep_disable(dev->dev_notify_ep);
				usb_ep_enable(dev->dev_notify_ep, dev->dev_notify_ep_desc);
			}
		} else {
			usb_ep_disable(dev->dev_in_ep);
			usb_ep_disable(dev->dev_out_ep);
			usb_ep_enable(dev->dev_in_ep, dev->dev_in_ep_desc);
			usb_ep_enable(dev->dev_out_ep, dev->dev_out_ep_desc);
		}
		ret = 0;
set_interface_done:
		spin_unlock(&dev->dev_lock);
		break;

	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE)
		|| dev->dev_config == GS_NO_CONFIG_ID)
			break;
		if (wIndex >= GS_MAX_NUM_INTERFACES
				|| (dev->dev_config == GS_BULK_CONFIG_ID
				&& wIndex != GS_BULK_INTERFACE_ID)) {
			ret = -EDOM;
			break;
		}
		/* no alternate interface settings */
		*(u8 *)req->buf = 0;
		ret = min(wLength, (u16)1);
		break;

	default:
		printk(KERN_ERR "gs_setup: unknown standard request, type=%02x, request=%02x, value=%04x, index=%04x, length=%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			wValue, wIndex, wLength);
		break;
	}

	return ret;
}

/*
 * gs_setup_classs
 * called by gs_setup
 * implements USB_TYPE_CLASS control request
 */
static int gs_setup_class(struct usb_gadget *gadget,
	const struct usb_ctrlrequest *ctrl)
{
	int ret = -EOPNOTSUPP;
	struct gs_dev *dev = get_gadget_data(gadget);
	struct gs_port *port = dev->dev_port[0];	/* ACM only has one port */
	struct usb_request *req = dev->dev_ctrl_req;
	u16 wIndex = le16_to_cpu(ctrl->wIndex);
	u16 wValue = le16_to_cpu(ctrl->wValue);
	u16 wLength = le16_to_cpu(ctrl->wLength);

	switch (ctrl->bRequest) {
	case USB_CDC_REQ_SET_LINE_CODING:
		ret = min(wLength,
			(u16)sizeof(struct usb_cdc_line_coding));
		if (port) {
			spin_lock(&port->port_lock);
			memcpy(&port->port_line_coding, req->buf, ret);
			spin_unlock(&port->port_lock);
		}
		ret = 0;
		break;

	case USB_CDC_REQ_GET_LINE_CODING:
		port = dev->dev_port[0];	/* ACM only has one port */
		ret = min(wLength,
			(u16)sizeof(struct usb_cdc_line_coding));
		if (port) {
			spin_lock(&port->port_lock);
			memcpy(req->buf, &port->port_line_coding, ret);
			spin_unlock(&port->port_lock);
		}
		break;

	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		ret = 0;
		break;

	default:
		printk(KERN_ERR "gs_setup: unknown class request, type=%02x, "
		       "request=%02x, value=%04x, index=%04x, length=%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			wValue, wIndex, wLength);
		break;
	}

	return ret;
}

/*
 * gs_setup_complete
 * completion routine for control requests
 */
static void gs_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length) {
		printk(KERN_ERR "gs_setup_complete: status error, status=%d, "
		       "actual=%d, length=%d\n",
			req->status, req->actual, req->length);
	}
}
/* KGDB over USB routines */

/* kgdbgs_init
 * initializes structures used by kgdb.
 * Returns 0 if successful, or -error
 */

static int kgdbgs_init(void)
{
	struct gs_port *port;
	unsigned long flags;
	int ret;

	gs_debug("kgdbgs_init\n");

	if (gs_device == NULL) {
		printk(KERN_ERR "kgdbgs_init: NULL device pointer\n");
		return -ENODEV;
	}

	spin_lock_irqsave(&gs_device->dev_lock, flags);

	if (gs_device->dev_config == GS_NO_CONFIG_ID) {
		printk(KERN_ERR "kgdbgs_init: device is not connected\n");
		ret = -ENODEV;
		goto exit_unlock_dev;
	}

	port = gs_device->dev_port[0];

	if (port == NULL) {
		printk(KERN_ERR "kgdbgs_init: NULL port pointer\n");
		ret = -ENODEV;
		goto exit_unlock_dev;
	}

	spin_unlock(&gs_device->dev_lock);

	gs_debug("kgdbgs_init: completed\n");

	return 0;

exit_unlock_dev:
	spin_unlock_irqrestore(&gs_device->dev_lock, flags);
	return ret;
}

/* copy_kgdb_buf()
 * copy data from a kgdbgs_buf to another buffer
 */
static void copy_kgdb_buf(char *out, struct kgdbgs_buf *in, int size)
{
	int len;

	if (in->tail + size > in->size) {
		len = in->size - in->tail;
		memcpy(out+len, in->buffer, size - len);
	} else
		len = size;

	memcpy(out, in->buffer + in->tail, len);
}

/* kgdbgs_send()
 * Find an available write request, fill it with data from the kgdb output
 * buffer, and send it to the usb subsystem.  Continue until there is no more
 * data to send, or there are no more free write requests.  Treat dev_req_list
 * as a circular list of requests which may be busy or free, rather than as the
 * free list used in the standard gs_serial driver.
 *
 * No real need for atomic operations, since this will always
 * be called with interrupts off.
 */
static int kgdbgs_send(struct gs_port *port)
{
	struct usb_ep *ep;
	struct gs_req_entry *circ_write_list;
	struct usb_request *req;
	unsigned int len;
	unsigned int req_len;
	unsigned int max_len;

	int ret=1; 	/* assume no free req_entries */

	if (port == NULL) {
		printk(KERN_ERR "kgdbgs_send: NULL device port\n");
		return -ENODEV;
	}

	len = atomic_read(&port->out_buf.count);

	if (len == 0)
		return 0;

	ep = gs_device->dev_in_ep;
	max_len = ep->maxpacket;

	/* start search from dev_req_list */
	circ_write_list = list_entry(gs_device->dev_req_list.next,
				     struct gs_req_entry, re_entry);

	while (len > 0) {
		if (circ_write_list->in_use == 0) {	/* got a free one */
			circ_write_list->in_use = 1;
			req = circ_write_list->re_req;
			req_len = min(len, max_len);
			copy_kgdb_buf(req->buf, &port->out_buf, req_len);
			gs_debug_level(3, "kgdbgs_send: len=%d, "
				       "0x%2.2x 0x%2.2x 0x%2.2x ...\n",
				       len,
				       *((unsigned char *)req->buf),
				       *((unsigned char *)req->buf+1),
				       *((unsigned char *)req->buf+2));
			req->length = req_len;
			if ((ret=usb_ep_queue(ep, req, GFP_ATOMIC))) {
				printk(KERN_ERR
				"kgdbgs_send: cannot queue write request, "
				       "ret=%d\n", ret);
				break;
			}

			port->out_buf.tail += req_len;
			if (port->out_buf.tail >= (port->out_buf.size))
				port->out_buf.tail -= port->out_buf.size;
			atomic_sub(req_len, &port->out_buf.count);
			len -= req_len;
		}
		if (list_is_last(&circ_write_list->re_entry,
				 &gs_device->dev_req_list))
			break;	/* exhausted all entries */

		circ_write_list = list_entry(circ_write_list->re_entry.next,
					      struct gs_req_entry, re_entry);
	}
	/* start next search where we left off. */
	if (!list_is_last(&circ_write_list->re_entry, &gs_device->dev_req_list))
		list_move_tail(&gs_device->dev_req_list,
			       &circ_write_list->re_entry);

	return ret;
}

/* kgdbgs_flush_buffer
 * send output buffer to USB. If no free usb write requests, poll until
 * one frees up.
 */
static void kgdbgs_flush_buffer(void)
{
	struct gs_port *port = gs_device->dev_port[0];

	gs_debug_level(4, "kgdbgs_flush\n");

	while (kgdbgs_send(port) > 0)
		usb_controller_poll();

	gs_debug_level(4, "kgdbgs_flush complete\n");
}

/* kgdbgs_putchar
 * put a char into the output buffer
 */
static void kgdbgs_putchar(u8 chr)
{
	struct gs_port *port = gs_device->dev_port[0];

	gs_debug_level(4, "kgdbgs_putchar 0x%2.2x\n", chr);

	port->out_buf.buffer[port->out_buf.head++] = chr;

	if (port->out_buf.head == port->out_buf.size)
		port->out_buf.head = 0;

	atomic_inc(&port->out_buf.count);

	if (atomic_read(&port->out_buf.count) == port->out_buf.size)
		kgdbgs_flush_buffer();
}

/* kgdbgs_getchar
 * gets a char from input buffer. If no chars in input buffer, poll
 * until one appears.
 */
static int kgdbgs_getchar(void)
{
	int chr;
	struct gs_port *port = gs_device->dev_port[0];

	gs_debug_level(4, "kgdbgs_getchar\n");

	while (atomic_read(&port->in_buf.count) == 0)
		usb_controller_poll();

	chr = port->in_buf.buffer[port->in_buf.tail++];
	if (port->in_buf.tail == port->in_buf.size)
		port->in_buf.tail = 0;

	atomic_dec(&port->in_buf.count);

	gs_debug_level(4, "kgdbgs_getchar complete, chr=0x%2.2x\n", chr);

	return chr;
}

/* kgdb over USB operations */

static struct kgdb_io kgdbgs_io_ops = {
	.init =		kgdbgs_init,
	.write_char =	kgdbgs_putchar,
	.read_char =	kgdbgs_getchar,
	.flush =	kgdbgs_flush_buffer,
};


/*
 * gs_setup
 *
 * Implements all the control endpoint functionality that's not
 * handled in hardware or the hardware driver.
 *
 * Returns the size of the data sent to the host, or a negative
 * error number.
 */
static int gs_setup(struct usb_gadget *gadget,
	const struct usb_ctrlrequest *ctrl)
{
	int ret = -EOPNOTSUPP;
	struct gs_dev *dev = get_gadget_data(gadget);
	struct usb_request *req = dev->dev_ctrl_req;
	u16 wIndex = le16_to_cpu(ctrl->wIndex);
	u16 wValue = le16_to_cpu(ctrl->wValue);
	u16 wLength = le16_to_cpu(ctrl->wLength);

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		ret = gs_setup_standard(gadget,ctrl);
		break;

	case USB_TYPE_CLASS:
		ret = gs_setup_class(gadget,ctrl);
		break;

	default:
		printk(KERN_ERR "gs_setup: unknown request, type=%02x, "
		       "request=%02x, value=%04x, index=%04x, length=%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			wValue, wIndex, wLength);
		break;
	}

	/* respond with data transfer before status phase? */
	if (ret >= 0) {
		req->length = ret;
		req->zero = ret < wLength
				&& (ret % gadget->ep0->maxpacket) == 0;
		ret = usb_ep_queue(gadget->ep0, req, GFP_ATOMIC);
		if (ret < 0) {
			printk(KERN_ERR "gs_setup: cannot queue response, ret=%d\n",
				ret);
			req->status = 0;
			gs_setup_complete(gadget->ep0, req);
		}
	}

	/* device either stalls (ret < 0) or reports success */
	if (ret == 0) {
		if (kgdbou_registered) {
			gs_debug_level(2, "kgdbou already registered!\n");
		} else {
			kgdb_register_io_module(&kgdbgs_io_ops);
			kgdbou_registered = 1;
		}
	}
	return ret;
}

/*
 * gs_disconnect
 *
 * Called when the device is disconnected.  Frees the closed
 * ports and disconnects open ports.  Open ports will be freed
 * on close.  Then reallocates the ports for the next connection.
 */
static void gs_disconnect(struct usb_gadget *gadget)
{
	unsigned long flags;
	struct gs_dev *dev = get_gadget_data(gadget);

#if defined (CONFIG_KGDBOU) || defined (CONFIG_KGDBOU_MODULE)
	if (kgdbou_registered) {
		kgdb_unregister_io_module(&kgdbgs_io_ops);
		kgdbou_registered = 0;
	}
#endif

	spin_lock_irqsave(&dev->dev_lock, flags);

	gs_reset_config(dev);

	/* free closed ports and disconnect open ports */
	/* (open ports will be freed when closed) */
	gs_free_ports(dev);

	/* re-allocate ports for the next connection */
	if (gs_alloc_ports(dev, GFP_ATOMIC) != 0)
		printk(KERN_ERR "gs_disconnect: cannot re-allocate ports\n");

	spin_unlock_irqrestore(&dev->dev_lock, flags);

	printk(KERN_INFO "gs_disconnect: %s disconnected\n", GS_LONG_NAME);
}

/*
 * kgdbgs_unbind
 *
 * Called on module unload.  Frees the control request and device
 * structure.
 */
static void /* __init_or_exit */ kgdbgs_unbind(struct usb_gadget *gadget)
{
	struct gs_dev *dev = get_gadget_data(gadget);

	gs_device = NULL;

	/* read/write requests already freed, only control request remains */
	if (dev != NULL) {
		if (dev->dev_ctrl_req != NULL) {
			gs_free_req(gadget->ep0, dev->dev_ctrl_req);
			dev->dev_ctrl_req = NULL;
		}
		gs_free_ports(dev);
		kfree(dev);
		set_gadget_data(gadget, NULL);
	}

	printk(KERN_INFO "kgdbgs_unbind: %s %s unbound\n",
	       GS_LONG_NAME, GS_VERSION_STR);
}

/*
 * kgdbgs_bind
 *
 * Called on gadget initialization.  Allocates and initializes the device
 * structure and a control request.
 */

static int __init kgdbgs_bind(struct usb_gadget *gadget)
{
	int ret;
	struct usb_ep *ep;
	struct gs_dev *dev;
	int gcnum;

	/* Some controllers can't support CDC ACM:
	 * - sh doesn't support multiple interfaces or configs;
	 * - sa1100 doesn't have a third interrupt endpoint
	 */
	if (gadget_is_sh(gadget) || gadget_is_sa1100(gadget))
		use_acm = 0;

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		gs_device_desc.bcdDevice =
				cpu_to_le16(GS_VERSION_NUM | gcnum);
	else {
		printk(KERN_WARNING "kgdbgs_bind: controller '%s' not recognized\n",
			gadget->name);
		/* unrecognized, but safe unless bulk is REALLY quirky */
		gs_device_desc.bcdDevice =
			__constant_cpu_to_le16(GS_VERSION_NUM|0x0099);
	}

	usb_ep_autoconfig_reset(gadget);

	ep = usb_ep_autoconfig(gadget, &gs_fullspeed_in_desc);
	if (!ep)
		goto autoconf_fail;
	EP_IN_NAME = ep->name;
	ep->driver_data = ep;	/* claim the endpoint */

	ep = usb_ep_autoconfig(gadget, &gs_fullspeed_out_desc);
	if (!ep)
		goto autoconf_fail;
	EP_OUT_NAME = ep->name;
	ep->driver_data = ep;	/* claim the endpoint */

	if (use_acm) {
		ep = usb_ep_autoconfig(gadget, &gs_fullspeed_notify_desc);
		if (!ep) {
			printk(KERN_ERR "kgdbgs_bind: cannot run ACM on %s\n",
			       gadget->name);
			goto autoconf_fail;
		}
		gs_device_desc.idProduct = __constant_cpu_to_le16(
						GS_CDC_PRODUCT_ID),
		EP_NOTIFY_NAME = ep->name;
		ep->driver_data = ep;	/* claim the endpoint */
	}

	gs_debug("KGDBGS endpoint names - In: %s; Out: %s; Notify: %s\n",
		 EP_IN_NAME, EP_OUT_NAME, EP_NOTIFY_NAME);

	gs_device_desc.bDeviceClass = use_acm
		? USB_CLASS_COMM : USB_CLASS_VENDOR_SPEC;
	gs_device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;

#ifdef CONFIG_USB_GADGET_DUALSPEED
	gs_qualifier_desc.bDeviceClass = use_acm
		? USB_CLASS_COMM : USB_CLASS_VENDOR_SPEC;
	/* assume ep0 uses the same packet size for both speeds */
	gs_qualifier_desc.bMaxPacketSize0 = gs_device_desc.bMaxPacketSize0;
	/* assume endpoints are dual-speed */
	gs_highspeed_notify_desc.bEndpointAddress =
		gs_fullspeed_notify_desc.bEndpointAddress;
	gs_highspeed_in_desc.bEndpointAddress =
		gs_fullspeed_in_desc.bEndpointAddress;
	gs_highspeed_out_desc.bEndpointAddress =
		gs_fullspeed_out_desc.bEndpointAddress;
#endif /* CONFIG_USB_GADGET_DUALSPEED */

	usb_gadget_set_selfpowered(gadget);

	if (gadget->is_otg) {
		gs_otg_descriptor.bmAttributes |= USB_OTG_HNP,
		gs_bulk_config_desc.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
		gs_acm_config_desc.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	gs_device = dev = kmalloc(sizeof(struct gs_dev), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	snprintf(manufacturer, sizeof(manufacturer), "%s %s with %s",
		init_utsname()->sysname, init_utsname()->release,
		gadget->name);

	memset(dev, 0, sizeof(struct gs_dev));
	dev->dev_gadget = gadget;
	spin_lock_init(&dev->dev_lock);
	INIT_LIST_HEAD(&dev->dev_req_list);
	set_gadget_data(gadget, dev);

	if ((ret=gs_alloc_ports(dev, GFP_KERNEL)) != 0) {
		printk(KERN_ERR "kgdbgs_bind: cannot allocate ports\n");
		kgdbgs_unbind(gadget);
		return ret;
	}

	/* preallocate control response and buffer */
	dev->dev_ctrl_req = gs_alloc_req(gadget->ep0, GS_MAX_DESC_LEN,
		GFP_KERNEL);
	if (dev->dev_ctrl_req == NULL) {
		kgdbgs_unbind(gadget);
		return -ENOMEM;
	}
	dev->dev_ctrl_req->complete = gs_setup_complete;

	gadget->ep0->driver_data = dev;

	printk(KERN_INFO "kgdbgs_bind: %s %s bound\n",
		GS_LONG_NAME, GS_VERSION_STR);

	return 0;

autoconf_fail:
	printk(KERN_ERR "kgdbgs_bind: cannot autoconfigure on %s\n", gadget->name);
	return -ENODEV;
}

/* Module */

/* gadget driver operations */
static struct usb_gadget_driver kgdbgs_gadget_driver = {
#ifdef CONFIG_USB_GADGET_DUALSPEED
	.speed =		USB_SPEED_HIGH,
#else
	.speed =		USB_SPEED_FULL,
#endif /* CONFIG_USB_GADGET_DUALSPEED */
	.function =		GS_LONG_NAME,
	.bind =			kgdbgs_bind,
	.unbind =		kgdbgs_unbind,
	.setup =		gs_setup,
	.disconnect =		gs_disconnect,
	.driver = {
		.name =		GS_SHORT_NAME,
	},
};

/*
 *  kgdbgs_module_init
 *
 *  Register as a USB gadget driver
 */
static int __init kgdbgs_module_init(void)
{
	int retval;

	retval = usb_gadget_register_driver(&kgdbgs_gadget_driver);
	if (retval)
		printk(KERN_ERR "kgdbgs_module_init: cannot register "
		       "gadget driver, ret=%d\n", retval);
	else
		printk(KERN_INFO "kgdbgs_module_init: %s %s loaded\n",
		       GS_LONG_NAME, GS_VERSION_STR);
	return retval;
}

/*
 * kgdbgs_module_exit
 *
 * Unregister as a USB gadget driver.
 */
static void __exit kgdbgs_module_exit(void)
{
	usb_gadget_unregister_driver(&kgdbgs_gadget_driver);

	printk(KERN_INFO "kgdbgs_module_exit: %s %s unloaded\n",
	       GS_LONG_NAME, GS_VERSION_STR);
}

MODULE_DESCRIPTION(GS_LONG_NAME);
MODULE_AUTHOR("John Mehaffey");
MODULE_LICENSE("GPL");

#ifdef GS_DEBUG
module_param(debug, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(debug, "Enable debugging, 0=off, 1=on");
#endif

#define DSTRING(x) #x
#define PARMQ(query, default) query DSTRING(default)

module_param(read_q_size, uint, S_IRUGO);
MODULE_PARM_DESC(read_q_size,
	PARMQ("Read request queue size, default=", GS_DEFAULT_READ_Q_SIZE);
	)
module_param(write_q_size, uint, S_IRUGO);
MODULE_PARM_DESC(write_q_size,
	PARMQ("Write request queue size, default=", GS_DEFAULT_WRITE_Q_SIZE));

#ifdef CONFIG_KGDBOU_MODULE
module_param(read_buf_size, uint, S_IRUGO);
MODULE_PARM_DESC(read_buf_size,
	PARMQ("Read buffer size, default=", GS_DEFAULT_READ_BUF_SIZE));
#endif

module_param(write_buf_size, uint, S_IRUGO);
MODULE_PARM_DESC(write_buf_size,
	PARMQ("Write buffer size, default=", GS_DEFAULT_WRITE_BUF_SIZE));

module_param(use_acm, uint, S_IRUGO);
MODULE_PARM_DESC(use_acm,
	PARMQ("Use CDC ACM, 0=no, 1=yes, default=", GS_DEFAULT_USE_ACM));

module_init(kgdbgs_module_init);
module_exit(kgdbgs_module_exit);

