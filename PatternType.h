#ifndef PATTERNTYPE
#define PATTERNTYPE

#include <regex>
#include <string>

class PatternType {
public:
  std::wregex pattern;
  std::wstring split;

  PatternType(std::wregex pattern, std::wstring split) : pattern(pattern), split(split) {};
};

#endif // PATTERNTYPE