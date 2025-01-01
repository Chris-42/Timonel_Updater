//#include "NbMicro.h"
//#include "TimonelTwiM.h"
#include "my_Network.h"
#include "TimonelUpdate.h"
#include <ArduinoOTA.h>

#ifdef DEBUG_LEVEL
  #define Serialprint(...) Serial.print(__VA_ARGS__)
  #define Serialprintln(...) Serial.println(__VA_ARGS__)
  #define Serialprintf(...) Serial.printf(__VA_ARGS__)
#else
  #define Serialprint(...)
  #define Serialprintln(...)
  #define Serialprintf(...)
#endif

const char* hostname = HOSTNAME;
const char* ssid = MY_SSID;
const char* password = MY_SSID_PW;

#define SDA_PIN 38
#define SCL_PIN 39
#define I2C_SPEED 300000

std::vector<Timonel*> timonel;

AsyncWebServer server(80);
TimonelUpdater timonelUpdate;

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serialprintln("WiFi connected");
  Serialprint("IP address: ");
  Serialprintln(IPAddress(info.got_ip.ip_info.ip.addr));
  Serialprint("Hostname: ");
  Serialprintln(WiFi.getHostname());
  Serialprint("RSSI: ");
  Serialprintln(WiFi.RSSI());
}

void WiFiStart(bool reconnect = false) {
  if(reconnect) {  
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    delay(1);
  }
  WiFi.mode(WIFI_STA);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  delay(1);
  WiFi.setHostname(hostname);
}

void setup() {
  #ifdef DEBUG_LEVEL
    delay(4000);
    Serial.begin(115200);
  #endif
  Wire.end();
  Wire.begin(SDA_PIN, SCL_PIN, I2C_SPEED);
  //Wire.setTimeOut(10);
  
  WiFiStart();
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  server.begin();
  timonelUpdate.begin(&server);

}

void loop() {

}