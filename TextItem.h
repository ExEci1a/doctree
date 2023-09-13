#ifndef TEXTITEM
#define TEXTITEM

#include <string>

#include "core/fxcrt/fx_coordinates.h"

class TextItem {
private:
  int pageIndex;
  CFX_FloatRect bounds;
  std::wstring content;

public:
  TextItem() = default;
  TextItem(int pageIndex, CFX_FloatRect bounds, std::wstring content);
  ~TextItem();
  void UnionItem(TextItem item);
  CFX_FloatRect GetBounds();
  std::wstring GetContent();
  int GetPageIndex();

  bool operator>(const TextItem &item) const {
    return this->bounds.top > item.bounds.top;
  }

  bool operator<(const TextItem &item) const {
    return this->bounds.top < item.bounds.top;
  }
};  

#endif // TEXTITEM