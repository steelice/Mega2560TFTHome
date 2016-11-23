#ifndef MainScreenRenderer_h
#define MainScreenRenderer_h

#include "Renderer.h"

#include <Time.h>
#include <TimeLib.h>

#include <Arduino.h>

#include "SunMoon.h"


extern uint8_t SixteenSegment40x60[];
extern uint8_t GroteskBold24x48[];
extern uint8_t SmallFont[];

#define BALLOON_Y 16
#define BALLOON_HEIGHT 280
#define BALLOON_WIDTH 10
#define BALLOON_TRICK_WIDTH 6


class MainScreenRenderer : public Renderer
{
public:
	MainScreenRenderer(UTFT * S, UTFT_SdRaw * SD, DataCollector * D) : Renderer(S, SD, D) {}

	void redraw(){
		screen->clrScr();
		
		preDrawHgBalloon(0, 40, 448, 464);
		preDrawHgBalloon(0, 40, 26, 3);

		drawTime();
		drawTemp();
		drawPressure();
		drawCalendar();

		drawSun();
	}

	void onNewSecond(byte second){
		drawTime();
	}
	void onNewMinute(byte minute){
		drawTemp();
		drawPressure();
		if(minute % 10 == 0) drawSun();
	}
	void onNewHour(byte hour){
	
	}

	void onNewDay(byte day){
		drawCalendar();
	}

	void drawSun(){
		SunMoon sun_moon(42.452452, 18.5463489, 1);
		timesInfo sun = sun_moon.getTimes(now());
		unsigned int sunPosition;

		screen->setColor(0,0,0);
		screen->fillRect(70, 137, 410, 155); // очищаем область
		screen->setFont(SmallFont);
		screen->setColor(70, 70, 70);
		screen->fillRect(80, 145, 400, 148);

		if(sun.rise > now())
		{

			unsigned long lastPeriod = sun.rise;
			// рисуем до восхода
			sun = sun_moon.getTimes(now() - (unsigned long)86400);
			sd->load(54, 137, 18, 13, F("i_sunset.raw"));
			sd->load(412, 137, 16, 11, F("i_sunris.raw"));

			
			screen->setColor(255, 198, 0);
			screen->print(getTimeStr(lastPeriod, false), 404, 152);

			

			screen->setColor(175, 62, 0);
			screen->print(getTimeStr(sun.set, false), 50, 152);


			sunPosition = 73 + floor((320.0 / (lastPeriod - sun.set) * (now() - sun.set)));

			screen->fillRect(80, 145, sunPosition + 9, 148);
			sd->load(sunPosition, 137, 18, 18, F("sun_sl_n.raw"));

		}else{

			sd->load(412, 137, 18, 13, F("i_sunset.raw"));
			sd->load(54, 137, 16, 11, F("i_sunris.raw"));
			
			screen->setColor(175, 62, 0);
			screen->print(getTimeStr(sun.set, false), 404, 152);

			screen->setColor(255, 198, 0);
			screen->print(getTimeStr(sun.rise, false), 50, 152);

			sunPosition = 73 + floor((320.0 / (sun.set - sun.rise) * (now() - sun.rise)));

			screen->fillRect(80, 145, sunPosition + 9, 148);
			sd->load(sunPosition, 137, 18, 18, F("sun_sl_d.raw"));

		}
	}

	void drawTime(){
		screen->setFont(SixteenSegment40x60);
		screen->setColor(255, 255, 255);
		screen->setBackColor(0, 0, 0);

		screen->print(getTimeStr(now(), true), CENTER, 72);
	}

	void drawTemp(){
		screen->setFont(GroteskBold24x48);
		screen->setColor(100, 200, 100);
		screen->setBackColor(0, 0, 0);
		screen->printNumF(data->getSensorValue(SENSOR_TEMP_IN), 1, 48, 10);

		screen->setColor(100, 200, 200);
		screen->printNumF(data->getSensorValue(SENSOR_TEMP_OUT), 1, 318, 10);

		drawHgBalloon(data->getSensorValue(SENSOR_TEMP_IN), 
			data->getMin(HISTORY_TEMP_IN).value, data->getMax(HISTORY_TEMP_IN).value, 26);

		drawHgBalloon(data->getSensorValue(SENSOR_TEMP_OUT), 
			data->getMin(HISTORY_TEMP_OUT).value, data->getMax(HISTORY_TEMP_OUT).value, 448);
	}

	void drawPressure(){
		screen->setColor(100, 100, 200);
		screen->printNumF(data->getSensorValue(SENSOR_PRESSURE), 1, 180, 240);
	}

	void drawCalendar(){
		sd->load(192, 10, 93, 48, "cal_bg.raw");
		TimeElements date;
		breakTime(now(), date);
		sd->load(196, 29, 40, 21, "dname_"+String(date.Wday)+".raw");
		sd->load(244, 29, 34, 21, "dnum_"+String(date.Day)+".raw");
		sd->load(207, 10, 63, 13, "mname_"+String(date.Month)+".raw");
	}

	void preDrawHgBalloon(byte tFrom, byte tTo, unsigned int x, unsigned int xLabel){
		screen->setFont(SmallFont);

		for(byte C = tFrom; C <= tTo; C++){
			screen->setColor(99, 99, 99);
			unsigned int y = (BALLOON_HEIGHT + BALLOON_Y) - ((BALLOON_HEIGHT / (float)(tTo - tFrom*1.0)) * C * 1.0 );
			screen->drawLine(x - BALLOON_TRICK_WIDTH, y,
					x + (BALLOON_TRICK_WIDTH) + BALLOON_WIDTH, y);
			if(C % 5 == 0){
				screen->drawLine(x - BALLOON_TRICK_WIDTH, y - 1,
					x + (BALLOON_TRICK_WIDTH) + BALLOON_WIDTH, y - 1);
				screen->setColor(255, 255, 255);
				screen->printNumI(C, xLabel, y - 4);
			}
		}

		screen->setColor(255,255,255);
		screen->fillRect(x, BALLOON_Y - 3, x + BALLOON_WIDTH, BALLOON_Y + BALLOON_HEIGHT + 3);
		screen->fillCircle(x + (BALLOON_WIDTH / 2.0), BALLOON_Y - 3, BALLOON_WIDTH / 2.0);
		screen->setColor(255,0,0);
		screen->fillCircle(x + (BALLOON_WIDTH / 2.0), BALLOON_Y + BALLOON_HEIGHT + 3, BALLOON_WIDTH);
	}
	void drawHgBalloon(float temp, float min, float max, unsigned int x){
		screen->setColor(255,255,255);
		screen->fillRect(x, BALLOON_Y, x + BALLOON_WIDTH, BALLOON_Y + BALLOON_HEIGHT - (temp * 7.0));
		screen->setColor(255,0,0);
		screen->fillRect(x, BALLOON_Y + BALLOON_HEIGHT - (temp * 7.0), x + BALLOON_WIDTH, BALLOON_Y + BALLOON_HEIGHT);

		screen->setColor(0,255,36);
		screen->fillRect(x, BALLOON_Y + BALLOON_HEIGHT - (min * 7.0), x + BALLOON_WIDTH, BALLOON_Y + BALLOON_HEIGHT - (min * 7.0) + 1);

		screen->setColor(255,0,222);
		screen->fillRect(x, BALLOON_Y + BALLOON_HEIGHT - (max * 7.0), x + BALLOON_WIDTH, BALLOON_Y + BALLOON_HEIGHT - (max * 7.0) + 1);

	}

};


#endif