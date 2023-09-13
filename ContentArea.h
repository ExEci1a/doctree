#ifndef CONTENTAREA
#define CONTENTAREA

#include "core/fxcrt/fx_coordinates.h"

class ContentArea {
private:
  int pageIndex;
  CFX_FloatRect rect;

public:
  ContentArea() = default;
  ContentArea(int pageIndex, CFX_FloatRect rect) : pageIndex(pageIndex), rect(rect) {}
  ~ContentArea() = default;

  int GetPageIndex() const { return this->pageIndex; };
  CFX_FloatRect GetRect() const { return this->rect; };

  void UnionArea(ContentArea area);
};

#endif // CONTENTAREA