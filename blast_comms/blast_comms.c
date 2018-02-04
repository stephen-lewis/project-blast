/**
 * blast_comms.c
 *
 * Main Module File
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
 * Last edited on 2/12/2012
 */

/*
 * Linux inclusions
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>

/*
 * Local inclusions
 */
#include "blast_comms.h"

/**
 *  Module Globals
 */
extern struct usb_driver		blast_comms_usb_drv;
extern const struct usb_device_id	*blast_comms_id_table;

/**
 * Module Information
 */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Stephen Lewis (stfl1g09@soton.ac.uk)");
MODULE_DESCRIPTION("Device Driver for the BLAST Uplink/Downlink Comms Device");
MODULE_VERSION(BLAST_COMMS_VERSION);
MODULE_DEVICE_TABLE(usb, blast_comms_id_table);

/**
 * blast_comms_init - initialise the module
 */
static int __init blast_comms_init(void) {
	blast_comms_link_ctrlinit();			/* link level work */
	return usb_register(&blast_comms_usb_drv);	/* register with usb */
}

/**
 *	blast_comms_exit - module exit
 */
static void __exit blast_comms_exit(void) {
	blast_comms_link_exit();		/* link level work */
	usb_deregister(&blast_comms_usb_drv);	/* deregister with usb */
}

/**
 * Module Entry Points
 */
module_init(blast_comms_init);
module_exit(blast_comms_exit);

/* EOF */
