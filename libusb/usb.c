/*
 * Main API entry point
 *
 * Copyright (c) 2000-2003 Johannes Erdfelt <johannes@erdfelt.com>
 *
 * This library is covered by the LGPL, read LICENSE for details.
 */

#include <stdlib.h>	/* getenv */
#include <stdio.h>	/* stderr */
#include <string.h>	/* strcmp */
#include <errno.h>

#include "usbi.h"
#include <pthread.h>

int usb_debug = 0;
struct usb_bus *usb_busses = NULL;

// mutex used for usb_busses - testing purpose
//pthread_mutex_t _bus_mutex;


#if 1
#define USB_BUS_BEGIN		pthread_mutex_lock(&__mutex_bus)
#define USB_BUS_END			pthread_mutex_unlock(&__mutex_bus)
#else
#define PTP_BEGIN
#define PTP_END

#endif


static int usb_device_change=0;

//Hot-swapping
usb_callback_t on_device_change = 0;

int usb_find_busses(void)
{
  struct usb_bus *busses, *bus;
  int ret, changes = 0;

  ret = usb_os_find_busses(&busses);
  if (ret < 0)
    return ret;

  /*
   * Now walk through all of the busses we know about and compare against
   * this new list. Any duplicates will be removed from the new list.
   * If we don't find it in the new list, the bus was removed. Any
   * busses still in the new list, are new to us.
   */

//USB_BUS_BEGIN;

  bus = usb_busses;
  while (bus) {
    int found = 0;
    struct usb_bus *nbus, *tbus = bus->next;

    nbus = busses;
    while (nbus) {
      struct usb_bus *tnbus = nbus->next;

      if (!strcmp(bus->dirname, nbus->dirname)) {
        /* Remove it from the new busses list */
        LIST_DEL(busses, nbus);

        usb_free_bus(nbus);
        found = 1;
        break;
      }

      nbus = tnbus;
    }

    if (!found) {
      /* The bus was removed from the system */
      LIST_DEL(usb_busses, bus);
      while(bus->devices) {
          struct usb_device* dev = bus->devices;
         // if(on_device_change) {
           //   on_device_change(dev,1);
		//DFUNC
       //   }
          bus->devices = dev->next;
          usb_free_dev(dev);
      }
      
      usb_free_bus(bus);
      changes++;
    }

    bus = tbus;
  }

  /*
   * Anything on the *busses list is new. So add them to usb_busses and
   * process them like the new bus it is.
   */
  bus = busses;
  while (bus) {
    struct usb_bus *tbus = bus->next;

    /*
     * Remove it from the temporary list first and add it to the real
     * usb_busses list.
     */
    LIST_DEL(busses, bus);

    LIST_ADD(usb_busses, bus);

    changes++;

    bus = tbus;
  }
//USB_BUS_END;
  return changes;
}

int usb_find_devices(void)
{
  struct usb_bus *bus;
  int ret, changes = 0;
//USB_BUS_BEGIN;
  for (bus = usb_busses; bus; bus = bus->next) {
    struct usb_device *devices, *dev;

    /* Find all of the devices and put them into a temporary list */
    ret = usb_os_find_devices(bus, &devices);
	
    if (ret < 0){//USB_BUS_END;
      return ret;}

    /*
     * Now walk through all of the devices we know about and compare
     * against this new list. Any duplicates will be removed from the new
     * list. If we don't find it in the new list, the device was removed.
     * Any devices still in the new list, are new to us.
     */
    dev = bus->devices;
    while (dev) {
      int found = 0;
      struct usb_device *ndev, *tdev = dev->next;

      ndev = devices;
      while (ndev) {
        struct usb_device *tndev = ndev->next;

        if (!strcmp(dev->filename, ndev->filename)) {
          /* Remove it from the new devices list */
          LIST_DEL(devices, ndev);

          usb_free_dev(ndev);
		
          found = 1;
          break;
        }

        ndev = tndev;
      }

      if (!found) {
        /* The device was removed from the system */
         LIST_DEL(bus->devices, dev);
		if (dev)
		{
			//DFUNC
				usb_free_dev(dev);
		}
		//DFUNC
        changes++;
      }

      dev = tdev;
    }

    /*
     * Anything on the *devices list is new. So add them to bus->devices and
     * process them like the new device it is.
     */
    dev = devices;
    while (dev) {
      struct usb_device *tdev = dev->next;

      /*
       * Remove it from the temporary list first and add it to the real
       * bus->devices list.
       */
      LIST_DEL(devices, dev);

      LIST_ADD(bus->devices, dev);

      /*
       * Some ports fetch the descriptors on scanning (like Linux) so we don't
       * need to fetch them again.
       */
      if (!dev->config) {
        usb_dev_handle *udev;

        udev = usb_open(dev);
        if (udev) {
          usb_fetch_and_parse_descriptors(udev);

          usb_close(udev);
        }
      }

      changes++;
      dev = tdev;
    }

    usb_os_determine_children(bus);

  }
//USB_BUS_END;
  return changes;
}

void usb_set_debug(int level)
{
  if (usb_debug || level)
    fprintf(stderr, "usb_set_debug: Setting debugging level to %d (%s)\n",
	level, level ? "on" : "off");

  usb_debug = level;
}

void usb_init(void)
{
#if 0
  if (getenv("USB_DEBUG"))
    usb_set_debug(atoi(getenv("USB_DEBUG")));
#endif
  usb_os_init();
}

usb_dev_handle *usb_open(struct usb_device *dev)
{
  usb_dev_handle *udev;

  udev = malloc(sizeof(*udev));
  if (!udev)
    return NULL;

  udev->fd = -1;
  udev->device = dev;
  udev->bus = dev->bus;
  udev->config = udev->interface = udev->altsetting = -1;

  if (usb_os_open(udev) < 0) {
    free(udev);
    return NULL;
  }

  return udev;
}

int usb_get_string(usb_dev_handle *dev, int indexL, int langid, char *buf,
	size_t buflen)
{
  /*
   * We can't use usb_get_descriptor() because it's lacking the index
   * parameter. This will be fixed in libusb 1.0
   */
  return usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
			(USB_DT_STRING << 8) + indexL, langid, buf, (int)buflen, 1000);//check
}

int usb_get_string_simple(usb_dev_handle *dev, int indexL, char *buf, size_t buflen)
{
  char tbuf[255];	/* Some devices choke on size > 255 */
  int ret, langid, si, di;

  /*
   * Asking for the zero'th index is special - it returns a string
   * descriptor that contains all the language IDs supported by the
   * device. Typically there aren't many - often only one. The
   * language IDs are 16 bit numbers, and they start at the third byte
   * in the descriptor. See USB 2.0 specification, section 9.6.7, for
   * more information on this. */
  ret = usb_get_string(dev, 0, 0, tbuf, sizeof(tbuf));
  if (ret < 0)
    return ret;

  if (ret < 4)
    return -EIO;

  langid = tbuf[2] | (tbuf[3] << 8);

  ret = usb_get_string(dev, indexL, langid, tbuf, sizeof(tbuf));
  if (ret < 0)
    return ret;

  if (tbuf[1] != USB_DT_STRING)
    return -EIO;

  if (tbuf[0] > ret)
    return -EFBIG;

  for (di = 0, si = 2; si < tbuf[0]; si += 2) {
    if (di >= ((int)buflen - 1))//check
      break;

    if (tbuf[si + 1])	/* high byte */
      buf[di++] = '?';
    else
      buf[di++] = tbuf[si];
  }

  buf[di] = 0;

  return di;
}

int usb_close(usb_dev_handle *dev)
{
  int ret;

  ret = usb_os_close(dev);
  free(dev);

  return ret;
}

//org : struct usb_device *usb_device(usb_dev_handle *dev)
struct usb_device *usb_get_device(usb_dev_handle *dev)
{
  return dev->device;
}

void usb_free_dev(struct usb_device *dev)
{

  usb_destroy_configuration(dev);
  free(dev->children);
  free(dev);

}

struct usb_bus *usb_get_busses(void)
{
  return usb_busses;
}

void usb_free_bus(struct usb_bus *bus)
{
  free(bus);
}

int usb_find_devices_mptp(void)
{
  struct usb_bus *bus;
  int ret, changes = 0;
//USB_BUS_BEGIN;
  for (bus = usb_busses; bus; bus = bus->next) {
    struct usb_device *devices, *dev;

    /* Find all of the devices and put them into a temporary list */
    ret = usb_os_find_devices(bus, &devices);
	
    if (ret < 0){//USB_BUS_END;
      return ret;}

    /*
     * Now walk through all of the devices we know about and compare
     * against this new list. Any duplicates will be removed from the new
     * list. If we don't find it in the new list, the device was removed.
     * Any devices still in the new list, are new to us.
     */
    dev = bus->devices;
    while (dev) {
      int found = 0;
      struct usb_device *ndev, *tdev = dev->next;

      ndev = devices;
      while (ndev) {
        struct usb_device *tndev = ndev->next;

        if (!strcmp(dev->filename, ndev->filename)) {
          /* Remove it from the new devices list */
          LIST_DEL(devices, ndev);

          usb_free_dev(ndev);
		
          found = 1;
          break;
        }

        ndev = tndev;
      }

      if (!found) {
        /* The device was removed from the system */
        if(on_device_change) {
            on_device_change(dev,1);
			usb_device_change = 1;		
		//printf ("\n\t*************************DEVICE CHANGE - REMOVED*********************************\n");
// 		DFUNC
        }
        LIST_DEL(bus->devices, dev);
	if (dev)
	{
// 		DFUNC
	        usb_free_dev(dev);
	}
 	//DFUNC
        changes++;
    	//printf("\nThe Device was removed from the system\n");//vishal
      }

      dev = tdev;
    }

    /*
     * Anything on the *devices list is new. So add them to bus->devices and
     * process them like the new device it is.
     */
    dev = devices;
    while (dev) {
      struct usb_device *tdev = dev->next;

      /*
       * Remove it from the temporary list first and add it to the real
       * bus->devices list.
       */
      LIST_DEL(devices, dev);

      LIST_ADD(bus->devices, dev);

      /*
       * Some ports fetch the descriptors on scanning (like Linux) so we don't
       * need to fetch them again.
       */
      if (!dev->config) {
        usb_dev_handle *udev;

        udev = usb_open(dev);
        if (udev) {
          usb_fetch_and_parse_descriptors(udev);

          usb_close(udev);
        }
      }

      changes++;
       
        
        if(on_device_change) {
            on_device_change(dev,0);
			usb_device_change = 1;
		//printf ("\n\t*************************DEVICE CHANGE - ADDED*********************************\n");
// 			DFUNC
        }

        dev = tdev;
    }

    usb_os_determine_children(bus);

  }
//USB_BUS_END;
  return changes;
}
/**
Function Name : usb_poll_for_change

Function Description :
This function can be used  by polling thread to find any change in number of devcies.

Input : void

Output : number of changes
	
*/
#if 1
int usb_poll_for_change(int *device_change)
{
    int changes = 0;

    usb_init();
	usb_device_change = 0;
    changes = usb_find_busses();
    changes += usb_find_devices_mptp();
	*device_change = usb_device_change;	

    return changes;
}
#endif



/**
Function Name : usb_set_callback

Function Description :
This function sets the callback if any change is there in count of connected devices.

Input : callback, callback routine to be invoked

Output : error code
	-1, for any error
	0,	for success
*/
int usb_set_callback(usb_callback_t callback)
{
	on_device_change = callback;

	return 0;
}


/**
Function Name : usb_get_device_list

Function Description :
This function returns the connected USB devices list.

Input : list_length, number of the connected usb devices.
	: device_list, list of usb_device

Output : error code
	-1, for any error
	0,	for success
*/
int usb_get_device_list(int* list_length, device_ptr_t** device_list)
{
  struct usb_bus *bus;
  struct usb_device **dev_list;

  //int ret;
  int device_count = 0;

  *list_length = 0;
  
  /* Count all of the devices */
  for (bus = usb_busses; bus; bus = bus->next) {
    struct usb_device *dev;

    dev = bus->devices;
    while (dev) {
	    device_count++;
	    dev = dev->next;
    }
  }
  
  if(device_count == 0) {
	  printf("ERROR: libusb: device count == NULL\n");
	  return -1; //todo correct errorcode
  }	
  
  /* Allocate memory for device list */
  dev_list = malloc(sizeof(struct usb_device*) * device_count);

  if(dev_list == 0) {
	  printf("ERROR: libusb: memory allocation failed\n");		
	  return -1; //todo correct errorcode
  }	

  /* Update device list */
  device_count = 0;
  for (bus = usb_busses; bus; bus = bus->next) {
    struct usb_device *dev;

    dev = bus->devices;
    while (dev) {
	    dev_list[device_count] = dev;
	    device_count++;
	    dev = dev->next;
    }
  }
  *list_length = device_count;
  *device_list = dev_list;
   printf("SUCCESS: libusb: device count = %d\n",device_count);		
 
  return 0;  	
}



