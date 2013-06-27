/*
 * RapidIO interconnect services
 * (RapidIO Interconnect Specification, http://www.rapidio.org)
 *
 * Copyright 2005 MontaVista Software, Inc.
 * Matt Porter <mporter@kernel.crashing.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef LINUX_RIO_H
#define LINUX_RIO_H

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/rio_regs.h>

#define RIO_ANY_DESTID		0xff
#define RIO_NO_HOPCOUNT		-1
#define RIO_INVALID_DESTID	0xffff

#define RIO_MAX_MPORT_RESOURCES	16
#define RIO_MAX_DEV_RESOURCES	16

#define RIO_GLOBAL_TABLE	0xff	/* Indicates access of a switch's
					   global routing table if it
					   has multiple (or per port)
					   tables */

#define RIO_INVALID_ROUTE	0xff	/* Indicates that a route table
					   entry is invalid (no route
					   exists for the device ID) */

#ifdef CONFIG_RAPIDIO_8_BIT_TRANSPORT
#define RIO_MAX_ROUTE_ENTRIES	(1 << 8)
#else
#define RIO_MAX_ROUTE_ENTRIES	(1 << 16)
#endif

#define RIO_MAX_MBOX		4
#define RIO_MAX_MSG_SIZE	0x1000

/*
 * Error values that may be returned by RIO functions.
 */
#define RIO_SUCCESSFUL			0x00
#define RIO_BAD_SIZE			0x81

/*
 * For RIO devices, the region numbers are assigned this way:
 *
 *	0	RapidIO outbound doorbells
 *      1-15	RapidIO memory regions
 *
 * For RIO master ports, the region number are assigned this way:
 *
 *	0	RapidIO inbound doorbells
 *	1	RapidIO inbound mailboxes
 *	1	RapidIO outbound mailboxes
 */
#define RIO_DOORBELL_RESOURCE	0
#define RIO_INB_MBOX_RESOURCE	1
#define RIO_OUTB_MBOX_RESOURCE	2

extern struct bus_type rio_bus_type;
extern struct list_head rio_devices;	/* list of all devices */

struct rio_mport;

/**
 * struct rio_dev - RIO device info
 * @global_list: Node in list of all RIO devices
 * @net_list: Node in list of RIO devices in a network
 * @net: Network this device is a part of
 * @did: Device ID
 * @vid: Vendor ID
 * @device_rev: Device revision
 * @asm_did: Assembly device ID
 * @asm_vid: Assembly vendor ID
 * @asm_rev: Assembly revision
 * @efptr: Extended feature pointer
 * @pef: Processing element features
 * @swpinfo: Switch port info
 * @src_ops: Source operation capabilities
 * @dst_ops: Destination operation capabilities
 * @dma_mask: Mask of bits of RIO address this device implements
 * @rswitch: Pointer to &struct rio_switch if valid for this device
 * @driver: Driver claiming this device
 * @dev: Device model device
 * @riores: RIO resources this device owns
 * @destid: Network destination ID
 */
struct rio_dev {
	struct list_head global_list;	/* node in list of all RIO devices */
	struct list_head net_list;	/* node in per net list */
	struct rio_net *net;	/* RIO net this device resides in */
	u16 did;
	u16 vid;
	u32 device_rev;
	u16 asm_did;
	u16 asm_vid;
	u16 asm_rev;
	u16 efptr;
	u32 pef;
	u32 swpinfo;		/* Only used for switches */
	u32 src_ops;
	u32 dst_ops;
	u64 dma_mask;
	struct rio_switch *rswitch;	/* RIO switch info */
	struct rio_driver *driver;	/* RIO driver claiming this device */
	struct device dev;	/* LDM device structure */
	struct resource riores[RIO_MAX_DEV_RESOURCES];
	u16 destid;
};

#define rio_dev_g(n) list_entry(n, struct rio_dev, global_list)
#define rio_dev_f(n) list_entry(n, struct rio_dev, net_list)
#define	to_rio_dev(n) container_of(n, struct rio_dev, dev)

/**
 * struct rio_msg - RIO message event
 * @res: Mailbox resource
 * @mcback: Message event callback
 */
struct rio_msg {
	struct resource *res;
	void (*mcback) (struct rio_mport * mport, void *dev_id, int mbox, int slot);
};

/**
 * struct rio_dbell - RIO doorbell event
 * @node: Node in list of doorbell events
 * @res: Doorbell resource
 * @dinb: Doorbell event callback
 * @dev_id: Device specific pointer to pass on event
 */
struct rio_dbell {
	struct list_head node;
	struct resource *res;
	void (*dinb) (struct rio_mport *mport, void *dev_id, u16 src, u16 dst, u16 info);
	void *dev_id;
};

/**
 * struct rio_mport - RIO master port info
 * @dbells: List of doorbell events
 * @node: Node in global list of master ports
 * @nnode: Node in network list of master ports
 * @iores: I/O mem resource that this master port interface owns
 * @riores: RIO resources that this master port interfaces owns
 * @inb_msg: RIO inbound message event descriptors
 * @outb_msg: RIO outbound message event descriptors
 * @host_deviceid: Host device ID associated with this master port
 * @ops: configuration space functions
 * @id: Port ID, unique among all ports
 * @index: Port index, unique among all port interfaces of the same type
 * @name: Port name string
 */
struct rio_mport {
	struct list_head dbells;	/* list of doorbell events */
	struct list_head node;	/* node in global list of ports */
	struct list_head nnode;	/* node in net list of ports */
	struct resource iores;
	struct resource riores[RIO_MAX_MPORT_RESOURCES];
	struct rio_msg inb_msg[RIO_MAX_MBOX];
	struct rio_msg outb_msg[RIO_MAX_MBOX];
	int host_deviceid;	/* Host device ID */
	struct rio_ops *ops;	/* maintenance transaction functions */
	unsigned char id;	/* port ID, unique among all ports */
	unsigned char index;	/* port index, unique among all port
				   interfaces of the same type */
	unsigned char name[40];
};

/**
 * struct rio_net - RIO network info
 * @node: Node in global list of RIO networks
 * @devices: List of devices in this network
 * @mports: List of master ports accessing this network
 * @hport: Default port for accessing this network
 * @id: RIO network ID
 */
struct rio_net {
	struct list_head node;	/* node in list of networks */
	struct list_head devices;	/* list of devices in this net */
	struct list_head mports;	/* list of ports accessing net */
	struct rio_mport *hport;	/* primary port for accessing net */
	unsigned char id;	/* RIO network ID */
};

/**
 * struct rio_switch - RIO switch info
 * @node: Node in global list of switches
 * @switchid: Switch ID that is unique across a network
 * @hopcount: Hopcount to this switch
 * @destid: Associated destid in the path
 * @route_table: Copy of switch routing table
 * @add_entry: Callback for switch-specific route add function
 * @get_entry: Callback for switch-specific route get function
 */
struct rio_switch {
	struct list_head node;
	u16 switchid;
	u16 hopcount;
	u16 destid;
	u8 route_table[RIO_MAX_ROUTE_ENTRIES];
	int (*add_entry) (struct rio_mport * mport, u16 destid, u8 hopcount,
			  u16 table, u16 route_destid, u8 route_port);
	int (*get_entry) (struct rio_mport * mport, u16 destid, u8 hopcount,
			  u16 table, u16 route_destid, u8 * route_port);
};

/* Low-level architecture-dependent routines */

/**
 * struct rio_ops - Low-level RIO configuration space operations
 * @lcread: Callback to perform local (master port) read of config space.
 * @lcwrite: Callback to perform local (master port) write of config space.
 * @cread: Callback to perform network read of config space.
 * @cwrite: Callback to perform network write of config space.
 * @dsend: Callback to send a doorbell message.
 */
struct rio_ops {
	int (*lcread) (int index, u32 offset, int len, u32 * data);
	int (*lcwrite) (int index, u32 offset, int len, u32 data);
	int (*cread) (int index, u16 destid, u8 hopcount, u32 offset, int len,
		      u32 * data);
	int (*cwrite) (int index, u16 destid, u8 hopcount, u32 offset, int len,
		       u32 data);
	int (*dsend) (int index, u16 destid, u16 data);
};

#define RIO_RESOURCE_MEM	0x00000100
#define RIO_RESOURCE_DOORBELL	0x00000200
#define RIO_RESOURCE_MAILBOX	0x00000400

#define RIO_RESOURCE_CACHEABLE	0x00010000
#define RIO_RESOURCE_PCI	0x00020000

#define RIO_RESOURCE_BUSY	0x80000000

/**
 * struct rio_driver - RIO driver info
 * @node: Node in list of drivers
 * @name: RIO driver name
 * @id_table: RIO device ids to be associated with this driver
 * @probe: RIO device inserted
 * @remove: RIO device removed
 * @suspend: RIO device suspended
 * @resume: RIO device awakened
 * @enable_wake: RIO device enable wake event
 * @driver: LDM driver struct
 *
 * Provides info on a RIO device driver for insertion/removal and
 * power management purposes.
 */
struct rio_driver {
	struct list_head node;
	char *name;
	const struct rio_device_id *id_table;
	int (*probe) (struct rio_dev * dev, const struct rio_device_id * id);
	void (*remove) (struct rio_dev * dev);
	int (*suspend) (struct rio_dev * dev, u32 state);
	int (*resume) (struct rio_dev * dev);
	int (*enable_wake) (struct rio_dev * dev, u32 state, int enable);
	struct device_driver driver;
};

#define	to_rio_driver(drv) container_of(drv,struct rio_driver, driver)

/**
 * struct rio_device_id - RIO device identifier
 * @did: RIO device ID
 * @vid: RIO vendor ID
 * @asm_did: RIO assembly device ID
 * @asm_vid: RIO assembly vendor ID
 *
 * Identifies a RIO device based on both the device/vendor IDs and
 * the assembly device/vendor IDs.
 */
struct rio_device_id {
	u16 did, vid;
	u16 asm_did, asm_vid;
};

/**
 * struct rio_route_ops - Per-switch route operations
 * @vid: RIO vendor ID
 * @did: RIO device ID
 * @add_hook: Callback that adds a route entry
 * @get_hook: Callback that gets a route entry
 *
 * Defines the operations that are necessary to manipulate the route
 * tables for a particular RIO switch device.
 */
struct rio_route_ops {
	u16 vid, did;
	int (*add_hook) (struct rio_mport * mport, u16 destid, u8 hopcount,
			 u16 table, u16 route_destid, u8 route_port);
	int (*get_hook) (struct rio_mport * mport, u16 destid, u8 hopcount,
			 u16 table, u16 route_destid, u8 * route_port);
};

/* Architecture and hardware-specific functions */
extern int rio_init_mports(void);
extern void rio_register_mport(struct rio_mport *);
extern int rio_hw_add_outb_message(struct rio_mport *, struct rio_dev *, int,
				   void *, size_t);
extern int rio_hw_add_inb_buffer(struct rio_mport *, int, void *);
extern void *rio_hw_get_inb_message(struct rio_mport *, int);
extern int rio_open_inb_mbox(struct rio_mport *, void *, int, int);
extern void rio_close_inb_mbox(struct rio_mport *, int);
extern int rio_open_outb_mbox(struct rio_mport *, void *, int, int);
extern void rio_close_outb_mbox(struct rio_mport *, int);

#endif				/* __KERNEL__ */
#endif				/* LINUX_RIO_H */
