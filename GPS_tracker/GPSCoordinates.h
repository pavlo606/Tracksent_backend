#ifndef GPSCoordinates_H
#define GPSCoordinates_H
#include "Arduino.h"

class GPSCoordinates{
    double lat;
    char N_S;
    double lon;
    char E_W;
    double alt;

public:
    GPSCoordinates(double lat, char N_S, double lon, char E_W, double alt);
    GPSCoordinates();

    double convert(double coord);
    double getLat();
    char getN_S();
    double getLon();
    char getE_W();
    double getAlt();
};

#endif