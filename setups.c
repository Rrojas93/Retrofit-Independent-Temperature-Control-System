/****************************************************************************************
setups.c
	by group 2: Raul Rojas, Christa McDaniel, Alex Fotso
	
	Objective: 
	This File contains the initialization of hardware as well as the boot and shutdown
	command sequences for the system. 
	
	Functions:
	setups()
	bootSequence()
	shutDownSequence()
	
****************************************************************************************/

#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <mcp23017.h>
#include <time.h>
#include "finalHeader.h"

//GLOBAL VARIABLES
extern int page; // variables to keep track of page and line
extern int line;
extern int lcdhdl;
extern int set;

/****************************************************************************************
void setups(void)
	Description: This function initializes the gpio functionality using the wiringPi 
	library as well as set all used pines to a known default state. The LCD is also 
	initialized here. 
****************************************************************************************/
void setups(void)
{
	int i;
	wiringPiSetupGpio();
	mcp23017Setup (100, 0x20);	//Gpio expander initialization.
	for (i=0;i<8;++i)		//loop to set pin modes for LCD. (on GPIO expander pins)
	{
		pinMode(100+i,OUTPUT);

	}
	//The following sets the gpio's to a default known state. 
	pinMode(FAN, OUTPUT);
	pinMode(COOL, OUTPUT);
	pinMode(HEAT, OUTPUT);
	pinMode(pwrLED, OUTPUT);
	//pinMode(syncLED, OUTPUT);
	pinMode(entSwitch, INPUT);
	pinMode(dwnSwitch, INPUT);
	pinMode(upSwitch, INPUT);
	pinMode(syncSwitch, INPUT);
	pinMode(rstSwitch, INPUT);
	pinMode(pwrSwitch, INPUT);
	
	digitalWrite(FAN, HIGH);
	digitalWrite(COOL, HIGH);
	digitalWrite(HEAT, HIGH);
	digitalWrite(pwrLED, HIGH);
	//digitalWrite(syncLED, HIGH);
	digitalWrite(pwrLED, LOW);

	lcdinitialization();
	
	
	return;
}

/****************************************************************************************
void bootSequence(void)
	Description: This function initializes all arrays used in this program to a known 
	state of 0. As well as perform an LCD test. 
****************************************************************************************/
void bootSequence(void)
{
	//lcdBoot();
	initArray();
	initTimeArr();
	initRegFlow();
	initTempsArr();
	initRocArray();
	
	return;
}

/****************************************************************************************
void shutDownSequence(void)
	Description: This function sets all led off and clears the LCD before the program 
	terminates. 
****************************************************************************************/
void shutDownSequence(void)
{
	//turn off all LED's, relay signals, and clear lcd.
	lcdClear(lcdhdl);
	digitalWrite(pwrLED, HIGH);
	digitalWrite(COOL, HIGH);
	digitalWrite(HEAT, HIGH);
	digitalWrite(FAN, HIGH);
	return;
}

