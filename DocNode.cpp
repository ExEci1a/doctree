#include "DocNode.h"

std::string WstringToUTF8(const std::wstring& wstr) {
  std::string out;
  for (wchar_t wc : wstr) {
    if (wc < 0x80) {
      out.push_back(static_cast<char>(wc));
    } else if (wc < 0x800) {
      out.push_back(0xC0 | ((wc >> 6) & 0x1F));
      out.push_back(0x80 | (wc & 0x3F));
    } else if (wc < 0x10000) {
      out.push_back(0xE0 | ((wc >> 12) & 0x0F));
      out.push_back(0x80 | ((wc >> 6) & 0x3F));
      out.push_back(0x80 | (wc & 0x3F));
    }
    // 对于大于 0x10000 的字符，此处省略了处理，因为它们超出了 BMP 范围。
    // 如果需要支持整个 Unicode 范围，你可以添加相应的逻辑。
  }
  return out;
}

DocNode::DocNode() {
  this->imagePaths = std::vector<std::string>();
  this->depth = 0;
}

DocNode::DocNode(std::wstring title) {
  this->title = title;
  this->imagePaths = std::vector<std::string>();
  this->depth = 0;
}

DocNode::~DocNode() {}

std::wstring DocNode::GetTitle() {
  return this->title;
}

void DocNode::SetTitle(std::wstring title) {
  this->title = title;
}

std::wstring DocNode::GetText() {
  return this->text;
}

void DocNode::SetText(std::wstring text) {
  this->text = text;
}

int DocNode::GetDepth() {
  return this->depth;
}

void DocNode::SetDepth(int depth) {
  this->depth = depth;
}

void DocNode::SetParentPtr(DocNode* parentPtr) {
  this->parentPtr = parentPtr;
}

DocNode* DocNode::GetParentPtr() {
  return this->parentPtr;
}

void DocNode::AddImagePath(std::string imagePath) {
  this->imagePaths.push_back(imagePath);
}

void DocNode::AddForSubNodes(DocNode subNode) {
  this->subNodes.push_back(subNode);
}

DocNode* DocNode::GetLastSubNodePtr() {
  return &this->subNodes.back();
}

void DocNode::AddForContentAreas(ContentArea contentArea) {
  for (ContentArea area : this->contentAreas) {
    if (area.GetPageIndex() == contentArea.GetPageIndex()) {
      if (!((area.GetRect().bottom > contentArea.GetRect().top) || (area.GetRect().top < contentArea.GetRect().bottom))) {
        area.GetRect().Union(contentArea.GetRect());
        return;
      }
    }
  }

  this->contentAreas.push_back(contentArea);
}

void DocNode::AppendText(std::wstring text) {
  this->text += text;
}

std::string DocNode::OutputTree() {
  // TODO: Exception handling for empty title and text

  std::string tempTitle = WstringToUTF8(this->title);
  std::string tempText = WstringToUTF8(this->text);

  std::string output = "{";
  // Output title and text
  output += "\"chapter\": \"" + tempTitle + "\",";
  output += "\"text\": \"" + tempText;
  //+ "\",";

  //// Output image paths
  // output += "\"imagePath\": [";
  // for (int i = 0; i < this->imagePaths.size(); i++) {
  //     output += "\"" + this->imagePaths[i] + "\"";
  //     if (i < this->imagePaths.size() - 1) {
  //         output += ",";
  //     }
  // }
  // output += "],";

  // Output sub nodes
  if (!this->subNodes.empty()) {
    output += "\"sub_chapter\": [";
    for (int i = 0; i < this->subNodes.size(); i++) {
      output += this->subNodes[i].OutputTree();
      if (i < this->subNodes.size() - 1) {
        output += ",";
      }
    }
    output += "]";
  }

  output += "}";
  return output;
}