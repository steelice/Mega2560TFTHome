#ifndef UniSensor_Dallas_h
#define UniSensor_Dallas_h

#include "UnifiedValueSensor.h"

#include <OneWire.h>

#include <Arduino.h>

class UniSensor_Dallas : public UnifiedValueSensor<float>
{

public:

	UniSensor_Dallas(OneWire * d, byte a[8]) : UnifiedValueSensor(){ 
		dallas = d;
		address = a;
	}

	~UniSensor_Dallas(){ }

	bool setup(){
		dallas->reset();
		dallas->select(address);
		dallas->write(0x44); 
	}

	bool parse(){

		byte i;
		byte present = 0;
		byte type_s = 0;
		byte data[9];
	
		dallas->reset();
	dallas->select(address);
	dallas->write(0x44); 
  
  present = dallas->reset();
      // Read Scratchpad

  if(present != 1){
  	incLastError();
  	return false;
  }

  dallas->select(address);    
  dallas->write(0xBE);     
  
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = dallas->read();
  }

  if(OneWire::crc8(data, 8) != data[8]){
  	incLastError();
  	return false;
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

  		importValue((float)raw / 16.0);

	}



protected:
	OneWire * dallas;
	byte * address;
	
};

#endif
// --- END OF FILE ---