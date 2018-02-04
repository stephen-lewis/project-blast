/**
 * blast_pic.c
 *
 * Main Program File
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
int 		device_mode;
char		xcvr_user_buffer[XCVR_MAX];

/**
 * irq - the interrupt function
 * The function is called every interrupt.
 */
static int irq(void)
{
	char buffer[XCVR_BUFFER_LEN];
	int result = 0;

	switch (device_mode) {

		case MODE_TRANSMIT:
			result = fifo_get(buffer, XCVR_BUFFER_LEN);

			if (result == 0)
				return xcvr_put(buffer, XCVR_BUFFER_LEN);
			
			return result;

		case MODE_RECEIVE:
			result = xvcr_get(buffer, XCVR_BUFFER_LEN);

			if (result == 0)
				return fifo_put(buffer, XCVR_BUFFER_LEN);

			return result;
			
		case MODE_SHUTDOWN:
		case MODE_STANDBY:
		default:
			return -EWTF;
	}
}

/**
 * mode - set device mode
 * @mode_number: the mode
 */
static int mode(int mode_number)
{
	switch(mode_number) {

	case MODE_STANDBY:
		device_mode = MODE_STANDBY;
		xcvr_standby();
		return RESP_ACK;

	case MODE_SHUTDOWN:
		fifo_clear();
		xcvr_shutdown();
		device_mode = MODE_SHUTDOWN;
		return RESP_ACK;

	case MODE_RECEIVE:
		if (xcvr_setreceive() != 0)
			return RESP_NACK;
		device_mode = MODE_RECEIVE;
		return RESP_ACK;

	case MODE_TRANSMIT:
		if (xcvr_settransmit() != 0)
			return RESP_NACK;
		device_mode = MODE_TRANSMIT;
		return RESP_ACK;

	default:
		return RESP_NACK;
	}
}

/**
 * cmd - decodes and executes commands from the USB host
 * @buffer: a pointer to the raw data contained in the URB.
 * This function should be called when an URB is received from the host.
 * The contents of the URB should be passed to the function as a pointer (buffer).
 * This function will then execute the command and return the response code
 * that should be sent to the host (i.e. either ACK, NACK, EXACK or EXNACK).  If 
 * the response code is extended (i.e. EXACK or EXNACK) then the extra data and its
 * length has been placed in buffer.  The maximum length of the buffer is 4096 bytes.
 */
static int cmd(char *buffer)
{
	int result = 0;			/* somewhere to store return codes */

	switch (buffer[0]) {

	case CMD_RESET:						/* RESET */
		result = mode(MODE_STANDBY);	/* set device mode */
		if (result == RESP_ACK)			/* if successful, reset the fifo */
			fifo_clear();
		return result;

	case CMD_SET_MODE:					/* SET MODE */
		return mode(buffer[1]);			/* set device mode */

	case CMD_SHUTDOWN:					/* SHUTDOWN */
		return mode(MODE_SHUTDOWN);		/* set device mode */

	case CMD_RAM_CLEAR:					/* RAM CLEAR */
		fifo_clear();					/* reset the fifo */
		return RESP_ACK;				/* response: ACK */

	case CMD_RAM_PUT:					/* RAM PUT */
		if (device_mode != MODE_TRANSMIT)
			return RESP_NACK;			/* not in transmit mode! */

		if (fifo_put(&buffer[3], *((size_t)&buffer[1])) == 0)
			return RESP_ACK;			/* put data successfully */
		else
			return RESP_NACK;			/* something went wrong */

	case CMD_RAM_GET:					/* RAM GET */
		if (device_mode != MODE_RECEIVE)
			return RESP_NACK;			/* not in receive mode! */

		if (fifo_get(&buffer[3], *((size_t)&buffer[1])) == 0)
			return RESP_EXACK;			/* got data ok */
		else
			return RESP_NACK;			/* oops */

	case CMD_XCVR_PUT:					/* XCVR PUT */
		xcvr_write(&buffer[3], xcvr_user_buffer, *((size_t)&buffer[1]));
		return RESP_ACK;				/* always ACK, host should check
										 * actual response with XCVR GET
										 */

	case CMD_XCVR_GET:					/* XCVR GET */
		xcvr_read(&buffer[3], xcvr_user_buffer, *((size_t)&buffer[1]));
		return RESP_EXACK;
	}
}

/**
 * init - device startup (entry point)
 */
static int init(void)
{
	ram_init();
	mode(MODE_STANDBY);
	while (1) {}		/* infinite loop: all work done by interrupts */
	return 0;
}

 /* EOF */
