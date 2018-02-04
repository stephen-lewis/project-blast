/**
 * main.c
 *
 * Application Entry Point
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

#include <curses.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bc.h"

/*
 * Global variables
 */

/* 'long' options - flags preceeded by '--': e.g. '--help' */
static struct option long_options[] = {
	{ "version",	no_argument,		0, 	0	},
	{ "help",	no_argument,		0, 	0 	},
	{ "config",	no_argument,		0, 	'c' 	},
	{ "device",	required_argument,	0, 	'd' 	},
	{ "freq",	required_argument,	0, 	'f' 	},
	{ "power",	required_argument,	0, 	'p' 	},
	{ "bitrate",	required_argument,	0, 	'r' 	},
	{ "setup",	no_argument,		0,	's'	},
	{ 0,		0,			0, 	0	}
};

/* 'short' options - single-character flags: e.g. '-d' */
static char *short_options = "cd:f:p:r:s";

/**
 * Usage text
 */
char *usage_str =
"Usage: blastcomms --device <device> [[options] | --help | --version] \n"
"options include:\n"
" --device <device>  Operate on device.\n"
" --bitrate <bps>    Set bitrate in bps.\n"
" --freq <kHz>       Set frequency in kHz.\n"
" --power <dBW>      Set power in dBW.\n"
" --setup            Perform initial device setup.\n"
" --config           Configure device.\n"
" --version          Display version information and exit.\n"
" --help             Display this message and exit.\n\n"
"Written by Stephen Lewis (stfl1g09@soton.ac.uk)\n"
"Copyright (c) 2013 University of Southampton\n";

/**
 * Version text
 */
char *version_str =
"version 0.1\n"
"Project BLAST Cubesat Communications Uplink/Downlink\n"
"Configuration Tool\n"
"Copyright (c) 2013 University of Southampton\n"
"License GPLv2: GNU GPL version 2 <http://gnu.org/licenses/"
"gpl.html>.\n"
"This is free software: you are free to change and "
"redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"\nWritten by Stephen Lewis, Project BLAST.\n";

/**
 * main
 * @argc: argument count
 * @argv: argument values
 * Application entry point
 */
int main(int argc, char **argv)
{
	/* locals */
	int optindex, this_option_optindex, option_index;
	int mode = BC_MODE_NONE;
	int len = 0;
	char c = 'c';
	char in = '\0';
	char out_head[BC_MAX_PACKET_LEN];
	char *out = out_head;
	char device[255];
	double freq = 0.0;
	unsigned long bitrate = 0;
	int power = 0;
	FILE *fp;

	/* option handling (infinite) loop */
	while (TRUE) {
		this_option_optindex = optindex ? optindex : 1;
		option_index = 0;

		opt = getopt_long(argc, argv, small_options, long_options,
								&option_index);
		if (opt == -1) /* no more options */
			break; /* break out of infinite loop */

		/* handle this option (opt) */
		switch (opt) {
		case 0: /* long argument */
			switch (option_index) {
			case 0: /* --version */
				bc_error(version_str, EXIT_SUCCESS);
			case 1: /* --help */
				bc_error(usage_str, EXIT_SUCCESS);
			}
			break;
		case 'c': /* -c/--config configure device */
			mode = BC_MODE_CONFIGURE;
			break;
		case 'f': /* -f/--freq set frequency */
			freq = atof(optarg);
			break;
		case 'p': /* -p/--power set power */
			power = atoi(optarg);
			break;
		case 'r': /* -r/--bitrate set bitrate */
			bitrate = atoi(optarg);
			break;
		case 'd': /* -d/--device act on device <device> */
			strcpy(device, optarg);
			break;
		case 's':
			mode = BC_MODE_SETUP;
			break;
		}
	}

	if (device_set != 1)
		bc_error("you must specify a device", EXIT_FAILURE);

	if (mode == BC_MODE_CONFIGURE || mode == BC_MODE_SETUP) {
		fp = fopen(device, "rw");
		if (!fp)
			bc_error("cannot open serial port", EXIT_FAILURE);

		if (mode == BC_MODE_SETUP) {
			rfm_set_preamble(fp, BC_PREAMBLE, BC_PREAMBLE_LEN);
			rfm_set_syncword(fp, BC_SYNCWORD);
		}

		if (freq != 0.0) {
			if ((freq > BC_MAX_FREQ) || (freq < BC_MIN_FREQ))
				bc_error("frequency out of range", EXIT_FAILURE);

			rfm_set_freq(fp, freq);
		}

		if (power != 0) {
			if ((power > BC_MAX_POWER) || (power < BC_MIN_POWER))
				bc_error("power out of range", EXIT_FAILURE);

			rfm_set_power(fp, power);
		}

		if (bitrate != 0.0) {
			if ((bitrate > BC_MAX_BITRATE) || (bitrate < BC_MIN_BITRATE))
				bc_error("bitrate out of range", EXIT_FAILURE);

			rfm_set_bitrate(fp, bitrate);
		}

		fclose(fp);
	} else if (mode == BC_MODE_PULSE) {
		fp = fopen(device, "rw");
		if (!fp)
			bc_error("cannot open serial port", EXIT_FAILURE);

		memset(out_head, 0, BC_MAX_PACKET_LEN);
		out_head[0] = BC_PULSE_CHAR_A;
		out_head[1] = BC_PULSE_CHAR_B;

		bc_print("pulsing... press 'q' to quit");

		while (c != 'q') {
			if (len > 250) {
				out_head = bc_build_packet(out_head, BC_RAM);
				fputs(fp, out_head);
				len = 0;

				fgetc(fp, in);
				if (in != BC_RESP_ACK)
					bc_error("device responded NACK", EXIT_FAILURE);
			}

			usleep(10);	/* sleep for a millisecond */
			len++;

			c = getch();	/* is 'q' pressed? */
		}

		fclose(fp);
	} else {
		fp = fopen(device, "rw");
		if (!fp)
			bc_error("cannot open serial port", EXIT_FAILURE);

		/* read from stdin and output to device until '\0' */
		while (1) {
			while ((c != '\n') && (c != '\0') && (len < BC_MAX_RAW_LEN)) {
				c = getc();
				*out = c;
				out++;
				len++;
			}

			if (len > 0) {
				out_head = bc_build_packet(out_head, BC_RAM);
				fputs(fp, out_head);
			}

			fgetc(fp, in);
			if (in != BC_RESP_ACK)
				bc_error("device responded NACK", EXIT_FAILURE);

			len = 0;
			out = out_head;
			memset(out, 0, BC_MAX_PACKET_LEN);

			if (c == '\0')
				break;
		}

		fclose(fp);
	}

	return EXIT_SUCCESS;
}
