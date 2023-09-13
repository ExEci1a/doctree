#include "TextItem.h"

TextItem::TextItem(int pageIndex, CFX_FloatRect bounds, std::wstring content) {
  this->bounds = bounds;
  this->content = content;
}

TextItem::~TextItem() {

}

void TextItem::UnionItem(TextItem item) {
  this->bounds.Union(item.bounds);
}

CFX_FloatRect TextItem::GetBounds() {
  return this->bounds;
}

std::wstring TextItem::GetContent() {
  return this->content;
}

int TextItem::GetPageIndex() {
  return this->pageIndex;
}