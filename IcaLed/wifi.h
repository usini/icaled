/* Wifi Management */

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <TimeLib.h>

ESP8266WiFiMulti WiFiMulti;
WiFiClient* stream;
HTTPClient https;
std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
unsigned long lastConnectionTime = 0;                  // last time you connected to the server, in milliseconds
#define NBREVENTS 30       // Maximum events stored
#define EVENTTEXTSIZE 80   // Size of text description of event    "'Initiation a la decoupe laser'
#define EVENTDATESIZE 50   // Size of date part of event + spaces. " le Vendredi 31 Septembre 18h15->20h00          "
#define HEADERSIZE 50      // Banner (introductory text)
#define FOOTERSIZE 50

//*********************************************************************/
// Connect or reconnect wifi
//*********************************************************************/
void connectWifi() {
  // Trying to (re)connect to wifi network
  Serial.println("Connecting wifi...");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid1, password1);
  WiFiMulti.addAP(ssid2, password2);
  client->setInsecure();
  // Wait for connection
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
  }
  // Connected ! Display SSID & IP
  Serial.println("Connected.");
  Serial.print("SSID: ");
  Serial.println(ssid1);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

//*********************************************************************/
// this method makes a HTTPS connection to the server:
//*********************************************************************/
// Ignore SSL Certificate : https://buger.dread.cz/simple-esp8266-https-client-without-verification-of-certificate-fingerprint.html
// Taken from BasicHTTPSClient example
//

boolean httpRequest() {
    Serial.println("Connecting to host");
    Serial.print("[HTTPS] begin...\n");
    
    // if there's a successful connection:
    if (https.begin(*client, url)) {
      Serial.println("Requesting URL");
      Serial.println(url);

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      https.setReuse(true);

      // httpCode will be negative on error
      if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        
        //The page is empty, we want the file which is downloaded
        int len = https.getSize();
        Serial.println(len);
        if(https.connected()) {
          stream = https.getStreamPtr(); //Get file stream
        } else {
          Serial.println("Invalid Stream");
          https.end();
          return false;
        }
      } else {
        Serial.println("Invalid HTTP CODE");
        https.end();
        return false;
      }
      // note the time that the connection was made:
      lastConnectionTime = millis();

      return true;
    } else {
      // if you couldn't make a connection:
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      Serial.println("Connection failed Invalid Request");
      https.end();
      return false;
    }
  } else {
    Serial.println("Request failed");
    https.end();
    return false;
  }
}


//*********************************************************************/
// Calendar
//*********************************************************************/

char allEvents[((EVENTTEXTSIZE + EVENTDATESIZE)*(NBREVENTS)) + HEADERSIZE + FOOTERSIZE ]; // string containing all processed events to display.
int allEventsSize = 0;     // number of chars if allEvents
time_t bEvent[NBREVENTS];  // array of NBREVENTS events: start date, end date, description.
time_t eEvent[NBREVENTS];
char  dEvent[NBREVENTS][EVENTTEXTSIZE];
int nbrEvents = 0;         // number of active events in array
const char* monthNames[] = { "Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"};
//const char* monthNames[] = { "January","February","March","April","May","June","Juillet","Aout","Septembre","Octobre","Novembre","Decembre"};

const char* dayNames[] = { "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};



//*********************************************************************/
// Swap sort events
//*********************************************************************/
void sortEvents() {
  time_t tmpbEvent;
  time_t tmpeEvent;
  char  tmpdEvent[EVENTTEXTSIZE];

  for (int j = 0; j < nbrEvents - 1; j++) {
    for (int i = j; i >= 0; i--) {
      if (bEvent[i] > bEvent[i + 1]) {
        // t=tab[i];
        tmpbEvent = bEvent[i];
        tmpeEvent = eEvent[i];
        memset (tmpdEvent, 0, EVENTTEXTSIZE);
        strcpy(tmpdEvent, dEvent[i]);
        // tab[i]=tab[i+1];
        bEvent[i] = bEvent[i + 1];
        eEvent[i] = eEvent[i + 1];
        memset (dEvent[i], 0, EVENTTEXTSIZE);
        strcpy(dEvent[i], dEvent[i + 1]);
        //tab[i+1]=t;
        bEvent[i + 1] = tmpbEvent;
        eEvent[i + 1] = tmpeEvent;
        memset (dEvent[i + 1], 0, EVENTTEXTSIZE);
        strcpy(dEvent[i + 1], tmpdEvent);
      }
    }
  }
}


//*********************************************************************/
// Parse an Ical file
//*********************************************************************/
// FORMAT :
// BEGIN:VCALENDAR
//  BEGIN:VEVENT
//    DTSTART (ex: DTSTART;TZID=Europe/Paris:20150818T181500 )
//    DTEND (ex: DTEND;TZID=Europe/Paris:20150818T200000 )
//    SUMMARY (ex: SUMMARY:Utilisation des imprimantes 3D du lab )
//  END:VEVENT
// END:VCALENDAR
//*********************************************************************/
void parseResponse() {
  String s_ddate;                 // for storing date temporarily in String format
  String s_event;                 // for storing event description temporarily
  tmElements_t tm_date;           // for storing date temporarily in tmElements_t format
  char oneEvent[EVENTTEXTSIZE + 30]; // String for storing one decoded event, ready to aggregate with allEvents
  bool eventStarted = false;  // state machine flag for decoding events.
  int i = 0;                  // loop counter
  bool endOfFeedRead = false; // State flag to be sure feed have beet completely read=.
  Serial.println("Stream Started");
  while (stream->available()) {
    // if any supported tokens
    String line = stream->readStringUntil('\n');
    line.trim();
    //Serial.println(line);
    String check_endcalendar = line.substring(0,13);
    String check_beginevent = line.substring(0,12);
    String check_endevent = line.substring(0,10);

    //Serial.println(line);
    //Serial.println("-----");
    //Serial.println(line);
    delay(10); // for 4G
    if (check_endcalendar.equals("END:VCALENDAR")) {
      //Serial.println("GOT END OF CALENDAR");
      endOfFeedRead = true;
    }
    if (check_beginevent.equals("BEGIN:VEVENT")) {
      //Serial.println("GOT BEGIN EVENT");
      eventStarted = true;
    }
    if  (check_endevent.equals("END:VEVENT")) {
      //Serial.println("GOT END EVENT");
      eventStarted = false;
      if (i <20) {
        i += 1;
        }
      if (i == 20) {
        Serial.print ("Warning : Ical feed contains more than ");
        Serial.print (NBREVENTS);
        Serial.println (" events.");
      }
    }
    if  (eventStarted && (i < NBREVENTS) ) { // We're only interested on DTSTART/END for events.
      String check_dtstart = line.substring(0,7);
      String check_dtend = line.substring(0,5);
      /*
      Serial.print("EVENT START:");
      Serial.println(check_dtstart);
      Serial.print("EVENT END:");
      Serial.println(check_dtend);
      */

      if ((check_dtstart.equals("DTSTART")) || (check_dtend.equals("DTEND"))) {
        // Serial.println(line);
        // skip timezone
        s_ddate = line.substring(line.indexOf(':') + 1);

        // convert to Y/M/D in a tmElements_t struct
        tm_date.Year   =  CalendarYrToTm(s_ddate.substring(0, 4).toInt()); // year (since 1970)
        tm_date.Month  =  s_ddate.substring(4, 6).toInt(); // month
        tm_date.Day    =  s_ddate.substring(6, 8).toInt(); // day
        tm_date.Hour   =  s_ddate.substring(9, 11).toInt(); // hour
        tm_date.Minute =  s_ddate.substring(11, 13).toInt(); // min
        tm_date.Second =  s_ddate.substring(13, 15).toInt(); // sec

        // Convert in time_t format ans store it
        if ((check_dtstart.equals("DTSTART"))) {
          //Serial.print("START TIME: ");
          bEvent[i] = makeTime(tm_date);
          //Serial.println(bEvent[i]);
        }
        else {
          //Serial.println("END TIME: ");
          eEvent[i] = makeTime(tm_date);
          //Serial.println(eEvent[i]);
        }
        // hack : if endevent <= now(), event is past. Discard it.
        /*
        if (eEvent[i] <= now()) {
          eventStarted = false;
        }
        */
      }

      //Serial.println(line);
      if (check_dtstart.equals("SUMMARY")) {
        // Event description
        //Serial.println("DESCRIPTION: ");
        s_event = line.substring(line.indexOf(':') + 1);
        s_event.toCharArray(dEvent[i], s_event.length() + 1);
        //Serial.println(s_event);
        // Unicode decode
      }
    }
  } // end while client available

  stream->stop();
  https.end();
  // ICAL file completely read?
  if (endOfFeedRead == true) {
    // Debug display # of valid events
    Serial.print("Valid events read: ");
    Serial.println(i);
    // Store number of valid events read.
    nbrEvents = i;

    // Sort events
    Serial.println("Sorting...");
    //sortEvents();

    Serial.println("Generate big string...");
    Serial.print(nbrEvents);
    Serial.println(" Events");
    // And generate a big string to display on Led matrix...
    memset (allEvents, '\0', sizeof(allEvents));
    strcpy (allEvents, HEADERTEXT);
    // Concatenate all events
    for (i = 0; i < nbrEvents; i++) {
      // If event is now
      //Serial.print("DEBUT:");
      //Serial.println(bEvent[i]);
      //Serial.print("MAINTENANT:");
      //Serial.println(now());
      //Serial.print("FIN:");
      //Serial.println(eEvent[i]);

      if ( (bEvent[i] > now()) && (eEvent[i] < now()) ) {
        sprintf (oneEvent, "'%s' [En ce moment]            ", dEvent[i]);
        strcat (allEvents, oneEvent);
      }
      else {
        // Asssemble data for one event
        sprintf (oneEvent, "%s le %s %d %s %02dh%02d->%02dh%02d            ",
                 dEvent[i],
                 dayNames[weekday(bEvent[i]) - 1],
                 day(bEvent[i]),
                 monthNames[month(bEvent[i]) - 1],
                 hour(bEvent[i]),
                 minute(bEvent[i]),
                 hour(eEvent[i]),
                 minute(eEvent[i]));
        // and concat to all evets string
        strcat (allEvents, oneEvent);
      }
    }
    // actualize string length
    strcat(allEvents, FOOTERTEXT);
    allEventsSize = strlen(allEvents);
    // And display them
    Serial.println(allEvents);
    Serial.println("");
    // Prepare the character array (the buffer) 
    //char char_array[allEventsSize];
    // Copy it over
    //allEvents.toCharArray(char_array, allEventsSize);
    //char char_array_ascii[allEventsSize];
    matrixText(allEvents);
  }
  else {
    nbrEvents = 0;
    Serial.println ("ERROR: Ical feed uncomplete (no 'END:VCALENDAR' markup).");
  }
}
