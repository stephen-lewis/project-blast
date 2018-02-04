/**
 * blast_comms_dev.c
 *
 * Device Sepcific Stuff
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

/*
 * Linux inclusions
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>

/*
 * Local inclusions
 */
#include "blast_comms_dev.h"
#include "blast_comms_pic.h"
#include "blast_comms_rfm.h"

/**
 * blast_comms_pic_write - writes raw data to the PIC
 * @dev: device to write to
 * @buf: buffer to write
 * @len: length of buffer
 */
static int blast_comms_pic_write(struct blast_comms_dev *dev, char *buf,
								size_t len)
{
	/* Check for valid len */
	if (len <= 0 || len > BLAST_COMMS_USB_MAXLEN) {
		kprint(KERN_WARNING "%s: blast_comms_pic_write: length too long",
			DRIVER_NAME);
		return -EINVAL;
	}

	/* Send buffer to device */
	result = usb_bulk_msg(dev->usb_dev,
				usb_sndbulkpipe(dev->usb_dev, dev->usb_ep_out),
				buf, len, &actual, BLAST_COMMS_USB_TIMEOUT);
	if (!result) {
		kprint(KERN_WARNING "%s: blast_comms_pic_write: send failed",
			DRIVER_NAME);
		return -EFAULT;
	}

	return 0;
}

/**
 * blast_comms_pic_read - read raw data from the PIC
 * @dev: device to read from
 * @buf: buffer to read to
 * @len: length of buffer
 * NOTE: RETURNS ACK/NACK/EXACK/EXNACK! (or -ve error code)
 */
static int blast_comms_pic_read(struct blast_comms_dev *dev, char *buf,
						size_t len, size_t *actual)
{
	int ret = 0;
	size_t act;
	char *resp = kmalloc(BLAST_COMMS_USB_MAXLEN, GFP_KERNEL);

	/* check if actual is being ignored */
	if (actual == NULL)
		actual = &act;

	/* check if len is being ignored */
	if (len == 0)
		len = BLAST_COMMS_USB_MAXLEN);

	/* check allocation */
	if (!resp) {
		printk(KERN_WARNING "%s: blast_comms_pic_read: out of memory",
			DRIVER_NAME);
		return -ENOMEM;
	}

	/* USB receive */
	ret = usb_bulk_msg(dev->usb_dev,
				usb_rcvbulkpipe(dev->usb_dev, dev->usb_ep_in),
				resp, len, actual, BLAST_COMMS_USB_TIMEOUT);
	if (!ret) {
		kprint(KERN_WARNING "%s: blast_comms_pic_read: receive failed",
			DRIVER_NAME);
		return -EFAULT;
	}

	/* check actual length and truncate if necessary */
	if ((actual - 3) > len) {
		printk(KERN_WARNING "%s: blast_comms_pic_read: caution, truncating",
			DRIVER_NAME);

		*actual = len;
	}

	/* fill buf if EX(N)ACK and buf isn't being ignored */
	ret = (int)resp[0];
	if (((ret == BLAST_COMMS_PIC_EXACK) || (ret == BLAST_COMMS_PIC_EXNACK))
							&& buf != NULL) {
		memset(buf, 0, len);
		memcpy(buf, &resp[3], actual);
	} else {
		*actual = 0;
	}

	kfree(resp);
	return ret;
}

static inline int blast_comms_pic_readack(struct blast_comms_dev *dev)
{
	return blast_comms_pic_read(dev, NULL, 0, NULL);
}

static char *blast_comms_pic_buildcmd(unsigned int cmd, char *data, size_t len)
{
	char *buf;

	/* check command data length */
	if (len > BLAST_COMMS_PIC_CMDMAXLEN) {
		kprint(KERN_WARNING "%s: blast_comms_pic_buildcmd: data too long",
			DRIVER_NAME);
		return NULL;
	}

	/* allocate buffer and check */
	buf = kmalloc(len + 3, GFP_KERNEL);
	if (!buf) {
		kprint(KERN_WARNING "%s: blast_comms_pic_buildcmd: out of memory",
			DRIVER_NAME);
		return NULL;
	}

	/* build command */
	buf[0] = (unsigned char)cmd;
	*((uint16_t)&buf[1]) = (uint16_t)len;
	memcpy(&buf[3], data, len);

	return buf;
}

/**
 * blast_comms_pic_tx_write - writes a frame to the PIC's stack
 * @dev: device to write to
 * @frame: buffer to read from
 * Uses the PIC's PUT RAM command, form:
 * 	Offset	Description
 *	0x00	Command Byte (PUT RAM)
 *	0x01	Length (bytes)
 *	0x03	Data
 */
static int blast_comms_pic_tx_write(struct blast_comms_dev *dev,
						struct blast_comms_frame *frame)
{
	char *buf;
	int ret = 0;

	/* build command */
	buf = blast_comms_pic_buildcmd(BLAST_COMMS_PIC_PUTRAM, frame,
					sizeof(struct blast_comms_frame));
	if (!buf)
		return -EFAULT;

	/* lock USB to prevent erroneous responses */
	down(dev->usb_lock);

	/* write command to PIC */
	if (blast_comms_pic_write(dev, buf, sizeof(struct blast_comms_frame) + 3)) {
		/* MUST unlock USB and free buf for cleanliness */
		up(dev->usb_lock);
		kfree(buf);
		return -EFAULT;
	}

	/* get PIC response */
	ret = blast_comms_pic_readack(dev);

	/* unlock USB and free buf for cleanliness */
	up(dev->usb_lock);
	kfree(buf);

	if (likely(ret == BLAST_COMMS_PIC_ACK))
		return 0;

	return -EFAULT;
}

/**
 * blast_comms_pic_rx_read - reads a frame off of the PIC's receive stack
 * @dev: device to read from
 * @frame: buffer to read to
 */
static int blast_comms_pic_rx_read(struct blast_comms_dev *dev,
						struct blast_comms_frame *frame)
{
	char cmd[3] = { BLAST_COMMS_PIC_GETRAM, 0, 0 };
	size_t actual = 0;

	*((uint16_t *)&cmd[1]) = (uint16_t)sizeof(struct blast_comms_frame);

	down(dev->usb_lock);

	/* write command to PIC */
	if (blast_comms_pic_write(dev, cmd, 3)) {
		/* MUST unlock USB for cleanliness */
		up(dev->usb_lock);
		return -EFAULT;
	}

	blast_comms_pic_read(dev, (char *)frame, sizeof(struct blast_comms_frame),
								&actual);

	up(dev->usb_lock);

	if (actual != sizeof(struct blast_comms_frame)) {
		memset((char *)frame, 0, sizeof(struct blast_comms_frame));
		return -EFAULT;
	}

	return 0;
}



/**
 * blast_comms_pic_rfm_write - writes a command to the RFM23 via the PIC
 * @dev: device to write to
 * @buf: buffer to write from
 * @len: length to write
 */
static int blast_comms_pic_rfm_write(struct blast_comms_dev *dev,  char *buf,
							uint16_t len)
{
	char *buffer = blast_comms_pic_buildcmd(BLAST_COMMS_PIC_PUTXCVR, buf, len);
	int ret = 0;

	if (!buffer)
		return -EFAULT;

	down(dev->usb_lock);

	if(!blast_comms_pic_write(dev, buffer, len + 3)) {
		up(dev->usb_lock);
		kfree(buffer);
		return -EFAULT;
	}

	ret = blast_comms_pic_readack(dev);

	up(dev->usb_lock);

	if (ret == BLAST_COMMS_PIC_ACK)
		ret = 0;
	else
		ret = -EFAULT;

	kfree(buffer);
	return ret;
}

/**
 * blast_comms_pic_rfm_read - read last response from the RFM23 via the PIC
 * @dev: device to read from
 * @buf: buffer to read to
 * @len: length to read
 */
static int blast_comms_pic_rfm_read(struct blast_comms_dev *dev,  char *buf,
							size_t len)
{
	char cmd[3] = { BLAST_COMMS_PIC_READRFM, 0, 0 };
	int ret = 0;
	*((uint16_t *)&cmd[1]) = (uint16_t)len;

	down(dev->usb_lock);

	if(!blast_comms_pic_write(dev, cmd, 3)) {
		up(dev->usb_lock);
		return -EFAULT;
	}

	ret = blast_comms_pic_read(dev, len, buf, NULL);

	up(dev->usb_lock);

	if (ret == BLAST_COMMS_PIC_EXACK)
		ret = 0;
	else
		ret = -EFAULT;

	return ret;
}

/**
 * blast_comms_dev_start - starts up the RFM23 module and the PIC
 * @dev: device to start
 * @mode: mode to start device into
 * @freq: frequency to set device to
 * @power: power to set device to
 * @br: bitrate to set device to
 * @preamble: preamble length to set device to
 */
static int blast_comms_dev_start(struct blast_comms_dev *dev, int mode,
						double freq, int power,
						unsigned long br, u8 preamble)
{
	u8 cmd[2];
	int ret_val = 0;

	/* Configure the RFM23 */
	cmd[0] = RFM_WRITE | RFM_REG_MOD_MODE_2;
	cmd[1] = RFM_MODE_FIFO; /* FIFO Mode */

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	blast_comms_rfm_frequency(dev, freq);
	blast_comms_rfm_power(dev, power);
	blast_comms_rfm_bitrate(dev, br);
	blast_comms_rfm_preamble_length(dev, preamble);

	/* Set RFM and PIC to appropriate mode */
	if (mode == BLAST_COMMS_TX) {
		cmd[0] = RFM_WRITE | RFM_REG;
		cmd[1] = RFM_MODE_TX;
		ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
		if (!ret_val)
			return ret_val;

		ret_val = blast_comms_pic_mode(dev, BLAST_COMMS_TX);
		if (!ret_val)
			return ret_val;
	} else {
		cmd[0] = RFM_WRITE | RFM_REG;
		cmd[1] = RFM_MODE_RX;
		ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
		if (!ret_val)
			return ret_val;

		ret_val = blast_comms_pic_mode(dev, BLAST_COMMS_RX);
		if (!ret_val)
			return ret_val;
	}

	return 0;
}

/**
 * blast_comms_dev_init - initialises the RFM23 module and the PIC
 * @dev: device to initialise
 */
static int blast_comms_dev_init(struct blast_comms_dev *dev)
{
	u8 cmd[2];
	int ret_val = 0;

	cmd[0] = RFM_WRITE | RFM_REG;
	cmd[1] = RFM_MODE_STANDBY;
	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	ret_val = blast_comms_pic_mode(dev, PIC_MODE_STANDBY);
	if (!ret_val)
		return ret_val;

	return 0;
}

/**
 * blast_comms_dev_stop - stops the RFM23 module and the PIC
 * @dev: device to stop
 */
static int blast_comms_dev_stop(struct blast_comms_dev *dev)
{
	return blast_comms_dev_init(dev);
}


/**
 * blast_comms_dev_starttx - starts device as a transmitter using defaults
 * @dev: device to start
 */
static inline int blast_comms_dev_starttx(struct blast_comms_dev *dev)
{
	return blast_comms_dev_start(dev, BLAST_COMMS_TX,
					BLAST_COMMS_DEFAULT_TXFREQ,
					BLAST_COMMS_DEFAULT_TXPOWER,
					BLAST_COMMS_DEFAULT_TXBR);
}

/**
 * blast_comms_dev_startrx - starts device as a receiver using defaults
 * @dev: device to start
 */
static inline int blast_comms_dev_startrx(struct blast_comms_dev *dev)
{
	return blast_comms_dev_start(dev, BLAST_COMMS_RX,
					BLAST_COMMS_DEFAULT_RXFREQ,
					BLAST_COMMS_DEFAULT_RXPOWER,
					BLAST_COMMS_DEFAULT_RXBR);
}

/**
 * blast_comms_rfm_frequency - sets the device frequency
 * @dev: device to configure
 * @freq: frequency in kHz
 */
static int blast_comms_rfm_frequency(struct blast_comms_dev *dev, double freq)
{
	u8 cmd[2];
	u16 f_c = 0;
	int ret_val = 0;

	/* This function is hard coded to deny access to any frequencies which
	 * are outside of the ISM bands recognised in the UK
	 */
	if (freq > RFM_MAX_FREQ || freq < RFM_MIN_FREQ)
		return -EINVAL;

	/* There are many combinations to create frequencies in the desired
	 * carrier frequency range.  We will use f_b = 19, f_o = 0. */
	cmd[0] = RFM_WRITE | RFM_REG_FREQ_BAND_SEL;
	cmd[1] = RFM_F_B | 0x40; /* register reset value is
				  * with bit 6 = true, assumed unchanged */

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_FREQ_OFF_1;
	cmd[1] = 0;

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_FREQ_OFF_0;
	cmd[1] = 0;

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	/* Datasheet states that
	 *	f_carrier = (f_b + 24 + (fc + f_o)/64000) * 10000 [kHz]
	 * Hence, as f_o = 0 and f_b = RFM_F_B,
	 */
	f_c = (freq - RFM_F_B - 24) * 64000; /* kHz */

	cmd[0] = RFM_WRITE | RFM_REG_NOM_CAR_FREQ_1;
	cmd[1] = ((u8 *)&f_c)[1];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_NOM_CAR_FREQ_0;
	cmd[1] = ((u8 *)&f_c)[0];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	dev->freq = freq;

	return 0;
}

/**
 * blast_comms_rfm_bitrate - sets the datarate
 * @dev: device to configure
 * @val: birtate to set to
 */
static int blast_comms_rfm_bitrate(struct blast_comms_dev *dev,
							unsigned long val)
{
	u8 cmd[2];
	u16 dr = 0;
	int ret_val;

	if (val > RFM_TX_DR_MAX || val < RFM_TX_DR_MIN)
		return -EINVAL;

	/* First, get current modulation mode control 1 register */
	cmd[0] = RFM_READ | RFM_REG_MOD_MODE_1;
	cmd[1] = 0;

	if (blast_comms_pic_rfm_read(dev, (char *)cmd, 2))
		return -1; /* TODO: change to suitable */

	if (val < 30) {
		dr = (val * RFM_TX_DR_2_21) / 1000;
		set_bit(RFM_TX_DR_SCALE_BIT, (unsigned long *)&cmd[1]);
	} else {
		dr = (val * RFM_TX_DR_2_16) / 1000;
		clear_bit(RFM_TX_DR_SCALE_BIT, (unsigned long *)&cmd[1]);
	}

	cmd[0] = RFM_WRITE | RFM_REG_MOD_MODE_1;

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_TX_DATA_RATE_1;
	cmd[1] = ((u8 *)&dr)[1];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_TX_DATA_RATE_0;
	cmd[1] = ((u8 *)&dr)[0];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	dev->bitrate = val;

	return 0;
}

/**
 * blast_comms_rfm_power - sets the transmitter power
 * @dev: device to configure
 * @power: power to set in dB
 */
static int blast_comms_rfm_power(struct blast_comms_dev *dev, int power)
{
	u8 cmd[2];
	int ret_val = 0;
	int new_power = 0;

	cmd[0] = RFM_WRITE | RFM_REG_TX_POWER;

	if (power < -5) {
		new_power = -8;
		cmd[1] = RFM_TX_POWER__8_DBM;
	} else if (power < -2) {
		new_power = -5;
		cmd[1] = RFM_TX_POWER__5_DBM;
	} else if (power < 1) {
		new_power = -2;
		cmd[1] = RFM_TX_POWER__2_DBM;
	} else if (power < 4) {
		new_power = 1;
		cmd[1] = RFM_TX_POWER_1_DBM;
	} else if (power < 7) {
		new_power = 4;
		cmd[1] = RFM_TX_POWER_4_DBM;
	} else if (power < 10) {
		new_power = 7;
		cmd[1] = RFM_TX_POWER_7_DBM;
	} else { /* 13 dBm is NOT LEGAL in the UK */
		new_power = 10;
		cmd[1] = RFM_TX_POWER_10_DBM;
	}

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);

	if (!ret_val)
		return ret_val;

	dev->power = new_power;

	return ret_val;
}

/**
 * blast_comms_rfm_preamble_length - sets the preamble length.
 * @dev: device to configure
 * @val: length in nibbles (one nibble = four bits)
 */
static int blast_comms_rfm_preamble_length(struct blast_comms_dev *dev, u8 val)
{
	u8 cmd[2];

	cmd[0] = RFM_WRITE | RFM_REG_PREAMBLE_LEN;
	cmd[1] = val;

	return blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
}

/**
 * blast_comms_rfm_sync_word - sets the value of the sync word
 * @dev: device to configure
 * @val: value to set
 */
static int blast_comms_rfm_sync_word(struct blast_comms_dev *dev, u32 val)
{
	u8 cmd[2];
	int ret_val = 0;

	cmd[0] = RFM_WRITE | RFM_REG_SYNC_WORD_3;
	cmd[1] = ((u8 *)val)[3];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_SYNC_WORD_2;
	cmd[1] = ((u8 *)val)[2];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_SYNC_WORD_1;
	cmd[1] = ((u8 *)val)[1];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	cmd[0] = RFM_WRITE | RFM_REG_SYNC_WORD_0;
	cmd[1] = ((u8 *)val)[0];

	ret_val = blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
	if (!ret_val)
		return ret_val;

	return 0;
}

/**
 * blast_comms_rfm_packet_len - sets the packet (frame) length
 * @dev: device to configure
 * @val: value to set (bytes)
 */
static int blast_comms_rfm_packet_len(struct blast_comms_dev *dev, u8 val)
{
	u8 cmd[2];

	cmd[0] = RFM_WRITE | RFM_REG_TX_PKT_LEN;
	cmd[1] = val;

	return blast_comms_pic_rfm_write(dev, (char *)cmd, 2);
}

/* EOF */
