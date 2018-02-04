/**
 * blast_pic_ram.c
 *
 * SRAM Chip Functions
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

 /**
 * ram_write - write to the RAM chip
 * @addr: RAM address to write to
 * @buffer: pointer to the data
 * @len: length of the data
 * Returns 0 on success, <0 on error.
 */
static int ram_write(ptr_t addr, char *buffer, size_t len)
{
	/* TODO */
	return 0;
}

/**
 * ram_read - read a byte from the RAM chip
 * @addr: RAM address to read from
 * @buffer: pointer to a buffer
 * @len: length of the buffer
 * Return 0 on success, <0 on error or >0 if <len bytes written
 */
static int ram_read(ptr_t addr, char *buffer, size_t len)
{
	/* TODO */
	return 0;
}

/**
 * ram_init - intialise the RAM chip
 */
static int ram_init(void)
{
	/* TODO */
	/* Put the RAM chip into SEQUENTIAL mode */
	return 0;
}

/* EOF */
