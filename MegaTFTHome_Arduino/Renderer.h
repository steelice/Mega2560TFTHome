#ifndef Renderer_h
#define Renderer_h

#include <Time.h>
#include <TimeLib.h>

class Renderer
{
public:
	virtual Renderer(UTFT * S, UTFT_SdRaw * SD, DataCollector * D)
		{
			screen = S;
			sd = SD;
			data = D;
	}
	virtual ~Renderer(){}
	
	virtual void redraw() = 0;

	virtual void onTick(){}
	virtual void onNewSecond(byte second){}
	virtual void onNewMinute(byte minute){}
	virtual void onNewHour(byte hour){}
	virtual void onNewDay(byte day){}

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

protected:
	UTFT * screen;
	UTFT_SdRaw * sd;
	DataCollector * data;
};

#endif
// --- END OF FILE ---