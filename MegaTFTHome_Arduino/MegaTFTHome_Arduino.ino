#include <IRremote.h>
#include <IRremoteInt.h>
#include "MyIRDefs.h"

#include "RunningMedian.h"
#include "mhTemperature.h"

#include <Time.h>
#include <TimeLib.h>

#include <UTFT.h>
#include <SdFat.h>
#include <UTFT_SdRaw.h>

#include <DS3231.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_BMP085_U.h>


#include "HistoryWorker.h"

#include <Agenda.h>

#include "SunMoon.h"

#include <OneWire.h>

#include <SPI.h>
#include "RF24.h"

RF24 radio(9,10);


#define TEMP_HISTORY_SIZE 144



#define DHTPIN            2         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)

#define IR_RECV_PIN 3

DHT_Unified dht(DHTPIN, DHTTYPE);

#define PRESSURE_HISTORY_SIZE 144
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

DS3231  rtc(SDA, SCL);

#define SD_CHIP_SELECT  53

#define WINDOW_MAIN 1
#define WINDOW_TEMP 2
#define WINDOW_TEMP_OUT 3
#define WINDOW_CALENDAR 4
#define WINDOW_COUNT 4

byte currentWindow = WINDOW_MAIN;



extern uint8_t BigFont[];
extern uint8_t SixteenSegment40x60[];
extern uint8_t GroteskBold24x48[];
extern uint8_t Ubuntu[];
extern uint8_t SmallFont[];
extern uint8_t various_symbols[];



TempPoint maxInTemp;
TempPoint minInTemp;

HistoryWorker<TempPoint, TEMP_HISTORY_SIZE> tempInHistory;
HistoryWorker<TempPoint, TEMP_HISTORY_SIZE> tempOutHistory;
HistoryWorker<TempPoint, TEMP_HISTORY_SIZE> pressureHistory;

UTFT myGLCD(ILI9481, 38, 39, 40, 41);
SdFat sd;
UTFT_SdRaw myFiles(&myGLCD);

Agenda scheduler;

IRrecv irrecv(IR_RECV_PIN);
decode_results IR_results;

SunMoon sun_moon(42.452452, 18.5463489, 1);

OneWire  dallas(4);
byte dallas_addr[8] = {0x28,0x33,0x93,0x5D,0x5,0x0,0x0,0x23};

RunningMedian<float, 6> median_temp_out;	
RunningMedian<float, 6> median_pressure;	

void setup() {	

	randomSeed(analogRead(0));
	
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setFont(Ubuntu);
	myGLCD.print(F("Loading..."), CENTER, 150);

	dallas.reset();
	dallas.select(dallas_addr);
	dallas.write(0x44); 

	while (!sd.begin(SD_CHIP_SELECT)) {
    
  	}

  	Serial.begin(115200);
	rtc.begin();
	dht.begin();
	bmp.begin();
	radio.begin();
  	irrecv.enableIRIn();
  	
  	setTime(rtc.getUnixTime(rtc.getTime()));

  // myFiles.load(0, 0, 480, 320, "temp.raw");

  // showTemperature(19.1, 12.3);
  // showMoonSun();
  // showFinance();
  // showFuel();



  getOutTemp();

  saveTemp();
  scheduler.insert(saveTemp, 600000000);
  scheduler.insert(showTemperature, 60000000);

  redrawCurWindow();

  Serial.println("Clock started!");

  sun_moon.getTimes(now());

}


unsigned long i = 100000;
unsigned long mi = 0;
void loop() {
  
	// myGLCD.clrScr();
	mi = millis();
	showTime();

	while(mi >= millis() - 1000){
		receiveIR();
		scheduler.update();
	}
}

void showTime(){
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
				if(currentWindow > WINDOW_COUNT) currentWindow = WINDOW_MAIN;
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
	sensors_event_t event;  
	dht.temperature().getEvent(&event);
	return isnan(event.temperature) ? 0 : (float) (event.temperature);
}

float getPressure(){
	sensors_event_t event;
  	bmp.getEvent(&event);
  	if (event.pressure)
  	{
  		median_pressure.add(event.pressure / 1.333224);
  		return event.pressure / 1.333224;
  	}

  	return 0;
}	

float getInHumidity(){
	sensors_event_t event;
	dht.humidity().getEvent(&event);
	return isnan(event.relative_humidity) ? 0 : (float) (event.relative_humidity);
}

void showTemperature(){

	if(currentWindow == WINDOW_MAIN) {

		myGLCD.setFont(GroteskBold24x48);
		myGLCD.setColor(100, 200, 100);
		myGLCD.setBackColor(0, 0, 0);
		
		myGLCD.printNumF(getInTemp(), 1, 20, 170);
		myGLCD.print(F("g"), 148, 170);

		myGLCD.setFont(GroteskBold24x48);
		TempSensor out = getOutTemp();
		float outTemp;
		if(out.working && (median_temp_out.getMedian(outTemp) == median_temp_out.OK)){
			myGLCD.setColor(100, 200, 200);
			myGLCD.printNumF(outTemp, 1, 220, 170);
			myGLCD.print(F("g"), 348, 170);
		}else{
			myGLCD.setBackColor(255, 0, 0);
			myGLCD.printNumF(out.temperature, 1, 220, 170);
			myGLCD.print(F("g"), 348, 170);
			myGLCD.setBackColor(0, 0, 0);
		}



		myGLCD.setColor(100, 100, 200);
		myGLCD.printNumI((int)getInHumidity(), 20, 250); 
		myGLCD.print("%", 84, 250); 

		myGLCD.printNumF(getPressure(), 1, 150, 250);
		myGLCD.print("MM", 312, 250);


		// myGLCD.setColor(100, 200, 100);
		// TempPoint m = tempInHistory.getMax();
		// if(m.time){
		// 	myGLCD.setFont(SmallFont);
		// 	myGLCD.print("Max:" + String(m.value) + " (" + getTimeStr(m.time, false) + ")", 330, 175);
		// }

		// m = tempInHistory.getMin();
		// if(m.time){
		// 	myGLCD.setFont(SmallFont);
		// 	myGLCD.print("Min:" + String(m.value) + " (" + getTimeStr(m.time, false) + ")", 330, 190);
		// }

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
		TempPoint m = tempInHistory.getMax();
		if(m.time){
			myGLCD.print(String(m.value) + " (" + getTimeStr(m.time, false) + ")", 266, 40);
		}

		m = tempInHistory.getMin();
		if(m.time){
			myGLCD.print(String(m.value) + " (" + getTimeStr(m.time, false) + ")", 266, 67);
		}

	}else if(currentWindow == WINDOW_TEMP_OUT){
		myGLCD.setFont(SixteenSegment40x60);
		myGLCD.setColor(80, 220, 220);
		myGLCD.setBackColor(0, 0, 0);
		TempSensor s = getOutTemp();
		myGLCD.print(String(s.temperature)+"g", 20, 32);

		myGLCD.setFont(various_symbols);
		myGLCD.setColor(180, 0, 0);
		myGLCD.print("f", 250, 40);
		myGLCD.setColor(0, 0, 180);
		myGLCD.print("g", 250, 67);

		myGLCD.setFont(BigFont);
		myGLCD.setColor(50, 170, 170);
		TempPoint m = tempOutHistory.getMax();
		if(m.time){
			myGLCD.print(String(m.value) + " (" + getTimeStr(m.time, false) + ")", 266, 40);
		}

		m = tempOutHistory.getMin();
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


void showMoonSun(){
	myGLCD.setFont(SmallFont);
	myGLCD.setColor(50, 50, 50);
	myGLCD.setBackColor(0, 0, 0);

	myGLCD.print("04:00", 85, 230);
	myGLCD.print("18:06", 85, 242);

	myGLCD.setColor(255, 240, 0);
	myGLCD.setBackColor(0, 0, 0);

	myGLCD.print("07:12", 190, 230);
	myGLCD.print("17:41", 190, 242);
}

void showFinance(){
	myGLCD.setFont(BigFont);
	myGLCD.setColor(0, 180, 70);
	myGLCD.setBackColor(58, 58, 58);

	myGLCD.print("26.12r", 100, 272);
	myGLCD.print(" 1.09e", 100, 288);
}

void showFuel(){
	myGLCD.setFont(BigFont);
	myGLCD.setColor(255, 128, 0);
	myGLCD.setBackColor(58, 58, 58);

	myGLCD.print("21.67r", 260, 272);
	myGLCD.print(" 1.12e", 260, 288);

}


void saveTemp(){
	TempPoint p = {rtc.getUnixTime(rtc.getTime()), getInTemp()};
	tempInHistory.push(p);

	/**
	 * Save out temperature
	 * @type {[type]}
	 */
	TempSensor s = getOutTemp();
	float t;
	if(s.working && (median_temp_out.getMedian(t) == median_temp_out.OK)){
		p.value = t;
		tempOutHistory.push(p);
	}

	/**
	 * Save pressure
	 * 
	 */	
	t = getPressure();
	if(t > 600 && median_pressure.getMedian(t) == median_pressure.OK){
		p.value = t;
		pressureHistory.push(p);
	}
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


void showTemperatureGraph(HistoryWorker<TempPoint, TEMP_HISTORY_SIZE> history, byte colorR, byte colorG, byte colorB, float curTemp){
	TempPoint max = history.getMax();
	TempPoint min = history.getMin();
	if(!max.time || !min.time) return;

	float step = 400.0 / history.size();

	myGLCD.setColor(0);
	myGLCD.fillRect(0, 100, 480, 320);

	history.reset();

	myGLCD.setColor(colorR, colorG, colorB);
	TempPoint p = history.each();
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

	showTemperatureGraph(tempInHistory, 20, 200, 20, getInTemp());
}

void redrawWindow_tempout() {
	myGLCD.setColor(22, 200, 200);
	myGLCD.fillRect(0, 0, 480, 16);
	showTime();

	showTemperature();

	showTemperatureGraph(tempOutHistory, 20, 200, 200, getOutTemp().temperature);
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

	// уебанская библиотека, 1 у них это воскресенье
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


	timesInfo sun = sun_moon.getTimes(now());

	myGLCD.setColor(200, 200, 80);
	myGLCD.setFont(BigFont);
	myGLCD.print(getTimeStr(sun.rise, false), CAL_WIDTH + CAL_PADD_W * 3+ 18, 40);
	myGLCD.print(getTimeStr(sun.set, false), CAL_WIDTH + CAL_PADD_W * 3 + 18, 62);

	myGLCD.setFont(various_symbols);
	myGLCD.print("=", CAL_WIDTH + CAL_PADD_W * 3, 40);
	myGLCD.print(">", CAL_WIDTH + CAL_PADD_W * 3, 62);




}


void drawHistory(struct TempPoint * arr, byte cr, byte cg, byte cb){
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


TempSensor getOutTemp(){
	byte i;
	byte present = 0;
	byte type_s = 0;
	byte data[9];


  	TempSensor result = {TEMP_ID_OUT1, millis(), false, 0, 0, TEMP_SENSOR_EXTENRAL};

  	dallas.reset();
	dallas.select(dallas_addr);
	dallas.write(0x44); 
  
  present = dallas.reset();
      // Read Scratchpad

  if(present != 1){
  	Serial.println(F("  Dallas not present! "));
  	return result;
  }

  dallas.select(dallas_addr);    
  dallas.write(0xBE);     
  Serial.print(F("Getting data "));
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = dallas.read();
  }

  if(OneWire::crc8(data, 8) != data[8]){
  	Serial.println(F(" CRC of dallas not correct! "));
  	return result;
  }
  

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  
  result.temperature = (float)raw / 16.0;
  result.working = true;

  Serial.println("Out temperature is OK: " + String(result.temperature));

  median_temp_out.add(result.temperature);

  
  return result;

}
