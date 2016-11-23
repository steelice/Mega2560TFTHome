#ifndef UniSensor_BMP085_h
#define UniSensor_BMP085_h

#include "UnifiedValueSensor.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

class UniSensor_BMP085 : public UnifiedValueSensor<float>
{

public:

	UniSensor_BMP085() : UnifiedValueSensor(){ 
		bmp = new Adafruit_BMP085_Unified(10085);
	}
	~UniSensor_BMP085(){ }

	bool setup(){
		bmp->begin();
		return true;
	}

	bool parse(){
		sensors_event_t event;  
		bmp->getEvent(&event);
		if(isnan(event.pressure)){
			incLastError();
			return false;
		} 
		importValue(event.pressure  / 1.333224);
		return true;
	}



protected:
	Adafruit_BMP085_Unified * bmp;
	
};

#endif
// --- END OF FILE ---