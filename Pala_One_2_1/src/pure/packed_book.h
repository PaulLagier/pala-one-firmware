#pragma once
#include "stream.h"
#include <stdint.h>
#include <stddef.h>
#include <memory>
#include <new>
#include <algorithm>

namespace PalaPacked {
constexpr uint32_t BlockSize = 4096, MaximumSize = 2 * 1024 * 1024;
inline uint32_t read32(const uint8_t* b) { return uint32_t(b[0]) | uint32_t(b[1])<<8 | uint32_t(b[2])<<16 | uint32_t(b[3])<<24; }
inline uint32_t crc32(const uint8_t* data, size_t length, uint32_t previous=0) {
  uint32_t crc=~previous;
  for(size_t i=0;i<length;++i){crc^=data[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320U:0U);}
  return ~crc;
}
// Metadata inspection is allocation-free. Read the offset table sequentially
// instead of issuing one filesystem seek/read for every 4-byte offset.
struct Header { uint32_t size=0, blocks=0; };
template<class Source> bool inspect(Source& source,Header& result){
  result=Header{};uint8_t h[16];
  if(!source.seek(0)||source.read(h,16)!=16||h[0]!='P'||h[1]!='B'||h[2]!='T'||h[3]!='1')return false;
  const uint32_t size=read32(h+4),count=read32(h+12);
  if(!size||size>MaximumSize||read32(h+8)!=BlockSize||count!=(size+BlockSize-1)/BlockSize)return false;
  const size_t physical=source.size();const uint32_t headerSize=16+4*(count+1);
  uint32_t previous=headerSize,index=0;uint8_t table[256];
  while(index<=count){
    const uint32_t entries=(std::min)(uint32_t(sizeof(table)/4),count+1-index);
    if(source.read(table,entries*4)!=entries*4)return false;
    for(uint32_t i=0;i<entries;++i,++index){
      const uint32_t next=read32(table+4*i);
      if(index==0){if(next!=headerSize)return false;}
      else if(next<=previous+8||next-previous>BlockSize+8||next>physical)return false;
      previous=next;
    }
  }
  if(previous!=physical)return false;
  result.size=size;result.blocks=count;return true;
}
// Source has the same read/seek/size interface as Arduino File. Inflate is a
// bounded callback, enabling the very same reader to run under host sanitizers.
template<class Source> class Stream : public IReadStream {
public:
  using Inflate = bool (*)(const uint8_t*,size_t,uint8_t*,size_t);
  Stream(Source& source, Inflate inflate):source_(source),inflate_(inflate){open();}
  bool valid() const {return valid_;}
  int read() override {
    if(!valid_ || pos_>=size_)return -1;
    const uint32_t block=pos_/BlockSize;
    if(cached_!=block && !load(block)){valid_=false;return -1;}
    return output_[pos_++%BlockSize];
  }
  bool seek(uint32_t pos) override {if(!valid_ || pos>size_)return false;pos_=pos;return true;}
  uint32_t position() override{return pos_;}
  size_t size() override{return valid_?size_:0;}
  bool available() override{return valid_ && pos_<size_;}
private:
  Source& source_;Inflate inflate_;
  std::unique_ptr<uint8_t[]> output_,input_;
  uint32_t size_=0,count_=0,pos_=0,cached_=0xffffffffU;
  bool valid_=false;
  bool exact(uint8_t* data,size_t length){return source_.read(data,length)==length;}
  bool offset(uint32_t block,uint32_t& value){uint8_t b[4];if(!source_.seek(16+4*block)||!exact(b,4))return false;value=read32(b);return true;}
  void open(){
    Header header;if(!inspect(source_,header))return;
    size_=header.size;count_=header.blocks;
    output_.reset(new(std::nothrow) uint8_t[BlockSize]);input_.reset(new(std::nothrow) uint8_t[BlockSize]);
    valid_=bool(output_)&&bool(input_);
  }
  bool load(uint32_t block){
    uint32_t start,end;uint8_t header[8];
    if(!offset(block,start)||!offset(block+1,end)||!source_.seek(start)||!exact(header,8))return false;
    const uint32_t stored=read32(header),length=stored&0x7fffffffU;
    const size_t expected=(std::min)(BlockSize,size_-block*BlockSize);
    if(!length||length>BlockSize||end-start!=length+8||!exact(input_.get(),length))return false;
    if(stored&0x80000000U){if(length!=expected)return false;std::copy(input_.get(),input_.get()+length,output_.get());}
    else if(!inflate_(input_.get(),length,output_.get(),expected))return false;
    if(crc32(output_.get(),expected)!=read32(header+4))return false;
    cached_=block;return true;
  }
};
}
