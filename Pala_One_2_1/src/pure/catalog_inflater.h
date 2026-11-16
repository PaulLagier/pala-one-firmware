#pragma once
#include <stddef.h>
#include <stdint.h>
#include <memory>
// Bounded zlib decoder for trusted generated dictionaries. Book decoding stays
// with the existing miniz reader. Large Huffman tables live on the heap.
class CatalogInflater {
  struct Workspace;
  std::unique_ptr<Workspace> workspace_;
public:
  CatalogInflater();
  ~CatalogInflater();
  bool ready()const{return bool(workspace_);}
  static size_t workspaceBytes();
  bool inflate(const uint8_t* input,size_t inputSize,uint8_t* output,size_t outputSize);
};
