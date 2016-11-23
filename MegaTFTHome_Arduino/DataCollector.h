#ifndef DataCollector_h
#define DataCollector_h

#include <Time.h>
#include <TimeLib.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_BMP085_U.h>

#include "UniSensor_Dallas.h"
#include "UniSensor_DHT.h"
#include "UniSensor_DHT_Humidity.h"
#include "UniSensor_BMP085.h"

#include <Arduino.h>

#define SENSORS_COUNT 4

#define SENSOR_TEMP_IN 		0
#define SENSOR_TEMP_OUT 	1
#define SENSOR_PRESSURE		2
#define SENSOR_HUMIDITY		3

#define HISTORYS_COUNT 4

#define HISTORY_SIZE 72

#define HISTORY_TEMP_IN 0
#define HISTORY_TEMP_OUT 1
#define HISTORY_PRESSURE 2
#define HISTORY_PRESSURE_2HOUR 3

OneWire  dallas(4);
byte dallas_addr[8] = {0x28,0x33,0x93,0x5D,0x5,0x0,0x0,0x23};

DHT_Unified DHT(2, DHT22);

typedef struct FloatPoint{
	unsigned long time;
	float value;
} ;


class DataCollector
{

public:

	DataCollector(){
		sensors[SENSOR_HUMIDITY] = new UniSensor_DHT_Humidity(&DHT);
		sensors[SENSOR_TEMP_IN] = new UniSensor_DHT(&DHT);
		sensors[SENSOR_TEMP_OUT] = new UniSensor_Dallas(&dallas, dallas_addr);
		sensors[SENSOR_PRESSURE] = new UniSensor_BMP085();

		historySavePeriod[HISTORY_TEMP_IN] = 1200000;
		historySavePeriod[HISTORY_TEMP_OUT] = 1200000;
		historySavePeriod[HISTORY_PRESSURE] = 1200000;
		historySavePeriod[HISTORY_PRESSURE_2HOUR] = 120000;

		historySensorIDs[HISTORY_TEMP_IN] = SENSOR_TEMP_IN;
		historySensorIDs[HISTORY_TEMP_OUT] = SENSOR_TEMP_OUT;
		historySensorIDs[HISTORY_PRESSURE] = SENSOR_PRESSURE;
		historySensorIDs[HISTORY_PRESSURE_2HOUR] = SENSOR_PRESSURE;

		for(byte i = 0; i < HISTORYS_COUNT; i++) {
			history[i].clear();
			historyLastSave[i] = historySavePeriod[i] * 2;
		}
	}

	~DataCollector(){

	}

	void initSensors(){
		for(byte i = 0; i < SENSORS_COUNT; i++){
			sensors[i]->setup();
		}
	}

	float getSensorValue(byte sensorID){
		if(sensorID > SENSORS_COUNT - 1) return 0;
		return sensors[sensorID]->getValue();
	}

	byte getSensorErrors(byte sensorID){
		if(sensorID > SENSORS_COUNT - 1) return 0;
		return sensors[sensorID]->getLastErrors();
	}

	void parseSensor(byte sensorID){ sensors[sensorID]->parse(); }
	void parseAll(){ for(byte i = 0; i < SENSORS_COUNT; i++) sensors[i]->parse(); }

	FloatPoint getMax(byte historyID){ return history[historyID].getMax(); }
	FloatPoint getMin(byte historyID){ return history[historyID].getMin(); }
	HistoryWorker<FloatPoint, HISTORY_SIZE> getHistory(byte historyID){ return history[historyID]; }

	void saveHistoryTick(){
		unsigned long mi;
		for(byte i = 0; i < HISTORYS_COUNT; i++){
			mi = millis();
			if(historyLastSave[i] +  historySavePeriod[i] < mi){
				Serial.print(historyLastSave[i], DEC);
				Serial.print(" < ");
				Serial.print(mi, DEC);
				Serial.print(" - ");
				Serial.print(historySavePeriod[i], DEC);
				Serial.print(", ");
				Serial.print(now(), DEC);
				Serial.print(": saved temp for index ");
				Serial.println(i, DEC);
				historyLastSave[i] = mi;
				FloatPoint p = {now(), getSensorValue(historySensorIDs[i])};
				history[i].push(p);
			}
		}
	}

protected:

	HistoryWorker<FloatPoint, HISTORY_SIZE>	history[HISTORYS_COUNT];
	unsigned long historyLastSave[HISTORYS_COUNT];
	unsigned long historySavePeriod[HISTORYS_COUNT];
	byte historySensorIDs[HISTORYS_COUNT];

	UnifiedValueSensor<float> * sensors[SENSORS_COUNT];
	
};

#endif
// --- END OF FILE ---