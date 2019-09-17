/*********************************************************************
IcaLED
Description : Display ICAL events on 8x8 MAX7219 Dot Matrix Modules from a wordpress plugin
Original Author : JP CIVADE 
Fork : Rémi Sarrailh
Licence     : GPL V3
Developped for LABSud, Montpellier FABLAB do display current and next events.

Libraries used
* MD_Parola
* MD_MAX72XX
* NTPClientLib
* ESPAsyncUDP - https://github.com/me-no-dev/ESPAsyncUDP
* TimeLib - https://github.com/PaulStoffregen/Time
*/

#include "settings.h"
#include "matrix.h"
#include "wifi.h"
#include "ntp.h"
#include <FS.h>

void setup() {

  Serial.begin(74880); // Default Baudrate for ESP8266 https://www.esp8266.com/viewtopic.php?t=12635
  Serial.println("");
  Serial.print("https://github.com/usini/icaled - ");
  Serial.println(VERSION);

  //https://circuits4you.com/2018/01/31/example-of-esp8266-flash-file-system-spiffs/
  SPIFFS.begin();
  bool calendar_exists = SPIFFS.exists("calendar.txt");
  if(calendar_exists){
    Serial.println("Display calendar from memory");
    File fileRead = SPIFFS.open("calendar.txt", "r");

    while(fileRead.available()){
      for(int i=0; i <fileRead.size();i++){
        allEvents[i] = (char)fileRead.read();
      }
    }
    fileRead.close();
    matrixText(allEvents);
  } else {
    matrixText("First Start!");
  }

  //Matrix
  initMatrix();
  //matrixText("WiFi");

  // Connect to WiFi
  connectWifi();

  //Start NTP client (to get time)
  //matrixText("Temps");
  startNTP();
  Serial.println("Syncing Time");

  //Wait to get Time on NTP server
  while(!timeSync){
    checkNTP();
    delay(100); // Without delay, this crash the code
  }
  Serial.println("Time synced...");
  Serial.println(now());

  //Get Calendar
  //matrixText("Calendrier");

  //Retry http request until it works (avoid timeout issue)
  while(true) {
    if(httpRequest()){
      break;
    }
  }

  Serial.println("Parsing...");
  parseResponse();
  Serial.println("Response parsed");
  matrixText(allEvents);

  File fileWrite = SPIFFS.open("calendar.txt", "w");
  if(fileWrite){
    fileWrite.write(utf8ascii(allEvents), sizeof(allEvents));
  }

  //Request finish, we don't need Wi-Fi anymore
  //WiFi.mode(WIFI_OFF);
  //WiFi.forceSleepBegin();
}

void loop() {
  //Check if it is time to recheck the schedule
  if ( ((millis() - lastConnectionTime) > connectionInterval) || (lastConnectionTime == 0)) {
    Serial.print("Last connection: ");
    Serial.print(millis() - lastConnectionTime);
    Serial.print(" / ");
    Serial.println(connectionInterval);
    ESP.reset(); //Since we basically need to redo everything we did on setup (as Wifi is disabled just reset)
  }
}
