/**
 * blast_comms_fop.c
 *
 * Userspace Entry Points (File Operations)
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
#include <linux/types.h>
#include <linux/fs.h>

/*
 * Local inclusions
 */
#include "blast_comms.h"

/**
 * blast_comms_open - opens a communications node
 * @inode: the inode to open
 * @filp: the created file pointer
 */
static int blast_comms_open(struct inode *inode, struct file *filp)
{
	struct blast_comms_link_dev *dev;  /* the link device */
	int result = 0;	/* return code */

	/* Get the device structure */
	dev = container_of(inode->i_cdev, struct blast_comms_link_dev, cdev);

	/* Check for current users */
	if (atomic_inc_return(&dev->refcount) > 1) {
		atomic_dec(&dev->refcount);
		return -EBUSY;
	}

	/* Check mode */
	if ((dev->mode == BLAST_COMMS_TX) && (filp->f_mode & FMODE_READ))
		return -EPERM;

	if ((dev->mode == BLAST_COMMS_RX) && (filp->f_mode & FMODE_WRITE))
		return -EPERM;

	/* Allocate stacks */
	result = kfifo_alloc(dev->tx_meta_stack, BLAST_COMMS_STACK_SIZE,
								GFP_KERNEL);
	if (!result)
		return result;

	result = kfifo_alloc(dev->rx_raw_stack, BLAST_COMMS_STACK_SIZE,
								GFP_KERNEL);
	if (!result)
		goto openfail_freetxkfifo;

	result = kfifo_alloc(dev->read_stack, BLAST_COMMS_STACK_SIZE,
								GFP_KERNEL);
	if (!result)
		goto openfail_freerxkfifo;

	result = dev->tx_data_stack = blast_comms_frame_stack_alloc();
	if (!result)
		goto openfail_freereadkfifo;

	result = dev->rx_data_stack = blast_comms_frame_stack_alloc();
	if (!result)
		goto openfail_releasetx;

	/* Initialise locking mechanisms */
	init_MUTEX(&dev->read_stack_sem);
	init_MUTEX(&dev->master_sem);
	spin_lock_init(&dev->tx_ptr_lock);
	spin_lock_init(&dev->usb_lock);

	/* Reset counters */
	atomic_set(&dev->unack, 0);
	atomic_set(&dev->unsent, 0);

	/* Fire up threads and timer list */
	if (dev->mode & BLAST_COMMS_TX)
		dev->transmit_thread = kthread_run(blast_comms_transmit_thread,
					dev, "bcxmit%d", MINOR(dev->devno));

	if (dev->mode & BLAST_COMMS_RX) {
		dev->rawrecv_thread = kthread_run(blast_comms_raw_receive, dev,
						"bcrawrx%d", MINOR(dev->devno));
		dev->framefnd_thread = kthread_run(blast_comms_frame_finder,
					dev, "bcfrfr%d", MINOR(dev->devno));
		dev->receive_thread = kthread_run(blast_comms_receive_thread,
					dev, "bcrecv%d", MINOR(dev->devno));
	}

	if (dev->mode & BLAST_COMMS_RTX) {
		dev->watchdog_thread = kthread_run(blast_comms_watchdog, dev,
						"bcwdog%d", MINOR(dev->devno));
		init_timer(&dev->softirq);
		dev->softirq.expires = BLAST_COMMS_WATCHDOG_PERIOD;
		dev->softirq.function = blast_comms_softirq;
		dev->softirq.data = dev;

		add_timer(&dev->softirq);
	}

	filp->private_data = dev;  /* for other methods */

	return 0;

openfail_releasetx:
	blast_comms_frame_stack_release(dev->tx_data_stack);
openfail_freereadkfifo:
	kfifo_free(dev->read_stack);
openfail_freerxkfifo:
	kfifo_free(dev->rx_raw_stack);
openfail_freetxkfifo:
	kfifo_free(dev->tx_meta_stack);
	return result;
}

/**
 * blast_comms_release - handles close() system call.
 * @inode: the associated inode
 * @filp: the file pointer to close
 */
static int blast_comms_release(struct inode *inode, struct file *filp)
{
	struct blast_comms_link_dev *dev = filp->private_data;
	int retval = 0;

	if (down_interruptible(dev->master_sem)) {
		return RESTARTSYS;
	}

	/* Reset Counters */
	atomic_set(&dev->unack, 0);
	atomic_set(&dev->unsent, 0);

	/* Stop threads */
	del_timer(&dev->softirgq);
	kthread_stop(dev->receive_thread);
	kthread_stop(dev->framefnd_thread);
	kthread_stop(dev->rawrecv_thread);
	kthread_stop(dev->watchdog_thread);
	kthread_stop(dev->transmit_thread);

	/* free stacks */
	kfifo_free(dev->tx_meta_stack);
	kfifo_free(dev->rx_raw_stack);
	kfifo_free(dev->read_stack);
	blast_comms_frame_stack_release(dev->tx_data_stack);
	blast_comms_frame_stack_release(dev->rx_data_stack);

	/* TODO: Check correct */
	kfree(dev->rs);

	return 0;
}

/**
 * blast_comms_read - handles the read() system call
 */
static ssize_t blast_comms_read(struct file *filp, char __user *buf,
						size_t count, loff_t *f_pos)
{
	struct blast_comms_link_dev *dev = filp->private_data;
	char *kbuf;

	kbuf = kmalloc(count, GFP_KERNEL);
	if (unlikely(kbuf))
		return -ENOMEM;

	if (down_interruptible(&dev->read_stack_sem))
		return -ERESTARTSYS;

	/* Check that there is enough to read, if not block and wait */
	if (kfifo_len(dev->read_stack) < count) {
		if (!wait_event_interruptible_timeout(dev->readers_q,
					kfifo_len(dev->read_stack) >= count,
					BLAST_COMMS_READ_TIMEOUT)) {
			up(&dev->read_stack_sem);
			return -ERESTARTSYS;
		}

		if (kfifo_len(dev->read_stack) < count) {
			/* Timeout, read whatever is there */
			count = kfifo_len(dev->read_stack);
		}
	}

	if (count == 0)
		return count;

	/* Read... */
	kfifo_out(dev->read_stack, kbuf, count);

	up(&dev->read_stack_sem);

	copy_to_user(buf, kbuf, count);
	kfree(kbuf);

	return count;
}

/**
 * blast_comms_write - handles the write() system call
 */
static ssize_t blast_comms_write(struct file *filp, char __user *buf,
						size_t count, loff_t *f_pos)
{
	struct blast_comms_link_dev *dev = filp->private_data;
	struct blast_comms_frame frame;
	ssize_t c = 0;
	u8	tx_ptr;
	char *data;
	char *data_head;

	data =  = kmalloc(count, GFP_KERNEL);
	if (unlikely(!data))
		return -ENOMEM;
	}

	data_head = data;

	copy_from_user(data, buf, count);

	while (count > 0) {
		blast_comms_build_frame(dev, &frame);

		if (count < BLAST_COMMS_FRAME_DATA_LEN) {
			memcpy(frame.data, data, count);
			frame.data_len = count;
			c += count;
			count = 0;
		} else {
			memcpy(frame.data, data, BLAST_COMMS_FRAME_DATA_LEN);
			data += BLAST_COMMS_FRAME_DATA_LEN;
			frame.data_len = BLAST_COMMS_FRAME_DATA_LEN;
			c += BLAST_COMMS_FRAME_DATA_LEN;
			count -= BLAST_COMMS_FRAME_DATA_LEN;
		}

		spin_lock(&dev->tx_ptr_lock);
			tx_ptr = dev->tx_ptr;
		spin_unlock(&dev->tx_ptr_lock);

		blast_comms_finalise_frame(dev, &frame, tx_ptr);

		/* Sleep - stack full! */
		if (dev->tx_data_stack.map[tx_ptr] != \
						BLAST_COMMS_STACK_MAP_CLEAR)
			wait_event_interruptible(dev->writers_q,
					dev->tx_data_stack.map[tx_ptr] == \
						BLAST_COMMS_STACK_MAP_CLEAR);

		spin_lock(&dev->tx_data_stack.lock);
			memcpy(&dev->tx_data_stack.frame[tx_ptr], &frame,
					sizeof(struct blast_comms_frame));
			dev->tx_data_stack.map[tx_ptr] = \
					BLAST_COMMS_STACK_MAP_READY;
		spin_unlock(&dev->tx_data_stack.lock);

		wake_up(transmit_q);
	}

	kfree(data_head);

	return c;
}

/**
 * blast_comms_ioctl - handles ioctl() system call.
 */
static int blast_comms_ioctl(struct inode *inode, struct file *filp,
					unsigned int cmd, unsigned long arg)
{
	struct blast_comms_dev *dev = filp->private_data;
	double freq = 0.0;

	/* Execute command */
	switch (cmd) {

	case BLAST_COMMS_IOCRESET:
		/* Reset device */
		break;

	case BLAST_COMMS_IOCSTXFREQ:
		/* Set transmit frequency */
		if (!capable(CAP_SYS_ADMIN)) /* Requires root permissions */
			return -EPERM;

		__get_user(freq, (double __user *)arg);
		return blast_comms_rfm_frequency(dev->tx, freq);
		break;

	case BLAST_COMMS_IOCGTXFREQ:
		/* Get transmit frequency */
		return __put_user(dev->tx->freq, (double __user *)arg);
		break;

	case BLAST_COMMS_IOCSRXFREQ:
		/* Set receive frequency */
		if (!capable(CAP_SYS_ADMIN)) /* Requires root permissions */
			return -EPERM;

		__get_user(freq, (double __user *)arg);
		return blast_comms_rfm_frequency(dev->rx, freq);
		break;

	case BLAST_COMMS_IOCGRXFREQ:
		/* Get receive frequency */
		return __put_user(dev->rx->freq, (double __user *)arg);
		break;


	case BLAST_COMMS_IOCTPOWER:
		/* Tell power (arg = power) */
		if (!capable(CAP_SYS_ADMIN)) {
			/* Requires root permissions */
			return -EPERM;
		}

		blast_comms_rfm_power(dev, (int)arg);
		break;

	case BLAST_COMMS_IOCQPOWER:
		/* Query power */
		return dev->power;
		break;
	case BLAST_COMMS_IOCTDR:
		if (!capable(CAP_SYS_ADMIN)) {
			return -EPERM;
		}
		blast_comms_rfm_datarate(dev, (int)arg);
		break;
	case BLAST_COMMS_IOCQDR:
		return dev->bitrate;
		break;
	case BLAST_COMMS_IOCTPRELEN:
		if (!capable(CAP_SYS_ADMIN)) {
			return -EPERM;
		}
		return blast_comms_rfm_preamble_length(dev, (u8)arg);
		break;
	case BLAST_COMMS_IOCQPRELEN:
		return dev->preamble_len;
		break;
	default:
		return -EINVAL;
	}
}

/* EOF */
