/**
 * blast_pic_lib.c
 *
 * Useful Library Functions
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
 * memcpy - copy memory
 * @dest: destination
 * @src: source
 * @len: number of bytes to copy
 */
static void *memcpy(void *dest, void *src, size_t len)
{
	void *ret = dest;

	while (len > 0) {
		*src = *dest;
		dest++;
		src++;
		len--;
	}

	return ret;
}

/**
 * memset - set memory
 * @dest: destination
 * @val: value to set memory to
 * @len: number of bytes to copy
 */
static void *memset(void *dest, int val, size_t len)
{
	void *ret = dest;

	while (len > 0) {
		*dest = (unsigned char)val;
		dest++;
		len--;
	}

	return ret;
}

/* EOF */
