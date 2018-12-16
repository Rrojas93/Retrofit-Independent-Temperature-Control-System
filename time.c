/****************************************************************************************
time.c
	by group 2: Raul Rojas, Christa McDaniel, Alex Fotso
	
	Objective: 
	This file is meant to handle all functions which are time related for the Master 
	Controller. The functions here provide the needed functionality for the system 
	to keep up with current time and also provide some time related functions to the 
	system for background computation purposes. 
	
	Functions:
	getSec()
	initTimeArr()
	updateTime()
	parseTime()
	rtTimerStart()
	rtTimerEnd()
	
****************************************************************************************/

#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <mcp23017.h>
#include <time.h>
#include "finalHeader.h"

int seconds;
int timeFlag = 0;
extern int page;

/****************************************************************************************
int getSec(void)
	Description: This function returns the current seconds from the time struct created 
	by time_t provided by <time.h>. the function first retrieves the epoch time, then 
	formats it into a structure containing hours, minutes, seconds, etc. This function 
	only retrieves the seconds. 
****************************************************************************************/
int getSec(void)
{
	time_t rawtime;
	struct tm *timestruct;
	
	time(&rawtime);	//retrieve epoch time. 
	timestruct = gmtime(&rawtime);	//organize epoch time into a struct
	seconds = timestruct->tm_sec;	//retrieve only the seconds of the struct.
	
	return seconds;
}

/****************************************************************************************
void initTimeArr()
	Description: This function initializes the array mcClock[3] which is created in the
	global variables. This funciton will be used to keep track of real time.
	It fills the array with predefined values (most likely 0's but these values can be 
	changed for testing pursposes.)
****************************************************************************************/
int mcClock[4];	//array to store time. [hour(0) | minute(1) | seconds(2) | am/pm(3]

/****************************************************************************************
void initTimeArr()
	Description: This function initializes the array, which will contain the values 
	of the MC's real time clock, into a known state.
****************************************************************************************/
void initTimeArr()
{
	int i;
	for(i=0;i<3;i++)
	{
		if(i==0)
			mcClock[i] = 12;
		else
			mcClock[i] = 59;
	}
	mcClock[arrAP] = 0;
	return;
}

/****************************************************************************************
void updateTime(void)
	Description: This function provides a quick single iteration method of updating the 
	real time clock. It retrieves and compares the seconds count from getSec() function 
	with the value stored in the seconds index in the array mcClock[]. If this value 
	is equal to 0, than the minutes need to be incrimented. However, it only does this if
	the value retrieved from getSec() is not equal to the stored seconds. This is done
	so that it only incraments the minutes once. once the minute count re
****************************************************************************************/
void updateTime(void)
{	
	if(page == 4)//if we are setting the time, do not adjust time values 
	{
		return;
	}
	int currSec = getSec();//retrieve the current epoch seconds.
	if(currSec == 0 && currSec != mcClock[arrSec])
	{//if the seconds make a transition from 59 to 0 then the minute must be adjusted. 
		
		if(mcClock[arrMin] == 59)
		{//if the minute makes a transition from 59 to 0, the hour must be adjusted
			timeFlag = 1;//This flag signals that the lcd needs to be updated to reflect the new time. 
			mcClock[arrMin] = 0;//reset minutes back to 0 if we incrimented from minute 59.
			if(mcClock[arrHr] == 12)
			{//if the hour is 12 we need to reset it back to 1 like in standard time. 
				mcClock[arrHr] = 1;
				
			}
			else//just incriment the hour if it is not 12. 
			{
				mcClock[arrHr]++;
				if(currSec == 0 && currSec != mcClock[arrSec] && mcClock[arrHr] == 12)
				{//if we are at hour 12 for the first time we need to adjust the am/pm setting.
					mcClock[arrAP] = (mcClock[arrAP]+1)%2;
				}
			}
		}
		else//if the minute is not 59 then just increment it.
		{
			timeFlag = 1;//Flag which signals that the lcd needs to be updated to reflect the new time. 
			mcClock[arrMin]++;
		}
		mcClock[arrSec] = currSec;//store the returned seconds from epoch time. 
	}
	else
	{
		mcClock[arrSec] = currSec;
	}
	
	
	//parseTime();
	if(timeFlag && page == 0)//if the time changes (hour and/or minute) the lcd needs to be updated.
	{
		dispTime();
		timeFlag = 0;
	}
	//printf("Current Time: %d:%d.%d %d\n", mcClock[arrHr], mcClock[arrMin], mcClock[arrSec], mcClock[arrAP]);
	return;
}

/****************************************************************************************
void parseTime(void)
	Description: This function parses the hour and minute into individual single digit 
	values so that they may be used for other conditions if necessary or to display 
	the time as independant digits. 
****************************************************************************************/
int parsedTime[4];
void parseTime(void)
{
	int hr = mcClock[arrHr];//get the current MC time.
	int min = mcClock[arrMin];//get the current MC time. 
	int hrT;
	int hrO;
	int minT;
	int minO;
	
	if(hr >= 10)//if the hour is a double digit number.
	{
		hrT = 1;//store the tenths place.
		hrO = hr % 10;//retrieve the one's place.
	}
	else//the hour is a single digit number. 
	{
		hrT = 0;//store the tenths place
		hrO = hr;//store the one's place. 
	}
	
	if(min >= 10)//if the minute is a double digit number. 
	{
		minT = min/10;//devide the minutes by ten to get the tenths place.
		minO = min%10;//retrieve the one's place of the minute. 
	}
	else//the minute is a single digit number. 
	{
		minT = 0;//store the tenth's place.
		minO = min;//store the one's place. 
	}
	
	//store the parsed time. 
	parsedTime[0] = hrT;
	parsedTime[1] = hrO;
	parsedTime[2] = minT;
	parsedTime[3] = minO;
	
	//printf("mcClock hr: %d\n\n", mcClock[arrHr]);
	printf("Parsed Time: %d%d:%d%d\n",parsedTime[0],parsedTime[1],parsedTime[2],parsedTime[3]);
	
	return;
}

/****************************************************************************************
void rtTimerStart(int timerNum, int timerType)
	Description: This function stores the current time into an array in total seconds
	which will be used to determine the time passed (in real time) when the timer is 
	stopped. 
****************************************************************************************/
int rtTimes[126][2];//column 1 is for thermostat timers, column 2 are for anything else.
void rtTimerStart(int timerNum, int timerType)
{
	int startSec;
	/*
	rtTimes[0] = mcClock[arrHr];
	rtTimes[1] = mcClock[arrMin];
	rtTimes[2] = mcClock[arrSec];
	rtTimes[3] = mcClock[arrAP];
	*/
	
	//calculating the start time. 
	startSec = (mcClock[arrHr]*3600)+(mcClock[arrMin]*60)+mcClock[arrSec];
	if(mcClock[arrAP])
	{//if the time of day is PM then add appropriate seconds. 
		startSec = startSec + (mcClock[arrHr]*12);
	}
	rtTimes[timerNum][timerType] = startSec;//store the start time. 
	return;
}

/****************************************************************************************
int rtTimerEnd(int timerNum, int timerType)
	Description: This function ends a timer. It takes the start time which was stored 
	in the rtTimerStart() function and calculates how much time has passed since then. 
	The array which contains the timers supports two types of timers, one for the 
	temperature rate of changes, and the other for miscellaneous use. 
****************************************************************************************/
int rtTimerEnd(int timerNum, int timerType)
{
	int startSec;
	int endSec;
	int resultSec;
	//3600 seconds in one hour.
	startSec = rtTimes[timerNum][timerType];//retrieve the start time for this timer.
	
	endSec = (mcClock[arrHr]*3600)+(mcClock[arrMin]*60)+mcClock[arrSec];//calculate current time in seconds.  
	if(mcClock[arrAP])//if the hour is passed 12, add 12 hours worth of seconds to the end time. 
	{
		endSec = endSec + (mcClock[arrHr]*12);
	}
	
	//determine elapsed time. 
	if(endSec > startSec)
	{
		resultSec = endSec-startSec;
	}
	else
	{
		resultSec = startSec - endSec;
	}
	//rtTimes[timerNum][timerType] = 0;//reset the timer to 0; 
	return resultSec;
}