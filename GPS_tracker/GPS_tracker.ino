#include <stdio.h>
#include <string.h>
#include <Regexp.h>
#include "GPSCoordinates.h"

#define DEVICE_ID 1
#define FIELD_ID 1
  
#define DEBUG true
#define MODE_1A
#define GPS_DELAY 5000

#define BTN_PIN 3
  
#define DTR_PIN 9
#define RI_PIN 8
  
#define LTE_PWRKEY_PIN 5
#define LTE_RESET_PIN 6
#define LTE_FLIGHT_PIN 7
  
String from_usb = "";
long int timer = 0;
  
void setup()
{
    pinMode(BTN_PIN, OUTPUT);
    pinMode(13, OUTPUT);
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial.println("SIM7600 4G Test Start!");

    if (moduleStateCheck()) {   // Waiting for module turning on
      delay(1000);

      Serial.println("Initializing...");

      sendData("AT+HTTPINIT\r\n", 1000, DEBUG);  // HTTP initializing
      
      if (sendData("AT+CGPS=1", 1000, DEBUG).indexOf("OK") >= 0) { // GPS initializing("OK" means that module need to make a cold stat of GPS, and if "ERROR" GPS is already initialized)
        delay(40000); // Waiting for GPS initializing
      }

      while (sendData("AT+CGPSINFO", 2000, DEBUG).length() < 50) {}  // Check if GPS coords is now aviable
      
      Serial.println("Module is ready!");
      // sendGPS();

    } else {
      Serial.println("Module state check has failed");
    }

    timer = millis();
}
  
void loop()
{
    if (millis() - timer >= 10000) {
      if (digitalRead(BTN_PIN)){
        sendGPS();
      }
      timer = millis();
    }
    // if (digitalRead(BTN_PIN)) {
    //   Serial.println("Button pressed");
    //   sendGPS();
    // }
    while (Serial1.available() > 0)
    {
        Serial.write(Serial1.read());
        yield();
    }
    while (Serial.available() > 0)
    {
#ifdef MODE_1A
        int c = -1;
        c = Serial.read();
        if (c != '\n' && c != '\r')
        {
            from_usb += (char)c;
        }
        else
        {
            if (!from_usb.equals(""))
            {
                sendData(from_usb, 0, DEBUG);
                from_usb = "";
            }
        }
#else
        Serial1.write(Serial.read());
        yield();
#endif
    }
}

GPSCoordinates getCoords(char* target) {
  MatchState ms;
  ms.Target(target);

  char result = ms.Match ("\+CGPSINFO: (%d+.%d+),([N,S]),(%d+.%d+),([E,W]),%d+,%d+.%d+,(%d+.%d+)");
  if (result == REGEXP_MATCHED)
  {
    char buf [100];

    GPSCoordinates coords(atof(ms.GetCapture (buf, 0)), ms.GetCapture (buf, 1)[0], atof(ms.GetCapture (buf, 2)), ms.GetCapture (buf, 3)[0], atof(ms.GetCapture (buf, 4)));

    return coords;

  } else if (result == REGEXP_NOMATCH)
  {
      Serial.println("no match");
      // return nullptr;
  }
}
  
bool moduleStateCheck()
{
    int i = 0;
    bool moduleState = false;
    for (i = 0; i < 10; i++)
    {
        String msg = String("");
        msg = sendData("AT", 2000, DEBUG);
        if (msg.indexOf("OK") >= 0)
        {
            Serial.println("SIM7600 Module had turned on.");
            moduleState = true;
            return moduleState;
        }
        delay(1000);
    }
    return moduleState;
}
  
String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    if (command.equals("1A") || command.equals("1a"))
    {
        Serial.println();
        Serial.println("Get a 1A, input a 0x1A");
  
        Serial1.write(0x1A);
        Serial1.println();
        return "";
    }
    else
    {
        Serial1.println(command);
    }
  
    long int time = millis();
    while ((time + timeout) > millis())
    {
        while (Serial1.available())
        {
            char c = Serial1.read();
            response += c;
        }
    }
    if (debug)
    {
        Serial.print(response);
    }
    return response;
}

void sendGPS() {
  String resp = sendData("AT+CGPSINFO", 1000, DEBUG);
  char char_resp[100];

  resp.toCharArray(char_resp, 100);
  GPSCoordinates coords = getCoords(char_resp);

  if (coords.getLat() == 0 && coords.getLon() == 0) { return; }

  HTTPSendGPS(coords.getLat(), coords.getLon(), coords.getAlt());  
}

String getDatetime(){
  String datetimeIn = sendData("AT+CCLK?", 100, DEBUG);
  String buff = "";
  for (int i = 19; i < 36; i++) {
    if (datetimeIn[i] == '/') {
      buff += '-';
    } else {
      buff += (char)datetimeIn[i];
    }
  }
  return buff;
}

void HTTPSendGPS(float lattitude, float longitude, float alt) {

  String http_str = "AT+HTTPPARA=\"URL\",\"https://pavlodykyi.pythonanywhere.com/coord/device?field_id="
                     + String(FIELD_ID) + "&vehicle_id=" + String(DEVICE_ID) + "&datetime=" + getDatetime() +
                     + "&lat=" + String(lattitude, 8) + "&long=" + String(longitude, 8) + "&alt=" + String(alt, 8) + "\"\r\n";
  Serial.println(http_str);

  sendData("AT+HTTPINIT", 100, DEBUG);
  sendData(http_str, 100, DEBUG);
  sendData("AT+HTTPACTION=0\r\n", 100, DEBUG);
  sendData("AT+HTTPTERM\r\n", 100, DEBUG);

  delay(100);
}
