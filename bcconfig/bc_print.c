/**
 * bc_print.c
 *
 * Print Routines
 *
 * Cubesat Communications Basic Downlink Software
 *
 * Project BLAST [http://www.projectblast.co.uk]
 * University of Southampton
 * Copyright (c) 2012,2013
 * Licensed under GPLv2
 *
 * Written by Stephen Lewis (stfl1g09@soton.ac.uk)
 * Last edited on 19/3/2012
 */

#include <stdio.h>
#include <stdlib.h>

static inline void bc_error(char *str, int retval)
{
	printf("%s: %s.\n", BC_NAME, str);
	exit(retval);
}

static inline void bc_print(char *str)
{
	printf("%s: %s.\n", BC_NAME, str);
}

/* EOF */
