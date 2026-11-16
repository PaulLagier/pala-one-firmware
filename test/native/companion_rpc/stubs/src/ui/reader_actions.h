#pragma once
#include <Arduino.h>
struct ButtonEvent { enum Kind {None,Short,Double,Triple,Quad,Long,VeryLong,ClickHold}; };
#include "../../../../../../Pala_One_2_1/src/ui/reader_actions.h"
