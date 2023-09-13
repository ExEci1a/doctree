#ifndef DOCNODE
#define DOCNODE

#include <string>
#include <vector>

#include "core/fxcrt/fx_coordinates.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "ContentArea.h"

class DocNode {
private:
  std::wstring title;
  std::wstring text;
  std::vector<std::string> imagePaths;
  std::vector<DocNode> subNodes;
  std::vector<ContentArea> contentAreas;

  DocNode* parentPtr;

  int depth;

public:
  DocNode();
  DocNode(std::wstring title);
  ~DocNode();

  std::wstring GetTitle() const;
  void SetTitle(std::wstring title);
  std::wstring GetText() const;
  void SetText(std::wstring text);

  int GetDepth() const;
  void SetDepth(int depth);
  void SetParentPtr(DocNode* parentPtr);
  DocNode* GetParentPtr() const;

  void AddImagePath(std::string imagePath);

  void AddForSubNodes(DocNode subNode);
  DocNode* GetLastSubNodePtr();

   std::vector<DocNode>& GetSubNodes() { return subNodes; }

  void AddForContentAreas(ContentArea contentArea);

  std::vector<ContentArea> GetRect() const;

  void AppendText(std::wstring text);
  std::string OutputTree();
};

#endif // DOCNODE