// header defining the interface of the source.
#ifndef _SUNMOON_H_
#define _SUNMOON_H_

// include Arduino basic header.
#include <Arduino.h>

#define SUN_PI 3.14159
#define SUN_RAD (SUN_PI / 180.0)
#define SUN_DAYS_MS 86400.0
#define SUN_J1970 2440588
#define SUN_J2000 2451545
#define SUN_E (SUN_RAD * 23.4397)
#define SUN_J0 0.0009

typedef struct {
  unsigned long rise;
  unsigned long set;
} timesInfo;

class SunMoon
{
public:
  SunMoon(double mlat, double mlng, int timezone);
  ~SunMoon();

  void setTimezone(int timezone);

  timesInfo getTimes(unsigned long date);
  timesInfo getTimes(unsigned long date, double angle);


private:
  double _lat;
  double _lng;
  unsigned long _date;
  unsigned long _hoursShift;
  int _timezone;


  double rightAscension(double l, double b);
  double declination(double l, double b);

  double azimuth(double H, double phi, double dec) ;
  double altitude(double H, double phi, double dec);
  double siderealTime(double d, double lw);
  double astroRefraction(double h);
  double toDays(unsigned long date);

  double solarMeanAnomaly(double d);

  double eclipticLongitude(double M);

  double julianCycle(double d, double lw) ;

  double approxTransit(double Ht, double lw, double n) ;
  unsigned long solarTransitJ(double ds, double M, double L) ;

  double hourAngle(double h, double phi, double d) ;

  unsigned long getSetJ(double h, double lw, double phi, double dec, double n, double M, double L);

  
};


SunMoon::SunMoon(double mlat, double mlng, int timezone){
  _lat = mlat;
  _lng = mlng;
  setTimezone(timezone);
}

SunMoon::~SunMoon(){

}

void SunMoon::setTimezone(int timezone){
	_timezone = timezone;
	_hoursShift = timezone * 3600L;
}

double SunMoon::rightAscension(double l, double b){ return atan2(sin(l) * cos(SUN_E) - tan(b) * sin(SUN_E), cos(l)); }
double SunMoon::declination(double l, double b){ return asin(sin(b) * cos(SUN_E) + cos(b) * sin(SUN_E) * sin(l)); }
double SunMoon::azimuth(double H, double phi, double dec)  { return atan2(sin(H), cos(H) * sin(phi) - tan(dec) * cos(phi));}
double SunMoon::altitude(double H, double phi, double dec) { return asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(H));}
double SunMoon::siderealTime(double d, double lw) { return SUN_RAD * (280.16 + 360.9856235 * d) - lw; }
double SunMoon::astroRefraction(double h) {
    if (h < 0) // the following formula works for positive altitudes only.
        h = 0; // if h = -0.08901179 a div/0 would occur.

    // formula 16.4 of "Astronomical Algorithms" 2nd edition by Jean Meeus (Willmann-Bell, Richmond) 1998.
    // 1.02 / tan(h + 10.26 / (h + 5.10)) h in degrees, result in arc minutes -> converted to rad:
    return 0.0002967 / tan(h + 0.00312536 / (h + 0.08901179));
}

double SunMoon::toDays(unsigned long date)   { return date / SUN_DAYS_MS - 10957.5; }

double SunMoon::solarMeanAnomaly(double d) { return SUN_RAD * (357.5291 + 0.98560028 * d); }

double SunMoon::eclipticLongitude(double M) {

    double C = SUN_RAD * (1.9148 * sin(M) + 0.02 * sin(2.0 * M) + 0.0003 * sin(3.0 * M)); // equation of center
    double P = SUN_RAD * 102.9372; // perihelion of the Earth

    return M + C + P + SUN_PI;
}


double SunMoon::julianCycle(double d, double lw) { return round(d - SUN_J0 - lw / (2.0 * SUN_PI)); }

double SunMoon::approxTransit(double Ht, double lw, double n) { return SUN_J0 + (Ht + lw) / (2.0 * SUN_PI) + n; }
unsigned long SunMoon::solarTransitJ(double ds, double M, double L)  { 

  uint64_t ds64 = ds * 100000ULL;
  uint64_t r = ds64 + 530ULL*sin(M) - 690ULL*sin(2.0*L) + 1095750000ULL;
  r = r * 864ULL;
  r = r / 1000;
  return r + _hoursShift;


//  unsigned long r = (ds + 0.0053 * sin(M) - 0.0069 * sin(2.0 * L) + 10957.5)  * SUN_DAYS_MS; 
  }

double SunMoon::hourAngle(double h, double phi, double d) { return acos((sin(h) - sin(phi) * sin(d)) / (cos(phi) * cos(d))); }

// returns set time for the given sun altitude
unsigned long SunMoon::getSetJ(double h, double lw, double phi, double dec, double n, double M, double L) {

    double w = hourAngle(h, phi, dec);
    double a = approxTransit(w, lw, n);
    return solarTransitJ(a, M, L);
}

timesInfo SunMoon::getTimes(unsigned long date, double angle){

    double lw = SUN_RAD * (-1.0 * _lng);
    double phi = SUN_RAD * (1.0 * _lat);
    double    d = toDays(date);
    double    n = julianCycle(d, lw);
    double    ds = approxTransit(0, lw, n);
    double    M = solarMeanAnomaly(ds);
    double    L = eclipticLongitude(M);
    double    dec = declination(L, 0);
    unsigned long    Jnoon = solarTransitJ(ds, M, L);
    unsigned long Jset = getSetJ(angle * SUN_RAD, lw, phi, dec, n, M, L);
    unsigned long Jrise = Jnoon - (Jset - Jnoon);

    timesInfo r = {
      Jrise, Jset
    };

    return r;

};

timesInfo SunMoon::getTimes(unsigned long date){ return getTimes(date, -0.833); }


#endif // _SUNMOON_H_