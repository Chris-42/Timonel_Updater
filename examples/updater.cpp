#include "NbMicro.h"
#include "TimonelTwiM.h"
#include "TimonelUpdate.h"

const char* hostname = HOSTNAME;
const char* ssid = MY_SSID;
const char* password = MY_SSID_PW;

#define SDA_PIN 38
#define SCL_PIN 39
#define I2C_SPEED 400000

std::vector<Timonel*> timonel;

AsyncWebServer server(80);
TimonelUpdater timonelUpdate;

void setup() {
  Wire.end();
  Wire.begin(SDA_PIN, SCL_PIN, I2C_SPEED);
  Wire.setTimeOut(10);
  
  // setup wifi

  server.begin();
  timonelUpdate.begin(&server);

}