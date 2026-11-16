#include "src/hal/companion_ble.h"
#ifdef PALA_COMPANION_TEST
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <esp_mac.h>
#include <esp_random.h>
#include "src/state.h"
#include "src/storage/fs_util.h"
#include "src/storage/library.h"
#include "src/ui/header_title.h"
#include "src/hal/companion_rpc.h"

namespace {
constexpr const char* SERVICE="a28b1001-4d48-4e42-9b82-6b6c50414c41";
constexpr const char* IDENTITY="a28b1002-4d48-4e42-9b82-6b6c50414c41";
constexpr const char* CONTROL="a28b1003-4d48-4e42-9b82-6b6c50414c41";
constexpr const char* RESPONSE="a28b1004-4d48-4e42-9b82-6b6c50414c41";
constexpr const char* DATA="a28b1005-4d48-4e42-9b82-6b6c50414c41";
constexpr const char* SIGNAL="a28b1006-4d48-4e42-9b82-6b6c50414c41";
constexpr uint8_t QUEUE_CAPACITY=8;
String pin;
bool running=false;
volatile bool connected=false;
uint16_t owner=BLE_HS_CONN_HANDLE_NONE;
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Packet { uint8_t bytes[512]; size_t length; bool data; uint32_t generation, session; };
Packet queue[QUEUE_CAPACITY];uint8_t head=0,count=0;
uint32_t generation=0,session=0;
bool resetRequested=false,redraw=false;
uint8_t identityBytes[512];size_t identityLength=0;
NimBLECharacteristic* responseCharacteristic=nullptr;
NimBLECharacteristic* signalCharacteristic=nullptr;

class ServerCallbacks:public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server,NimBLEConnInfo& c) override {
    if(connected){server->disconnect(c.getConnHandle());return;}
    uint32_t token=esp_random();if(!token)token=1;
    owner=c.getConnHandle();connected=true;
    portENTER_CRITICAL(&mux);head=0;count=0;generation++;session=token;resetRequested=true;redraw=true;portEXIT_CRITICAL(&mux);
    responseCharacteristic->setValue("{\"id\":0,\"ok\":true}");
    server->updateConnParams(owner,12,24,0,400);
    Serial0.println("[companion] BLE link opened");
  }
  void onDisconnect(NimBLEServer*,NimBLEConnInfo& c,int reason) override {
    if(c.getConnHandle()!=owner)return;
    connected=false;owner=BLE_HS_CONN_HANDLE_NONE;
    portENTER_CRITICAL(&mux);head=0;count=0;generation++;session=0;resetRequested=true;redraw=true;portEXIT_CRITICAL(&mux);
    Serial0.printf("[companion] BLE link closed: %d\n",reason);
  }
  void onAuthenticationComplete(NimBLEConnInfo& c) override {
    Serial0.printf("[companion] BLE authenticated: %d\n",c.isAuthenticated());
    portENTER_CRITICAL(&mux);redraw=true;portEXIT_CRITICAL(&mux);
  }
};
ServerCallbacks serverCallbacks;
class IdentityCallbacks:public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* c,NimBLEConnInfo& peer) override {
    if(!peer.isEncrypted()||!peer.isAuthenticated()){c->setValue("");return;}
    uint8_t bytes[512];size_t length;uint32_t token;
    portENTER_CRITICAL(&mux);length=identityLength;memcpy(bytes,identityBytes,length);token=session;portEXIT_CRITICAL(&mux);
    JsonDocument doc;if(deserializeJson(doc,bytes,length)){c->setValue("");return;}
    doc["session"]=token;String value;serializeJson(doc,value);c->setValue(value.c_str());
  }
};
IdentityCallbacks identityCallbacks;
class QueueCallbacks:public NimBLECharacteristicCallbacks {
public:
  explicit QueueCallbacks(bool data):isData(data){}
  void onWrite(NimBLECharacteristic* c,NimBLEConnInfo& peer) override {
    if(peer.getConnHandle()!=owner||!peer.isEncrypted()||!peer.isAuthenticated())return;
    auto value=c->getValue();if(value.size()>512||!value.size())return;
    if(!isData && responseCharacteristic)responseCharacteristic->setValue("{\"id\":0,\"ok\":true}");
    // Bounded ring accepts a four-packet window. All FS/NVS work stays on the main loop.
    portENTER_CRITICAL(&mux);
    if(count<QUEUE_CAPACITY){auto& packet=queue[(head+count)%QUEUE_CAPACITY];memcpy(packet.bytes,value.data(),value.size());packet.length=value.size();packet.data=isData;packet.generation=generation;packet.session=session;count++;}
    portEXIT_CRITICAL(&mux);
  }
private:bool isData;
};
QueueCallbacks controlCallbacks(false),dataCallbacks(true);
}

void companionBleRefreshIdentity() {
  uint8_t mac[6];esp_read_mac(mac,ESP_MAC_WIFI_STA);char id[18];
  snprintf(id,sizeof(id),"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  JsonDocument doc;doc["protocol"]="pala-companion";doc["version"]=3;doc["transport"]="ble";
  doc["model"]="pala-one";doc["id"]=id;doc["name"]=HeaderTitle::current();doc["firmware"]=FW_VERSION;doc["freeBytes"]=fsFreeBytesSafe();
  auto capabilities=doc["capabilities"].to<JsonArray>();capabilities.add("bookmarks-v1");capabilities.add("bulk-window-v1");capabilities.add("session-v1");capabilities.add("pbt1");
  String value;serializeJson(doc,value);
  if(value.length()>sizeof(identityBytes))return;
  portENTER_CRITICAL(&mux);identityLength=value.length();memcpy(identityBytes,value.c_str(),identityLength);portEXIT_CRITICAL(&mux);
}
void companionBleBegin() {
  if(running)return;
  companionRpcReset();loadBooks();companionBleRefreshIdentity();
  uint32_t number=100000+esp_random()%900000;pin=String(number);
  if(!NimBLEDevice::init("Pala One")){Serial0.println("[companion] BLE init failed");return;}
  NimBLEDevice::setMTU(247);NimBLEDevice::setSecurityAuth(true,true,true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);NimBLEDevice::setSecurityPasskey(number);
  auto* server=NimBLEDevice::createServer();server->setCallbacks(&serverCallbacks,false);server->advertiseOnDisconnect(true);
  auto* service=server->createService(SERVICE);
  auto* identity=service->createCharacteristic(IDENTITY,NIMBLE_PROPERTY::READ|NIMBLE_PROPERTY::READ_ENC|NIMBLE_PROPERTY::READ_AUTHEN,512);identity->setCallbacks(&identityCallbacks);
  auto* control=service->createCharacteristic(CONTROL,NIMBLE_PROPERTY::WRITE|NIMBLE_PROPERTY::WRITE_ENC|NIMBLE_PROPERTY::WRITE_AUTHEN,512);control->setCallbacks(&controlCallbacks);
  auto* data=service->createCharacteristic(DATA,NIMBLE_PROPERTY::WRITE|NIMBLE_PROPERTY::WRITE_NR|NIMBLE_PROPERTY::WRITE_ENC|NIMBLE_PROPERTY::WRITE_AUTHEN,512);data->setCallbacks(&dataCallbacks);
  responseCharacteristic=service->createCharacteristic(RESPONSE,NIMBLE_PROPERTY::READ|NIMBLE_PROPERTY::READ_ENC|NIMBLE_PROPERTY::READ_AUTHEN,512);responseCharacteristic->setValue("{\"id\":0,\"ok\":true}");
  signalCharacteristic=service->createCharacteristic(SIGNAL,NIMBLE_PROPERTY::NOTIFY,4);
  auto* advertising=NimBLEDevice::getAdvertising();advertising->addServiceUUID(SERVICE);advertising->enableScanResponse(true);advertising->setName("Pala One");advertising->enableScanResponse(true);
  running=advertising->start();Serial0.printf("[companion] Bluetooth service active, window=4: %d\n",running);
  if(!running){NimBLEDevice::deinit(true);responseCharacteristic=nullptr;signalCharacteristic=nullptr;}
}
void companionBleTick() {
  if(!running)return;
  String latest;uint32_t latestGeneration=0;
  // Bound each iteration so input handling remains responsive during transfers.
  for(uint8_t i=0;i<QUEUE_CAPACITY;i++) {
    Packet packet{};bool reset=false,hasPacket=false;
    portENTER_CRITICAL(&mux);reset=resetRequested;resetRequested=false;
    if(count){packet=queue[head];head=(head+1)%QUEUE_CAPACITY;count--;hasPacket=true;}
    portEXIT_CRITICAL(&mux);
    if(reset){companionRpcReset();latest="";}
    if(!hasPacket||!connected)break;
    String result=packet.data?companionRpcChunk(packet.bytes,packet.length):companionRpcControl(packet.bytes,packet.length);
    portENTER_CRITICAL(&mux);bool same=connected && packet.generation==generation;portEXIT_CRITICAL(&mux);
    if(!same)continue;
    JsonDocument reply;if(deserializeJson(reply,result))continue;
    reply["session"]=packet.session;latest="";serializeJson(reply,latest);latestGeneration=packet.generation;
    if(latest.length()>512){const uint32_t id=reply["id"]|0U;reply.clear();reply["id"]=id;reply["ok"]=false;reply["error"]="response-too-large";reply["session"]=packet.session;latest="";serializeJson(reply,latest);}
    // Publish an error immediately, so a later frame cannot hide a failed write.
    if(!(reply["ok"]|false))break;
  }
  portENTER_CRITICAL(&mux);bool same=connected && latestGeneration==generation;portEXIT_CRITICAL(&mux);
  if(!latest.length()||!same)return;
  responseCharacteristic->setValue(latest.c_str());
  JsonDocument reply;if(!deserializeJson(reply,latest)){uint32_t id=reply["id"]|0U;uint8_t signal[4]={uint8_t(id),uint8_t(id>>8),uint8_t(id>>16),uint8_t(id>>24)};signalCharacteristic->notify(signal,sizeof(signal));}
}
void companionBleEnd() {
  if(running){NimBLEDevice::deinit(true);running=false;}
  responseCharacteristic=nullptr;signalCharacteristic=nullptr;connected=false;owner=BLE_HS_CONN_HANDLE_NONE;
  portENTER_CRITICAL(&mux);head=0;count=0;resetRequested=false;generation++;session=0;identityLength=0;portEXIT_CRITICAL(&mux);
  companionRpcReset();
}
bool companionBleConnected(){return connected;}
bool companionBleRunning(){return running;}
bool companionBleNeedsRedraw(){portENTER_CRITICAL(&mux);bool value=redraw;redraw=false;portEXIT_CRITICAL(&mux);return value;}
String companionPairingPin(){return pin;}
void companionResetPairing(){if(running)NimBLEDevice::deleteAllBonds();prefs.remove("app_token");}
#else
void companionBleBegin(){}
void companionBleEnd(){}
void companionBleTick(){}
void companionBleRefreshIdentity(){}
bool companionBleConnected(){return false;}
bool companionBleRunning(){return false;}
bool companionBleNeedsRedraw(){return false;}
String companionPairingPin(){return "";}
void companionResetPairing(){}
#endif
