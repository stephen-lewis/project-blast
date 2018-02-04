/**
 * blast_comms.h
 *
 * Main Header File
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

#ifndef _BLAST_COMMS_H_
#define _BLAST_COMMS_H_

/*
 * Linux inclusions
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/kfifo.h>
#include <linux/timer.h>
#include <linux/semaphore.h>

/*
 * Local inclusions
 */
#include "blast_comms_link.h"
#include "blast_comms_usb.h"
#include "blast_comms_dev.h"

/*
 * Constants
 */
#define	DRIVER_NAME			"blast-comms"	/* driver name */
#define	BLAST_COMMS_VERSION		"0.1"	/* driver version number */

#define	BLAST_COMMS_DEFAULT_FREQUENCY	433050
#define	BLAST_COMMS_DEFAULT_POWER	10
#define	BLAST_COMMS_DEFAULT_BITRATE	10
#define	BLAST_COMMS_DEFAULT_PREAMBLE	8

#define	BLAST_COMMS_TX			0x70
#define	BLAST_COMMS_RX			0x07
#define	BLAST_COMMS_UNCONFIG		0x00
#define	BLAST_COMMS_RTX			(BLAST_COMMS_TX | BLAST_COMMS_RX)
#define	BLAST_COMMS_PAIRED		0x88

#define	BLAST_COMMS_OPEN		0x01
#define	BLAST_COMMS_CLOSED		0x02
#define	BLAST_COMMS_FLUSHING		0x04

/* ioctl */
#define	BLAST_COMMS_IOC_MAGIC	'b'

#define	BLAST_COMMS_IOCRESET	_IO(BLAST_COMMS_IOC_MAGIC, 0)

#define	BLAST_COMMS_IOCSFREQ	_IO(BLAST_COMMS_IOC_MAGIC, 1, double)
#define	BLAST_COMMS_IOCGFREQ	_IO(BLAST_COMMS_IOC_MAGIC, 2, double)

#define	BLAST_COMMS_IOCTPOWER	_IO(BLAST_COMMS_IOC_MAGIC, 3, int)
#define	BLAST_COMMS_IOCQPOWER	_IO(BLAST_COMMS_IOC_MAGIC, 4, int)

#define	BLAST_COMMS_IOCTDR	_IO(BLAST_COMMS_IOC_MAGIC, 5, unsigned long)
#define	BLAST_COMMS_IOCQDR	_IO(BLAST_COMMS_IOC_MAGIC, 6, unsigned long)

#define	BLAST_COMMS_IOCTPRELEN	_IO(BLAST_COMMS_IOC_MAGIC, 7, __u8)
#define	BLAST_COMMS_IOCQPRELEN	_IO(BLAST_COMMS_IOC_MAGIC, 8, __u8)

#define	BLAST_COMMS_IOCTSW	_IO(BLAST_COMMS_IOC_MAGIC, 9, __u32)
#define	BLAST_COMMS_IOCQSW	_IO(BLAST_COMMS_IOC_MAGIC, 10, __u32)

#define	BLAST_COMMS_IOCPAIR	_IO(BLAST_COMMS_IOC_MAGIC, 11, \
						struct blast_comms_pair *)

#define	BLAST_COMMS_IOC_MAXNR	13

/*
 * The Device Structure
 */
struct blast_comms_dev {
	unsigned int		magic;
	/* USB Stuff */
	struct usb_device 	*usb_dev;
	struct usb_interface 	*usb_ctl_if;
	struct usb_interface 	*usb_if;
	int 			usb_ep_out;
	int 			usb_ep_in;

	/* Device Configuration */
	double	freq;
	int	power;
	int	bitrate;
	u8	preamble_len;

	struct blast_comms_link_dev *dev_list;
};

/*
 * Function Prototypes
 */

/* Utilities */
static void bitflip(u8 *buf, size_t len);

/* Threads */
static void blast_comms_softirq(struct blast_comms_dev *dev);
static void blast_comms_transmit(struct blast_comms_dev *dev);
static void blast_comms_watchdog(struct blast_comms_dev *dev);
static void blast_comms_receive(struct blast_comms_dev *dev);
static void blast_comms_frame_finder(struct blast_comms_dev *dev);

/* File Operations */
static int blast_comms_open(struct inode *inode, struct file *filp);
static int blast_comms_release(struct inode *inode, struct file *filp);
static ssize_t blast_comms_read(struct file *filp, char __user *buf,
						size_t count, loff_t *f_pos);
static ssize_t blast_comms_write(struct file *filp, char __user *buf,
						size_t count, loff_t *f_pos);
static int blast_comms_ioctl(struct inode *inode, struct file *filp,
					unsigned int cmd, unsigned long arg);
static int blast_comms_flush(struct file *filp);

#endif /* _BLAST_COMMS_H_ */

/* EOF */
