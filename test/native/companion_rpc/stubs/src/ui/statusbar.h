#pragma once
namespace Statusbar {
enum Mode {Full,Minimal,Hidden}; inline Mode current=Full;
inline Mode mode(){return current;} inline void setMode(Mode value){current=value;}
}
