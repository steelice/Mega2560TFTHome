#include <IRremote.h>
#include <IRremoteInt.h>
#include "MyIRDefs.h"

#include "RunningMedian.h"

#include <Time.h>
#include <TimeLib.h>

#include <UTFT.h>
#include <SdFat.h>
#include <UTFT_SdRaw.h>

#include <DS3231.h>

#include "HistoryWorker.h"

#include <Agenda.h>

#include "SunMoon.h"

#include "DataCollector.h"

#include <SPI.h>
#include "RF24.h"

#include "Renderer.h"
#include "MainScreenRenderer.h"

RF24 radio(9,10);

#define TEMP_HISTORY_SIZE 144


#define IR_RECV_PIN 3

#define PRESSURE_HISTORY_SIZE 144


DS3231  rtc(SDA, SCL);

#define SD_CHIP_SELECT  53

#define WINDOW_MAIN 0
#define WINDOW_TEMP 1
#define WINDOW_TEMP_OUT 2
#define WINDOW_CALENDAR 3

#define WINDOWS_COUNT 4

byte currentWindow = WINDOW_MAIN;



extern uint8_t BigFont[];
extern uint8_t SixteenSegment40x60[];
extern uint8_t GroteskBold24x48[];
extern uint8_t Ubuntu[];
extern uint8_t SmallFont[];
extern uint8_t various_symbols[];


UTFT myGLCD(ILI9481, 38, 39, 40, 41);
SdFat sd;
UTFT_SdRaw myFiles(&myGLCD);

Agenda scheduler;

IRrecv irrecv(IR_RECV_PIN);
decode_results IR_results;

DataCollector allData;

Renderer * windows[WINDOWS_COUNT];

TimeElements curTE;
TimeElements lastTE;

void setup() {	

	randomSeed(analogRead(0));
	
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setFont(Ubuntu);
	myGLCD.print(F("Loading..."), CENTER, 150);

	while (!sd.begin(SD_CHIP_SELECT)) {
    
  	}

  	Serial.begin(115200);
	rtc.begin();
	allData.initSensors();
	radio.begin();
  	irrecv.enableIRIn();
  	
  	setTime(rtc.getUnixTime(rtc.getTime()));

  	delay(500);
  	allData.parseAll();

	scheduler.insert(parseSensors, 30000000);
	scheduler.insert(showTemperature, 60000000);

	windows[WINDOW_MAIN] = new MainScreenRenderer(&myGLCD, &myFiles, &allData);

	windows[currentWindow]->redraw();

	// Serial.println(F("Clock started!"));

	

	breakTime(now(), curTE);
	lastTE = curTE;
}

unsigned long mi = 0;

void loop() {
  
	// myGLCD.clrScr();
	mi = millis();
	showTime();

	while(mi >= millis() - 1000){
		receiveIR();
		scheduler.update();
		allData.saveHistoryTick();
		breakTime(now(), curTE);

		if(curTE.Second == lastTE.Second) continue;

		windows[currentWindow]->onNewSecond(curTE.Second);
		if(curTE.Minute != lastTE.Minute){
			windows[currentWindow]->onNewMinute(curTE.Minute);
			if(curTE.Hour != lastTE.Hour){
				windows[currentWindow]->onNewHour(curTE.Hour);
				if(curTE.Day != lastTE.Day){
					windows[currentWindow]->onNewDay(curTE.Day);
				}
			}
			setTime(rtc.getUnixTime(rtc.getTime()));
		}

		lastTE = curTE;
	}
}


void showTime(){
	return;
	if(currentWindow == WINDOW_MAIN){
		myGLCD.setFont(SixteenSegment40x60);
		myGLCD.setColor(255, 255, 255);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print(rtc.getTimeStr(), CENTER, 70);

		myGLCD.setColor(155, 155, 155);
		myGLCD.setFont(Ubuntu);
		myGLCD.print(String(rtc.getDOWStr()) + ", " + String(rtc.getDateStr()), CENTER, 10);
	}else if(currentWindow == WINDOW_TEMP){
		myGLCD.setFont(SmallFont);
		myGLCD.setColor(0xffffff);
		myGLCD.setBackColor(22, 200, 22);
		myGLCD.print(rtc.getTimeStr(), 480 - (8*8 + 5), 2);

		myGLCD.print(String(rtc.getDOWStr()) + ", " + String(rtc.getDateStr()), 5, 2);
	}
	
}

void receiveIR(){
	if (irrecv.decode(&IR_results)) {
		Serial.print("IR: ");
		Serial.println(IR_results.value, HEX);

		switch(IR_results.value){
			case IR_FUEGO_SCREEN_UP_DOWN:
				currentWindow++;
				if(currentWindow > WINDOWS_COUNT) currentWindow = WINDOW_MAIN;
				redrawCurWindow();
			break;
			case IR_FUEGO_TIMER:
				currentWindow = WINDOW_MAIN;
				redrawCurWindow();
			break;
		}


		irrecv.resume(); // Receive the next value
	}	
}

float getInTemp(){
	return allData.getSensorValue(SENSOR_TEMP_IN);
}

float getPressure(){
  	return allData.getSensorValue(SENSOR_PRESSURE);
}	

float getInHumidity(){
	return allData.getSensorValue(SENSOR_HUMIDITY);
}

void showTemperature(){
	return;

	if(currentWindow == WINDOW_MAIN) {

		
	}else if(currentWindow == WINDOW_TEMP){
		myGLCD.setFont(SixteenSegment40x60);
		myGLCD.setColor(80, 220, 80);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print(String(getInTemp())+"g", 20, 32);

		myGLCD.setFont(various_symbols);
		myGLCD.setColor(180, 0, 0);
		myGLCD.print("f", 250, 40);
		myGLCD.setColor(0, 0, 180);
		myGLCD.print("g", 250, 67);

		myGLCD.setFont(BigFont);
		myGLCD.setColor(50, 170, 50);
		FloatPoint m = allData.getMax(SENSOR_TEMP_IN);
		if(m.time){
			myGLCD.print(String(m.value) + " (" + getTimeStr(m.time, false) + ")", 266, 40);
		}

		m = allData.getMin(SENSOR_TEMP_IN);
		if(m.time){
			myGLCD.print(String(m.value) + " (" + getTimeStr(m.time, false) + ")", 266, 67);
		}

	}else if(currentWindow == WINDOW_TEMP_OUT){
		myGLCD.setFont(SixteenSegment40x60);
		myGLCD.setColor(80, 220, 220);
		myGLCD.setBackColor(0, 0, 0);
		float s = getOutTemp();
		myGLCD.print(String(s)+"g", 20, 32);

		myGLCD.setFont(various_symbols);
		myGLCD.setColor(180, 0, 0);
		myGLCD.print("f", 250, 40);
		myGLCD.setColor(0, 0, 180);
		myGLCD.print("g", 250, 67);

		myGLCD.setFont(BigFont);
		myGLCD.setColor(50, 170, 170);
		FloatPoint m = allData.getMax(SENSOR_TEMP_OUT);
		if(m.time){
			myGLCD.print(String(m.value) + " (" + getTimeStr(m.time, false) + ")", 266, 40);
		}

		m = allData.getMin(SENSOR_TEMP_OUT);
		if(m.time){
			myGLCD.print(String(m.value) + " (" + getTimeStr(m.time, false) + ")", 266, 67);
		}

	}
	

	return;
/*

	myGLCD.setFont(Ubuntu);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(String(in) + "`", 56, 6);
	myGLCD.print(String(out) + "`", 275, 6);

	unsigned int barHeight = 285 * (in / 30);

	myGLCD.setColor(255,255,255);
	myGLCD.fillRect(9,11, 18,296 - barHeight);

	myGLCD.setColor(255,0,0);
	myGLCD.fillRect(9,296 - barHeight, 18,296);

	barHeight = 285 * (out / 30);

	myGLCD.setColor(255,255,255);
	myGLCD.fillRect(461,11, 470,296 - barHeight);

	myGLCD.setColor(255,0,0);
	myGLCD.fillRect(461,296 - barHeight, 470,296);
	*/
}



void redrawCurWindow() {
	myGLCD.clrScr();
	switch(currentWindow){
		case WINDOW_MAIN:
			redrawWindow_main();
		break;
		case WINDOW_TEMP:
			redrawWindow_temp();
		break;
		case WINDOW_TEMP_OUT:
			redrawWindow_tempout();
		break;
		case WINDOW_CALENDAR:
			redrawWindow_calendar();
		break;
		default:
			myGLCD.setFont(Ubuntu);
			myGLCD.print(F("Window not found"), 150, CENTER);
	}
}


void showTemperatureGraph(HistoryWorker<FloatPoint, HISTORY_SIZE> history, byte colorR, byte colorG, byte colorB, float curTemp){
	FloatPoint max = history.getMax();
	FloatPoint min = history.getMin();
	if(!max.time || !min.time) return;

	float step = 400.0 / history.size();

	myGLCD.setColor(0);
	myGLCD.fillRect(0, 100, 480, 320);

	history.reset();

	myGLCD.setColor(colorR, colorG, colorB);
	FloatPoint p = history.each();
	int i = 0;
	float pxPerC = 200.0 / (max.value - min.value + 7.0);
	bool drawedMidnight = false, drawedHalfday = false;
	myGLCD.setFont(SmallFont);
	while(p.time != 0){
		myGLCD.fillRect(60.0 + (1.0*i*step), 300 - ((p.value - (min.value - 5)) * pxPerC), 60.0+ (i+1)*step*1.0, 300);

		if(!drawedMidnight && (hour(p.time) == 0)){
			int px = 60 + ((i*step) + (step / 2.0));
			myGLCD.setColor(200, 200, 200);
			myGLCD.drawLine(px, 100, px, 300);
			myGLCD.print(F("00"), px - 8, 303);
			myGLCD.setColor(20, 200, 20);

			drawedMidnight = true;
			
		}

		if(!drawedHalfday && (hour(p.time) == 12)){
			int px = 60 + ((i*step) + (step / 2.0));
			myGLCD.setColor(200, 200, 200);
			myGLCD.drawLine(px, 100, px, 300);
			myGLCD.print(F("12"), px - 8, 303);
			myGLCD.setColor(20, 200, 20);

			drawedHalfday = true;
			
		}

		i++;
		p = history.each();
	}

	myGLCD.setColor(60, 60, 60);
	for(int i = 100; i <= 300; i += pxPerC) myGLCD.drawLine(60, i, 460, i);
	
	myGLCD.setColor(200, 200, 200);
	myGLCD.drawLine(60, 300 - ((curTemp - (min.value - 5)) * pxPerC), 460, 300 - ((curTemp - (min.value - 5)) * pxPerC));
	myGLCD.printNumF(curTemp, 1, 5, 300 - ((curTemp - (min.value - 5)) * pxPerC) - 8);
}



void redrawWindow_main() {
	showTime();
  	showTemperature();
}

void redrawWindow_temp() {
	myGLCD.setColor(22, 200, 22);
	myGLCD.fillRect(0, 0, 480, 16);
	showTime();

	showTemperature();

	showTemperatureGraph(allData.getHistory(SENSOR_TEMP_IN), 20, 200, 20, getInTemp());
}

void redrawWindow_tempout() {
	myGLCD.setColor(22, 200, 200);
	myGLCD.fillRect(0, 0, 480, 16);
	showTime();

	showTemperature();

	showTemperatureGraph(allData.getHistory(SENSOR_TEMP_OUT), 20, 200, 200, getOutTemp());
}

#define CAL_CELL_WIDTH 40
#define CAL_CELL_HEIGHT 44
#define CAL_GRID_COLOR 150,150,150
#define CAL_SA_COLOR 250, 150, 150
#define CAL_SU_COLOR 250, 50, 50
#define CAL_WD_COLOR 250, 250, 250
#define CAL_PADD_W 20
#define CAL_PADD_H 36
#define CAL_WIDTH (CAL_CELL_WIDTH * 7)
#define CAL_HEIGHT (CAL_CELL_HEIGHT * 6)

#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0

char* shortDays[7] = {"Mo", "Th", "We", "Th", "Fr", "Sa", "Su"};

void redrawWindow_calendar() {
	myGLCD.setFont(BigFont);
	myGLCD.setColor(22, 200, 220);
	myGLCD.fillRect(0, 0, 480, 16);
	myGLCD.setBackColor(0);
	showTime();

	myGLCD.setColor(CAL_GRID_COLOR);
	myGLCD.drawRoundRect(CAL_PADD_W, CAL_PADD_H, CAL_WIDTH + CAL_PADD_W, CAL_HEIGHT + CAL_PADD_H);

	for(int i = CAL_PADD_W + CAL_CELL_WIDTH; i < CAL_WIDTH; i += CAL_CELL_WIDTH)
		myGLCD.drawLine(i, CAL_PADD_H, i, CAL_HEIGHT + CAL_PADD_H);

	for(int i = CAL_PADD_H + CAL_CELL_HEIGHT; i < CAL_HEIGHT; i += CAL_CELL_HEIGHT)
		myGLCD.drawLine(CAL_PADD_W, i, CAL_PADD_W + CAL_WIDTH, i);
	myGLCD.drawLine(CAL_PADD_W, CAL_PADD_H + CAL_CELL_HEIGHT + 1, CAL_PADD_W + CAL_WIDTH, CAL_PADD_H + CAL_CELL_HEIGHT + 1);

	for(int i = 0; i <= 6; i++) myGLCD.print(shortDays[i], CAL_PADD_W + 4 + (i * CAL_CELL_WIDTH), CAL_PADD_H + 14);

	myGLCD.setColor(0xFFFFFF);

	TimeElements te;
	breakTime(now(), te);
	te.Day = 1;
	time_t firstDay = makeTime(te);
	breakTime(firstDay, te);

	// уебанская библиотека, 0 это суббота, 1 у них это воскресенье
	byte startD = te.Wday >= 2 ? te.Wday - 2 : te.Wday + 5;

	byte row = 1;
	
	myGLCD.setBackColor(VGA_TRANSPARENT);

	for(byte dayN = 1; dayN <= (te.Month == 2 ? (LEAP_YEAR(te.Year) ? 29 : 28) : monthDays[te.Month - 1]); dayN++){

		if(dayN == day()){
			myGLCD.setColor(40, 180, 40);
			myGLCD.fillCircle(CAL_PADD_W + (startD * CAL_CELL_WIDTH) + (CAL_CELL_WIDTH / 2), CAL_PADD_H + (row * CAL_CELL_HEIGHT) + (CAL_CELL_HEIGHT / 2), (CAL_CELL_WIDTH / 2) - 4);
		}

		if(startD == 5)
			myGLCD.setColor(CAL_SA_COLOR);
		else if(startD == 6)
			myGLCD.setColor(CAL_SU_COLOR);
		else 
			myGLCD.setColor(CAL_WD_COLOR);


		myGLCD.printNumI(dayN, CAL_PADD_W + (startD * CAL_CELL_WIDTH) + (dayN < 10 ? 12 : 4), CAL_PADD_H + (row * CAL_CELL_HEIGHT) + 14);
		startD++;
		if(startD > 6) {
			startD = 0;
			row++;
		}
	}

}

void drawHistory(struct FloatPoint * arr, byte cr, byte cg, byte cb){
	int step = 400 / TEMP_HISTORY_SIZE;
	myGLCD.setColor(cr,cg,cb);
	for(int i = 1; i < TEMP_HISTORY_SIZE; i++){
		if(arr[i].time == 0) return;
		int j = i - 1;
		// myGLCD.drawLine(j * step, 320 - arr[j].value * 10, i * step, 320 - arr[i].value * 10);
	}
}

String getTimeStr(time_t t, bool showSeconds){
	String result = "";
	byte h = hour(t);
	if(h < 10) result += "0";
	result += String(h) + ":";
	h = minute(t);
	if(h < 10) result += "0";
	result += String(h);
	if(!showSeconds) return result;
	result += ":";
	h = second(t);
	if(h < 10) result += "0";
	result += String(h);

	return result;
}


float getOutTemp(){
  	return allData.getSensorValue(SENSOR_TEMP_OUT);
}

void parseSensors(){
	allData.parseAll();
}
