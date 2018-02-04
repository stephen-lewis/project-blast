#include <stdio.h>

#include "bcconfig.h"



/**
 * print_usage
 * Display command usage information.
 */
inline void print_usage(void)
{
	printf(usage_str, BC_NAME);
}

/**
 * print_version
 * Display version information.
 */
inline void print_version(void)
{
	printf("%s %f\n", BCCONFIG_NAME, BCCONFIG_VERSION);
	printf("Project BLAST Cubesat Communications Uplink/Downlink\n");
	printf("Configuration Tool\n");
	printf("Copyright (c) 2013 University of Southampton\n");
	printf("License GPLv2: GNU GPL version 2 <http://gnu.org/licenses/");
	printf("gpl.html>.\n");
	printf("This is free software: you are free to change and ");
	printf("redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
	printf("\nWritten by Stephen Lewis, Project BLAST.\n");
}

/**
 * print_error
 * @error: error text
 * Display error message.
 */
inline void print_error(char *error)
{
	printf("%s: %s\nTry '%s --help' for more information.\n", BCCONFIG_NAME,
			error, BCCONFIG_NAME);
}

/* EOF */
