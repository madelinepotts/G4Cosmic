#include "OutputConfig.hh"

#include <algorithm>

G4String OutputConfig::fileName_ = "g4cosmic.root";
G4bool OutputConfig::writeTrackEnd_ = false;
std::vector<G4String> OutputConfig::sensitiveVolumeNames_;

void OutputConfig::SetFileName(const G4String& fileName) {
    if (!fileName.empty()) fileName_ = fileName;
}
const G4String& OutputConfig::GetFileName() { return fileName_; }

void OutputConfig::SetWriteTrackEnd(G4bool enabled) { writeTrackEnd_ = enabled; }
G4bool OutputConfig::GetWriteTrackEnd() { return writeTrackEnd_; }

void OutputConfig::ClearSensitiveVolumeNames() {
    sensitiveVolumeNames_.clear();
}

void OutputConfig::RegisterSensitiveVolumeName(const G4String& logicalVolumeName) {
    if (logicalVolumeName.empty()) {
        return;
    }

    const auto it = std::find(
        sensitiveVolumeNames_.begin(),
        sensitiveVolumeNames_.end(),
        logicalVolumeName);
    if (it == sensitiveVolumeNames_.end()) {
        sensitiveVolumeNames_.push_back(logicalVolumeName);
    }
}

const std::vector<G4String>& OutputConfig::GetSensitiveVolumeNames() {
    return sensitiveVolumeNames_;
}
