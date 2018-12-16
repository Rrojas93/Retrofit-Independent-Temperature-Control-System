/****************************************************************************************
finalHeader.h
	by group 2: Raul Rojas, Christa McDaniel, Alex Fotso
	
	Objective: 
	This file contains all defined constants as well as all function prototypes for all 
	files associated with the main program of the Master Controller. 
	
****************************************************************************************/

#ifndef		FinalHeader_H
#define		FinalHeader_H

//Define constants
#define FAN             22
#define COOL            13
#define HEAT            26
#define LCD_D4			104
#define LCD_D5			105
#define LCD_D6			106
#define LCD_D7			107
#define LCD_RS			102	
#define LCD_E			103
#define pwrLED			6
#define syncLED			23
#define entSwitch		16
#define dwnSwitch		25
#define upSwitch		24
#define syncSwitch		17
#define rstSwitch		5
#define pwrSwitch		27

#define rows 			128
#define columns 		128

#define	arrHr			0
#define arrMin			1
#define arrSec			2
#define	arrAP			3

#define thermTimer		0
#define miscTimer		1

#define retCurrTemp		0
#define retSetTemp		1
#define retHum			2

//3-dimensional array z axis index reference constants
#define sendingAF	0	//sending AirFlow
#define	receivingAF	1	//receiving AirFlow

//temps[][]: [Current temp | Set temp | Current Humidity | temp difference | start difference]
#define currTemp	0
#define setTemp		1
#define	tempDiff	3
#define startDiff	4
#define hvacOut		3

//Define Prototypes

	//menuFunctions.c prototypes	
	void lcdinitialization(void);
	void lcdBoot(void);
	void updateDisplay(void);
	void mainDisplay(void);
	void settings(int line);
	void settingConfig(int line);
	void errorPage1(int line);
	void errorPage2(void);
	void warningPage(void);
	void display(int, int, int);
	//void SuccessMessage(int line);
	void syncMenu(int line);
	void manageDeviceMenu(int line);
	void addDeviceMenu(int line);
	void removeDeviceMenu(int line);
	void SuccessMessage(int line, int page);
	void setTimeDisplay(int);
	void page0(void);
    void page1(void);
    void page2(void);
    void page3(void);
    void page4(void);
    void page5and6(void);
    void page7(void);
    void page8(void); 
    void page9(void);
    void page10(void);
	void page11(void);
	void page12(void);
	void page13(void);
	void dispTime(void);
	void remDevWarn(void);
	void toRoomMenu(void);
	
	
	//syncing.c prototypes
	void initArray(void);
	void populateArrays(void);
	int addTherm(void);
	int addReg(int addToRoom);
	void dispMatrix(void);
	void rmDevAddr(unsigned char address);
	void retrieveTemps(void);
	void setReg(void);
	void initRegFlow(void);
	void initTempsArr(void);
	int countErrors(void);
	void calcDiff(void);
	void hvacControl(void);
	void initRocArray(void);
	void adjustReg(int i, int flowIndex);
	void initFailedCon(void);
	
	//time.c prototypes
	int getSec(void);
	void initTimeArr(void);
	void updateTime(void);
	void parseTime(void);
	void rtTimerStart(int timerNum, int timerType);
	int rtTimerEnd(int timerNum, int timerType);

	
	//misc
	int smartDelay(int i);
	void setups(void);
	void bootSequence(void);
	void shutDownSequence(void);
	void updateStatus(void);
	
	
#endif