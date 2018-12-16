/**************************************************************************
*	messaging.c
*		by Eric Ceballos, LeValle Bradix, and Robert Young
*
*	OBJECTIVE:
*	This file lays out the functions to be called in each device's main
*	C file.
*
*	FUNCTIONS:
*		initRF()		- initializes the nRF24L01 module
*		writeReadRF()	- reads and writes to the nRF24L01
*		rxMode()		- sets the RF module into receiver mode
*		txMode()		- sets the RF module into transmitter mode
*		initMC()		- initialization function for Master Controller
*		initThermo()	- initialization function for Thermostats
*		initReg()		- initialization function for Registers
*		addDevice()		- listens for a new device, then returns address
*		getMessage()	- receives message from another device
*		getSyncMessage()- receives a sync message from a device or MC
*		sendMessage()	- sends a message to another device
*
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <time.h>
#include "messaging.h"

// Global variables to be used by functions
unsigned char myAddr = 0;
unsigned char Master;
int LEDStatus = 2;
int addrCount;
int init = 0;
int devCount = 0;
int myNumber = 0;

/**************************************************************************
*	initRF()
*
*	To be used prior to initialization. Sets the RF module's registers to
*	desired settings.
*
*	PARAMETERS:
*		Input: none
*		Output: integer indicating success(1), or failure(0)
*
**************************************************************************/
int initRF(void)
{
	// printf("\nStarting initRF()");
	// Initialize Sync GPIO
	pinMode(CE, OUTPUT);
	digitalWrite(CE, LOW);
	pinMode(SyncLED, OUTPUT);
	digitalWrite(SyncLED, HIGH);
	pinMode(SyncBtn, INPUT);
	
	// Initialize Variables used
	unsigned char data[11];
	int a;
	
	// Initialize SPI module: Channel 0, 8Mbps
	if (wiringPiSPISetup(Chan, Speed) < 0)
	{
		// printf("\nInitRF() failed!");
		a = 0;
		init = 2;
	}
	else
	{
		// Flush TX and RX_FIFOs
		writeReadRF((unsigned char)(FLUSH_TX), data, 1);
		writeReadRF((unsigned char)(FLUSH_RX), data, 1);
		
		// Clear all STATUS flags
		data[0] = 0x70;
		writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
		
		// Disable Auto Acknowledge
		data[0] = 0x00;
		writeReadRF((unsigned char)(W_REGISTER|EN_AA), data, 2);
		
		// Disable all but first data pipe
		data[0] = 0x01;
		writeReadRF((unsigned char)(W_REGISTER|EN_RXADDR), data, 2);
		
		// Config Initializes: PWR_UP (0), CRC enabled, 1 byte CRC
		data[0] = 0x08;
		writeReadRF((unsigned char)(W_REGISTER|CONFIG), data, 2);
		
		a = 1;
		init = 1;
		// printf("\nInitRF() Successful!");
	}
	printf("\nLeaving initRF()\n");
	return a;
}

/**************************************************************************
*	writeReadRF()
*
*	Used by the functions as an all purpose SPI read/write command.
*	Designed to condense the functions that use the same code flow several
*	times.
*
*	PARAMETERS:
*		Input:	unsigned character command for the function of the nRF24L01
*				unsigned character pointer to the data array
*					clear the array before read command.
*				integer total byte length of SPI transfer including command
*		Output: unsigned character contents of the nRF24L01's STATUS
*
**************************************************************************/
unsigned char writeReadRF(unsigned char command, unsigned char *data, int length)
{
	// Define variables to be used
	int i;
	unsigned char dataBuffer[12];
	unsigned char status;
	
	// Command Byte always first
	dataBuffer[0] = command;
	
	// Write data to buffer
	for(i = 1; i < length; i++)
	{
		dataBuffer[i] = data[(i - 1)];
	}
	// Clear the remaining space in buffer
	for(i = length; i < sizeof(dataBuffer); i++)
	{
		dataBuffer[i] = 0x00;
	}
	delay(1);
	
	// Simultaneous read/write to SPI module
	wiringPiSPIDataRW(Chan, dataBuffer, length);
	delay(1);
	
	// STATUS Register is always returned first
	status = dataBuffer[0];
	
	// Write to the data buffer input
	for(i = 0; i < 11; i++)
	{
		data[i] = dataBuffer[(i + 1)];
	}
	
	return status;
}

/**************************************************************************
*	rxMode()
*
*	Used by the functions to set the nRF24L01 into receiver mode
*
*	PARAMETERS:
*		Input:	None
*		Output: None
*
**************************************************************************/
void rxMode(void)
{
	// printf("\nEntering RX Mode");
	// Declare function variables
	int i;
	unsigned char data[11];
	
	// CE starts LOW
	digitalWrite(CE, LOW);
	delay(1);
	
	// CONFIG: PWR_UP (1), CRC enabled, 1byte CRC, RX mode
	data[0] = 0x7B;
	for(i = 1; i < sizeof(data); i++)
	{
		data[i] = 0x00;
	}
	writeReadRF((unsigned char)(W_REGISTER|CONFIG), data, 2);
	
	// Set desired payload width to 11 bytes
	data[0] = 11;
	for(i = 1; i < sizeof(data); i++)
	{
		data[i] = 0x00;
	}
	writeReadRF((unsigned char)(W_REGISTER|RX_PW_P0), data, 2);
	
	// Start listening
	digitalWrite(CE, HIGH);
	delay(1);
	
	// printf("\n");
	return;
}

/**************************************************************************
*	txMode()
*
*	Used by the functions to set the nRF24L01 into transmitter mode
*
*	PARAMETERS:
*		Input:	None
*		Output: None
*
**************************************************************************/
void txMode(void)
{
	// printf("\nEntering TX Mode");
	// Declare function variables
	int i;
	unsigned char data[11];
	
	// CE starts LOW
	digitalWrite(CE, LOW);
	delay(1);
	
	// CONFIG: PWR_UP (1), CRC enabled, 1byte CRC, TX mode
	data[0] = 0x7A;
	for(i = 1; i < sizeof(data); i++)
	{
		data[i] = 0x00;
	}
	writeReadRF((unsigned char)(W_REGISTER|CONFIG), data, 2);
	
	// printf("\n");
	return;
}
/**************************************************************************
*	initMC()
*
*	To be used by the Master Control device. Generates an address for the
*	Master controller, then listens for a device id from a thermostat, 
*	then registers. For each device, an address is generated and sent out.
*	The MC then listens for an ACK packet from the same device. The address
*	created for that device is only saved if an ACK packet is received.
*
*	PARAMETERS:
*		Input:	int pointer to variable for total number of devices
*				unsigned char pointer to array of devices listed in order
*		Output: integer indicating success(1), or failure(0)
*
**************************************************************************/
int initMC(int *devNumber, unsigned char *devArray)
{
	printf("\nInitializing MC!\n");
	
	// Declare and Initialize function variables
	unsigned char source;
	int stat;
	unsigned char msgCommand;
	unsigned char thermoAddr;
	unsigned char regAddr;
	unsigned char dev[126];
	int data1;
	int data2;
	int i;
	int j;
	int success = 0;
	int loop = 0;
	devCount = 0;
	
	myAddr = 0;
	LEDStatus = 0;
	
	if(init == 0)
	{
		initRF();
	}
	delay(1);
	piThreadCreate(SyncLEDPulse);
	
	printf("\nLED blinking");
	
	for(i = 0; i < sizeof(dev); i++)
	{
		dev[i] = 0;
	}
	
	// printf("\ndevArray cleared");
	
	srand(time(NULL));
	myAddr = ((rand() % 15) + 1) & 0xFF;
	addrCount = myAddr;
	printf("\nMy Addr: %#.2x", myAddr);
	
	rxMode();
	printf("\nSend Thermostat ID");
	delay(25);
	do
	{
		stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
		//printf("\nstat = %d", stat);
		if(stat == 1)
		{
			delay(25);
			if((msgCommand == CREATE_ADDR) && (data1 & 0x00000080))
			{
				thermoAddr = (data1) | (addrCount);
				if(addrCount >= 127)
				{
					printf("\nRolling over Thermostat Address");
					thermoAddr = (data1)|((addrCount + 1) & 0x7F);
				}
				dev[devCount] = thermoAddr;
				printf("\nThermostat Addr: %#.2x", dev[devCount]);
				sendMessage(SET_ADDR, SYNC, dev[devCount], data2);
				j = 0;
				delay(1);
				
				do
				{
					stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
					if(stat == 1)
					{
						printf("\nmsgCommand: %#.2x", msgCommand);
						printf("\nsource: %#.2x", source);
						printf("\ndata: %d %d", data1, data2);
						dev[devCount] = thermoAddr;
						if(msgCommand == RECEIVED_ADDR)
						{
							printf("\nThermostat Set! Send next device.");
							addrCount++;
							devCount++;
							printf("\n\ndevCount: %d", devCount);
							loop = 1;
						}
						else if(msgCommand == ADDR_NOT_SET)
						{
							dev[devCount] = 0;
							printf("\nThermostat failed to initialize");
							loop = 1;
						}
						else
						{
							printf("\nWrong Packet");
							loop = 0;
						}
					}
					j++;
					delay(18);
				}while((j < 250) && (loop == 0));
				printf("\nExiting Loop");
				loop = 0;
				if(j >= 250)
				{
					dev[devCount] = 0;
					printf("\nTimed out. Try again.");
				}
			}
			else if((msgCommand == CREATE_ADDR) && (!(data1 & 0x00000080)) && (devCount > 0))
			{
				regAddr = (data1) | (addrCount);
				if(addrCount >= 128)
				{
					printf("\nRolling Over Register Address");
					regAddr = (data1)|((addrCount + 1) & 0x7F);
				}
				dev[devCount] = regAddr;
				printf("\nRegister Addr: %#.2x", dev[devCount]);
				
				sendMessage(SET_ADDR, SYNC, dev[devCount], data2);
				j = 0;
				delay(1);
				
				do
				{
					stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
					if(stat == 1)
					{
						printf("\nmsgCommand: %#.2x", msgCommand);
						printf("\nsource: %#.2x", source);
						printf("\ndata: %d %d", data1, data2);
						dev[devCount] = regAddr;
						if(msgCommand == RECEIVED_ADDR)
						{
							printf("\nRegister Set! Send next device.");
							addrCount++;
							devCount++;
							printf("devCount: %d", devCount);
							loop = 1;
							j = 0;
						}
						else if(msgCommand == ADDR_NOT_SET)
						{
							dev[devCount] = 0;
							printf("\nRegister failed to initialize");
							loop = 1;
						}
						else
						{
							printf("\nWrong Packet");
							loop = 0;
						}
					}
					j++;
					delay(18);
				}while((j < 250) && (loop == 0));
				printf("\nExiting Loop");
				loop = 0;
				if(j == 250)
				{
					dev[devCount] = 0;
					printf("\nTimed out. Try again.");
				}
			}
			else if((msgCommand == CREATE_ADDR) && (!(data1 & 0x00000080)) && (devCount == 0))
			{
				printf("\nRegister pressed too early");
				sendMessage(REJECT_ADDR, SYNC, 0, data2);
			}
			else
			{
				msgCommand = 0;
				source = 0;
				data1 = 0;
				data2 = 0;
			}
		}
		delay(18);
	}while((digitalRead(SyncBtn)) && (devCount < 126));
	if(devCount == 126)
	{
		printf("\nMax device count reached");
	}
	
	*devNumber = devCount;
	printf("\nNumber of devices: %d", *devNumber);
	for(i = 0; i < sizeof(dev); i++)
	{
		devArray[i] = dev[i];
	}
	for(i = 0; i < devCount; i++)
	{
		printf("\nAddress[%d]: %#.2x", i, devArray[i]);
	}
	if(dev[0] == 0)
	{
		LEDStatus = 2;
		success = 0;
	}
	else
	{
		LEDStatus = 1;
		success = 1;
	}
	return success;
}

/**************************************************************************
*	initThermo()
*
*	To be used by the thermostat device. Sends out a thermostat ID packet
*	with a randomly generated number then listens for a response from the
*	MC. If the MC sends back an address with the same number, the MC
*	address is saved, and an ACK packet is sent out.
*
*	PARAMETERS:
*		Input:	unsigned character pointer to Master Control address
*		Output: integer indicating success(1), or failure(0)
*
**************************************************************************/
int initThermo(unsigned char *MCAddr)
{
	printf("\nInitializing Thermostat!\n");
	unsigned char source;
	unsigned char stat;
	unsigned char msgCommand;
	int data1;
	int data2;
	int j;
	int a;
	
	if(LEDStatus != 0)
	{
		LEDStatus = 0;
		piThreadCreate(SyncLEDPulse);
	}
	myAddr = 0;
	delay(1);
	
	if(myNumber == 0)
	{
		srand(time(NULL));
		myNumber = rand();
	}
	
	sendMessage(CREATE_ADDR, SYNC, thermoID, myNumber);
	j = 0;
	
	do
	{
		stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
		//printf("\nstat = %d", stat);
		if(stat == 1)
		{
			if((msgCommand == SET_ADDR) && (data1 & 0x80) && (data2 == myNumber))
			{
				myAddr = (data1 & 0xFF);
				*MCAddr = source;
				delay(25);
				sendMessage(RECEIVED_ADDR, *MCAddr, 0, 0);
				printf("\nMy Address: %#.2x\nMC Address: %#.2x", myAddr, *MCAddr);
				LEDStatus = 1;
				a = 1;
			}
			else if((msgCommand == SET_ADDR) && (data1 & 0x80) && (data2 != myNumber))
			{
				*MCAddr = source;
				delay(25);
				sendMessage(ADDR_NOT_SET, *MCAddr, 0, 0);
			}
			else
			{
				msgCommand = 0;
				source = 0;
				data1 = 0;
				data2 = 0;
			}
		}
		delay(18);
		j++;
	}while((myAddr == 0) && (j < 250));
	if(j == 250)
	{
		printf("\nTimeout. Leaving Thermostat initialization.\n");
		LEDStatus = 2;
		a = 0;
	}
	else
	{
		printf("\nAddress set\n");
	}
	rxMode();
	return a;
}

/**************************************************************************
*	initReg()
*
*	To be used by the register devices. Sends out a register ID packet
*	with a randomly generated number then listens for a response from the
*	MC. If the MC sends back an address with the same number, the MC
*	address is saved, and an ACK packet is sent out. If a reject packet is
*	received, the program exits with a failure status.
*
*	PARAMETERS:
*		Input:	int pointer to Master Control address
*		Output: integer indicating success(1), or failure(0)
*
**************************************************************************/
int initReg(unsigned char *MCAddr)
{
	printf("\nInitializing Register!\n");
	unsigned char source;
	unsigned char stat;
	unsigned char msgCommand;
	int data1;
	int data2;
	int j;
	int a;
	
	if(LEDStatus != 0)
	{
		LEDStatus = 0;
		piThreadCreate(SyncLEDPulse);
	}
	
	myAddr = 0;
	delay(1);
	
	if(myNumber == 0)
	{
		srand(time(NULL));
		myNumber = rand();
	}
	sendMessage(CREATE_ADDR, SYNC, regID, myNumber);
	j = 0;
	
	do
	{
		stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
		//printf("\nstat = %d", stat);
		if(stat == 1)
		{
			if((msgCommand == SET_ADDR) && !(data1 & 0x80) && (data2 == myNumber))
			{
				myAddr = (data1 & 0xFF);
				*MCAddr = source;
				delay(25);
				sendMessage(RECEIVED_ADDR, *MCAddr, 0, 0);
				printf("\nMy Address: %#.2x\nMC Address: %#.2x", myAddr, *MCAddr);
				LEDStatus = 1;
				a = 1;
			}
			else if((msgCommand == SET_ADDR) && !(data1 & 0x80) && (data2 != myNumber))
			{
				*MCAddr = source;
				delay(25);
				sendMessage(ADDR_NOT_SET, *MCAddr, 0, 0);
			}
			else if(msgCommand == REJECT_ADDR)
			{
				LEDStatus = 2;
				a = 0;
			}
			else
			{
				myAddr = 0;
				msgCommand = 0;
				data1 = 0;
				data2 = 0;
				LEDStatus = 0;
			}
		}
		delay(18);
		j++;
	}while((myAddr == 0) && (j < 250) && (LEDStatus != 2));
	if(j == 250)
	{
		LEDStatus = 2;
		a = 0;
		printf("\nTimeout. Leaving Register initialization.\n");
	}
	/* else if(LEDStatus == 2)
	{
		printf("\nNot your time.");
	}
	else
	{
		printf("\nAddress set\n");
	} */
	rxMode();
	return a;
}

/**************************************************************************
*	addDevice()
*
*	To be used by the Master Control device. Listens for a device ID to be
*	transmitted by a thermostat or register. Once it receives the ID, it
*	generates an address for the device, then sends it to the device. The
*	MC then waits for a confirmation packet from the device. The function
*	then returns from execution.
*
*	PARAMETERS:
*		Input:	unsigned char pointer to a device address variable
*				integer indicating expected device type
*		Output: integer indicating success(1), or failure(0)
*
**************************************************************************/
int addDevice(unsigned char *devAddr, int type)
{
	printf("\nAdding a Device!\n");
	unsigned char source;
	int stat;
	unsigned char msgCommand;
	unsigned char thermoAddr;
	unsigned char regAddr;
	int dev = 0;
	int data1;
	int data2;
	int j;
	int success = 0;
	
	LEDStatus = 0;
	delay(1);
	piThreadCreate(SyncLEDPulse);
	
	rxMode();
	printf("\nSend Device ID");
	delay(25);
	
	while(!(digitalRead(SyncBtn)))
	{
		delay(20);
	}
	while((success == 0) && digitalRead(SyncBtn) && devCount < 126);
	{
		stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
		if(stat == 1)
		{
			delay(25);
			if((msgCommand == CREATE_ADDR) && (data1 & 0x00000080) && (type == 1))
			{
				thermoAddr = (data1) | (addrCount);
				if(addrCount >= 127)
				{
					thermoAddr = (data1)|((addrCount+1) & 0x7F);
				}
				dev = thermoAddr;
				printf("\nThermostat Addr: %#.2x", dev);
				sendMessage(SET_ADDR, SYNC, dev, data2);
				j = 0;
				delay(1);
				
				do
				{
					stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
					if(stat == 1)
					{
						printf("\nmsgCommand: %#.2x", msgCommand);
						printf("\nsource: %#.2x", source);
						printf("\ndata: %d %d", data1, data2);
						if(msgCommand == RECEIVED_ADDR)
						{
							printf("\nThermostat Set! Send next device.");
							addrCount++;
							devCount++;
							success = 1;
							j = 0;
						}
						else if(msgCommand == ADDR_NOT_SET)
						{
							dev = 0;
							success = 2;
						}
						else
						{
							printf("\nWrong Packet");
							success = 0;
						}
					}
					j++;
					delay(18);
				}while((j < 250) && (success == 0));
				if(j == 250)
				{
					printf("\nTimed out.");
				}
			}
			else if((msgCommand == CREATE_ADDR) && (!(data1 & 0x00000080)) && (type == 2))
			{
				regAddr = (data1) | (addrCount);
				if(addrCount >= 128)
				{
					regAddr = (data1)|((addrCount + 1) & 0x7F);
				}
				dev = regAddr;
				printf("\nRegister Addr: %#.2x", dev);
				
				sendMessage(SET_ADDR, SYNC, dev, data2);
				j = 0;
				delay(1);
				do
				{
					stat = getSyncMessage(&msgCommand, &source, &data1, &data2);
					if(stat == 1)
					{
						printf("\nmsgCommand: %#.2x", msgCommand);
						printf("\nsource: %#.2x", source);
						printf("\ndata: %d %d", data1, data2);
						if(msgCommand == RECEIVED_ADDR)
						{
							printf("\nRegister Set! Send next device.");
							addrCount++;
							devCount++;
							success = 1;
							j = 0;
						}
						else if(msgCommand == ADDR_NOT_SET)
						{
							dev = 0;
							success = 2;
						}
						else
						{
							printf("\nWrong Packet");
							success = 0;
						}
					}
					j++;
					delay(18);
				}while((j < 250) && (success == 0));
				if(j == 250)
				{
					printf("\nTimed out. Try again.");
				}
			}
			else
			{
				msgCommand = 0;
				source = 0;
				data1 = 0;
				data2 = 0;
			}
		}
		delay(18);
	}
	if(devCount == 126)
	{
		printf("\nMax device count reached");
	}
	
	*devAddr = dev;
	if(dev == 0)
	{
		LEDStatus = 2;
		success = 0;
	}
	else
	{
		LEDStatus = 1;
		success = 1;
	}
	return success;
}

/*************************************************************************
*	getMessage()
*
*	Checks for available message, then reads the contents of the RX FIFO.
*
*	PARAMETERS:
*		Input:	unsigned char pointer to the type of message
*				unsigned char pointer to source address
*				int pointer to first data value
*				int pointer to second data value
*				int type of device calling the getMessage
*					(0) Master Control, (1) Thermostat, (2) Register
*		Output: int that indicates a read packet: (0) No Packet, (1) Packet
*
**************************************************************************/
int getMessage(unsigned char *msgType, unsigned char *msgSourceAddr, int *msgVal1, int *msgVal2, int devType)
{
	// printf("\nReading Message");
	int i;
	int j = 0;
	int a;
	unsigned char data[11];
	unsigned char stat;
	
	if(init == 0)
	{
		if(initRF())
		{
			printf("\nRF Module Initialized");
		}
		else
		{
			printf("\nRF Module failed to initialize");
		}
	}
	if((myAddr == 0) & (LEDStatus != 0))
	{
		LEDStatus = 0;
		piThreadCreate(SyncLEDPulse);
		//printf("\nDevice not Synced");
	}
	if((!digitalRead(SyncBtn)))
	{
		while((!digitalRead(SyncBtn)) && (j < 60))
		{
			delay(50);
			j++;
		}
		writeReadRF((unsigned char)(FLUSH_RX), data, 1);
		data[0] = 0x40;
		writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
		if((j == 60) && (devType == 1))
		{
			initThermo(&Master);
		}
		else if((j == 60) && (devType == 2))
		{
			initReg(&Master);
		}
	}
	//printf("\nMy address: %#.2x", myAddr);
	stat = writeReadRF((unsigned char)(NOP), data, 1);
	if((stat & 0x40) && (myAddr != 0))
	{
		for(i = 0; i < sizeof(data); i++)
		{
			data[i] = 0x00;
		}
		writeReadRF((unsigned char)(R_RX_PAYLOAD), data, 12);
	
		if(((data[0] == myAddr) || (data[0] == BROADCAST)))
		{
			*msgType = data[2];
			*msgSourceAddr = data[1];
			*msgVal1 = (data[3] << 24)|(data[4] << 16)|(data[5] << 8)|(data[6]);
			*msgVal2 = (data[7] << 24)|(data[8] << 16)|(data[9] << 8)|(data[10]);
			delay(1);
		
			printf("\nReturned msgTyp: %#.2x\nReturned SourceAddr: %#.2x\nReturned Payload: %d %d", *msgType, *msgSourceAddr, *msgVal1, *msgVal2);
		
			writeReadRF((unsigned char)(FLUSH_RX), data, 1);
			data[0] = 0x40;
			writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
			
			txMode();
			delay(25);
	
			data[0] = *msgSourceAddr;
			data[1] = myAddr;
			data[2] = ACK;
			data[3] = 0;
			data[4] = 0;
			data[5] = 0;
			data[6] = *msgType;
			data[7] = ((*msgVal1) >> 24) & 0xFF;
			data[8] = ((*msgVal1) >> 16) & 0xFF;
			data[9] = ((*msgVal1) >> 8) & 0xFF;
			data[10] = (*msgVal1) & 0xFF;
	
			stat = writeReadRF((unsigned char)(W_TX_PAYLOAD), data, 12);
	
			digitalWrite(CE, HIGH);
			delayMicroseconds(100);
			digitalWrite(CE, LOW);
			delayMicroseconds(100);
	
			do
			{
				stat = writeReadRF((unsigned char)(NOP), data, 1);
			}while(!(stat & 0x30));
	
			if(stat & 0x10)
			{
				printf("\nFailed to ACK");
				data[0] = 0x30;
				stat = writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
				a = 0;
			}
			else
			{
				printf("\nACK sent");
				data[0] = 0x30;
				stat = writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
				a = 1;
			}
			rxMode();
			// a = 1;
		}
		else
		{
			a = 0;
			delay(1);
		
			// printf("\nReturned msgTyp: %#.2x\nReturned SourceAddr: %#.2x\nReturned Payload: %d %d", *msgType, *msgSourceAddr, *msgVal1, *msgVal2);
			
			writeReadRF((unsigned char)(FLUSH_RX), data, 1);
			data[0] = 0x40;
			writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
		}
	}
	else
	{
		a = 0;
	}
		return a;
}

/**************************************************************************
*	getSyncMessage()
*
*	For use by the syncing and initialization functions. Reads a sync
*	packet and returns the contents.
*
*	PARAMETERS:
*		Input:	unsigned char pointer to the command type
*				unsigned char pointer to source address
*				int pointer to first sync value
*				int pointer to second sync value
*		Output: int that indicates a read packet: (0) No Packet, (1) Packet
*
**************************************************************************/
int getSyncMessage(unsigned char *syncType, unsigned char *syncSource, int *syncVal1, int *syncVal2)
{
	//printf("\nReading Message");
	int i;
	int a;
	unsigned char data[11];
	unsigned char stat;
	
	//printf("\nMy address: %#.2x", myAddr);
	stat = writeReadRF((unsigned char)(NOP), data, 1);
	if(stat & 0x40)
	{
		for(i = 0; i < sizeof(data); i++)
		{
			data[i] = 0x00;
		}
		writeReadRF((unsigned char)(R_RX_PAYLOAD), data, 12);
	
		if(((data[0] == myAddr) || (data[0] == BROADCAST) || (data[0] == SYNC)))
		{
			*syncType = data[2];
			*syncSource = data[1];
			*syncVal1 = (data[3] << 24)|(data[4] << 16)|(data[5] << 8)|(data[6]);
			*syncVal2 = (data[7] << 24)|(data[8] << 16)|(data[9] << 8)|(data[10]);
			delay(1);
		
			// printf("\nReturned msgTyp: %#.2x\nReturned SourceAddr: %#.2x\nReturned Payload: %d %d", *syncType, *syncSource, *syncVal1, *syncVal2);
		
			writeReadRF((unsigned char)(FLUSH_RX), data, 1);
			data[0] = 0x40;
			writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
			
			a = 1;
		}
		else
		{
			a = 0;
			delay(1);
		
			// printf("\nReturned msgTyp: %#.2x\nReturned SourceAddr: %#.2x\nReturned Payload: %d %d", *syncType, *syncSource, *syncVal1, *syncVal2);
			
			writeReadRF((unsigned char)(FLUSH_RX), data, 1);
			data[0] = 0x40;
			writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
		}
		//printf("\nDone Reading Message\n");
	}
	else
	{
		a = 0;
	}
		return a;
}

/**************************************************************************
*	sendMessage()
*
*	Puts the device into TX mode, then builds a packet to be transmitted.
*	
*
*	PARAMETERS:
*		Input:	unsigned char the type of message
*				unsigned char destination address
*				int first data value
*				int second data value
*		Output: integer (0) failed transmission (1) successful transmission
*
**************************************************************************/
int sendMessage(unsigned char msgType, unsigned char msgAddr, int msgVal1, int msgVal2)
{
	// printf("\nBeginning sendMessage()");
	unsigned char data[11];
	unsigned char stat;
	int a = 0;
	int ACKVal;
	int j = 0;
	int i;
	
	if((msgVal1 == 0))
	{
		switch(msgType)
		{
			case SET_ADDR:
			case CREATE_ADDR:
			case RECEIVED_ADDR:
			case REJECT_ADDR:
			case SET_FLOW:
			case RETURN_FLOW:
			case ADDR_NOT_SET:
			{
				break;
			}
			default:
			{
				srand(time(NULL));
				msgVal1 = rand();
			}
		}
	}
	
	txMode();
	
	data[0] = msgAddr;
	data[1] = myAddr;
	data[2] = msgType;
	data[3] = (msgVal1 >> 24) & 0xFF;
	data[4] = (msgVal1 >> 16) & 0xFF;
	data[5] = (msgVal1 >> 8) & 0xFF;
	data[6] = (msgVal1) & 0xFF;
	data[7] = (msgVal2 >> 24) & 0xFF;
	data[8] = (msgVal2 >> 16) & 0xFF;
	data[9] = (msgVal2 >> 8) & 0xFF;
	data[10] = (msgVal2) & 0xFF;
	printf("\nSending to %#.2x: %#.2x %d %d\n", msgAddr, msgType, msgVal1, msgVal2);
	stat = writeReadRF((unsigned char)(W_TX_PAYLOAD), data, 12);
	
	digitalWrite(CE, HIGH);
	delayMicroseconds(100);
	digitalWrite(CE, LOW);
	delayMicroseconds(100);
	
	do
	{
		stat = writeReadRF((unsigned char)(NOP), data, 1);
	}while(!(stat & 0x30));
	
	if(stat & 0x10)
	{
		// printf("\nFailed to send");
		data[0] = 0x30;
		stat = writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
		a = 0;
	}
	else
	{
		// printf("\nSend successful");
		data[0] = 0x30;
		stat = writeReadRF((unsigned char)(W_REGISTER|STATUS), data, 2);
		// a = 1;
	}
	rxMode();
	
	switch(msgType)
	{
		case CREATE_ADDR:
		case SET_ADDR:
		case REJECT_ADDR:
		case RECEIVED_ADDR:
		case ADDR_NOT_SET:
		{
			break;
		}
		default:
		{
			j = 0;
			do
			{
				stat = writeReadRF((unsigned char)(NOP), data, 1);
				if(stat & 0x40)
				{
					for(i = 0; i < sizeof(data); i++)
					{
						data[i] = 0;
					}
					for(i = 0; i < sizeof(data); i++)
					{
						data[i] = 0x00;
					}
					writeReadRF((unsigned char)(R_RX_PAYLOAD), data, 12);
	
					if(((data[0] == myAddr) || (data[0] == BROADCAST)))
					{
						ACKVal = (data[7] << 24)|(data[8] << 16)|(data[9] << 8)|(data[10]);
						if((data[1] == msgAddr) && (data[2] == ACK) && (ACKVal == msgVal1) && (data[6] == msgType))
						{
							a = 1;
						}
						else
						{
							a = 0;
						}
					}
				}
				j++;
				delay(18);
			}while((j < 50) && (a == 0));
		}
	}
	
	return a;
}

/**************************************************************************
*	SyncLEDPulse
*
*	Used by the initialization functions. A separate thread blinks the
*	SyncLED until LEDStatus changes. The LED is left ON for successful
*	initialization and OFF for failed initialization.
*
*	PARAMETERS:
*		Input:	none
*		Output: none
*
**************************************************************************/

PI_THREAD(SyncLEDPulse)
{
	do
	{
        
        digitalWrite(SyncLED, HIGH);
        delay( 500);

        
        digitalWrite(SyncLED, LOW);
        delay( 500);
		
		if(LEDStatus==1)
		{
			digitalWrite(SyncLED, LOW);
			//printf("Sync LED ON");
			break;
		}
		else if(LEDStatus==2)
		{
			digitalWrite(SyncLED, HIGH);
			//printf("Sync LED OFF");
			break;
		}
    }while(LEDStatus == 0);
	return 0;
}