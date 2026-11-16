#pragma once
#include "Arduino.h"
struct ButtonEvent{enum Kind{None,Short,Double,Triple,Quad,Long,VeryLong,ClickHold};Kind kind=None;bool any()const{return kind!=None;}};
class Screen{public:virtual~Screen()=default;virtual void onEnter(){};virtual void onButton(const ButtonEvent&)=0;virtual void draw()=0;Screen*nextScreen=nullptr;};
