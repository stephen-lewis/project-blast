/**
 * blast_pic_xcvr.c
 *
 * Transceiver Functions
 *
 * Transceiver Assembly
 * Programmable Integrated Circuit (PIC18) Software
 *
 * Project BLAST [http://www.projectsharp.co.uk]
 * University of Southampton
 * Copyright (c) 2012
 * Licensed under GPLv2
 *
 * Written by Stephen Lewis (stfl1g09@soton.ac.uk)
 * Last edited on 24/12/2012
 */

#include "blast_pic.h"

/*
 * Globals
 */
char		xcvr_buffer[XCVR_MAX];

/**
 * xvcr_get - get from the RFM23 receive buffer
 * @buffer: where to put the data
 * @len: number of bytes to get
 */
static int xvcr_get(char *buffer, size_t len)
{
	char put[XCVR_MAX];

	memset(put, 0, len + 1);
	put[0] = XCVR_GET_CMD;

	xcvr_write(put, xcvr_buffer, len + 1);
	xcvr_read(put, xcvr_buffer, len + 1);

	memcpy(buffer, &put[1], len);

	return 0;
}

/**
 * xvcr_put - put to the RFM23 tranmit buffer
 * @buffer: where to put the data from
 * @len: number of bytes to put
 */
static int xcvr_put(char *buffer, size_t len)
{
	char put[XCVR_MAX];

	if (len > XCVR_MAX)
		return -EBADLENGTH;

	put[0] = XCVR_PUT_CMD;
	memcpy(&put[1], buffer, len);

	return xcvr_write(put, xcvr_buffer, len + 1);
}

/**
 * xcvr_write - write to the RFM23
 * @buffer: what to write
 * @user_buffer: where to put result
 * @len: number of bytes to write
 */
static int xcvr_write(char *buffer, char *user_buffer, size_t len)
{
	/* TODO */
	return 0;
}

/**
 * xcvr_write - read from the RFM23
 * @buffer: where to read to
 * @user_buffer: where to find result
 * @len: number of bytes to read
 */
static int xcvr_read(char *buffer, char *user_buffer, size_t len)
{
	memcpy(buffer, user_buffer, len);
	return 0;
}

/**
 * xcvr_shutdown - shutdown the RFM23
 */
static int xcvr_shutdown(void)
{
	/* TODO: shutdown RFM23 */
	return 0;
}

/**
 * xcvr_standby
 */
static int xcvr_standby(void)
{
	/* TODO: put RFM23 into standby */
	return 0;
}

/**
 * xcvr_receive - RFM23 receive mode
 */
static int xcvr_receive(void)
{
	/* TODO: set RFM23 into receive mode and
	 * enable 'receive queue almost full' interrupt.
	 * All other interrupts should be disabled.
	 */
	return 0;
}

/**
 * xcvr_transmit - RFM23 transmit mode
 */
static int xcvr_transmit(void)
{
	/* TODO: set RFM23 into transmit mode and
	 * enable 'transmit queue almost empty' interrupt.
	 * All other interrupts should be disabled.
	 */
	return 0;
}

/* EOF */
