#pragma once
#include "src/ui/screen.h"
class BookmarkPreviewScreen:public Screen{public:void onButton(const ButtonEvent&)override{}void draw()override{}};inline BookmarkPreviewScreen g_bmPreviewScreen;
