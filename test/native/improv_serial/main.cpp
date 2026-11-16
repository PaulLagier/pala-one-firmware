#include "test_framework.h"
#include <ImprovWiFiLibrary.h>
#include <deque>
#include <algorithm>

TestWiFi WiFi;
struct Wire : Stream {
  std::deque<uint8_t> input;
  std::vector<uint8_t> output;
  int available() override { return int(input.size()); }
  int read() override { if(input.empty()) return -1; auto b=input.front();input.pop_front();return b; }
  size_t write(const uint8_t* bytes,size_t n) override { output.insert(output.end(),bytes,bytes+n);return n; }
};
using Bytes = std::vector<uint8_t>;
static Bytes frame(const Bytes& payload) {
  Bytes bytes={'I','M','P','R','O','V',1,3,uint8_t(payload.size())};
  bytes.insert(bytes.end(),payload.begin(),payload.end());
  uint8_t checksum=0;for(auto b:bytes)checksum+=b;bytes.push_back(checksum);return bytes;
}
static void sendRaw(ImprovWiFi& parser,Wire& wire,const Bytes& bytes) {
  for(auto b:bytes)wire.input.push_back(b);
  while(wire.available())parser.handleSerial();
}
static void send(ImprovWiFi& parser,Wire& wire,const Bytes& payload) { sendRaw(parser,wire,frame(payload)); }
static void setup(ImprovWiFi& parser) {
  parser.setDeviceInfo(ImprovTypes::CF_ESP32_S3,"Pala One","serial-test","Pala One");
}
static bool validFrame(const Bytes& bytes) {
  if(bytes.size()<10 || bytes.size()!=size_t(bytes[8])+10) return false;
  const Bytes signature={'I','M','P','R','O','V',1};
  if(!std::equal(signature.begin(),signature.end(),bytes.begin()))return false;
  uint8_t sum=0;for(size_t i=0;i+1<bytes.size();++i)sum+=bytes[i];
  return sum==bytes.back();
}
static bool validReply(const Bytes& bytes) { return validFrame(bytes) && bytes[7]==4; }
static bool validFrames(const Bytes& bytes) {
  size_t offset=0;
  while(offset<bytes.size()) {
    if(bytes.size()-offset<10)return false;
    size_t length=size_t(bytes[offset+8])+10;
    if(length>bytes.size()-offset)return false;
    if(!validFrame(Bytes(bytes.begin()+offset,bytes.begin()+offset+length)))return false;
    offset+=length;
  }
  return true;
}
static std::string savedSsid,savedPassword;
static void connected(const char* ssid,const char* password) { savedSsid=ssid;savedPassword=password; }
static Bytes credentials(const std::string& ssid,const std::string& password) {
  Bytes data={1,uint8_t(ssid.size()+password.size()+2),uint8_t(ssid.size())};
  data.insert(data.end(),ssid.begin(),ssid.end());data.push_back(uint8_t(password.size()));
  data.insert(data.end(),password.begin(),password.end());return data;
}

TEST_CASE("every consecutive device-info query gets a checksummed reply") {
  Wire wire;ImprovWiFi parser(&wire);setup(parser);
  for(int i=0;i<30;++i) {
    wire.output.clear();send(parser,wire,{3,0});
    REQUIRE(validReply(wire.output));
    CHECK_EQ(wire.output[9],3);
    const std::string output(wire.output.begin(),wire.output.end());
    CHECK(output.find("serial-test")!=std::string::npos);
  }
}
TEST_CASE("short malformed credential records never call WiFi begin and recover") {
  Wire wire;ImprovWiFi parser(&wire);setup(parser);WiFi={};
  for(const Bytes& data : std::vector<Bytes>{{},{1},{1,0},{1,1,255},{1,3,1,'x',255},{1,5,2,'x','y',10,'z'}, {1,2,0,0}}) {
    send(parser,wire,data);CHECK_EQ(WiFi.beginCalls,0u);CHECK(validFrames(wire.output));
    wire.output.clear();send(parser,wire,{3,0});CHECK(validReply(wire.output));
  }
}
TEST_CASE("all receive lengths fit or are rejected without memory corruption") {
  Wire wire;ImprovWiFi parser(&wire);setup(parser);WiFi={};
  for(unsigned length=0;length<256;++length) {
    Bytes data(length,0);if(length)data[0]=0xfe;
    if(length>1)data[1]=uint8_t(length-2);
    send(parser,wire,data);CHECK_EQ(WiFi.beginCalls,0u);CHECK(validFrames(wire.output));
    wire.output.clear();send(parser,wire,{3,0});CHECK(validReply(wire.output));
  }
}
TEST_CASE("valid credentials preserve special characters and open-network passwords") {
  Wire wire;ImprovWiFi parser(&wire);setup(parser);WiFi={};parser.onImprovConnected(connected);
  for(const auto& password: {std::string("correct!password"),std::string()}) {
    const std::string ssid="Pala! café";
    wire.output.clear();send(parser,wire,credentials(ssid,password));
    CHECK(validFrames(wire.output));
    CHECK(savedSsid==ssid);CHECK(savedPassword==password);
  }
  CHECK_EQ(WiFi.beginCalls,2u);
}
TEST_CASE("bad checksums and signatures do not connect or poison the next request") {
  Wire wire;ImprovWiFi parser(&wire);setup(parser);WiFi={};
  Bytes bad=frame(credentials("test-network","test-password"));bad.back()^=0x40;
  sendRaw(parser,wire,bad);CHECK_EQ(WiFi.beginCalls,0u);
  wire.output.clear();send(parser,wire,{3,0});CHECK(validReply(wire.output));
  sendRaw(parser,wire,{'I','M','I','M'}); // Overlap with the next full signature.
  wire.output.clear();send(parser,wire,{3,0});CHECK(validReply(wire.output));
  CHECK_EQ(WiFi.beginCalls,0u);
}
TEST_CASE("partial frames expire between bytes including millis wraparound") {
  for(uint32_t start : {0u,UINT32_MAX-100u}) {
    testMillis=start;Wire wire;ImprovWiFi parser(&wire);setup(parser);WiFi={};
    Bytes partial=frame({3,0});partial.resize(10);sendRaw(parser,wire,partial);
    testMillis+=1001;wire.output.clear();send(parser,wire,{3,0});
    CHECK(validReply(wire.output));CHECK_EQ(WiFi.beginCalls,0u);
  }
}
TEST_CASE("timeout is inter-byte rather than a deadline for the entire frame") {
  testMillis=0;Wire wire;ImprovWiFi parser(&wire);setup(parser);
  for(auto byte:frame({3,0})) {testMillis+=500;sendRaw(parser,wire,{byte});}
  CHECK(validReply(wire.output));
}
TEST_CASE("explicit parser reset discards an incomplete frame") {
  Wire wire;ImprovWiFi parser(&wire);setup(parser);
  sendRaw(parser,wire,{'I','M','P','R','O'});parser.resetSerialParser();
  send(parser,wire,{3,0});CHECK(validReply(wire.output));
}
