#ifndef UniSensor_DHT_h
#define UniSensor_DHT_h

#include "UnifiedValueSensor.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>

class UniSensor_DHT : public UnifiedValueSensor<float>
{

public:

	UniSensor_DHT(DHT_Unified * DHT) : UnifiedValueSensor(){ 
		// DHT_Unified dht;
		dht = DHT;
	}
	~UniSensor_DHT(){ }

	bool setup(){
		dht->begin();
		return true;
	}

	bool parse(){
		sensors_event_t event;  
		dht->temperature().getEvent(&event);
		if(isnan(event.temperature)){
			incLastError();
			return false;
		} 
		importValue(event.temperature);
		return true;
	}



protected:
	DHT_Unified * dht;
	
};

#endif
// --- END OF FILE ---