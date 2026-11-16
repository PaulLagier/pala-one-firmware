#pragma once
inline unsigned recordedPageTurns=0;
namespace Statistics {inline void onReaderPageTurn(){++recordedPageTurns;}}
