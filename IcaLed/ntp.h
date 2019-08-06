/* Time Management */

#include <TimeLib.h>
#include <NtpClientLib.h>

const PROGMEM char *ntpServer = "pool.ntp.org";
int8_t timeZone = 1;
int8_t minutesTimeZone = 0;
boolean syncEventTriggered = false; // True if a time even has been triggered
boolean timeSync = false;
NTPSyncEvent_t ntpEvent; // Last triggered event

void processSyncEvent (NTPSyncEvent_t ntpEvent) {
    Serial.print(".");
    if (ntpEvent < 0) {
        Serial.printf ("Time Sync error: %d\n", ntpEvent);
        if (ntpEvent == noResponse)
            Serial.println ("NTP server not reachable");
        else if (ntpEvent == invalidAddress)
            Serial.println ("Invalid NTP server address");
        else if (ntpEvent == errorSending)
            Serial.println ("Error sending request");
        else if (ntpEvent == responseError)
            Serial.println ("NTP response error");
    } else {
        if (ntpEvent == timeSyncd) {
            Serial.print ("Got NTP time: ");
            Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
            setTime(NTP.getTime());
            timeSync = true;
        }
    }
}

void startNTP(){
    NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
    });
    NTP.setInterval(3600);
    //Server / TimeZone / Daylight Saving / Minutes Offset
    NTP.begin(ntpServer, timeZone, true, minutesTimeZone);
}

void checkNTP(){
    if (syncEventTriggered) {
        processSyncEvent(ntpEvent);
        syncEventTriggered = false;
    }
}
