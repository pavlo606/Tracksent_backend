#include <stdio.h>
#include <string.h>
#include <Regexp.h>
#include "GPSCoordinates.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#define sclk 52
#define mosi 51
#define cs   45
#define rst  46
#define dc   47

#define DEVICE_ID 1
#define FIELD_ID 2
  
#define DEBUG true
#define MODE_1A

#define BTN_PIN 3

#define DTR_PIN 9
#define RI_PIN 8
  
#define LTE_PWRKEY_PIN 5
#define LTE_RESET_PIN 6
#define LTE_FLIGHT_PIN 7

// Color definitions
#define  BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

String status = "Init..";  

String from_usb = "";
long int timer = 0;

Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);

void updateDisp(String resp = "") {
  display.fillScreen(BLACK);
  display.fillRect(0, 0, 95, 12, RED);
  display.setCursor(5, 2);
  display.setTextColor(BLACK);
  display.setTextSize(1);
  display.print("SIM7600E-H LTE");

  display.setCursor(10, 13);
  if (status == "Ready") {
    display.setTextColor(GREEN);
  } else if (status == "Error") {
    display.setTextColor(RED);
  } else {
    display.setTextColor(WHITE);
  }
  display.setTextSize(2);
  display.print(status);

  display.setCursor(0, 31);
  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.print(resp);
}

  
void setup()
{
    pinMode(BTN_PIN, OUTPUT);
    pinMode(13, OUTPUT);
    Serial.begin(115200);
    Serial1.begin(115200);
    display.begin();
    display.fillScreen(BLACK);
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.setTextSize(1);

    Serial.println("SIM7600 4G Test Start!");
    updateDisp();

    if (moduleStateCheck()) {   // Waiting for module turning on
      delay(1000);

      Serial.println("Initializing...");

      // sendData("AT+HTTPINIT\r\n", 1000, DEBUG);  // HTTP initializing
      
      if (sendData("AT+CGPS=1", 1000, DEBUG).indexOf("OK") >= 0) { // GPS initializing("OK" means that module need to make a cold stat of GPS, and if "ERROR" GPS is already initialized)
        delay(40000); // Waiting for GPS initializing
      }

      while (sendData("AT+CGPSINFO", 2000, DEBUG).length() < 50) {}  // Check if GPS coords is now aviable

      delay(1000);
      
      Serial.println("Module is ready!");
      status = "Ready";
      updateDisp();
      // sendGPS();

    } else {
      Serial.println("Module state check has failed");
      status = "Error";
      updateDisp();
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
            // Serial.println("SIM7600 Module had turned on.");
            // print("Module turn on");
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
        Serial.println(response);
        updateDisp(response);
    }
    return response;
}

void sendGPS() {
  Serial.println("Getting coordinates...");
  String resp = sendData("AT+CGPSINFO", 1000, DEBUG);
  char char_resp[100];

  resp.toCharArray(char_resp, 100);
  GPSCoordinates coords = getCoords(char_resp);

  if (coords.getLat() == 0 && coords.getLon() == 0) { return; }

  HTTPSendGPS(coords.getLat(), coords.getLon(), coords.getAlt());  
}

String getDatetime(){
  Serial.println("Getting time...");
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
  Serial.println("Sending data...");

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

// void updateDisp(String command = "", String resp = "") {
//   display.fillScreen(BLACK);
//   display.setCursor(0, 0);
//   display.setTextColor(WHITE);
//   display.setTextSize(1);
//   display.print("SIM7600E-H LTE");

//   display.setCursor(10, 10);
//   display.setTextColor(GREEN);
//   display.setTextSize(2);
//   display.print(status);

//   display.setCursor(0, 28);
//   display.setTextColor(WHITE);
//   display.setTextSize(1);
//   display.print(command);

//   display.setCursor(0, 38);
//   display.setTextColor(WHITE);
//   display.setTextSize(1);
//   display.print(resp);
// }

// void pop_first(String (&arr)[8], uint16_t (&colors)[8], size_t len) {
//   for (int i = 0; i < len - 1; i++) {
//     arr[i] = arr[i+1];
//     colors[i] = colors[i+1];
//   }
//   arr[len-1] = "";
//   colors[len-1] = WHITE;
// }

// void print(String text, uint16_t color = WHITE) {
//   display.fillScreen(BLACK);
//   display.setCursor(0, 0);
//   if (rows < 8) {
//     disp_data[rows] = text;
//     rows_color[rows] = color;
//     rows++;
//   } else {
//     pop_first(disp_data, rows_color, rows);
//     disp_data[rows-1] = text;
//     rows_color[rows-1] = color;
//   }
//   for (int i = 0; i < rows; i++) {
//     display.setTextColor(rows_color[i]);
//     display.println(disp_data[i]);
//   }
// }
