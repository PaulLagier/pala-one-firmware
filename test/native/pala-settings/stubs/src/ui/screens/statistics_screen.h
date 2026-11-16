#pragma once
#include "runtime.h"
class StatisticsScreen:public Screen{public:void draw()override{}void onButton(const ButtonEvent&)override{}};inline StatisticsScreen g_statsScreen;
