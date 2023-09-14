#ifndef PATTERNTYPE
#define PATTERNTYPE

#include <regex>
#include <string>

class PatternType {
public:
  std::wregex mainTextPattern;
  wchar_t mainTextSplit;
  std::wregex mainTextMajorPattern;

  std::wregex appendixPattern;
  wchar_t appendixSplit;
  std::wregex appendixMajorPattern;

  PatternType(std::wregex mainTextPattern,
              wchar_t mainTextSplit,
              std::wregex mainTextMajorPattern,
              std::wregex appendixPattern,
              wchar_t appendixSplit,
              std::wregex appendixMajorPattern) {
    this->mainTextPattern = mainTextPattern;
    this->mainTextSplit = mainTextSplit;
    this->mainTextMajorPattern = mainTextMajorPattern;

    this->appendixPattern = appendixPattern;
    this->appendixSplit = appendixSplit;
    this->appendixMajorPattern = appendixMajorPattern;
  }
};

#endif // PATTERNTYPE