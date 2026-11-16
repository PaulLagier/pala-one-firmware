#pragma once
// One owner for menu pagination, also used by native behavior adapters.
namespace PalaMenuGeometry {
constexpr int visibleCards = 3, firstCardTop = 28, cardStep = 32, cardHeight = 30;
static_assert(firstCardTop + (visibleCards - 1) * cardStep + cardHeight <= 122,
              "Three menu rows must fit the panel");
inline int firstVisible(int selected, int count) {
  if (count <= 0)
    return 0;
  if (selected < 0)
    selected = 0;
  if (selected >= count)
    selected = count - 1;
  const int aligned = selected / visibleCards * visibleCards;
  const int last = count > visibleCards ? count - visibleCards : 0;
  return aligned < last ? aligned : last;
}
} // namespace PalaMenuGeometry
