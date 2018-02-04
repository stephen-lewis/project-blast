/**
 * blast_comms_link.c
 *
 * Link-level Implementation
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
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>

/*
 * Local inclusions
 */
#include "blast_comms_link.h"

/**
 * Link Layer
 */
struct blast_comms_link_layer 		*bcll;

/**
 * File Operations Structures
 */
extern struct file_operations		blast_comms_rx_fops;
extern struct file_operations		blast_comms_tx_fops;
extern struct file_operations		blast_comms_rtx_fops;
struct file_operations			blast_comms_ctrl_fops;

/**
 * blast_comms_link_ctrlinit - initialise the link layer and control device
 */
static int blast_comms_link_ctrlinit(void)
{
	int result = 0;		/* result buffer */
	dev_t devno;		/* device number buffer */

	/* allocate link layer, fail gracefully */
	bcll = kmalloc(sizeof(struct blast_comms_link_layer), GFP_KERNEL);
	if (unlikely(!bcll)) {
		printk(KERN_CRIT "%s: blast_comms_link_ctrlinit: out of memory",
				DRIVER_NAME);
		return -ENOMEM;
	}

	/* initialise list head */
	INIT_LIST_HEAD(&bcll->used_list);
	INIT_LIST_HEAD(&bcll->avail_list);
	INIT_LIST_HEAD(&bcll->link_dev_list);

	/* initialise spinlocks */
	spin_lock_init(&bcll->link_lock);

	/* allocate chrdev region, fail gracefully */
	result = alloc_chrdev_region(&devno, 0, BLAST_COMMS_LINKDEV_COUNT,
							BLAST_COMMS_DEVNAME);
	if (!result) {
		printk(KERN_CRIT "%s: blast_comms_link_ctrlinit: chrdev region"
				" allocation failed",
				DRIVER_NAME);
		return result;
	}

	/* save devno details to link layer */
	bcll->major = MAJOR(devno);
	atomic_set(&bcll->minor_count, BLAST_COMMS_LINKDEV_COUNT);

	/* create the control device, always device 0 */
	cdev_init(&bcll->ctrl_cdev, &blast_comms_ctrl_fops);
	cdev_add(&bcll->ctrl_cdev, MKDEV(bcll->major, 0), 1);

	/* create master device symbolic link */
	symlink("/dev/bcll0", "/dev/bcll_master");

	printk(KERN_NOTICE "%s: control device set up.\n", DRIVER_NAME);

	return 0;
}

/**
 * blast_comms_link_exit - release all link layer stuff
 */
static void blast_comms_link_exit(void)
{
	struct list_head *ptr;
	struct list_head *next;
	struct blast_comms_link_dev *dev_entry;

	/* Stop EVERYTHING! */
	list_for_each_safe(ptr, next, &link_dev_list) {
		dev_entry = list_entry(ptr, struct blast_comms_link_dev,
								dev_list);
		blast_comms_link_release(dev_entry);
	}

	/* remove master device symbolic link */
	unlink("/dev/bcll_master");

	/* Release the control device */
	cdev_del(&bcll->ctrl_cdev);

	/* Release chrdev region */
	unregister_chrdev_region(MKDEV(bcll->major, 0),
					atomic_read(&bcll->minor_count));
	atomic_set(&bcll->minor_count, 0);
	atomic_set(&bcll->next_minor, 0);
	&bcll->major = 0;

	printk(KERN_NOTICE "%s: exiting.\n", DRIVER_NAME);
}

/**
 * blast_comms_cdev_add_minors - add a device to the chrdev region
 */
static int blast_comms_cdev_add_minor(void)
{
	dev_t devno;
	int ret = 0;

	/* calculate the next device number */
	devno = MKDEV(bcll->major, atomic_read(&bcll->next_minor));

	/* add additional minor devices to chrdev region */
	result = register_chrdev_region(devno, 1, BLAST_COMMS_DEVNAME);
	if (!result) {
		printk(KERN_CRIT "%s: blast_comms_cdev_add_minor: unable to add"
				 " additional chrdev region", DRIVER_NAME);
		return ret;
	}

	/* increment minor device count */
	atomic_inc(&bcll->minor_count);

	return 0;
}

/**
 * blast_comms_link_init - initialise a link layer device
 * @dev: pointer to a preallocated link device structure
 * @mode: type of device - RTX, just TX or just RX
 */
static int blast_comms_link_init(struct blast_comms_link_dev *dev, int mode)
{
	struct list_head *rx;	/** receiver */
	struct list_head *tx;	/** transmitter */
	dev_t	devno;		/** device number */

	/* Sanity: valid mode? */
	if (unlikely(!(mode | BLAST_COMMS_RTX))) {
		printk(KERN_WARNING "%s: link_init - invalid mode.\n",
								DRIVER_NAME);
		return -EINVAL;
	}

	/* Check just in case we are out of minors */
	if (unlikely(atomic_read(&bcll->next_minor) > 	\
					(atomic_read(&bcll->minor_count) - 1)) {
		result = blast_comms_cdev_add_minor();	/* allocate more! */
		if (!result)
			return result;
	}

	/* Make the devices unavailable */
	/* Enter a locked, softirq-less environment to manipulate lists */
	spin_lock_bh(&bcll->list_lock);

	/* Test for valid entries */
	if (list_empty(&bcll->avail_list) ||		 \
			(list_singular(&bcll->avail_list) && \
					mode == BLAST_COMMS_RTX)) {
		spin_unlock_bh(&bcll->list_lock);
		printk(KERN_WARNING "%s: no available devices.\n", DRIVER_NAME);
		return -ENODEV;
	}

	/* Allocate the next device number */
	devno = MKDEV(bcll->major, atomic_inc_return(&bcll->next_minor) - 1);
	dev->devno = devno;

	/* Get device(s) */
	if (mode & BLAST_COMMS_RX) {
		rx = bcll->avail_list->next;
		list_move_tail(rx, &bcll->used_list);
	}

	if (mode & BLAST_COMMS_TX) {
		tx = bcll->avail_list->next;
		list_move_tail(tx, &bcll->used_list);
	}
	spin_unlock_bh(&bcll->list_lock);

	/* Link device(s) */
	if (mode & BLAST_COMMS_RX) {
		dev->rx = list_entry(rx, struct blast_comms_dev, dev_list);
		dev->rx->link_dev = dev;
	}

	if (mode & BLAST_COMMS_TX) {
		dev->tx = list_entry(tx, struct blast_comms_dev, dev_list);
		dev->tx->link_dev = dev;
	}

	/* Initialise device(s) */
	if (mode & BLAST_COMMS_RX)
		blast_comms_dev_startrx(dev->rx);
	if (mode & BLAST_COMMS_TX)
		blast_comms_dev_starttx(dev->tx);

	/* Initialise chrdev */
	switch (mode) {
	case BLAST_COMMS_RX:
		cdev_init(&dev->cdev, &blast_comms_rx_fops);
		break;
	case BLAST_COMMS_TX:
		cdev_init(&dev->cdev, &blast_comms_tx_fops);
		break;
	default:
		cdev_init(&dev->cdev, &blast_comms_rtx_fops);
		break;
	}

	/* Add link device to list, and register the chrdev with the kernel */
	list_add_tail(&dev->dev_list, &bcll->link_dev_list);
	cdev_add(&dev->cdev, devno, 1);

	printk(KERN_NOTICE "%s: created device %d:%d, mode %d.\n", DRIVER_NAME,
					MAJOR(devno), MINOR(devno), mode);

	return 0;
}

/**
 * blast_comms_link_alloc - allocate and initialise a link layer device
 */
static inline struct blast_comms_link_dev *blast_comms_link_alloc(int mode)
{
	/* Allocate the link device structure */
	struct blast_comms_link_dev *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (unlikely(!dev)) {
		printk(KERN_WARNING "%s: link_alloc - out of memory.\n",
								DRIVER_NAME);
		return NULL;
	}

	/* Initialise */
	if (likely(blast_comms_link_init(dev, mode)))
		return dev;

	/* Clean up on failure */
	kfree(dev);
	return NULL;
}

/**
 * blast_comms_link_release - release a link device
 * @dev: link device to release
 */
static void blast_comms_link_release(struct blast_comms_link_dev *dev)
{
	dev_t devno;
	/* TODO: check for open files - kref? */
	if (dev) {
		devno = dev->devno;

		/* Remove the link device from the kernel and the list */
		cdev_del(dev->cdev);
		list_del(&dev->dev_list, &bcll->link_dev_list);

		/* Put the device(s) into standby */
		if (dev->tx != NULL) {
			blast_comms_dev_stop(dev->tx);
			dev->tx->link_dev = NULL;
		}

		if (dev->rx != NULL) {
			blast_comms_dev_stop(dev->rx);
			dev->rx->link_dev = NULL;
		}

		/* Make the devices available for reuse */
		/* Enter locked, softirq-less environment to manipulate lists */
		spin_lock_bh(&bcll->list_lock);

		if (dev->tx != NULL)
			list_move_tail(dev->tx->dev_list, &bcll->avail_list);

		if (dev->rx != NULL)
			list_move_tail(dev->rx->dev_list, &bcll->avail_list);

		spin_unlock_bh(&bcll->list_lock);

		/* Free the link device structure (if necesary) */
		kfree(dev);

		printk(KERN_NOTICE "%s: removed device %d:%d.\n", DRIVER_NAME,
						MAJOR(devno), MINOR(devno));
	}
}

/**
 * blast_comms_link_register - registers a device with the link layer
 * @dev: device to register
 */
static int blast_comms_link_register(struct blast_comms_dev *dev)
{
	/* Sanity: check arguments, fail gracefully */
	if (unlikely(dev))
		return -EINVAL;

	/* Sanity: ensure device is in the stopped state */
	blast_comms_dev_stop(dev);

	/* Make device available */
	list_add_tail(dev->dev_list, &bcll->avail_list);

	printk(KERN_NOTICE "%s: device has been registered.\n", DRIVER_NAME);

	return 0;
}

/**
 * blast_comms_link_unregister - unregister device from the link level
 * @dev: device to unregister
 *
 * Use to register a blast_comms device with the link layer - if the device is
 * still used in a link device, then the device will not be unregistered.
 */
static int blast_comms_link_unregister(struct blast_comms_dev *dev)
{
	struct list_head *ptr;
	struct list_head *next;
	struct blast_comms_dev *dev_entry;

	/* Sanity: check arguments, fail gracefully */
	if (unlikely(dev))
		return -EINVAL;

	/* Enter a softirq-less environment to manipulate lists */
	spin_lock_bh(&bcll->list_lock);

	/* Check used list for dev, if found fail gracefully */
	list_for_each(ptr, &bcll->used_list) {
		dev_entry = list_entry(ptr, struct blast_comms_dev, dev_list);
		if (unlikely(dev_entry == dev))
			return -EINVAL;
	}

	/* Device must be available, so remove it */
	list_for_each_safe(ptr, next, &bcll->avail_list) {
		dev_entry = list_entry(ptr, struct blast_comms_dev, dev_list);
		if (dev_entry == dev)
			link_del(ptr);
	}

	/* Back to reality */
	spin_unlock_bh(&bcll->list_lock);

	printk(KERN_NOTICE "%s: device has been unregistered.\n", DRIVER_NAME);

	return 0;
}

/**
 * blast_comms_linkctrl_ioctl - ioctl routine for the control device
 * @inode: the inode associated with the ioctl call (can only be control dev)
 * @filp: the file associated with the ioctl call (can only be control dev)
 * @cmd: the command
 * @arg: argument of the command
 */
static int blast_comms_linkctrl_ioctl(struct inode *inode, struct file *filp,
					unsigned int cmd, unsigned long arg)
{
	struct blast_comms_node node;
	struct blast_comms_link_dev *dev;
	struct list_head *ptr;
	struct list_head *next;

	switch (cmd) {
	case BLAST_COMMS_IOCRESET:
		/* Reset the comms system */
		/* OVERRIDES EVERYTHING! USE WITH CAUTION!!! */
		/* TODO: open file clean-up, maybe using kref? */
		list_for_each_safe(ptr, next, &bcll->link_dev_list) {
			dev = list_entry(ptr, struct blast_comms_link_dev,
								dev_list);
			blast_comms_link_release(dev);
		}

		printk(KERN_NOTICE "%s: comms system reset.\n", DRIVER_NAME);

		break;
	case BLAST_COMMS_IOCSILENCE:
		/* Silence the comms system: release all TX/RTX */
		/* OVERRIDES EVERYTHING! USE WITH CAUTION!!! */
		/* TODO: open file clean-up, maybe using kref? */
		list_for_each_safe(ptr, next, &bcll->link_dev_list) {
			dev = list_entry(ptr, struct blast_comms_link_dev,
								dev_list);
			if (dev->mode & BLAST_COMMS_TX)
				blast_comms_link_release(dev);
		}

		printk(KERN_NOTICE "%s: comms system silenced.\n", DRIVER_NAME);

		break;
	case BLAST_COMMS_IOCMKNOD:
		/* Make a communication node */
		copy_from_user(&node, arg, sizeof(struct blast_comms_node));

		dev = blast_comms_link_alloc(node.mode);
		if (unlikely(!dev))
			return -ENODEV;

		copy_to_user(arg, &node, sizeof(struct blast_comms_node));

		break;
	case BLAST_COMMS_IOCRMNOD:
		/* Remove a communication node */
		/* TODO: open file clean-up, maybe using kref? */
		copy_from_user(&node, arg, sizeof(struct blast_comms_node));

		list_for_each_safe(ptr, next, &bcll->link_dev_list) {
			dev = list_entry(ptr, struct blast_comms_link_dev,
								dev_list);
			if (dev->devno == node.devno)
				blast_comms_link_release(dev);
		}

		break;
	default:
		/* Say what? */
		return -ENOTTY;
	}

	return 0;
}

/**
 * blast_comms_linkctrl_open - opens the link control device
 * @inode: the inode associated with the ioctl call (can only be control dev)
 * @filp: the file associated with the ioctl call (can only be control dev)
 */
static int blast_comms_linkctrl_open(struct inode *inode, struct file *filp)
{
	/* Required to allow ioctl */
	return 0;
}

/**
 * blast_comms_linkctrl_release - releases the link control device
 * @inode: the inode associated with the ioctl call (can only be control dev)
 * @filp: the file associated with the ioctl call (can only be control dev)
 */
static int blast_comms_linkctrl_release(struct inode *inode, struct file *filp)
{
	/* Required to allow ioctl */
	return 0;
}

/**
 * THE file operations VFT for the link layer control device
 */
struct file_operations blast_comms_ctrl_fops = {
	.owner = THIS_MODULE,
	.open = blast_comms_linkctrl_open,
	.release = blast_comms_linkctrl_release,
	.ioctl = blast_comms_linkctrl_ioctl
};

/* EOF */
