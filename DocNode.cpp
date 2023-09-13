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
  for (ContentArea& area : this->contentAreas) {
    if (area.GetPageIndex() == contentArea.GetPageIndex()) {
      area.UnionArea(contentArea);
      return;
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

  tempTitle.erase(std::remove(tempTitle.begin(), tempTitle.end(), '\n'), tempTitle.end());
  tempText.erase(std::remove(tempText.begin(), tempText.end(), '\n'), tempText.end());
  // size_t pos = 0;
  // while ((pos = tempTitle.find('\n', pos)) != std::string::npos) {
  //     tempTitle.replace(pos, 1, "\\n");
  //     pos += 2;  // Move past the newly inserted characters
  // }

  // size_t pos2 = 0;
  // while ((pos2 = tempText.find('\n', pos2)) != std::string::npos) {
  //     tempText.replace(pos2, 1, "\\n");
  //     pos2 += 2;  // Move past the newly inserted characters
  // }

  output += "\"chapter\": \"" + tempTitle + "\",";
  output += "\"text\": \"" + tempText + "\"";

  if (!this->contentAreas.empty()) {
    output += ",\"contentArea\": [";
    for (int i = 0; i < this->contentAreas.size(); i++) {
      output += "{\"pageIndex\": " + std::to_string(this->contentAreas[i].GetPageIndex()) + ",";
      output += "\"rect\":";
      output += "[" + std::to_string(this->contentAreas[i].GetRect().left) + ",";
      output += std::to_string(this->contentAreas[i].GetRect().bottom) + ",";
      output += std::to_string(this->contentAreas[i].GetRect().right) + ",";
      output += std::to_string(this->contentAreas[i].GetRect().top) + "]}";
      if (i < this->contentAreas.size() - 1) {
        output += ",";
      }
    }
    output += "]";
  }

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
    output += ",\"sub_chapter\": [";
    for (int i = 0; i < this->subNodes.size(); i++) {
      output += this->subNodes[i].OutputTree();
      if (i < this->subNodes.size() - 1) {
        output += ",";
      }
    }
    output += "]";
  }

  output += "}";

  // rapidjson::Document document;
	// document.SetObject();
	// rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	// rapidjson::Value object1(rapidjson::kObjectType);

  // document.AddMember("chapter", tempTitle, allocator);
  // document.AddMember("text", tempText, allocator);

  // if (!this->subNodes.empty()) {
  //   rapidjson::Value subChapter(rapidjson::kArrayType);
  //   for (int i = 0; i < this->subNodes.size(); i++) {
  //     rapidjson::Value subChapterObject(rapidjson::kObjectType);
  //     rapidjson::Document tempDoc;
  //     tempDoc.Parse(this->subNodes[i].OutputTree().c_str());
  //     subChapterObject.CopyFrom(tempDoc, allocator);

  //     // subChapterObject.Parse(this->subNodes[i].OutputTree().c_str());
  //     subChapter.PushBack(subChapterObject, allocator);
  //   }

  //   document.AddMember("sub_chapter", subChapter, allocator);
  // }
  
  // rapidjson::StringBuffer buffer;
  // rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  // document.Accept(writer);

  // std::string output = buffer.GetString();

  return output;
}