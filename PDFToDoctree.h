#ifndef PDF_TO_DOCTREE
#define PDF_TO_DOCTREE

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <regex>

#include "public/fpdfview.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_text.h"
#include "core/fxcrt/fx_coordinates.h"

#include "../OCR/psoOCR.h"
#include "DocNode.h"
#include "TextItem.h"

class PDFToDoctree {
 private:
  std::string filePath;
  std::string password = "";

  std::string outPath;
  std::string image_out_path_;

  int options = -1;

  std::vector<TextItem> textItems;
  DocNode* currentNode = nullptr;
  DocNode* rootNode = nullptr;
  std::vector<DocNode> rootNodes;
#ifdef USEOCR
  std::vector<OcrResult> det_results_;
#endif // USEOCR
  
  int currentDepth = 0;
  int currentMajorChapterIndex = -1;
  int current_page_index_ = 0;
  int total_page_count_ = 0;

  void AnalyzeByPage(FPDF_DOCUMENT pdf_doc, int pageIndex);
  void GetTextItemsFromPage(FPDF_PAGE page, int pageIndex);
  std::string SavePage(FPDF_PAGE page);
  void CaptureChapterImages();

  int GetMajorChapterIndex(std::wstring title);
  void RevertDoctree(std::vector<TextItem>& textItems);
  bool YProjectionsIntersect(float top1,
                             float bottom1,
                             float top2,
                             float bottom2);



 public:
  // input path, output path, password, options, error code;
  PDFToDoctree() = default;
  PDFToDoctree(std::string filePath, std::string outPath, std::string password, int options);
  ~PDFToDoctree();
  DocNode Analyze();

  void OutputDoctreeJson();
};

#endif  // PDF_TO_DOCTREE