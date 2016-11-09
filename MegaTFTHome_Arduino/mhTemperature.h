#define TEMP_SENSOR_INTERNAL 1
#define TEMP_SENSOR_EXTENRAL 2

#define TEMP_ID_CABINET 1
#define TEMP_ID_OUT1 2

typedef struct TempPoint{
	unsigned long time;
	float value;
} ;



typedef struct TempSensor{
	byte ID;
	unsigned long lastCheck;
	bool working;
	double temperature;
	double humidity;
	byte type;
} ;