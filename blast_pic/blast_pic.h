/**
 * blast_pic.h
 *
 * Header File
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
 * Last edited on 30/12/2012
 */

/*
 * Errors
 */
#define		EBADCMD				0x01
#define		EFIFOFULL			0x04
#define		EFIFOEMPTY			0x05
#define		EWTF				0x0F

/*
 * Modes
 */
#define		MODE_STANDBY		0x01
#define		MODE_TRANSMIT		0x02
#define		MODE_RECEIVE		0x03
#define		MODE_SHUTDOWN		0x0F

/*
 * Commands
 */
#define		CMD_RESET			0xF0
#define		CMD_SET_MODE		0xF1
#define		CMD_SHUTDOWN		0xFF
#define		CMD_RAM_CLEAR		0x10
#define		CMD_RAM_PUT			0x11
#define		CMD_RAM_GET			0x12
#define		CMD_XCVR_PUT		0x21
#define		CMD_XCVR_GET		0x22

/*
 * Responses
 */
#define		RESP_ACK			0xFF
#define		RESP_EXACK			0xFE
#define		RESP_NACK			0x0F
#define		RESP_EXNACK			0x0E

/*
 * Transceiver constants
 */
#define		XCVR_MAX			256 /* bytes */
#define		XCVR_BUFFER_LEN		40 /* bytes */
#define		XCVR_PUT_CMD		
#define		XCVR_GET_CMD

/*
 * RAM constants
 */
#define		RAM_BASE			0x0000
#define		RAM_LIMIT			0xFFFF

/*
 * Other constants
 */
#define		BUFFER_LEN 			4096 /* bytes */

/*
 * Types
 */
typedef 	unsigned int 		size_t;		/* buffer length */
typedef		unsigned int 		ptr_t;		/* RAM address */

/*
 * Function Prototypes
 */
static void *memcpy(void *dest, void *src, size_t len);
static void *memset(void *dest, int val, size_t len);

static void fifo_clear(void);
static int fifo_put(char *buffer, size_t len);
static int fifo_get(char *buffer, size_t len);

static int ram_write(ptr_t addr, char *buffer, size_t len);
static int ram_read(ptr_t addr, char *buffer, size_t len);
static int ram_init(void);

static int xvcr_get(char *buffer, size_t len);
static int xcvr_put(char *buffer, size_t len);
static int xcvr_write(char *buffer, char *user_buffer, size_t len);
static int xcvr_read(char *buffer, char *user_buffer, size_t len);
static int xcvr_shutdown(void);
static int xcvr_standby(void);
static int xcvr_receive(void);
static int xcvr_transmit(void);

/* EOF */
