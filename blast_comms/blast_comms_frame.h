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

#ifndef _BLAST_COMMS_FRAME_H_
#define _BLAST_COMMS_FRAME_H_

/*
 * Linux inclusions
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/spinlock.h>

/*
 * Local inclusions
 */
#include "blast_comms.h"

/*
 * Frame Constants
 */
#define		BLAST_COMMS_FRAME_CORREL_TAG_1		0x26FF60A6
#define		BLAST_COMMS_FRAME_CORREL_TAG_2		0x00CC8FDE
#define		BLAST_COMMS_FRAME_SYNCWORD		0x7E

#define		BLAST_COMMS_ADDR_LEN			32

#define		BLAST_COMMS_FRAME_PID			0xF0

#define		BLAST_COMMS_DATA_FRAME			0x00
#define		BLAST_COMMS_NACK_FRAME			0x01
#define		BLAST_COMMS_ACK_FRAME			0x03

#define		BLAST_COMMS_FRAME_SEQNUM_LIM		0x7D

#define		BLAST_COMMS_FRAME_DATA_LEN		128
#define		BLAST_COMMS_FRAME_CHKSYM_LEN		32

/*
 * Stack map entries
 * Format:
 * | READY | SENT  |                COUNTER                |
 * |   1   |   1   |   0   |   0   |   0   |   0   |   1   |
 */
#define		BLAST_COMMS_STACK_MAP_CLEAR		0x00
#define		BLAST_COMMS_STACK_MAP_READY		0x80
#define		BLAST_COMMS_STACK_MAP_SENT		0x40
#define		BLAST_COMMS_STACK_MAP_UNREAD		0x08
#define		BLAST_COMMS_STACK_MAP_EXPIRED		0xFF

/**
 * The Frame Structure
 * (ISO-3309 Compliant)
 */
struct blast_comms_frame {
	u32 correlation_tag[2];			/** correlation tag */

	u8 head_sync_word;			/** sync word */

/*	char dest[BLAST_COMMS_ADDR_LEN];	/** destination address */
/*	char src[BLAST_COMMS_ADDR_LEN];		/** source address */

/*	u8 ctl;					/** control flags */
/*	u8 pid;					/** level 3 protocol id */
/*	u8 seq_num;				/** sequence number */

	u16 data_len;				/** length of data */
	u8 data[BLAST_COMMS_FRAME_DATA_LEN];	/** data */

	u16 fcs;				/** checksum */
	u8 tail_sync_word;			/** sync word */
/*	u8 check_sym[BLAST_COMMS_FRAME_CHKSYM_LEN];	/** check symbols */
};

/**
 *The Frame Stack Structure
 */
struct blast_comms_frame_stack {
	struct blast_comms_frame *frame;	/** the stack */
	size_t size;				/** stack size */
	char *map;				/** map of frames */
	spinlock_t lock;			/** stack lock */
};

#define BLAST_COMMS_STACK_SIZE 		4096

/*
 * Function Prototypes
 */
static void blast_comms_build_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *buf);
static void blast_comms_build_ack_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *buf,
						u8 seqnum);
static void blast_comms_build_nack_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *buf,
						u8 seqnum);
static void blast_comms_finalise_frame(struct blast_comms_dev *dev,
						struct blast_comms_frame *buf,
						u8 seqnum);
static int blast_comms_validate_frame(struct blast_comms_dev *dev,
					struct blast_comms_frame *frame);

static int blast_comms_frame_stack_init(struct blast_comms_frame_stack *stack);
static void blast_comms_frame_stack_release(struct blast_comms_frame_stack *stack);

#endif /* _BLAST_COMMS_FRAME_H_ */

/* EOF */
