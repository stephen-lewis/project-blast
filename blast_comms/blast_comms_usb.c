/**
 * blast_comms_usb.c
 *
 * USB Functions
 *
 * Cubesat Communications Uplink/Downlink Driver
 * Use with Linux Kernel >=3.0 [http://www.kernel.org/]
 * Designed for Google Android [http://android.google.com/]
 *
 * Project BLAST [http://www.projectblast.co.uk]
 * University of Southampton
 * Copyright (c) 2012,2013
 * Licensed under GPLv2
 *
 * Written by Stephen Lewis (stfl1g09@soton.ac.uk)
 * Last edited on 4/12/2012
 */

/*
 * Linux inclusions
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb.h>

/*
 * Local inclusions
 */
#include "blast_comms_usb.h"

/**
 * USB Device Table
 */
static const struct usb_device_id blast_comms_id_table[] = {
	{ USB_DEVICE(USB_VENDOR_ID_BLASTCOMMS, USB_DEVICE_ID_BLASTCOMMS) },
	{ }	/* null terminator */
};

/*
 * The usb_device Structure (Declared Later)
 */
static struct usb_driver blast_comms_usb_dev;

/**
 * blast_comms_usb_probe - add USB device to the system
 * @interface: the interface added
 * @id: the USB device id
 */
static int blast_comms_usb_probe(struct usb_interface *interface,
						const struct usb_device_id *id)
{
	struct blast_comms_dev *dev;		/* the device */
	struct usb_interface *usb_if;		/* a USB interface */
	struct usb_host_endpoint *ep_in;	/* USB endpoint (input) */
	struct usb_host_endpoint *ep_out;	/* USB endpoint (output) */
	int result = 0;		/* return code */

	/* Allocate and zero the device structure, fail gracefully. */
	dev = kzalloc(sizeof(struct blast_comms_dev), GFP_KERNEL);
	if (!dev) {
		kprint(KERN_WARNING "%s: out of memory.\n", DRIVER_NAME);
		return -ENOMEM;
	}

	/* Get the (control) interface. */
	dev->usb_dev = usb_get_dev(interface_to_usbdev(interface));
	dev->usb_ctl_if = interface;

	/* Only accept a probe on the control interface. */
	usb_if = usb_ifnum_to_if(dev->usb_dev, 0);

	if (usb_if != interface) {
		result = -ENODEV;
		kprint(KERN_WARNING "%s: USB control interface not found.\n",
								DRIVER_NAME);
		goto probe_failure_free;
	}

	/* Get the data interface */
	usb_if = usb_ifnum_to_if(dev->usb_dev, 1);

	if (!usb_if) {
		result = -ENODEV;
		kprint(KERN_WARNING "%s: USB data interface not found.\n",
								DRIVER_NAME);
		goto probe_failure_free;
	}

	/* Check if interface is already claimed. */
	if (usb_interface_claimed(usb_if)) {
		result = -EBUSY;
		kprint(KERN_WARNING "%s: USB data interface busy.\n",
								DRIVER_NAME);
		goto probe_failure_free;
	}

	/* Setup endpoints. */
	if (usb_if->cur_altsetting->desc.bNumEndpoints < 2) {
		result = -EINVAL;
		kprint(KERN_WARNING "%s: insufficient USB endpoints found.\n",
								DRIVER_NAME);
		goto probe_failure_free;
	}

	/* Find endpoint specifics  */
	ep_out = &usb_if->cur_altsetting->endpoint[0];
	ep_in = &usb_if->cur_altsetting->endpoint[1];

	/* Check them... */
	if (!ep_out || !ep_in) {
		result -EINVAL;
		kprint(KERN_WARNING "%s: invalid USB endpoints.\n",
								DRIVER_NAME);
		goto probe_failure_free;
	}

	/* Save endpoints and data interface to device structure */
	dev->usb_ep_out = ep_out->desc.bEndpointAddress;
	dev->usb_ep_in = ep_in->desc.bEndpointAddress;
	dev->usb_if = usb_get_intf(usb_if);

	/* Claim the interface to stop other drivers doing so. */
	result = usb_driver_claim_interface(&blast_comms_usb_dev, dev->usb_if,
									dev);

	if (result < 0) {
		kprint(KERN_WARNING "%s: unable to claim USB data interface.\n",
								DRIVER_NAME);
		goto probe_failure_put;
	}

	result = usb_register_dev(interface, &blast_comms_class);
	if (result) {
		kprint(KERN_WARNING "%s: unable to claim USB data interface.\n",
								DRIVER_NAME);
		goto probe_failure_release;
	}

	/* Save our data pointer in this interface device. */
	usb_set_intfdata(interface, dev);

	/* Setup device (transciever), fail gracefully */
	result = blast_comms_dev_init(dev);
	if (result < 0) {
		result = -ENODEV;
		kprint(KERN_WARNING "%s: unable to initialise device.\n",
								DRIVER_NAME);
		goto probe_failure_release;
	}

	/* Everything went ok, register device with link layer. */
	blast_comms_link_register(dev);
	return 0;

	/* Something went wrong. Clean up. */
probe_failure_release:
	usb_set_intfdata(interface, NULL);
	usb_set_intfdata(dev->usb_if, NULL);
	usb_driver_release_interface(&blast_comms_usb_dev, dev->usb_if);
probe_failure_put:
	usb_put_intf(dev->usb_if);
probe_failure_free:
	usb_put_dev(dev->usb_dev);
	kfree(dev);

	return result;
}

/**
 * blast_comms_usb_disconnect - remove USB device from the system
 * @interface: the interface to be removed
 */
static void blast_comms_usb_disconnect(struct usb_interface *interface)
{
	/* TODO: do we need any files open handling here? */
	struct blast_comms_dev *dev;
	struct usb_host_interface *alt;

	dev = (struct blast_comms_dev *)usb_get_intfdata(interface);
	alt = interface->cur_altsetting;

	if (alt->desc.bInterfaceNumber)
		return;

	usb_set_intfdata(interface, NULL);
	usb_set_intfdata(dev->usb_if, NULL);
	usb_driver_release_interface(&blast_comms_usb_dev, dev->usb_if);
	usb_put_intf(dev->usb_if);
	usb_put_dev(dev->usb_dev);

	kfree(dev);
}

/**
 * blast_comms_usb_read - read from the USB device
 * @dev: the device
 * @buf: buffer to read to
 * @len: length to read
 */
static inline int blast_comms_usb_read(struct blast_comms_dev *dev, char *buf,
									int len)
{
	if (len <= 0 || len > BLAST_COMMS_USB_MAX_TRANSFER)
		return -EINVAL;

	return usb_bulk_msg(dev->usb_dev,
				usb_rcvbulkpipe(dev->usb_dev, dev->usb_ep_in),
				buf, len, &len, BLAST_COMMS_USB_TIMEOUT);
}

/**
 * blast_comms_usb_write - write to the USB device
 * @dev: the device
 * @buf: buffer to write from
 * @len: length of buffer
 */
static u32 blast_comms_usb_write(struct blast_comms_dev *dev, char *buf,
									int len)
{
	int result = 0;
	int actual = 0;
	char response[5];

	/* Check for valid len */
	if (len <= 0 || len > BLAST_COMMS_USB_MAXLEN) {
		kprint(KERN_WARNING "%s: blast_comms_usb_write: data too long",
			DRIVER_NAME);
		return -EINVAL;
	}

	/* Send buffer to device */
	result = usb_bulk_msg(dev->usb_dev,
				usb_sndbulkpipe(dev->usb_dev, dev->usb_ep_out),
				buf, len, &actual, BLAST_COMMS_USB_TIMEOUT);
	if (!result) {
		kprint(KERN_WARNING "%s: blast_comms_usb_write: send failed",
			DRIVER_NAME);
		return -EFAULT;
	}

	result = usb_bulk_msg(dev->usb_dev,
				usb_rcvbulkpipe(dev->usb_dev, dev->usb_ep_in),
				response, 5, &actual, BLAST_COMMS_USB_TIMEOUT);
	if (!result) {
		kprint(KERN_WARNING "%s: blast_comms_usb_write: receive failed",
			DRIVER_NAME);
		return -EFAULT;
	}

	if (response[0] != BLAST_COMMS_PIC_ACK)
		return *(u32 *)response[1];

	return 0;
}

/**
 * THE usb_driver Structure
 */
static struct usb_driver blast_comms_usb_dev = {
	.name 		= DRIVER_NAME,
	.probe 		= blast_comms_usb_probe,
	.disconnect	= blast_comms_usb_disconnect,
	.id_table	= blast_comms_id_table
};

/* EOF */
