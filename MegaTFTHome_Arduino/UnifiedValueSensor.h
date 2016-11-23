#ifndef UnifiedValueSensor_h
#define UnifiedValueSensor_h

#include <Time.h>
#include <TimeLib.h>

template <typename T>
class UnifiedValueSensor
{

public:

	virtual UnifiedValueSensor(){ 
		lastErrors = 0;
	}
	virtual ~UnifiedValueSensor(){ }

	virtual bool setup() = 0;
	virtual bool parse() = 0;

	virtual T getValue(){
		
		T medianValue;
		if(median.getMedian(medianValue) == median.OK){
			return medianValue;
		}else{
			if(!parse()){
				incLastError();
				return lastValue;
			}else{
				return getLastValue();
			}
		}
	}
	
	/**
	 * Получает в переменную value текущее значение сенсора
	 * @param  value 
	 * @return Успех операции
	 */
	virtual bool getActualValue(T &value){
		if(parse()){
			value = lastValue;
			return true;
		}

		return false;
	}

	virtual T getLastValue() {
		return lastValue;
	}

	virtual byte getLastErrors() {
		return lastErrors;
	}

	virtual unsigned long getLastParsedTime(){ return lastParsedTime; }

	/**
	 * Импорт значения.
	 * Нужно для таких, например, штук, как радиомодули, которые не известно, когда пришлют данные
	 * @param value [description]
	 */
	virtual void importValue(T value) {
		lastErrors = 0;
		lastValue = value;
		lastParsedTime = now();
		median.add(value);
	}


protected:
	byte lastErrors;
	T lastValue;
	RunningMedian<T, 5> median;
	long unsigned lastParsedTime;

	virtual void incLastError(){
		if(lastErrors < 254) lastErrors++;
	}
	
};

#endif
// --- END OF FILE ---