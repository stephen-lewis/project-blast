/**
 * rfm.c
 *
 * RFM23 Routines
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

#include "bc.h"
#include "bc_rfm.h"

static char *bc_build_packet(char *packet, int target)
{
	char output[BC_MAX_PACKET_LEN];
	int len;

	memset(output, 0, BC_MAX_PACKET_LEN);

	if (target == BC_RFM) {
		len = strlen(packet);
		output[0] = BC_PIC_WRITE_RFM;
		*((unsigned short)&output[1]) = len;
		memcpy(&output[3], packet, len);
	} else {
		len = strlen(packet);
		output[0] = BC_PIC_WRITE_RAM;
		*((unsigned short)&output[1]) = len;
		memcpy(&output[3], packet, len);
	}

	memcpy(packet, output, BC_MAX_PACKET_LEN);

	return packet;
}

static int rfm_set_freq(FILE *fp, double freq)
{
	char packet[BC_MAX_PACKET_LEN];

	memset(packet, 0, BC_MAX_PACKET_LEN);

	/* There are many combinations to create frequencies in the desired
	 * carrier frequency range.  We will use f_b = 19, f_o = 0. */
	packet[0] = RFM_WRITE | RFM_REG_FREQ_BAND_SEL;
	packet[1] = RFM_F_B | 0x40; /* register reset value is
				     * with bit 6 = true, assumed unchanged */

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_FREQ_OFF_1;
	packet[1] = 0;

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_FREQ_OFF_0;

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	/* Datasheet states that
	 *	f_carrier = (f_b + 24 + (fc + f_o)/64000) * 10000 [kHz]
	 * Hence, as f_o = 0 and f_b = RFM_F_B,
	 */
	f_c = (freq - RFM_F_B - 24) * 64000; /* kHz */

	packet[0] = RFM_WRITE | RFM_REG_NOM_CAR_FREQ_1;
	packet[1] = ((unsigned char *)&f_c)[1];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_NOM_CAR_FREQ_0;
	packet[1] = ((unsigned char *)&f_c)[0];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	return 0;
}

static int rfm_set_bitrate(FILE *fp, unsigned long bitrate)
{
	char packet[BC_MAX_PACKET_LEN];
	int dr;

	memset(packet, 0, BC_MAX_PACKET_LEN);

	/* First, get current modulation mode control 1 register */
	packet[0] = RFM_READ | RFM_REG_MOD_MODE_1;

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);
	memset(packet, 0, BC_MAX_PACKET_LEN);
	fgets(fp, packet);

	if (val < 30) {
		dr = (bitrate * RFM_TX_DR_2_21) / 1000;
		set_bit(RFM_TX_DR_SCALE_BIT, (unsigned long *)&packet[1]);
	} else {
		dr = (bitrate * RFM_TX_DR_2_16) / 1000;
		clear_bit(RFM_TX_DR_SCALE_BIT, (unsigned long *)&packet[1]);
	}

	packet[0] = RFM_WRITE | RFM_REG_MOD_MODE_1;

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_TX_DATA_RATE_1;
	packet[1] = ((unsigned char *)&dr)[1];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_TX_DATA_RATE_0;
	packet[1] = ((unsigned char *)&dr)[0];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	return 0;
}

static int rfm_set_power(FILE *fp, int power)
{
	char packet[BC_MAX_PACKET_LEN];

	memset(packet, 0, BC_MAX_PACKET_LEN);

	packet[0] = RFM_WRITE | RFM_REG_TX_POWER;

	if (power < -5) {
		packet[1] = RFM_TX_POWER__8_DBM;
	} else if (power < -2) {
		packet[1] = RFM_TX_POWER__5_DBM;
	} else if (power < 1) {
		packet[1] = RFM_TX_POWER__2_DBM;
	} else if (power < 4) {
		packet[1] = RFM_TX_POWER_1_DBM;
	} else if (power < 7) {
		packet[1] = RFM_TX_POWER_4_DBM;
	} else if (power < 10) {
		packet[1] = RFM_TX_POWER_7_DBM;
	} else { /* 13 dBm is NOT LEGAL in the UK */
		packet[1] = RFM_TX_POWER_10_DBM;
	}

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	return 0;
}

static int rfm_set_preamble(FILE *fp, char c, int len)
{
	char packet[BC_MAX_PACKET_LEN];

	memset(packet, 0, BC_MAX_PACKET_LEN);

	packet[0] = RFM_WRITE | RFM_REG_PREAMBLE_LEN;
	packet[1] = len;

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	return 0;
}

static int rfm_set_syncword(FILE *fp, unsigned long val)
{
	char packet[BC_MAX_PACKET_LEN];

	memset(packet, 0, BC_MAX_PACKET_LEN);

	packet[0] = RFM_WRITE | RFM_REG_SYNC_WORD_3;
	packet[1] = ((unsigned char *)val)[3];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_SYNC_WORD_2;
	packet[1] = ((unsigned char *)val)[2];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_SYNC_WORD_1;
	packet[1] = ((unsigned char *)val)[1];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	packet[0] = RFM_WRITE | RFM_REG_SYNC_WORD_0;
	packet[1] = ((unsigned char *)val)[0];

	packet = bc_build_packet(packet, BC_RFM);
	fputs(fp, packet);

	return 0;
}
