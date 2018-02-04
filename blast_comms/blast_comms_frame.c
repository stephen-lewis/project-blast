/**
 * blast_comms_frame.c
 *
 * Frame Related Functions
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
#include <linux/errno.h>
#include <linux/crc-ccitt.h>
#include <linux/rslib.h>
#include <linux/spinlock.h>

/*
 * Local inclusions
 */
#include "blast_comms.h"

/**
 * blast_comms_build_frame - build an empty (data) frame and place it in buf
 * @dev: device to use configuration
 * @buf: frame buffer
 */
static void blast_comms_build_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *buf)
{
	/* Clear out buf */
	memset(buf, 0, sizeof(struct blast_comms_frame));

	/* Setup empty frame */
	buf->correlation_tag[0] = BLAST_COMMS_FRAME_CORREL_TAG_1;
	buf->correlation_tag[1] = BLAST_COMMS_FRAME_CORREL_TAG_2;
	buf->head_sync_word = BLAST_COMMS_FRAME_SYNCWORD;
	/*memcpy(buf->dest, dev->dest_addr, BLAST_COMMS_ADDR_LEN);*/
	/*memcpy(buf->src, dev->src_addr, BLAST_COMMS_ADDR_LEN);*/
	/*buf->ctl = BLAST_COMMS_DATA_FRAME;	/* default to data frame */
	/*buf->pid = BLAST_COMMS_FRAME_PID;*/
	buf->data_len = BLAST_COMMS_FRAME_DATA_LEN;
	buf->tail_sync_word = BLAST_COMMS_FRAME_SYNCWORD;
}

/**
 * blast_comms_build_ack_frame - build an ACK meta frame
 * @dev: device to use configuration
 * @buf: frame buffer
 * @seqnum: sequence number
 */
static void blast_comms_build_ack_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *buf,
						u8 seqnum)
{
	/* Build empty frame */
	blast_comms_build_frame(dev, buf);

	/* Make ACK frame */
	/*buf->ctl = BLAST_COMMS_ACK_FRAME;*/
	/*buf->seq_num = seqnum;*/
}

/**
 * blast_comms_build_nack_frame - build a NACK meta frame
 * @dev: device to use configuration
 * @buf: frame buffer
 * @seqnum: sequence number
 */
static void blast_comms_build_nack_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *buf,
						u8 seqnum)
{
	/* Build empty frame */
	blast_comms_build_frame(dev, buf);

	/* Make NACK frame */
	/*buf->ctl = BLAST_COMMS_NACK_FRAME;*/
	/*buf->seq_num = seqnum;*/
}

/**
 * blast_comms_finalise_frame - finalise a built frame
 * @dev: device to use configuration
 * @frame: frame buffer
 * @seqnum: sequence number
 */
static void blast_comms_finalise_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *frame,
						u8 seqnum)
{
	u16 crc = 0;

	/*frame->seq_num = seqnum;*/

	/* do CRC work */
	/*crc = crc_ccitt(~0, (char *)frame->dest, BLAST_COMMS_ADDR_LEN);
	crc = crc_ccitt(crc, (char *)frame->src, BLAST_COMMS_ADDR_LEN);
	crc = crc_ccitt(crc, &frame->ctl, sizeof(u8));
	crc = crc_ccitt(crc, &frame->pid, sizeof(u8));
	crc = crc_ccitt(crc, &frame->seq_num, sizeof(u8));*/
	/*crc = crc_ccitt(crc, &frame->data_len, sizeof(u16));*/
	crc = crc_ccitt(~0, &frame->data_len, sizeof(u16));
	crc = crc_ccitt(crc, frame->data, BLAST_COMMS_FRAME_DATA_LEN);

	bitflip((u8 *)&crc, sizeof(u16));

	frame->fcs = crc;

	/* do RS work */
}

/**
 * blast_comms_validate_frame - validate and correct a received frame
 * @dev: device to use configuration
 * @frame: frame buffer
 */
static int blast_comms_validate_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *frame)
{
	u16 crc = 0;
	/* do rs validation */
	/* TODO */

	/* do crc validation */
	bitflip((u8 *)frame->fcs, sizeof(__u16));

	/*crc = crc_ccitt(~0, (char *)frame->dest, BLAST_COMMS_ADDR_LEN);
	crc = crc_ccitt(crc, (char *)frame->src, BLAST_COMMS_ADDR_LEN);
	crc = crc_ccitt(crc, &frame->ctl, sizeof(u8));
	crc = crc_ccitt(crc, &frame->pid, sizeof(u8));
	crc = crc_ccitt(crc, &frame->seq_num, sizeof(u8));*/
	/*crc = crc_ccitt(crc, &frame->data_len, sizeof(u16));*/
	crc = crc_ccitt(~0, &frame->data_len, sizeof(u16));
	crc = crc_ccitt(crc, frame->data, BLAST_COMMS_FRAME_DATA_LEN);

	if (crc != frame->fcs))
		return BLAST_COMMS_E_BADFRAME_NOSEQ;

	return frame->ctl & BLAST_COMMS_ACK_FRAME;
}

/**
 * blast_comms_frame_stack_init - initialise a blast_comms_frame_stack structure
 * @stack: stack pointer
 */
static int blast_comms_frame_stack_init(struct blast_comms_frame_stack *stack)
{
	if (!stack)
		return -EINVAL;

	/* Allocate frame buffer, fail gracefully */
	stack->frame = kcalloc(sizeof(struct blast_comms_frame),
							BLAST_COMMS_STACK_SIZE,
							GFP_KERNEL);
	if (stack->frame)
		return -ENOMEM;

	/* Allocate map, fail gracefully */
	stack->map = (char *)kzalloc(BLAST_COMMS_STACK_SIZE, GFP_KERNEL);
	if (stack->map) {
		kfree(stack->frame);
		return -ENOMEM;
	}

	/* Initialise spinlock and size */
	spin_lock_init(&stack->lock);
	stack->size = BLAST_COMMS_STACK_SIZE;

	return 0;
}

/**
 * blast_comms_frame_stack_alloc - allocates a frame stack
 */
static struct blast_comms_frame_stack *blast_comms_frame_stack_alloc(void)
{
	struct blast_comms_frame_stack *stack;

	stack = kzalloc(sizeof(struct blast_comms_frame_stack), GFP_KERNEL);
	if (!stack)
		return NULL;

	if (!blast_comms_frame_stack_init(stack))
		return NULL;

	return stack;
}

/**
 * blast_comms_frame_stack_release - releases a frame stack structure
 * @stack: stack pointer
 */
static void blast_comms_frame_stack_release(struct blast_comms_frame_stack *stack)
{
	kfree(stack->frame);
	kfree(stack->map);
	kfree(stack);
}

/* EOF */
