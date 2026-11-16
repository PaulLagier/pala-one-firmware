#pragma once
#include "runtime.h"
class UpdateScreen:public Screen{public:void draw()override{}void onButton(const ButtonEvent&)override{}};inline UpdateScreen g_updateScreen;
