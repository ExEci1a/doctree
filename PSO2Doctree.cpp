#include "public/pso/2doctree/PSO2Doctree.h"
#include "PDFToDoctree.h"

PSO2Doctree::PSO2Doctree(std::string filePath, std::string outPath, std::string password, int options) {
    this->analyzer = std::make_unique<PDFToDoctree>(filePath, outPath, password, options);
}

PSO2Doctree::~PSO2Doctree() {
    // do nothing
}

void PSO2Doctree::StartAnalyze() {
    this->analyzer->Analyze();
}

void PSO2Doctree::OutputDoctreeJson() {
    this->analyzer->OutputDoctreeJson();
}