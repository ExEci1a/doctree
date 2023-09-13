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

#include "DocNode.h"
#include "TextItem.h"

class PDFToDoctree {
 private:
  std::string filePath;
  std::string password = "";

  std::string outPath;

  int options = -1;

  std::vector<TextItem> textItems;
  DocNode* currentNode = nullptr;
  DocNode* rootNode = nullptr;
  std::vector<DocNode> rootNodes;
  
  int currentDepth = 0;
  int currentMajorChapterIndex = -1;

  void AnalyzeByPage(FPDF_DOCUMENT pdf_doc, int pageIndex);
  void GetTextItemsFromPage(FPDF_PAGE page, int pageIndex);

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