/**
 * blast_comms_link.h
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

#ifndef _BLAST_COMMS_LINK_H_
#define _BLAST_COMMS_LINK_H_

/*
 * Linux inclusions
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/cdev.h>

/*
 * Local inclusions
 */
#include "blast_comms.h"

/*
 * THE link layer structure layout
 */
struct blast_comms_link_layer {
	struct list_head 	used_list;
	struct list_head 	avail_list;
	struct list_head 	link_dev_list;

	spinlock_t 		list_lock;

	struct cdev 		ctrl_cdev;

	int 			major;

	atomic_t 		next_minor;
	atomic_t 		minor_count;
};

/*
 * Node structure, used by some ioctls
 */
struct blast_comms_node {
	dev_t 		devno;
	int 		mode;
};

/*
 * The Link Device Structure
 */
struct blast_comms_link_dev {
	unsigned int 			magic;
	struct cdev 			cdev;
	struct blast_comms_dev 		*rx;
	struct blast_comms_dev 		*tx;

	int				mode;
	atomic_t 			refcount;
	dev_t				devno;
	struct rc_control		*rs;

	char				dest_addr[BLAST_COMMS_ADDR_LEN];
	char				src_addr[BLAST_COMMS_ADDR_LEN];

	struct blast_comms_frame_stack	*tx_data_stack;
	struct kfifo			*tx_meta_stack;
	struct blast_comms_frame_stack	*rx_data_stack;
	struct kfifo			*rx_raw_stack;

	struct kfifo			*read_stack;
	struct semaphore		read_stack_sem;

	struct semaphore		master_sem;

	u8				tx_ptr;
	spinlock_t			tx_ptr_lock;

	/* Stack Data */
	atomic_t			unack;
	atomic_t			unsent;

	/* Soft IRQ Timer */
	struct timer_list		softirq;

	/* Thread Wait Queues */
	wait_queue_head_t		transmit_q;
	wait_queue_head_t		watchdog_q;
	wait_queue_head_t		receive_q;
	wait_queue_head_t		framefinder_q;
	wait_quene_head_t		decoder_q;
	wait_queue_head_t		readers_q;
	wait_queue_head_t		writers_q;

	struct task_struct		*transmit_thread;
	struct task_struct 		*watchdog_thread;
	struct task_struct 		*rawrecv_thread;
	struct task_struct 		*framefnd_thread;
	struct task_struct 		*receive_thread;

	struct list_head 		*dev_list;
};

/*
 * Function Prototypes
 */
static int blast_comms_link_ctrlinit(void);
static void blast_comms_link_exit(void);
static int blast_comms_cdev_add_minor(void);
static int blast_comms_link_init(struct blast_comms_link_dev *dev, int mode);
static inline struct blast_comms_link_dev *blast_comms_link_alloc(int mode);
static void blast_comms_link_release(struct blast_comms_link_dev *dev);
static int blast_comms_link_register(struct blast_comms_dev *dev);
static int blast_comms_link_unregister(struct blast_comms_dev *dev);
static int blast_comms_linkctrl_ioctl(struct inode *inode, struct file *filp,
					unsigned int cmd, unsigned long arg);
static int blast_comms_linkctrl_open(struct inode *inode, struct file *filp);
static int blast_comms_linkctrl_release(struct inode *inode, struct file *filp);

#endif /* _BLAST_COMMS_LINK_H_ */

/* EOF */
