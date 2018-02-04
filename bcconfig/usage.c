int main(int argc, char **argv)
{
	int opt;
	char **device_list;
	char **device_use_list;
	int devcount = 0;
	int dev_use_count = 0;
	int i = 0;
	double freq = 0.0;
	int mode = 0;
	int sub_mode = 0;

	/* handle arguments */
	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;

		opt = getopt_long(argc, argv, small_options, long_options,
								&option_index);
		if (opt == -1) /* no more options */
			break;

		switch (opt) {
		case 0:
			switch (option_index) {
			case 0:
				print_version();
				exit(EXIT_SUCCESS);
				break;
			case 3:
				mode = BCCONFIG_CREATE;
				break;
			case 4:
			case 5:
				mode = BCCONFIG_DESTROY;
				break;
			case 3:
			case 4:
			case 5:
				enum_devices(device_list, &devcount);
				dev_use_count = 1; /* TODO ??? */
				break;
			case 5:
				device_use_list = device_list;
				dev_use_count = devcount;
				break;
			}
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
			break;
		case 'v':
			verbosity += 1;
			break;
		case 'd':
			dev_use_list[dev_use_count] = malloc(255);
			if (!dev_use_list[dev_use_count]) {
				/* TODO: clean up */
				exit(EXIT_FAILURE);
			}

			strncpy(dev_use_list[dev_use_count], /* TODO */,
					255);

			dev_use_count++;

			break;
		case 'f':
			if (mode != 0) {
				/* mode already set! */
				print_error("multiple commands given");
				exit(EXIT_FAILURE);
			}

			mode = BCCONFIG_SETFREQ;
			freq = atof(/* TODO */);
			break;
		case 'l':
			/* list devices */
			enum_devices(device_list, &devcount);
			while (i < devcount) {
				printf("%s\n", device_list[i]);
				free(device_list[i]);
				i++;
			}
			free(device_list);
			exit(EXIT_SUCCESS);
		}
	}

	switch (mode) {
	case BCCONFIG_SETFREQ:

		break;
	}

	exit(EXIT_SUCCESS);
}



void enum_devices(char **list, int *devcount)
{
	int master_desc;
	int i_resp, i;

	if (devsenumed == TRUE)
		return;

	master_desc = open(BCCONFIG_MASTER_DEVNAME, 0);
	if (master_desc < 0) {
		print_error("unable to open master device");
		exit(EXIT_FAILURE);
	}

	ioctl(master_desc, BLAST_COMMS_IOCDEVCOUNT, &i_resp);

	list = calloc(sizeof(char *), i_resp);
	if (!list) {
		print_error("out of memory");
		close(master_desc);
		exit(EXIT_FAILURE);
	}

	i = 0;
	*devcount = i_resp;

	ioctl(master_desc, BLAST_COMMS_IOCRESETENUMDEV, 0);

	while (i_resp > 0) {
		list[i] = malloc(255);
		if (!list[i]) {
			print_error("out of memory");
			i--;
			while (i > 0) {
				free(list[i]);
				i--;
			}
			free(list);
			close(master_desc);
			exit(EXIT_FAILURE);
		}

		ioctl(master_desc, BLAST_COMMS_IOCENUMDEV, list[i]);
		i++;
		i_resp--;
	}

	close(master_desc);
}

static struct option long_options[] = {
	{ "version",	no_argument,		0, 	0	},
	{ "help",	no_argument,		0, 	'h' 	},
	{ "verbose",	no_argument,		0, 	'v'	},
	{ "create",	no_argument,		0,	0	},
	( "destroy",	no_argument,		0,	0,	},
	( "destroyall",	no_argument,		0,	0,	},
	{ 0,		0,			0, 	0	}
};

static char *short_options = "hvd:f:l";
static int verbosity = 0;
int devsenumed = FALSE;


