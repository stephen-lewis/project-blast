/**
 * blast_comms_threads.c
 *
 * Worker Threads
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
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/rslib.h>

/*
 * Local inclusions
 */
#include "blast_comms.h"

/**
 * blast_comms_transmit_thread
 * @dev: the device structure
 * This routine is initailised as work in a workqueue kernel thread by open().
 * It scans the transmit queue for unsent frames and sends them.
 */
static void blast_comms_transmit_thread(struct blast_comms_link_dev *dev)
{
	long this_frame = 0;			/* stack frame counter */
	struct blast_comms_frame frame;		/* meta frame storage */

	while (!kthread_should_stop()) {
		/* Check meta frame stack first (for NACK/ACKs) */
		if (!kfifo_is_empty(dev->tx_meta_stack)) {
			/* Get the meta frame */
			kfifo_get(dev->tx_meta_stack, &frame);

			/* Copy it to the device for transmission */
			blast_comms_pic_tx_write(dev, &frame);
		}

		if (atomic_read(&dev->unsent) < 1)
			wait_event_interruptible(dev->transmit_q,
				(!kfifo_is_empty(dev->tx_meta_stack) || \
				(atomic_read(&dev->unsent) > 0)));

		/* Now scan the data frame stack for unsent, ready frames */
		while ((this_frame < dev->tx_data_stack.size) && \
					(atomic_read(&dev->unsent) > 0)) {
			/* Is it unsent and ready? */
			if (dev->tx_data_stack.map[this_frame] & \
						BLAST_COMMS_STACK_MAP_READY) {
				/* Then copy it to the device, marking it in the
				 * stack as sent and updating counters
				 */
				blast_comms_pic_tx_write(dev,
					&dev->tx_data_stack.frame[this_frame]);

				spin_lock(&dev->tx_data_stack.lock);

				dev->tx_data_stack.map[this_frame] =  	     \
					dev->tx_data_stack.map[this_frame] | \
					BLAST_COMMS_STACK_MAP_SENT;

				spin_unlock(&dev->tx_data_stack.lock);

				atomic_dec(&dev->unsent);
				atomic_inc(&dev->unack);

				/* Increment counter, and break out of scanning
				 * to check again for meta frames
				 */
				this_frame++;
				break;
			} else {
				/* Increment counter, next frame */
				this_frame++;
			}
		}

		/* Have we scanned the whole stack? */
		if (this_frame >= dev->tx_data_stack.size) {
			/* Reset the counter and empty the meta stack
			 * into the device
			 */
			this_frame = 0;

			while (!kfifo_is_empty(dev->tx_meta_stack)) {
				kfifo_get(dev->tx_meta_stack, &frame);

				blast_comms_pic_tx_write(dev, &frame);
			}

			/* Sleep */
			wait_event_interruptible_timeout(dev->transmit_q,
				(!kfifo_is_empty(dev->tx_meta_stack) || \
				(atomic_read(&dev->unsent) > 0)),
				BLAST_COMMS_TRANSMIT_THREAD_TIMEOUT);
		}
	}
}

/**
 * blast_comms_watchdog
 * @dev: the device structure
 * This routine is initailised as work in a workqueue kernel thread by
 * open().  It scans the transmit queue periodically to check for sent, but
 * unacknowledged frames.
 */
static void blast_comms_watchdog(struct blast_comms_link_dev *dev)
{
	long this_frame = 0;

	while (!kthread_should_stop()) {
	while ((this_frame < dev->tx_data_stack.size) && \
					(atomic_read(&dev->unack) > 0)) {
		/* Is it sent and unacknowledged? */
		if (dev->tx_data_stack.map[this_frame] & \
		(BLAST_COMMS_STACK_MAP_SENT | BLAST_COMMS_STACK_MAP_READY)) {
			/* No? Increment unacknowledgement counter and
			 * test for expired counter
			 */
			dev->tx_data_stack.map[this_frame]++;

			if (dev->tx_data_stack.map[this_frame] ==   \
						BLAST_COMMS_STACK_MAP_EXPIRED) {
				/* Counter has expired, so flag frame
				 * for retransmission and update stack counters
				 */
				spin_lock(&dev->tx_data_stack.lock);
				dev->tx_data_stack.map[this_frame] =  \
						BLAST_COMMS_STACK_MAP_READY;
				spin_unlock(&dev->tx_data_stack.lock);

				atomic_inc(&dev->unsent);
				atomic_dec(&dev->unack);

				/* Wake up transmit thread */
				wake_up(dev->transmit_q);
			}

			/* Increment counter, and break out of scanning to
			 * check again for meta frames
			 */
			this_frame++;
			break;
		} else {
			/* Increment counter, next frame */
			this_frame++;
		}
	}

	/* Have we scanned the whole stack? */
	if (this_frame >= dev->tx_data_stack.size) {
		/* Reset counter and sleep */
		this_frame = 0;

		wait_event_interruptible(dev->watchdog_q, (this_frame < \
					dev->tx_data_stack.size) && 	\
					(atomic_read(&dev->unack) > 0));
	}
	} /* while(!kthread_should_stop()) */
}

/**
 * blast_comms_softirq
 * @dev: the device structure
 * This function is run at regular intervals to wake any recurring threads.
 */
static void blast_comms_softirq(struct blast_comms_link_dev *dev)
{
	/* Wakeup watchdog thread */
	wake_up(dev->watchdog_q);

	/* Reschedule soft irq */
	add_timer(&dev->softirq);
}

/**
 * blast_comms_receive
 * @dev: the device structure
 * This function is run when there is data on the device.
 */
static void blast_comms_raw_receive(struct blast_comms_link_dev *dev)
{
	char buf = 0;

	while (!kthread_should_stop()) {
		while (blast_comms_pic_rx_read(dev, &buf, 1)) {
			if (kfifo_len(dev->rx_raw_stack) < 1) {
				wake_up(dev->framefinder_q);

				wait_event_interruptible(dev->receive_q,
					kfifo_len(dev->rx_raw_stack) > 0);
			}

			kfifo_put(dev->rx_raw_stack, &buf);
		}

		wait_event_interruptible(dev->receive_q,
			blast_comms_dev_status(BLAST_COMMS_DEV_DATA_TO_SEND));
	}
}

/**
 * blast_comms_frame_finder
 * @dev: the device structure
 * This function is run when there is data to pull frames from.
 */
static void blast_comms_frame_finder(struct blast_comms_link_dev *dev)
{
	u32	chunk = 0;
	struct blast_comms_frame frame;

	while (!kthread_should_stop()) {
	if (kfifo_len(dev->rx_raw_stack) < 1)
		wait_event_interruptible(dev->framefinder_q,
					kfifo_avail(dev->rx_raw_stack) > 0);

		while (chunk != BLAST_COMMS_FRAME_CORREL_TAG_2)
			kfifo_out(dev->rx_raw_stack, &chunk, sizeof(u32));

		if (kfifo_len(dev->rx_raw_stack) <   			  \
					(sizeof(struct blast_comms_frame) \
					- (2 * sizeof(u32))))
			wait_event_interruptible(dev->framefinder_q,
					kfifo_len(dev->rx_raw_stack) >=      \
					(sizeof(struct blast_comms_frame) -  \
					sizeof(u16) - 			     \
					BLAST_COMMS_FRAME_CHKSYM_LEN - 1));

		frame.correlation_tag[1] = chunk;

		kfifo_out(dev->rx_raw_stack, &frame.head_sync_word,
			sizeof(struct blast_comms_frame) - (2 * sizeof(u32)));

		switch (blast_comms_validate_frame(dev, &frame)) {
		case BLAST_COMMS_DATA_FRAME:
			if (dev->status == BLAST_COMMS_FLUSHING)
			/* Flushing buffers, ignore incoming data frames */
				break;

			/* Put validated frame on received stack */
			spin_lock(&dev->tx_data_stack.lock);
			memcpy(&dev->rx_data_stack.frame[frame.seq_num], &frame,
					sizeof(struct blast_comms_frame));
			dev->rx_data_stack.map[frame.seq_num] =   \
						BLAST_COMMS_STACK_MAP_UNREAD;
			spin_unlock(&dev->tx_data_stack.lock);

			/* Send ACK */
			blast_comms_build_ack_frame(dev, &frame, frame.seq_num);

			kfifo_put(dev->tx_meta_stack, &frame);

			wake_up(dev->transmit_q);
			wake_up(dev->decoder_q);
			break;
		case BLAST_COMMS_ACK_FRAME:
			spin_lock(&dev->tx_data_stack.lock);
			dev->tx_data_stack.map[frame.seq_num] =   \
						BLAST_COMMS_STACK_MAP_CLEAR;
			spin_unlock(&dev->tx_data_stack.lock);

			atomic_dec(&dev->unack);
			break;
		case BLAST_COMMS_NACK_FRAME:
			spin_lock(&dev->tx_data_stack.lock);
			dev->tx_data_stack.map[frame.seq_num] =   \
						BLAST_COMMS_STACK_MAP_READY;
			spin_unlock(&dev->tx_data_stack.lock);

			atomic_inc(&dev->unsent);

			wake_up(dev->q_transmit);
			break;
		case BLAST_COMMS_E_BADFRAME_SEQ:
			/* Send NACK on sequence number */
			blast_comms_build_nack_frame(dev, &frame,
								frame.seq_num);

			kfifo_put(dev->tx_meta_stack, &frame);

			wake_up(dev->q_transmit);
			break;
		default:
			/* Drop frame */
		}

		memset(&frame, 0, sizeof(struct blast_comms_frame));
		chunk = 0;
	}
}

/**
 * blast_comms_receive_thread
 * @dev: the device structure
 */
static void blast_comms_receive_thread(struct blast_comms_dev *dev) {
	size_t	rx_ptr = 0;

	while (!kthread_should_stop()) {
		if (dev->rx_data_stack.map[rx_ptr] != \
					BLAST_COMMS_STACK_MAP_UNREAD)
			wait_event_interruptible(dev->decoder_q,
					dev->rx_data_stack.map[rx_ptr] == \
					BLAST_COMMS_STACK_MAP_UNREAD);

		if (kfifo_avail(dev->read_stack) < \
				dev->rx_data_stack.frame[rx_ptr].data_len) {
			wait_event_interruptible(dev->decoder_q,
				kfifo_avail(dev->read_stack) < \
				dev->rx_data_stack.frame[rx_ptr].data_len);
			continue;
		}

		kfifo_in(dev->read_stack,
				&dev->rx_data_stack.frame[rx_ptr].data,
				dev->rx_data_stack.frame[rx_ptr].data_len);

		spin_lock(&dev->tx_data_stack.lock);
		dev->rx_data_stack.map[rx_ptr] = BLAST_COMMS_STACK_MAP_CLEAR;
		spin_unlock(&dev->tx_data_stack.lock);

		rx_ptr++;
		if (rx_ptr > BLAST_COMMS_FRAME_SEQNUM_LIM)
			rx_ptr = 0;
	}
}

/* EOF */
