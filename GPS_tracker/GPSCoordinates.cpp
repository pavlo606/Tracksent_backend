#include "GPSCoordinates.h"

GPSCoordinates::GPSCoordinates(double lat, char N_S, double lon, char E_W, double alt) {
    this->lat = convert(lat);
    this->N_S = N_S;
    this->lon = convert(lon);
    this->E_W = E_W;
    this->alt = alt;
}

GPSCoordinates::GPSCoordinates() {
    this->lat = 0;
    this->N_S = 'N';
    this->lon = 0;
    this->E_W = 'S';
    this->alt = 0;
}

double GPSCoordinates::convert(double coord) {
    int hour = coord / 100;
    int min = coord - hour * 100;
    double sec = (coord - (int)coord) * 60;
    
    return hour + ((double)min/60) + ((double)sec/3600);
}

double GPSCoordinates::getLat() {
    return lat;
}

char GPSCoordinates::getN_S() {
    return N_S;
}

double GPSCoordinates::getLon() {
    return lon;
}

char GPSCoordinates::getE_W() {
    return E_W;
}

double GPSCoordinates::getAlt() {
    return alt;
}
