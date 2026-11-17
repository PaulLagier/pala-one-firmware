#pragma once
namespace Font {
enum class Family { Helvetica, OpenDyslexic };
inline int size = 12, gap = 1, writes = 0;
inline Family family = Family::Helvetica;
inline bool bionic = true, halfGaps = true;
void invalidateLayoutCache();
inline int currentBodySize() { return size; }
inline int currentLineGap() { return gap; }
inline Family currentFamily() { return family; }
inline bool bionicEnabled() { return bionic; }
inline bool halfParagraphGapsEnabled() { return halfGaps; }
inline void setBodySize(int v) {
  size = v;
  ++writes;
}
inline void setLineGap(int v) {
  gap = v;
  ++writes;
}
inline void setFamily(Family v) {
  family = v;
  ++writes;
}
inline void setBionic(bool v) {
  bionic = v;
  ++writes;
}
inline void setHalfParagraphGaps(bool v) {
  halfGaps = v;
  ++writes;
}
} // namespace Font
