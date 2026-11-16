#pragma once
namespace Font {
enum class Family {Helvetica,OpenDyslexic};
inline int size=10,gap=0; inline Family family=Family::Helvetica;
inline bool bionic=false,half=false;
inline int currentBodySize(){return size;}
inline int currentLineGap(){return gap;}
inline Family currentFamily(){return family;}
inline bool bionicEnabled(){return bionic;}
inline bool halfParagraphGapsEnabled(){return half;}
inline void setBodySize(int value){size=value;}
inline void setLineGap(int value){gap=value;}
inline void setFamily(Family value){family=value;}
inline void setBionic(bool value){bionic=value;}
inline void setHalfParagraphGaps(bool value){half=value;}
}
