#pragma once
#include "Arduino.h"
#include <map>
#include <vector>
#include <memory>
#include <limits>
struct MemoryNode {std::vector<uint8_t> bytes;};
inline size_t fileReads=0;
inline size_t writeBudget=(std::numeric_limits<size_t>::max)();
class File {
 struct Handle {std::shared_ptr<MemoryNode> node;std::string path;size_t position=0;};
 std::shared_ptr<Handle> handle;
public:
 File()=default;
 File(std::shared_ptr<MemoryNode> node,const std::string& path):handle(std::make_shared<Handle>(Handle{node,path,0})){}
 explicit operator bool()const{return bool(handle);}
 bool isDirectory()const{return false;}
 const char* path()const{return handle?handle->path.c_str():"";}
 size_t size()const{return handle?handle->node->bytes.size():0;}
 uint32_t position()const{return handle?uint32_t(handle->position):0;}
 bool seek(uint32_t position){if(!handle||position>size())return false;handle->position=position;return true;}
 bool available()const{return handle&&handle->position<size();}
 size_t read(uint8_t* output,size_t count){++fileReads;if(!handle)return 0;count=(std::min)(count,size()-handle->position);if(count)memcpy(output,handle->node->bytes.data()+handle->position,count);handle->position+=count;return count;}
 int read(){uint8_t byte;return read(&byte,1)?byte:-1;}
 size_t write(const uint8_t* input,size_t count){if(!handle)return 0;count=(std::min)(count,writeBudget);writeBudget-=count;auto& bytes=handle->node->bytes;bytes.resize((std::max)(bytes.size(),handle->position+count));if(count)memcpy(bytes.data()+handle->position,input,count);handle->position+=count;return count;}
 size_t print(const String& text){return write(reinterpret_cast<const uint8_t*>(text.c_str()),text.length());}
 void close(){handle.reset();}
 void flush(){}
};
class MemoryFS {
public:
 std::map<std::string,std::shared_ptr<MemoryNode>> nodes;
 bool failRename=false;
 File open(const String& path,const char* mode="r"){
  const std::string key=path.c_str();if(mode[0]=='w')nodes[key]=std::make_shared<MemoryNode>();
  auto i=nodes.find(key);return i==nodes.end()?File():File(i->second,key);
 }
 bool exists(const String& path){return nodes.count(path.c_str());}
 bool remove(const String& path){return nodes.erase(path.c_str());}
 bool rename(const String& from,const String& to){if(failRename)return false;auto i=nodes.find(from.c_str());if(i==nodes.end())return false;nodes[to.c_str()]=i->second;nodes.erase(i);return true;}
 void put(const std::string& path,const std::vector<uint8_t>& bytes){nodes[path]=std::make_shared<MemoryNode>(MemoryNode{bytes});}
};
