/**
 * blast_comms_util.c
 *
 * Utility Functions
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

/*
 * Local inclusions
 */
#include "blast_comms.h"

/**
 * Bit reversal lookup table (used by __u8_bitflip)
 */
static u8 bitreversal_lookup[16] = {
   0x0, 0x8, 0x4, 0xC,
   0x2, 0xA, 0x6, 0xE,
   0x1, 0x9, 0x5, 0xD,
   0x3, 0xB, 0x7, 0xF
};

/**
 * __u8_bitflip - reverses the bits in a single byte
 * @n: byte to flip
 */
static u8 __u8_bitflip(u8 n)
{
   return (bitreversal_lookup[n & 0x0F] << 4) | bitreversal_lookup[n >> 4];
}

/**
 * bitflip - reverses the bits and endian of the data
 * @buf: buffer to flip
 * @len: length of buffer
 */
static void bitflip(u8 *buf, size_t len)
{
	u8 *tmp = (u8 *)kmalloc(len, GFP_ATOMIC);	/* Temporary buffer */
	u8 *buf_head = buf;			/* Hold the buffer head */
	size_t l = len;				/* Hold the buffer length */

	/* Check allocation, fail gracefully (ish) */
	if (tmp) {
		printk(KERN_NOTICE "%s: bitflip() unable to allocate memory "
					"for tmp. Aborting.\n", DRIVER_NAME);
		return;
	}

	/* Flip and reverse. */
	/* TODO: change to take into account cpu endianness */
	while (len > 0) {
		tmp[len - 1] = __u8_bitflip(*buf);
		buf++;
		len--;
	}

	/* Copy back to buffer */
	memcpy(buf_head, tmp, l);

	/* Clean up */
	kfree(tmp);
}

/* EOF */
