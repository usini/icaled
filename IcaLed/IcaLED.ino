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

void setup() {

  Serial.begin(74880); // Default Baudrate for ESP8266 https://www.esp8266.com/viewtopic.php?t=12635
  Serial.println("");

  //Matrix
  initMatrix();
  matrixText("WiFi");

  // Connect to WiFi
  connectWifi();

  //Start NTP client (to get time)
  matrixText("Temps");
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
  matrixText("Calendrier");
  if (httpRequest()) {
    Serial.println("Parsing...");
    parseResponse();
    Serial.println("Response parsed");
  }

  //Request finish, we don't need Wi-Fi anymore
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
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
