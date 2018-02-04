/**
 * blast_pic_fifo.c
 *
 * FIFO Functions
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
 * FIFO Globals
 */
ptr_t		fifo_head = RAM_BASE;
ptr_t		fifo_tail = RAM_BASE;
ptr_t		fifo_count = 0;

#define		fifo_limit 			RAM_LIMIT
#define		fifo_avail			((fifo_limit - fifo_count)
#define		fifo_empty			(fifo_count == 0)
#define		fifo_full			(fifo_count == fifo_limit)
#define		fifo_used			fifo_count

 /**
 * fifo_clear - clears the fifo
 */
static void fifo_clear(void)
{
	fifo_head = RAM_BASE;
	fifo_tail = fifo_head;
	fifo_count = 0;
}

/**
 * fifo_put - puts data into the RAM FIFO
 * @buffer: pointer to the data to put
 * @len: length of the data
 */
static int fifo_put(char *buffer, size_t len)
{
	ptr_t top_avail = fifo_limit - fifo_head;
	int result = 0;

	if (fifo_full)
		return -EFIFOFULL;

	result = ram_write(fifo_head, buffer, len);
	if (result != 0)
		return result;

	if (len > top_avail) {
		fifo_head = len - top_avail;
	} else {
		fifo_head += len;
	}

	return 0;
}

/**
 * fifo_get - gets data from the RAM FIFO
 * @buffer: pointer to a buffer
 * @len: length of the buffer
 */
static int fifo_get(char *buffer, size_t len)
{
	int result = 0;
	ptr_t used = fifo_used;
	ptr_t top_used = fifo_limit - fifo_tail;

	if (fifo_empty)
		return -EFIFOEMPTY;

	if (fifo_used < len) {
		result = ram_read(fifo_tail, buffer, fifo_used);
		if (result != 0)
			return result;

		fifo_tail = fifo_head;
		fifo_count = 0;
		return used;
	} else {
		result = ram_read(fifo_tail, buffer, len);
		if (result != 0)
			return result;

		if (top_used < len)
			fifo_tail = len - top_used;
		else
			fifo_tail += len;

		return 0;
	}
}

/* EOF */
