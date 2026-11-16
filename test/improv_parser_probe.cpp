#include <ImprovWiFiLibrary.h>
#include <deque>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
TestWiFi WiFi;
static unsigned checks=0;
static void require(bool ok,const char* label) {
  ++checks;if(!ok){fprintf(stderr,"FAIL: %s\n",label);std::exit(1);}
}
struct TestStream : Stream {
  std::deque<uint8_t> input;std::vector<uint8_t> output;
  int available() override { return input.size(); }
  int read() override {if(input.empty())return -1;auto b=input.front();input.pop_front();return b;}
  size_t write(const uint8_t*p,size_t n) override {output.insert(output.end(),p,p+n);return n;}
};
static void feed(ImprovWiFi& library,TestStream& stream,std::vector<uint8_t> payload) {
  std::vector<uint8_t> frame={'I','M','P','R','O','V',1,3,(uint8_t)payload.size()};
  frame.insert(frame.end(),payload.begin(),payload.end());uint8_t checksum=0;
  for(auto byte:frame)checksum+=byte;frame.push_back(checksum);
  for(auto byte:frame)stream.input.push_back(byte);
  while(stream.available())library.handleSerial();
}
static void setup(ImprovWiFi& library) {
  library.setDeviceInfo(ImprovTypes::CF_ESP32_S3,"Pala One","parser-test","Pala One");
}
static std::string savedSsid,savedPassword;
static void connected(const char*s,const char*p){savedSsid=s;savedPassword=p;}
int main() {
  {
    TestStream stream;ImprovWiFi library(&stream);setup(library);
    for(int i=0;i<20;++i){
      stream.output.clear();feed(library,stream,{3,0});
      require(stream.output.size()>=10,"every consecutive device-info RPC receives a reply");
      require(stream.output[7]==4,"device-info result is an RPC response");
      require(std::search(stream.output.begin(),stream.output.end(),
        (const uint8_t*)"parser-test",(const uint8_t*)"parser-test"+11)!=stream.output.end(),"version is preserved");
    }
  }
  {
    TestStream stream;ImprovWiFi library(&stream);setup(library);
    const std::vector<std::vector<uint8_t>> malformed={
      {},{1},{1,0},{1,1,255},{1,3,1,'x',255},{1,5,2,'x','y',10,'z'}
    };
    for(const auto& packet:malformed){
      const auto before=WiFi.beginCalls;feed(library,stream,packet);
      require(WiFi.beginCalls==before,"malformed credential lengths never initiate a connection");
      stream.output.clear();feed(library,stream,{3,0});
      require(!stream.output.empty(),"parser recovers after malformed credentials");
    }
    for(int length:{119,128,255}) {
      auto before=WiFi.beginCalls;feed(library,stream,std::vector<uint8_t>(length,0));
      require(WiFi.beginCalls==before,"oversized packets cannot initiate a connection");
      stream.output.clear();feed(library,stream,{3,0});
      require(!stream.output.empty(),"parser recovers after an oversized frame");
    }
  }
  {
    TestStream stream;ImprovWiFi library(&stream);setup(library);library.onImprovConnected(connected);
    std::string ssid="Pala!Net",password="correct!password";
    std::vector<uint8_t> payload={1,(uint8_t)(ssid.size()+password.size()+2),(uint8_t)ssid.size()};
    payload.insert(payload.end(),ssid.begin(),ssid.end());payload.push_back(password.size());
    payload.insert(payload.end(),password.begin(),password.end());feed(library,stream,payload);
    require(savedSsid==ssid && savedPassword==password,"valid credentials including exclamation marks are unchanged");
  }
  {
    TestStream stream;ImprovWiFi library(&stream);setup(library);
    const uint8_t partial[]={'I','M','P','R','O','V',1,3,2,3,0};
    for(int count:{1,5,8,10,11}){
      const auto calls=WiFi.beginCalls;
      for(int i=0;i<count;++i)stream.input.push_back(partial[i]);
      while(stream.available())library.handleSerial();
      library.resetSerialParser();stream.output.clear();feed(library,stream,{3,0});
      require(!stream.output.empty(),"complete retry succeeds after discarding a timed-out partial frame");
      require(stream.output[7]==4,"retry returns a real device-info response");
      require(WiFi.beginCalls==calls,"discarding a partial frame never connects or changes credentials");
    }
  }
  {
    TestStream stream; ImprovWiFi library(&stream); setup(library);
    unsigned calls=0; std::string selected;
    library.onSetLanguage([&](const char* code){++calls;selected=code;return selected=="de";});
    feed(library,stream,{0x81,2,'d','e'});
    require(calls==1 && selected=="de", "language callback receives exact supported code");
    require(stream.output[7]==4 && stream.output[9]==0x81, "language success acknowledged by vendor RPC");
    for(auto packet:std::vector<std::vector<uint8_t>>{{0x81,0},{0x81,1,'d'},{0x81,3,'d','e','x'},{0x81,2,0,'e'},{0x81,2,'D','E'}}){
      stream.output.clear();feed(library,stream,packet);require(calls==1,"malformed language never reaches persistence");
    }
    stream.output.clear();feed(library,stream,{0x81,2,'z','z'});
    require(calls==2 && stream.output[7]==2,"unsupported locale rejected, no success acknowledgement");
  }
  {
    TestStream stream;ImprovWiFi library(&stream);setup(library);library.setLocalSetupOnly(true);
    unsigned calls=0;library.onSetLanguage([&](const char*){++calls;return true;});
    const auto before=WiFi.beginCalls;
    for(auto packet:std::vector<std::vector<uint8_t>>{{1,4,1,'x',1,'y'},{2,0},{4,0}}){
      stream.output.clear();feed(library,stream,packet);
      require(stream.output[7]==2 && stream.output[9]==4,"recovery setup rejects network commands before WiFi access");
    }
    feed(library,stream,{0x81,2,'d','e'});require(calls==1,"recovery permits language without filesystem");
    stream.output.clear();feed(library,stream,{3,0});require(stream.output[7]==4,"recovery permits device information");
    require(WiFi.beginCalls==before,"recovery language never starts WiFi");
  }
  printf("%u Improv parser checks passed\n",checks);
}
