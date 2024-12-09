#ifndef WEB_FUNCIONS_H
#define WEB_FUNCIONS_H

#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "TimonelTwiM.h"

#define WEBSERVER AsyncWebServer

class TimonelUpdater {
public:
  struct task_data_t {
    TimonelUpdater* obj;
    uint8_t cmd;
    uint8_t twiId;
    AsyncWebSocket* ws;
    Timonel* t;
  };
  TimonelUpdater();
  ~TimonelUpdater() {};
  void begin(WEBSERVER *server, String path = "/timonel", TwoWire* i2c = &Wire);
  uint8_t scanBus(int8_t busId = -1);
private:
  void handleWebsocketsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
  bool parseHexBuffer();
  void startApp(uint8_t twiId);
  void erase(uint8_t twiId);
  void flash(uint8_t twiId);
  static void flashTask(void* data);
  void flashTaskWorker();
  WEBSERVER* _server;
  AsyncWebSocket* _ws;
  unsigned long _progress;
  String _currentFile;
  char* _hexBuffer;
  uint8_t* _binBuffer;
  uint16_t _startAddress;
  uint16_t _binSize;
  std::vector<Timonel*> _timonel;
  TwoWire* _i2c;
  TaskHandle_t _flashTaskHandle;
  task_data_t _task_data;
  uint8_t _activeTwiId;
#ifdef HAS_I2C_MUX
  i2c_muxer _i2cMux;
#endif
};

#endif