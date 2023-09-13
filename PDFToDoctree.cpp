#include "PDFToDoctree.h"

PDFToDoctree::PDFToDoctree(std::string filePath, std::string outPath, std::string password, int options) {
  this->filePath = filePath;
  this->outPath = outPath;
  this->password = password;
  this->options = options;
}

PDFToDoctree::~PDFToDoctree() {
  /*if (this->currentNode) {
    delete this->currentNode;
    this->currentNode = nullptr;
  }
  if (this->rootNode) {
    delete this->rootNode;
    this->rootNode = nullptr;
  }*/
}

bool PDFToDoctree::YProjectionsIntersect(float top1,
                                         float bottom1,
                                         float top2,
                                         float bottom2) {
  return !((bottom1 > top2) || (top1 < bottom2));
}

void PDFToDoctree::GetTextItemsFromPage(FPDF_PAGE page, int pageIndex) {
  std::vector<CFX_FloatRect> boundsVector;
  std::set<int> bottomSet;

  // 遍历页面内容
  for (int i = 0; i < FPDFPage_CountObjects(page); i++) {
    FPDF_PAGEOBJECT page_object = FPDFPage_GetObject(page, i);
    if (FPDFPageObj_GetType(page_object) == FPDF_PAGEOBJ_TEXT) {
      // 获取文本矩形框
      float left, bottom, right, top;
      FPDFPageObj_GetBounds(page_object, &left, &bottom, &right, &top);
      CFX_FloatRect rect(left, bottom, right, top);
      bool isUnion = false;

      for (CFX_FloatRect& bounds : boundsVector) {
        // 如果两个矩形的 Y 投影有交集，就合并它们
        if (YProjectionsIntersect(bounds.Top(), bounds.Bottom(), top, bottom)) {
          bounds.Union(rect);
          isUnion = true;
          break;
        }
      }

      if (isUnion)
        continue;

      boundsVector.push_back(rect);
    }
  }

  FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page);
  int temp = 0;

  std::sort(boundsVector.begin(), boundsVector.end(),
            [](CFX_FloatRect a, CFX_FloatRect b) { return a.top > b.top; });

  for (CFX_FloatRect rect : boundsVector) {
    temp++;

    int buffer_len = FPDFText_GetBoundedText(text_page, rect.left, rect.bottom,
                                             rect.right, rect.top, NULL, 0);
    unsigned short* buffer = new unsigned short[buffer_len];
    FPDFText_GetBoundedText(text_page, rect.left, rect.bottom, rect.right,
                            rect.top, buffer, buffer_len);

    // 转换 unsigned short* 到 wchar_t*
    wchar_t* wbuffer = new wchar_t[buffer_len];
    for (int i = 0; i < buffer_len; ++i) {
      wbuffer[i] = static_cast<wchar_t>(buffer[i]);
    }

    std::wstring extracted_text(wbuffer, buffer_len);

    this->textItems.push_back(TextItem(pageIndex, rect, extracted_text));

    delete[] buffer;
    delete[] wbuffer;
  }
}

int PDFToDoctree::GetMajorChapterIndex(std::wstring title) {

  // 查找第一个点的位置
  auto dotPos = title.find(L'.');

  // 如果找到点，截取子字符串；否则，使用整个字符串
  std::wstring firstPart = (dotPos != std::wstring::npos) ? title.substr(0, dotPos) : title;

  // 将子字符串转换为 int
  int majorChapter = std::stoi(firstPart);

  return majorChapter;
}

void PDFToDoctree::RevertDoctree(std::vector<TextItem>& textItems) {
  std::wregex pattern(L"^(\\d{1,2})(\\.\\d{1,2})*(?=([\\s]))");
  for (TextItem item : textItems) {
    std::wsmatch match;
    std::wstring content = item.GetContent();
    if (std::regex_search(content, match, pattern)) {
      if (this->rootNode == nullptr) {
        this->rootNodes.push_back(DocNode());

        this->currentNode = &this->rootNodes.back();

        int majorChapter = GetMajorChapterIndex(match[0]);
        this->currentMajorChapterIndex = majorChapter;

        this->currentNode->SetTitle(match[0]);
        this->currentNode->SetText(match.suffix());
        this->currentNode->SetDepth(0);

        this->currentDepth = 0;
        this->rootNode = this->currentNode;
      } else {
        std::wstring title = match[0];
        DocNode newNode = DocNode(match[0]);

        int depth = std::count(title.begin(), title.end(), L'.');

        if (depth > this->currentDepth) {
          // 新建的章节为当前章节的子章节
          newNode.SetParentPtr(this->currentNode);
          this->currentNode->AddForSubNodes(newNode);
          
          this->currentNode = this->currentNode->GetLastSubNodePtr();
        } else if (depth == this->currentDepth) {
          // 新建的章节与当前章节同级
          newNode.SetParentPtr(this->currentNode->GetParentPtr());
          this->currentNode = this->currentNode->GetParentPtr();
          this->currentNode->AddForSubNodes(newNode);
          
          this->currentNode = this->currentNode->GetLastSubNodePtr();
        } else if (depth == 0) {
          // 新建的章节为新的根章节
          if (GetMajorChapterIndex(match[0]) == (this->currentMajorChapterIndex + 1)) {
            this->currentMajorChapterIndex++;

            newNode.SetParentPtr(nullptr);
            this->rootNodes.push_back(newNode);
            
            this->currentNode = &this->rootNodes.back();
            this->rootNode = this->currentNode;
          } else {
            this->currentNode->AppendText(item.GetContent());
            this->currentNode->AddForContentAreas(ContentArea(item.GetPageIndex(), item.GetBounds()));
          }
        } else {
          // 新建的章节为当前章节的兄弟章节，即为当前章节的父章节的子章节
          while (depth < this->currentDepth) {
            this->currentNode = this->currentNode->GetParentPtr();
            this->currentDepth--;
          }
          newNode.SetParentPtr(this->currentNode->GetParentPtr());
          this->currentNode = this->currentNode->GetParentPtr();
          this->currentNode->AddForSubNodes(newNode);
          
          this->currentNode = this->currentNode->GetLastSubNodePtr();
        }

        this->currentNode->AppendText(match.suffix());
        this->currentNode->AddForContentAreas(ContentArea(item.GetPageIndex(), item.GetBounds()));
        this->currentDepth = depth;
      }
    } else {
      this->currentNode->AppendText(item.GetContent());
      this->currentNode->AddForContentAreas(ContentArea(item.GetPageIndex(), item.GetBounds()));
    }
  }
}

void PDFToDoctree::AnalyzeByPage(FPDF_DOCUMENT pdf_doc, int pageIndex) {
  // 获取页面
  textItems.clear();
  FPDF_PAGE page = FPDF_LoadPage(pdf_doc, pageIndex);
  if (!page) {
    std::cerr << "Failed to load the page." << std::endl;
    FPDF_CloseDocument(pdf_doc);
  }
  // 从页面中提取行文本
  GetTextItemsFromPage(page, pageIndex);
  RevertDoctree(textItems);

  FPDF_ClosePage(page);
}

DocNode PDFToDoctree::Analyze() {
  FPDF_LIBRARY_CONFIG config;
  config.version = 2;
  config.m_pUserFontPaths = NULL;
  config.m_pIsolate = NULL;
  config.m_v8EmbedderSlot = 0;
  FPDF_InitLibraryWithConfig(&config);

  // 加载PDF文件
  FPDF_DOCUMENT pdf_doc = FPDF_LoadDocument(filePath.c_str(), password.c_str());
  if (!pdf_doc) {
    std::cerr << "Failed to load the document." << std::endl;
  }

  for (int i = 0; i < FPDF_GetPageCount(pdf_doc); i++) {
    AnalyzeByPage(pdf_doc, i);
  }

  FPDF_CloseDocument(pdf_doc);
  FPDF_DestroyLibrary();

  return DocNode();
}

void PDFToDoctree::OutputDoctreeJson() {
  std::string fileName = this->filePath.substr(this->filePath.find_last_of('\\') + 1);

  std::string outPutFileName = this->outPath + fileName.substr(0, fileName.find_last_of('.')) + ".json";
  
  std::ofstream out(outPutFileName);
  if (!out.is_open()) {
    std::cerr << "Failed to open the output file." << std::endl;
    return;
  }

  out << "{\"" + fileName + "\": [";
  for (DocNode node : this->rootNodes) {
    out << node.OutputTree();
    if (&node != &this->rootNodes.back()) {
      out << ",";
    }
  }
  out << "]}";

  out.close();
}