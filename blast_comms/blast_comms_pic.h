/**
 * blast_comms_rfm.h
 *
 * PIC Sepcific Stuff
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
 * Last edited on 5/2/2013
 */

#ifndef _BLAST_COMMS_PIC_H_
#define _BLAST_COMMS_PIC_H_

#define BLAST_COMMS_PIC_RESET		0xF0
#define BLAST_COMMS_PIC_SETMODE		0xF1
#define BLAST_COMMS_PIC_SHUTDOWN	0xFF
#define BLAST_COMMS_PIC_CLEARRAM	0x10
#define	BLAST_COMMS_PIC_PUTRAM		0x11
#define	BLAST_COMMS_PIC_GETRAM		0x12
#define	BLAST_COMMS_PIC_PUTXCVR		0x21
#define	BLAST_COMMS_PIC_GETXCVR		0x22

#define	BLAST_COMMS_PIC_ACK		0xFF
#define	BLAST_COMMS_PIC_EXACK		0xFE
#define	BLAST_COMMS_PIC_NACK		0x0F
#define	BLAST_COMMS_PIC_EXNACK		0x0E

#define	BLAST_COMMS_PIC_MODE_RESET	0x00
#define	BLAST_COMMS_PIC_MODE_STANDBY	0x01
#define	BLAST_COMMS_PIC_MODE_TRANSMIT	0x02
#define	BLAST_COMMS_PIC_MODE_RECEIVE	0x03
#define	BLAST_COMMS_PIC_MODE_SHUTDOWN	0x0F

#endif /* _BLAST_COMMS_PIC_H_ */

/* EOF */
