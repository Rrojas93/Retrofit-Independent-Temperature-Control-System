/*****************************************************************************************
finalMain.c
	by group 2: Raul Rojas, Christa McDaniel, Alex Fotso
	
	Objective: 
	This file Contains the main of the program used by the Master Controller. It runs in 
	a main loop which calls multiple different functions which provide the funcitonality 
	necessary to achieve the systems tasks.
	
	Functions: 
	PI_THREAD (myThread)
	PI_THREAD (displayThread)
	main()
	
*****************************************************************************************/
#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <mcp23017.h>
#include "finalHeader.h"
#include "messaging.h"


//GLOBAL VARIABLES
int page; // variables to keep track of page and line
int line;
int lcdhdl;
int set;
int entry;
int hour;
int min;
extern unsigned char newTherm;
extern unsigned char newReg;
extern unsigned char addresses[126];
extern unsigned char devices[rows][columns];
extern int mcClock[3];
int devNumber;
int day;
char dayTime[2];
extern int hvacSetting;
extern int temps[126][5];
extern int failedCon[126][126];
extern int regFlow[126][126][2];
extern int hvacAuto;
extern int hvacStatus;
int mainHalt = 0; //used to halt the main program for alternate threads while they complete critical tasks. (adding devices)
int threadGreenLight = 0; //gives permission to threads to coninue once the main reaches a safe stopping point for mainHalt usage.

/*****************************************************************************************
PI_THREAD (myThread)
	Description: This thread is used to update the real time clock the the master control. 
	A seperate thread was created for this function so that the real time clock can be 
	updated accurately without any delay and time miscalculations that could be 
	caused by other function delay or lag. 
*****************************************************************************************/
PI_THREAD (myThread)
{
	while(1)
	{
		delay(20);
		updateTime();
	}
}

/*****************************************************************************************
PI_THREAD (displayThread)
	Description: This thread is used to update the lcd display on the master control. 
	We used a seperate thread for this function to avoid any latency that could occur 
	from user operation and navigation that could be caused by an other time consuming 
	functions. 
*****************************************************************************************/
PI_THREAD (displayThread)
{
	while(1)
	{
		delay(20);
		updateDisplay();
	}
}

int main(void)
{
	//the following assignments are for initializing variables to a know state. 
	dayTime[0] = 'A';
	dayTime[1] = 'P';
	day = 0;
	set = 0;
	entry = 0;
	hour = 0;
	
	setups();//setup the wiringPi library so that we may have control over GPIO's
	//lcdClear(lcdhdl);
	while(digitalRead(pwrSwitch))// wait until the power switch is turned on to continue.
	{
		delay(500);
		printf("Turn on power Switch.\n");
	}
	
	bootSequence(); //This function initializes all the necessary arrays and performs an lcd test.
	initMC(&devNumber, addresses);//performs a system sync for all system devices.
	
	while(!digitalRead(syncSwitch))//waits until the sync switch is released from the initMC function
	{
		delay(20);
	}
	page = 4;//we start at page 4 so that the user may input the system time of the system. 
	line = 0;
	display(page, line, set);

	populateArrays(); //this function orginizes the addresses gathered form initMC()
	
	//start threads
	if(piThreadCreate(myThread))
	{
		printf("myThread did not start.\n");
	}
	if(piThreadCreate(displayThread))
	{
		printf("displayThread did not start.\n");
	}
	
	/* //===THE FOLLOWING ASSIGNMENTS ARE FOR TESTING PURPOSES.===
	hvacSetting = 0;	//auto: 0, cooling: 1, heating: 2, fan: 3
	temps[1][setTemp] = 76;
	temps[1][currTemp] = 78;
	temps[2][setTemp] = 70;
	temps[2][currTemp] = 72;
	
	int room1Set;
	int room1Cur;
	int room2Set;
	int room2Cur;
	
	printf("enter room1Set: ");
	scanf("%d",&room1Set);
	printf("enter room2Set: ");
	scanf("%d",&room2Set);
	temps[1][setTemp] = room1Set;
	temps[2][setTemp] = room2Set;
	*/ //===END OF TEST ASSIGNMENTS===
	
	
	//MAIN LOOP BEGINS HERE!
	while(!digitalRead(pwrSwitch))
	{
		if(mainHalt)//if the user initiated an "add device", halt main until done.
		{
			printf("MAIN HALTED.\n");
			threadGreenLight = 1;//give the thread permission to continue.
			while(mainHalt)//wait until the thread releases the main halt. 
			{
				delay(200);
			}
			threadGreenLight = 0;//once the thread has finished, continue main routines.
		}
		
		retrieveTemps();//retrieve the temeratures from the thermostat devices. 
		delay(1500);
		/* //===THE FOLLOWING SEGMENT OF CODE IS FOR TESTING PURPOSES===
		//printf("enter room1Cur: ");
		//scanf("%d",&room1Cur);
		//temps[1][currTemp] = room1Cur;
		//printf("enter room2Cur: ");
		//scanf("%d",&room2Cur);
		//temps[2][currTemp] = room2Cur;
		*/ //===END OF TEST CODE SEGMENT===
		
		calcDiff();//determine temperature difference and if the hvac needs to turn on. 
		hvacControl();//turn on/off the hvac system if needed and calculate rates of change.
		setReg();//set the registers where they need to be. 
		
		
		//break;
	}
	
	
	shutDownSequence();//sequency which turns off all LED's and clears the LCD. 
	return 0; 
}