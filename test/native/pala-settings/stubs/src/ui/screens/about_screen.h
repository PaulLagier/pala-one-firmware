#pragma once
#include "runtime.h"
class AboutScreen:public Screen{public:void draw()override{}void onButton(const ButtonEvent&)override{}};inline AboutScreen g_aboutScreen;
