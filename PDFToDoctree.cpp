#include "PDFToDoctree.h"

#include <sys/stat.h>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#endif  // _WIN32

#include "../PSOImginfo.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "fpdfsdk/cpdfsdk_helpers.h"

#ifdef USEOCR
#include "opencv2/opencv.hpp"
#endif // USEOCR

namespace {
bool DirectoryExists(const std::string& path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    return false;
  }
  return (info.st_mode & S_IFDIR) != 0;
}

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

#ifdef USEOCR
void CaptureChapter(std::vector<DocNode>& rootNodes, std::string out_path) {
  if (!rootNodes.empty()) {
    for (auto& item : rootNodes) {
      auto& sub_nodes = item.GetSubNodes();
      if (!sub_nodes.empty()) {
        CaptureChapter(sub_nodes, out_path);
      }
      if (item.GetDepth() == 2) {
        auto rects = item.GetRects();

        auto title = WstringToUTF8(item.GetTitle());
        int count = 0;

        for (const auto& rect : rects) {
          auto page_index = rect.GetPageIndex();
          ByteString filename =
              filename.Format("%sPage%d.png", out_path.c_str(), page_index + 1);
          auto src_image = cv::imread(filename.c_str());
          auto real_rect = rect.GetRect();
          if (!src_image.empty()) {
            if (real_rect.Height() == 0 || real_rect.Width() == 0)
            {
              continue;
            }
            auto chapter_image =
                src_image(cv::Rect(0, src_image.rows - (real_rect.top * 2),
                                   src_image.cols, real_rect.Height() * 2));
            auto suffix = count > 0 ? "_" + std::to_string(count) : "";
            auto item_image_path = out_path + title + suffix + ".png";
            cv::imwrite(item_image_path, chapter_image);

            item.AddImagePath(item_image_path);
            count++;
          }
        }
      }
    }
  }
}
#endif // USEOCR
}

PDFToDoctree::PDFToDoctree() {
  FPDF_LIBRARY_CONFIG config;
  config.version = 2;
  config.m_pUserFontPaths = NULL;
  config.m_pIsolate = NULL;
  config.m_v8EmbedderSlot = 0;
  FPDF_InitLibraryWithConfig(&config);
}

PDFToDoctree::PDFToDoctree(std::string filePath,
                           std::string outPath,
                           std::string password,
                           PSO2DoctreeOpt options) {
  this->filePath = filePath;
  this->outPath = outPath;
  if (outPath.back() != '\\') {
    this->outPath += '\\';
  }
  image_out_path_ = this->outPath + "imgs\\";
  this->password = password;
  this->options = options;

  PDFToDoctree();

  // 加载PDF文件
  pdf_doc = FPDF_LoadDocument(filePath.c_str(), password.c_str());
}

PDFToDoctree::~PDFToDoctree() {
  FPDF_CloseDocument(pdf_doc);
  FPDF_DestroyLibrary();
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

  int test = FPDFPage_CountObjects(page);
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

#ifdef USEOCR
  auto c_page = CPDFPageFromFPDFPage(page);
  auto page_height = static_cast<int>(c_page->GetPageHeight());
  auto layout_results = GetLayoutAnalysisResults();
  auto need_delete_item = [&layout_results, &page_height,
                           this](CFX_FloatRect item) -> bool {
    for (auto& it : layout_results) {
      if (it.content != L"Text" && it.content != L"Title" && it.content != L"Reference" &&
          YProjectionsIntersect(page_height - (it.zone.top / 2), page_height - (it.zone.bottom / 2), item.Top(), item.Bottom())) {
        return true;
      }
    }
    
    return false;
  };
  for (auto it = boundsVector.begin(); it != boundsVector.end();) {
    if (need_delete_item(*it)) {
      it = boundsVector.erase(it);
    } else {
      ++it;
    }
  }
#endif // USEOCR

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

std::string PDFToDoctree::SavePage(FPDF_PAGE page) {
  if (!DirectoryExists(image_out_path_)) {
#if _WIN32
    _mkdir(image_out_path_.c_str());
#else
    mkdir(image_out_path_.c_str());
#endif  // _WIN32
  }

  auto m_page_bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  auto c_page = CPDFPageFromFPDFPage(page);
  auto ocr_width = static_cast<int>(c_page->GetPageWidth()) * 2;
  auto ocr_height = static_cast<int>(c_page->GetPageHeight()) * 2;
  m_page_bitmap->Create(ocr_width, ocr_height, FXDIB_Rgb32);

  auto m_pRenderDevice = std::make_unique<CFX_DefaultRenderDevice>();
  auto* pDevice = m_pRenderDevice.get();
  pDevice->Attach(m_page_bitmap, !!(0x01 & FPDF_REVERSE_BYTE_ORDER), nullptr,
                  false);

  FPDFBitmap_FillRect(FPDFBitmapFromCFXDIBitmap(m_page_bitmap.Get()), 0, 0,
                      ocr_width, ocr_height, 0xFFFFFFFF);
  FPDF_RenderPageBitmap(FPDFBitmapFromCFXDIBitmap(m_page_bitmap.Get()), page, 0,
                        0, ocr_width, ocr_height, 0, 0);
  ByteString filename = filename.Format(
      "%sPage%d.png", this->image_out_path_.c_str(), current_page_index_);
  PSOimginfo::SavePng(m_page_bitmap.Get(), filename.c_str());
  return filename.c_str();
}

#ifdef USEOCR
void PDFToDoctree::CaptureChapterImages() {
  CaptureChapter(rootNodes, this->image_out_path_);
  for (size_t i = 0; i < total_page_count_; i++) {
    ByteString filename =
        filename.Format("%sPage%d.png", image_out_path_.c_str(), i + 1);
    std::remove(filename.c_str());
  }
}
#endif // USEOCR

int PDFToDoctree::GetMajorChapterIndex(std::wstring title) {

  // 查找第一个点的位置
  auto dotPos = title.find(L'.');

  // 如果找到点，截取子字符串；否则，使用整个字符串
  std::wstring firstPart =
      (dotPos != std::wstring::npos) ? title.substr(0, dotPos) : title;

  // 将子字符串转换为 int
  int majorChapter = std::stoi(firstPart);

  return majorChapter;
}

int PDFToDoctree::GetAppendixChapterIndex(std::wstring title) {
  if (title.length() == 1) {
    return -1;
  }
  // 查找第一个点的位置
  auto dotPos = title.find(L' ');

  std::wstring firstPart;
  // 如果找到空格，截取子字符串；否则，使用整个字符串 针对（附录 A）情况
  if (dotPos != std::wstring::npos) {
    firstPart = title.substr(dotPos + 1);
  } else {
    // 如果找到点，截取子字符串； 针对（A.1.2）情况
    dotPos = title.find(L'.');
    firstPart = title.substr(0, dotPos);
  }

  // 将子字符串转换为 int
  int appendixChapter = static_cast<int>(firstPart[0]) - static_cast<int>(L'A');

  return appendixChapter;
}

void PDFToDoctree::NewRootNode(std::wsmatch match, TextItem item) {
  // 新建根节点
  this->rootNodes.push_back(DocNode());

  this->currentNode = &this->rootNodes.back();

  this->currentNode->SetTitle(match[0]);
  this->currentNode->SetText(match.suffix());
  this->currentNode->SetDepth(0);

  this->currentDepth = 0;
  this->currentNode->AddForContentAreas(
      ContentArea(item.GetPageIndex(), item.GetBounds()));
  this->rootNode = this->currentNode;
}

void PDFToDoctree::SetSubNodeForCurrentNode(DocNode newNode) {
  // 新建的章节为当前章节的子章节
  newNode.SetParentPtr(this->currentNode);
  this->currentNode->AddForSubNodes(newNode);

  this->currentNode = this->currentNode->GetLastSubNodePtr();
}

void PDFToDoctree::SetBrotherNodeForCurrentNode(DocNode newNode) {
  if (this->currentDepth == 0) {
    SetNextRootNode(newNode);
  } else {
    // 新建的章节与当前章节同级
    if (this->currentNode->GetParentPtr() == nullptr) {
      return;
    }
    newNode.SetParentPtr(this->currentNode->GetParentPtr());
    this->currentNode = this->currentNode->GetParentPtr();
    this->currentNode->AddForSubNodes(newNode);

    this->currentNode = this->currentNode->GetLastSubNodePtr();
  }
}

void PDFToDoctree::SetNextRootNode(DocNode newNode) {
  // 新建的章节为新的根章节
  newNode.SetParentPtr(nullptr);
  this->rootNodes.push_back(newNode);

  this->currentNode = &this->rootNodes.back();
  this->rootNode = this->currentNode;
}

void PDFToDoctree::SetHighLevelNode(DocNode newNode) {
  // 新建的章节为当前章节的兄弟章节，即为当前章节的父章节的子章节
  while (newNode.GetDepth() < this->currentDepth && this->currentNode->GetParentPtr() != nullptr) {
    this->currentNode = this->currentNode->GetParentPtr();
    this->currentDepth--;
  }
  newNode.SetParentPtr(this->currentNode->GetParentPtr());
  this->currentNode = this->currentNode->GetParentPtr();
  this->currentNode->AddForSubNodes(newNode);

  this->currentNode = this->currentNode->GetLastSubNodePtr();
}

bool PDFToDoctree::CheckForMainTextChapter(const std::wstring& content,
                                           std::wsmatch& match) {
  return std::regex_search(content, match, localPatternType.mainTextPattern);
}

bool PDFToDoctree::CheckForAppendixChapter(const std::wstring& content,
                                           std::wsmatch& match) {
  if (std::regex_search(content, match, localPatternType.appendixPattern)) {
    return true;
  } else if (std::regex_search(content, match, localPatternType.appendixMajorPattern)) {
    return true;
  }

  return false;
}

void PDFToDoctree::AnalyzeNodeForMainText(std::wsmatch& match, TextItem item) {
  if (this->rootNode == nullptr) {
    int majorChapter = GetMajorChapterIndex(match[0]);
    NewRootNode(match, item);
    this->currentMajorChapterIndex = majorChapter;
  } else {
    std::wstring title = match[0];
    DocNode newNode = DocNode(match[0]);

    int depth = std::count(title.begin(), title.end(), this->localPatternType.mainTextSplit);
    newNode.SetDepth(depth);

    if (depth > this->currentDepth) {
      // 新建的章节为当前章节的子章节
      SetSubNodeForCurrentNode(newNode);
    } else if (depth == 0) {
      // 新建的章节为新的根章节
      if (GetMajorChapterIndex(match[0]) ==
          (this->currentMajorChapterIndex + 1)) {
        SetNextRootNode(newNode);
        this->currentMajorChapterIndex++;
      } else {
        // TODO:可能导致一些文字被重复添加，考虑删除以下部分。
        this->currentNode->AppendText(item.GetContent());
        this->currentNode->AddForContentAreas(
            ContentArea(item.GetPageIndex(), item.GetBounds()));
      }
    } else if (depth == this->currentDepth) {
      // 新建的章节与当前章节同级
      SetBrotherNodeForCurrentNode(newNode);
    } else {
      SetHighLevelNode(newNode);
    }
    
    this->currentNode->AppendText(match.suffix());
    this->currentNode->AddForContentAreas(
        ContentArea(item.GetPageIndex(), item.GetBounds()));
    this->currentDepth = depth;
  }
}

void PDFToDoctree::AnalyzeNodeForAppendix(std::wsmatch& match, TextItem item) {
  this->is_in_appendix = true;
  if (this->rootNode == nullptr) {
    int appendixIndex = GetAppendixChapterIndex(match[0]);
    NewRootNode(match, item);
    this->currentAppendixChapterIndex = appendixIndex;
  } else {
    std::wstring title = match[0];
    DocNode newNode = DocNode(match[0]);

    int depth = std::count(title.begin(), title.end(), this->localPatternType.appendixSplit);
    newNode.SetDepth(depth);

    if (depth > this->currentDepth) {
      // 新建的章节为当前章节的子章节
      SetSubNodeForCurrentNode(newNode);
    } else if (depth == 0) {
      // 新建的章节为新的根章节
      int tempAppendixIndex = GetAppendixChapterIndex(match[0]);
      if ( tempAppendixIndex > this->currentAppendixChapterIndex) {
        SetNextRootNode(newNode);
        this->currentAppendixChapterIndex = tempAppendixIndex;
      } else {
        // TODO:可能导致一些文字被重复添加，考虑删除以下部分。
        this->currentNode->AppendText(item.GetContent());
        this->currentNode->AddForContentAreas(
            ContentArea(item.GetPageIndex(), item.GetBounds()));
      }
    } else if (depth == this->currentDepth) {
      // 新建的章节与当前章节同级
      SetBrotherNodeForCurrentNode(newNode);
    } else {
      SetHighLevelNode(newNode);
    }
    
    this->currentNode->AppendText(match.suffix());
    this->currentNode->AddForContentAreas(
        ContentArea(item.GetPageIndex(), item.GetBounds()));
    this->currentDepth = depth;
  }
}

void PDFToDoctree::RevertDoctree(std::vector<TextItem>& textItems) {
  int index = 0;
  for (TextItem item : textItems) {
    std::wsmatch match;
    std::wstring content = item.GetContent();
    if (CheckForMainTextChapter(content, match)) {
      if (is_in_appendix) {
        if (this->currentNode == nullptr)
          continue;
      
        this->currentNode->AppendText(item.GetContent());
        this->currentNode->AddForContentAreas(
            ContentArea(item.GetPageIndex(), item.GetBounds()));
        
        continue;
      }
      
      AnalyzeNodeForMainText(match, item);
    } else if (std::regex_search(content, match, localPatternType.appendixMajorPattern)) {
      if (std::regex_search(content, match, localPatternType.appendixMajorPattern) && index != 0) {
        this->currentNode->AppendText(item.GetContent());
        this->currentNode->AddForContentAreas(
            ContentArea(item.GetPageIndex(), item.GetBounds()));
        
        continue;
      }
      AnalyzeNodeForAppendix(match, item);
    } else if (std::regex_search(content, match, localPatternType.appendixPattern)) {
      // 如果开头就有附录就无了
      AnalyzeNodeForAppendix(match, item);
    } else {
      if (this->currentNode == nullptr)
        continue;
      
      this->currentNode->AppendText(item.GetContent());
      this->currentNode->AddForContentAreas(
          ContentArea(item.GetPageIndex(), item.GetBounds()));
    }
    index++;
  }
}

bool PDFToDoctree::AnalyzeByPage(FPDF_DOCUMENT pdf_doc, int pdf_page_index, int current_page_index) {
  // 获取页面
  textItems.clear();
  FPDF_PAGE page = FPDF_LoadPage(pdf_doc, pdf_page_index);
  if (!page) {
    std::cerr << "Failed to load the page." << std::endl;
    FPDF_CloseDocument(pdf_doc);
    return false;
  }

  auto image_path = SavePage(page);
#if USEOCR
  IdentiImg(image_path.c_str());
  layout_analysis_results_ = GetDetResult();
  ClearOcrResult();
#endif // USE_OCR

  // 从页面中提取行文本
  GetTextItemsFromPage(page, current_page_index - 1);
  RevertDoctree(textItems);

#if USEOCR
  if (!start_page_ && !rootNodes.empty())
  {
    auto c_page = CPDFPageFromFPDFPage(page);
    auto page_height = static_cast<int>(c_page->GetPageHeight());
    auto check_page_start = [this, &page_height]() -> bool {
      for (const auto& it : layout_analysis_results_)
      {
        if (it.content == L"Title" && rootNodes.front().GetTitle() == L"1")
        {
          return YProjectionsIntersect(page_height - (it.zone.top / 2), page_height - (it.zone.bottom / 2), 
                                       rootNodes.front().GetRects().front().GetRect().Top(), rootNodes.front().GetRects().front().GetRect().Bottom());
        }
      }
    };
    start_page_ = check_page_start();
  }
#endif // USE_OCR
  FPDF_ClosePage(page);

#if USEOCR
  if (!start_page_)
  {
    remove(image_path.c_str());
    ClearDoctree();
  }

  return start_page_;
#endif
 return true;
}

DocNode PDFToDoctree::Analyze() {
  if (!pdf_doc) {
    std::cerr << "Failed to load the document." << std::endl;
  }

  auto page_count = FPDF_GetPageCount(pdf_doc);
  for (auto i{0}; i < page_count; i++) {
    current_page_index_ = total_page_count_ + 1;
    if (AnalyzeByPage(pdf_doc, i, current_page_index_))
    {
      total_page_count_++;
    }
  }

#if USEOCR
  CaptureChapterImages();
#endif // USEOCR

  return DocNode();
}

int PDFToDoctree::GetDocPageCount() {
  return FPDF_GetPageCount(pdf_doc);
}

void PDFToDoctree::OutputDoctreeJson() {
  std::string fileName =
      this->filePath.substr(this->filePath.find_last_of('\\') + 1);

  std::string outPutFileName =
      this->outPath + fileName.substr(0, fileName.find_last_of('.')) + ".json";
  
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