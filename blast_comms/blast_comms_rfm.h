/**
 * blast_comms_rfm.h
 *
 * RFM23 Sepcific Stuff
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

#ifndef _BLAST_COMMS_RFM_H_
#define _BLAST_COMMS_RFM_H_

/*
 * Local inclusions
 */
#include "blast_comms.h"

/* Registers */
#define	RFM_READ	0x00	/* RW Bit Settings (MSB of Address Field) */
#define	RFM_WRITE	0x80

#define	RFM_REG_DEVICE_TYPE	0x00	/* Get Device Type */
#define	RFM_REG_DEVICE_VER	0x01	/* Get Device Version */
#define	RFM_REG_DEVICE_STATUS	0x02	/* Get Device Status */

#define	RFM_REG_INTR_STATUS_1	0x03	/* Get Interrupt Status 1 */
#define	RFM_REG_INTR_STATUS_2	0x04	/* Get Interrupt Status 2 */
#define	RFM_REG_INTR_ENABLE_1	0x05	/* Get/Set Interrupt Enable 1 */
#define	RFM_REG_INTR_ENABLE_2	0x06	/* Get/Set Interrupt Enable 2 */

#define	RFM_REG_OPERATING_MODE	0x07	/* Get/Set Operating Mode */
#define	RFM_REG_OPERATING_CTL_2	0x08 	/* Get/Set Operating and Function
								Control 2 */

#define	RFM_REG_OSC_LOAD_CAP	0x09	/* Get/Set Crystal Oscillator
							Load Capacitance */

#define	RFM_REG_MICRO_OCLK	0x0A	/* Microcontroller Output Clock */

#define	RFM_REG_GPIO0_CONFIG	0x0B	/* GPIO0 Configuration */
#define	RFM_REG_GPIO1_CONFIG	0x0C	/* GPIO1 Configuration */
#define	RFM_REG_GPIO2_CONFIG	0x0D	/* GPIO2 Configuration */
#define	RFM_REG_IO_CONFIG	0x0E	/* I/O Port Configuration */

#define	RFM_REG_ADC_CONFIG	0x0F	/* ADC Configuration */

#define	RFM_REG_SENSOR_OFF	0x10	/* Sensor Offset */

#define	RFM_REG_ADC_VAL		0x11	/* ADC Value */

#define	RFM_REG_TEMP_CTL	0x12	/* Temperature Sensor Control */
#define	RFM_REG_TEMP_VAL	0x13	/* Temperature Sensor Value Offset */

#define	RFM_REG_WAKE_PER_1	0x14	/* Wake-Up Timer Period 1 */
#define	RFM_REG_WAKE_PER_2	0x15	/* Wake-Up Timer Period 2 */
#define	RFM_REG_WAKE_PER_3	0x16	/* Wake-Up Timer Period 3 */

#define	RFM_REG_WAKE_VAL_1	0x17	/* Wake-Up Timer Value 1 */
#define	RFM_REG_WAKE_VAL_2	0x18	/* Wake-Up Timer Value 2 */

#define	RFM_REG_LOW_BAT_THRES	0x1A	/* Low Battery Detector Threshold */
#define	RFM_REG_BAT_LVL		0x1B	/* Battery Voltage Level */

#define	RFM_REG_RSSI		0x26	/* Received Signal Strength Indicator */
#define	RFM_REG_RSSI_CCI	0x27	/* RSSI Threshold for Clear
							Channel Indicator */

#define	RFM_REG_DAC		0x30	/* Data Access Control */
#define RFM_REG_EZMAC_STATUS	0x31	/* EzMAC Status */

#define	RFM_REG_HDR_CTL_1	0x32	/* Header Control 1 */
#define	RFM_REG_HDR_CTL_2	0x33	/* Header Control 2 */

#define	RFM_REG_PREAMBLE_LEN	0x34	/* Preamble Length */
#define	RFM_REG_PREAMBLE_DCTL	0x35	/* Preamble Detection Control */

#define	RFM_REG_SYNC_WORD_3	0x36	/* Sync Word 3 */
#define	RFM_REG_SYNC_WORD_2	0x37	/* Sync Word 2 */
#define	RFM_REG_SYNC_WORD_1	0x38	/* Sync Word 1 */
#define	RFM_REG_SYNC_WORD_0	0x39	/* Sync Word 0 */

#define	RFM_REG_TX_HDR_3	0x3A	/* Transmit Header 3 */
#define	RFM_REG_TX_HDR_2	0x3B	/* Transmit Header 2 */
#define	RFM_REG_TX_HDR_1	0x3C	/* Transmit Header 1 */
#define	RFM_REG_TX_HDR_0	0x3D	/* Transmit Header 0 */

#define	RFM_REG_TX_PKT_LEN	0x3E	/* Transmit Packet Length */

#define	RFM_REG_CHECK_HDR_3	0x3F	/* Check Header 3 */
#define	RFM_REG_CHECK_HDR_2	0x40	/* Check Header 2 */
#define	RFM_REG_CHECK_HDR_1	0x41	/* Check Header 1 */
#define	RFM_REG_CHECK_HDR_0	0x42	/* Check Header 0 */

#define	RFM_REG_HDR_ENABLE_3	0x43	/* Header Enable 3 */
#define	RFM_REG_HDR_ENABLE_2	0x44	/* Header Enable 2 */
#define	RFM_REG_HDR_ENABLE_1	0x45	/* Header Enable 1 */
#define	RFM_REG_HDR_ENABLE_0	0x46	/* Header Enable 0 */

#define	RFM_REG_RX_HDR_3	0x47	/* Receive Header 3 */
#define	RFM_REG_RX_HDR_2	0x48	/* Receive Header 2 */
#define	RFM_REG_RX_HDR_1	0x49	/* Receive Header 1 */
#define	RFM_REG_RX_HDR_0	0x4A	/* Receive Header 0 */

#define	RFM_REG_RX_PKT_LEN	0x4B	/* Receive Packet Length */

#define	RFM_REG_TX_POWER	0x6D	/* Get/Set Transmission Power */
#define	RFM_REG_TX_DATA_RATE_1	0x6E	/* Get/Set Transmission Data Rate 1 */
#define	RFM_REG_TX_DATA_RATE_0	0x6F	/* Get/Set Transmission Data Rate 0 */

#define	RFM_REG_MOD_MODE_1	0x70	/* Modulation Mode Control 1 */
#define	RFM_REG_MOD_MODE_2	0x71	/* Modulation Mode Control 2 */
#define	RFM_REG_FREQ_DEV	0x72	/* Frequency Deviation */

#define	RFM_REG_FREQ_OFF_0	0x73	/* Get/Set Frequency Offset 1 */
#define	RFM_REG_FREQ_OFF_1	0x74	/* Get/Set Frequency Offset 2 */
#define	RFM_REG_FREQ_BAND_SEL	0x75	/* Frequency Band Selection */
#define	RFM_REG_NOM_CAR_FREQ_1	0x76	/* Get/Set Nominal Carrier Frequency 1 */
#define	RFM_REG_NOM_CAR_FREQ_0	0x77	/* Get/Set Nominal Carrier Frequency 0 */

#define	RFM_REG_FREQ_HOP_CHAN	0x79	/* Frequency Hopping Channel Selection */
#define	RFM_REG_FREQ_HOP_STEP	0x7A	/* Get/Set Frequency Hopping Step Size */

#define	RFM_REG_TX_FIFO_CTL_1	0x7C	/* Transmission FIFO Control 1 */
#define	RFM_REG_TX_FIFO_CTL_2	0x7D	/* Transmission FIFO Control 2 */

#define	RFM_REG_RX_FIFO_CTL	0x7E	/* Reception FIFO Control */

#define	RFM_REG_FIFO 	 	0x7F	/* Get RX FIFO/Set TX FIFO */

/* Transmit Power Settings */
#define	RFM_TX_POWER__8_DBM	0x0 		/* -8 dBm */
#define	RFM_TX_POWER__5_DBM	0x1 		/* -5 dBm */
#define	RFM_TX_POWER__2_DBM	0x2 		/* -2 dBm */
#define	RFM_TX_POWER_1_DBM	0x3 		/* 1 dBm */
#define	RFM_TX_POWER_4_DBM	0x4 		/* 4 dBm */
#define	RFM_TX_POWER_7_DBM	0x5 		/* 7 dBm */
#define	RFM_TX_POWER_10_DBM	0x6 		/* 10 dBm */
#define	RFM_TX_POWER_13_DBM	0x7 		/* 13 dBm */

/* Data Rate Constants */
#define	RFM_TX_DR_2_21		2097152			/* 2^21 */
#define	RFM_TX_DR_2_16		65536			/* 2^16 */

#define	RFM_TX_DR_SCALE_BIT	5

#define	RFM_TX_DR_MAX		256	/* kbps */
#define	RFM_TX_DR_MIN		1	/* kbps */

/* Frequency Band Selection */

/* This are ISM band limits defined by section S5.138 of Ofcom's regulations
	for radio communication in the UK */
#define	RFM_MAX_FREQ		434790	/* kHz */
#define	RFM_MIN_FREQ		433050	/* kHz */

#define	RFM_F_B		19

#endif /* _BLAST_COMMS_RFM_H_ */

/* EOF */
