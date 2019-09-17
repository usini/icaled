
/* Settings */
#define VERSION "3.0"

// Wifi
const char* ssid1     = ""; //First WiFi Network
const char* password1 = "";
const char* ssid2     = ""; //Alternative WiFi Network
const char* password2 = "";

// HTTP Request
const char* url  = "https://labsud.org/?plugin=all-in-one-event-calendar&controller=ai1ec_exporter_controller&action=export_events";
static const unsigned long connectionInterval = 1 * (60 * (60 * 1000)); // Update ICAL feed each hour (in milliseconds)

// Message Settings
#define HEADERTEXT ""
#define FOOTERTEXT ""
#define SCROLLDELAY 20   //Delay between scroll (more is slower, less is faster)

// Matrix
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
/*
#define MAX_DEVICES 4
#define CLK_PIN   D7  // or SCK
#define DATA_PIN  D5  // or MOSI
#define CS_PIN    D6  // or SS
*/

#define MAX_DEVICES 8
#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D6  // or MOSI
#define CS_PIN    D7  // or SS