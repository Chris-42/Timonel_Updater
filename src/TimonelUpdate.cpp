#include "TimonelUpdate.h"
//#include <LittleFS.h>
#include <esp_task_wdt.h>
//#include "helper.h"
#include "html.h"

#ifndef MUX_ADDRESS
 #define MUX_ADDRESS 0x00
#endif

TimonelUpdater::TimonelUpdater() {
}

void TimonelUpdater::begin(WEBSERVER* server, String path, TwoWire* i2c) {
  _server = server;
  _i2c = i2c;
  _ws = new AsyncWebSocket(path + "/ws");
  //_server->serveStatic(path.c_str(), LittleFS, "/").setDefaultFile("update.html");
  _server->on(path.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", UPDATE_HTML, sizeof(UPDATE_HTML));
    response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });

  _ws->onEvent([&](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if(type == WS_EVT_CONNECT) {
      //Serial.println("Websocket client connection received");
      client->text("init");
    } else if(type == WS_EVT_DISCONNECT) {
      //Serial.println("Client disconnected");
    } else if(type == WS_EVT_DATA) {
      char str[len + 1];
      str[len] = '\0';
      strncpy(str, (const char*)data, len);
      //Serialprintf("ws rec: %s\r\n", str);
      String cmd(str);
      if(cmd == "Ping") {
        client->text("Pong");
      } else if(cmd == "rescan") {
        client->text("devices=empty");
        uint8_t num = scanBus();
        char str[32];
        sprintf(str, "Found %d devices", num);
        client->text(str);
        for(auto t : _timonel) {
          sprintf(str, "devices=0x%02X-%c", t->GetTwiAddress(), t->GetSignature());
          client->text(str);
        }
      } else if(cmd.startsWith("erase:")) {
        uint8_t twiId = strtol(cmd.substring(6).c_str(), 0, 16);
        if(twiId) {
          erase(twiId);
        } else {
          client->text("missing device");
        }
      } else if(cmd.startsWith("flash:")) {
        uint8_t twiId = strtol(cmd.substring(6).c_str(), 0, 16);
        if(twiId) {
          if(_flashTaskHandle) {
            client->text("currently flashing");
          } else {
            flash(twiId);
          }
        } else {
          client->text("missing device");
        }
      } else if(cmd.startsWith("startApp:")) {
        uint8_t twiId = strtol(cmd.substring(9).c_str(), 0, 16);
        if(twiId) {
          startApp(twiId);
        } else {
          client->text("missing device");
        }
      }
    }
  });

  _server->on(path.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", UPDATE_HTML, sizeof(UPDATE_HTML));
    response->addHeader("Content-Encoding", "gzip");
    return request->send(response);
  });

  _server->on(path.c_str(), HTTP_POST, [&](AsyncWebServerRequest *request) {
      // Post-update callback
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
      response->addHeader("Connection", "close");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
  }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      //Upload handler chunks in data
      if (!index) {
        // Reset progress size on first frame
        _progress = 0;
        _currentFile = filename;
        size_t len = request->contentLength();
        if(!len) {
          len = 22000;
        }
        if(_hexBuffer) {
          free(_hexBuffer);
        }
        _hexBuffer = (char*)malloc(len+1);
        _hexBuffer[0] = '\0';
      }

      // Write chunked data to the free sketch space
      if(len) {
        if(_hexBuffer) {
          bcopy(data, &_hexBuffer[_progress], len);
          _progress += len;
          _hexBuffer[_progress] = '\0';
        } else {
          return request->send(400, "text/plain", "Failed to write chunked data to free space");
        }
        // Progress update callback
        //if (progressUpdateCallback != NULL) progressUpdateCallback(_current_progress_size, request->contentLength());
      }
          
      if (final) { // if the final flag is set then this is the last frame of data
        _ws->textAll("parse hex");
        bool res = parseHexBuffer();
        char str[36];
        sprintf(str, "fileinfo=size:%d, start:%04X\r\n", _binSize, _startAddress);
        _ws->textAll(str);
        if(res) {
          _ws->textAll("file ok");
        } else {
          _ws->textAll("file bad");
        }
      } else {
        return;
      }
  });
  _server->addHandler(_ws);
}

bool TimonelUpdater::parseHexBuffer() {
  char* p = strchr(_hexBuffer, ':');
  int Bytes;
  int Addr;
  int Type;
  char Data[35];
  _startAddress = 0xFFFF;
  uint16_t end_address = 0;
  char q[46];
  q[43] = '\0';
  while(p) {
    strncpy(q, p, 43);
    int n;
    if((n = sscanf (q, ":%2x%4x%2x%s", &Bytes, &Addr, &Type, &Data)) != 4) {
      Serial.println("hex decode failed");
      return false;
    }
    if(Type == 0) {
      if(_startAddress > Addr) {
        _startAddress = Addr;
      }
      if(end_address < (Addr + Bytes)) {
        end_address = Addr + Bytes;
      }
    }
    p = strchr(p+1, ':');
  };
  _binSize = end_address - _startAddress;

  if(_binBuffer) {
    free(_binBuffer);
  }
  _binBuffer = (uint8_t*)malloc(_binSize);
  if(!_binBuffer) {
    Serial.println("malloc failed");
    return false;
  }
  p = strchr(_hexBuffer, ':');
  while(p) {
    strncpy(q, p, 43);
    if(sscanf (q, ":%2x%4x%2x%s", &Bytes, &Addr, &Type, &Data) != 4) {
      Serial.println("hex decode2 failed");
      return false;
    }
    if(Type == 0) {
      for(int i = 0; i < Bytes; ++i) {
        uint8_t b;
        if(sscanf(&Data[i*2], "%2x", &b) != 1) {
          Serial.println("byte parse failed");
          return 0;
        }
        _binBuffer[(Addr + i) - _startAddress] = b;
      }
    }
     p = strchr(p+1, ':');
  }
  return true;
}

uint8_t TimonelUpdater::scanBus(int8_t busId) {
  #ifdef HAS_I2C_MUX
  if(_i2cMux) {
    _i2cMux->setChannel(busId);
    if(busId < 0) {
      _i2cMux->setMux(0);
    } else {
      if(!_i2cMux->setMuxChannel(busId)) {
        Serial.println("unable to set mux");
      }
    }
  }
  #endif
  //Serial.printf("Scanning I2C Addresses Channel %d\r\n", busId);
  for(auto t : _timonel) {
    delete(t);
  }
  _timonel.clear();
  for(uint8_t i = 1; i < 128; ++i) {
    _i2c->beginTransmission(i);
    uint8_t ec = _i2c->endTransmission(true);
    Serial.printf("T %d -> %d\r\n", i, ec);
    if(ec == 0 && (i != MUX_ADDRESS)) {
      Timonel *micro = new Timonel(i, *_i2c);
      if(micro) {
        if(!micro->Detected()) {  
          delete(micro);
        } else {
          Timonel::Status s = micro->GetStatus();
          _timonel.push_back(micro);
          //Serial.printf("detected %02X%c\r\n", i, s.signature);
        }
      }
    }
    esp_task_wdt_reset();
  }
  #ifdef HAS_I2C_MUX
  _i2cMux->free();
  #endif
  //Serial.printf("found %d Timonel Devices\r\n", _timonel.size());
  return _timonel.size();
}

void TimonelUpdater::startApp(uint8_t twiId) {
  //Serial.printf("start App %02X\r\n", twiId);
  for(auto t : _timonel) {
    if(t->GetTwiAddress() == twiId) {
      t->RunApplication();
      //Serial.println(F("started Timo"));
      return;
    }
  }
  ("rescan");
}

AsyncWebSocket* static_ws;
static void callbackHelper(uint8_t progress) {
  static int8_t last_progress = -1;
  if(progress < last_progress) {
    last_progress = progress - 5;
  }
  if((progress > (last_progress + 5)) || (progress == 100)) {
    last_progress = progress;
    static_ws->textAll(String("progress=") + String(progress));
  }
}

void TimonelUpdater::erase(uint8_t twiId) {
  //Serial.printf("erase %02X\r\n", twiId);
  ("erasing flash");
  for(auto t : _timonel) {
    if(t->GetTwiAddress() == twiId) {
      //Serial.println(F("clean flash"));
      t->DeleteApplication();
      //Serial.println(F("clean done"));
      break;
    }
  }
  _ws->textAll("erasing flash done");
}

void TimonelUpdater::flash(uint8_t twiId) {
  _activeTwiId = twiId;
  xTaskCreatePinnedToCore(&flashTask, "Flash", 4000, this, 5, &_flashTaskHandle, ARDUINO_RUNNING_CORE);
}

void TimonelUpdater::flashTask(void* data) {
  TimonelUpdater* p_this = (TimonelUpdater*)data;
  p_this->flashTaskWorker();
  p_this->_flashTaskHandle = nullptr;
  vTaskDelete(NULL);
}

void TimonelUpdater::flashTaskWorker() {
  //Serial.printf("flash %02X\r\n", _activeTwiId);
  _ws->textAll("flashing");
  if(!_binSize) {
    Serial.println(F("no binary"));
    _ws->textAll("file bad");
    return;
  }
  for(auto t : _timonel) {
    if(t->GetTwiAddress() == _activeTwiId) {
      static_ws = _ws;
      t->setProgressCallback(callbackHelper);
      I2C_MUTEX_LOCK
      uint8_t ret = t->UploadApplication(_binBuffer, _binSize, _startAddress);
      I2C_MUTEX_FREE
      t->setProgressCallback(nullptr);
      //Serial.printf(F("flashed Timo %d\r\n"), ret);
      if(ret) {
        _ws->textAll("flashing failed");
      } else {
        _ws->textAll("flashing done");
      }
      break;
    }
  }
}
