#pragma once
#include "src/ui/screen.h"
struct LibraryScreen : Screen { void onButton(const ButtonEvent&)override{} void draw()override{} };
extern LibraryScreen g_libraryScreen;
