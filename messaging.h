/**************************************************************************
*	messaging.h
*		by Eric Ceballos, LeValle Bradix, and Robert Young
*
*	OBJECTIVE:
*	This file will be used to define the nRF24L01 functions and registers
*	for use by the C file messaging.c.
*
*	DEFINITIONS:
*	This section defines the nRF24L01's register maps and SPI command
*	structure.
*
**************************************************************************/

// nRF24L01 Register Map
#define CONFIG			0x00
#define EN_AA			0x01
#define EN_RXADDR		0x02
#define SETUP_AW		0x03
#define SETUP_RETR		0x04
#define RF_CH			0x05
#define RF_SETUP		0x06
#define STATUS			0x07
#define OBSERVE_TX		0x08
#define CD				0x09
#define RX_ADDR_P0		0x0A
#define RX_ADDR_P1		0x0B
#define RX_ADDR_P2		0x0C
#define RX_ADDR_P3		0x0D
#define RX_ADDR_P4		0x0E
#define RX_ADDR_p5		0x0F
#define TX_ADDR			0x10
#define RX_PW_P0		0x11
#define RX_PW_P1		0x12
#define RX_PW_P2		0x13
#define RX_PW_P3		0x14
#define RX_PW_P4		0x15
#define RX_PW_P5		0x16
#define FIFO_STATUS		0x17
#define DYNPD			0x1C
#define FEATURE			0x1D

// nRF24L01 SPI Commands
#define R_REGISTER		0x00 // Mask w/ Register you wish to read
#define W_REGISTER		0x20 // Mask w/ Register you wish to write to
#define R_RX_PAYLOAD	0x61
#define W_TX_PAYLOAD	0xA0
#define FLUSH_TX		0xE1
#define FLUSH_RX		0xE2
#define REUSE_TX_PL		0xE3
#define NOP				0xFF

// Messaging Reserved Addresses
#define BROADCAST		0x00
#define SYNC			0xFF

// Messaging Command Constants
#define SEND_BYTE		0x01
#define RETURN_BYTE		0x02
#define ACK				0x03
#define SET_TEMP		0x10
#define GET_TEMPS		0x11
#define RETURN_TEMPS	0x12
#define GET_HUM			0x13
#define RETURN_HUM		0x14
#define GET_FLOW		0x15
#define RETURN_FLOW		0x16
#define SET_FLOW		0x17
#define CREATE_ADDR		0x29
#define SET_ADDR		0x2A
#define REJECT_ADDR		0x2B
#define RECEIVED_ADDR	0x2C
#define ADDR_NOT_SET	0x2D

// GPIO to be used
#define CE				12
#define CSN				8
#define MOSI			10
#define MISO			9
#define SCK				11
#define SyncBtn			17
#define SyncLED			23

// Constants used in functions
#define Chan 			0
#define Speed			8000000
#define thermoID		0x80
#define regID			0x00
#define typeMC			0
#define typeTherm		1
#define typeReg			2


/**************************************************************************
*
*	FUNCTION DEFINITIONS:
*	This section lays out prototypes for the functions to be used by the 
*	C file messaging.c
*
*	FUNCTIONS:
*		initRF()		- initializes the nRF24L01 module
*		writeReadRF()	- writes to or reads from the nRF24L01 registers
*		rxMode()		- sets the nRF24L01 into receiver mode
*		txMode()		- sets the nRf24L01 into transmitter mode
*		initMC()		- initialization routine for Master Controller
*		initThermo()	- initialization routine for Thermostats
*		initReg()		- initialization routine for Registers
*		getMessage()	- receives message from another device
*		getSyncMessage()- used to receive messages during syncing
*		sendMessage()	- sends a message to another device
*		SyncLEDPulse	- defined thread for blinking LED
*		ButtonHold()	- Function used in ProtocolA.c to control buttons
*
**************************************************************************/

int initRF(void);
unsigned char writeReadRF(unsigned char command, unsigned char *data, int length);
void rxMode(void);
void txMode(void);
int initMC(int *devNumber, unsigned char *devArray);
int initThermo(unsigned char *MCAddr);
int initReg(unsigned char *MCAddr);
int addDevice(unsigned char *devAddr, int type);
int getMessage(unsigned char *msgType, unsigned char *msgSourceAddr, int *msgVal1, int *msgVal2, int devType);
int getSyncMessage(unsigned char *syncType, unsigned char *syncSource, int *syncVal1, int *syncVal2);
int sendMessage(unsigned char msgType, unsigned char msgAddr, int msgVal1, int msgVal2);
PI_THREAD(SyncLEDPulse);
int ButtonHold(void);