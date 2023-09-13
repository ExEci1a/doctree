#include  "ContentArea.h"

void ContentArea::UnionArea(ContentArea area) {
  this->rect.Union(area.GetRect());
}