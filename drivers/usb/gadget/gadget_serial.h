/* gadget_serial.h
 * include file for gadget serial drivers
 */

#define GS_MAJOR			127
#define GS_MINOR_START			0

#define GS_NUM_CONFIGS			1
#define GS_NO_CONFIG_ID			0
#define GS_BULK_CONFIG_ID		1
#define GS_ACM_CONFIG_ID		2

#define GS_MAX_NUM_INTERFACES		2
#define GS_BULK_INTERFACE_ID		0
#define GS_CONTROL_INTERFACE_ID		0
#define GS_DATA_INTERFACE_ID		1

#define GS_MAX_DESC_LEN			256

#define GS_CLOSE_TIMEOUT		15

#define GS_DEFAULT_DTE_RATE		9600
#define GS_DEFAULT_DATA_BITS		8
#define GS_DEFAULT_PARITY		USB_CDC_NO_PARITY
#define GS_DEFAULT_CHAR_FORMAT		USB_CDC_1_STOP_BITS

/* select highspeed/fullspeed, hiding highspeed if not configured */
#ifdef CONFIG_USB_GADGET_DUALSPEED
#define GS_SPEED_SELECT(is_hs,hs,fs) ((is_hs) ? (hs) : (fs))
#else
#define GS_SPEED_SELECT(is_hs,hs,fs) (fs)
#endif /* CONFIG_USB_GADGET_DUALSPEED */

/* debug settings */
#ifdef GS_DEBUG
static int debug = GS_DEBUG;

#define gs_debug(format, arg...) \
	do { if (debug) printk(KERN_DEBUG format, ## arg); } while(0)
#define gs_debug_level(level, format, arg...) \
	do { if (debug>=level) printk(KERN_DEBUG format, ## arg); } while(0)

#else

#define gs_debug(format, arg...) \
	do { } while(0)
#define gs_debug_level(level, format, arg...) \
	do { } while(0)

#endif /* GS_DEBUG */

/* Thanks to NetChip Technologies for donating this product ID.
 *
 * DO NOT REUSE THESE IDs with a protocol-incompatible driver!!  Ever!!
 * Instead:  allocate your own, using normal USB-IF procedures.
 */
#define GS_VENDOR_ID			0x0525	/* NetChip */
#define GS_PRODUCT_ID			0xa4a6	/* Linux-USB Serial Gadget */
#define GS_CDC_PRODUCT_ID		0xa4a7	/* ... as CDC-ACM */

#define GS_LOG2_NOTIFY_INTERVAL		5	/* 1 << 5 == 32 msec */
#define GS_NOTIFY_MAXPACKET		8


/* Structures */

struct gs_dev;

/* buffer structure.  For KGDB, use atomic operations to avoid the need
 * for locks.  For each buffer (in/out), there is only one producer and
 * one consumer, so only count needs to be atomic.
 */
#if defined (CONFIG_KGDBOU) || defined (CONFIG_KGDBOU_MODULE)
struct kgdbgs_buf {
	int			size;
	char			*buffer;
	int			head;
	int			tail;
	atomic_t		count;
};

#else /* !CONFIG_KGDBOU */

/* circular buffer */
struct gs_buf {
	unsigned int		buf_size;
	char			*buf_buf;
	char			*buf_get;
	char			*buf_put;
};

#endif /* CONFIG_KGDBOU */

/* list of write requests
 * for KGDB, you can't use list insert/delete operations, so
 * add an in-use marker
 */
struct gs_req_entry {
	struct list_head	re_entry;
	struct usb_request	*re_req;
#if defined (CONFIG_KGDBOU) || defined (CONFIG_KGDBOU_MODULE)
	int			in_use; /* 0=free, 1=in-use */
#endif
};

/* the port structure holds info for each port, one for each minor number */
struct gs_port {
	struct gs_dev 		*port_dev;	/* pointer to device struct */
	spinlock_t		port_lock;
#if defined (CONFIG_KGDBOU) || defined (CONFIG_KGDBOU_MODULE)
	struct kgdbgs_buf	in_buf;
	struct kgdbgs_buf	out_buf;
#else
	struct gs_buf		*port_write_buf;
	struct tty_struct	*port_tty;	/* pointer to tty struct */
#endif
	int 			port_num;
	int			port_open_count;
	int			port_in_use;	/* open/close in progress */
	wait_queue_head_t	port_write_wait;/* waiting to write */
	struct usb_cdc_line_coding	port_line_coding;
};

/* the device structure holds info for the USB device */
struct gs_dev {
	struct usb_gadget	*dev_gadget;	/* gadget device pointer */
	spinlock_t		dev_lock;	/* lock for set/reset config */
	int			dev_config;	/* configuration number */
	struct usb_ep		*dev_notify_ep;	/* address of notify endpoint */
	struct usb_ep		*dev_in_ep;	/* address of in endpoint */
	struct usb_ep		*dev_out_ep;	/* address of out endpoint */
	struct usb_endpoint_descriptor		/* descriptor of notify ep */
				*dev_notify_ep_desc;
	struct usb_endpoint_descriptor		/* descriptor of in endpoint */
				*dev_in_ep_desc;
	struct usb_endpoint_descriptor		/* descriptor of out endpoint */
				*dev_out_ep_desc;
	struct usb_request	*dev_ctrl_req;	/* control request */
	struct list_head	dev_req_list;	/* list of write requests */
	int			dev_sched_port;	/* round robin port scheduled */
	struct gs_port		*dev_port[GS_NUM_PORTS]; /* the ports */
};

/* USB descriptors */

#define GS_MANUFACTURER_STR_ID	1
#define GS_PRODUCT_STR_ID	2
#define GS_SERIAL_STR_ID	3
#define GS_BULK_CONFIG_STR_ID	4
#define GS_ACM_CONFIG_STR_ID	5
#define GS_CONTROL_STR_ID	6
#define GS_DATA_STR_ID		7

/* static strings, in UTF-8 */
static char manufacturer[50];
static struct usb_string gs_strings[] = {
	{ GS_MANUFACTURER_STR_ID, manufacturer },
	{ GS_PRODUCT_STR_ID, GS_LONG_NAME },
	{ GS_SERIAL_STR_ID, "0" },
	{ GS_BULK_CONFIG_STR_ID, "Gadget Serial Bulk" },
	{ GS_ACM_CONFIG_STR_ID, "Gadget Serial CDC ACM" },
	{ GS_CONTROL_STR_ID, "Gadget Serial Control" },
	{ GS_DATA_STR_ID, "Gadget Serial Data" },
	{  } /* end of list */
};

static struct usb_gadget_strings gs_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		gs_strings,
};

static struct usb_device_descriptor gs_device_desc = {
	.bLength =		USB_DT_DEVICE_SIZE,
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		__constant_cpu_to_le16(0x0200),
	.bDeviceSubClass =	0,
	.bDeviceProtocol =	0,
	.idVendor =		__constant_cpu_to_le16(GS_VENDOR_ID),
	.idProduct =		__constant_cpu_to_le16(GS_PRODUCT_ID),
	.iManufacturer =	GS_MANUFACTURER_STR_ID,
	.iProduct =		GS_PRODUCT_STR_ID,
	.iSerialNumber =	GS_SERIAL_STR_ID,
	.bNumConfigurations =	GS_NUM_CONFIGS,
};

static struct usb_otg_descriptor gs_otg_descriptor = {
	.bLength =		sizeof(gs_otg_descriptor),
	.bDescriptorType =	USB_DT_OTG,
	.bmAttributes =		USB_OTG_SRP,
};

static struct usb_config_descriptor gs_bulk_config_desc = {
	.bLength =		USB_DT_CONFIG_SIZE,
	.bDescriptorType =	USB_DT_CONFIG,
	/* .wTotalLength computed dynamically */
	.bNumInterfaces =	1,
	.bConfigurationValue =	GS_BULK_CONFIG_ID,
	.iConfiguration =	GS_BULK_CONFIG_STR_ID,
	.bmAttributes =		USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower =		1,
};

static struct usb_config_descriptor gs_acm_config_desc = {
	.bLength =		USB_DT_CONFIG_SIZE,
	.bDescriptorType =	USB_DT_CONFIG,
	/* .wTotalLength computed dynamically */
	.bNumInterfaces =	2,
	.bConfigurationValue =	GS_ACM_CONFIG_ID,
	.iConfiguration =	GS_ACM_CONFIG_STR_ID,
	.bmAttributes =		USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower =		1,
};

static const struct usb_interface_descriptor gs_bulk_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bInterfaceNumber =	GS_BULK_INTERFACE_ID,
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	.iInterface =		GS_DATA_STR_ID,
};

static const struct usb_interface_descriptor gs_control_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bInterfaceNumber =	GS_CONTROL_INTERFACE_ID,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_COMM,
	.bInterfaceSubClass =	USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol =	USB_CDC_ACM_PROTO_AT_V25TER,
	.iInterface =		GS_CONTROL_STR_ID,
};

static const struct usb_interface_descriptor gs_data_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bInterfaceNumber =	GS_DATA_INTERFACE_ID,
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	.iInterface =		GS_DATA_STR_ID,
};

static const struct usb_cdc_header_desc gs_header_desc = {
	.bLength =		sizeof(gs_header_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_HEADER_TYPE,
	.bcdCDC =		__constant_cpu_to_le16(0x0110),
};

static const struct usb_cdc_call_mgmt_descriptor gs_call_mgmt_descriptor = {
	.bLength =  		sizeof(gs_call_mgmt_descriptor),
	.bDescriptorType = 	USB_DT_CS_INTERFACE,
	.bDescriptorSubType = 	USB_CDC_CALL_MANAGEMENT_TYPE,
	.bmCapabilities = 	0,
	.bDataInterface = 	1,	/* index of data interface */
};

static struct usb_cdc_acm_descriptor gs_acm_descriptor = {
	.bLength =  		sizeof(gs_acm_descriptor),
	.bDescriptorType = 	USB_DT_CS_INTERFACE,
	.bDescriptorSubType = 	USB_CDC_ACM_TYPE,
	.bmCapabilities = 	0,
};

static const struct usb_cdc_union_desc gs_union_desc = {
	.bLength =		sizeof(gs_union_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_UNION_TYPE,
	.bMasterInterface0 =	0,	/* index of control interface */
	.bSlaveInterface0 =	1,	/* index of data interface */
};

static struct usb_endpoint_descriptor gs_fullspeed_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =		1 << GS_LOG2_NOTIFY_INTERVAL,
};

static struct usb_endpoint_descriptor gs_fullspeed_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor gs_fullspeed_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static const struct usb_descriptor_header *gs_bulk_fullspeed_function[] = {
	(struct usb_descriptor_header *) &gs_otg_descriptor,
	(struct usb_descriptor_header *) &gs_bulk_interface_desc,
	(struct usb_descriptor_header *) &gs_fullspeed_in_desc,
	(struct usb_descriptor_header *) &gs_fullspeed_out_desc,
	NULL,
};

static const struct usb_descriptor_header *gs_acm_fullspeed_function[] = {
	(struct usb_descriptor_header *) &gs_otg_descriptor,
	(struct usb_descriptor_header *) &gs_control_interface_desc,
	(struct usb_descriptor_header *) &gs_header_desc,
	(struct usb_descriptor_header *) &gs_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &gs_acm_descriptor,
	(struct usb_descriptor_header *) &gs_union_desc,
	(struct usb_descriptor_header *) &gs_fullspeed_notify_desc,
	(struct usb_descriptor_header *) &gs_data_interface_desc,
	(struct usb_descriptor_header *) &gs_fullspeed_in_desc,
	(struct usb_descriptor_header *) &gs_fullspeed_out_desc,
	NULL,
};

#ifdef CONFIG_USB_GADGET_DUALSPEED
static struct usb_endpoint_descriptor gs_highspeed_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =		GS_LOG2_NOTIFY_INTERVAL+4,
};

static struct usb_endpoint_descriptor gs_highspeed_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor gs_highspeed_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_qualifier_descriptor gs_qualifier_desc = {
	.bLength =		sizeof(struct usb_qualifier_descriptor),
	.bDescriptorType =	USB_DT_DEVICE_QUALIFIER,
	.bcdUSB =		__constant_cpu_to_le16 (0x0200),
	/* assumes ep0 uses the same value for both speeds ... */
	.bNumConfigurations =	GS_NUM_CONFIGS,
};

static const struct usb_descriptor_header *gs_bulk_highspeed_function[] = {
	(struct usb_descriptor_header *) &gs_otg_descriptor,
	(struct usb_descriptor_header *) &gs_bulk_interface_desc,
	(struct usb_descriptor_header *) &gs_highspeed_in_desc,
	(struct usb_descriptor_header *) &gs_highspeed_out_desc,
	NULL,
};

static const struct usb_descriptor_header *gs_acm_highspeed_function[] = {
	(struct usb_descriptor_header *) &gs_otg_descriptor,
	(struct usb_descriptor_header *) &gs_control_interface_desc,
	(struct usb_descriptor_header *) &gs_header_desc,
	(struct usb_descriptor_header *) &gs_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &gs_acm_descriptor,
	(struct usb_descriptor_header *) &gs_union_desc,
	(struct usb_descriptor_header *) &gs_highspeed_notify_desc,
	(struct usb_descriptor_header *) &gs_data_interface_desc,
	(struct usb_descriptor_header *) &gs_highspeed_in_desc,
	(struct usb_descriptor_header *) &gs_highspeed_out_desc,
	NULL,
};

#endif /* CONFIG_USB_GADGET_DUALSPEED */


