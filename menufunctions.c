#include <wiringPi.h>
#include <lcd.h>
#include <stdio.h>
#include <mcp23017.h>
#include "finalHeader.h"

//GLOBAL VARIABLES
extern unsigned char devices[rows][columns];
extern int failedCon[126][126];
extern int page; // variables to keep track of page and line
extern int line;
extern int set;
extern int conStatus;
extern int lcdhdl;
extern int hour;
extern int min;
extern int day;
extern char dayTime[2];
extern int entry;
extern int mcClock[3];
int lcdBusy;
int holdFlag = 0;
extern int hvacSetting;
int toRoom = 1;
extern int mainHalt; //used to halt the main program while alternate threads while they complete critical tasks. (adding devices)
int threadGreenLight; //gives permission to threads to coninue once the main reaches a safe stopping point for mainHalt usage.
int buttonPressed;

/**********************************************************************
<void updateDisplay>
	Description: This function directly communicates with the up, down,
	sync, and enter button in other to change the difeerent lcd pages 
	and displays it on the lcd.
************************************************************************/
void updateDisplay(void)
{
		//-Display section
	//-come back to this section each time a line or a page is updated
	if(!buttonPressed)
	{
		buttonPressed = smartDelay(20);
	}
	
	
	if(buttonPressed == 0)
	{
		return;
	}
	else if(buttonPressed == syncSwitch)
	{
		page = 7;
		line = 0;
	}

	printf("\nFor logic:\nPage: %d\nLine: %d\n", page,line);
	switch(page)
		{
			case 0:
				page0();//MainDisplay
				break;
			case 1:
				page1();//settings
				break;
			case 2:
				page2();//settingsConfig
				break;
			case 3:
				page3();//errorPage1
				break;
			case 4:
				page4();//setTimeDisplay
				break;
			case 5:
				page5and6();//errorPage2
				break;
			case 6:
				page5and6();//warningPage
				break;	
			case 7:
				page7();//syncMenu
				break;
			case 8:
				page8();//manageDeviceMenu
				break;
			case 9:
				page9();//addDeviceMenu
				break;
			case 10:
				page10();//removeDeviceMenu
				break;
          	case 11:
				page11();//remDevWarn
				break;
			case 12: 
				page12();//removing devices action
				break;
			case 13:
				page13();//Room select for addReg.
				break;
		}	
			
	display(page, line, set);
	buttonPressed = 0;
	return;
	
}

/**********************************************************************
void lcdInitialization(void)
	Description: This function initialises the Lcd by calling the LCD 
	Init function of lcd.h. It also creates the LCD handler variable
	that will be used to control the lcd display
************************************************************************/
void lcdinitialization(void)
{
   //LCD initialization and handle set up
	lcdhdl = lcdInit(2,16,4,LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7,0,0,0,0);
	return;
}

/********************************************************************
void lcdBoot(void)
	Description: This function is an introductory page which shows 
	the name, of all the group members, class, and semester. This is 
	the page been displayed each time the device boots in other to 
	provide a brief intro to the user 
********************************************************************/
void lcdBoot(void)
{
	lcdClear(lcdhdl);
	lcdPosition(lcdhdl,0,0);			//Postion cursor on the first line in the first column
	lcdPuts(lcdhdl,"Christa McDaniel");		//prints our names, class, term, and group number
	delay(1000);
	lcdPosition(lcdhdl,0,1);
	lcdPuts(lcdhdl,"Raul Rojas      ");
	delay(1000);
	lcdPosition(lcdhdl,0,0);
	lcdPuts(lcdhdl,"Alex Fotso      ");
	delay(1000);
	lcdPosition(lcdhdl,0,0);
	lcdPuts(lcdhdl,"ECET 4720-Group2");
	lcdPosition(lcdhdl,0,1);
	lcdPuts(lcdhdl,"Fall 2016       ");
	delay(2000);
	lcdClear(lcdhdl);				//clears LCD
	delay(2000);
	
	lcdClear(lcdhdl);
	lcdPosition(lcdhdl,0,0);
	lcdPuts(lcdhdl,"Syncing.. Press");
	lcdPosition(lcdhdl,0,1);
	lcdPuts(lcdhdl,"Sync to finish.");


	return;
}

void dispTime(void)
{
	while(lcdBusy)
	{}
	lcdBusy = 1;
	lcdPosition(lcdhdl,0,0);
	lcdPrintf(lcdhdl, "Time: %02d:%02d %cM", mcClock[arrHr], mcClock[arrMin], dayTime[day]);
	lcdBusy = 0;
	return;
}
/********************************************************************
void mainDisplay(void)
	Description: This function provides the lcd page for the default 
	display of the master control. This is the display when the
	master cpntrol is in stand by mode or doing nothing. It displays 
	The time and the connection statue of the Master Control
********************************************************************/
void mainDisplay(void)
{
	lcdClear (lcdhdl);
	lcdPosition(lcdhdl,0,0);
	lcdPrintf(lcdhdl, "Time: %02d:%02d %cM", mcClock[arrHr], mcClock[arrMin], dayTime[day]);
	if(conStatus)
	{
		lcdPosition(lcdhdl,0,1);
		lcdPrintf(lcdhdl, " Status: Errors ");
	}
	else
	{
		lcdPosition(lcdhdl,0,1);
		lcdPrintf(lcdhdl, "   Status: Ok   ");
	}
	//lcdPosition(lcdhdl,14,0);
	return;
}

/********************************************************************
void lcdInitialization(void)
	Description: This function displays the different options the 
	user has at his disposal when using the master control. This 
	functions include taking direct control of the HVAC and 
	displaying the Connectiom errors or warnings. 	
*********************************************************************/
void settings(int lines)
{
	lcdClear(lcdhdl);
	if (lines == 0)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> HVAC Settings");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Error Page");
	}
	
    else if (lines == 1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   HVAC Settings");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Error Page");
    }
	
	else if(lines == 2)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Set Time");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "         BACK ");
    }
	
	else if(lines == 3)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Set Time");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "      -> BACK ");
    }
	else if(lines > 3 || lines < 0)
	{
		printf("line out of range in settings page.\n");
	}
		 
    return;
}

/********************************************************************
void settingConfig(int)
	Description: This function displays the setting details when the 
	setting option is selected form the HVAC setting page. 
	It provides the user with the cooling or the heating option  
********************************************************************/
void settingConfig(int lines)
{
	lcdClear(lcdhdl);
	if (lines == 0)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Heating");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Cooling");
	}
	
    else if (lines ==1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Heating");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Cooling");
    }
	
	else if (lines == 2) 
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Fan");
		lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Auto");
	}
	else if(lines == 3)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Fan");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Auto");
    }
	else if(lines == 4)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Auto");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "     -> BACK");
	}
	else if(lines > 4 || lines < 0)
	{
		printf("line out of range in settingsConfig page.\n");
		
	}
	return;
}

/********************************************************************
void errorPage1(void)
	Description: This function is the display when the error option
	is selected from the Settung page. It provided the user with the
	option of displaying warning messages or connection errors 
	messages
********************************************************************/
int errors = 0;
void errorPage1(int lines)
{
	errors = countErrors();
	lcdClear(lcdhdl);
	int warnings = failedCon[0][1];
	if (lines == 0)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Errors: %d",errors);
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Warnings: %d",warnings);	
	}
    else if(lines == 1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Errors: %d",errors);
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Warnings: %d",warnings);	
    }
	else if(lines == 2)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Warnings: %d",warnings);
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "     -> BACK");
    }
	else if(lines > 2 || lines < 0)
	{
		printf("line out of range in errorPage1 page.\n");
	}
	return;
}

/********************************************************************
void errorPage2(void)
	Description: This function displays the error messages. When the
	user is done reading, he presses the enter key in other to return
	to the main display
*********************************************************************/
void errorPage2(void)
{
	lcdClear(lcdhdl);
	lcdPosition(lcdhdl,0,0);
	lcdPrintf(lcdhdl, "Connect. failed with %d devices", errors);	
	return;
}

/********************************************************************
void warningPage(void)
	Description: This function displays the warning generated.
	The user has to press the enter key when he is done reading in 
	other to return to the main menu 
**********************************************************************/
void warningPage(void)
{
	if(failedCon[0][1] == 1)
	{
		lcdClear(lcdhdl);
		lcdPosition(lcdhdl,0,0);
		lcdPrintf(lcdhdl, "Cannot achieve");
		lcdPosition(lcdhdl,0,1);
		lcdPrintf(lcdhdl, "desired temps");	
		delay(1500);
		lcdClear(lcdhdl);
		lcdPosition(lcdhdl,0,0);
		lcdPrintf(lcdhdl, "Update Hvac ");
		lcdPosition(lcdhdl,0,1);
		lcdPrintf(lcdhdl, "Settings.");	
		delay(1500);
		page = 2;
		line = 0;
		lcdBusy = 0;
		display(page, line, set);
	}
	
	return;
}

/********************************************************************
void display(int, int)
	Description: This function gives priority to the page the user 
	calls. It eases the use of the Update display function 	
********************************************************************/

void display(int pages, int lines, int time)
{
	while(lcdBusy)
	{}
	lcdBusy = 1;
	printf("For Display:\nPage: %d\nLine: %d\n", page,line);
	switch(pages)
	{
		case 0:
			mainDisplay();
			break;
		case 1:
			settings(lines);
			break;
		case 2:
			settingConfig(lines);
			break;
		case 3:
			errorPage1(lines);
			break;
		case 4:
			setTimeDisplay(time);
			break;
		case 5:
			errorPage2();
			break;
		case 6:
			warningPage();
			break;	
		case 7:
			syncMenu(lines);
			break;
		case 8:
			manageDeviceMenu(lines);
			break;
		case 9:
			addDeviceMenu(lines);
			break;
		case 10:
			removeDeviceMenu(lines);
			break;            		
		case 11:
			remDevWarn();
			break;
		case 12:
			SuccessMessage(0,page);
			break;
		case 13:
			toRoomMenu();
			break;
			
	}
	lcdBusy = 0;
	return;
}

/********************************************************************
int smartDisplay(int)
	Description: This function creates a delay while listening to
	switches. In case a switch is pressed, the delay is aborted and 
	the pin number of the switched pressed is returned 
********************************************************************/
int smartDelay(int i)
{
	int j = 0;
	int buttonPressed = 0;//return variable.
	if(i<20)i=20;//ensure at least 20ms as minimum delay.
	i=i/20;//converting time into loop iterations
	
//iterates i times and exits at i=0 or if power switch is off
	while(i != 0 && !digitalRead(pwrSwitch))
	{
		if(!digitalRead(entSwitch))
		{
			//wait untill button is released.
			while(!digitalRead(entSwitch))
			{
				delay(20);
			}
			buttonPressed=entSwitch;
		}
		else if(!digitalRead(dwnSwitch))
		{
			while(!digitalRead(dwnSwitch))
			{
				if(page ==4)
				{
					if(j >= 25)
					{
						holdFlag = 1;
						delay(100);
						if (set == 0)
						{
							if (hour == 1)
							{
								hour = 12;
							}
							else 
							{
								hour = (hour - 1)%13;
							}
							mcClock[arrHr] = hour;			
						}
						else if (set ==1)
						{
							if (min == 0)
							{
								min = 59;
							}
							else
							{
								min = (min - 1)%60;
							}				
							mcClock[arrMin] = min;	
						}
						setTimeDisplay(set);
					}
					else
					{
						j++;
					}
				}
				
				delay(20);
			}
			holdFlag = 0;
			j=0;
			buttonPressed=dwnSwitch;
		}
		else if(!digitalRead(upSwitch))
		{
			while(!digitalRead(upSwitch))
			{
				if(page ==4)
				{
					if(j >= 25)
					{
						holdFlag = 1;
						delay(100);
						if (set==0)
						{
							if(hour == 12)
							{
								hour = 1;
							}
							else
							{
								hour = (hour +1)%13;
							}
							mcClock[arrHr] = hour;
						}
						else if(set == 1)
						{
							min = (min+1) % 60;
							mcClock[arrMin] = min;
						}
						setTimeDisplay(set);
					}
					else
					{
						j++;
					}

				}
				delay(20);
			}
			holdFlag = 0;
			j = 0;
			buttonPressed=upSwitch;
		}
		else if(!digitalRead(syncSwitch))
		{
			while(!digitalRead(syncSwitch))
			{
				delay(20);
			}
			buttonPressed=syncSwitch;
		}
		else if(!digitalRead(rstSwitch))
		{
			while(!digitalRead(rstSwitch))
			{
				delay(20);
			}
			buttonPressed=rstSwitch;
		}
		//if a button was pressed exit loop.
		if(buttonPressed!=0)
		{
			break;
		}
		else
		{
			//decriment i and delay 20ms.
			i--;
			delay(20);
		}
	}
	
	//return the pin # of button pressed or 0 if none pressed.
	return buttonPressed;
}


/********************************************************************
void syncMenu(int)
	Description: This function mainly displays the setting options 
	when the sync button is pressed. The mainoptions it proposses 
	are Manage Device option and the Re-initialisation option
********************************************************************/
void syncMenu(int lines)
{
	lcdClear(lcdhdl);
	if (!lines)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Manage Devices");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Re-initialize");
	}
	
    else if (lines == 1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Manage Device");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Re-initialize");
    }   
	else if(lines == 2)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Re-initialize");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "     ->  Back");
    }
	else if(lines > 2 || lines < 0)
	{
		printf("line out of range in syncMenu page.\n");
	}
	return;
}

/***********************************************************************
<manageDeviceMenu>
     Description: This function presents the user with the option of 
	 managing the connected devices. He has the choice of either 
	 adding a device or removing a device. By device we mean thermostat
	 an Registers 
************************************************************************/
void manageDeviceMenu(int lines)
{
	lcdClear(lcdhdl);
	if (lines == 0)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Add Device");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Remove Device");
	}
	
    else if(lines == 1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Add Device");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Remove Device");
    }   
	else if(lines == 2)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Remove Device");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "     -> BACK");
    }
	else if(lines > 2 || lines < 0)
	{
		printf("line out of range in manageDeviceMenu page.\n");
	}
	return;
}

/********************************************************************
<addDeviceMenu>
	Description: This function provides the menu display for the 
	user to select between adding a thermostate or adding a register. 
********************************************************************/
void addDeviceMenu(int lines)
{
	lcdClear(lcdhdl);
	if (!lines)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Add Register");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Add Thermo");
	}
	
    else if (lines == 1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Add Register");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Add Thermo");
    }
	else if(lines == 2)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   ADD Thermo");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "     -> BACK");
    }
	else if(lines > 2 || lines < 0)
	{
		printf("line out of range in settings page.\n");
	}
	return;
}

void remDevWarn (void)//page 11
{
	lcdClear(lcdhdl);
	lcdPosition(lcdhdl,0,0);
	lcdPrintf(lcdhdl, "Remove Unconnect-ed devices?");	
	return;
}
/********************************************************************
void removeDeviceMenu(int)
	description: This function provides the lcd display for the user
	to remove a device. It presents the number of device disconected, 
	and this corresponds to the actual number the user has 
	disconnected then, he can select the yes option to remove all
	those devices. he can also selects the No option which will
	bring him back to the Home menu
*********************************************************************/
void removeDeviceMenu(int lines)//page 10
{
	/*lcdClear(lcdhdl);
	if (!lines)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Remove Register");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "   Remove Thermo");
	}
	
    else
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "   Remove Register");
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "-> Remove Thermo");
    }*/
	lcdClear(lcdhdl);
	int critErrors = countErrors();
	if (!lines)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "Remove %d devices", critErrors);
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "->YES         NO");
	}
	
    else if(lines ==1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "Remove %d devices", critErrors);
	    lcdPosition(lcdhdl,0,1);
	    lcdPrintf(lcdhdl, "  YES       ->NO");
    }
	else if(lines > 1 || lines < 0)
	{
		printf("line out of range in settings page.\n");
	}
	return;
}

/********************************************************************
<function name>
	<description>
********************************************************************/
void SuccessMessage(int lines, int pages)
{
	//printf("in successMessage()\n");
   if (pages == 2)
	{
		if (lines == 0)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "Heating         configured");
			hvacSetting = 2;
			//digitalWrite(FAN, 0);
			//digitalWrite(COOL, 1);	 	 
			//digitalWrite(HEAT, 0);	 
		}		
		else if(lines == 1)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "Cooling         Configured ");
			hvacSetting = 1;
			//digitalWrite(FAN, 0);
			//digitalWrite(HEAT, 1);	 	 
			//digitalWrite(COOL, 0);	 	  
		}
		else if(lines ==2)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "Fan Configured ");
			hvacSetting = 3;
			//digitalWrite(HEAT, 1);
			//digitalWrite(COOL, 1);	 	 
			//digitalWrite(FAN, 0);	 
		}
		else if(lines == 3)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "Auto Configured ");
			hvacSetting = 0;
		}
		else if(lines > 3 || lines < 0)
		{
			printf("line out of range in SuccessMessage page 2.\n");
		}
		printf("hvacSetting: %d\n", hvacSetting);
	}
	
	else if(pages == 9)
	{
		if (lines == 0)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "  Register Added");
			lcdPosition(lcdhdl,0,1);
			lcdPrintf(lcdhdl, " Successfully "); 
		}
		else if(lines ==1)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "  Thermostate Added ");
			lcdPosition(lcdhdl,0,1);
			lcdPrintf(lcdhdl, "    Successfully "); 	 	  
		}
		else if(lines == 2)//failed to add device message.
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "Failed to add");
			lcdPosition(lcdhdl,0,1);
			lcdPrintf(lcdhdl, "Device."); 
		}
		else if(lines == 3)//instruction message for add device.
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "Press sync");
			lcdPosition(lcdhdl,0,1);
			lcdPrintf(lcdhdl, "button to cancel"); 
		}
		else if(lines == 4)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "No thermostats");
			lcdPosition(lcdhdl,0,1);
			lcdPrintf(lcdhdl, "to add to."); 
		}
		else if(lines > 4 || lines < 0)
		{
			printf("line out of range in SuccessMessage page 9.\n");
		}
	}
	else if (pages == 12)
	{
		if (lines == 0)
		{
			lcdClear (lcdhdl);
			lcdPosition(lcdhdl,0,0);
			lcdPrintf(lcdhdl, "Devices Removed");
			lcdPosition(lcdhdl,0,1);
			lcdPrintf(lcdhdl, "Successfully "); 
			page = 0;
			line = 0;
		}	
		else if(lines > 0 || lines < 0)
		{
			printf("line out of range in SuccessMessage page 10.\n");
		}
		
	}	
	delay(2000);
	return;
}

/*void setTimeDisplay(int timeSet)
{
	// We use the global variables for time created above in other to display the set time.
	lcdClear(lcdhdl);
	
	if (timeSet == 0)
	{
		lcdPosition(lcdhdl,3,1);
	    lcdPrintf(lcdhdl, "^");
	    lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "  %02d : %02d  %cM", hour, min, dayTime[day]);
	}
	
    else if (timeSet == 1)
	{
		lcdPosition(lcdhdl,8,1);
	    lcdPrintf(lcdhdl, "^");
	    lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "  %02d : %02d  %cM", hour, min, dayTime[day]);
    }
	else
	{
		lcdPosition(lcdhdl,11,1);
	    lcdPrintf(lcdhdl, "^");
	    lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "  %02d : %02d  %cM", hour, min, dayTime[day]);
    }
	
}*/

void setTimeDisplay(int set)
{
	// We use the global variables for time created above in other to display the set time.
	if(holdFlag)
	{
		lcdPosition(lcdhdl,0,1);
		lcdPrintf(lcdhdl, "  %02d : %02d  %cM", mcClock[arrHr], mcClock[arrMin], dayTime[day]);
		return;
	}
	
	lcdClear(lcdhdl);
	lcdPosition(lcdhdl,0,1);
	lcdPrintf(lcdhdl, "  %02d : %02d  %cM", mcClock[arrHr], mcClock[arrMin], dayTime[day]);
	
	if (set == 0)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "Set Hour");
	}
	
    else if (set == 1)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "Set Minutes");
    }
	else if(set == 2)
	{
		lcdPosition(lcdhdl,0,0);
	    lcdPrintf(lcdhdl, "-> Set AM/PM");
    }
	else if(set > 2 || set < 0)
	{
		printf("set out of range in setTimeDisplay.\n");
	}
	return;
}

void toRoomMenu(void)
{
	lcdPosition(lcdhdl,0,0);
	lcdPrintf(lcdhdl, "Add register to ");
	lcdPosition(lcdhdl,0,1);
	lcdPrintf(lcdhdl, "room:   %d <-   ", toRoom);
	
	return;
}

void page0(void)//mainDisplay
{
	
	//use smart display to detect if any switch is pressed
		//take action only if the enter key is pressed
		//When enter switch is pressed, we update the page and go back to the display section for the new display
		
		if(buttonPressed == entSwitch)
		{
			page = 1;
		}
		return;
}
void page1(void)//settings
{
			// if enter key is pressed, we update the page and go to display section to show the new display
		//if the up or down button is pressed, we update the line and go back to the display to outline the new line
	if (buttonPressed==entSwitch)
	{
		if ( line == 0)
		{
			page = 2;
		}
						
		else if (line == 1) 
		{
			page = 3;
		}
			
		else if (line == 2)
		{
			page = 4;
		}
			
		else if (line == 3) // Back line, if pressed we return to previous screen
		{
			page = 0;
		}
		
		line = 0;   				
	}
	 
	
	 else if(buttonPressed == dwnSwitch)
	{
		line = (line+1) % 4;// on this page, we have 3 lines. so, we odate the lines using mod 3
	}
	 
	else if(buttonPressed == upSwitch)
	{  
		if(line == 0 ) 
			line = 3;
		else 
			line = (line-1) % 4;
	}
	return;
	
}
void page2(void)//settingsConfig
{
	// if the enter key is pressed, the line at which it is pressed is taken in concideration
	if (buttonPressed == entSwitch)
	{	
		// This function displays the cooling, heating, or fan success message based on which ever line the cursor is at.
		// it also activates the HVAC heating, cooling, or fan system. according to which line the cursor is at			
		if (line != 4)
		{
			SuccessMessage(line, page); 
			page = 0;
			line = 0;
		}
		else if (line == 4) 
		{
			page = 1;
		}
			
		line =0;
	}
	else if(buttonPressed == dwnSwitch)
	{
		line = (line+1) % 5;// on this page, we have 3 lines. so, we odate the lines using mod 3
	}
	 
	else if(buttonPressed == upSwitch)
	{  
		if(line == 0 ) 
		{
			line = 4;
		}
		else 
		{
			line = (line-1) % 5;
		}
	}
	return;
}
void page3(void)//errorPage1
{
	if (buttonPressed==entSwitch)
	{
		if ( line == 0)
		{
			page = 5;
			
		}
		else if( line == 1) 
		{
			page = 6; 
			
		}
		else if(line == 2)
		{
			page = 1;
			
		}
			
		
		line = 0;			
			
	}
	
	else if(buttonPressed == dwnSwitch)
	{
		line = (line+1) % 3;// on this page, we have 3 lines. so, we odate the lines using mod 3
	}
	 
	else if(buttonPressed == upSwitch)
	{  
		if(line == 0 ) 
			line = 2;
		else 
			line = (line-1) % 3;
	}
	return;
}
void page4(void)//setTimeDisplay
{
		
	hour = mcClock[arrHr];
	min  = mcClock[arrMin];
	day = mcClock[arrAP];
	
	if (buttonPressed==entSwitch)
	{
		if (set >= 2)
		{
			page = 0;
			set = 0;
			line = 0;

			mcClock[arrHr] = hour;
			mcClock[arrMin] = min;
			
		}
		else 
		++set;	 				
	}
	else if(buttonPressed == upSwitch)
	{					
		if (set==0)
		{
			if(hour == 12)
			{
				hour = 1;
			}
			else
			{
				hour = (hour +1)%13;
			}
			mcClock[arrHr] = hour;
		}
		else if(set == 1)
		{
			min = (min+1) % 60;
			mcClock[arrMin] = min;
		}
			
	}
	else if (buttonPressed == dwnSwitch)
	{
				
		if (set == 0)
		{
			if (hour == 1)
			{
				hour = 12;
			}
			else 
			{
				hour = (hour - 1)%13;
			}
			mcClock[arrHr] = hour;			
		}
		else if (set ==1)
		{
			if (min == 0)
			{
				min = 59;
			}
			else
			{
				min = (min - 1)%60;
			}				
			mcClock[arrMin] = min;	
		}	
	}
	
	if (set==2 && (buttonPressed == dwnSwitch || buttonPressed == upSwitch))
	{
		day = (day+1)%2;
		mcClock[arrAP] = day;
	}
	return;
}

void page5and6(void)//errorPage2
{
	if (buttonPressed==entSwitch)
	{
		page = 0;
		
		line = 0;
	}
	return;
}
void page7(void)//syncMenu
{
	if (buttonPressed==entSwitch)
	{
		if ( line == 0)
			page = 8;
		//else if( line == 1) 
			//page = 6; 
		else if(line == 2)
			page = 0;
		
		line = 0;			
			
	}
	
	else if(buttonPressed == dwnSwitch)
	{
		line = (line+1) % 3;// on this page, we have 3 lines. so, we odate the lines using mod 3
	}
	 
	else if(buttonPressed == upSwitch)
	{  
		if(line == 0 ) 
			line = 2;
		else 
			line = (line-1) % 3;
	}
	return;
}

void page8(void)//manageDeviceMenu
{
	if (buttonPressed==entSwitch)
	{
		if ( line == 0)
		{
			page = 9;
		}
		else if( line == 1)
		{
			page = 11;
		}
		else if(line == 2)
		{
			page = 7;
		}
		
		line = 0;			
			
	}
	
	else if(buttonPressed == dwnSwitch)
	{
		line = (line+1) % 3;// on this page, we have 3 lines. so, we odate the lines using mod 3
	}
	 
	else if(buttonPressed == upSwitch)
	{  
		if(line == 0 ) 
			line = 2;
		else 
			line = (line-1) % 3;
	}
	return;
}

void page9(void)//addDeviceMenu
{
	//printf("in page 9\n");
	if (buttonPressed == entSwitch)
	{	
		if (line != 2)//back button was not pressed.
		{
			int success;
			
			if(line == 0)//add register
			{
				if(devices[0][0])
				{
					page = 13;//"which room" menu.
				}
				else
				{
					//printf("there are no thermos to add registers to.\n");
					SuccessMessage(4,9);//error, no therms to add to. 
				}
			}
			else if(line == 1)//add thermo
			{
				SuccessMessage(3, page);//instruction message.
				mainHalt = 1;
				while(!threadGreenLight)
				{
					delay(20);
				}
				success = addTherm();
				mainHalt = 0;
			}
			else
			{
				printf("line out of range in page 9\n");
			}
			if(success == 1 && line != 0)
			{
				SuccessMessage(line, page); 
				page = 0;
				
			}
			else if(success == 0 && line != 0)
			{
				SuccessMessage(2, page);//failed to add device message.
			}
			
		}
		else //back was pressed for this page.
		{
			page = 8;
		}
			
		line = 0;			
	}
	 else if(buttonPressed == dwnSwitch)
	{
		line = (line+1) % 3;// on this page, we have 3 lines. so, we odate the lines using mod 3
	}
	else if(buttonPressed == upSwitch)
	{  
		if(line == 0 ) 
			line = 2;
		else 
			line = (line-1) % 3;
	}
	
	return;
}

void page10(void)//removeDeviceMenu
{
	// if the enter key is pressed, the line at which it is pressed is taken in concideration
	if (buttonPressed == entSwitch)
	{	
		if (line == 0)//yes remove devices.
		{
			//remove devices here.
			dispMatrix();
			//failedCon[1][0] = 0;
			//failedCon[2][0] = 0;
			countErrors();
			unsigned char addr;
			int i;
			int j;
			for(i = 1; i <= devices[0][0]; i++)
			{
				for(j = 0; j <= devices[0][i]; j++)
				{
					printf("\nfailedCon[%d][%d]: %d\n",i,j,failedCon[i][j]);
					if(failedCon[i][j] == 1 && failedCon[0][0] != 0)
					{
						//printf("\nIn page 10: i=%d, j=%d\n",i,j);
						addr = devices[i][j];
						printf("Address being removed: %#.2x \ni: %d\nj:%d\n",addr,i,j);
						mainHalt = 1;
						while(!threadGreenLight)
						{
							delay(20);
						}
						rmDevAddr(addr);
						mainHalt = 0;
						i = 0;
						//j = 0;
						break;
					}
				}
			}
			SuccessMessage(0,12);
			page = 0;
			line = 0;
		}				
		else if(line == 1)//Do not remove devices.
		{
			page = 0;
			line = 0;
		}
	}
	else if(buttonPressed == dwnSwitch || buttonPressed == upSwitch)
	{
		line = (line+1) % 2;// on this page, we have 2 lines. so, we odate the lines using mod 2
	}
	
	return;
}

void page11(void)//remDevWarn
{
	if (buttonPressed==entSwitch)
	{
		page = 10;
		line = 0;
	}
	return;
}

void page12(void)
{
	
	page = 0;
	line = 0;
	
	return;
}


void page13(void)//add reg to room..
{
	//printf("in page 13.\n");
	
	//printf("there are thermos to add to.\n");
	if(buttonPressed == entSwitch)
	{
		SuccessMessage(3, 9);//instruction message.
		printf("adding register..\n");
		mainHalt = 1;
		while(!threadGreenLight)
		{
			delay(20);
		}
		int success = addReg(toRoom);
		mainHalt = 0;
		printf("done adding register.\n");
		if(success)
		{
			SuccessMessage(0,9);//display reg add successful.
			page = 9;
			line = 0;
		}
		else
		{
			SuccessMessage(2,9);//failed to add device message.
			page = 9;
			line = 0;
		}
	}
	else if(buttonPressed == upSwitch)
	{
		toRoom = (toRoom+1) % (devices[0][0] + 1);
		if(toRoom == 0)
		{
			toRoom = 1;
		}
	}
	else if(buttonPressed == dwnSwitch)
	{
		if(toRoom == 1)
		{
			toRoom = devices[0][0];
		}
		else
		{
			toRoom--;
		}
	}
	return;
}