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

std::wstring DocNode::GetTitle() const {
  return this->title;
}

void DocNode::SetTitle(std::wstring title) {
  this->title = title;
}

std::wstring DocNode::GetText() const {
  return this->text;
}

void DocNode::SetText(std::wstring text) {
  this->text = text;
}

int DocNode::GetDepth() const {
  return this->depth;
}

void DocNode::SetDepth(int depth) {
  this->depth = depth;
}

void DocNode::SetParentPtr(DocNode* parentPtr) {
  this->parentPtr = parentPtr;
}

DocNode* DocNode::GetParentPtr() const {
  return this->parentPtr;
}

void DocNode::AddImagePath(std::string imagePath) {
  this->imagePaths.push_back(imagePath);
}

std::vector<ContentArea> DocNode::GetRects() const {
  return contentAreas;
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
  std::string tempTitle = WstringToUTF8(this->title);
  std::string tempText = WstringToUTF8(this->text);

  rapidjson::Value titleValue(rapidjson::kStringType);
  titleValue.SetString(tempTitle.c_str(), tempTitle.size());

  rapidjson::Value textValue(rapidjson::kStringType);
  textValue.SetString(tempText.c_str(), tempText.size());

  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  rapidjson::Value object1(rapidjson::kObjectType);

  document.AddMember("chapter", titleValue, allocator);
  document.AddMember("text", textValue, allocator);

  if (!this->imagePaths.empty()) {
    rapidjson::Value imagePaths(rapidjson::kArrayType);
    for (int i = 0; i < this->imagePaths.size(); i++) {
      rapidjson::Value imagePath(rapidjson::kStringType);
      imagePath.SetString(this->imagePaths[i].c_str(), this->imagePaths[i].size());
      imagePaths.PushBack(imagePath, allocator);
    }

    document.AddMember("image_path", imagePaths, allocator);
  }

  if (!this->subNodes.empty()) {
    rapidjson::Value subChapter(rapidjson::kArrayType);
    for (int i = 0; i < this->subNodes.size(); i++) {
      rapidjson::Value subChapterObject(rapidjson::kObjectType);
      rapidjson::Document tempDoc;
      tempDoc.Parse(this->subNodes[i].OutputTree().c_str());
      subChapterObject.CopyFrom(tempDoc, allocator);
      subChapter.PushBack(subChapterObject, allocator);
    }

    document.AddMember("sub_chapter", subChapter, allocator);
  }
  
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  document.Accept(writer);

  std::string output = buffer.GetString();

  return output;
}