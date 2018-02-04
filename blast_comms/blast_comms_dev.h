/**
 * blast_comms_dev.h
 *
 * Device Specific Stuff
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

#ifndef _BLAST_COMMS_DEV_H_
#define _BLAST_COMMS_DEV_H_

/*
 * Linux inclusions
 */
#include <linux/types.h>

/*
 * Function Prototypes
 */
static inline int blast_comms_pic_write(struct blast_comms_dev *dev, char *buf,
								size_t len);
static inline int blast_comms_pic_read(struct blast_comms_dev *dev, char *cmd,
				size_t cmdlen, char *buf, size_t buflen);
static int blast_comms_pic_tx_write(struct blast_comms_dev *dev,
						struct blast_comms_frame *frame);
static int blast_comms_pic_rx_read(struct blast_comms_dev *dev,
						struct blast_comms_frame *frame);
static int blast_comms_pic_rfm_write(struct blast_comms_dev *dev,  char *buf,
							unsigned char len);
static int blast_comms_pic_rfm_read(struct blast_comms_dev *dev,  char *buf,
							unsigned char len);
static int blast_comms_dev_start(struct blast_comms_dev *dev, int mode,
						double freq, int power,
						unsigned long br, u8 preamble);
static int blast_comms_dev_stop(struct blast_comms_dev *dev);
static inline int blast_comms_dev_starttx(struct blast_comms_dev *dev);
static inline int blast_comms_dev_startrx(struct blast_comms_dev *dev);
static int blast_comms_rfm_frequency(struct blast_comms_dev *dev, double freq);
static int blast_comms_rfm_bitrate(struct blast_comms_dev *dev,
							unsigned long val);
static int blast_comms_rfm_power(struct blast_comms_dev *dev, int power);
static int blast_comms_rfm_preamble_length(struct blast_comms_dev *dev, u8 val);
static int blast_comms_rfm_sync_word(struct blast_comms_dev *dev, u32 val);
static int blast_comms_rfm_packet_len(struct blast_comms_dev *dev, u8 val);

#endif /* _BLST_COMMS_DEV_H_ */

/* EOF */
