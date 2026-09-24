#include "OutputConfig.hh"

G4String OutputConfig::fileName_ = "g4cosmic.root";

void OutputConfig::SetFileName(const G4String& fileName) {
    if (!fileName.empty()) fileName_ = fileName;
}
const G4String& OutputConfig::GetFileName() { return fileName_; }
