#pragma once
inline bool navigatedToLibrary=false;
inline void navigateToLibraryRoot(){navigatedToLibrary=true;}

#include "src/ui/screen.h"
class LibraryScreen:public Screen{public:void onButton(const ButtonEvent&)override{}void draw()override{}};inline LibraryScreen g_libraryScreen;
