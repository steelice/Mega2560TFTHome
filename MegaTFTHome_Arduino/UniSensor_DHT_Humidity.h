#ifndef UniSensor_DHT_Humidity_h
#define UniSensor_DHT_Humidity_h

#include "UnifiedValueSensor.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>

class UniSensor_DHT_Humidity : public UnifiedValueSensor<float>
{

public:

	UniSensor_DHT_Humidity(DHT_Unified *DHT) : UnifiedValueSensor(){ 
		dht = DHT;
	}
	~UniSensor_DHT_Humidity(){ }

	bool setup(){
		dht->begin();
		return true;
	}

	bool parse(){
		sensors_event_t event;  
		dht->humidity().getEvent(&event);
		if(isnan(event.relative_humidity)){
			incLastError();
			return false;
		} 
		importValue(event.relative_humidity);
		return true;
	}



protected:
	DHT_Unified * dht;
	
};

#endif
// --- END OF FILE ---