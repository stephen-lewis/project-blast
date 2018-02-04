/**
 * blast_comms_usb.h
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

#ifndef _BLAST_COMMS_USB_H_
#define _BLAST_COMMS_USB_H_

/*
 * Local inclusions
 */
#include "blast_comms.h"

/*
 * Constants
 */
#define USB_VENDOR_ID_BLASTCOMMS 			0x00
#define USB_DEVICE_ID_BLASTCOMMS 			0x00

#define BLAST_COMMS_USB_MAX_TRANSFER		4096
#define BLAST_COMMS_USB_TIMEOUT				4
#define	BLAST_COMMS_USB_MINOR_BASE			0

/*
 * Function prototypes
 */
static int blast_comms_usb_probe(struct usb_interface *interface,
						const struct usb_device_id *id);
static void blast_comms_usb_disconnect(struct usb_interface *interface);
static inline int blast_comms_usb_read(struct blast_comms_dev *dev, char *buf,
								int len);
static u32 blast_comms_usb_write(struct blast_comms_dev *dev, char *buf,
								int len);

#endif /* _BLAST_COMMS_USB_H_ */

/* EOF */
